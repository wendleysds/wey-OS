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

struct PagingDirectory* paging_new_directory(uint32_t tableAmount, uint8_t flags);
void paging_switch(struct PagingDirectory* directory);
void enable_paging();

void paging_free_directory(struct PagingDirectory* directory);

#endif

