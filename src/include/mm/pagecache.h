#ifndef _PAGECACHE_H
#define _PAGECACHE_H

#include <sync/spinlock.h>
#include <lib/list.h>
#include <sys/types.h>
#include <mm/page.h>
#include <stdbool.h>
#include <stddef.h>

struct inode;
struct vm_region;
struct address_space;

struct address_space_operations {
	int (*readpage)(struct address_space *m, struct page *page);
	int (*writepage)(struct address_space *m, struct page *page);
};

struct address_space {
	struct inode *host;
	const struct address_space_operations *a_ops;

	spinlock_t lock;
	struct page **pages;
	size_t capacity;
	size_t nrpages;

	spinlock_t i_mmap_lock;
	struct list_head i_mmap; // inode`s VMAs MAP_SHARED (vm_region::shared)
};

void address_space_init(struct address_space *m, struct inode *host,
                        const struct address_space_operations *ops);

struct page *pagecache_lookup(struct address_space *m, pgoff_t idx);
struct page *pagecache_get_page(struct address_space *m, pgoff_t idx, bool create);
void pagecache_zero(struct page *page, size_t from, size_t to);

static inline void set_page_dirty(struct page *page)
{
	page->flags |= PG_DIRTY;
}

void truncate_pagecache(struct address_space *m, off_t new_size);
void pagecache_destroy(struct address_space *m);
int pagecache_writeback(struct address_space *m);
void unmap_mapping_range(struct address_space *m, pgoff_t first, pgoff_t end);

void i_mmap_add(struct address_space *m, struct vm_region *r);
void i_mmap_remove(struct address_space *m, struct vm_region *r);

#endif