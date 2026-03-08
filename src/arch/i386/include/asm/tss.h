#ifndef _TSS_H
#define _TSS_H

#include <stdint.h>

struct tss{
	uint32_t link;
	uint32_t esp0; // Kernel stack pointer
	uint32_t ss0;  // Kernel stack segment
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t sr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax, ecx, edx, ebx;
	uint32_t esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldtr;
	uint16_t iopb;
	uint16_t ssp;
} __attribute__((packed));

static inline void tss_load(unsigned long tss_segment){
	__asm__ volatile ("ltr %0" :: "r"(tss_segment) : "memory");
}

#endif
