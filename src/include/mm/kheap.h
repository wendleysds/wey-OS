#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <stddef.h>
#include <lib/string.h>

void* kmalloc(size_t size);
void* kcalloc(size_t nmemb, size_t size);
void kfree(void* ptr);

static inline void* kzalloc(size_t size) {
	return kcalloc(1, size);
}

#endif
