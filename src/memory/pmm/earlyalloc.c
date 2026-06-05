#include <kernel/init.h>
#include <def/config.h>
#include <def/linker.h>
#include <lib/string.h>
#include <asm/paging.h>
#include <asm/page.h>
#include <stdint.h>
#include <stddef.h>

// Pre-memblock brk early allocator
// Synchronized boot and normal phases

#define ALIGN(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

extern unsigned long _brk_start;
extern unsigned long _brk_ptr;
extern unsigned long _brk_end;

void* __init early_alloc_boot(size_t size, size_t align){
	if(size == 0){
		return NULL;
	}

	size_t* p = (size_t*)__pa(_brk_ptr);
	size_t* e = (size_t*)__pa(_brk_end);

	if(align == 0){
		align = 1;
	}

	*p = ALIGN(*p, align);

	if(*p + size > *e){
		return NULL;
	}

	void* ret = (void*)*p;
	*p += size;

	memset(ret, 0, size);
	return ret;
}

void* __init early_alloc(size_t size, size_t align){
	if(size == 0){
		return NULL;
	}

	if(align == 0){
		align = 1;
	}

	_brk_ptr = ALIGN(_brk_ptr, align);

	if(_brk_ptr + size > _brk_end){
		return NULL;
	}

	void* ret = (void*)_brk_ptr;
	_brk_ptr += size;

	memset(ret, 0, size);
	return ret;
}
