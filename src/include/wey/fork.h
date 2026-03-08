#ifndef _FORK_H
#define _FORK_H

#include <wey/pid.h>
#include <def/compile.h>

int fork_init();

asmlinkage __no_return void kernel_thread_trampoline(int (*fn)(void*), void* args);
pid_t kernel_thread(int (*fn)(void*), const char* name, void* args);

#endif
