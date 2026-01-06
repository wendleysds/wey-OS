#ifndef _PAGE_ALLOCATOR_H
#define _PAGE_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SLAB     0x10
#define PAGE_KERNEL   0x20
#define PAGE_USER     0x40
#define PAGE_RESERVED 0x80

struct slab;
struct e820_entry;
struct list_head;

struct page {
	void* addr;
	int flags;
	short refcount;
	struct slab* slab;
};

struct page_metadata {
	uintptr_t start_addr;
	uintptr_t end_addr;

	struct page* pages;
	size_t allocated_pages;  /* Number of allocated pages */
	size_t pages_length; /* Pages array size */
};

struct page_metadata* page_init(struct e820_entry* table);
struct page* page_alloc(size_t page_count, uint8_t flags);
struct page* page_alloc_zeroed(size_t page_count, uint8_t flags);
int page_free(struct page* page);
struct page* addr_to_page(uintptr_t addr);
uintptr_t page_to_addr(struct page* page);

#endif
