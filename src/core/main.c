#include <memory/paging.h>
#include <def/config.h>
#include <def/compile.h>

#include <stddef.h>
#include <stdint.h>

#define __section(x) __attribute__((section(x)))
#define _MASK 0xFFFFF000

uintptr_t allocAddr __section(".bss.boot");
size_t allocOff __section(".bss.boot");

#define _MALLOC_INIT(addr) \
	allocAddr = addr; \
	allocOff = 0

__section(".text.boot")
void* bumb_alloc(void* start, size_t* off, size_t size){
	uintptr_t ret = (uintptr_t)start + *off;

	size = (size + HEAP_BLOCK_SIZE - 1) & ~(HEAP_BLOCK_SIZE - 1);

	*off += size; 
	return (void*)ret;
}

__section(".text.boot")
static inline void* malloc(size_t size){
	return bumb_alloc((void*)allocAddr, &allocOff, size);
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

	PagingTable* table = (PagingTable*)(directory->entry[dirIndex] & _MASK);
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
      "mov %%eax, %%cr0"
      :
      :
      : "eax", "memory"
  );
}

__section(".text.boot") 
static struct PagingDirectory* _pag_dir_new(uint32_t tablesAmount){
	struct PagingDirectory* directory = malloc(sizeof(struct PagingDirectory));
	uint32_t* entry = (uint32_t*)malloc(sizeof(uint32_t) * tablesAmount);

	uint32_t offset = 0;
	uint8_t flags = (FPAGING_P | FPAGING_RW);

	for (uint32_t i = 0; i < tablesAmount; i++){
		PagingTable* table = (PagingTable*)malloc(sizeof(PagingTable) * PAGING_TOTAL_ENTRIES_PER_TABLE);

		for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++){
			table[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
		}

		offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
		entry[i] = (PagingTable)table | flags;
	}

	// identity map entry
	directory->entry = entry;
	directory->tableCount = tablesAmount;

	return directory;
}

__section(".text.boot") 
static void map_kernel_high(){

	struct PagingDirectory* dir = _pag_dir_new(PAGING_TOTAL_ENTRIES_PER_TABLE);
	
	// Map high
	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_phys_end;
	extern uintptr_t __kernel_high_start;
	
	size_t kernel_size = (size_t)&__kernel_phys_end - (size_t)&__kernel_phys_start;
	size_t kernel_pages = (kernel_size + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;

	_map_range(
			dir, 
			kernel_pages,
			(void*)&__kernel_high_start,
      (void*)&__kernel_phys_start,
			(FPAGING_P | FPAGING_RW)
	);
	
	_ldir(dir->entry);
}

__section(".text.boot") 
void main(){
	_MALLOC_INIT(HEAP_ADDRESS);

	map_kernel_high();
	_epg();

	extern __no_return void kmain(void);

	void (*jmp_kmain)(void) = (void(*)(void))((uintptr_t)&kmain);
	jmp_kmain();

	while(1){
		__asm__ volatile ("hlt");
	}
}

