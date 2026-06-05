#include <mm/memblock.h>
#include <kernel/printk.h>
#include <kernel/init.h>

#define ALIGN_UP(v,a)  (((v) + (a) - 1) & ~((a)-1))

unsigned long max_low_pfn;
unsigned long max_pfn;

struct memblock memblock __initdata;

static __init void memblock_sort(struct memblock_type* type){
	int i, j;
	for (i = 1; i < type->count; i++) {
		struct memblock_region key = type->regions[i];
		j = i;

		while (j > 0 && type->regions[j - 1].base > key.base) {
			type->regions[j] = type->regions[j - 1];
			j--;
		}

		type->regions[j] = key;
	}
}

static __init void memblock_merge(struct memblock_type* type) {
	if (type->count < 2)
		return;

	int out = 0;

	for (int i = 1; i < type->count; i++) {
		uint64_t prev_base = type->regions[out].base;
		uint64_t cur_base  = type->regions[i].base;
		
		uint64_t prev_end = prev_base + type->regions[out].size;
		if (type->regions[out].size > UINTPTR_MAX - prev_base)
			prev_end = UINTPTR_MAX;

		uint64_t cur_end = cur_base + type->regions[i].size;
		if (type->regions[i].size > UINTPTR_MAX - cur_base)
			cur_end = UINTPTR_MAX;

		if (cur_base <= prev_end) {
			// merge
			if (cur_end > prev_end) {
				type->regions[out].size = cur_end - prev_base;
			}
		} else {
			// move forward
			out++;
			type->regions[out] = type->regions[i];
		}
	}

	type->count = out + 1;

	type->totalmem = 0;
	for (int i = 0; i < type->count; i++) {
		type->totalmem += type->regions[i].size;
	}
}

static __init void memblock_dump(struct memblock_type* type){
	printk("Memblock: %s.count = 0x%lx\n", type->name, type->count);

	for(size_t i = 0; i < type->count; i++){
		printk(" [%#018llx - %#018llx] %lu bytes\n",
			type->regions[i].base, 
			type->regions[i].base + type->regions[i].size, 
			type->regions[i].size
		);
	}
}

void __init memblock_dump_all(void){
	printk("Memblock: total memory: %#llx; total reserved: %#llx\n",
		memblock.memory.totalmem,
		memblock.reserved.totalmem
	);

	memblock_dump(&memblock.memory);
	memblock_dump(&memblock.reserved);
}

void __init memblock_init(){
	memblock.reserved.count = 0;
	memblock.reserved.name = "Reserved";

	memblock.memory.count = 0;
	memblock.memory.name = "Memory";
}

void __init memblock_reserve(uint64_t base, size_t size) {
	if (size == 0){
		return;
	}

	struct memblock_type* res = &memblock.reserved;

	if (res->count >= MAX_REGIONS) {
		return;
	}

	res->regions[res->count++] = (struct memblock_region){
		.base = base,
		.size = size
	};

	memblock_sort(res);
	memblock_merge(res);
}

int __init memblock_is_reserved(uint64_t base, size_t size) {
	if (size > UINTPTR_MAX - base){
		return 1;
	}

	uint64_t end = base + size;
	for (int i = 0; i < memblock.reserved.count; i++) {
		uint64_t r_base = memblock.reserved.regions[i].base;
		uint64_t r_end  = r_base + memblock.reserved.regions[i].size;

		if (!(end <= r_base || base >= r_end)) {
			return 1; // overlap
		}
	}
	return 0;
}

void* __init memblock_alloc(size_t size, size_t align) {
	if (size == 0){
		return NULL;
	}

	struct memblock_type* mem = &memblock.memory;

	if (align == 0) align = 1;

	for (int i = 0; i < mem->count; i++) {
		uint64_t start = mem->regions[i].base;

		if (mem->regions[i].size > UINTPTR_MAX - start)
			continue;

		uint64_t end = start + mem->regions[i].size;

		uint64_t cur = ALIGN_UP(start, align);

		if (size > UINTPTR_MAX - cur)
			break;

		while (cur + size <= end) {
			if (!memblock_is_reserved(cur, size)) {
				memblock_reserve(cur, size);
				return (void*)cur;
			}

			cur += align;
		}
	}

	return NULL;
}

void* __init memblock_alloc_range(uint64_t base, uint64_t end, size_t size) {
	if (size == 0 || base >= end){
		return NULL;
	}

	struct memblock_type* mem = &memblock.memory;

	for (int i = 0; i < mem->count; i++) {
		uint64_t region_base = mem->regions[i].base;
		uint64_t region_end = region_base + mem->regions[i].size;
		if (mem->regions[i].size > UINTPTR_MAX - region_base)
			region_end = UINTPTR_MAX;

		uint64_t alloc_start = (region_base > base) ? region_base : base;
		uint64_t alloc_end = (region_end < end) ? region_end : end;

		if (alloc_start >= alloc_end || size > alloc_end - alloc_start)
			continue;

		uint64_t cur = alloc_start;
		while (cur + size <= alloc_end) {
			if (!memblock_is_reserved(cur, size)) {
				memblock_reserve(cur, size);
				return (void*)cur;
			}
			cur++;
		}
	}

	return NULL;
}

void __init memblock_add(uint64_t base, size_t size) {
	if (size == 0){
		return;
	}

	struct memblock_type* mem = &memblock.memory;

	if (mem->count >= MAX_REGIONS) {
		return;
	}

	mem->regions[mem->count++] = (struct memblock_region){
		.base = base,
		.size = size
	};

	memblock_sort(mem);
	memblock_merge(mem);
}
