#ifndef _VMA_H
#define _VMA_H

#include <mm/mmu.h>
#include <fs/vfs.h>

typedef enum {
	PROT_MAP_POPULATE  = 1 << 0,
	PROT_MAP_FIXED  = 1 << 1,
	PROT_MAP_PRIVATE  = 1 << 2, 
	PROT_MAP_ANONYMOUS  = 1 << 3,
} prot_flags_t;

struct vm_region {
	uintptr_t start;
	uintptr_t end;

	mem_flags_t mem_flags;
	prot_flags_t prot_flags;

	struct file* file;
	off_t file_offset;

	struct vm_region *next;
};

struct mm_struct {
	struct paging_ctx *ctx;
	struct vm_region *vma;
	uintptr_t brk_start;
	uintptr_t brk;
	atomic_t refcount;
	spinlock_t lock;
};

// vm region operations
struct vm_region* vma_lookup(struct mm_struct* mm, uintptr_t virtaddr);
struct vm_region* vma_add(
	struct mm_struct* mm, uintptr_t start, uintptr_t end,
	mem_flags_t mem_flags, prot_flags_t prot_flags,
	struct file* file, off_t file_offset
);

int vma_remove(struct mm_struct* mm, uintptr_t virtaddr);
int vma_remove_range(struct mm_struct* mm, uintptr_t start, uintptr_t end);

// mm struct operations
struct mm_struct* mm_alloc(void);
void mm_clean(struct mm_struct* mm);
void mm_destroy(struct mm_struct* mm);
struct mm_struct* mm_dup(struct mm_struct* mm);

static inline void mm_put(struct mm_struct* mm){
	if(atomic_dec_and_test(&mm->refcount)){
		mm_destroy(mm);
	}
}

static inline struct mm_struct* mm_get(struct mm_struct* mm){
	if(mm) atomic_inc(&mm->refcount);
	return mm;
}

#endif
