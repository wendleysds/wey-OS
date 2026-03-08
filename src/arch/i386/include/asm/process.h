#ifndef _X86_PROCESS_H
#define _X86_PROCESS_H

#include <asm/cpu.h>
#include <def/compile.h>

#define current get_cpu()->current

struct task;
struct registers;

extern asmlinkage void ret_from_registers(struct registers* regs);

void start_thread_user(struct registers* regs, void* entry_point, void* user_stack);
void start_thread_kernel(struct registers* regs, void* entry_point, void* kernel_stack);

void copy_thread(struct task *p, struct task *c, int (*fn)(void*), void* args);
void context_switch(struct task* prev, struct task* to);

#endif
