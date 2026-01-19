#ifndef _VMA_H
#define _VMA_H

#include <asm/paging.h>
#include <stdint.h>

#define MAP_POPULATE  0x1
#define MAP_FIXED     0x2
#define MAP_PRIVATE   0x4
#define MAP_ANONYMOUS 0x8

typedef enum {
	// Memory flags
	MEM_READ   = 1 << 0,
	MEM_WRITE  = 1 << 1,
	MEM_KERNEL = 1 << 2,
	MEM_USER   = 1 << 3,
	// Process flags / flags for mem_region
	MEM_EXEC   = 1 << 4,
	MEM_SHARED = 1 << 5,
	MEM_LOCKED = 1 << 6,
} mem_flags_t;

struct mem_region {
	uintptr_t start;
	uintptr_t end;
	mem_flags_t mem_flags;
	unsigned int mpa_flags;
	struct mem_region *next;
};

struct mm_struct {
	pgd_t *pgd;
	struct mem_region *vma;
	uintptr_t brk_start;
	uintptr_t brk;
	uint16_t refcount;
};

#endif
