#include <mm/page.h>
#include <mm/memblock.h>
#include <def/err.h>
#include <def/init.h>
#include <def/config.h>
#include <lib/string.h>

struct page* phys_to_page(uintptr_t phys_addr){
	return ERR_PTR(NOT_IMPLEMENTED);
}

uintptr_t page_to_phys(struct page* page){
	return NOT_IMPLEMENTED;
}

struct page* virt_to_page(uintptr_t virt_addr){
	return ERR_PTR(NOT_IMPLEMENTED);
}

uintptr_t page_to_virt(struct page* page){
	return NOT_IMPLEMENTED;
}

int __init page_init(void){
	return NOT_IMPLEMENTED;
}

struct page* _page_alloc(size_t page_count, uint32_t flags, uint8_t order){
	return ERR_PTR(NOT_IMPLEMENTED);
}

struct page* page_alloc(size_t page_count, uint16_t order){
	return ERR_PTR(NOT_IMPLEMENTED);
}

int page_free(struct page* page){
	return NOT_IMPLEMENTED;
}
