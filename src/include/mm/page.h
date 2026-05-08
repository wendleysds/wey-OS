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

struct page {
	uint16_t flags;
	atomic_t refcount;
	union {
		void* private;
		struct slab* slab;
	};
} __attribute__((aligned(16)));

int page_init(void);

struct page* page_alloc(size_t page_count, uint16_t flags);
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
