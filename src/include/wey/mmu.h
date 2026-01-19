#ifndef _MEMORY_MANAGER_UNIT_H
#define _MEMORY_MANAGER_UNIT_H

#include <wey/vma.h>

int mmu_init();
pgd_t* mmu_create_page();
int mmu_mmap(void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags);
int mmu_munmap(void* vaddr, int size);
int mmu_set_flags(void* paddr, int size, mem_flags_t flags);

int mmu_page_switch(pgd_t* pgd);
int mmu_destroy_page(pgd_t* pgd);

void* mmu_translate(void* virt);

#endif
