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

	.text : {
		*(.text*)
	}

	.text32 : {
		*(.text32)
	}

	. = ALIGN(16);
	.rodata : {
		*(.rodata*)
	}

	. = ALIGN(16);
	.data  : {
		*(.data*)
	}

	.setup : {
		setup_size = ALIGN(ABSOLUTE(.), 4096);
		setup_sectors = ABSOLUTE(setup_size / 512);
	}

	. = ALIGN(16);
	.bss : {
		*(COMMON)
		*(.bss*)
	}
}

