#ifndef _MEMORY_MANAGER_UNIT_H
#define _MEMORY_MANAGER_UNIT_H

#include <asm-generic/paging_ctx.h>
#include <wey/vma.h>

extern int mmu_flags_arch(mem_flags_t flags);
extern mem_flags_t arch_mmu_flags(int flags);

int mmu_early_mmap(struct paging_ctx *ctx, uintptr_t vaddr, uintptr_t paddr, mem_flags_t flags);

int mmu_present(struct paging_ctx *ctx, uintptr_t vaddr);
pte_t* mmu_get_pte(struct paging_ctx *ctx, uintptr_t vaddr);

int mmu_init(void);
pgd_t* mmu_create_page();
int mmu_mmap(void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags);
int mmu_munmap(void* vaddr, int size);
int mmu_set_flags(void* paddr, int size, mem_flags_t flags);

int mmu_page_switch(pgd_t* pgd);
int mmu_destroy_page(pgd_t* pgd);

void* mmu_translate(void* virt);

#endif
