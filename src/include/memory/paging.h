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

typedef uint32_t PagingTable;

struct PagingDirectory {
	uint32_t* entry;
	uint32_t tableCount;
};

struct PagingDirectory* paging_new_directory(uint32_t tablesAmount, uint8_t dirFlags, uint8_t tblFlags);
struct PagingDirectory* paging_new_directory_empty();
void paging_free_directory(struct PagingDirectory* directory);

void paging_switch(struct PagingDirectory* directory);
void enable_paging();

void* paging_align_to_lower(void* addr);
void* paging_align_address(void* ptr);

int paging_map(struct PagingDirectory* directory, void* virtual, void* physic, uint8_t flags);
int paging_map_range(struct PagingDirectory* directory, int count, void* virtualAddr, void* physicalAddr, uint8_t flags);

int paging_unmap(struct PagingDirectory* directory, void* virtual);
int paging_unmap_range(struct PagingDirectory* directory, int count, void* virtual);

void* paging_translate(struct PagingDirectory* directory, void* virt);
uint8_t paging_is_user_pointer_valid(void* ptr);

#endif

