#include <mmu.h>
#include <core/kernel.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Paging Controller and Loader
 */

#define _MASK 0xFFFFF000

// Flags when the paging_map create a directory
#define DEFAULT_DIR_FLAGS (FPAGING_P | FPAGING_US | FPAGING_RW)

#define _ADDRS_NOT_ALING(virt, phys) \
	(((uintptr_t)(virt) & (PAGING_PAGE_SIZE - 1)) || \
	((uintptr_t)(phys) & (PAGING_PAGE_SIZE - 1)))

extern void paging_load_directory(uint32_t* addr);
static struct PagingDirectory* _currentDirectory = 0x0;

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

static inline void _invlpg(void* virtAddr){
	__asm__ volatile("invlpg (%0)" : : "r"(virtAddr) : "memory");
}

void* paging_translate(struct PagingDirectory* directory, void* virt){
	uintptr_t lowVirt = (uintptr_t)((uintptr_t)virt & ~(PAGING_PAGE_SIZE - 1));
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

struct PagingDirectory* paging_new_directory(uint32_t tablesAmount, uint8_t dirFlags, uint8_t tblFlags){
	if(tablesAmount > PAGING_TOTAL_ENTRIES_PER_TABLE || tablesAmount < 0){
		return 0x0; // Invalid tables amount
	}

	if(tablesAmount == 0){
		return paging_new_directory_empty();
	}

	struct PagingDirectory* directory = (struct PagingDirectory*)kcalloc(sizeof(struct PagingDirectory));
	if(!directory)
		return 0x0;

	uint32_t* entry = (uint32_t*)kcalloc(sizeof(uint32_t) * tablesAmount);
	if(!entry){
		kfree(directory);
		return 0x0;
	}

	uint32_t offset = 0;

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
			table[j] = (offset + (j * PAGING_PAGE_SIZE)) | tblFlags;
		}

		offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
		entry[i] = (PagingTable)table | dirFlags | FPAGING_P;
	}

	directory->entry = entry;
	directory->tableCount = tablesAmount;

	return directory;
}

struct PagingDirectory* paging_new_directory_empty(){
    struct PagingDirectory* directory = (struct PagingDirectory*)kcalloc(sizeof(struct PagingDirectory));
    if (!directory){
		return NULL;
	}

    directory->entry = (uint32_t*)kcalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    if (!directory->entry) {
        kfree(directory);
        return NULL;
    }

    directory->tableCount = 0;
    return directory;
}


void paging_switch(struct PagingDirectory *directory){
	if(!directory){
		panic("paging_switch(*directory): Directory is null!");
	}

	if (_currentDirectory == directory) {
		return;
	}

	if (!directory->entry) {
		panic("paging_switch(*directory): Directory entry is null!");
	}

	if ((uintptr_t)(directory->entry) & (PAGING_PAGE_SIZE - 1)) {
		panic("paging_switch(*directory): Directory entry is not page-aligned!");
	}

	if (directory->tableCount == 0) {
		panic("paging_switch(*directory): Directory has no tables!");
	}

	if (directory->tableCount > PAGING_TOTAL_ENTRIES_PER_TABLE) {
		panic("paging_switch(*directory): Directory table count exceeds 1024!");
	}

	paging_load_directory(directory->entry);
	_currentDirectory = directory;
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

	PagingTable* table = 0x0;
	if(!(directory->entry[dirIndex] & FPAGING_P)){
		if (!table) {
			table = (PagingTable*)kcalloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);
			if (!table){
				return NO_MEMORY;
			}

			directory->tableCount++;
			directory->entry[dirIndex] = (PagingTable)table | DEFAULT_DIR_FLAGS;
		}else{
			table = (PagingTable*)(directory->entry[dirIndex] & _MASK);
		}
	}

	if (table[tblIndex] & FPAGING_P) {
		return ALREADY_MAPD;
	}

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

int paging_unmap(struct PagingDirectory* directory, void* virtual){
	uint32_t dirIndex, tblIndex;
	_get_indexes(virtual, &dirIndex, &tblIndex);

	if (!(directory->entry[dirIndex] & FPAGING_P)) {
		return ALREADY_UMAPD;
	}

	PagingTable *table = (PagingTable*)(directory->entry[dirIndex] & _MASK);
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

int paging_unmap_range(struct PagingDirectory* directory, int count, void* virtual){
	for (int i = 0; i < count; i++) {
		int status = paging_unmap(directory, virtual);
		if (status != SUCCESS)
			return status;

		virtual = (void*)((uintptr_t)virtual + PAGING_PAGE_SIZE);
	}
	return SUCCESS;
}
