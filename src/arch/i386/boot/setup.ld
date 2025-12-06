OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(main)

SECTIONS{
	. = 0;
	.pad : {
		. = 0x38;
	} = 0xFFFFFFFF
	
	.header : {
		*(.header*)
	}

	.text : ALIGN(256){
		*(.text*)
	}

	.rodata : ALIGN(256){
		*(.rodata*)
	}

	.data : ALIGN(256){
		*(.data*)
	}

	.setup : {
		setup_size = ALIGN(ABSOLUTE(.), 4096);
		setup_sectors = ABSOLUTE(setup_size / 512);
	}

	.bss : ALIGN(256){
		*(COMMON)
		*(.bss*)
	}
}

