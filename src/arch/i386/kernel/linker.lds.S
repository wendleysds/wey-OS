#include <asm-generic/linker.lds.h>

#define PAGE_SIZE 4096

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(phys_startup_32)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;
LOAD_OFFSET = KERNEL_VIRT_BASE;

SECTIONS
{
	. = KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;

	phys_startup_32 = ABSOLUTE(__start_32 - LOAD_OFFSET);

	.text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - LOAD_OFFSET) {
		__kernel_text_start = .;
		*(.entry.boot*)
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
		*(.bss*)
		*(COMMON)
		__kernel_bss_end = .;
	}

	__kernel_high_start = ADDR(.text);
	__kernel_high_end   = .;

	__kernel_size  = (__kernel_high_start - __kernel_high_end + 0xFFF) & ~0xFFF;

	__kernel_high_base  = KERNEL_VIRT_BASE;
	__kernel_phys_base  = KERNEL_PHYS_BASE;

	_end_of_kernel_reserve = .;

	.brk : AT(ADDR(.brk) - LOAD_OFFSET) {
		__brk_base = .;
		. += 64K;
		*(.bss..brk)
		__brk_limit = .;
	}

	_end = .;

	DISCARDS
}