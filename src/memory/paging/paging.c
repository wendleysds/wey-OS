#include <memory/paging.h>
#include <memory/kheap.h>
#include <core/kernel.h>
#include <lib/mem.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

extern void paging_load_directory(uint32_t* addr);
static uint32_t* currentDirectoryAddr = 0x0;

static int _paging_is_aligned(void* addr){
	return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

static int _get_indexes(void* virtualAddr, uint32_t* outDirIndex, PagingTable* outTabIndex){
	if(!_paging_is_aligned(virtualAddr)){
		return FAILED;
	}

	*outDirIndex = ((uint32_t)virtualAddr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
	*outTabIndex = ((uint32_t)virtualAddr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);
	return OK;
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
	for(size_t i = 0; directory->tableCount < i; i++){
		PagingTable tableEntry = directory->entry[i];
		PagingTable* table = (PagingTable*)(tableEntry & 0xFFFFF000);
		kfree(table);
	}

	kfree(directory->entry);
	kfree(directory);
}

int paging_map(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint8_t flags){
	if(((uintptr_t)virtualAddr % PAGING_PAGE_SIZE) || ((uintptr_t)physicalAddr % PAGING_PAGE_SIZE)){
		return FAILED;
	}

	if(!_paging_is_aligned(virtualAddr)){
		return FAILED;
	}

	uint32_t dirIndex, tblIndex;
	if(_get_indexes(virtualAddr, &dirIndex, &tblIndex)){
		return FAILED;
	}

	if(!(directory->entry[dirIndex] & FPAGING_P)){
		return ERROR;
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

