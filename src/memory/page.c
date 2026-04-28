#include <def/config.h>
#include <def/linker.h>
#include <def/init.h>
#include <def/err.h>
#include <lib/list.h>
#include <lib/string.h>
#include <asm/page.h>
#include <asm/paging.h>
#include <uapi/headers.h>
#include <mm/page.h>
#include <mm/earlyalloc.h>

#define _FPAGE_FREE     0x0000
#define _FPAGE_ALLOCADE 0x1000
#define _FPAGE_FIRST    0x2000
#define _FPAGE_NEXT     0x4000

/* Allocation flag (free, used, next, ...)*/
#define PAGE_FLAG_ALLOC_MASK  0xF000
/* Page memory flag (kernel, slab, user...)*/
#define PAGE_FLAG_TYPE_MASK 0x0FFF

static struct page_metadata metadata;
static size_t last_free_idx_hint;

static inline uintptr_t align_up(uintptr_t val){
    return (val + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static inline uintptr_t align_down(uintptr_t val){
    return val & ~(PAGE_SIZE - 1);
}

static inline uint8_t ptr_aligned(uintptr_t ptr){
	return (ptr & (PAGE_SIZE - 1)) == 0;
}

static inline size_t page_idx(struct page* page){
	return ((uintptr_t)page - (uintptr_t)metadata.pages) / sizeof(struct page);
}

struct page* phys_to_page(uintptr_t phys_addr){
	if(phys_addr < metadata.start_addr || phys_addr >= metadata.end_addr)
		return NULL;

	size_t idx = (phys_addr - metadata.start_addr) / PAGE_SIZE;
	return &metadata.pages[idx];
}

uintptr_t page_to_phys(struct page* page){
	return metadata.start_addr + (page_idx(page) * PAGE_SIZE);
}

struct page* virt_to_page(uintptr_t virt_addr){
	uintptr_t phys_addr = __pa(virt_addr);
	return phys_to_page(phys_addr);
}

uintptr_t page_to_virt(struct page* page){
	uintptr_t phys_addr = page_to_phys(page);
	return __va(phys_addr);
}

struct page_metadata* __init page_init(struct e820_entry* table, size_t table_length){
	struct e820_entry* region = NULL;

	uintptr_t page_phys_start = __pa(0x0);
	page_phys_start = align_up(page_phys_start);

	for (size_t i = 0; i < table_length; i++) {
		if (table[i].type != 1) 
			continue;

		uintptr_t region_start = align_up(table[i].base_addr);
		uintptr_t region_end   = align_down(region_start + table[i].length);

		if (region_end <= page_phys_start) 
			continue;

		region = &table[i];
		break;
	}

	if(!region)
		return ERR_PTR(NOT_FOUND);

	uintptr_t phys_start = page_phys_start;
	uintptr_t phys_end   = align_down(region->base_addr + region->length);

	if (phys_end <= phys_start)
		return ERR_PTR(NO_MEMORY);

	size_t total_pages = (phys_end - phys_start) / PAGE_SIZE;

	metadata.pages = (void*)early_alloc(sizeof(struct page) * total_pages);
	if(!metadata.pages){
		return ERR_PTR(NO_MEMORY);
	}

	for(size_t i = 0; i < total_pages; i++){
		struct page* page = &metadata.pages[i];
		page->flags = _FPAGE_FREE;
		atomic_set(&page->refcount, 1);
		page->slab = NULL;
	}

	metadata.start_addr = phys_start;
	metadata.end_addr = phys_start + total_pages * PAGE_SIZE;
	metadata.pages_length = total_pages;
	metadata.allocated_pages = 0;

	last_free_idx_hint = 0;

	return &metadata;
}

struct page* page_alloc(size_t page_count, uint16_t flags){
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
		page->flags &= ~PAGE_FLAG_ALLOC_MASK;
		page->flags |= (flags & PAGE_FLAG_TYPE_MASK);
		page->flags |= _FPAGE_ALLOCADE;
		atomic_set(&page->refcount, 1);
		
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

struct page* page_alloc_zeroed(size_t page_count, uint16_t flags){
	struct page* page = page_alloc(page_count, flags);
	if(page){
		uintptr_t addr = page_to_virt(page);
		memset((void*)addr, 0, page_count * PAGE_SIZE);
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

	if(((uintptr_t)page & (sizeof(struct page) - 1)) != 0)
		return BAD_ALIGNMENT;

	size_t idx = page_idx(page);
	size_t free_count = 0;
	for(size_t i = idx; i < metadata.pages_length; i++){
		struct page* cur_page = &metadata.pages[i];
		uint16_t flag = cur_page->flags & PAGE_FLAG_ALLOC_MASK;

		if(flag == _FPAGE_FREE) 
			break;
		if (!(flag & _FPAGE_ALLOCADE))
			break;

		cur_page->flags = _FPAGE_FREE;
		atomic_set(&page->refcount, 0);
		free_count++;

		if(!(flag & _FPAGE_NEXT))
			break;
	}

	metadata.allocated_pages -= free_count;
	last_free_idx_hint = idx;
	return SUCCESS;
}
