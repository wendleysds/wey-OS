#ifndef _PAGE_ALLOCATOR_H
#define _PAGE_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>
#include <wey/atomic.h>

#define PAGE_SLAB     0x01
#define PAGE_TABLE    0x02
#define PAGE_KERNEL   0x04
#define PAGE_USER     0x08
#define PAGE_COW      0x10
#define PAGE_RESERVED 0x20

struct slab;
struct e820_entry;
struct list_head;

struct page {
	uint16_t flags;
	atomic_t refcount;
	union {
		void* private;
		struct slab* slab;
	};
} __attribute__((aligned(16)));

struct page_metadata {
	uintptr_t start_addr;
	uintptr_t end_addr;

	struct page* pages;
	size_t allocated_pages;  /* Number of allocated pages */
	size_t pages_length; /* Pages array size */
};

struct page_metadata* page_init(struct e820_entry* table, size_t table_length);
struct page* page_alloc(size_t page_count, uint16_t flags);
struct page* page_alloc_zeroed(size_t page_count, uint16_t flags);
int page_free(struct page* page);

struct page* phys_to_page(uintptr_t phys_addr);
uintptr_t page_to_phys(struct page* page);

struct page* virt_to_page(uintptr_t virt_addr);
uintptr_t page_to_virt(struct page* page);

static inline void page_put(struct page* page){
	if(atomic_dec_and_test(&page->refcount)){
		page_free(page);
	}
}

static inline void page_get(struct page* page){
	atomic_inc(&page->refcount);
}

#endif
