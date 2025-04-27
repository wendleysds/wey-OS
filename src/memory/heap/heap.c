#include <memory/heap.h>
#include <lib/mem.h>
#include <def/config.h>
#include <def/status.h>
#include <stdint.h>

static int _validade_table(struct HeapTable* table, void* ptr, void* end){
	size_t size = (size_t)(end - ptr);
	size_t totalBlocks = size / HEAP_BLOCK_SIZE;

	if(table->total != totalBlocks){
		return FAILED;
	}

	return SUCCESS;
}

static int _is_aligned(void* ptr){
	return ((unsigned int)ptr % HEAP_BLOCK_SIZE) == 0;
}

int create_heap(struct Heap *heap, struct HeapTable *table, void *startPtr, void *end){

	if(!_is_aligned(startPtr) || !_is_aligned(end)){
		return INVALID_VARG;
	}

	memset(heap, 0, sizeof(struct Heap));
	heap->startAddress = startPtr;
	heap->table = table;

	if(_validade_table(table, startPtr, end)){
		return FAILED;
	}

	size_t tableSize = sizeof(uint8_t) * table->total;
	memset(table->blockEntries, 0x00, tableSize);

	return SUCCESS;
}

