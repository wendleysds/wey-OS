#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#include <stdint.h>

struct mm_struct;
struct mem_region;

struct mem_region* vma_lookup(struct mm_struct* mm, void* virtualAddr);
struct mem_region* vma_add(struct mm_struct* mm, void* physicalAddr, void* virtualAddr, uint32_t size, mem_flags_t mem_flags, uint8_t map_flags);
int vma_remove(struct mm_struct* mm, struct mem_region* mm_region, void* virtualAddr, uint32_t size);
int vma_clean(struct mm_struct* mm);

#endif
