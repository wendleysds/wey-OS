#ifndef _MM_TYPES_H
#define _MM_TYPES_H

#include <asm/paging.h>
#include <stdint.h>

typedef enum {
	MEM_READ = 1 << 0,
	MEM_WRITE = 1 << 1,
	MEM_EXEC = 1 << 2,
	MEM_USER = 1 << 3,
	MEM_KERNEL = 1 << 4,
} mem_flags_t;

#define MAP_POPULATE  0x0
#define MAP_FIXED     0x1
#define MAP_PRIVATE   0x2
#define MAP_ANONYMOUS 0x4

struct file;

typedef unsigned long long off_t;

struct mem_region {
	void* physBaseAddress;
    void* virtualBaseAddress;
    void* virtualEndAddress;
	
	uint32_t size;

	struct file* backing_file;
	off_t file_offset;

	mem_flags_t mem_flags;
	uint8_t map_flags;

    struct mem_region* next;
}__attribute__((packed));

struct mm_struct{
    struct mem_region* regions;
    void* mmapBase;

    pgd_t* pgd;
};

#endif
