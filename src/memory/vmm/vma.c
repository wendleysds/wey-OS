#include <mm/vma.h>
#include <def/err.h>
#include <def/config.h>

#include <asm/paging.h>

#define ALIGN_DOWN(v,a) ((v) & ~((a)-1))
#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

struct mm_struct* vma_alloc(void){
	struct mm_struct* mm = kzalloc(sizeof(struct mm_struct));

	if(mm){
		atomic_set(&mm->refcount, 1);
		spinlock_init(&mm->spinlock);
	}

	return mm;
}

struct vm_region* vma_add(
	struct mm_struct* mm,
	uintptr_t start,
	uintptr_t end,
	mem_flags_t mem_flags,
	prot_flags_t prot_flags,
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

	struct vm_region* prev = NULL;
	struct vm_region* curr = mm->vma;
	while (curr && curr->start < aligned_end) {
		if (aligned_start < curr->end && aligned_end > curr->start) {
			spin_unlock(&mm->spinlock);
			return ERR_PTR(INVALID_ARG);
		}

		prev = curr;
		curr = curr->next;
	}

	spin_unlock(&mm->spinlock);

	struct vm_region* new_region = kzalloc(sizeof(struct vm_region));
	if (!new_region){
		return ERR_PTR(NO_MEMORY);
	}

	new_region->start = aligned_start;
	new_region->end = aligned_end;
	new_region->mem_flags = mem_flags;
	new_region->prot_flags = prot_flags;
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

struct vm_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr){
	struct vm_region* region = mm->vma;

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

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr){
	spin_lock(&mm->spinlock);

	struct vm_region* prev = NULL;
	struct vm_region* region = mm->vma;
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
	struct vm_region* region = mm->vma;
	while (region) {
		struct vm_region* next = region->next;
		if(region->file){
			file_put(region->file);
		}

		kfree(region);
		region = next;
	}

	mm->vma = NULL;
}

void vma_destroy(struct mm_struct* mm){
	if(mm->vma) vma_clean(mm);
	if(mm->ctx) mmu_destroy_context(mm->ctx);

	kfree(mm);
}

struct mm_struct* vma_dup(struct mm_struct* mm){
	struct mm_struct* new_mm = vma_alloc();
	if(!new_mm){
		return NULL;
	}

	spin_lock(&mm->spinlock);
	struct vm_region* cur = mm->vma;
	while(cur){
		struct vm_region* region = vma_add(
			new_mm,
			cur->start,
			cur->end,
			cur->mem_flags,
			cur->prot_flags,
			cur->file,
			cur->file_offset
		);

		if(IS_ERR_VALUE(region)){
			spin_unlock(&mm->spinlock);
			goto out_free;
		}

		cur = cur->next;
	}
	spin_unlock(&mm->spinlock);

	struct paging_ctx* cloned_ctx = mmu_clone_context(mm->ctx);
	if(!cloned_ctx){
		goto out_free;
	}
	
	new_mm->ctx = cloned_ctx;
	new_mm->brk = mm->brk;
	new_mm->brk_start = mm->brk_start;

	return new_mm;

out_free:
	vma_destroy(new_mm);
	return NULL;
}

