#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <stddef.h>

void init_kheap();
void* kmalloc(size_t size);
void* kcalloc(size_t size);
void kfree(void* ptr);

#endif
