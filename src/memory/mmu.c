#include <mmu.h>
#include <memory/paging.h>
#include <def/status.h>
#include <core/kernel.h>
#include <stdint.h>

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

void mmu_init(struct PagingDirectory** kernelDirectory){
	struct PagingDirectory* dir;

	uint32_t tableAmount = PAGING_TOTAL_ENTRIES_PER_TABLE;
	dir = paging_new_directory(
		tableAmount, 
		(FPAGING_P | FPAGING_RW), 
		(FPAGING_P | FPAGING_RW)
	);

	if(!dir || dir->tableCount != tableAmount){
		panic("Failed to initializing paging!");
	}

	*kernelDirectory = dir;

	paging_switch(dir);
	enable_paging();
}

struct PagingDirectory* mmu_create_page(){
	return paging_new_directory(
		PAGING_TOTAL_ENTRIES_PER_TABLE, 
		(FPAGING_P | FPAGING_RW | FPAGING_US), 
		(FPAGING_RW | FPAGING_US)
	);
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