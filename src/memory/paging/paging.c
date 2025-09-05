#include "drivers/terminal.h"
#include <mmu.h>
#include <core/kernel.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

static inline void _invlpg(void* virtAddr){
	__asm__ volatile("invlpg (%0)" : : "r"(virtAddr) : "memory");
}

static inline void _get_indexes(void* virtualAddr, uint32_t* outDirIndex, uint32_t* outTabIndex){
	uintptr_t virt = (uintptr_t)virtualAddr;
	*outDirIndex = virt >> 22;
	*outTabIndex = (virt >> 12) & 0x3FF;
}

static inline uint32_t _get_page(struct PagingDirectory* directory, void* virt){
    uint32_t dirIndex, tblIndex;
    _get_indexes(virt, &dirIndex, &tblIndex);
    
	uint32_t entry = directory->entry[dirIndex];
	if (!(entry & FPAGING_P)) return 0;

    uint32_t* table = (uint32_t*)(entry & PAGE_MASK);
    return table[tblIndex];
}

void* paging_translate(struct PagingDirectory* directory, void* virt){
	uintptr_t lowVirt = (uintptr_t)((uintptr_t)virt & ~(PAGING_PAGE_SIZE - 1));
	uintptr_t difference = (uintptr_t) virt - lowVirt;

	uintptr_t pagePhysic = _get_page(directory, (void*)lowVirt) & PAGE_MASK;
	if (pagePhysic == 0) return NULL;

	return (void*)(pagePhysic + difference);
}

struct PagingDirectory* paging_new_directory(){
    struct PagingDirectory* directory = (struct PagingDirectory*)kcalloc(sizeof(struct PagingDirectory), 1);
    if (!directory){
		return NULL;
	}

    directory->entry = (uint32_t*)kcalloc(sizeof(uint32_t), PAGING_TOTAL_ENTRIES_PER_TABLE);
    if (!directory->entry) {
        kfree(directory);
        return NULL;
    }

    directory->tableCount = 0;
    return directory;
}

void paging_free_directory(struct PagingDirectory *directory){
	for(size_t i = 0; i < directory->tableCount; i++){
		PagingTable tableEntry = directory->entry[i];

		if(tableEntry & FPAGING_P){
			PagingTable* table = (PagingTable*)(tableEntry & PAGE_MASK);
			kfree(table);
		}
	}

	kfree(directory->entry);
	kfree(directory);
}

int paging_map(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint8_t flags){
	if(_ADDRS_NOT_ALING(virtualAddr, physicalAddr)){
		return BAD_ALIGNMENT;
	}

	uint32_t dirIndex, tblIndex;
	_get_indexes(virtualAddr, &dirIndex, &tblIndex);

	PagingTable* table = 0x0;
	if(!(directory->entry[dirIndex] & FPAGING_P)){
		table = (PagingTable*)kcalloc(sizeof(PagingTable), PAGING_TOTAL_ENTRIES_PER_TABLE);
		if (!table){
			return NO_MEMORY;
		}

		directory->tableCount++;
		directory->entry[dirIndex] = (PagingTable)mmu_translate(table) | flags;
	}else{
		uintptr_t tableVirt = (uintptr_t)phys_to_virt((void*)directory->entry[dirIndex]);
		table = (PagingTable*)(tableVirt & PAGE_MASK);
	}

	if (table[tblIndex] & FPAGING_P) {
		return ALREADY_MAPD;
	}

	table[tblIndex] = (uint32_t) physicalAddr | flags;

	return SUCCESS;
}

int paging_unmap(struct PagingDirectory* directory, void* virtual){
	uint32_t dirIndex, tblIndex;
	_get_indexes(virtual, &dirIndex, &tblIndex);

	if (!(directory->entry[dirIndex] & FPAGING_P)) {
		return ALREADY_UMAPD;
	}

	uintptr_t tableVirt = (uintptr_t)phys_to_virt((void*)directory->entry[dirIndex]);
	PagingTable *table = (PagingTable*)(tableVirt & PAGE_MASK);
	table[tblIndex] = 0;

	_invlpg(virtual);

	for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
		if (table[i] & FPAGING_P) {
			return SUCCESS;
		}
	}

	kfree(table);
	directory->entry[dirIndex] = 0x0;
	directory->tableCount--;

	return SUCCESS;
}

int paging_map_range(struct PagingDirectory* directory, int count, void* virtualAddr, void* physicalAddr, uint8_t flags){
	for(int i = 0; i < count; i++){
		int status = paging_map(directory, virtualAddr, physicalAddr, flags);
		if(status != SUCCESS)
			return status;

		virtualAddr = (void*)((uintptr_t)virtualAddr + PAGING_PAGE_SIZE);
		physicalAddr = (void*)((uintptr_t)physicalAddr + PAGING_PAGE_SIZE);
	}

	return SUCCESS;
}

int paging_unmap_range(struct PagingDirectory* directory, int count, void* virtual){
	for (int i = 0; i < count; i++) {
		int status = paging_unmap(directory, virtual);
		if (status != SUCCESS)
			return status;

		virtual = (void*)((uintptr_t)virtual + PAGING_PAGE_SIZE);
	}
	return SUCCESS;
}
