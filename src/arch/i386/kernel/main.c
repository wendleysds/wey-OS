#include <wey/mm_types.h>
#include <asm/paging.h>
#include <def/config.h>
#include <def/compile.h>
#include <drivers/terminal.h>

#include <stddef.h>
#include <stdint.h>

#define PAGE_COUNT(size) (((size) + PTE_PAGE_SIZE - 1) / PTE_PAGE_SIZE);

#define VIDEO_INFO_LOCATION 0x8000

#define _hlt while(1){	__asm__ volatile ("hlt"); }
#define __boot __section(".text.boot")

extern void kmain();

uintptr_t allocAddr __section(".bss.boot");
size_t allocOff __section(".bss.boot");

#define _MALLOC_INIT(addr) \
	allocAddr = addr; \
	allocOff = 0

static void* __boot _memset(void* ptr, int c, unsigned long size){
	char* temp = (char*) ptr;

	for(unsigned long i = 0; i < size; i++){
		temp[i] = (char) c;
	}

	return ptr;
}

void* __boot bumb_alloc(void* start, size_t* off, size_t size){
	uintptr_t ret = (uintptr_t)start + *off;

	size = (size + HEAP_BLOCK_SIZE - 1) & ~(HEAP_BLOCK_SIZE - 1);

	*off += size; 
	return (void*)ret;
}

static inline void* __boot malloc(size_t size){
	void* addr = bumb_alloc((void*)allocAddr, &allocOff, size);
	_memset(addr, 0x0, size);
	return addr;
}

static inline void __boot _ldir(uint32_t* val) {
	__asm__ __volatile__ (
		"mov %0, %%cr3"
		:
		: "r"(val)
		: "memory"
  );
}

static inline void __boot _epg(void) {
	__asm__ __volatile__ (
		"mov %%cr0, %%eax\n\t"
		"or  $0x80000000, %%eax\n\t"
		"mov %%eax, %%cr0\n\t"
		"jmp flush\n\t"
		"flush:"
		:
		:
		: "eax", "memory"
  );
}

static void __boot _map(pgd_t* pgd, void* virtualAddr, void* physicalAddr, uint8_t flags){
	uintptr_t virt = (uintptr_t)virtualAddr;
	uint32_t dirIndex = virt >> 22;
	uint32_t tblIndex = (virt >> 12) & 0x3FF;

	pte_t* pte = NULL;
	if(!(pgd[dirIndex] & _PAGE_P)){
		pte = (pte_t*)malloc(sizeof(pte_t) * PTE_MAX_ENTRIES);

		pgd[dirIndex] = (pgd_t)pte | (_PAGE_P | _PAGE_RW);
	}else{
		pte = (pte_t*)(pgd[dirIndex] & PAGE_MASK);
	}

	pte[tblIndex] = (uint32_t) physicalAddr | flags;
}

static void __boot _map_range(pgd_t* pgd, int count, void* virtualAddr, void* physicalAddr, uint8_t flags){
	for(int i = 0; i < count; i++){
		_map(pgd, virtualAddr, physicalAddr, flags);

		virtualAddr = (void*)((uintptr_t)virtualAddr + PTE_PAGE_SIZE);
		physicalAddr = (void*)((uintptr_t)physicalAddr + PTE_PAGE_SIZE);
	}
}

static pgd_t* __boot _pgd_alloc(){
	return (pgd_t*)malloc(sizeof(pgd_t) * PGD_MAX_PTE);
}

static void __boot _map_kernel(pgd_t* pgd){
	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_high_start;
	extern uintptr_t __kernel_phys_size;
	extern uintptr_t __boot_end;

	size_t kernel_pages = PAGE_COUNT((size_t)&__kernel_phys_size);
	size_t ident_pages = PAGE_COUNT(VIDEO_INFO_LOCATION - (size_t)&__boot_end);

	uint8_t flags = (_PAGE_P | _PAGE_RW);

	// Identity
	_map_range(
		pgd,
		ident_pages,
		(void*)VIDEO_INFO_LOCATION,
		(void*)VIDEO_INFO_LOCATION,
		flags
	);

	// High
	_map_range(
		pgd, 
		kernel_pages,
		(void*)&__kernel_high_start,
		(void*)&__kernel_phys_start,
		flags
	);
}

static void __boot _map_heap(pgd_t* pgd){
	uint8_t flags = (_PAGE_P | _PAGE_RW);

	size_t heap_pages = PAGE_COUNT(HEAP_SIZE_BYTES + PTE_PAGE_SIZE);

	_map_range(
		pgd, 
		heap_pages,
		(void*)HEAP_VIRT_BASE, 
		(void*)HEAP_PHYS_BASE,  
		flags
	);

	size_t table_size = (HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE);
	size_t table_pages = PAGE_COUNT(table_size);

	_map_range(
		pgd, 
		table_pages,
		(void*)HEAP_TABLE_VIRT_BASE, 
		(void*)HEAP_TABLE_PHYS_BASE,  
		flags
	);
}

static void __boot _map_framebuffer(pgd_t* pgd){
	struct VideoStructPtr* video = (struct VideoStructPtr*)VIDEO_INFO_LOCATION;
	size_t framebuffer_size = video->height * video->width * (video->bpp / 8);

	size_t fb_pages = PAGE_COUNT(framebuffer_size);

	_map_range(
		pgd, 
		fb_pages,
		(void*)KERNEL_FB_VIRT_BASE,
		(void*)video->framebuffer_physical,
		(_PAGE_P | _PAGE_RW)
	);

	video->framebuffer_virtual = KERNEL_FB_VIRT_BASE;
}

static void __boot _map_stack(pgd_t* pgd){

	size_t stack_pages = PAGE_COUNT(KERNEL_STACK_SIZE);

	_map_range(
		pgd, 
		stack_pages, 
		(void*)KERNEL_STACK_VIRT_BOTTOM,
		(void*)KERNEL_STACK_PHYS_BOTTOM,
		(_PAGE_P | _PAGE_RW)
	);
}

__section(".entry.boot")
void main(){
	__asm__ volatile (
		"mov %0, %%esp\n\t"
		"mov %%esp, %%ebp\n\t"
		:
		: "r"(0x90000)
	);

	volatile char* mem = (volatile char*)0xB8000;
	mem[0] = 'O';
	mem[1] = 0x0f;
	mem[2] = 'K';
	mem[3] = 0x0f; 

	_hlt

	_MALLOC_INIT(_TEMP_PAGE_DIRECTORY_ADDRESS);

	pgd_t* pgd = _pgd_alloc();

	_map_kernel(pgd);
	_map_stack(pgd);
	_map_heap(pgd);
	_map_framebuffer(pgd);

	pgd[1023] = (pgd_t)pgd | _PAGE_P | _PAGE_RW;
	
	_ldir(pgd);

	_epg();

	__asm__ volatile (
		"mov %0, %%esp\n\t"
		"mov %%esp, %%ebp\n\t"
		"jmp *%1\n\t"
		:
		: "r"(KERNEL_STACK_VIRT_TOP), "r"(&kmain)
	);
}
