#include <wey/mmu.h>
#include <wey/printk.h>
#include <mm/memblock.h>
#include <mm/page.h>
#include <mm/kheap.h>
#include <def/err.h>
#include <def/init.h>
#include <def/config.h>
#include <def/linker.h>
#include <lib/string.h>

#include <asm-generic/paging_ctx.h>
#include <asm/page.h>
#include <asm/paging.h>

/**
* Main virtual memory management unit (MMU) kernel API
*/

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

struct walk_level {
	void *table;
	pte_t *entry;
};

struct paging_ctx kernel_ctx = {
	.root = initial_pgdir,
	.fmt = &arch_paging_fmt,
	.ops = &arch_paging_ops,
};

static __init pte_t* walk_early(struct paging_ctx *ctx, uintptr_t vaddr){
	void *table = ctx->root;

	for (int i = 0; i < ctx->fmt->levels; i++) {
		size_t idx = (vaddr >> ctx->fmt->lvl[i].shift) & ctx->fmt->lvl[i].mask;
		pte_t *entry = &((pte_t*)table)[idx];

		if (ctx->ops->pte_leaf(*entry) || i == (ctx->fmt->levels - 1)) {
			return entry;
		}

		// ensure
		if(!ctx->ops->pte_present(*entry)){
			void* new_tbl = memblock_alloc(PAGE_SIZE, PAGE_SIZE);
			if(!new_tbl) return NULL;

			pte_t e = ctx->ops->mk_table((uintptr_t)new_tbl);

			ctx->ops->set_pte(
				entry,
				e
			);

			memset((void*)__va(new_tbl), 0, 4096);
		}

		table = ctx->ops->pte_to_virt(*entry);
	}

	return NULL;
}

int __init mmu_early_mmap(struct paging_ctx *ctx, uintptr_t vaddr, uintptr_t paddr, mem_flags_t flags) {
	pte_t *pte = walk_early(ctx, vaddr);
	if(!pte){
		return FAILED;
	}

	uint32_t arch_flags = mmu_flags_arch(flags);
	pte_t val = ctx->ops->mk_pte(paddr, arch_flags);

	ctx->ops->set_pte(pte, val);
	ctx->ops->flush_tlb_one(vaddr);

	return OK;
}

static struct paging_ctx* alloc_context(void){
	struct paging_ctx* ctx = kzalloc(sizeof(struct paging_ctx));
	if(ctx){
		ctx->root = NULL;
		ctx->ops = &arch_paging_ops;
		ctx->fmt = &arch_paging_fmt;
	}

	return ctx;
}

static void free_context(struct paging_ctx* ctx){
	if(!ctx) return;
	kfree(ctx);
}

static void* ensure_table(const struct paging_ctx *restrict ctx, pte_t *entry) {
	const struct paging_ops *restrict ops = ctx->ops;

	if (!ops->pte_present(*entry)) {
		struct page* page = page_alloc(0, PG_KERNEL | PG_TABLE);
		if(!page) return NULL;

		uintptr_t new_tbl = page_to_virt(page);
		memset((void*)new_tbl, 0x0, PAGE_SIZE);

		pte_t e = ops->mk_table(new_tbl);
		ops->set_pte(entry, e);
	}

	return ops->pte_to_virt(*entry);
}

static int table_empty(struct paging_ctx *ctx, void *table, uint8_t level) {
	typeof(ctx->ops->pte_present) pte_present = ctx->ops->pte_present;
	size_t entries = ctx->fmt->lvl[level].mask + 1;

	for (size_t i = 0; i < entries; i++) {
		pte_t *e = &((pte_t*)table)[i];

		if (pte_present(*e))
			return 0;
	}

	return 1;
}

static int walk_to(struct paging_ctx *ctx, uintptr_t va, struct walk_level *levels) {
	void *table = ctx->root;

	for (uint8_t i = 0; i < ctx->fmt->levels; i++) {
		size_t idx =
			(va >> ctx->fmt->lvl[i].shift) &
			ctx->fmt->lvl[i].mask;

		pte_t *entry = &((pte_t*)table)[idx];

		levels[i].table = table;
		levels[i].entry = entry;

		if (!ctx->ops->pte_present(*entry))
			return -1;

		if (i != ctx->fmt->levels - 1)
			table = ctx->ops->pte_to_virt(*entry);
	}

	return 0;
}

static pte_t* walk_common(const struct paging_ctx *restrict ctx, uintptr_t vaddr, int create) {
	const struct paging_format *restrict fmt = ctx->fmt;
	const struct paging_ops *restrict ops = ctx->ops;

	typeof(ops->pte_present) pte_present = ops->pte_present;
	typeof(ops->pte_leaf) pte_leaf = ops->pte_leaf;
	
	void *table = ctx->root;
	uint8_t levels = fmt->levels;

	for (uint8_t i = 0; i < levels; i++) {
		const struct paging_level *restrict lvl = &fmt->lvl[i];
		size_t idx = (vaddr >> lvl->shift) & lvl->mask;
		pte_t *entry = &((pte_t*)table)[idx];

		const pte_t pte_val = *entry;

		if (likely(pte_present(pte_val))) {
			if (pte_leaf(pte_val) || i == (levels - 1)) {
				return entry;
			}

			table = ops->pte_to_virt(pte_val);
		} 
		else if (create) {
			table = ensure_table(ctx, entry);
			if (unlikely(!table)) return NULL;
		} 
		else {
			return NULL;
		}
	}
	return NULL;
}

static inline pte_t* walk_create(const struct paging_ctx *restrict ctx, uintptr_t vaddr) {
	return walk_common(ctx, vaddr, 1);
}

static inline pte_t* walk(const struct paging_ctx *restrict ctx, uintptr_t vaddr) {
	return walk_common(ctx, vaddr, 0);
}

int __init mmu_init(){
	mmu_set_flags_range(
		&kernel_ctx,
		(uintptr_t)__kernel_text_start,
		(size_t)__kernel_text_end - (size_t)__kernel_text_start,
		(MEM_READ | MEM_KERNEL)
	);

	mmu_set_flags_range(
		&kernel_ctx,
		(uintptr_t)__kernel_rodata_start,
		(size_t)__kernel_rodata_end - (size_t)__kernel_rodata_start,
		(MEM_READ | MEM_KERNEL)
	);

	kernel_ctx.ops->flush_all();

	return OK;
}

int mmu_mmap(struct paging_ctx *ctx, uintptr_t paddr, uintptr_t vaddr, size_t size, mem_flags_t mem_flags){
	size_t count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	uintptr_t roolback_virt_addr = vaddr;

	uint32_t arch_flags = mmu_flags_arch(mem_flags);
	for(size_t i = 0; i < count; i++){
		pte_t *pte = walk_create(ctx, vaddr);

		if(unlikely(!pte)){
			if(i > 0){
				mmu_munmap(ctx, roolback_virt_addr, i * PAGE_SIZE);
			}
	
			return NO_MEMORY;
		}

		pte_t val = ctx->ops->mk_pte(paddr, arch_flags);

		ctx->ops->set_pte(pte, val);
		ctx->ops->flush_tlb_one(vaddr);

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}

	return OK;
}

void mmu_munmap(struct paging_ctx *ctx, uintptr_t vaddr, size_t size) {
	size_t pages = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	const struct paging_format *restrict fmt = ctx->fmt;
	typeof(ctx->ops->clear_pte) clear_pte = ctx->ops->clear_pte;
	typeof(ctx->ops->flush_tlb_one) flush_tlb_one = ctx->ops->flush_tlb_one;

	for (; pages--; vaddr += PAGE_SIZE) {

		struct walk_level levels[MAX_LEVELS];

		if (walk_to(ctx, vaddr, levels) < 0)
			continue;

		pte_t *pte = levels[fmt->levels - 1].entry;

		clear_pte(pte);
		flush_tlb_one(vaddr);

		for (uint8_t lvl = fmt->levels - 1; lvl > 0; lvl--) {
			void *table = levels[lvl].table;

			if (!table_empty(ctx, table, lvl))
				break;

			pte_t *parent = levels[lvl - 1].entry;

			clear_pte(parent);

			page_put(virt_to_page((uintptr_t)table));
		}
	}
}

uintptr_t mmu_translate(struct paging_ctx *ctx, uintptr_t vaddr){
	pte_t* pte = walk(ctx, vaddr);
	if(pte){
		uintptr_t phys = ctx->ops->pte_phys(*pte);
		return phys & PAGE_MASK;
	}

	return 0;
}

void mmu_set_flags(struct paging_ctx *ctx, uintptr_t vaddr, mem_flags_t flags){
	const struct paging_ops *restrict ops = ctx->ops;
	pte_t* pte = walk(ctx, vaddr);

	if(pte){
		int arch_flags = mmu_flags_arch(flags);
		pte_t pte_val = *pte;

		uintptr_t phys = ops->pte_phys(pte_val);
		pte_val = ops->mk_pte(phys, arch_flags);
		ops->set_pte(pte, pte_val);
		ops->flush_tlb_one(vaddr);
	}
}

void mmu_set_flags_range(struct paging_ctx *ctx, uintptr_t vaddr, size_t size, mem_flags_t flags){
	size_t pages = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	for(size_t i = 0; i < pages; i++){
		mmu_set_flags(ctx, vaddr + (i * PAGE_SIZE), flags);
	}
}

mem_flags_t mmu_get_flags(struct paging_ctx *ctx, uintptr_t vaddr){
	pte_t* pte = walk(ctx, vaddr);
	if(pte){
		uintptr_t phys = ctx->ops->pte_phys(*pte);
		uint32_t flags = phys & FLAGS_MASK;
		return arch_mmu_flags(flags);
	}

	return MEM_NOT_MAPPED;
}

struct paging_ctx* mmu_create_context(void){
	struct page* page = page_alloc(0, PG_KERNEL | PG_TABLE);
	if(!page){
		return NULL;
	}

	struct paging_ctx* ctx = alloc_context();
	if(!ctx){
		page_free(page);
		return NULL;
	}

	uintptr_t phys = page_to_phys(page);
	pte_t table = arch_paging_ops.mk_table(phys);

	ctx->root = arch_paging_ops.pte_to_virt(table);

	return ctx;
}

/*static void* clone_level(
	const struct paging_ctx *restrict dst,
	const struct paging_ctx *restrict src,
	void *src_table,
	const int level)
{
	void *new_table = alloc_page();
	memset(new_table, 0, 4096);

	size_t entries = src->fmt->lvl[level].mask + 1;

	for (size_t i = 0; i < entries; i++) {
		pte_t *src_e = &((pte_t*)src_table)[i];
		pte_t *dst_e = &((pte_t*)new_table)[i];

		if (!src->ops->pte_present(*src_e))
			continue;

		if (level == src->fmt->levels - 1) {
			// LEAF: aplicar COW
			uintptr_t phys = src->ops->pte_phys(*src_e);

			struct page* page = phys_to_page(phys);
			page_get(page);

			pte_t new_pte = *src_e;

			if (pte_writable(new_pte)) {
				new_pte = pte_mark_cow(new_pte);
				src->ops->set_pte(src_e, new_pte); // pai vira RO também
			}

			dst->ops->set_pte(dst_e, new_pte);
		} else {
			void *next = src->ops->pte_to_virt(*src_e);

			void *child = clone_level(dst, src, next, level + 1);

			pte_t table_entry = dst->ops->mk_table((uintptr_t)child);
			dst->ops->set_pte(dst_e, table_entry);
		}
	}

	return new_table;
}

struct paging_ctx* mmu_clone_context(struct paging_ctx *src) {
	struct paging_ctx *dst = alloc_context();

	dst->root = clone_level(dst, src, src->root, 0);

	return dst;
}*/

int mmu_copy_context(struct paging_ctx *src, struct paging_ctx *dst);

int mmu_context_switch(struct paging_ctx *ctx){
	if(!ctx){
		return INVALID_ARG;
	}

	uintptr_t target_phys = mmu_translate(ctx, (uintptr_t)ctx->root);
	if (target_phys & (PAGE_SIZE - 1)) {
		return BAD_ALIGNMENT;
	}

	uintptr_t curr_phys = paging_current_table_phys();

	if(target_phys == curr_phys){
		return OK;
	}

	paging_load_table(target_phys);
	return OK;
}

/*static void destroy_level(
	const struct paging_ctx *restrict ctx,
	const void *table,
	const int level,
	const int free_leaf_pages
) {
	const struct paging_format *restrict fmt = ctx->fmt;
	const struct paging_ops *restrict ops = ctx->ops;

	typeof(ops->pte_present) pte_present = ops->pte_present;
	typeof(ops->pte_leaf) pte_leaf = ops->pte_leaf;
	typeof(ops->clear_pte) clear_pte = ops->clear_pte;

	size_t entries = fmt->lvl[level].mask + 1;

	for (size_t i = 0; i < entries; i++) {
		pte_t *entry = &((pte_t*)table)[i];
		const pte_t pte_val = *entry;

		if (!pte_present(pte_val))
			continue;

		if (pte_leaf(pte_val) || level == fmt->levels - 1) {
			if (free_leaf_pages) {
				uintptr_t phys = ops->pte_phys(pte_val);
				page_free(phys_to_page(phys));
			}

			clear_pte(entry);
		} else {
			const void *next = ops->pte_to_virt(pte_val);

			destroy_level(ctx, next, level + 1, free_leaf_pages);

			page_free(virt_to_page((uintptr_t)next));

			clear_pte(entry);
		}
	}
}

void mmu_destroy_context(struct paging_ctx *ctx){
	destroy_level(ctx, ctx->root, 0, 1);
	page_free(virt_to_page((uintptr_t)ctx->root));
	kfree(ctx);
}*/

void mmu_destroy_context(struct paging_ctx *ctx){}

int mmu_present(struct paging_ctx *ctx, uintptr_t vaddr);
uintptr_t mmu_translate(struct paging_ctx *ctx, uintptr_t vaddr);