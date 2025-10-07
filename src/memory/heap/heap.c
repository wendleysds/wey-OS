#include <mm/heap.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/status.h>
#include <stdint.h>

/*
 * Basic heap memory manager.
 *
 * It uses a table of fixed-size blocks to manage memory allocations manually, without
 * relying on the system's malloc or similar functions.
 *
 * The goal is to allocate, free, and reallocate memory using a custom heap structure.
 */

// flags

#define _FBLOCK_USED 0x01
#define _FBLOCK_FREE 0x00

#define _FBLOCK_HAS_NEXT 0x08
#define _FBLOCK_IS_FIRST 0x04

uint32_t total_memory_allocated_in_blocks = 0;

// Checks if the number of blocks in the heap table matches the actual memory region size
static uint8_t _validate_table(struct HeapTable *table, void *ptr, void *end)
{
	size_t size = (size_t)(end - ptr);
	size_t totalBlocks = size / HEAP_BLOCK_SIZE;

	if (table->total != totalBlocks)
	{
		return OUT_OF_BOUNDS;
	}

	return SUCCESS;
}

// Verifies if a pointer is aligned to the HEAP_BLOCK_SIZE
static inline uint8_t _is_aligned(void *ptr)
{
	return ((uintptr_t)ptr % HEAP_BLOCK_SIZE) == 0;
}

// Rounds up a value to the nearest multiple of HEAP_BLOCK_SIZE
// Optimization if the HEAP_BLOCK_SIZE is a multiple of 2 (2, 4, 8, 16, ...)
static inline uint32_t _align_value_to_block_size(uint32_t val)
{
	return (val + HEAP_BLOCK_SIZE - 1) & ~(HEAP_BLOCK_SIZE - 1);
}

// Get the entry flags (used/free)
static inline uint8_t _get_block_entry_flag(uint8_t entry)
{
	return entry & 0x0F;
}

// Finds a sequence of contiguous free blocks in the heap large
// enough to satisfy an allocation request
static int _get_start_block(struct Heap *heap, uint32_t totalBlocks)
{
	struct HeapTable *table = heap->table;
	size_t consecutiveFreeBlocks = 0;
	int startIndex = -1;

	for (size_t i = 0; i < table->total; i++)
	{
		if (_get_block_entry_flag(table->blockEntries[i]) == _FBLOCK_FREE) // Verify if current block is free
		{
			if (startIndex == -1) // if this is the first block
				startIndex = i;

			consecutiveFreeBlocks++;

			if (consecutiveFreeBlocks == totalBlocks)
			{
				return startIndex;
			}
		}
		else
		{
			consecutiveFreeBlocks = 0;
			startIndex = -1;
		}
	}

	return NO_MEMORY;
}

static void _set_blocks_free(struct Heap *heap, int startingBlock)
{
	struct HeapTable *table = heap->table;
	for (int i = startingBlock; i < table->total; i++)
	{
		uint8_t entry = table->blockEntries[i];
		uint8_t flag = _get_block_entry_flag(entry);

		total_memory_allocated_in_blocks--;

		if (flag == _FBLOCK_FREE)
		{
			break; // Already free, stop
		}

		table->blockEntries[i] = _FBLOCK_FREE;
		if (!(entry & _FBLOCK_HAS_NEXT))
		{
			break;
		}
	}
}

static void _set_blocks_taken(struct Heap *heap, int startBlock, int totalBlocks)
{
	int end_block = (startBlock + totalBlocks) - 1;

	uint8_t entry = _FBLOCK_USED | _FBLOCK_IS_FIRST;
	if (totalBlocks > 1)
	{
		entry |= _FBLOCK_HAS_NEXT;
	}

	for (int i = startBlock; i <= end_block; i++)
	{
		heap->table->blockEntries[i] = entry;
		entry = _FBLOCK_USED;
		if (i != end_block - 1)
		{
			entry |= _FBLOCK_HAS_NEXT;
		}

		total_memory_allocated_in_blocks++;
	}
}

static inline void *_block_to_address(struct Heap *heap, int block)
{
	return heap->startAddress + (block * HEAP_BLOCK_SIZE);
}

static inline int _address_to_block(struct Heap *heap, void *address)
{
	return (uintptr_t)((uintptr_t)address - (uintptr_t)heap->startAddress) / HEAP_BLOCK_SIZE;
}

// Finds free blocks and marks them as allocated, returning a pointer to the memory
static void *_malloc_blocks(struct Heap *heap, uint32_t totalBlocks)
{
	void *address = 0;

	int start_block = _get_start_block(heap, totalBlocks); // Get free area
	if (start_block < 0)
	{
		return 0x0;
	}

	address = _block_to_address(heap, start_block);

	_set_blocks_taken(heap, start_block, totalBlocks);

	return address;
}

int create_heap(struct Heap *heap, struct HeapTable *table, void *startPtr, void *end)
{
	if (!_is_aligned(startPtr) || !_is_aligned(end))
	{
		return BAD_ALIGNMENT;
	}

	memset(heap, 0, sizeof(struct Heap));
	heap->startAddress = startPtr;
	heap->table = table;

	if (_validate_table(table, startPtr, end))
	{
		return OUT_OF_BOUNDS;
	}

	size_t tableSize = sizeof(uint8_t) * table->total;
	memset(table->blockEntries, _FBLOCK_FREE, tableSize);

	return SUCCESS;
}

void *hmalloc(struct Heap *heap, size_t size)
{
	size_t aligned_size = _align_value_to_block_size(size);
	uint32_t total_blocks = aligned_size / HEAP_BLOCK_SIZE;
	return _malloc_blocks(heap, total_blocks);
}

void *hcalloc(struct Heap *heap, size_t nmemb, size_t size)
{
	// check if multiplication would overflow
	if (nmemb != 0 && size > SIZE_MAX / nmemb)
	{
		return 0x0;
	}

	size_t total_size = nmemb * size;
	void *ptr = hmalloc(heap, total_size);
	if (ptr)
	{
		memset(ptr, 0, total_size);
	}

	return ptr;
}

void *hrealloc(struct Heap *heap, void *ptr, size_t newSize)
{
	if (!ptr)
		return hmalloc(heap, newSize);

	if (newSize == 0) {
		hfree(heap, ptr);
		return 0x0;
	}

	size_t aligned_size = _align_value_to_block_size(newSize);
	int total_blocks = aligned_size / HEAP_BLOCK_SIZE;
	int start_block = _address_to_block(heap, ptr);
	if (start_block < 0)
		return 0x0;

	// Count current allocated blocks
	size_t current_blocks = 0;
	for (int i = start_block; i < heap->table->total; i++) {
		current_blocks++;
		if (!(heap->table->blockEntries[i] & _FBLOCK_HAS_NEXT))
			break;
	}

	if (current_blocks >= total_blocks)
		return ptr; // Enough space

	// Check if we can extend in place
	int extendable = 1;
	for (int i = start_block + current_blocks; i < start_block + total_blocks && i < heap->table->total; i++) {
		if (_get_block_entry_flag(heap->table->blockEntries[i]) != _FBLOCK_FREE) {
			extendable = 0;
			break;
		}
	}

	if (extendable) {
		// Mark additional blocks as used
		for (int i = start_block + current_blocks; i < start_block + total_blocks; i++) {
			uint8_t entry = _FBLOCK_USED;
			if (i != start_block + total_blocks - 1)
				entry |= _FBLOCK_HAS_NEXT;
			heap->table->blockEntries[i] = entry;
			total_memory_allocated_in_blocks++;
		}

		return ptr;
	}

	// Allocate new block and copy data
	void *new_ptr = _malloc_blocks(heap, total_blocks);
	if (!new_ptr)
		return 0x0;

	size_t copy_size = current_blocks * HEAP_BLOCK_SIZE;
	memcpy(new_ptr, ptr, copy_size);
	_set_blocks_free(heap, start_block);

	return new_ptr;
}

void hfree(struct Heap *heap, void *ptr)
{
	_set_blocks_free(heap, _address_to_block(heap, ptr));
}
