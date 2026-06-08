#ifndef _RAMFS_INTERNALS_H
#define _RAMFS_INTERNALS_H

#include <lib/list.h>
#include <sync/spinlock.h>

struct page;
struct inode;

struct ramfs_inode {
	char *name;
	uint8_t isDir;
	struct page** pages; // file content blocks
	size_t page_capacity;

	struct ramfs_inode *parent;
	struct inode* ino;

	spinlock_t spinlock;

	struct list_head children;
	struct list_head sibling;
};

extern const struct inode_operations ramfs_iops;
extern const struct file_operations ramfs_fops;

int ramfs_grow_pages_arr(struct ramfs_inode *rino, size_t new_cap);
int ramfs_do_truncate(struct inode *inode, size_t new_size);

#endif