#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

struct biosreg{
	uint16_t ax, bx, cx, dx; 
	uint16_t ah, di, es;
} __attribute__((packed));

// bioscall.c
void bios_intcall(int int_no, const struct biosreg *inReg, struct biosreg* outReg);

// biosutils.c
void bios_printf(const char* fmt, ...);
uint16_t SEG(uint32_t ptr);
uint16_t OFF(uint32_t ptr);

#endif
