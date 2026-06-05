#include <kernel/printk.h>
#include <mm/earlyalloc.h>
#include <lib/string.h>
#include <kernel/init.h>
#include <def/config.h>
#include <def/linker.h>

#include <asm/paging.h>
#include <asm/page.h>

#include <uapi/headers.h>

#define PAGE_COUNT(size) (((size) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGE_TABLE_SIZE(pages) ((pages) / PGD_MAX_ENTRIES)
#define LOWMEM_PAGES (((2Ull << 31) - PAGE_OFFSET) >> PAGE_SHIFT)

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

extern struct boot_header boot_header;

extern int terminal_early_init();
extern void kmain();

extern unsigned long max_pfn_mapped;
extern unsigned long _brk_end;

static __init __no_stack_protector pte_t init_map(pgd_t *pgd, pte_t pte, unsigned long limit) {
	uintptr_t addr = pte.val & PAGE_MASK;
	size_t pgd_index = 0;
	const size_t mirror_offset = PAGE_OFFSET >> PGDIR_SHIFT;

	while (addr < limit) {
		if (pgd_index >= PGD_MAX_ENTRIES)
			break;

		pte_t *pt = (pte_t*)early_alloc_boot(PAGE_SIZE, PAGE_SIZE);
		memset(pt, 0x0, PAGE_SIZE);

		pgd[pgd_index].val = (size_t)pt | (_PAGE_P | _PAGE_RW);

		if (pgd_index + mirror_offset < PGD_MAX_ENTRIES)
			pgd[pgd_index + mirror_offset] = pgd[pgd_index];

		for (int i = 0; i < PTE_MAX_ENTRIES && addr < limit; i++) {
			pt[i] = pte;

			pte.val += PAGE_SIZE;
			addr += PAGE_SIZE;
		}

		pgd_index++;
	}

	return pte;
}


void __init __no_stack_protector mk_early_pgtbl_32(void){
	const unsigned long limit = __pa(__end) + (PAGE_TABLE_SIZE(LOWMEM_PAGES) << PAGE_SHIFT);

	pte_t pte = { .val = 0 | (_PAGE_P | _PAGE_RW) };
	pgd_t *pgdp = (pgd_t *)ALIGN(__pa(initial_pgdir), PAGE_SIZE);

	pgdp[1023].val = ((size_t)pgdp & PAGE_MASK) | (_PAGE_P | _PAGE_RW);

	pte = init_map(pgdp, pte, limit);

	unsigned long *ptr = (unsigned long *)__pa(&max_pfn_mapped);
	*ptr = (pte.val & PAGE_MASK) >> PAGE_SHIFT;
}

void __regparm(1) __init __no_return i386_start_kernel(struct boot_header* boot_header_ptr){
	memcpy(&boot_header, boot_header_ptr, sizeof(struct boot_header));
	terminal_early_init();

	printk("Started i386 kernel\n");

	kmain();

	__builtin_unreachable();
}
