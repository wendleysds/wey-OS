ENTRY(_entry16)
OUTPUT_FORMAT(binary)

SECTIONS{
	. = 0x1000;

	.text : ALIGN(0){
		*(.text)
	}

	.asm : ALIGN(0){
		*(.asm)
	}

	.rodata : ALIGN(0){
		*(.rodata)
	}

	.data : ALIGN(0){
		*(.data)
	}

	.bss : ALIGN(0){
		*(COMMON)
		*(.bss)
	}
}

