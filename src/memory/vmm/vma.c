#include <mm/vma.h>
#include <def/errno.h>
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
) {
	if (!mm || start >= end) return ERR_PTR(-EINVAL);

	uintptr_t aligned_start = ALIGN_DOWN(start, PAGE_SIZE);
	uintptr_t aligned_end   = ALIGN_UP(end, PAGE_SIZE);
	off_t aligned_offset    = file_offset;

	if (file) {
		aligned_offset -= (start - aligned_start);
	}

	struct vm_region* new_region = kzalloc(sizeof(struct vm_region));
	if (!new_region) return ERR_PTR(-ENOMEM);

	new_region->start = aligned_start;
	new_region->end = aligned_end;
	new_region->mem_flags = mem_flags;
	new_region->prot_flags = prot_flags;
	new_region->file = file;
	new_region->file_offset = aligned_offset;

	spin_lock(&mm->spinlock);

	struct vm_region* prev = NULL;
	struct vm_region* curr = mm->vma;

	while (curr && curr->start < aligned_start) {
		prev = curr;
		curr = curr->next;
	}

	if ((curr && aligned_end > curr->start) || (prev && aligned_start < prev->end)) {
		spin_unlock(&mm->spinlock);
		kfree(new_region);
		return ERR_PTR(-EINVAL);
	}

	new_region->next = curr;
	if (prev) {
		prev->next = new_region;
	} else {
		mm->vma = new_region;
	}

	spin_unlock(&mm->spinlock);

	if (file) {
		file_get(file);
	}

	return new_region;
}

struct vm_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr) {
	if (!mm) return NULL;

	spin_lock(&mm->spinlock);

	struct vm_region* region = mm->vma;

	while (region) {
		if (virtaddr < region->start) {
			region = region->next;
			continue;
		}

		if (virtaddr >= region->start && virtaddr < region->end) {
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

	return -ENOENT;
}

int vma_remove_range(struct mm_struct* mm, uintptr_t start, uintptr_t end) {
	if(!mm || start >= end)
		return -EINVAL;

	if(start & (PAGE_SIZE - 1))
		return -EINVAL;

	end = ALIGN_UP(end, PAGE_SIZE);
	if(end <= start)
		return -EINVAL;

	struct vm_region* spare = kzalloc(sizeof(struct vm_region));
	if(!spare)
		return -ENOMEM;

	struct vm_region* dead = NULL;

	spin_lock(&mm->spinlock);

	struct vm_region* prev = NULL;
	struct vm_region* curr = mm->vma;

	while(curr && curr->start < end){
		struct vm_region* next = curr->next;

		if(curr->end <= start){
			prev = curr;
			curr = next;
			continue;
		}

		if(start <= curr->start && end >= curr->end){
			if(prev)
				prev->next = next;
			else
				mm->vma = next;

			curr->next = dead;
			dead = curr;

			curr = next;
			continue;
		}

		if(start <= curr->start){
			if(curr->file)
				curr->file_offset += (off_t)(end - curr->start);

			curr->start = end;
		}else if(end >= curr->end){
			curr->end = start;
		}else{
			spare->start      = end;
			spare->end        = curr->end;
			spare->mem_flags  = curr->mem_flags;
			spare->prot_flags = curr->prot_flags;
			spare->file       = curr->file;

			if(curr->file){
				spare->file_offset = curr->file_offset + (off_t)(end - curr->start);
				file_get(curr->file);
			}

			spare->next = next;
			curr->end   = start;
			curr->next  = spare;

			spare = NULL;
		}

		prev = curr;
		curr = next;
	}

	spin_unlock(&mm->spinlock);

	while(dead){
		struct vm_region* n = dead->next;

		if(dead->file)
			file_put(dead->file);

		kfree(dead);
		dead = n;
	}

	if(spare)
		kfree(spare);

	return SUCCESS;
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

