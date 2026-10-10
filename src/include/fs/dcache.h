#ifndef DCACHE_H
#define DCACHE_H

#include <lib/list.h>
#include <fs/vfs.h>

struct dentry {
    struct inode *inode;
    struct dentry *parent;
    struct qstr name;
	struct mount *mounted_here;

    spinlock_t lock;
	atomic_t refcount;

    struct list_head hash;
    struct list_head childs;
	struct list_head siblings;

	struct list_head lru; // for cleanup
};

struct dentry *dcache_lookup(struct dentry *parent, const struct qstr *name);
struct dentry *dcache_add(struct dentry *parent, const struct qstr *name, struct inode *inode);
struct dentry* dentry_get(struct dentry *dentry);
void dentry_put(struct dentry *dentry);

#endif