#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#include <memory/paging.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <stdint.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_POPULATE  0x0
#define MAP_FIXED     0x1
#define MAP_PRIVATE   0x2
#define MAP_ANONYMOUS 0x4

struct mem_region {
    uint8_t flags;
    uint32_t size;

    void* virtualBaseAddress;
    void* virtualEndAddress;

    struct mem_region* next;
}__attribute__((packed));

struct mm_struct{
    struct mem_region* regions;
    void* mmapBase; // Next free space

    struct PagingDirectory* pageDirectory;
};

void mmu_init(struct PagingDirectory** kernelDirectory);

struct PagingDirectory* mmu_create_page();
int mmu_page_switch(struct PagingDirectory* directory);
int mmu_destroy_page(struct PagingDirectory* directory);
int mmu_map_pages(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint32_t size, uint8_t flags);
int mmu_unmap_pages(struct PagingDirectory* directory, void* virtualStart, uint32_t size);
void* mmu_translate(struct PagingDirectory* directory, void* virt);
uint8_t mmu_user_pointer_valid(void* ptr);
uint8_t mmu_user_range_valid(const void* userPtr, size_t size);

#endif
