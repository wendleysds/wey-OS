#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE 4096

// Flags
#define FPAGING_PCD 0x16
#define FPAGING_PWT 0x8
#define FPAGING_US  0x4
#define FPAGING_RW  0x2
#define FPAGING_P   0x1

// Mask
#define PAGE_MASK 0xFFFFF000
#define FLAGS_MASK 0x00000FFF

typedef uint32_t PagingTable;

struct PagingDirectory {
	uint32_t* entry;
	uint32_t tableCount;
};

struct PagingDirectory* paging_new_directory();
void paging_free_directory(struct PagingDirectory* directory);

void paging_load_directory(uint32_t* physicalAddr);
void paging_enable();

int paging_map(void* virtualAddr, void* physicalAddr, uint8_t flags);
int paging_map_range(int count, void* virtualAddr, void* physicalAddr, uint8_t flags);

int paging_unmap(void* virtualAddr);
int paging_unmap_range(int count, void* virtualAddr);

void* paging_translate(void* virtualAddr);

static inline void* paging_align_to_lower(void* addr){
    return (void*)((uintptr_t)addr & ~(PAGING_PAGE_SIZE - 1));
}

static inline void* paging_align_address(void* ptr){
	uintptr_t addr = (uintptr_t)ptr;
	if (addr & (PAGING_PAGE_SIZE - 1))
	{
		addr = (addr + PAGING_PAGE_SIZE - 1) & ~(PAGING_PAGE_SIZE - 1);
		return (void*)addr;
	}

	return ptr;
}

#endif

