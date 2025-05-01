#include "lib/utils.h"
#include <memory/paging.h>
#include <memory/kheap.h>
#include <core/kernel.h>
#include <lib/mem.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

extern void paging_load_directory(uint32_t* addr);
static uint32_t* currentDirectoryAddr = 0x0;

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

