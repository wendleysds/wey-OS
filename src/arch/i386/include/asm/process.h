#ifndef _X86_PROCESS_H
#define _X86_PROCESS_H

#include <asm/cpu.h>

#define current get_cpu()->current

struct task;
struct registers;

void start_thread_user(struct registers* regs, void* entry_point, void* user_stack);
void start_thread_kernel(struct registers* regs, void* entry_point, void* user_stack);

int copy_thread(struct task *p, struct task *c);
void context_switch(struct task* prev, struct task* to);

#endif
