#include <wey/mm_types.h>
#include <wey/vma.h>
#include <wey/mmu.h>
#include <mm/kheap.h>
#include <def/err.h>

struct mem_region* vma_lookup(struct mm_struct* mm, void* virtualAddr){
	if(!mm || !virtualAddr){
		return ERR_PTR(INVALID_ARG);
	}

	struct mem_region* cur = mm->regions;

	while(cur){
		if(virtualAddr >= cur->virtualBaseAddress && 
			virtualAddr < cur->virtualEndAddress
		){
			return cur;
		}

		cur = cur->next;
	}

	return ERR_PTR(NOT_FOUND);
}

struct mm_struct* vma_alloc(pgd_t* pgd){
	struct mm_struct* mm = (struct mm_struct*)kzalloc(sizeof(struct mm_struct));
	if(mm){
		mm->pgd = pgd;
	}

	return mm;
}

struct mem_region* vma_add(struct mm_struct* mm, 
	void* physicalAddr, void* virtualAddr, uint32_t size, 
	mem_flags_t mem_flags, uint8_t map_flags)
{
	if(!mm) return NULL;

	struct mem_region* mem_region = NULL;

	mem_region = vma_lookup(mm, virtualAddr);
	if(!IS_ERR(mem_region)){
		uintptr_t virtEnd = (uintptr_t)virtualAddr + size;
		if((uintptr_t)mem_region->virtualEndAddress > virtEnd){
			uint32_t new_size = (uint32_t)(virtEnd - (uintptr_t)mem_region->virtualBaseAddress);
			mem_region->virtualEndAddress = (void*)virtEnd;
			mem_region->size = new_size;
		}

		goto out;
	}

	mem_region = (struct mem_region*)kzalloc(sizeof(struct mem_region));
	if(mem_region){
		mem_region->physBaseAddress = physicalAddr;
		mem_region->virtualBaseAddress = virtualAddr;
		mem_region->virtualEndAddress = (void*)((uintptr_t)virtualAddr + size);

		mem_region->size = size;

		mem_region->map_flags = map_flags;
		mem_region->mem_flags = mem_flags;
		mem_region->next = mm->regions;
		mm->regions = mem_region;
	}

out:
	return mem_region;
}

int vma_remove(struct mm_struct* mm, struct mem_region* mm_region, void* virtualAddr, uint32_t size) {
	if (!mm) {
		return INVALID_ARG;
	}

	struct mem_region *prev = NULL, *cur = mm->regions;
	void* virtend = (void*)((uintptr_t)virtualAddr + size);

	// Find the target region
	while (cur) {
		if ((mm_region && cur == mm_region) || 
			(!mm_region && cur->virtualBaseAddress == virtualAddr && cur->size == size)) {
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	// If not found directly, try lookup by virtual address
	if (!cur) {
		cur = vma_lookup(mm, virtualAddr);
		if (!cur) {
			return NOT_FOUND;
		}
	}

	// Case 1: Exact size match - remove entire region
	if (cur->size == size) {
		if ((cur->map_flags & MAP_PRIVATE) && cur->physBaseAddress) {
			kfree(cur->physBaseAddress);
		}
		if (prev) {
			prev->next = cur->next;
		} else {
			mm->regions = cur->next;
		}
		kfree(cur);
		return SUCCESS;
	}

	// Case 2: Resize from end
	if (virtend == cur->virtualEndAddress && virtualAddr != cur->virtualBaseAddress) {
		cur->virtualBaseAddress = virtualAddr;
		cur->size = (uint32_t)virtend - (uint32_t)virtualAddr;
		return SUCCESS;
	}

	// Case 3: Resize from start
	if(virtualAddr == cur->virtualBaseAddress && virtend < cur->virtualEndAddress){
		cur->virtualEndAddress = virtend;
		cur->size = (uint32_t)virtend - (uint32_t)virtualAddr;
		return SUCCESS;
	}

	// Case 4: Invalid size
	if (cur->size > size && virtend != cur->virtualEndAddress) {
		return OUT_OF_BOUNDS;
	}

	return SUCCESS;
}

int vma_clean(struct mm_struct* mm){
	if(!mm){
		return INVALID_ARG;
	}

	struct mem_region *next, *cur = mm->regions;

	while(cur){
		next = cur->next;
		if(cur->map_flags & MAP_PRIVATE){
			if(cur->virtualBaseAddress && cur->size > 0){
				mmu_munmap(NULL, cur->virtualBaseAddress, cur->size);
			}

			if(cur->physBaseAddress){
				kfree(cur->physBaseAddress);
			}
		}

		kfree(cur);
		cur = next;
	}

	return NOT_IMPLEMENTED;
}
