ENTRY(_entry16)
OUTPUT_FORMAT(binary)

SECTIONS{
	. = 0x1000;

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
}

