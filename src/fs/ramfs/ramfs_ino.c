#include <fs/vfs.h>
#include <fs/stat.h>
#include <lib/string.h>
#include <lib/list.h>
#include <sync/spinlock.h>
#include <def/err.h>
#include <def/config.h>
#include <mm/page.h>	

#include "ramfs_internal.h"

int ramfs_do_truncate(struct inode *inode, size_t new_size){
	struct ramfs_inode *rino = inode->private_data;

	size_t old_size = inode->size;
	if(new_size == old_size){
		return 0;
	}

	size_t old_page_count = (old_size + PAGE_SIZE - 1) / PAGE_SIZE;
	size_t new_page_count = (new_size + PAGE_SIZE - 1) / PAGE_SIZE;

	/* Shrink */
	if(new_size < old_size){
		/* If new_size is page-aligned, new_page_count may equal old_page_count; handle partial page zeroing */
		if(new_page_count < old_page_count){

			if(new_page_count <= rino->page_capacity){
				/* free surplus pages */
				for(size_t i = new_page_count; i < old_page_count; i++){
					if(rino->pages[i]){
						page_free(rino->pages[i]);
						rino->pages[i] = NULL;
					}
				}
			}
		}

		/* Zero out tail of the last page if needed */
		if(new_size % PAGE_SIZE && new_page_count > 0){
			struct page *p = rino->pages[new_page_count - 1];
			if(p){
				char *ptr = (char*)page_to_virt(p);
				size_t off = new_size % PAGE_SIZE;
				memset(ptr + off, 0, PAGE_SIZE - off);
			}
		}
	} else { /* Expand */
		/* allocate needed pages */
		if(new_page_count > rino->page_capacity){

			size_t new_cap = rino->page_capacity ? rino->page_capacity * 2 : 4;
			while(new_cap < new_page_count) new_cap *= 2;

			int res = ramfs_grow_pages_arr(rino, new_cap);
			if(res) return res;
		}
	}

	inode->size = new_size;

	return 0;
}

struct inode* ramfs_lookup(struct inode *dir, struct qstr *name){
	struct ramfs_inode *rdir = dir->private_data;

	spin_lock(&rdir->spinlock);

	struct ramfs_inode* rino;
	list_for_each_entry(rino, &rdir->children, sibling){
		if(strlen(rino->name) == name->len && strncmp(rino->name, name->name, name->len) == 0){
			inode_get(rino->ino);
			spin_unlock(&rdir->spinlock);
			return rino->ino;
		}
	}

	spin_unlock(&rdir->spinlock);
	return NULL;
}

static int ramfs_create_common(struct inode *dir, struct qstr *name, uint16_t mode, uint8_t isDir){
	struct ramfs_inode *rdir = dir->private_data;

	struct inode *existing = ramfs_lookup(dir, name);
	if(existing){
		inode_put(existing);
		return FILE_EXISTS;
	}

	char tmp[name->len + 1];
	strncpy(tmp, name->name, name->len);
	tmp[name->len] = '\0';

	char* rino_name = strdup(tmp);
	if(!rino_name){
		return NO_MEMORY;
	}

	struct inode *inode = inode_new(dir->i_sb);
	if(!inode){
		kfree(rino_name);
		return NO_MEMORY;
	}

	struct ramfs_inode *rino = inode->private_data;

	rino->name = rino_name;
	rino->parent = rdir;
	rino->isDir = isDir;
	inode->mode = mode;
	inode->i_op = &ramfs_iops;
	inode->i_fop = &ramfs_fops;

	spin_lock(&rdir->spinlock);
	list_add(&rino->sibling, &rdir->children);
	spin_unlock(&rdir->spinlock);

	return SUCCESS;
}

static int ramfs_remove_common(struct inode *dir, struct qstr *name, uint8_t isFile){
	struct ramfs_inode *rdir = dir->private_data;

	spin_lock(&rdir->spinlock);

	struct ramfs_inode *rino;
	list_for_each_entry(rino, &rdir->children, sibling){
		if(strlen(rino->name) == name->len && strncmp(rino->name, name->name, name->len) == 0){
			if(rino->isDir && isFile){
				spin_unlock(&rdir->spinlock);
				return INVALID_FILE;
			}

			if(rino->isDir){
				if(!list_empty(&rino->children)){
					spin_unlock(&rdir->spinlock);
					return DIR_NOT_EMPTY;
				}
			}

			list_remove(&rino->sibling);
			spin_unlock(&rdir->spinlock);

			inode_put(rino->ino);
			return SUCCESS;
		}
	}

	spin_unlock(&rdir->spinlock);
	return FILE_NOT_FOUND;
}


static int ramfs_create(struct inode *dir, struct qstr *name, uint16_t mode){
	return ramfs_create_common(dir, name, mode, 0);
}

static int ramfs_mkdir(struct inode *dir, struct qstr *name){
	return ramfs_create_common(dir, name, S_IFDIR, 1);
}

static int ramfs_unlink(struct inode *dir, struct qstr *name){
	return ramfs_remove_common(dir, name, 1);
}

static int ramfs_rmdir(struct inode *dir, struct qstr *name){
	return ramfs_remove_common(dir, name, 0);
}

static int ramfs_getattr(struct inode *ino, struct stat* restrict statbuf){
	statbuf->ino = ino->ino;
	statbuf->mode = ino->mode;

	statbuf->size = ino->size;
	statbuf->dev = ino->dev;

	statbuf->atime = ino->atime_sec;
	statbuf->ctime = ino->ctime_sec;
	statbuf->mtime = ino->mtime_sec;

	return OK;
}

static int ramfs_setarrt(struct inode *ino, struct iattr* attr){
	struct stat* st = &attr->stat;

	if(attr->valid & ATTR_MODE)
		ino->mode = st->mode;

	if(attr->valid & ATTR_SIZE){
		ramfs_do_truncate(ino, st->size);
	}

	if(attr->valid & ATTR_DEV)
		ino->dev = st->dev;

	if(attr->valid & ATTR_ATIME)
		ino->mtime_sec = st->mtime;

	if(attr->valid & ATTR_MTIME)
		ino->atime_sec = st->atime;

	if(attr->valid & ATTR_CTIME)
		ino->ctime_sec = st->ctime;

	return OK;
}

static int ramfs_mknod(struct inode *dir, struct qstr *name, uint16_t mode, dev_t dev){
	return NOT_IMPLEMENTED;
}

const struct inode_operations ramfs_iops = {
	.lookup = ramfs_lookup,
	.create = ramfs_create,
	.unlink = ramfs_unlink,
	.mkdir = ramfs_mkdir,
	.rmdir = ramfs_rmdir,
	.getattr = ramfs_getattr,
	.setarrt = ramfs_setarrt,
	.mknod = ramfs_mknod
};
