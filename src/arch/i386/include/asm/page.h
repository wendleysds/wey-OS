#ifndef _PAGE_H
#define _PAGE_H

#include <def/linker.h>

#define __va(addr) ((uintptr_t)(addr) + (uintptr_t)__kernel_high_base)
#define __pa(addr) ((uintptr_t)(addr) - (uintptr_t)__kernel_high_base)

#endif
