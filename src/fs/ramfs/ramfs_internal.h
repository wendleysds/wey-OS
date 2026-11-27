#ifndef _RAMFS_INTERNALS_H
#define _RAMFS_INTERNALS_H

#include <lib/list.h>
#include <sync/spinlock.h>
#include <stddef.h>

struct page;
struct inode;
struct inode_operations;
struct file_operations;
struct address_space_operations;

struct ramfs_dirent {
	char *name;
	struct inode *ino;
	struct list_head sibling;
};

struct ramfs_inode {
	struct inode *ino;
	spinlock_t spinlock;
	struct list_head children;
};

extern const struct inode_operations ramfs_iops;
extern const struct file_operations ramfs_file_fops;
extern const struct file_operations ramfs_dir_fops;
extern const struct address_space_operations ramfs_aops;

int ramfs_do_truncate(struct inode *inode, size_t new_size);

#endif