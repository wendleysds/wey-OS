#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <lib/mem.h>
#include <stddef.h>

int init_kheap();
void* kmalloc(size_t size);
void* kcalloc(size_t nmemb, size_t size);
void* krealloc(void *ptr, size_t newSize);
void kfree(void* ptr);

static inline void* kzalloc(size_t size) {
	void* ptr = kmalloc(size);
	if (ptr) {
		memset(ptr, 0x0, size);
	}

	return ptr;
}

#endif
