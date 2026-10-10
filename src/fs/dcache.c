#include <kernel/init.h>
#include <fs/dcache.h>
#include <def/errno.h>
#include <lib/string.h>

#define DCACHE_NR_HASHES 256

static struct list_head dcache_hash_table[DCACHE_NR_HASHES];
static spinlock_t dcache_lock;

static inline uint32_t dcache_hash(struct dentry *parent, const char *name, size_t len) {
    uint32_t hash = ((uintptr_t)parent >> 4) ^ 0x9e3779b9;
    for (size_t i = 0; i < len; i++) {
        hash = (hash * 31) + (uint8_t)name[i];
    }
    return hash;
}

struct dentry *dcache_lookup(struct dentry *parent, const struct qstr *name) {
    uint32_t hash = dcache_hash(parent, name->name, name->len);
    uint32_t bucket = hash % DCACHE_NR_HASHES;

    spin_lock(&dcache_lock);
    
    struct dentry *d;
    list_for_each_entry(d, &dcache_hash_table[bucket], hash) {
        if (d->parent == parent && d->name.len == name->len &&
            memcmp(d->name.name, name->name, name->len) == 0) {
            dentry_get(d);
            spin_unlock(&dcache_lock);
            return d;
        }
    }

    spin_unlock(&dcache_lock);
    return NULL;
}

struct dentry *dcache_add(struct dentry *parent, const struct qstr *name, struct inode *inode) {
    struct dentry *d = kmalloc(sizeof(struct dentry));
    if (!d) return NULL;

    char *name_copy = kmalloc(name->len + 1);
    if (!name_copy) {
        kfree(d);
        return NULL;
    }
    memcpy(name_copy, name->name, name->len);
    name_copy[name->len] = '\0';

    memset(d, 0, sizeof(struct dentry));
    d->inode = inode;
    d->parent = parent ? dentry_get(parent) : d;
    d->name.name = name_copy;
    d->name.len = name->len;
    
    atomic_set(&d->refcount, 1);
	spinlock_init(&d->lock);
    INIT_LIST_HEAD(&d->childs);
    INIT_LIST_HEAD(&d->siblings);
    INIT_LIST_HEAD(&d->lru);

    if (inode) {
        inode_get(inode);
    }

    spin_lock(&dcache_lock);
    if (parent && parent != d) {
        list_add_tail(&d->siblings, &parent->childs);
    }

    uint32_t bucket = dcache_hash(parent, name->name, name->len) % DCACHE_NR_HASHES;
    list_add(&d->hash, &dcache_hash_table[bucket]);
    spin_unlock(&dcache_lock);

    return d;
}

struct dentry *dentry_get(struct dentry *dentry) {
    if (dentry) {
        atomic_inc(&dentry->refcount);
    }
    return dentry;
}

void dentry_put(struct dentry *dentry) {
    if (!dentry) return;

    if (atomic_dec_and_test(&dentry->refcount)) {
        struct dentry *parent = dentry->parent;

        spin_lock(&dcache_lock);
        list_remove(&dentry->hash);

        if (parent && parent != dentry) {
            list_remove(&dentry->siblings);
        }
        spin_unlock(&dcache_lock);

        if (dentry->inode) {
            inode_put(dentry->inode);
        }

        if (parent && parent != dentry) {
            dentry_put(parent);
        }

        kfree((void *)dentry->name.name);
        kfree(dentry);
    }
}

static __init int dcache_init(void) {
    for (size_t i = 0; i < DCACHE_NR_HASHES; i++) {
        INIT_LIST_HEAD(&dcache_hash_table[i]);
    }
    
    spinlock_init(&dcache_lock);
    return 0;
}

fs_initcall(dcache_init);
