#ifndef _UTILS_H
#define _UTILS_H

#include <def/compile.h>
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#define __hlt while(1) asm volatile("hlt")

void printf(const char* restrict fmt, ...);
void __no_return die();

#endif