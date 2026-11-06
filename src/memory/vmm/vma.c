#include <mm/vma.h>
#include <def/errno.h>
#include <def/config.h>

#include <asm/paging.h>

#define ALIGN_DOWN(v,a) ((v) & ~((a)-1))
#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

static struct vm_region* vma_lookup_locked(struct mm_struct* mm, uintptr_t addr){
	struct vm_region* region = mm->vma;

	while (region) {
		if (addr < region->start) {
			break;
		}

		if (addr >= region->start && addr < region->end) {
			return region;
		}
		
		region = region->next;
	}

	return NULL;
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
	
	off_t aligned_offset = file ? file_offset - (start - aligned_start) : 0;

	struct vm_region* new_region = kzalloc(sizeof(struct vm_region));
	if (!new_region) return ERR_PTR(-ENOMEM);

	new_region->start = aligned_start;
	new_region->end = aligned_end;
	new_region->mem_flags = mem_flags;
	new_region->prot_flags = prot_flags;
	new_region->file = file;
	new_region->file_offset = aligned_offset;

	spin_lock(&mm->lock);

	struct vm_region *prev = NULL;
	struct vm_region *curr = mm->vma;

	while (curr && curr->start < aligned_start) {
		prev = curr;
		curr = curr->next;
	}

	if ((curr && aligned_end > curr->start) || 
		(prev && aligned_start < prev->end)) {
		spin_unlock(&mm->lock);
		kfree(new_region);
		return ERR_PTR(-EINVAL);
	}

	new_region->next = curr;
	if (prev) {
		prev->next = new_region;
	} else {
		mm->vma = new_region;
	}

	if (file) {
		file_get(file);
	}

	spin_unlock(&mm->lock);
	return new_region;
}


struct vm_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr) {
	if (!mm) return NULL;

	spin_lock(&mm->lock);
	struct vm_region* region = vma_lookup_locked(mm, virtaddr);
	spin_unlock(&mm->lock);

	return region;
}

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr) {
	if (!mm) return -EINVAL;

	spin_lock(&mm->lock);

	struct vm_region **curr = &mm->vma;
	
	while (*curr) {
		struct vm_region *entry = *curr;

		if (virtaddr < entry->start) break;

		if (virtaddr >= entry->start && virtaddr < entry->end) {
			*curr = entry->next;

			if (entry->file) {
				file_put(entry->file);
			}

			kfree(entry);
			spin_unlock(&mm->lock);
			return SUCCESS;
		}

		curr = &entry->next;
	}

	spin_unlock(&mm->lock);
	return -ENOENT;
}

int vma_resize(struct mm_struct* mm, uintptr_t start_addr, size_t new_size) {
	if (!mm || new_size == 0) return -EINVAL;

	uintptr_t aligned_start = ALIGN_DOWN(start_addr, PAGE_SIZE);
	size_t aligned_size = ALIGN_UP(new_size, PAGE_SIZE);
	uintptr_t new_end = aligned_start + aligned_size;

	spin_lock(&mm->lock);

	struct vm_region* curr = vma_lookup_locked(mm, aligned_start);

	if (!curr) {
		spin_unlock(&mm->lock);
		return -ENOENT;
	}

	if (new_end > curr->end) {
		if (curr->next && new_end > curr->next->start) {
			spin_unlock(&mm->lock);
			return -ENOSPC;
		}
	}

	curr->end = new_end;

	spin_unlock(&mm->lock);
	return SUCCESS;
}


int vma_remove_range(struct mm_struct* mm, uintptr_t start, uintptr_t end) {
	if(!mm || start >= end) return -EINVAL;
	if(start & (PAGE_SIZE - 1)) return -EINVAL;

	end = ALIGN_UP(end, PAGE_SIZE);
	if(end <= start) return -EINVAL;

	struct vm_region* spare = kzalloc(sizeof(struct vm_region));
	if(!spare) return -ENOMEM;

	struct vm_region* dead = NULL;
	struct vm_region* prev = NULL;

	spin_lock(&mm->lock);
	
	struct vm_region* curr = mm->vma;

	while(curr && curr->start < end) {
		struct vm_region* next = curr->next;

		if(curr->end <= start) {
			prev = curr;
			curr = next;
			continue;
		}

		if(start <= curr->start && end >= curr->end) {
			if(prev) prev->next = next;
			else mm->vma = next;

			curr->next = dead;
			dead = curr;
			curr = next;
			continue;
		}

		if(start <= curr->start) {
			if(curr->file) {
				curr->file_offset += (off_t)(end - curr->start);
			}
			curr->start = end;
		} 
		else if(end >= curr->end) {
			curr->end = start;
		} 
		else {
			spare->start      = end;
			spare->end        = curr->end;
			spare->mem_flags  = curr->mem_flags;
			spare->prot_flags = curr->prot_flags;
			spare->file       = curr->file;

			if(curr->file) {
				spare->file_offset = curr->file_offset + (off_t)(end - curr->start);
				file_get(curr->file);
			}

			spare->next = next;
			curr->end   = start;
			curr->next  = spare;
			
			spare = NULL; // Consumed
		}

		prev = curr;
		curr = next;
	}

	spin_unlock(&mm->lock);

	while(dead) {
		struct vm_region* n = dead->next;
		if(dead->file) file_put(dead->file);
		kfree(dead);
		dead = n;
	}

	if(spare) kfree(spare);
	return SUCCESS;
}

struct mm_struct* mm_alloc(void){
	struct mm_struct* mm = kzalloc(sizeof(struct mm_struct));

	if(mm){
		atomic_set(&mm->refcount, 1);
		spinlock_init(&mm->lock);
	}

	return mm;
}

void mm_clean(struct mm_struct* mm){
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

void mm_destroy(struct mm_struct* mm){
	if(mm->vma) mm_clean(mm);
	if(mm->ctx) mmu_destroy_context(mm->ctx);
	kfree(mm);
}

struct mm_struct* mm_dup(struct mm_struct* mm){
	struct mm_struct* new_mm = mm_alloc();
	if(!new_mm){
		return NULL;
	}

	spin_lock(&mm->lock);
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
			spin_unlock(&mm->lock);
			goto out_free;
		}

		cur = cur->next;
	}
	spin_unlock(&mm->lock);

	struct paging_ctx* cloned_ctx = mmu_clone_context(mm->ctx);
	if(!cloned_ctx){
		goto out_free;
	}
	
	new_mm->ctx = cloned_ctx;
	new_mm->brk = mm->brk;
	new_mm->brk_start = mm->brk_start;

	return new_mm;

out_free:
	mm_destroy(new_mm);
	return NULL;
}

