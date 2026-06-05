#ifndef _PAGE_ALLOCATOR_H
#define _PAGE_ALLOCATOR_H

#include <sync/atomic.h>
#include <def/compile.h>
#include <lib/list.h>
#include <stdint.h>

#define MAX_ORDER 11

// State
#define PG_BUDDY      (1U << 0)
#define PG_RESERVED   (1U << 1)
#define PG_BAD        (1U << 2)
// Ownership
#define PG_KERNEL     (1U << 3)
// Type
#define PG_SLAB       (1U << 4)
#define PG_TABLE      (1U << 5)
#define PG_HIGHMEM    (1U << 6)
#define PG_COMPOUND   (1U << 7)
// Runtime Flags
#define PG_DIRTY      (1U << 8)
#define PG_LOCKED     (1U << 9)
#define PG_REFERENCED (1U << 10)
#define PG_WRITEBACK  (1U << 11)

struct page {
	uint8_t order;
	uint8_t pad;

	uint16_t flags;
	atomic_t refcount;

	void* private;

	struct list_head list;
} __aligned(32);

int page_init(void);

struct page* page_alloc(uint8_t order, uint16_t flags);
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
