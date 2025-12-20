
#include <asm-generic/linker.lds.h>
#define LOAD_OFFSET KERNEL_VIRT_BASE + KERNEL_PHYS_BASE
#define PAGE_SIZE 0x1000

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(__start_32)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
	. = KERNEL_PHYS_BASE;

	.boot ALIGN(PAGE_SIZE) : AT(ADDR(.boot)) {
		__boot_start = .;
		*(.entry.boot*) *(.text.boot*) *(.rodata.boot*) *(.data.boot*) *(.bss.boot*) *(COMMON)
		__boot_end = .;
	}

	__file_off_after_boot = . - KERNEL_PHYS_BASE;

	. = KERNEL_VIRT_BASE + __file_off_after_boot;

	.text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - LOAD_OFFSET) {
		__kernel_text_start = .;
		*(.text*)
		__kernel_text_end = .;
	}

	.rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - LOAD_OFFSET) {
		__kernel_rodata_start = .;
		*(.rodata*)
		__kernel_rodata_end = .;
	}

	.data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - LOAD_OFFSET) {
		__kernel_data_start = .;
		*(.data*)
		__kernel_data_end = .;
	}

	. = ALIGN(PAGE_SIZE);
	.init.begin : AT(ADDR(.init.begin) - LOAD_OFFSET) {
		__init_begin = .;
	}

	INIT_TEXT_SECTION(PAGE_SIZE)

	INIT_DATA_SECTION(PAGE_SIZE)

	.exit.text : AT(ADDR(.exit.text) - LOAD_OFFSET) {
		EXIT_TEXT
	}

	.exit.data : AT(ADDR(.exit.data) - LOAD_OFFSET) {
		EXIT_DATA
	}

	. = ALIGN(PAGE_SIZE);
	.init.end : AT(ADDR(.init.end) - LOAD_OFFSET) {
		__init_end = .;
	}

	.bss ALIGN(PAGE_SIZE) (NOLOAD) : AT(ADDR(.bss) - LOAD_OFFSET) {
		__kernel_bss_start = .;
		*(COMMON) *(.bss*)
		__kernel_bss_end = .;
	}

	__kernel_high_start = ADDR(.text);
	__kernel_high_end   = .;

	__kernel_phys_start = LOADADDR(.text);
	__kernel_phys_end   = LOADADDR(.bss) + SIZEOF(.bss);
	__kernel_phys_size  = (__kernel_phys_end - __kernel_phys_start + 0xFFF) & ~0xFFF;

	__kernel_high_base  = KERNEL_VIRT_BASE;

	DISCARDS
}


