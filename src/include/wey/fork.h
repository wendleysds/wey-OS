#ifndef _FORK_H
#define _FORK_H

#include <wey/pid.h>
#include <def/compile.h>

int fork_init();
pid_t kernel_thread(int (*fn)(void*), const char* name, void* args);

#endif
