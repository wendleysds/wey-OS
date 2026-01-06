#include <asm/paging.h>
#include <def/compile.h>
#include <def/config.h>
#include <def/init.h>
#include <uapi/headers.h>
#include <stdint.h>
#include <stddef.h>

#define __hlt do {__asm__ volatile("hlt");}while(1)
#define __boot __section(".text.boot")

#define PAGE_COUNT(size) (((size) + PTE_PAGE_SIZE - 1) / PTE_PAGE_SIZE)

static size_t allocAddr __section(".bss.boot");
static size_t allocOff __section(".bss.boot");

extern __no_return void kmain();

static void* __boot memset(void* ptr, int c, unsigned long size){
	char* temp = (char*) ptr;

	for(unsigned long i = 0; i < size; i++){
		temp[i] = (char) c;
	}

	return ptr;
}

static void* __boot memcpy(void* dest, const void* src, unsigned long length){
	char *d = (char*)dest;
	const char *s = (const char*)src;

	if (d < s) while(length--)
		*d++ = *s++;
	else while(length--)
		*(d + length) = *(s + length);

	return d;
}

static void* __boot malloc(size_t size){
	uintptr_t ret = allocAddr + allocOff;
	size = (size + PTE_PAGE_SIZE - 1) & ~(PTE_PAGE_SIZE - 1);
	allocOff += size;

	memset((void*)ret, 0x0, size);
	return (void*)ret;
}

static void __boot mmap(pgd_t* pgd, void* virtualAddr, void* physicalAddr, uint16_t page_count, uint8_t flags){
	for(uint16_t i = 0; i < page_count; i++){
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

		pte[tblIndex] = ((uint32_t)physicalAddr & PAGE_MASK) | flags;

		virtualAddr = (void*)((uintptr_t)virtualAddr + PTE_PAGE_SIZE);
		physicalAddr = (void*)((uintptr_t)physicalAddr + PTE_PAGE_SIZE);
	}
}

pgd_t* __boot mk_early_pgtbl_32(){
	allocAddr = _TEMP_PAGE_DIRECTORY_ADDRESS; allocOff = 0;
	pgd_t* pgd = (pgd_t*)malloc(sizeof(pgd_t) * PTE_MAX_ENTRIES);
	int pgd_flags = (_PAGE_P | _PAGE_RW);

	// Map indentity 0x0 - __boot_end
	extern uintptr_t __boot_end;
	mmap(
		pgd, 
		(void*)0,
		(void*)0,
		PAGE_COUNT((uint32_t)&__boot_end), 
		pgd_flags
	);

	// Map high
	extern uintptr_t __kernel_phys_start;
	extern uintptr_t __kernel_high_start;
	extern uintptr_t __kernel_phys_size;
	mmap(
		pgd, 
		(void*)&__kernel_high_start,
		(void*)&__kernel_phys_start,
		PAGE_COUNT((size_t)&__kernel_phys_size),
		pgd_flags
	);

	// Map stack
	mmap(
		pgd,
		(void*)KERNEL_STACK_VIRT_BOTTOM,
		(void*)KERNEL_STACK_PHYS_BOTTOM,
		PAGE_COUNT(KERNEL_STACK_SIZE),
		pgd_flags
	);

	pgd[1023] = ((pgd_t)pgd & PAGE_MASK) | (_PAGE_P | _PAGE_RW);
	return pgd;
}

struct boot_header boot_header;
void __regparm(1) __init __no_return i386_start_kernel(struct boot_header* boot_header_ptr){
	memcpy(&boot_header, boot_header_ptr, sizeof(struct boot_header));

	// TODO: add early idt handler

	kmain();
}
