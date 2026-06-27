#include <platform.h>
#include <stdint.h>
#include <stddef.h>
#include <def/errno.h>
#include <string.h>
#include <utils.h>
#include "bios.h"
#include <mem.h>

#define SMAP 0x534d4150 /* ASCII "SMAP" */

uint8_t mainDriverNum = 0;

int platform_getchar(){
	struct biosregs ireg, oreg;
	initregs(&ireg);
	intcall(0x16, &ireg, &oreg);

	return oreg.al;
}

static uint8_t gettime(void){
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ah = 0x02;
	intcall(0x1a, &ireg, &oreg);

	return oreg.dh;
}

static int kbd_pending(void){
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ah = 0x01;
	intcall(0x16, &ireg, &oreg);

	return !(oreg.eflags & X86_EFLAGS_ZF);
}

int platform_timeout(int secs){
	int start, current;

	start = gettime();

	while (secs) {
		if (kbd_pending()){
			return platform_getchar();
		}

		current = gettime();
		if (start != current) {
			secs--;
			start = current;
		}
	}

	return 0;
}

static void keyboard_init(){
	struct biosregs ireg;
	initregs(&ireg);

	ireg.ax = 0x0305;
	intcall(0x16, &ireg, 0x0);
}

int platform_init(){
	asm volatile (
		"movb %%dl, %0"
		: "=rm"(mainDriverNum)
	);

	keyboard_init();

	return SUCCESS;
}

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

		if (oreg.eflags & X86_EFLAGS_CF)
			break;

		if (oreg.eax != SMAP) {
			return -EAGAIN;
		}

		*desc++ = buf;
		*count += 1;
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