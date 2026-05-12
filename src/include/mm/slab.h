#ifndef _SLAB_H
#define _SLAB_H

#include <lib/list.h>
#include <stdint.h>
#include <stddef.h>

// Slab allocator for small objects

#define SLAB_SIZES { 8, 16, 32, 64, 128, 256, 512, 1024, 2048 }
#define SLAB_MAX_SIZE 2048

struct slab {
	struct list_head list;  // link to other slabs in cache
	void *start;            // start of slab memory
	void *free_list;        // list of free objects
	uint32_t free_count;    // number of free objects
	uint32_t total_objects; // total objects in slab
	struct page *page;      // page allocated for this slab
};

struct slab_cache {
	size_t object_size;
	struct list_head slabs; // list of slabs
};

void slab_init();
void *slab_alloc(size_t size);
void slab_free(void *ptr);

#endif
