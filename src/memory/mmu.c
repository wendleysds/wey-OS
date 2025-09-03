#include "def/status.h"
#include <mmu.h>
#include <memory/paging.h>
#include <def/config.h>
#include <def/err.h>
#include <core/kernel.h>
#include <stddef.h>
#include <stdint.h>
#include <arch/i386/idt.h>
#include <drivers/terminal.h>
#include <core/sched.h>

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

struct PagingDirectory* _currentDirectory = 0x0;

#define VIRT_PD ((uint32_t*)0xFFFFF000)
#define VIRT_PT(i) \
	((uint32_t*)(0xFFC00000 + (i) * 0x1000))

static uintptr_t virt_to_phys(uintptr_t virt) {
	uint32_t dir_idx = virt >> 22;
	uint32_t tbl_idx = (virt >> 12) & 0x3FF;

	uint32_t pde = VIRT_PD[dir_idx];
	if (!(pde & 0x1)) {
		return 0;
	}

	uint32_t* pt = VIRT_PT(dir_idx);
	uint32_t pte = pt[tbl_idx];
	if (!(pte & 0x1)) {
		return 0;
	}

	uintptr_t phys = (pte & 0xFFFFF000) | (virt & 0xFFF);
	return phys;
}

int mmu_init(struct PagingDirectory** kernelDirectory){
	struct PagingDirectory* dir = mmu_create_page();

	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_phys_end;
	extern uintptr_t __kernel_high_start;

	size_t kernel_size = (size_t)&__kernel_phys_end - (size_t)&__kernel_phys_start;

	uint8_t flags = (FPAGING_P | FPAGING_RW);

	mmu_map_pages(
		dir,
		(void*)&__kernel_high_start, 
		(void*)&__kernel_phys_start, 
		kernel_size,
		flags
	);

	mmu_map_pages(
		dir,
		(void*)KERNEL_STACK_VIRT_BASE,
		(void*)KERNEL_STACK_PHYS_BASE,
		KERNEL_STACK_SIZE,
		flags
	);

	mmu_map_pages(
		dir,
		(void*)HEAP_VIRT_BASE,
		(void*)HEAP_PHYS_BASE,
		HEAP_SIZE_BYTES,
		flags
	);

	mmu_map_pages(
		dir, 
		(void*)HEAP_TABLE_VIRT_BASE, 
		(void*)HEAP_TABLE_PHYS_BASE, 
		(HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE), 
		flags
	);

	extern struct VideoStructPtr* _get_video();
	struct VideoStructPtr* vPtr = _get_video();

	mmu_map_pages(
		dir,
		(void*)KERNEL_FB_VIRT_BASE,
		(void*)vPtr->framebuffer_physical,
		(vPtr->height * vPtr->width * (vPtr->bpp / 2)),
		flags
	);

	return NOT_IMPLEMENTED;
}

struct PagingDirectory* mmu_create_page(){
	return paging_new_directory_empty();
}

int mmu_page_switch(struct PagingDirectory* directory){
	if(!directory){
		return NULL_PTR;
	}

	paging_switch(directory);

	return SUCCESS;
}

int mmu_destroy_page(struct PagingDirectory* directory){
	if(!directory){
		return NULL_PTR;
	}

	paging_free_directory(directory);

	return SUCCESS;
}

int mmu_map_pages(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags){
	uint32_t alignedSize = (size + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
	uint32_t count = alignedSize / PAGING_PAGE_SIZE;

	return paging_map_range(directory, count, paging_align_to_lower(virtualAddr),  paging_align_to_lower(physicalAddr), flags);
}

int mmu_unmap_pages(struct PagingDirectory* directory, void* virtualStart, uint32_t size){
	uint32_t alignedSize = (size + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
	uint32_t count = alignedSize / PAGING_PAGE_SIZE;

	return paging_unmap_range(directory, count, paging_align_to_lower(virtualStart));
}

void* mmu_translate(struct PagingDirectory* directory, void* virt){
	return paging_translate(directory, virt);
}

uint8_t mmu_user_pointer_valid(void* ptr){
	if(!ptr){
		return 0;
	}
	
	return paging_is_user_pointer_valid(ptr);
}

uint8_t mmu_user_pointer_valid_range(const void* userPtr, size_t size){
    uintptr_t start = (uintptr_t)userPtr;
    uintptr_t end = start + size;

    if (end < start){
        return 0;
	}

    for (uintptr_t addr = start; addr < end; addr += PAGING_PAGE_SIZE){
        if (!mmu_user_pointer_valid((void*)addr)){
            return 0;
		}
    }

    return 1;
}

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
