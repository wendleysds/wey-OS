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

void* paging_translate(void* virtualAddr){
	uintptr_t virt = (uintptr_t)virtualAddr;
	uint32_t dir_idx = virt >> 22;
	uint32_t tbl_idx = (virt >> 12) & 0x3FF;

	uint32_t pde = VIRT_PDIR[dir_idx];
	if (!(pde & FPAGING_P)) {
		return 0;
	}

	uint32_t* pt = VIRT_PTBL(dir_idx);
	uint32_t pte = pt[tbl_idx];
	if (!(pte & FPAGING_P)) {
		return 0;
	}

	uintptr_t phys = (pte & 0xFFFFF000) | (virt & 0xFFF);
	return (void*)phys;
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
	if(directory == _currentDirectory){
		panic("paging_free_directory(): Trying to free current directory!");
	}

	for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
		if (directory->entry[i] & FPAGING_P) {
			PagingTable* pte = (PagingTable*)VIRT_PTBL(i);
			kfree(pte);

			directory->entry[i] = 0;
		}
	}

	kfree(directory->entry);
	kfree(directory);
}

int paging_map(void* virtualAddr, void* physicalAddr, uint8_t flags){
	if(_ADDRS_NOT_ALING(virtualAddr, physicalAddr)){
		return BAD_ALIGNMENT;
	}

	uint32_t dirIndex, tblIndex;
	_get_indexes(virtualAddr, &dirIndex, &tblIndex);

	uint32_t* pde = VIRT_PDIR;
	PagingTable* pte = VIRT_PTBL(dirIndex);

	if (!(pde[dirIndex] & FPAGING_P)) {
		void* newTable = kcalloc(PAGING_TOTAL_ENTRIES_PER_TABLE, sizeof(uint32_t));
		if (!newTable) {
			return NO_MEMORY;
		}

		pde[dirIndex] = (PagingTable)paging_translate(newTable) | flags;
		pte = VIRT_PTBL(dirIndex);
		_currentDirectory->tableCount++;
	}

	if (pte[tblIndex] & FPAGING_P) {
		return ALREADY_MAPD;
	}

	pte[tblIndex] = ((uintptr_t)physicalAddr & PAGE_MASK) | flags;

	return SUCCESS;
}

int paging_unmap(void* virtualAddr){
	uint32_t dirIndex, tblIndex;
	_get_indexes(virtualAddr, &dirIndex, &tblIndex);

	uint32_t* pde = VIRT_PDIR;
	if (!(pde[dirIndex] & FPAGING_P)) {
        return ALREADY_UMAPD;
    }

	PagingTable* pte = VIRT_PTBL(dirIndex);
	if (!(pte[tblIndex] & FPAGING_P)) {
		return ALREADY_UMAPD;
	}

	pte[tblIndex] = 0;
    _invlpg(virtualAddr);

	for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
		if (pte[i] & FPAGING_P) {
			return SUCCESS;
		}
	}

	kfree(pte);
    pde[dirIndex] = 0;
    _invlpg((void*)((uintptr_t)dirIndex << 22));
	_currentDirectory->tableCount--;

	return SUCCESS;
}

int paging_map_range(int count, void* virtualAddr, void* physicalAddr, uint8_t flags){
	for(int i = 0; i < count; i++){
		int status = paging_map(virtualAddr, physicalAddr, flags);
		if(status != SUCCESS)
			return status;

		virtualAddr = (void*)((uintptr_t)virtualAddr + PAGING_PAGE_SIZE);
		physicalAddr = (void*)((uintptr_t)physicalAddr + PAGING_PAGE_SIZE);
	}

	return SUCCESS;
}

int paging_unmap_range(int count, void* virtualAddr){
	for (int i = 0; i < count; i++) {
		int status = paging_unmap(virtualAddr);
		if (status != SUCCESS)
			return status;

		virtualAddr = (void*)((uintptr_t)virtualAddr + PAGING_PAGE_SIZE);
	}
	return SUCCESS;
}
