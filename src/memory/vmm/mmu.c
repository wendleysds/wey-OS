#include <stddef.h>
#include <stdint.h>
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
#include <lib/assert.h>

#include <asm-generic/paging_ctx.h>
#include <asm/page.h>
#include <asm/paging.h>

/**
* Main virtual memory management unit (MMU) kernel API
*/

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

#ifndef walker
#define USE_GENERIC_WALKER 1
#define walker __generic_walker
#endif

struct walk_level {
	void *table;
	pte_t *entry;
};

struct paging_ctx kernel_ctx = {
	.root = swapper_pgdir,
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

			pte_t e = ctx->ops->mk_table((uintptr_t)new_tbl, 0);

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

static inline struct paging_ctx* alloc_context(void){
	struct paging_ctx* ctx = kzalloc(sizeof(struct paging_ctx));
	if(ctx){
		ctx->root = NULL;
		ctx->ops = &arch_paging_ops;
		ctx->fmt = &arch_paging_fmt;
	}

	return ctx;
}

static inline void free_context(struct paging_ctx* ctx){
	if(!ctx) return;

	BUG_ON(ctx->root);

	kfree(ctx);
}

static void* ensure_table(const struct paging_ctx *restrict ctx, pte_t *entry, uint8_t user_table) {
	const struct paging_ops *restrict ops = ctx->ops;

	if (!ops->pte_present(*entry)) {
		struct page* page = page_alloc(0, PG_KERNEL | PG_TABLE);
		if(!page) return NULL;

		uintptr_t new_tbl = page_to_virt(page);
		memset((void*)new_tbl, 0x0, PAGE_SIZE);

		pte_t e = ops->mk_table(page_to_phys(page), user_table);
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

#ifdef USE_GENERIC_WALKER
static pte_t* __generic_walker(const struct paging_ctx *restrict ctx, uintptr_t vaddr, uint8_t create, uint8_t user_table) {
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
			table = ops->pte_to_virt(pte_val);
		} 
		else if (create) {
			table = ensure_table(ctx, entry, user_table);
			if (unlikely(!table)) return NULL;
		} 
		else {
			return NULL;
		}

		if (pte_leaf(pte_val) || i == (levels - 1)) {
			return entry;
		}
	}

	return NULL;
}
#endif

static void destroy_level(
	const struct paging_ctx *restrict ctx,
	const uintptr_t table,
	const int level,
	uintptr_t base_va
) {
	const struct paging_format *restrict fmt = ctx->fmt;
	const struct paging_ops *restrict ops = ctx->ops;

	typeof(ops->pte_present) pte_present = ops->pte_present;
	typeof(ops->pte_leaf) pte_leaf = ops->pte_leaf;
	typeof(ops->clear_pte) clear_pte = ops->clear_pte;
	typeof(ctx->ops->pte_phys) pte_phys = ctx->ops->pte_phys;

	const size_t entries = fmt->lvl[level].mask + 1;
	const uint8_t shift = fmt->lvl[level].shift;

	for (size_t i = 0; i < entries; i++) {
		pte_t *entry = &((pte_t*)table)[i];
		const pte_t pte_val = *entry;
		uintptr_t phys = pte_phys(pte_val);

		if (!pte_present(pte_val)){
			continue;
		}

		uintptr_t entry_va = base_va + (i << shift);
		if (entry_va >= USER_SPACE_END) {
			continue;
		}

		struct page* page = phys_to_page(phys);

		if (!pte_leaf(pte_val) && level != fmt->levels - 1) {
			const uintptr_t next = (uintptr_t)ops->pte_to_virt(pte_val);
			destroy_level(ctx, next, level + 1, entry_va);
		}

		page_put(page);
		clear_pte(entry);
	}
}

static void rollback_clone_level(
	const struct paging_ctx *restrict ctx, 
	void *table, 
	size_t max_entries, 
	int level, 
	uintptr_t base_va
) {
	const uint8_t shift = ctx->fmt->lvl[level].shift;

	typeof(ctx->ops->pte_present) pte_present = ctx->ops->pte_present;
	typeof(ctx->ops->pte_phys) pte_phys = ctx->ops->pte_phys;
	typeof(ctx->ops->pte_leaf) pte_leaf = ctx->ops->pte_leaf;

	for (size_t i = 0; i < max_entries; i++) {
		pte_t *e = &((pte_t*)table)[i];
		const pte_t pte_val = *e;
		uintptr_t phys = pte_phys(pte_val);

		if (!pte_present(pte_val))
			continue;

		uintptr_t entry_va = base_va + (i << shift);
		if (entry_va >= USER_SPACE_END) {
			ctx->ops->clear_pte(e);
			continue;
		}

		/* If this entry is a leaf, drop the page reference we took during
		* cloning. If it's a table entry, recursively destroy the child
		* subtree and free the table page. In all cases clear the PTE so
		* the partially-built destination table does not reference freed
		* memory. 
		*/
		if (pte_leaf(pte_val) || level == ctx->fmt->levels - 1) {
			struct page *page = phys_to_page(phys);
			page_put(page);
			ctx->ops->clear_pte(e);
		} else {
			void *child = ctx->ops->pte_to_virt(pte_val);
			destroy_level(ctx, (uintptr_t)child, level + 1, entry_va);

			page_free(
				phys_to_page(phys)
			);

			ctx->ops->clear_pte(e);
		}
	}
}

static inline void restore_src_mods(const struct paging_ctx *restrict src, pte_t **list, pte_t *orig_vals, size_t count) {
	for (size_t i = 0; i < count; i++) {
		pte_t *pte = list[i];
		pte_t old = orig_vals[i];

		/* revert the source PTE to its original value and drop the extra 
		* reference we took while cloning.
		*/
		src->ops->set_pte(pte, old);
	}

	src->ops->flush_all();
}

static void* clone_level(
	const struct paging_ctx *restrict dst,
	const struct paging_ctx *restrict src,
	const void *src_table,
	const int level,
	uintptr_t base_va
){
	struct page* page = page_alloc(0, PG_KERNEL | PG_TABLE);
	if(!page){
		return NULL;
	}

	void* new_table = (void*)page_to_virt(page);
	memset(new_table, 0, 4096);

	typeof(src->ops->pte_leaf) pte_leaf = src->ops->pte_leaf;
	typeof(src->ops->pte_present) pte_present = src->ops->pte_present;
	typeof(src->ops->pte_flags) pte_flags = src->ops->pte_flags;

	const size_t entries = src->fmt->lvl[level].mask + 1;
	const uint8_t shift = src->fmt->lvl[level].shift;

	/* Track source PTEs we modify at this level so we can restore them on
	* failure. */
	pte_t **const modified_list = kzalloc(entries * sizeof(pte_t*));
	pte_t *const orig_vals = kzalloc(entries * sizeof(pte_t));

	if (!modified_list || !orig_vals) {
		if(modified_list) kfree(modified_list);
		if(orig_vals) kfree(orig_vals);
		page_free(page);
		return NULL;
	}

	size_t mod_count = 0;
	for (size_t i = 0; i < entries; i++) {
		pte_t *src_e = &((pte_t*)src_table)[i];
		pte_t *dst_e = &((pte_t*)new_table)[i];
		const pte_t pte_val = *src_e;

		if (!pte_present(pte_val))
			continue;

		uintptr_t entry_va = base_va + (i << shift);

		if (entry_va >= USER_SPACE_END) {
			// share
			dst->ops->set_pte(dst_e, pte_val);
			continue;
		}

		mem_flags_t flags = arch_mmu_flags(pte_flags(pte_val));
		if (!pte_leaf(pte_val) && level != src->fmt->levels - 1) {
			uintptr_t phys = src->ops->pte_phys(pte_val);

			struct page* leaf_page = phys_to_page(phys);
			page_get(leaf_page);

			/* record original src pte so we can restore it if cloning
			* fails later */
			modified_list[mod_count] = src_e;
			orig_vals[mod_count] = pte_val;

			pte_t new_pte = pte_val;

			if ((flags & MEM_WRITE) && (flags & MEM_USER)) {
				new_pte = src->ops->mk_pte(
					phys, mmu_flags_arch(flags & ~MEM_WRITE)
				);

				src->ops->set_pte(src_e, new_pte);
				src->ops->flush_tlb_one(entry_va);
			}

			dst->ops->set_pte(dst_e, new_pte);
			mod_count++;
		} else {
			void *next = src->ops->pte_to_virt(pte_val);
			void *child = clone_level(dst, src, next, level + 1, entry_va);

			if (!child) {
				if (mod_count){
					restore_src_mods(src, modified_list, orig_vals, mod_count);
				}

				rollback_clone_level(dst, new_table, i, level, base_va);
				kfree(modified_list);
				kfree(orig_vals);
				page_free(page);
				return NULL;
			}

			pte_t table_entry = dst->ops->mk_table(__pa(child), flags & MEM_USER);
			dst->ops->set_pte(dst_e, table_entry);
		}
	}

	kfree(modified_list);
	kfree(orig_vals);

	return new_table;
}

static inline pte_t* walk_create(const struct paging_ctx *restrict ctx, uintptr_t vaddr, uint8_t user_table) {
	return walker(ctx, vaddr, 1, user_table);
}

static inline pte_t* walk(const struct paging_ctx *restrict ctx, uintptr_t vaddr) {
	return walker(ctx, vaddr, 0, 0x0);
}

int __init mmu_init(){
	mmu_set_flags_range(
		&kernel_ctx,
		(uintptr_t)__kernel_text_start,
		(size_t)__kernel_text_end - (size_t)__kernel_text_start,
		(MEM_READ | MEM_GLOBAL)
	);

	mmu_set_flags_range(
		&kernel_ctx,
		(uintptr_t)__kernel_rodata_start,
		(size_t)__kernel_rodata_end - (size_t)__kernel_rodata_start,
		(MEM_READ | MEM_GLOBAL)
	);

	kernel_ctx.ops->flush_all();

	return OK;
}

int mmu_mmap(struct paging_ctx *ctx, uintptr_t paddr, uintptr_t vaddr, size_t size, mem_flags_t mem_flags){
	size_t count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	uintptr_t roolback_virt_addr = vaddr;

	const uint32_t arch_flags = mmu_flags_arch(mem_flags);
	for(size_t i = 0; i < count; i++){
		pte_t *pte = walk_create(ctx, vaddr, mem_flags & MEM_USER);

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
		return ctx->ops->pte_phys(*pte);
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
		return arch_mmu_flags(ctx->ops->pte_flags(*pte));
	}

	return MEM_NOT_MAPPED;
}

struct paging_ctx* mmu_create_context(void){
	return mmu_clone_context(&kernel_ctx);
}

struct paging_ctx* mmu_clone_context(struct paging_ctx *src) {
	struct paging_ctx *dst = alloc_context();
	if(!dst) return NULL;

	void* root = clone_level(dst, src, src->root, 0, 0);
	if(!root){
		free_context(dst);
		return NULL;
	}

	dst->root = root;

	return dst;
}

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

void mmu_destroy_context(struct paging_ctx *ctx){
	uintptr_t root = (uintptr_t)ctx->root;

	destroy_level(ctx, root, 0,0);

	ctx->ops->flush_all();

	page_free(
		virt_to_page(root)
	);

	ctx->root = NULL;
	free_context(ctx);
}

void mmu_invlpg(struct paging_ctx *ctx, uintptr_t vaddr){
	ctx->ops->flush_tlb_one(vaddr);
}

void mmu_flush_all(struct paging_ctx *ctx){
	ctx->ops->flush_all();
}