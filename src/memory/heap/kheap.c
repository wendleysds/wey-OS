#include <memory/kheap.h>
#include <memory/heap.h>

#include <drivers/terminal.h>
#include <core/kernel.h>
#include <def/config.h>
#include <lib/mem.h>

/*
 * Kernel heap manager
 */

#include <stdint.h>

static struct Heap kernelHeap;
static struct HeapTable kernelHeapTable;

int init_kheap(){
	kernelHeapTable.blockEntries = (uint8_t*) HEAP_TABLE_ADDRESS;
	kernelHeapTable.total = HEAP_BSIZE / HEAP_BLOCK_SIZE;

	void* end = (void*)(HEAP_ADDRESS + HEAP_BSIZE);
	return create_heap(&kernelHeap, &kernelHeapTable, (void*)HEAP_ADDRESS, end);
}

void* kmalloc(size_t size){
	return hmalloc(&kernelHeap, size);
}

void* kcalloc(size_t nmemb, size_t size){
	return hcalloc(&kernelHeap, nmemb, size);
}

void kfree(void *ptr){
	hfree(&kernelHeap, ptr);
}

