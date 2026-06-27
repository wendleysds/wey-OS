#include <mm/page.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <lib/string.h>

#include "ramfs_internal.h"

int ramfs_grow_pages_arr(struct ramfs_inode *rino, size_t new_cap){
	struct page** new_pages = kcalloc(sizeof(struct page*), new_cap);
	if(!new_pages){
		return -ENOMEM;
	}

	if(rino->pages){
		memcpy(new_pages, rino->pages, rino->page_capacity * sizeof(struct page*));
		kfree(rino->pages);
	}

	rino->pages = new_pages;
	rino->page_capacity = new_cap;

	return SUCCESS;
}