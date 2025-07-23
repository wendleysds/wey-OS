#include <memory/mm.h>
#include <core/kernel.h>
#include <lib/mem.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)virt % PAGING_PAGE_SIZE) || \
	((uintptr_t)phys % PAGING_PAGE_SIZE))

extern void paging_load_directory(uint32_t* addr);
static uint32_t* currentDirectoryAddr = 0x0;

static int _paging_is_aligned(void* addr){
	return ((uintptr_t)addr % PAGING_PAGE_SIZE) == 0;
}

static int _get_indexes(void* virtualAddr, uint32_t* outDirIndex, PagingTable* outTabIndex){
	if(!_paging_is_aligned(virtualAddr)){
		return BAD_ALIGNMENT;
	}

	*outDirIndex = ((uintptr_t)virtualAddr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
	*outTabIndex = ((uintptr_t)virtualAddr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);
	return SUCCESS;
}

static uint32_t _get_page(struct PagingDirectory* directory, void* virt){
	uint8_t status = SUCCESS;
    uint32_t dirIndex = 0;
    uint32_t tblIndex = 0;
    if((status = _get_indexes(virt, &dirIndex, &tblIndex)) != SUCCESS){
		return status;
	}
    
	uint32_t entry = directory->entry[dirIndex];
    uint32_t* table = (uint32_t*)(entry & 0xFFFF000);
    return table[tblIndex];
}

static void* _get_physical_address(struct PagingDirectory* directory, void* virt){
    void* lowVirt = (void*) paging_align_to_lower(virt);
    void* difference = (void*)((uintptr_t) virt - (uintptr_t) lowVirt);
    return (void*)((_get_page(directory, lowVirt) & 0xfffff000) + difference);
}

void* paging_align_address(void* ptr){
	uintptr_t addr = (uintptr_t)ptr;
	if (addr & (PAGING_PAGE_SIZE - 1))
	{
		addr = (addr + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
		return (void*)addr;
	}

	return ptr;
}

void* paging_align_to_lower(void* addr){
    return (void*)((uintptr_t)addr & ~(PAGING_PAGE_SIZE - 1));
}

struct PagingDirectory* paging_new_directory(uint32_t tablesAmount, uint8_t flags){
	struct PagingDirectory* directory = (struct PagingDirectory*)kcalloc(sizeof(struct PagingDirectory));

	if(!directory)
		return 0x0;

	uint32_t* entry = (uint32_t*)kcalloc(sizeof(uint32_t) * tablesAmount);

	if(!entry){
		kfree(directory);
		return 0x0;
	}

	int offset = 0;

  for (uint32_t i = 0; i < tablesAmount; i++)
  {
		PagingTable* table = (PagingTable*)kcalloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);

		if(!table){
			directory->entry = entry;
			directory->tableCount = tablesAmount - (i + 1);
			return directory;
		}

		for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++){
			table[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
		}

		offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
		entry[i] = (PagingTable)table | flags;
	}

	directory->entry = entry;
	directory->tableCount = tablesAmount;

	return directory;
}

void paging_switch(struct PagingDirectory *directory){
	paging_load_directory(directory->entry);
	currentDirectoryAddr = directory->entry;
}

void paging_free_directory(struct PagingDirectory *directory){
	for(size_t i = 0; i < directory->tableCount; i++){
		PagingTable tableEntry = directory->entry[i];
		PagingTable* table = (PagingTable*)(tableEntry & 0xFFFFF000);
		kfree(table);
	}

	kfree(directory->entry);
	kfree(directory);
}

int paging_map(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint8_t flags){
	if(_ADDRS_NOT_ALING(virtualAddr, physicalAddr)){
		return BAD_ALIGNMENT;
	}

	if(!_paging_is_aligned(virtualAddr)){
		return BAD_ALIGNMENT;
	}

	uint32_t dirIndex, tblIndex;
	if(_get_indexes(virtualAddr, &dirIndex, &tblIndex) != SUCCESS){
		return BAD_ALIGNMENT;
	}

	if(!(directory->entry[dirIndex] & FPAGING_P)){
		PagingTable* newTable = (PagingTable*)kcalloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
		if(!newTable){
			return NO_MEMORY;
		}

		memset(newTable, 0, sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
		directory->entry[dirIndex] = (PagingTable)newTable | FPAGING_P | FPAGING_RW | FPAGING_US;
	}

	PagingTable *table = (PagingTable*)(directory->entry[dirIndex] & 0xFFFFF000);
	table[tblIndex] = (uint32_t) physicalAddr | flags;

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

int map_pages(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags){
	if (_ADDRS_NOT_ALING(virtualAddr, physicalAddr)) {
        return BAD_ALIGNMENT;
    }

	uint32_t alignedSize = (size + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
	uint32_t count = alignedSize / PAGING_PAGE_SIZE;

	return paging_map_range(directory, count, virtualAddr, physicalAddr, flags);
}
