#include <platform.h>
#include <stdint.h>
#include <stddef.h>
#include <def/err.h>
#include <string.h>
#include <utils.h>
#include "bios.h"

void platform_putchar(char c){
	struct biosregs r;
	initregs(&r);
	r.bx = 0x0007;
	r.cx = 0x0001;
	r.ah = 0x0E;
	r.al = c;
	intcall(0x10, &r, 0x0);
}

int platform_init(){
	uint8_t drivenum = 0;
	asm volatile (
		"movb %%dl, %0"
		: "=rm"(drivenum)
	);

	return SUCCESS;
}

#define SMAP 0x534d4150 /* ASCII "SMAP" */
int platform_get_memory_map(struct e820_entry* table, int tablesize, uint32_t *count){
	*count = 0;

	struct biosregs ireg, oreg;
	struct e820_entry *desc = table;
	static struct e820_entry buf;

	initregs(&ireg);
	ireg.ax  = 0xe820;
	ireg.cx  = sizeof(buf);
	ireg.edx = SMAP;
	ireg.di  = (size_t)&buf;

	do {
		intcall(0x15, &ireg, &oreg);
		ireg.ebx = oreg.ebx;

		if (oreg.eflags & (((1UL)) << (0)))
			break;

		if (oreg.eax != SMAP) {
			return FAILED;
		}

		*desc++ = buf;
		*count += 1;

		buf.acpi_attrs = 1;

	} while (ireg.ebx && *count < tablesize);

	return SUCCESS;
}

void initregs(struct biosregs *reg){
	memset(reg, 0, sizeof(*reg));
	reg->eflags |= (((1UL)) << (0));
	reg->ds = ds();
	reg->es = ds();
	reg->fs = fs();
	reg->gs = gs();
}

void __no_return platform_die(){
	asm volatile ("int $0x18");
	__hlt;
}