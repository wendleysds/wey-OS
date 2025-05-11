#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

struct biosreg{
	uint16_t ax, bx, cx, dx; 
	uint16_t di, es;
} __attribute__((packed));

// bioscall.asm
void bios_int10h(const struct biosreg *inReg);

// biosutils.c
void bios_printf(const char* fmt, ...);
uint16_t SEG(uint32_t ptr);
uint16_t OFF(uint32_t ptr);

#endif
