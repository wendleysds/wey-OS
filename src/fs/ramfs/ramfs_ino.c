#include <fs/vfs.h>
#include <fs/dcache.h>
#include <fs/stat.h>
#include <lib/string.h>
#include <lib/list.h>
#include <sync/spinlock.h>
#include <def/errno.h>
#include <def/config.h>
#include <mm/page.h>	

#include <uapi/sys/stat.h>

#include "ramfs_internal.h"

static struct ramfs_dirent *ramfs_find_dirent(struct ramfs_inode *dir, const struct qstr *n) {
	struct ramfs_dirent *de;
	list_for_each_entry(de, &dir->children, sibling) {
		if (strlen(de->name) == n->len && strncmp(de->name, n->name, n->len) == 0)
			return de;
	}
	return NULL;
}

static struct ramfs_dirent *ramfs_dirent_new(const struct qstr *n, struct inode *ino) {
	struct ramfs_dirent *de = kzalloc(sizeof(*de));
	if (!de)
		return NULL;

	de->name = kmalloc(n->len + 1);
	if (!de->name) {
		kfree(de);
		return NULL;
	}

	memcpy(de->name, n->name, n->len);
	de->name[n->len] = '\0';
	de->ino = ino;
	INIT_LIST_HEAD(&de->sibling);
	return de;
}

static void ramfs_dirent_free(struct ramfs_dirent *de) {
	kfree(de->name);
	kfree(de);
}

int ramfs_do_truncate(struct inode *inode, size_t new_size){
	size_t old_size = inode->size;
	if (new_size == old_size)
		return 0;

	inode->size = new_size;
	if (new_size < old_size)
		truncate_pagecache(inode->i_mapping, (off_t)new_size);

	return 0;
}

struct inode* ramfs_lookup(struct inode *dir, struct dentry *dentry){
	struct ramfs_inode *rdir = dir->private_data;

	spin_lock(&rdir->spinlock);
	struct ramfs_dirent *de = ramfs_find_dirent(rdir, &dentry->name);
	struct inode *ino = de ? inode_get(de->ino) : NULL;
	spin_unlock(&rdir->spinlock);

	return ino;
}

static int ramfs_create_common(struct inode *dir, struct dentry *dentry, umode_t mode, uint8_t isDir, dev_t dev){
	struct ramfs_inode *rdir = dir->private_data;

	struct inode *inode = inode_new(dir->i_sb);
	if (!inode) return -ENOMEM;

	struct ramfs_dirent *de = ramfs_dirent_new(&dentry->name, inode);
	if (!de) {
		inode_put(inode);
		return -ENOMEM;
	}

	inode->i_op = &ramfs_iops;
	inode->i_mapping->a_ops = &ramfs_aops;
	inode->mode = mode;
	inode->dev = dev;

	if (S_ISDIR(mode))
		inode->i_fop = &ramfs_dir_fops;
	else if (S_ISREG(mode))
		inode->i_fop = &ramfs_file_fops;
	else
		inode_init_special(inode, mode, dev);

	spin_lock(&rdir->spinlock);

	if (ramfs_find_dirent(rdir, &dentry->name)) {
		spin_unlock(&rdir->spinlock);
		ramfs_dirent_free(de);
		inode_put(inode);
		return -EEXIST;
	}

	list_add(&de->sibling, &rdir->children);
	spin_unlock(&rdir->spinlock);

	return SUCCESS;
}

static int ramfs_remove_common(struct inode *dir, struct dentry *dentry, uint8_t isFile){
	struct ramfs_inode *rdir = dir->private_data;

	spin_lock(&rdir->spinlock);

	struct ramfs_dirent *de = ramfs_find_dirent(rdir, &dentry->name);
	if (!de) { spin_unlock(&rdir->spinlock); return -ENOENT; }

	struct inode *ino = de->ino;
	bool is_dir = S_ISDIR(ino->mode);

	if (isFile && is_dir)   { spin_unlock(&rdir->spinlock); return -EISDIR;  }
	if (!isFile && !is_dir) { spin_unlock(&rdir->spinlock); return -ENOTDIR; }

	if (is_dir) {
		struct ramfs_inode *crino = ino->private_data;
		spin_lock(&crino->spinlock);
		bool empty = list_empty(&crino->children);
		spin_unlock(&crino->spinlock);
		if (!empty) { spin_unlock(&rdir->spinlock); return -ENOTEMPTY; }
	}

	list_remove(&de->sibling);
	spin_unlock(&rdir->spinlock);

	spin_lock(&ino->lock);
	ino->nlink--;
	spin_unlock(&ino->lock);

	ramfs_dirent_free(de);
	inode_put(ino);
	return SUCCESS;
}

static int ramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev){
	return ramfs_create_common(dir, dentry, mode, S_ISDIR(mode), dev);
}

static int ramfs_create(struct inode *dir, struct dentry *dentry, umode_t mode){
	if(S_ISDIR(mode)) return -EINVAL;
	return ramfs_create_common(dir, dentry, mode | S_IFREG, 0, 0);
}

static int ramfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode){
	return ramfs_create_common(dir, dentry, S_IFDIR | mode, 1, 0);
}

static int ramfs_unlink(struct inode *dir, struct dentry *dentry){
	return ramfs_remove_common(dir, dentry, 1);
}

static int ramfs_rmdir(struct inode *dir, struct dentry *dentry){
	return ramfs_remove_common(dir, dentry, 0);
}

static int ramfs_rename(
	struct inode *old_dir, struct dentry *old_dentry,
	struct inode *new_dir, struct dentry *new_dentry
){
	struct ramfs_inode *old_rdir = old_dir->private_data;
	struct ramfs_inode *new_rdir = new_dir->private_data;
	struct ramfs_dirent *src, *dst, *dead = NULL;
	struct inode *victim = NULL;
	int err = 0;

	char *name = kmalloc(new_dentry->name.len + 1);
	if (!name) return -ENOMEM;

	memcpy(name, new_dentry->name.name, new_dentry->name.len);
	name[new_dentry->name.len] = '\0';

	struct ramfs_inode *a = old_rdir, *b = new_rdir;
	if (a > b) { struct ramfs_inode *t = a; a = b; b = t; }
	spin_lock(&a->spinlock);
	if (a != b) spin_lock(&b->spinlock);

	src = ramfs_find_dirent(old_rdir, &old_dentry->name);
	if (!src) { err = -ENOENT; goto out; }

	dst = ramfs_find_dirent(new_rdir, &new_dentry->name);

	if (dst && dst->ino == src->ino)
		goto out;

	if (dst) {
		bool sd = S_ISDIR(src->ino->mode);
		bool dd = S_ISDIR(dst->ino->mode);

		if (sd && !dd) { err = -ENOTDIR;  goto out; }
		if (!sd && dd) { err = -EISDIR;   goto out; }

		if (dd) {
			struct ramfs_inode *drino = dst->ino->private_data;
			if (!list_empty(&drino->children)) { err = -ENOTEMPTY; goto out; }
		}

		list_remove(&dst->sibling);
		victim = dst->ino;
		dead = dst;
	}

	char *old = src->name;
	src->name = name;
	name = old;

	if (old_rdir != new_rdir) {
		list_remove(&src->sibling);
		list_add(&src->sibling, &new_rdir->children);
	}

out:
	if (a != b) spin_unlock(&b->spinlock);
	spin_unlock(&a->spinlock);

	kfree(name);

	if (dead) {
		spin_lock(&victim->lock);
		victim->nlink--;
		spin_unlock(&victim->lock);

		ramfs_dirent_free(dead);
		inode_put(victim);
	}

	return err;
}

static int ramfs_getattr(struct inode *ino, struct stat* restrict statbuf){
	statbuf->ino = ino->ino;
	statbuf->size = ino->size;
	statbuf->blocks = ino->blocks;

	statbuf->dev = ino->dev;
	statbuf->nlink = ino->nlink;

	statbuf->mode = ino->mode;

	statbuf->atim.tv_sec = ino->access_time_sec;
	statbuf->ctim.tv_sec = ino->create_time_sec;
	statbuf->mtim.tv_sec = ino->modify_time_sec;

	return OK;
}

static int ramfs_setarrt(struct inode *ino, struct iattr* attr){
	if(attr->valid & ATTR_MODE)
		ino->mode = attr->mode;

	if(attr->valid & ATTR_SIZE){
		ramfs_do_truncate(ino, attr->size);
	}

	if(attr->valid & ATTR_ATIME)
		ino->modify_time_sec = attr->mtim.tv_sec;

	if(attr->valid & ATTR_MTIME)
		ino->access_time_sec = attr->atim.tv_sec;

	if(attr->valid & ATTR_CTIME)
		ino->create_time_sec = attr->ctim.tv_sec;

	return OK;
}

static int ramfs_link(struct dentry *old, struct inode *dir, struct dentry *new){
	struct inode *ino = old->inode;
	struct ramfs_inode *rdir = dir->private_data;

	if (S_ISDIR(ino->mode))     return -EPERM;
	if (ino->i_sb != dir->i_sb) return -EXDEV;

	struct ramfs_dirent *de = ramfs_dirent_new(&new->name, ino);
	if (!de) return -ENOMEM;

	spin_lock(&rdir->spinlock);

	if (ramfs_find_dirent(rdir, &new->name)) {
		spin_unlock(&rdir->spinlock);
		ramfs_dirent_free(de);
		return -EEXIST;
	}

	spin_lock(&ino->lock);
	if (ino->nlink == 0) {
		spin_unlock(&ino->lock);
		spin_unlock(&rdir->spinlock);
		ramfs_dirent_free(de);
		return -ENOENT;
	}
	ino->nlink++;
	inode_get(ino);
	spin_unlock(&ino->lock);

	list_add(&de->sibling, &rdir->children);
	spin_unlock(&rdir->spinlock);

	return SUCCESS;
}

const struct inode_operations ramfs_iops = {
	.lookup = ramfs_lookup,
	.create = ramfs_create,
	.rename = ramfs_rename,
	.unlink = ramfs_unlink,
	.mkdir = ramfs_mkdir,
	.rmdir = ramfs_rmdir,
	.getattr = ramfs_getattr,
	.setattr = ramfs_setarrt,
	.mknod = ramfs_mknod
};
