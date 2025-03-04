ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS{
	. = 0x1000;

	.text : {
		*(.text)
	}

	.rodata : {
		*(.rodata)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(COMMON)
		*(.bss)
	}
	
	/DISCARD/ : {
		*(.fini_array*)
		*(.comment)
	}
}

