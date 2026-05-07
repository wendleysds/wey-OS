#include "arch-config.h"
#include "mm/memblock.h"
#include <wey/mmu.h>
#include <def/err.h>
#include <def/init.h>
#include <def/config.h>

#include <asm/page.h>
#include <asm/paging.h>

/**
 * Main virtual memory management unit (MMU) kernel API
 */

struct paging_ctx kernel_ctx = {
	.root = initial_pgdir,
	.fmt = &arch_paging_fmt,
	.ops = &arch_paging_ops,
};

static void* ensure_table(struct paging_ctx *ctx, pte_t *entry) {
	/*if (!ctx->ops->pte_present(*entry)) {
		void *new_tbl = alloc_page();
		memset(new_tbl, 0, 4096);

		pte_t e = ctx->ops->mk_table(new_tbl);
		ctx->ops->set_pte(entry, e);
	}
	return ctx->ops->pte_to_virt(*entry);*/
	return 0x0;
}

static pte_t* walk_common(struct paging_ctx *ctx, uintptr_t* vaddr, int create){
	void *table = ctx->root;
	uint8_t last_level = ctx->fmt->levels - 1;

	for (uint8_t i = 0; i < ctx->fmt->levels; i++) {
		size_t idx = (*vaddr >> ctx->fmt->lvl[i].shift) & ctx->fmt->lvl[i].mask;
		pte_t *entry = &((pte_t*)table)[idx];

		if (i == last_level){
			return entry;// leaf
		}

		if(ctx->ops->pte_present(*entry)){
			table = ctx->ops->pte_to_virt(*entry);
		}else if(create){
			table = ensure_table(ctx, entry);
		}else break;

		if(!table) break;
	}

	return NULL;
}

static inline pte_t* walk_create(struct paging_ctx *ctx, uintptr_t* vaddr) {
	return walk_common(ctx, vaddr, 1);
}

static inline pte_t* walk(struct paging_ctx *ctx, uintptr_t* vaddr) {
	return walk_common(ctx, vaddr, 0);
}

static __init pte_t* walk_early(struct paging_ctx *ctx, uintptr_t* vaddr){
	void *table = ctx->root;

	for (int i = 0; i < ctx->fmt->levels; i++) {
		size_t idx = (*vaddr >> ctx->fmt->lvl[i].shift) & ctx->fmt->lvl[i].mask;
		pte_t *entry = &((pte_t*)table)[idx];

		if (i == ctx->fmt->levels - 1) {
			return entry; // leaf
		}

		// ensure
		if(!ctx->ops->pte_present(*entry)){
			void* new_tbl_phys = memblock_alloc(PAGE_SIZE, PAGE_SIZE);
			if(!new_tbl_phys) return NULL;

			void* new_tbl_virt = (void*)__va(new_tbl_phys);
		
			memset(new_tbl_virt, 0, 4096);
			pte_t e = ctx->ops->mk_table((uintptr_t)new_tbl_virt);
			ctx->ops->set_pte(entry, e);
		}

		table = ctx->ops->pte_to_virt(*entry);
	}

	return NULL;
}

int __init mmu_early_mmap(struct paging_ctx *ctx, uintptr_t vaddr, uintptr_t paddr, mem_flags_t flags) {
	pte_t *pte = walk_early(ctx, &vaddr);
	if(!pte){
		return FAILED;
	}

	uint32_t arch_flags = mmu_flags_arch(flags);
	pte_t val = ctx->ops->mk_pte(paddr, arch_flags);

	ctx->ops->set_pte(pte, val);
	ctx->ops->flush_tlb_one(vaddr);

	return OK;
}

int mmu_present(struct paging_ctx *ctx, uintptr_t vaddr){
	pte_t* pte = walk(ctx, &vaddr);
	if(!pte){
		return 0;
	}

	return ctx->ops->pte_present(*pte);
}

pte_t* mmu_get_pte(struct paging_ctx *ctx, uintptr_t vaddr){
	return walk(ctx, &vaddr);
}

void map_page(struct paging_ctx *ctx, uintptr_t vaddr, uintptr_t paddr, uint32_t flags){
	pte_t *pte = walk_create(ctx, &vaddr);

	uint32_t arch_flags = mmu_flags_arch(flags);
	pte_t val = ctx->ops->mk_pte(paddr, arch_flags);

	ctx->ops->set_pte(pte, val);
	ctx->ops->flush_tlb_one(vaddr);
}

void unmap_page(struct paging_ctx *ctx, uintptr_t vaddr) {
	pte_t *pte = walk(ctx, &vaddr);
	if (!pte) return;

	ctx->ops->clear_pte(pte);
	ctx->ops->flush_tlb_one(vaddr);
}

int __init mmu_init(){
	return OK;
}

int mmu_mmap(void* physaddr, void* virtaddr, int size, mem_flags_t mem_flags){
	return NOT_IMPLEMENTED;
}

int mmu_munmap(void* virtaddr, int size){
	return NOT_IMPLEMENTED;
}

int mmu_set_flags(void* virtaddr, int size, mem_flags_t flags){
	return NOT_IMPLEMENTED;
}

int mmu_page_switch(pgd_t* pgd){
	return NOT_IMPLEMENTED;
}

int mmu_destroy_page(pgd_t* pgd){
	return NOT_IMPLEMENTED;
}

pgd_t* mmu_create_page(){
	return ERR_PTR(NOT_IMPLEMENTED);
}

void* mmu_translate(void* virtaddr){
	return ERR_PTR(NOT_IMPLEMENTED);
}
