#ifndef _PAGE_H
#define _PAGE_H

#ifndef __ASSEMBLY__
extern unsigned long __kernel_high_base;
#define __va(addr) ((uintptr_t)(addr) + (unsigned long)&__kernel_high_base)
#define __pa(addr) ((uintptr_t)(addr) - (unsigned long)&__kernel_high_base)
#else
extern __kernel_high_base
#define __va(addr) [addr + __kernel_high_base]
#define __pa(addr) [addr - __kernel_high_base]
#endif

#endif