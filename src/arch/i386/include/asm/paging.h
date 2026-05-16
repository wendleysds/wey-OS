#ifndef _PAGING_i386_H
#define _PAGING_i386_H

#define PGD_MAX_ENTRIES 1024
#define PTE_MAX_ENTRIES 1024

// Flags
#define _PAGE_GLOBAL   0x100
#define _PAGE_PSIZE    0x80
#define _PAGE_DIRTY    0x40
#define _PAGE_ACCESSED 0x20
#define _PAGE_PCD      0x10
#define _PAGE_PWT      0x8
#define _PAGE_US       0x4
#define _PAGE_RW       0x2
#define _PAGE_P        0x1

#define PAGE_MASK 0xFFFFF000
#define FLAGS_MASK (~PAGE_MASK)
#define PAGE_SHIFT 12
#define PGDIR_SHIFT 22

typedef struct { unsigned long val; } pte_t;
typedef struct { unsigned long val; } pmd_t;
typedef struct { unsigned long val; } pud_t;
typedef struct { unsigned long val; } pgd_t;

static inline void paging_load_table(unsigned long paddr){
	__asm__ volatile("mov %0, %%cr3\r\n" : : "r"(paddr) : "memory");
}

static inline unsigned long paging_current_table_phys(void){
	unsigned long ret;
	__asm__ volatile ("mov %%cr3, %0" : "=rm" (ret));
	return ret;
}

extern pgd_t initial_pgdir[PGD_MAX_ENTRIES];

extern const struct paging_ops arch_paging_ops;
extern const struct paging_format arch_paging_fmt;

#endif
