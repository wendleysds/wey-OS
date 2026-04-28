#ifndef _PAGING_i386_H
#define _PAGING_i386_H

#include <stdint.h>

#define PGD_MAX_ENTRIES 1024
#define PTE_MAX_ENTRIES 1024
#define PTE_PAGE_SIZE 4096

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

struct page;

static inline void invlpg(void* virtaddr){
	__asm__ volatile("invlpg (%0)" : : "r"(virtaddr) : "memory");
}

pgd_t* pgd_current();

int pgd_load(pgd_t* pgd);

pgd_t* pgd_alloc();
void pgd_free(pgd_t* pgd);

int pgd_map(uintptr_t virtaddr, uintptr_t physaddr, int flags);
int pgd_remap(uintptr_t virtaddr, uintptr_t physaddr, int flags);
int pgd_unmap(uintptr_t virtaddr);

int pte_update_flags(uintptr_t virtaddr, int flags);
int pte_get_flags(uintptr_t virtaddr);

void* pgd_translate(void* virtaddr);

void pgd_copy(pgd_t* dest, pgd_t* src);
void pgd_copy_kernel(pgd_t* kernel_pgd, pgd_t* pgd);
void pgd_remove_kernel(pgd_t* pgd);

int pgd_dup_current(pgd_t* pgd, uint8_t copy_kernel_area);

#endif
