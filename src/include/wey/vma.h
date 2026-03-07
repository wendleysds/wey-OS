#ifndef _VMA_H
#define _VMA_H

#include <asm/paging.h>
#include <wey/vfs.h>
#include <wey/spinlock.h>
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
	MEM_GLOBAL = 1 << 4,
	MEM_DEVICE = 1 << 5,
	MEM_CACHE  = 1 << 6,
	MEM_GROWSDOWN = 1 << 7,

	// Process flags / flags for mem_region
	MEM_EXEC   = 1 << 8,
	MEM_SHARED = 1 << 9,
	MEM_LOCKED = 1 << 10,
} mem_flags_t;

struct mem_region {
	uintptr_t start;
	uintptr_t end;
	mem_flags_t mem_flags;
	unsigned int mpa_flags;
	struct file* file;
	off_t file_offset;
	struct mem_region *next;
};

struct mm_struct {
	pgd_t *pgd;
	struct mem_region *vma;
	uintptr_t brk_start;
	uintptr_t brk;
	atomic_t refcount;
	spinlock_t spinlock;
};

struct mm_struct* vma_alloc(pgd_t *pgd);

struct mem_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr);

int vma_add(
	struct mm_struct* mm, uintptr_t start, uintptr_t end,
	mem_flags_t mem_flags, unsigned int mpa_flags,
	struct file* file, off_t file_offset
);

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr);

void vma_destroy(struct mm_struct* mm);

static inline void vma_put(struct mm_struct* mm){
	if(atomic_dec_and_test(&mm->refcount)){
		vma_destroy(mm);
	}
}

static inline void vma_get(struct mm_struct* mm){
	atomic_inc(&mm->refcount);
}

struct mm_struct* vma_dup(struct mm_struct* mm);

#endif
