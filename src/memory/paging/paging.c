#include <mmu.h>
#include <core/kernel.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

#define _MASK 0xFFFFF000

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

extern void paging_load_directory(uint32_t* addr);
static uint32_t* currentDirectoryAddr = 0x0;

static void _get_indexes(void* virtualAddr, uint32_t* outDirIndex, uint32_t* outTabIndex){
	uintptr_t virt = (uintptr_t)virtualAddr;
	*outDirIndex = virt >> 22;
	*outTabIndex = (virt >> 12) & 0x3FF;
}

static uint32_t _get_page(struct PagingDirectory* directory, void* virt){
    uint32_t dirIndex, tblIndex;
    _get_indexes(virt, &dirIndex, &tblIndex);
    
	uint32_t entry = directory->entry[dirIndex];
	if (!(entry & FPAGING_P)) return 0;

    uint32_t* table = (uint32_t*)(entry & _MASK);
    return table[tblIndex];
}

void* paging_translate(struct PagingDirectory* directory, void* virt){
    uintptr_t lowVirt = (uintptr_t) paging_align_to_lower(virt);
    uintptr_t difference = (uintptr_t) virt - lowVirt;

	uintptr_t pagePhysic = _get_page(directory, (void*)lowVirt) & _MASK;
	if (pagePhysic == 0) return NULL;

    return (void*)(pagePhysic + difference);
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

	for (uint32_t i = 0; i < tablesAmount; i++){
		PagingTable* table = (PagingTable*)kcalloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
		if(!table){
			for(uint32_t j = 0; j < i; j++){
				kfree((void*)(entry[j] & _MASK));
			}

			kfree(entry);
			kfree(directory);
			return 0x0;
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

		if(!tableEntry & FPAGING_P){
			PagingTable* table = (PagingTable*)(tableEntry & _MASK);
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

	if(!(directory->entry[dirIndex] & FPAGING_P)){
		PagingTable* newTable = (PagingTable*)kcalloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
		if(!newTable){
			return NO_MEMORY;
		}

		memset(newTable, 0, sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
		directory->entry[dirIndex] = (PagingTable)newTable | FPAGING_P | FPAGING_RW | FPAGING_US;
	}

	PagingTable *table = (PagingTable*)(directory->entry[dirIndex] & _MASK);
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
