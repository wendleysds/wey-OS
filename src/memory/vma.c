#include "wey/mmu.h"
#include "wey/panic.h"
#include "wey/spinlock.h"
#include <asm/paging.h>
#include <stdint.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <def/err.h>
#include <def/config.h>
#include <mm/page.h>
#include <lib/string.h>

#define ALIGN_DOWN(v,a) ((v) & ~((a)-1))
#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

static inline size_t page_hash(uintptr_t vaddr){
	vaddr >>= 12; // remove low bits
	return (vaddr ^ (vaddr >> 16)) % MAX_VM_PAGE_HASH;
}

struct mm_struct* vma_alloc(){
	pgd_t *pgd = mmu_create_page();
	if(!pgd){
		return NULL;
	}

	struct mm_struct* mm = kmalloc(sizeof(struct mm_struct));
	if(!mm){
		mmu_destroy_page(pgd);
		return NULL;
	}

	memset(mm, 0x0, sizeof(struct mm_struct));
	mm->pgd = pgd;
	atomic_set(&mm->refcount, 1);
	spinlock_init(&mm->spinlock);

	return mm;
}

struct mem_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr) {
	struct mem_region* region = mm->vma;

	spin_lock(&mm->spinlock);
	while (region && virtaddr >= region->start) {
		if (virtaddr < region->end){
			spin_unlock(&mm->spinlock);
			return region;
		}

		region = region->next;
	}

	spin_unlock(&mm->spinlock);
	return NULL;
}

struct mem_region* vma_add(
	struct mm_struct* mm,
	uintptr_t start,
	uintptr_t end,
	mem_flags_t mem_flags,
	unsigned int mpa_flags,
	struct file* file,
	off_t file_offset
){
	if (!mm){
		return ERR_PTR(NULL_PTR);
	}

	if (start >= end){
		return ERR_PTR(INVALID_ARG);
	}

	uintptr_t aligned_start = ALIGN_DOWN(start, PAGE_SIZE);
	uintptr_t aligned_end   = ALIGN_UP(end, PAGE_SIZE);

	off_t aligned_offset = file_offset;

	if (file) {
		uintptr_t delta = start - aligned_start;
		aligned_offset -= delta;
	}

	spin_lock(&mm->spinlock);

	struct mem_region* prev = NULL;
	struct mem_region* curr = mm->vma;
	while (curr && curr->start < aligned_end) {
		if (aligned_start < curr->end && aligned_end > curr->start) {
			spin_unlock(&mm->spinlock);
			return ERR_PTR(INVALID_ARG);
		}

		prev = curr;
		curr = curr->next;
	}

	spin_unlock(&mm->spinlock);

	struct mem_region* new_region = kzalloc(sizeof(struct mem_region));
	if (!new_region){
		return ERR_PTR(NO_MEMORY);
	}

	new_region->start = aligned_start;
	new_region->end = aligned_end;
	new_region->mem_flags = mem_flags;
	new_region->mpa_flags = mpa_flags;
	new_region->file = file;
	new_region->file_offset = aligned_offset;
	new_region->next = curr;

	if (file){
		file_get(file);
	}

	spin_lock(&mm->spinlock);

	if (prev){
		prev->next = new_region;
	}
	else{
		mm->vma = new_region;
	}

	spin_unlock(&mm->spinlock);

	return new_region;
}

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr){
	spin_lock(&mm->spinlock);

	struct mem_region* prev = NULL;
	struct mem_region* region = mm->vma;
	while (region) {
		if (virtaddr >= region->start && virtaddr < region->end) {
			if (prev) {
				prev->next = region->next;
			} else {
				mm->vma = region->next;
			}

			if(region->file){
				file_put(region->file);
			}

			kfree(region);
			spin_unlock(&mm->spinlock);
			return SUCCESS;
		}

		prev = region;
		region = region->next;
	}

	spin_unlock(&mm->spinlock);

	return NOT_FOUND;
}

void vma_clean(struct mm_struct* mm){
	struct mem_region* region = mm->vma;
	while (region) {
		struct mem_region* next = region->next;
		if(region->file){
			file_put(region->file);
		}

		vma_page_hash_destroy(region);
	
		kfree(region);
		region = next;
	}

	mm->vma = NULL;
}

void vma_destroy(struct mm_struct* mm){
	if(mm->vma) vma_clean(mm);

	if(mm->pgd){
		int res = mmu_destroy_page(mm->pgd);
		if(IS_ERR_VALUE(res)){
			panic("vma: destroy: failed to free pgd! %d\n", res);
		}
	}

	kfree(mm);
}

static void set_mm_cow(struct mm_struct* mm, uint8_t increse_page_ref, pgd_t* cur_pgd){
	mmu_page_switch(mm->pgd);
	struct mem_region *cur = mm->vma;
	while(cur){
		for(int i = 0; i < MAX_VM_PAGE_HASH; i++){
			for(struct vm_page* node = cur->page_hash[i]; 
				node; 
				node = node->next)
			{
				if(increse_page_ref) page_get(node->page);
				if((cur->mem_flags & MEM_WRITE) == 0) continue;

				node->page->flags |= PAGE_COW;
				//int flags = mmu_flags_arch(cur->mem_flags & ~MEM_WRITE);
				//pte_update_flags(node->addr, flags);
			}
		}

		cur = cur->next;
	}

	if(cur_pgd) mmu_page_switch(cur_pgd);
}

struct mm_struct* vma_dup(struct mm_struct* mm){
	struct mm_struct* new_mm = vma_alloc();
	if(!new_mm){
		return NULL;
	}

	/*if(pgd_dup_current(vma_get_pgd(new_mm), 0) != SUCCESS){
		vma_destroy(new_mm);
		return NULL;
	}*/

	spin_lock(&mm->spinlock);
	struct mem_region *cur = mm->vma;
	while(cur){
		struct mem_region* region_new = vma_add(
			new_mm, 
			cur->start, 
			cur->end, 
			cur->mem_flags, 
			cur->mpa_flags, 
			cur->file, 
			cur->file_offset
		);

		if(IS_ERR_VALUE(region_new)){
			goto out_vma;
		}

		for(int i = 0; i < MAX_VM_PAGE_HASH; i++){
			struct vm_page* node = cur->page_hash[i];
			while(node){
				struct vm_page* next = node->next;

				int res = vma_page_hash_insert(region_new, node->addr, node->page);
				if(IS_ERR_VALUE(res)){
					goto out_vma;
				}

				node = next;
			}
		}

		cur = cur->next;
	}

	set_mm_cow(new_mm, 1, 0x0);
	set_mm_cow(mm, 0, 0x0);

	goto out;

out_vma:
	vma_destroy(new_mm);
	new_mm = NULL;
out:
	spin_unlock(&mm->spinlock);
	return new_mm;
}

struct page* vma_page_hash_lookup(struct mem_region* region, uintptr_t vaddr){
	uintptr_t page_addr = ALIGN_DOWN(vaddr, PAGE_SIZE);
	size_t hash = page_hash(page_addr);

	struct vm_page* node = region->page_hash[hash];

	while(node){
		if(node->addr == page_addr)
			return node->page;
		node = node->next;
	}

	return 0x0;
}

int vma_page_hash_insert(struct mem_region* region, uintptr_t vaddr, struct page* page){
	uintptr_t page_addr = ALIGN_DOWN(vaddr, PAGE_SIZE);

	size_t hash = page_hash(page_addr);

	struct vm_page* node = kmalloc(sizeof(struct vm_page));
	if(!node){
		return NO_MEMORY;
	}

	node->addr = page_addr;
	node->page = page;

	node->next = region->page_hash[hash];
	region->page_hash[hash] = node;

	return SUCCESS;
}

int vma_page_hash_remove(struct mem_region* region, uintptr_t vaddr){
	uintptr_t page_addr = ALIGN_DOWN(vaddr, PAGE_SIZE);
	size_t hash = page_hash(page_addr);

	struct vm_page* cur = region->page_hash[hash];
	struct vm_page* prev = NULL;

	while(cur){
		if(cur->addr == page_addr){
			if(!prev){
				region->page_hash[hash] = cur->next;
			}else{
				prev->next = cur->next;
			}

			page_put(cur->page);
			kfree(cur);
			return SUCCESS;
		}

		prev = cur;
		cur = cur->next;
	}
	
	return NOT_FOUND;
}

void vma_page_hash_destroy(struct mem_region* region){
	for(int i = 0; i < MAX_VM_PAGE_HASH; i++){
		struct vm_page* node = region->page_hash[i];

		while(node){
			struct vm_page* next = node->next;
			page_put(node->page);
			kfree(node);
			node = next;
		}

		region->page_hash[i] = NULL;
	}
}

void vma_set_cow(struct mm_struct* p, struct mm_struct* c){
	
}
