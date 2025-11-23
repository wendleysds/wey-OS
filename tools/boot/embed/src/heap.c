#include <heap.h>
#include <string.h>

// Basic bump allocator
// Note: Not thread-safe, no free implementation

static size_t pointer = 0;
static size_t heap_end = 0;

void heap_init(void* heap_start, size_t heap_size){
	pointer = (size_t)heap_start;
	heap_end = pointer + heap_size;
	memset(heap_start, 0, heap_size);
}

void* malloc(size_t size){
	if (pointer + size > heap_end){
		return NULL;
	}

	size_t current = pointer;
	pointer += size;

	memset((void*)current, 0, size);
	return (void*)current;
}