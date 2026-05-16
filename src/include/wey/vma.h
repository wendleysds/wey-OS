#ifndef _VMA_H
#define _VMA_H

#include <wey/mmu.h>
#include <wey/vfs.h>
#include <wey/spinlock.h>
#include <stdint.h>

#define MAP_POPULATE  0x1
#define MAP_FIXED     0x2
#define MAP_PRIVATE   0x4
#define MAP_ANONYMOUS 0x8

#define MAX_VM_PAGE_HASH 64

struct vm_page {
	uintptr_t addr;
	struct page* page;
	struct vm_page* next;
};

struct mem_region {
	uintptr_t start;
	uintptr_t end;
	mem_flags_t mem_flags;
	unsigned int mpa_flags;
	struct file* file;
	off_t file_offset;

	struct vm_page* page_hash[MAX_VM_PAGE_HASH];

	struct mem_region *next;
};

struct mm_struct {
	struct paging_ctx *ctx;
	struct mem_region *vma;
	uintptr_t brk_start;
	uintptr_t brk;
	atomic_t refcount;
	spinlock_t spinlock;
};

struct mm_struct* vma_alloc(void);
struct mem_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr);
struct mem_region* vma_add(
	struct mm_struct* mm, uintptr_t start, uintptr_t end,
	mem_flags_t mem_flags, unsigned int mpa_flags,
	struct file* file, off_t file_offset
);

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr);
void vma_clean(struct mm_struct* mm);
void vma_destroy(struct mm_struct* mm);
struct mm_struct* vma_dup(struct mm_struct* mm);

struct page* vma_page_hash_lookup(struct mem_region* region, uintptr_t vaddr);
int vma_page_hash_insert(struct mem_region* region, uintptr_t vaddr, struct page* page);
int vma_page_hash_remove(struct mem_region* region, uintptr_t vaddr);
void vma_page_hash_destroy(struct mem_region* region);

static inline void vma_put(struct mm_struct* mm){
	if(atomic_dec_and_test(&mm->refcount)){
		vma_destroy(mm);
	}
}

static inline void vma_get(struct mm_struct* mm){
	atomic_inc(&mm->refcount);
}

#endif
