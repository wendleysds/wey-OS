#ifndef _EARLY_ALLOC_H
#define _EARLY_ALLOC_H

#include <stddef.h>

void* early_alloc_boot(size_t size);
void* early_alloc(size_t size);

#endif
