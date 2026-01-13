#ifndef _EARLY_ALLOC_H
#define _EARLY_ALLOC_H

#include <stddef.h>

// used in .text.boot
void early_alloc_init_boot();
void* early_alloc_boot(size_t size);

// used in .text
void early_alloc_init();
void* early_alloc(size_t size);

#endif
