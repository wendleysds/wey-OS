#include <mm/slab.h>
#include <mm/page.h>
#include <def/config.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/assert.h>
#include <asm/paging.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#define NUM_SLAB_SIZES ARRAY_SIZE(slab_sizes)

static const size_t slab_sizes[] = SLAB_SIZES;
static struct slab_cache slab_caches[NUM_SLAB_SIZES];

static int get_cache_index(size_t size){
	for (int i = 0; i < NUM_SLAB_SIZES; i++) {
		if (size <= slab_sizes[i]) {
			return i;
		}
	}

	return -1; // too big
}

static struct slab* slab_create(struct slab_cache* cache){
	// metadata + objs
	struct page* page = page_alloc(0, PG_KERNEL | PG_SLAB);
	if(!page)
		return NULL;

	uintptr_t virt = page_to_virt(page);

	struct slab* slab = (struct slab*)virt;
	memset(slab, 0x0, sizeof(*slab));

	slab->page = page;
	page->private = slab;

	uintptr_t obj_start = virt + sizeof(struct slab);
	slab->start = (void*)obj_start;
	slab->total_objects = (PAGE_SIZE - (obj_start - virt)) / 
		cache->object_size;

	// Initialize free list
	void* obj = slab->start;
	for(uint32_t i = 0; i < slab->total_objects; i++){
		*(void**)obj = slab->free_list;
		slab->free_list = obj;
		obj += cache->object_size;
	}

	slab->free_count = slab->total_objects;

	INIT_LIST_HEAD(&slab->list);
	return slab;
}

static void slab_destroy(struct slab* slab){
	page_free(slab->page);
}

void __init slab_init(){
	for (int i = 0; i < NUM_SLAB_SIZES; i++) {
		slab_caches[i].object_size = slab_sizes[i];
		INIT_LIST_HEAD(&slab_caches[i].slabs);
	}
}

void *slab_alloc(size_t size){
	if(size == 0)
		return NULL;

	void *obj = NULL;
	int cache_idx = get_cache_index(size);
	if(cache_idx < 0){
		return NULL;
	}

	struct slab_cache *cache = &slab_caches[cache_idx];
	struct slab *slab = NULL;

	list_for_each_entry(slab, &cache->slabs, list) {
		if (slab->free_count > 0)
			goto found;
	}

	slab = slab_create(cache);
	if (!slab)
		return NULL;

	list_add(&slab->list, &cache->slabs);

found:
	obj = slab->free_list;
	slab->free_list = *(void **)obj;
	slab->free_count--;
	return obj;
}

void slab_free(void* ptr){
	if (!ptr)
		return;

	struct page* page = virt_to_page((uintptr_t)ptr);
	if(!(page->flags & PG_SLAB)){
		return;
	}

	struct slab* slab = page->private;

	if (!slab){
		return;
	}

	BUG_ON(slab->page != page);

	*(void**)ptr = slab->free_list;
	slab->free_list = ptr;
	slab->free_count++;

	if (slab->free_count == slab->total_objects) {
		list_remove(&slab->list);
		slab_destroy(slab);
	}
}
