#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdarg.h>

void init_kernel();
void panic(const char* fmt, ...);
void kernel_registers();

#endif
