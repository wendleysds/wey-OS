#include <memory/heap.h>
#include <lib/mem.h>
#include <def/config.h>
#include <def/status.h>
#include <stdint.h>

//flags

#define _FBLOCK_USED 0x01
#define _FBLOCK_FREE 0x00

#define _FBLOCK_HAS_NEXT 0x08
#define _FBLOCK_IS_FIRST 0x04

static uint8_t _validate_table(struct HeapTable *table, void *ptr, void *end)
{
  size_t size = (size_t)(end - ptr);
  size_t totalBlocks = size / HEAP_BLOCK_SIZE;

  if (table->total != totalBlocks)
  {
    return FAILED;
  }

  return SUCCESS;
}

static uint8_t _is_aligned(void *ptr)
{
  return ((uintptr_t)ptr % HEAP_BLOCK_SIZE) == 0;
}

// Optimization if the HEAP_BLOCK_SIZE is a multiple of 2 (2, 4, 8, 16, ...)
static uint32_t _align_value_to_block_size(uint32_t val)
{
  return (val + HEAP_BLOCK_SIZE - 1) & ~(HEAP_BLOCK_SIZE - 1);
}

static int _get_block_status_flag(uint8_t entry)
{
  return entry & 0x0F;
}

int _get_start_block(struct Heap *heap, uint32_t totalBlocks)
{
  struct HeapTable *table = heap->table;
  size_t consecutiveFreeBlocks = 0;
  int startIndex = -1;

  for (size_t i = 0; i < table->total; i++)
  {
    if (_get_block_status_flag(table->blockEntries[i]) == _FBLOCK_FREE)
    {
      if (startIndex == -1)
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

void _set_blocks_free(struct Heap *heap, int startingBlock)
{
  struct HeapTable *table = heap->table;
  for (int i = startingBlock; i < (int)table->total; i++)
  {
    uint8_t entry = table->blockEntries[i];
    table->blockEntries[i] = _FBLOCK_FREE;
    if (!(entry & _FBLOCK_HAS_NEXT))
    {
      break;
    }
  }
}

void _set_blocks_taken(struct Heap *heap, int startBlock, int totalBlocks)
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
  }
}

void *_block_to_address(struct Heap *heap, int block)
{
  return heap->startAddress + (block * HEAP_BLOCK_SIZE);
}

int _address_to_block(struct Heap *heap, void *address)
{
  return ((int)(address - heap->startAddress)) / HEAP_BLOCK_SIZE;
}

void *_malloc_blocks(struct Heap *heap, uint32_t totalBlocks)
{
  void *address = 0;

  int start_block = _get_start_block(heap, totalBlocks);
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
    return INVALID_VARG;
  }

  memset(heap, 0, sizeof(struct Heap));
  heap->startAddress = startPtr;
  heap->table = table;

  if (_validate_table(table, startPtr, end))
  {
    return FAILED;
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

void *hcalloc(struct Heap *heap, size_t size)
{
  void *ptr = hmalloc(heap, size);
  if (!ptr)
    return 0x0;

  memset(ptr, 0x0, size);
  return ptr;
}

// Poor realloc
void *hrealloc(struct Heap *heap, void *ptr, size_t newSize)
{
  if (!ptr)
    return hmalloc(heap, newSize);

  if (newSize == 0)
  {
    hfree(heap, ptr);
    return 0x0;
  }

  void *new_ptr = hmalloc(heap, newSize);
  if (!new_ptr)
    return 0x0;

  memcpy(new_ptr, ptr, newSize);

  hfree(heap, ptr);
  return new_ptr;
}

void hfree(struct Heap *heap, void *ptr)
{
  _set_blocks_free(heap, _address_to_block(heap, ptr));
}

