OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(main)

SECTIONS{
	. = 0x1000;

	.entry ALIGN(256) : AT(ADDR(.entry)) {
		*(.text.entry*) *(.rodata.entry*) *(.data.entry*) *(.bss.entry*) *(COMMON)
	}

	.text : ALIGN(256){
		*(.text)
	}

	.asm : ALIGN(256){
		*(.asm)
	}

	.rodata : ALIGN(256){
		*(.rodata)
	}

	.data : ALIGN(256){
		*(.data)
	}

	.bss : ALIGN(256){
		*(COMMON)
		*(.bss)
	}

	bin_end = .;
	. = ALIGN(4096);
	end_align = .;
}

