#ifndef _UTILS_H
#define _UTILS_H

#include <def/compile.h>
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#define TO_SECTORS(bytes) (((bytes) + 511) >> 9)

#ifndef NULL
#define NULL ((void*)0x0)
#endif

#define __hlt while(1) asm volatile("hlt")

// cpu flags
#define X86_EFLAGS_CF (1UL << 0)
#define X86_EFLAGS_ZF (1UL << 6)

void printf(const char* restrict fmt, ...);
int atoi(const char *str);

#endif