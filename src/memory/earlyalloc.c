#include <def/config.h>
#include <def/init.h>
#include <asm/paging.h>
#include <asm/page.h>
#include <stdint.h>
#include <stddef.h>

// Early allocator
// Synchronized boot and normal phases

static size_t alloc_addr __initdata;
static size_t alloc_offs __initdata;
static size_t alloc_end __initdata;

static size_t* alloc_addr_boot __section(".bss.boot");
static size_t* alloc_offs_boot __section(".bss.boot");
static size_t* alloc_end_boot __section(".bss.boot");

void __section(".text.boot") early_alloc_init_boot(){
	alloc_addr_boot = (size_t*)(__pa(&alloc_addr) + KERNEL_PHYS_BASE);
	alloc_offs_boot = (size_t*)(__pa(&alloc_offs) + KERNEL_PHYS_BASE);
	alloc_end_boot = (size_t*)(__pa(&alloc_end) + KERNEL_PHYS_BASE);

	*alloc_addr_boot = __pa(KERNEL_EARLY_START);
	*alloc_end_boot = __pa(KERNEL_EARLY_END);
	*alloc_offs_boot = 0;
}

void __init early_alloc_init(){
	alloc_addr = KERNEL_EARLY_START;
	alloc_end = KERNEL_EARLY_END;
	alloc_offs = 0;
}

void* __section(".text.boot") early_alloc_boot(size_t size){
	uintptr_t ret = *alloc_addr_boot + *alloc_offs_boot;
	size = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	*alloc_offs_boot += size;
	return (void*)ret;
}

void* __init early_alloc(size_t size){
	uintptr_t ret = alloc_addr + alloc_offs;
	size = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	alloc_offs += size;
	return (void*)ret;
}
