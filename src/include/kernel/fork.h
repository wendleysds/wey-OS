#ifndef _FORK_H
#define _FORK_H

#include <kernel/pid.h>

pid_t kernel_thread(int (*fn)(void*), const char* name, void* args);

#endif
