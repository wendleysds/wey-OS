#include <mm/slab.h>
#include <mm/page.h>
#include <asm/paging.h>
#include <lib/string.h>
#include <def/config.h>
#include <stdint.h>

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

static inline int order_for_pages(size_t pages) {
	if (pages <= 1) return 0;
	return 64 - __builtin_clzll((uint64_t)(pages - 1));
}

void* kmalloc(size_t size) {
	if (size == 0)
		return NULL;

	if (size > SLAB_MAX_SIZE) {
		size_t pages = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;
		struct page* page = page_alloc(order_for_pages(pages), PG_KERNEL);
		if (!page) {
			return NULL;
		}

		return (void*)page_to_virt(page);
	}

	return slab_alloc(size);
}

void* kcalloc(size_t nmemb, size_t size) {
	if (nmemb != 0 && size > SIZE_MAX / nmemb) {
		return NULL;
	}

	size_t total_size = nmemb * size;
	void *ptr = kmalloc(total_size);
	if (ptr) {
		memset(ptr, 0, total_size);
	}

	return ptr;
}

void kfree(void* ptr) {
	if (!ptr) {
		return;
	}

	struct page* page = virt_to_page((uintptr_t)ptr);
	if (!page) {
		return;
	}

	if ((page->flags & PG_SLAB) && page->private) {
		slab_free(ptr);
	}else{
		page_free(page);
	}
}
