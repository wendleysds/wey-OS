#ifndef _PANIC_H
#define _PANIC_H

#include <def/compile.h>

asmlinkage __no_return void panic(const char* fmt, ...);

#endif
