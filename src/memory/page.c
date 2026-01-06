#include <def/config.h>
#include <def/init.h>
#include <def/err.h>
#include <lib/list.h>
#include <lib/string.h>
#include <asm/paging.h>
#include <uapi/headers.h>
#include <mm/page.h>

#define _FPAGE_FREE     0x00
#define _FPAGE_ALLOCADE 0x01
#define _FPAGE_FIRST    0x02
#define _FPAGE_NEXT     0x04

/* Allocation flag (free, used, next, ...)*/
#define PAGE_FLAG_ALLOC_MASK  0x0F
/* Page memory flag (kernel, slab, user...)*/
#define PAGE_FLAG_TYPE_MASK 0xF0

static struct page_metadata metadata;
static size_t last_free_idx_hint;

static uintptr_t bump_current __initdata;
static uintptr_t bump_end __initdata;

// Bump/Buddy/Arena alloc to reserve memory for pages array
static void bump_alloc_init(uintptr_t start_addr, uintptr_t end_addr){
	bump_current = start_addr;
	bump_end = end_addr;
}

static void* bumb_alloc(size_t size){
	if (size == 0) return NULL;

	size = ALIGN(size, sizeof(void*));

	if (bump_current + size > bump_end) return NULL;

	void* addr = (void*)bump_current;
	bump_current += size;

	return addr;
}

static inline uintptr_t align_up(uintptr_t val){
    return (val + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
}

static inline uintptr_t align_down(uintptr_t val){
    return val & ~(PTE_PAGE_SIZE - 1);
}

static inline uint8_t ptr_aligned(uintptr_t ptr){
	return (ptr & (PTE_PAGE_SIZE - 1)) == 0;
}

struct page* addr_to_page(uintptr_t addr){
	if(addr < metadata.start_addr || addr > metadata.end_addr)
		return NULL;

	size_t idx = (addr - metadata.start_addr) / PTE_PAGE_SIZE;
	return &metadata.pages[idx];
}

static inline size_t page_idx(struct page* page){
	return ((uintptr_t)page - (uintptr_t)metadata.pages) / sizeof(struct page);
}

uintptr_t page_to_addr(struct page* page){
	return metadata.start_addr + (page_idx(page) * PTE_PAGE_SIZE);
}

struct page_metadata* __init page_init(struct e820_entry* table){
	size_t page_size_bytes = ALIGN(PAGE_SIZE_BYTES, PTE_PAGE_SIZE);
	size_t total_pages_mem = (page_size_bytes / PTE_PAGE_SIZE) + 1;
	size_t total_pages_arr = ALIGN(sizeof(struct page) * total_pages_mem, PTE_PAGE_SIZE) / PTE_PAGE_SIZE;

	uintptr_t _virt = PAGE_VIRT_ADDR;
	uintptr_t _phys = PAGE_PHYS_ADDR;

	for(uint32_t i = 0; i < total_pages_mem + total_pages_arr; i++){
		pgd_map(_virt, _phys, _PAGE_P | _PAGE_RW);
		_virt += PTE_PAGE_SIZE;
		_phys += PTE_PAGE_SIZE;
	}

	bump_alloc_init(PAGE_VIRT_ADDR, PAGE_VIRT_ADDR + page_size_bytes);
	metadata.pages = bumb_alloc(sizeof(struct page) * total_pages_mem);

	uintptr_t startReserved = align_up(bump_current);
	uintptr_t endReserved = align_down(startReserved + page_size_bytes);

	size_t num_pages = (endReserved - startReserved) / PTE_PAGE_SIZE;

	uint8_t found_region = 0;
	for (int i = 0; table[i].length != 0; i++) {
		if (table[i].type != 1){
			continue;
		}

		uintptr_t region_start = align_up(table[i].base_addr);
		uintptr_t region_end   = align_down(table[i].base_addr + table[i].length);

		if (region_end <= startReserved || region_start >= endReserved)
			continue;

		found_region = 1;

		if (region_start < startReserved)
			region_start = startReserved;

		if (region_end > endReserved)
			region_end = endReserved;

		for (size_t idx = 0; idx < num_pages; idx++) {
			//uintptr_t addr = region_start + idx * PTE_PAGE_SIZE;

			metadata.pages[idx].flags    = _FPAGE_FREE;
			metadata.pages[idx].refcount = 0;
			metadata.allocated_pages++;
		}
	}

	if(!found_region){
		return NULL;
	}

	metadata.start_addr = startReserved;
	metadata.end_addr = endReserved;
	metadata.pages_length = num_pages;
	metadata.allocated_pages = 0;

	last_free_idx_hint = 0;

	return &metadata;
}

struct page* page_alloc(size_t page_count, uint8_t flags){
	size_t consecutiveFreePages = 0;
	long start_idx = -1;
	struct page* page = NULL;

	size_t search_idx_start = last_free_idx_hint;
	size_t search_idx_end = metadata.pages_length;
	uint8_t tried_again = 0;

try_again:
	for (size_t i = search_idx_start; i < search_idx_end; i++){
		page = &metadata.pages[i];
		if ((page->flags & PAGE_FLAG_ALLOC_MASK) == _FPAGE_FREE){
			if(start_idx == -1)
				start_idx = i;

			consecutiveFreePages++;
			if(consecutiveFreePages == page_count)
				break;
		}else{
			consecutiveFreePages = 0;
			start_idx = -1;
		}
	}

	// If not enough consecutive pages found, reset
	if (consecutiveFreePages < page_count) {
		start_idx = -1;
	}

	// Not found!
	if(start_idx == -1){
		if(tried_again || last_free_idx_hint == 0)
			return NULL;

		tried_again = 1;
		search_idx_start = 0;
		search_idx_end = last_free_idx_hint;
		consecutiveFreePages = 0;
		last_free_idx_hint = 0; // Invalidade
		goto try_again;
	}

	for(size_t i = 0; i < page_count; i++){
		page = &metadata.pages[i + start_idx];
		memset(page, 0x0, sizeof(struct page));

		page->flags &= ~PAGE_FLAG_ALLOC_MASK;
		page->flags |= (flags & PAGE_FLAG_TYPE_MASK);
		page->flags |= _FPAGE_ALLOCADE;
		page->refcount = 1;
		
		if (i == 0)
			page->flags |= _FPAGE_FIRST;

		if (i + 1 < page_count)
			page->flags |= _FPAGE_NEXT;
	}

	last_free_idx_hint = start_idx + page_count;
	if (last_free_idx_hint >= metadata.pages_length)
		last_free_idx_hint = 0;

	metadata.allocated_pages += page_count;
	return &metadata.pages[start_idx];
}

struct page* page_alloc_zeroed(size_t page_count, uint8_t flags){
	struct page* page = page_alloc(page_count, flags);
	if(page){
		uintptr_t addr = page_to_addr(page);
		memset((void*)addr, 0, page_count * PTE_PAGE_SIZE);
	}

	return page;
}

int page_free(struct page* page){
	if(!page || 
		(uintptr_t)page > (uintptr_t)&metadata.pages[metadata.pages_length - 1] || 
		(uintptr_t)page < (uintptr_t)metadata.pages){
		return OUT_OF_BOUNDS;
	}

	if(!(page->flags & _FPAGE_FIRST))
		return INVALID_ARG;

	size_t idx = page_idx(page);
	size_t free_count = 0;
	for(size_t i = idx; i < metadata.pages_length; i++){
		struct page* cur_page = &metadata.pages[i];
		uint8_t flag = cur_page->flags & PAGE_FLAG_ALLOC_MASK;

		if(flag == _FPAGE_FREE) 
			break;
		if (!(flag & _FPAGE_ALLOCADE))
			break;

		cur_page->flags = _FPAGE_FREE;
		page->refcount = 0;
		free_count++;

		if(!(flag & _FPAGE_NEXT))
			break;
	}

	metadata.allocated_pages -= free_count;
	last_free_idx_hint = idx;
	return SUCCESS;
}
