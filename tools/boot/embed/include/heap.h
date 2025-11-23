#ifndef _HEAP_H
#define _HEAP_H

#include <def/compile.h>
#include <stddef.h>

void heap_init(void* heap_start, size_t heap_size);
void* malloc(size_t size);

#endif