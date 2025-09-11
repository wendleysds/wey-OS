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

extern uint32_t total_memory_allocated_in_blocks;

int init_kheap(){
	total_memory_allocated_in_blocks = 0;
	
	kernelHeapTable.blockEntries = (uint8_t*) HEAP_TABLE_VIRT_BASE;
	kernelHeapTable.total = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;

	return create_heap(&kernelHeap, &kernelHeapTable, (void*)HEAP_VIRT_BASE, (void*)HEAP_VIRT_END);
}

void* kmalloc(size_t size){
	return hmalloc(&kernelHeap, size);
}

void* kcalloc(size_t nmemb, size_t size){
	return hcalloc(&kernelHeap, nmemb, size);
}

void* krealloc(void *ptr, size_t newSize){
	return hrealloc(&kernelHeap, ptr, newSize);
}

void kfree(void *ptr){
	hfree(&kernelHeap, ptr);
}

