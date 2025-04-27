#ifndef _HEAP_H
#define _HEAP_H

#include <stdint.h>
#include <stddef.h>

struct HeapTable{
	uint8_t* blockEntries;
	size_t total;
};

struct Heap{
	struct HeapTable* table;
	void* startAddress;
};

int create_heap(struct Heap* heap, struct HeapTable* table, void* startPtr, void* end);
void* hmalloc(struct Heap* heap, size_t size);
void hfree(struct Heap* heap, void* ptr);

#endif
