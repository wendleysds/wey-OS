#ifndef _MEMBLOCK_H
#define _MEMBLOCK_H

#include <stdint.h>
#include <stddef.h>

#define MAX_REGIONS 128

struct memblock_region {
	uint64_t base;
	size_t size;
};

struct memblock_type {
	const char* name;
	struct memblock_region regions[MAX_REGIONS];
	size_t count;
	uint64_t totalmem;
};

struct memblock {
	struct memblock_type memory;
	struct memblock_type reserved;
};

extern struct memblock memblock;
extern unsigned long max_low_pfn;
extern unsigned long max_pfn;

void memblock_init();
void memblock_add(uint64_t base, size_t size);
void memblock_reserve(uint64_t base, size_t size);
int memblock_is_reserved(uint64_t base, size_t size);
void* memblock_alloc(size_t size, size_t align);
void* memblock_alloc_range(uint64_t base, uint64_t end, size_t size);
void memblock_dump_all(void);

#endif
