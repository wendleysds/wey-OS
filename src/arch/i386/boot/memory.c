#include <def/err.h>
#include "boot.h"


#define SMAP 0x534d4150 // ASCII "SMAP"

static void e820(){
	int count = 0;
	struct biosregs ireg, oreg;
	struct e820_entry *desc = boot_header.e820_table;
	static struct e820_entry buff;

	initregs(&ireg);
	ireg.ax  = 0xe820;
	ireg.cx  = sizeof(buff);
	ireg.edx = SMAP;
	ireg.di = (size_t)&buff;

	do {
		intcall(0x15, &ireg, &oreg);
		ireg.ebx = oreg.ebx;

		if (oreg.eflags & X86_EFLAGS_CF)
			break;

		if (oreg.eax != SMAP) {
			count = 0;
			break;
		}

		*desc++ = buff;
		count++;
	} while (ireg.ebx && count < ARRAY_SIZE(boot_header.e820_table));

	boot_header.e820_entries_count = count;
}

void detect_memory(){
	e820();
}
