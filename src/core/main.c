#include <memory/paging.h>
#include <def/config.h>
#include <def/compile.h>
#include <drivers/terminal.h>

#include <stddef.h>
#include <stdint.h>

#define _MASK 0xFFFFF000

#define VIDEO_INFO_LOCATION 0x8000

#define _hlt while(1){	__asm__ volatile ("hlt"); }

extern void kmain();

uintptr_t allocAddr __section(".bss.boot");
size_t allocOff __section(".bss.boot");

#define _MALLOC_INIT(addr) \
	allocAddr = addr; \
	allocOff = 0

__section(".text.boot")
static void* _memset(void* ptr, int c, unsigned long size){
	char* temp = (char*) ptr;

	for(unsigned long i = 0; i < size; i++){
		temp[i] = (char) c;
	}

	return ptr;
}

__section(".text.boot")
void* bumb_alloc(void* start, size_t* off, size_t size){
	uintptr_t ret = (uintptr_t)start + *off;

	size = (size + HEAP_BLOCK_SIZE - 1) & ~(HEAP_BLOCK_SIZE - 1);

	*off += size; 
	return (void*)ret;
}

__section(".text.boot")
static inline void* malloc(size_t size){
	void* addr = bumb_alloc((void*)allocAddr, &allocOff, size);
	_memset(addr, 0x0, size);
	return addr;
}

__section(".text.boot")
static inline void _ldir(uint32_t* val) {
	__asm__ __volatile__ (
			"mov %0, %%cr3"
      :
      : "r"(val)
      : "memory"
  );
}

__section(".text.boot")
static inline void _epg(void) {
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

__section(".text.boot")
static void _get_indexes(void* virtualAddr, uint32_t* outDirIndex, uint32_t* outTabIndex){
	uintptr_t virt = (uintptr_t)virtualAddr;
	*outDirIndex = virt >> 22;
	*outTabIndex = (virt >> 12) & 0x3FF;
}

__section(".text.boot")
static void _map(struct PagingDirectory* directory, void* virtualAddr, void* physicalAddr, uint8_t flags){
	uint32_t dirIndex, tblIndex;
	_get_indexes(virtualAddr, &dirIndex, &tblIndex);

	PagingTable* table = 0x0;
	if(!(directory->entry[dirIndex] & FPAGING_P)){
		table = (PagingTable*)malloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);

		directory->entry[dirIndex] = (PagingTable)table | (FPAGING_P | FPAGING_RW);
		directory->tableCount++;
	}else{
		table = (PagingTable*)(directory->entry[dirIndex] & _MASK);
	}

	table[tblIndex] = (uint32_t) physicalAddr | flags;
}

__section(".text.boot")
static void _map_range(struct PagingDirectory* directory, int count, void* virtualAddr, void* physicalAddr, uint8_t flags){
	for(int i = 0; i < count; i++){
		_map(directory, virtualAddr, physicalAddr, flags);

		virtualAddr = (void*)((uintptr_t)virtualAddr + PAGING_PAGE_SIZE);
		physicalAddr = (void*)((uintptr_t)physicalAddr + PAGING_PAGE_SIZE);
	}
}

__section(".text.boot") 
static struct PagingDirectory* _pag_dir_new_empty(){
	struct PagingDirectory* directory = malloc(sizeof(struct PagingDirectory));
	uint32_t* entry = (uint32_t*)malloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

	directory->entry = entry;
	return directory;
}

__section(".text.boot") 
static void _map_kernel(struct PagingDirectory* directory){
	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_phys_end;
	extern uintptr_t __kernel_high_start;

	size_t kernel_size = (size_t)&__kernel_phys_end - (size_t)&__kernel_phys_start;
	size_t kernel_pages = (kernel_size + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	size_t ident_pages = (MiB(2) + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	uint8_t flags = (FPAGING_P | FPAGING_RW);

	// Identity
	_map_range(
		directory,
		ident_pages,
		(void*)0x0,
		(void*)0x0,
		flags
	);

	// High
	_map_range(
		directory, 
		PAGING_PAGE_SIZE + (kernel_pages - kernel_pages),
		(void*)&__kernel_high_start,
		(void*)&__kernel_phys_start,
		flags
	);
}

__section(".text.boot")
static void _map_heap(struct PagingDirectory* directory){
	uint8_t flags = (FPAGING_P | FPAGING_RW);

	size_t heap_pages = (HEAP_SIZE_BYTES + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	_map_range(
		directory, 
		heap_pages,
		(void*)HEAP_VIRT_BASE, 
		(void*)HEAP_PHYS_BASE,  
		flags
	);

	size_t table_size = (HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE);
	size_t table_pages = (table_size + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	_map_range(
		directory, 
		table_pages,
		(void*)HEAP_TABLE_VIRT_BASE, 
		(void*)HEAP_TABLE_PHYS_BASE,  
		flags
	);
}

__section(".text.boot")
static void _map_framebuffer(struct PagingDirectory* directory){
	struct VideoStructPtr* video = (struct VideoStructPtr*)VIDEO_INFO_LOCATION;
	size_t framebuffer_size = video->height * video->width * (video->bpp / 8);

	size_t fb_pages = (framebuffer_size + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	_map_range(
		directory, 
		fb_pages,
		(void*)KERNEL_FB_VIRT_BASE,
		(void*)video->framebuffer_physical,
		(FPAGING_P | FPAGING_RW)
	);

	video->framebuffer_virtual = KERNEL_FB_VIRT_BASE;
}

__section(".text.boot")
static void _map_stack(struct PagingDirectory* directory){

	size_t stack_pages = (KERNEL_STACK_SIZE + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	_map_range(
		directory, 
		stack_pages, 
		(void*)KERNEL_STACK_VIRT_BOTTOM,
		(void*)KERNEL_STACK_PHYS_BOTTOM,
		(FPAGING_P | FPAGING_RW)
	);
}

__section(".text.entry")
void main(){
	_MALLOC_INIT(_TEMP_PAGE_DIRECTORY_ADDRESS);

	struct PagingDirectory* dir = _pag_dir_new_empty();

	_map_kernel(dir);
	_map_stack(dir);
	_map_heap(dir);
	_map_framebuffer(dir);

	dir->entry[1023] = (uintptr_t)dir->entry | FPAGING_P | FPAGING_RW; // self-referencing PDE
	
	_ldir(dir->entry);

	_epg();

	__asm__ volatile (
		"mov %0, %%esp\n\t"
		"mov %%esp, %%ebp\n\t"
		"jmp %1\n\t"
		:
		: "r"(KERNEL_STACK_VIRT_TOP), "r"(&kmain)
	);
}
