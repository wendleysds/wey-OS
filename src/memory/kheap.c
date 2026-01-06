#include <mm/slab.h>
#include <mm/page.h>
#include <asm/paging.h>
#include <lib/string.h>

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

void* kmalloc(size_t size){
    if (size == 0)
        return NULL;

    if (size >= PTE_PAGE_SIZE) {
        size_t pages = ALIGN(size, PTE_PAGE_SIZE) / PTE_PAGE_SIZE;
        struct page* page = page_alloc(pages, PAGE_KERNEL);
        if (!page)
            return NULL;

        return (void*)page_to_addr(page);
    }

    void* obj = slab_alloc(size);
    return obj;
}

void* kcalloc(size_t nmemb, size_t size){
	if (nmemb != 0 && size > SIZE_MAX / nmemb)
	{
		return 0x0;
	}

	size_t total_size = nmemb * size;
	void *ptr = kmalloc(total_size);
	if (ptr)
	{
		memset(ptr, 0, total_size);
	}

	return ptr;
}

void kfree(void* ptr){
    if (!ptr)
        return;

    struct page* page = addr_to_page((uintptr_t)ptr);
    if (!page)
        return;

    if (page->slab) {
        slab_free(ptr);
    }else{
		page_free(page);
	}
}

int kfree_phys(void* ptr){
	return -1;
}
