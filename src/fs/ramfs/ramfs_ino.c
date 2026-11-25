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

static struct ramfs_inode *ramfs_find_child(struct ramfs_inode *dir, const struct qstr *n) {
	struct ramfs_inode *r;
	list_for_each_entry(r, &dir->children, sibling) {
		if (strlen(r->name) == n->len && strncmp(r->name, n->name, n->len) == 0)
			return r;
	}
	return NULL;
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
	const struct qstr *name = &dentry->name;

	spin_lock(&rdir->spinlock);

	struct ramfs_inode* rino = ramfs_find_child(rdir, name);
	if(rino){
		inode_get(rino->ino);
		spin_unlock(&rdir->spinlock);
		return rino->ino;
	}

	spin_unlock(&rdir->spinlock);
	return NULL;
}

static int ramfs_create_common(struct inode *dir, struct dentry *dentry, umode_t mode, uint8_t isDir, dev_t dev){
	struct ramfs_inode *rdir = dir->private_data;

	struct inode *existing = ramfs_lookup(dir, dentry);
	if(existing){
		inode_put(existing);
		return -EEXIST;
	}

	const struct qstr *name = &dentry->name;
	char tmp[name->len + 1];
	memcpy(tmp, name->name, name->len);
	tmp[name->len] = '\0';

	char* rino_name = strdup(tmp);
	if(!rino_name){
		return -ENOMEM;
	}

	struct inode *inode = inode_new(dir->i_sb);
	if(!inode){
		kfree(rino_name);
		return -ENOMEM;
	}

	struct ramfs_inode *rino = inode->private_data;

	rino->name = rino_name;
	rino->parent = rdir;
	inode->i_op = &ramfs_iops;
	inode->i_mapping->a_ops = &ramfs_aops;
	inode->mode = mode;
	inode->dev = dev;

	if(S_ISREG(mode) || S_ISDIR(mode))
		inode->i_fop = &ramfs_fops;
	else
		inode_init_special(inode, mode, dev);

	spin_lock(&rdir->spinlock);
	list_add(&rino->sibling, &rdir->children);
	spin_unlock(&rdir->spinlock);

	return SUCCESS;
}

static int ramfs_remove_common(struct inode *dir, struct dentry *dentry, uint8_t isFile){
	struct ramfs_inode *rdir = dir->private_data;
	const struct qstr *name = &dentry->name;

	spin_lock(&rdir->spinlock);

	struct ramfs_inode *rino;
	list_for_each_entry(rino, &rdir->children, sibling){
		if(strlen(rino->name) == name->len && strncmp(rino->name, name->name, name->len) == 0){
			if(S_ISDIR(rino->ino->mode) && isFile){
				spin_unlock(&rdir->spinlock);
				return -EINVAL;
			}

			if(S_ISDIR(rino->ino->mode)){
				if(!list_empty(&rino->children)){
					spin_unlock(&rdir->spinlock);
					return -ENOTEMPTY;
				}
			}

			list_remove(&rino->sibling);
			spin_unlock(&rdir->spinlock);

			inode_put(rino->ino);
			return SUCCESS;
		}
	}

	spin_unlock(&rdir->spinlock);
	return -ENOENT;
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
	struct ramfs_inode *src, *dst;
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

	src = ramfs_find_child(old_rdir, &old_dentry->name);
	if (!src) { err = -ENOENT; goto out; }

	dst = ramfs_find_child(new_rdir, &new_dentry->name);
	if (dst == src) goto out;

	if (dst) {
		bool sd = S_ISDIR(src->ino->mode), dd = S_ISDIR(dst->ino->mode);
		if (sd && !dd)                    { err = -ENOTDIR;   goto out; }
		if (!sd && dd)                    { err = -EISDIR;    goto out; }
		if (dd && !list_empty(&dst->children)) { err = -ENOTEMPTY; goto out; }
		list_remove(&dst->sibling);
		victim = dst->ino;
	}

	char *old = src->name;
	src->name = name;
	name = old;

	if (old_rdir != new_rdir) {
		list_remove(&src->sibling);
		list_add(&src->sibling, &new_rdir->children);
		src->parent = new_rdir;
	}

out:
	if (a != b) spin_unlock(&b->spinlock);
	spin_unlock(&a->spinlock);
	kfree(name);
	if (victim) inode_put(victim);
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
