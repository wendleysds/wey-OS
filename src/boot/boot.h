#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

#define SEG(ptr) (uint16_t)(((uint32_t)(ptr) >> 4) & 0xFFFF)
#define OFF(ptr) (uint16_t)((uint32_t)(ptr) & 0xF)
#define memset(d,c,l) __builtin_memset(d,c,l)
#define memcpy(d,s,l) __builtin_memcpy(d,s,l)

struct biosregs {
	union {
		struct {
			uint32_t edi;
			uint32_t esi;
			uint32_t ebp;
			uint32_t _esp;
			uint32_t ebx;
			uint32_t edx;
			uint32_t ecx;
			uint32_t eax;
			uint32_t _fsgs;
			uint32_t _dses;
			uint32_t eflags;
		};
		struct {
			uint16_t di, hdi;
			uint16_t si, hsi;
			uint16_t bp, hbp;
			uint16_t _sp, _hsp;
			uint16_t bx, hbx;
			uint16_t dx, hdx;
			uint16_t cx, hcx;
			uint16_t ax, hax;
			uint16_t gs, fs;
			uint16_t es, ds;
			uint16_t flags, hflags;
		};
		struct {
			uint8_t dil, dih, edi2, edi3;
			uint8_t sil, sih, esi2, esi3;
			uint8_t bpl, bph, ebp2, ebp3;
			uint8_t _spl, _sph, _esp2, _esp3;
			uint8_t bl, bh, ebx2, ebx3;
			uint8_t dl, dh, edx2, edx3;
			uint8_t cl, ch, ecx2, ecx3;
			uint8_t al, ah, eax2, eax3;
		};
	};
};

static inline uint16_t ds(void){
	uint16_t seg;
	asm("movw %%ds,%0" : "=rm" (seg));
	return seg;
}

static inline uint16_t fs(void){
	uint16_t seg;
	asm volatile("movw %%fs,%0" : "=rm" (seg));
	return seg;
}

static inline uint16_t gs(void){
	uint16_t seg;
	asm volatile("movw %%gs,%0" : "=rm" (seg));
	return seg;
}

void initregs(struct biosregs *reg);
void intcall(uint8_t int_no, const struct biosregs *ireg, struct biosregs *oreg);

void setup_video();

void bios_printf(const char* fmt, ...);

#endif
