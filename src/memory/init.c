#include <wey/printk.h>
#include <wey/mmu.h>
#include <mm/page.h>
#include <mm/memory.h>
#include <mm/memblock.h>
#include <def/init.h>
#include <def/err.h>
#include <def/config.h>
#include <lib/string.h>

#include <asm/page.h>

#define PAGE_COUNT(size) (((size) + PAGE_SIZE - 1) / PAGE_SIZE)
#define ALIGN_UP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))
#define ALIGN_DOWN(value, alignment) ((value) & ~((alignment) - 1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

extern struct paging_ctx kernel_ctx;
struct paging_ctx* ctx = &kernel_ctx;

extern unsigned long max_pfn_mapped;

static struct page** sections __initdata;
static size_t max_sections __initdata;

static void __init paging_map_ram(void){
	uintptr_t max_phys =
		MIN(
			(uintptr_t)max_pfn << PAGE_SHIFT,
			KERNEL_DIRECTMAP_SIZE
		);

	uintptr_t last_phys_mapped = max_pfn_mapped << PAGE_SHIFT;

	for(size_t i = 0; i < memblock.memory.count; i++){
		struct memblock_region *r = &memblock.memory.regions[i];

		uintptr_t start = ALIGN_DOWN(r->base, PAGE_SIZE);
		uintptr_t end   = ALIGN_UP(r->base + r->size, PAGE_SIZE);

		if(start >= max_phys)
			continue;

		end = MIN(end, max_phys);

		if(last_phys_mapped != 0){
			if(end <= last_phys_mapped){
				continue;
			}

			// continue from where we left off
			start = MAX(start, last_phys_mapped);
		}

		for(uintptr_t phys = start; phys < end; phys += PAGE_SIZE){
			if(mmu_early_mmap(
				&kernel_ctx,
				__va(phys),
				phys,
				(MEM_READ | MEM_WRITE | MEM_KERNEL)
			)) {
				printk("Failed to map RAM at %#lx\n", phys);
				continue;
			}
		}
	}

	arch_paging_ops.flush_all();

	max_pfn_mapped = max_phys >> PAGE_SHIFT;

	printk("Memory: Max PFN mapped = %#lx/%#lx (max_phys=0x%p)\n", max_pfn_mapped, max_pfn, max_phys);
}

static __init int vmemmap_sections_init(void){
	max_sections = pfn_to_section_nr(max_pfn) + 1;
	const size_t total_mem = sizeof(struct page**) * max_sections;

	printk("VMEMMAP: max sections = %d\n", max_sections);
	sections = memblock_alloc(
		total_mem,
		__alignof__(struct page**)
	);

	if(!sections){
		return NO_MEMORY;
	}

	sections = (void*)__va(sections);

	for(size_t i = 0; i < max_sections; i++){
		sections[i] = NULL;
	}

	return OK;
}

static __init int vmemmap_populate(void) {
	const size_t section_pages = PFN_PER_SECTION;
	const size_t section_size  = section_pages * sizeof(struct page);
	const size_t map_size = ALIGN_UP(section_size, PAGE_SIZE);

	size_t total_pages = 0;

	for (size_t i = 0; i < memblock.memory.count; i++) {
		struct memblock_region *r = &memblock.memory.regions[i];

		size_t start_pfn = r->base >> PAGE_SHIFT;
		size_t end_pfn   = (r->base + r->size) >> PAGE_SHIFT;

		if (start_pfn >= max_pfn) {
			continue;
		}

		if(end_pfn > max_pfn) {
			end_pfn = max_pfn;
		}

		for (size_t pfn = start_pfn; pfn < end_pfn; ) {
			size_t sec = pfn_to_section_nr(pfn);
			if(sec >= max_sections) {
				printk("VMEMMAP: section %lu out of range (max %lu)\n", sec, max_sections);
				return OUT_OF_BOUNDS;
			}

			struct page *pages = sections[sec];

			if (!pages) {
				size_t sec_start = sec * PFN_PER_SECTION;
				size_t sec_end   = sec_start + PFN_PER_SECTION;

				size_t valid_start = MAX(sec_start, start_pfn);
				size_t valid_end   = MIN(sec_end, end_pfn);

				uintptr_t phys_meta = (uintptr_t)memblock_alloc(map_size, PAGE_SIZE);
				if (!phys_meta)
					return NO_MEMORY;

				uintptr_t vaddr_base =
					KERNEL_VMEMMAP_START +
					(sec_start * sizeof(struct page));

				size_t total = 0;
				for (size_t off = 0; off < map_size; off += PAGE_SIZE, total += PAGE_SIZE) {
					if(mmu_early_mmap(
						ctx,
						vaddr_base + off,
						phys_meta + off,
						(MEM_READ | MEM_WRITE | MEM_KERNEL)
					)) {
						return FAILED;
					}
				}

				pages = (struct page *)vaddr_base;

				memset(pages, 0, map_size);

				sections[sec] = pages;

				for (size_t i = 0; i < section_pages; i++) {
					size_t cur_pfn = sec_start + i;

					if (cur_pfn < valid_start || cur_pfn >= valid_end) {
						pages[i].flags = PAGE_RESERVED;
					}
					
					total_pages++;
				}
			}

			pfn = MIN(end_pfn, (sec + 1) * PFN_PER_SECTION) + map_size - map_size;
		}
	}

	size_t vmemmap_size = total_pages * sizeof(struct page);

	printk("VMEMMAP: start=0x%p;",
		KERNEL_VMEMMAP_START
	);

	printk(" end=0x%p\n", 
		KERNEL_VMEMMAP_START + vmemmap_size
	);

	printk("VMEMMAP: pages %u, mem %luMiB, metadata %luKiB\n",
		total_pages, (total_pages * PAGE_SIZE) / MiB(1), vmemmap_size / KiB(1)
	);

	return SUCCESS;
}

int __init memory_init(void) {
	int res = 0;

	paging_map_ram();

	if(IS_ERR_VALUE(res = vmemmap_sections_init())){
		return res;
	}

	if(IS_ERR_VALUE(res = vmemmap_populate())){
		return res;
	}

	// init buddy system
	// init slab
	// return SUCCESS;

	return INVALID_STATE; // Not fully implemented yet
}
