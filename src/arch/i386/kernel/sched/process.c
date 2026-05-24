#include "wey/mmu.h"
#include <def/config.h>
#include <def/status.h>
#include <asm/ptrace.h>
#include <asm/cpuflags.h>
#include <asm/cpu.h>
#include <stdint.h>
#include <wey/sched.h>
#include <wey/fork.h>
#include <lib/string.h>

extern asmlinkage void _switch_to(struct task* prev, struct task* to);
extern asmlinkage __no_return void ret_from_fork();
extern asmlinkage __no_return void kernel_thread_trampoline(int (*fn)(void*), void* args);

static void start_thread_common(
	struct registers* regs, void* entry_point, 
	unsigned long cs, unsigned long ss
){
	memset(regs, 0, sizeof(struct registers));
	regs->cs = cs;
	regs->ss = ss;
	regs->ip = (uint32_t)entry_point;
	regs->flags = (X86_EFLAGS_IF | X86_EFLAGS_FIXED); // enable interrupts
}

void start_thread_user(struct registers* regs, void* entry_point, void* user_stack){
	start_thread_common(regs, entry_point, GDT_USER_CODE, GDT_USER_DATA);
	regs->sp = (uint32_t)user_stack;
}

void start_thread_kernel(struct registers* regs, void* entry_point, void* kernel_stack){
	start_thread_common(regs, entry_point, GDT_KERNEL_CODE, GDT_KERNEL_DATA);
	regs->ksp = (uint32_t)kernel_stack;
}

void copy_thread(struct task *p, struct task *c, int (*fn)(void*), void* args){
	unsigned long* ksp = c->kstack + PROC_KERNEL_STACK_SIZE;

	// Create kernel thread
	if(!p){
		start_thread_kernel(
			&c->regs,
			kernel_thread_trampoline,
			0x0 // setted later
		);

		*(--ksp) = (unsigned long)args;
		*(--ksp) = (unsigned long)fn;
		goto out;
	}

	// Create user thread
	memcpy(&c->regs, &p->regs, sizeof(struct registers));
	c->regs.ax = 0;

out:
	ksp = (unsigned long*)((uint8_t*)ksp - sizeof(struct registers));
	memcpy(ksp, &c->regs, sizeof(struct registers));

	*(--ksp) = (unsigned long)ret_from_fork;
	c->regs.ksp = (unsigned long)ksp;
	return;
}

void context_switch(struct task* prev, struct task* to){
	#undef current
	struct cpu* cpu = get_cpu();

	if(prev == to || to == cpu->current){
		return;
	}

	// TODO: do some checks here, like if the tasks are valid or permissions

	cpu->current = to;
	cpu->current->state = TASK_RUNNING;
	cpu->tss.esp0 = (unsigned long)(to->kstack + PROC_KERNEL_STACK_SIZE);

	if(to->mm){
		mmu_context_switch(to->mm->ctx);
	}

	_switch_to(prev, to);
}
