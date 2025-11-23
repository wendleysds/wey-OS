#ifndef _UTILS_H
#define _UTILS_H

#include <def/compile.h>
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#define X86_EFLAGS_CF (1UL << 0)
#define TO_SECTORS(bytes) (((bytes) + 511) >> 9)

#ifndef NULL
#define NULL ((void*)0x0)
#endif

#define __hlt while(1) asm volatile("hlt")

void printf(const char* restrict fmt, ...);

#endif