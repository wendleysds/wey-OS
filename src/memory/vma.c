#include <mmu.h>
#include <def/err.h>

struct mem_region* vma_lookup(struct mm_struct* mm, void* virtualAddr){
	if(!mm || !virtualAddr){
		return 0x0;
	}

	struct mem_region* region = mm->regions;
	while(region){
		if(virtualAddr >= region->virtualBaseAddress && virtualAddr < region->virtualEndAddress){
			return region;
		}

		region = region->next;
	}

	return 0x0;
}

int vma_add(struct mm_struct* mm, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags, uint8_t isPrivate){
	if (!mm || !mm->pageDirectory) {
		return NULL_PTR;
	}

	struct mem_region* region = (struct mem_region*)kzalloc(sizeof(struct mem_region));
	if (!region) {
		return NO_MEMORY;
	}

	region->physBaseAddress = physicalAddr;

	region->virtualBaseAddress = virtualAddr;
	region->virtualEndAddress = paging_align_address((void*)((uintptr_t)virtualAddr + size));

	region->flags = flags;
	region->size = size;
	region->isPrivite = isPrivate;

	region->next = mm->regions;
	mm->regions = region;

	return SUCCESS;
}

int vma_remove(struct mm_struct* mm, void* virtualAddr, uint32_t size){
	if (!mm || !mm->pageDirectory) {
		return NULL_PTR;
	}

	struct mem_region* prev = NULL;
	struct mem_region* current = mm->regions;

	while (current) {
		if (current->virtualBaseAddress == virtualAddr && current->size == size) {
			if (prev) {
				prev->next = current->next;
			} else {
				mm->regions = current->next;
			}

			if(current->physBaseAddress){
				kfree(current->physBaseAddress);
			}

			kfree(current);
			break;
		}
		prev = current;
		current = current->next;
	}

	return SUCCESS;
}

int vma_clean(struct mm_struct* mm){
	if (!mm) {
		return NULL_PTR;
	}

	struct mem_region* current = mm->regions;
	while (current) {
		struct mem_region* next = current->next;

		if(current->isPrivite){
			if(current->virtualBaseAddress && current->size > 0){
				mmu_unmap_pages(mm->pageDirectory, current->virtualBaseAddress, current->size);
			}

			if(current->physBaseAddress){
				kfree(current->physBaseAddress);
			}

			kfree(current);
		}
		
		current = next;
	}

	return SUCCESS;
}

int vma_destroy(struct mm_struct* mm){
	if (!mm) {
		return NULL_PTR;
	}

	vma_clean(mm);

	if (mm->pageDirectory) {
		mmu_destroy_page(mm->pageDirectory);
	}

	kfree(mm);

	return SUCCESS;
}
