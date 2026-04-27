#include <wey/printk.h>
#include <mm/earlyalloc.h>
#include <lib/string.h>
#include <def/compile.h>
#include <def/init.h>
#include <def/config.h>

#include <asm/paging.h>
#include <asm/page.h>

#include <uapi/headers.h>

#define PAGE_COUNT(size) (((size) + PTE_PAGE_SIZE - 1) / PTE_PAGE_SIZE)
#define PAGE_TABLE_SIZE(pages) ((pages) / PGD_MAX_PTE)
#define LOWMEM_PAGES (((2Ull << 31) - PAGE_OFFSET) >> PAGE_SHIFT)

#define hlt() do { __asm__ volatile("hlt"); } while(1)

struct boot_header boot_header;

pgd_t initial_pgdir [PGD_MAX_PTE] __aligned(PTE_PAGE_SIZE);

extern int terminal_early_init();
extern void kmain();

extern unsigned long max_pfn_mapped;
extern unsigned long _brk_end;

static __init void e820_print_type(enum e820_type type)
{
	switch (type) {
		case E820_TYPE_RAM:	
			printk(" System RAM");
			break;
		case E820_TYPE_RESERVED:
			printk(" device reserved");
			break;
		case E820_TYPE_SOFT_RESERVED:
			printk(" soft reserved");
			break;
		case E820_TYPE_ACPI:
			printk(" ACPI data");
			break;
		case E820_TYPE_NVS:
			printk(" ACPI NVS");
			break;
		case E820_TYPE_UNUSABLE:
			printk(" unusable");
			break;
		case E820_TYPE_PMEM:
		case E820_TYPE_PRAM:
			printk(" persistent RAM (type %u)", type);
			break;
		default:
			printk(" type %u", type);
			break;
	}
}

static __init void e820_sort(struct e820_entry* table, uint32_t count){
    uint32_t i, j;

    for (i = 1; i < count; i++) {
        struct e820_entry key = table[i];
        j = i;

        while (j > 0 &&
               table[j - 1].base_addr > key.base_addr) {
            table[j] = table[j - 1];
            j--;
        }

        table[j] = key;
    }
}

static __init __no_stack_protector pte_t init_map(pgd_t *pgd, unsigned long limit) {
	uintptr_t addr = 0; 
	size_t pgd_index = 0; 
	const size_t mirror_offset = PAGE_OFFSET >> PGDIR_SHIFT; 

	while (addr < limit) {
		if (pgd_index >= PGD_MAX_PTE) {
			break;
		}

		pte_t *pt = (pte_t*)early_alloc_boot(PTE_PAGE_SIZE);
		memset(pt, 0x0, PTE_PAGE_SIZE);

		pgd[pgd_index] = (size_t)pt | (_PAGE_P | _PAGE_RW);

		if (pgd_index + mirror_offset < PGD_MAX_PTE) { 
			pgd[pgd_index + mirror_offset] = pgd[pgd_index]; 
		}

		for (int i = 0; i < PTE_MAX_ENTRIES && addr < limit; i++) {
			pt[i] = (addr & PAGE_MASK) | (_PAGE_P | _PAGE_RW);
			addr += PAGE_SIZE;
		}

		++pgd_index;
	}

	return addr;
}

void __init __no_stack_protector mk_early_pgtbl_32(void){
	pgd_t* pgd = (pgd_t*)__pa(initial_pgdir);
	memset(pgd, 0x0, sizeof(initial_pgdir));

	const unsigned long limit = __pa(_end) + (PAGE_TABLE_SIZE(LOWMEM_PAGES) << PAGE_SHIFT);

	init_map(pgd, limit);

	pgd[1023] = ((pgd_t)initial_pgdir & PAGE_MASK) | (_PAGE_P | _PAGE_RW);
}

void __regparm(1) __init __no_return i386_start_kernel(struct boot_header* boot_header_ptr){
	memcpy(&boot_header, boot_header_ptr, sizeof(struct boot_header));
	terminal_early_init();

	printk("Started i386 kernel\n");

	printk("Sorted E820\n");
	e820_sort(boot_header.e820_table, boot_header.e820_entries_count);

	printk("E820-provided physical RAM map:\n");
	uint64_t range_end_prev = 0;
	uint32_t idx;

	for (idx = 0; idx < boot_header.e820_entries_count; idx++) {
		struct e820_entry* entry = &boot_header.e820_table[idx];
		uint64_t range_start, range_end;

		range_start = entry->base_addr;

		if (entry->length > UINT64_MAX - entry->base_addr) {
			printk("E820 entry overflow!\n");
			continue;
		}

		range_end = entry->base_addr + entry->length;

		/* Out of order E820 maps should not happen: */
		if (range_start < range_end_prev)
			printk("out of order E820 entry!\n");

		if (range_start > range_end_prev) {
			printk("  [gap %#018llx-%#018llx]\n",
				range_end_prev,
				range_start-1);
		}

		printk("  [mem %#018llx-%#018llx] ", range_start, range_end-1);
		e820_print_type(entry->type);
		printk("\n");

		range_end_prev = range_end;
	}

	kmain();

	hlt();
}
