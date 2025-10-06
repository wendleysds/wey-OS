ENTRY(main)
OUTPUT_FORMAT(binary)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
	. = KERNEL_PHYS_BASE;

	.entry ALIGN(4096) : AT(ADDR(.entry)) {
		*(.text.entry*) *(.rodata.entry*) *(.data.entry*) *(.bss.entry*) *(COMMON)
	}

	.boot ALIGN(4096) : AT(ADDR(.boot)) {
		__boot_start = .;
		*(.text.boot*) *(.rodata.boot*) *(.data.boot*) *(.bss.boot*) *(COMMON)
		__boot_end = .;
	}

	__file_off_after_boot = . - KERNEL_PHYS_BASE;

	. = KERNEL_VIRT_BASE + __file_off_after_boot;

	.text ALIGN(4096) : AT(ADDR(.text) - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE) {
		__kernel_text_start = .;
		*(.text*)
		__kernel_text_end = .;
	}

	.rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE) {
		__kernel_rodata_start = .;
		*(.rodata*)
		__kernel_rodata_end = .;
	}

	.data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE) {
		__kernel_data_start = .;
		*(.data*)
		__kernel_data_end = .;
	}

	.bss ALIGN(4096) (NOLOAD) : AT(ADDR(.bss) - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE) {
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
}

