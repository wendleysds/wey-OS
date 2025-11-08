#ifndef _PAGING_i386_H
#define _PAGING_i386_H

#include <stdint.h>

#define PGD_MAX_PTE 1024
#define PTE_MAX_ENTRIES 1024
#define PTE_PAGE_SIZE 4096

// Flags
#define _PAGE_PCD 0x16
#define _PAGE_PWT 0x8
#define _PAGE_US  0x4
#define _PAGE_RW  0x2
#define _PAGE_P   0x1

#define PTE_FLAGS 0xFFFFF000

#define PAGE_MASK 0xFFFFF000
#define FLAGS_MASK 0x00000FFF

typedef unsigned long pte_t;
typedef unsigned long pgd_t;

extern pgd_t* _current_pgd;

static inline void invlpg(void* virtaddr){
	__asm__ volatile("invlpg (%0)" : : "r"(virtaddr) : "memory");
}

int pgd_load(pgd_t* pgd);

pgd_t* pgd_alloc();
void pgd_free(pgd_t* pgd);

int pgd_map(uintptr_t virtaddr, uintptr_t physaddr, int flags);
int pgd_unmap(uintptr_t virtaddr);

int pte_update_flags(uintptr_t virtaddr, int flags);
int pte_get_flags(uintptr_t virtaddr);

void* pgd_translate(void* virtaddr);

void pgd_copy(pgd_t* dest, pgd_t* src);
void pgd_copy_kernel(pgd_t* kernel_pgd, pgd_t* dest);
void pgd_remove_kernel(pgd_t* dest);

int pgd_dup_current(pgd_t* dest, uint8_t copy_kernel_area);

#endif

