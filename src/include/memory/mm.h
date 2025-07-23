#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#include <memory/heap.h>
#include <memory/kheap.h>
#include <memory/paging.h>
#include <lib/mem.h>

struct mem_region {
    int size;

    void* virtualBaseAddress;
    void* virtualEndAddress;

    void* physicalBaseAddress;
    void* physicalEndAddress;

    struct mem_region* next;
}__attribute__((packed));

struct mm_struct{
    struct mem_region* regions;
};

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_POPULATE  0x0
#define MAP_FIXED     0x1
#define MAP_PRIVATE   0x2
#define MAP_ANONYMOUS 0x4

void* mmap(void* addr, uint32_t length, uint32_t prot, uint32_t flags, int fd, uint32_t offset);
int munmap(void* addr, size_t length);

#endif
