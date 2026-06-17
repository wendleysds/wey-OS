#include <def/err.h>
#include "boot.h"

#define SMAP 0x534d4150 // ASCII "SMAP"

static int e820(struct e820_entry* table){
	int count = 0;
	struct biosregs ireg, oreg;
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
			return 1;
		}

		if(table){
			*table++ = buff;
		}

		count++;
	} while (ireg.ebx);

	return count;
}

int detect_memory(struct e820_entry* table){
	return e820(table);
}