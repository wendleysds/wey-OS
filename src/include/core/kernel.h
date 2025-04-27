#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdarg.h>

void init_kernel();
void panic(const char* fmt, ...);
void kernel_registers();

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

#endif
