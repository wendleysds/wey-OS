#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#include <mm/paging.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <stdint.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_POPULATE  0x0
#define MAP_FIXED     0x1
#define MAP_PRIVATE   0x2
#define MAP_ANONYMOUS 0x4

#define SELF_PDE_INDEX 1023

#define VIRT_PDIR ((uint32_t*)0xFFFFF000)
#define VIRT_PTBL(i) ((uint32_t*)(0xFFC00000 + (i) * PAGING_PAGE_SIZE))

struct mem_region {
	void* physBaseAddress;
    void* virtualBaseAddress;
    void* virtualEndAddress;
	
	uint32_t size;
	uint8_t flags;

	uint8_t isPrivite;
    struct mem_region* next;
}__attribute__((packed));

struct mm_struct{
    struct mem_region* regions;
    void* mmapBase; // Next free space

    struct PagingDirectory* pageDirectory;
};

extern struct PagingDirectory* _currentDirectory;

int mmu_init();

struct PagingDirectory* mmu_create_page();
int mmu_page_switch(struct PagingDirectory* directory);
int mmu_destroy_page(struct PagingDirectory* directory);
int mmu_map_pages(void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags);
int mmu_unmap_pages(void* virtualStart, uint32_t size);
void* mmu_translate(void* virt);
uint8_t mmu_user_pointer_valid(void* ptr);
uint8_t mmu_user_pointer_valid_range(const void* userPtr, size_t size);

void mmu_copy_kernel_to_directory(struct PagingDirectory* directory);

struct mem_region* vma_lookup(struct mm_struct* mm, void* virtualAddr);
int vma_add(struct mm_struct* mm, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags, uint8_t isPrivate);
int vma_remove(struct mm_struct* mm, void* virtualAddr, uint32_t size);
int vma_clean(struct mm_struct* mm);
int vma_destroy(struct mm_struct* mm);

void* phys_to_virt(void* physicalAddr);

#endif
