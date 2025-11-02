#ifndef _MEMORY_MANAGER_UNIT_H
#define _MEMORY_MANAGER_UNIT_H

struct mm_struct;

int mmu_init();

pgd_t* mmu_create_page();

int mmu_mmap(struct mm_struct* mm, void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags, uint8_t map_flags);
int mmu_munmap(struct mm_struct* mm, void* vaddr, int size);
int mmu_set_flags(struct mm_struct* mm, void* paddr, int size, mem_flags_t flags);

int mmu_page_switch(pgd_t* pgd);
int mmu_destroy_page(pgd_t* pgd);

void* mmu_translate(void* virt);

#endif
