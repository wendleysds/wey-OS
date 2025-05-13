ENTRY(_entry32)
OUTPUT_FORMAT(binary)

SECTIONS{
	. = 1M;

	.text : ALIGN(4096){
		*(.text)
	}

	.asm : ALIGN(4096){
		*(.asm)
	}

	.rodata : ALIGN(4096){
		*(.rodata)
	}

	.data : ALIGN(4096){
		*(.data)
	}

	.bss : ALIGN(4096){
		*(COMMON)
		*(.bss)
	}
}

