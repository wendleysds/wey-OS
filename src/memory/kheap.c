#include <mm/slab.h>
#include <mm/page.h>
#include <asm/paging.h>
#include <lib/string.h>
#include <def/config.h>
#include <stdint.h>

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

void* kmalloc(size_t size){
    if (size == 0)
        return NULL;

    if (size >= SLAB_MAX_SIZE) {
        size_t pages = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;
        struct page* page = page_alloc(pages, 0x0);
        if (!page)
            return NULL;

		/*uintptr_t phys = page_to_phys(page);
		uintptr_t virt = page_to_virt(page);

		for(size_t i = 0; i < pages; i++){
			pgd_map(
				virt,
				phys,
				(_PAGE_P | _PAGE_RW)
			);

			virt += PAGE_SIZE;
			phys += PAGE_SIZE;
		}*/

        return (void*)page_to_virt(page);
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

    struct page* page = virt_to_page((uintptr_t)ptr);
    if (!page)
        return;

    if (page->private) {
        slab_free(ptr);
    }else{
		page_free(page);
	}
}
