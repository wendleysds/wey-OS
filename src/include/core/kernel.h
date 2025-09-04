#ifndef _KERNEL_H
#define _KERNEL_H

#include <core/rings.h>
#include <syscall.h>
#include <stdarg.h>

void panic(const char* fmt, ...);
void warning(const char* fmt, ...);

#endif
