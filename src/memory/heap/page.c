#include <mm/page.h>
#include <mm/memblock.h>
#include <wey/spinlock.h>
#include <wey/printk.h>
#include <def/err.h>
#include <def/init.h>
#include <def/config.h>

#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define vmemmap ((struct page *)KERNEL_VMEMMAP_START)

struct free_area {
	struct list_head free_list;
	unsigned long nr_free;
};

struct zone {
	struct free_area free_area[MAX_ORDER];
	spinlock_t lock;

	unsigned long reserved_pages;
	unsigned long free_pages;
};

static struct zone global_zone;

static inline uintptr_t page_to_pfn(struct page *page){
	return (uintptr_t)(page - vmemmap);
}

static inline struct page *pfn_to_page(uintptr_t pfn){
	return &vmemmap[pfn];
}

struct page* phys_to_page(uintptr_t phys_addr) {
	return pfn_to_page(phys_addr >> PAGE_SHIFT);
}

uintptr_t page_to_phys(struct page* page) {
	return page_to_pfn(page) << PAGE_SHIFT;
}

uintptr_t page_to_virt(struct page* page) {
	uintptr_t phys = page_to_phys(page);
	return phys + KERNEL_DIRECTMAP_START;
}

struct page* virt_to_page(uintptr_t virt_addr) {
	uintptr_t phys = virt_addr - KERNEL_DIRECTMAP_START;
	return phys_to_page(phys);
}

static __init int pfn_is_reserved(unsigned long pfn) {
	struct page* page = pfn_to_page(pfn);

	if((page->flags & PG_RESERVED) || page->flags == PG_BAD) {
		return 1;
	}

	if(page->flags & PG_BUDDY) {
		return 1;
	}

	uint64_t addr = pfn << PAGE_SHIFT;
	return memblock_is_reserved(addr, PAGE_SIZE);
}

static __init int pfn_range_reserved(uintptr_t start, uintptr_t pages){
	for(uintptr_t pfn = start; pfn < start + pages; pfn++){
		if(pfn_is_reserved(pfn))
			return 1;
	}

	return 0;
}

int __init page_init(void){
	global_zone.reserved_pages = 0;
	global_zone.free_pages = 0;
	spinlock_init(&global_zone.lock);
	for(int i = 0; i < MAX_ORDER; i++) {
		INIT_LIST_HEAD(&global_zone.free_area[i].free_list);
		global_zone.free_area[i].nr_free = 0;
	}

	struct memblock_type* mem = &memblock.memory;
	for (int i = 0; i < mem->count; i++) {
		uintptr_t start_pfn = ALIGN_UP(mem->regions[i].base, PAGE_SIZE) >> PAGE_SHIFT;
		uintptr_t end_pfn = (mem->regions[i].base + mem->regions[i].size) >> PAGE_SHIFT;

		if (start_pfn >= max_pfn) {
			continue;
		}

		if(end_pfn > max_pfn) {
			end_pfn = max_pfn;
		}

		if (start_pfn >= max_pfn) {
			continue;
		}

		for (uintptr_t pfn = start_pfn; pfn < end_pfn; ) {
			if (pfn_is_reserved(pfn)) {
				struct page* page = pfn_to_page(pfn);
				page->flags = PG_RESERVED;
				global_zone.reserved_pages++;
				pfn++;
				continue;
			}

			uint8_t order = 0;
			while(order < MAX_ORDER - 1){
				uintptr_t pages = 1UL << (order + 1);

				if(pfn + pages > end_pfn)
					break;

				if(pfn & (pages - 1))
					break;

				if(pfn_range_reserved(pfn, pages))
					break;

				order++;
			}

			uintptr_t num_pages = 1UL << order;
			struct page* page = pfn_to_page(pfn);
			page->order = order;
			page->flags = PG_BUDDY;
			atomic_set(&page->refcount, 0);
			INIT_LIST_HEAD(&page->list);

			list_add(&page->list, &global_zone.free_area[order].free_list);
			global_zone.free_area[order].nr_free++;
			global_zone.free_pages += num_pages;

			pfn += num_pages;
		}
	}

	printk("Buddy: Initialized with %lu managed pages\n",
		global_zone.free_pages + global_zone.reserved_pages
	);

	printk("Buddy: Pages: free=%lu reserved=%lu\n",
		global_zone.free_pages, global_zone.reserved_pages
	);

	return SUCCESS;
}

struct page* page_alloc(uint8_t order, uint16_t flags) {
	spin_lock(&global_zone.lock);

	for (uint8_t cur = order; cur < MAX_ORDER; cur++) {
		struct free_area *area = &global_zone.free_area[cur];

		if (list_empty(&area->free_list))
			continue;

		struct page *page = list_entry(
			area->free_list.next, struct page, list
		);

		list_remove(&page->list);
		INIT_LIST_HEAD(&page->list);
		area->nr_free--;

		while (cur > order) {
			cur--;

			struct page *buddy = &page[1UL << cur];

			INIT_LIST_HEAD(&buddy->list);

			buddy->flags = PG_BUDDY;
			buddy->order = cur;

			atomic_set(&buddy->refcount, 0);

			list_add(
				&buddy->list,
				&global_zone.free_area[cur].free_list
			);

			global_zone.free_area[cur].nr_free++;
		}

		page->flags = flags;
		page->order = order;

		atomic_set(&page->refcount, 1);
		global_zone.free_pages -= (1UL << order);

		spin_unlock(&global_zone.lock);
		return page;
	}

	spin_unlock(&global_zone.lock);
	return NULL;
}

int page_free(struct page *page){
	if (!page)
		return INVALID_ARG;

	spin_lock(&global_zone.lock);

	int res = SUCCESS;

	if (page->flags & PG_RESERVED){
		res = INVALID_STATE;
		goto out;
	}

	if (atomic_read(&page->refcount) > 1){
		res = INVALID_STATE;
		goto out;
	}

	if (page->flags & PG_BUDDY){
		res = INVALID_STATE;
		goto out;
	}

	uintptr_t pfn = page_to_pfn(page);
	uint8_t order = page->order;
	uint8_t orig_order = order;

	page->flags = PG_BUDDY;
	atomic_set(&page->refcount, 0);

	while (order < MAX_ORDER - 1) {
		uintptr_t buddy_pfn = pfn ^ (1UL << order);

		if (buddy_pfn >= max_pfn)
			break;

		struct page *buddy = pfn_to_page(buddy_pfn);

		if (!(buddy->flags & PG_BUDDY) || (buddy->order != order))
			break;

		list_remove(&buddy->list);
		global_zone.free_area[order].nr_free--;

		buddy->flags &= ~PG_BUDDY; 
		buddy->order = 0;
		INIT_LIST_HEAD(&buddy->list);

		pfn &= ~(1UL << order); 
		order++;
	}

	struct page *merged = pfn_to_page(pfn);

	INIT_LIST_HEAD(&merged->list);
	merged->flags = PG_BUDDY;
	merged->order = order;
	atomic_set(&merged->refcount, 0);

	list_add(&merged->list, &global_zone.free_area[order].free_list);

	global_zone.free_area[order].nr_free++;
	global_zone.free_pages += (1UL << orig_order);

out:
	spin_unlock(&global_zone.lock);
	return res;
}

void __page_add_memory(uintptr_t vaddr, size_t size){
	uintptr_t start = ALIGN_UP((uintptr_t)vaddr, PAGE_SIZE);
    uintptr_t end = ALIGN_DOWN(start + size, PAGE_SIZE);
    size_t pages_freed = 0;

	if (size > UINTPTR_MAX - start) {
		return; // overflow
	}

	for (uintptr_t addr = start; addr < end; addr += PAGE_SIZE) {
        struct page *page = virt_to_page(addr);

        page->flags = 0; 
        page->order = 0;
        atomic_set(&page->refcount, 0);

        if (page_free(page) == SUCCESS) {
            pages_freed++;
        }
    }

	printk("Buddy: Added \"%lx\" pages.\n", pages_freed);
}
