#include <def/init.h>
#include <def/config.h>
#include <def/linker.h>
#include <asm/paging.h>
#include <asm/page.h>
#include <stdint.h>
#include <stddef.h>

// Early allocator
// Synchronized boot and normal phases

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

static size_t alloc_addr __initdata = (size_t)__brk_base;
static size_t alloc_end __initdata = (size_t)__brk_limit;

void* __init early_alloc_boot(size_t size){
	size_t* alloc_addr_ptr = (size_t*)__pa(&alloc_addr);
	size_t* alloc_end_ptr = (size_t*)__pa(&alloc_end);

	uintptr_t ret = ALIGN(*alloc_addr_ptr, PAGE_SIZE);
	size = ALIGN(size, PAGE_SIZE);

	if(ret > *alloc_end_ptr){
		return NULL;
	}

	*alloc_addr_ptr = (ret + size);

	return (void*)__pa(ret);
}

void* __init early_alloc(size_t size){
	uintptr_t ret = ALIGN(alloc_addr, PAGE_SIZE);
	size = ALIGN(size, PAGE_SIZE);

	if(ret > alloc_end){
		return NULL;
	}

	alloc_addr = (ret + size);

	return (void*)ret;
}
