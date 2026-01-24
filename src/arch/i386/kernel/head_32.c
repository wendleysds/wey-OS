#include <asm/paging.h>
#include <asm/page.h>
#include <def/compile.h>
#include <def/config.h>
#include <def/init.h>
#include <mm/earlyalloc.h>
#include <uapi/headers.h>
#include <lib/font.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/printk.h>
#include <stdint.h>
#include <stddef.h>

#define __hlt do {__asm__ volatile("hlt");}while(1)
#define __boot __section(".text.boot")

#define PAGE_COUNT(size) (((size) + PTE_PAGE_SIZE - 1) / PTE_PAGE_SIZE)

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

static void __boot mmap(pgd_t* pgd, void* virtualAddr, void* physicalAddr, uint16_t page_count, uint8_t flags){
	for(uint16_t i = 0; i < page_count; i++){
		uintptr_t virt = (uintptr_t)virtualAddr;
		uint32_t dirIndex = virt >> 22;
		uint32_t tblIndex = (virt >> 12) & 0x3FF;

		pte_t* pte = NULL;
		if(!(pgd[dirIndex] & _PAGE_P)){
			pte = (pte_t*)early_alloc_boot(sizeof(pte_t) * PTE_MAX_ENTRIES);
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
	pgd_t* pgd = (pgd_t*)early_alloc_boot(sizeof(pgd_t) * PTE_MAX_ENTRIES);
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
	extern uintptr_t __kernel_size;
	mmap(
		pgd, 
		(void*)&__kernel_high_start,
		(void*)&__kernel_phys_start,
		PAGE_COUNT((size_t)&__kernel_size),
		pgd_flags
	);

	// Map early alloc area
	mmap(
		pgd,
		(void*)KERNEL_EARLY_START,
		(void*)__pa(KERNEL_EARLY_START),
		PAGE_COUNT(RESERVED_SIZE),
		pgd_flags
	);

	pgd[1023] = ((pgd_t)pgd & PAGE_MASK) | (_PAGE_P | _PAGE_RW);
	return pgd;
}

__init static void e820_print_type(enum e820_type type)
{
	switch (type) {
		case E820_TYPE_RAM:	
			printk(" System RAM");
			break;
		case E820_TYPE_RESERVED:
			printk(" device reserved");
			break;
		case E820_TYPE_SOFT_RESERVED:
			printk(" soft reserved");
			break;
		case E820_TYPE_ACPI:
			printk(" ACPI data");
			break;
		case E820_TYPE_NVS:
			printk(" ACPI NVS");
			break;
		case E820_TYPE_UNUSABLE:
			printk(" unusable");
			break;
		case E820_TYPE_PMEM:
		case E820_TYPE_PRAM:
			printk(" persistent RAM (type %u)", type);
			break;
		default:
			printk(" type %u", type);
			break;
	}
}

struct boot_header boot_header;

static __init void e820_sort(struct e820_entry* table, uint32_t count)
{
    uint32_t i, j;

    for (i = 1; i < count; i++) {
        struct e820_entry key = table[i];
        j = i;

        while (j > 0 &&
               table[j - 1].base_addr > key.base_addr) {
            table[j] = table[j - 1];
            j--;
        }

        table[j] = key;
    }
}


void __regparm(1) __init __no_return i386_start_kernel(struct boot_header* boot_header_ptr){
	memcpy(&boot_header, boot_header_ptr, sizeof(struct boot_header));
	terminal_early_init();

	printk("Started i386 kernel\n");

	printk("Sorted E820\n");
	e820_sort(boot_header.e820_table, boot_header.e820_entries_count);

	printk("E820-provided physical RAM map:\n");
	uint64_t range_end_prev = 0;
	uint32_t idx;

	for (idx = 0; idx < boot_header.e820_entries_count; idx++) {
		struct e820_entry* entry = &boot_header.e820_table[idx];
		uint64_t range_start, range_end;

		range_start = entry->base_addr;

		if (entry->length > UINT64_MAX - entry->base_addr) {
			printk("E820 entry overflow!\n");
			continue;
		}

		range_end = entry->base_addr + entry->length;


		/* Out of order E820 maps should not happen: */
		if (range_start < range_end_prev)
			printk("out of order E820 entry!\n");

		if (range_start > range_end_prev) {
			printk("  [gap %#018llx-%#018llx]\n",
				range_end_prev,
				range_start-1);
		}

		printk("  [mem %#018llx-%#018llx] ", range_start, range_end-1);
		e820_print_type(entry->type);
		printk("\n");

		range_end_prev = range_end;
	}

	printk("Kernel mapped physical start at %p to high start at %p.\n", (void*)&__kernel_phys_start, (void*)&__kernel_high_start);

	// TODO: add early idt handler

	kmain();
}
