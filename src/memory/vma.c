#include <asm/paging.h>
#include <wey/vfs.h>
#include <wey/vma.h>
#include <def/err.h>

#define ALIGN_DOWN(v,a) ((v) & ~((a)-1))
#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

struct mem_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr) {
	struct mem_region* region = mm->vma;

	while (region && virtaddr >= region->start) {
		if (virtaddr < region->end){
			return region;
		}

		region = region->next;
	}

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

	struct mem_region* prev = NULL;
	struct mem_region* curr = mm->vma;

	while (curr && curr->start < aligned_end) {
		if (aligned_start < curr->end && aligned_end > curr->start) {
			return INVALID_ARG;
		}

		prev = curr;
		curr = curr->next;
	}

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

	if (prev){
		prev->next = new_region;
	}
	else{
		mm->vma = new_region;
	}

	return SUCCESS;
}

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr){
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
			return SUCCESS;
		}
		prev = region;
		region = region->next;
	}

	return NOT_FOUND;
}

int vma_destroy(struct mm_struct* mm){
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
	return SUCCESS;
}
