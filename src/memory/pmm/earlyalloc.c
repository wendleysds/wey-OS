#include <kernel/init.h>
#include <def/config.h>
#include <def/linker.h>
#include <lib/string.h>
#include <asm/paging.h>
#include <asm/page.h>
#include <stddef.h>

#include <io/ports.h>

// Pre-memblock brk early allocator
// Synchronized boot and normal phases

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

extern unsigned long _brk_start;
extern unsigned long _brk_ptr;
extern unsigned long _brk_end;

static inline size_t early_align(size_t value, size_t alignment){
	if(alignment == 0){
		alignment = 1;
	}

	return ALIGN(value, alignment);
}

static void* __init early_alloc_internal(size_t size, size_t align, size_t *ptr, size_t end){
	if(size == 0){
		return NULL;
	}

	size_t cur = early_align(*ptr, align);

	if(cur + size > end){
		return NULL;
	}

	void* ret = (void*)cur;
	*ptr = cur + size;

	memset(ret, 0, size);
	return ret;
}

void* __init early_alloc_boot(size_t size, size_t align){
	size_t *boot_brk_ptr = (size_t *)__pa(&_brk_ptr);
	size_t *boot_brk_end = (size_t *)__pa(&_brk_end);

	const size_t end = (size_t)__pa(*boot_brk_end);
	return early_alloc_internal(size, align, boot_brk_ptr, end);
}

void* __init early_alloc(size_t size, size_t align){
	return early_alloc_internal(size, align, (size_t *)&_brk_ptr, (size_t)_brk_end);
}
