OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(kmain)

SECTIONS{
	. = 0x100000;

	.multiboot : {
		*(.multiboot)
	}

	.text : {
		*(.text)
		*(.rodata)
	}

	.data : {
		start_ctors = .;
		KEEP(*( .init_array ));
		KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
		end_ctors = .;
		*(.data)
	}

	.bss : {
		*(COMMON)
		*(.bss)
		kernel_stack = .;
	}

	/DISCARD/ : {
		*(.fini_array*)
		*(.comment)
	}
}

