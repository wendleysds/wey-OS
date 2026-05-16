#ifndef _MEMORY_MANAGER_UNIT_H
#define _MEMORY_MANAGER_UNIT_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
	// Memory flags
	MEM_READ   = 1 << 0,
	MEM_WRITE  = 1 << 1,
	MEM_KERNEL = 1 << 2,
	MEM_USER   = 1 << 3,
	MEM_GLOBAL = 1 << 4,
	MEM_DEVICE = 1 << 5,
	MEM_CACHE  = 1 << 6,
	MEM_GROWSDOWN = 1 << 7,

	// Process flags / flags for mem_region
	MEM_EXEC   = 1 << 8,
	MEM_SHARED = 1 << 9,
	MEM_LOCKED = 1 << 10,
	MEM_NOT_MAPPED = 1 << 11
} mem_flags_t;

struct paging_ctx;

extern int mmu_flags_arch(mem_flags_t flags);
extern mem_flags_t arch_mmu_flags(int flags);

int mmu_early_mmap(struct paging_ctx *ctx, uintptr_t vaddr, uintptr_t paddr, mem_flags_t flags);

int mmu_init(void);

int mmu_mmap(struct paging_ctx *ctx, uintptr_t paddr, uintptr_t vaddr, size_t size, mem_flags_t mem_flags);
void mmu_munmap(struct paging_ctx *ctx, uintptr_t vaddr, size_t size);

void mmu_set_flags(struct paging_ctx *ctx, uintptr_t vaddr, mem_flags_t flags);
void mmu_set_flags_range(struct paging_ctx *ctx, uintptr_t vaddr, size_t size, mem_flags_t flags);
mem_flags_t mmu_get_flags(struct paging_ctx *ctx, uintptr_t vaddr);

struct paging_ctx* mmu_create_context(void);
struct paging_ctx* mmu_clone_context(struct paging_ctx *src);
int mmu_copy_context(struct paging_ctx *src, struct paging_ctx *dst);

int mmu_context_switch(struct paging_ctx *ctx);
void mmu_destroy_context(struct paging_ctx *ctx);

uintptr_t mmu_translate(struct paging_ctx *ctx, uintptr_t vaddr);

#endif
