#include "wey/spinlock.h"
#include <asm/paging.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <def/err.h>
#include <lib/string.h>

#define ALIGN_DOWN(v,a) ((v) & ~((a)-1))
#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

struct mm_struct* vma_alloc(pgd_t *pgd){
	struct mm_struct* mm = kmalloc(sizeof(struct mm_struct));
	if(mm){
		memset(mm, 0x0, sizeof(struct mm_struct));
		mm->pgd = pgd;
		atomic_set(&mm->refcount, 1);
		spinlock_init(&mm->spinlock);
	}

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

int vma_add(
	struct mm_struct* mm,
	uintptr_t start,
	uintptr_t end,
	mem_flags_t mem_flags,
	unsigned int mpa_flags,
	struct file* file,
	off_t file_offset
){
	if (!mm){
		return NULL_PTR;
	}

	if (start >= end){
		return INVALID_ARG;
	}

	uintptr_t aligned_start = ALIGN_DOWN(start, PTE_PAGE_SIZE);
	uintptr_t aligned_end   = ALIGN_UP(end, PTE_PAGE_SIZE);

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
			return INVALID_ARG;
		}

		prev = curr;
		curr = curr->next;
	}

	spin_unlock(&mm->spinlock);

	struct mem_region* new_region = kmalloc(sizeof(struct mem_region));
	if (!new_region){
		return NO_MEMORY;
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

	return SUCCESS;
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

void vma_destroy(struct mm_struct* mm){
	struct mem_region* region = mm->vma;
	while (region) {
		struct mem_region* next = region->next;
		if(region->file){
			file_put(region->file);
		}
	
		kfree(region);
		region = next;
	}

	mm->vma = NULL;
}

struct mm_struct* vma_dup(struct mm_struct* mm){
	pgd_t* new_pgd = pgd_alloc();

	if(!new_pgd){
		return NULL;
	}

	if(pgd_dup_current(new_pgd,  1) != SUCCESS){
		pgd_free(new_pgd);
		return NULL;
	}

	struct mm_struct* new_mm = vma_alloc(new_pgd);

	if(!new_mm){
		goto out_free;
	}

	struct mem_region *cur = mm->vma;
	while(cur){
		int res = vma_add(
			new_mm, 
			cur->start, 
			cur->end, 
			cur->mem_flags, 
			cur->mpa_flags, 
			cur->file, 
			cur->file_offset
		);

		if(IS_ERR_VALUE(res)){
			spin_unlock(&mm->spinlock);
			vma_destroy(new_mm);
			kfree(new_mm);
			goto out_free;
		}

		cur = cur->next;
	}

	return new_mm;

out_free:
	pgd_free(new_pgd);
	return NULL;
}
