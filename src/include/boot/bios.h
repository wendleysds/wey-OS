#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

#define SEG(ptr) (uint16_t)(((uint32_t)(ptr) >> 4) & 0xFFFF)
#define OFF(ptr) (uint16_t)((uint32_t)(ptr) & 0xF)

struct biosreg{
	uint16_t ax, bx, cx, dx; 
	uint16_t ah, di, es;
} __attribute__((packed));

void bios_intcall(int int_no, const struct biosreg *inReg, struct biosreg* outReg);
void bios_printf(const char* fmt, ...);

#endif
