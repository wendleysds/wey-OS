#include "def/linker.h"
#include <stdint.h>
#include <wey/printk.h>
#include <mm/earlyalloc.h>
#include <lib/string.h>
#include <def/compile.h>
#include <def/init.h>
#include <def/config.h>

#include <asm/paging.h>
#include <asm/page.h>

#include <uapi/headers.h>

#define PAGE_COUNT(size) (((size) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGE_TABLE_SIZE(pages) ((pages) / PGD_MAX_ENTRIES)
#define LOWMEM_PAGES (((2Ull << 31) - PAGE_OFFSET) >> PAGE_SHIFT)

#define hlt() do { __asm__ volatile("hlt"); } while(1)

struct boot_header boot_header;

pgd_t initial_pgdir [PGD_MAX_ENTRIES] __aligned(PAGE_SIZE);

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

static __init __no_stack_protector pte_t init_map(pgd_t **pgdp, pte_t **ptep, pte_t pte, const unsigned long limit) {
	while ((pte.val & PAGE_MASK) < limit) {
		pgd_t pt = { .val = (unsigned long)*ptep | (_PAGE_P | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED) };

		**pgdp = pt;
		*(*pgdp + ((PAGE_OFFSET >> PGDIR_SHIFT))) = pt;

		for (int i = 0; i < 1024; i++) {
			**ptep = pte;
			pte.val += PAGE_SIZE;
			(*ptep)++;
		}

		(*pgdp)++;
	}

	return pte;
}

void __init __no_stack_protector mk_early_pgtbl_32(void){
	const unsigned long limit = __pa(_end) + (PAGE_TABLE_SIZE(LOWMEM_PAGES) << PAGE_SHIFT);

	pte_t pte = { .val = 0 | (_PAGE_P | _PAGE_RW) };

	pte_t *ptep = (pte_t *)__pa(__brk_base);
	pgd_t *pgdp = (pgd_t *)__pa(initial_pgdir);

	pte = init_map(&pgdp, &ptep, pte, limit);

	unsigned long *ptr = (unsigned long *)__pa(&max_pfn_mapped);
	*ptr = (pte.val & PAGE_MASK) >> PAGE_SHIFT;

	ptr = (unsigned long *)__pa(&_brk_end);
	*ptr = (unsigned long)ptep + PAGE_OFFSET;
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

	printk("boot: heap: 0x%p - 0x%p (%u KiB)\n", __brk_base, _brk_end, (_brk_end - (size_t)__brk_base) / KiB(1));

	kmain();

	hlt();
}
