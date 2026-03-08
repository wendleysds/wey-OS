#include <def/config.h>
#include <def/status.h>
#include <asm/ptrace.h>
#include <asm/cpuflags.h>
#include <asm/cpu.h>
#include <stdint.h>
#include <wey/sched.h>
#include <wey/fork.h>
#include <lib/string.h>

extern asmlinkage void _switch_to(struct registers* prev, struct registers* to);
extern asmlinkage __no_return void _ret_from_interrupt(struct registers* regs);
extern asmlinkage __no_return void ret_from_fork(unsigned long from_user);

static void start_thread_common(
	struct registers* regs, void* entry_point, void* stack, 
	unsigned long cs, unsigned long ss
){
	memset(regs, 0, sizeof(struct registers));
	regs->cs = cs;
	regs->ss = ss;
	regs->ip = (uint32_t)entry_point;
	regs->flags = (X86_EFLAGS_IF | X86_EFLAGS_FIXED); // enable interrupts
}

void start_thread_user(struct registers* regs, void* entry_point, void* user_stack){
	start_thread_common(regs, entry_point, user_stack, GDT_USER_CODE, GDT_USER_DATA);
	regs->sp = (uint32_t)user_stack;
}

void start_thread_kernel(struct registers* regs, void* entry_point, void* kernel_stack){
	start_thread_common(regs, entry_point, kernel_stack, GDT_KERNEL_CODE, GDT_KERNEL_DATA);
	regs->ksp = (uint32_t)kernel_stack;
}

void copy_thread(struct task *p, struct task *c, int (*fn)(void*), void* args){
	// Create kernel thread
	if(!p){
		unsigned long* ksp = c->kstack + PROC_KERNEL_STACK_SIZE;
		*(--ksp) = (unsigned long)args;
		*(--ksp) = (unsigned long)fn;
		*(--ksp) = 0;
		*(--ksp) = (unsigned long)ret_from_fork;
		
		c->regs.ksp = (unsigned long)ksp;
		return;
	}

	memcpy(&c->regs, &p->regs, sizeof(struct registers));
	uintptr_t offset = p->regs.ksp - (uintptr_t)p->kstack;
	c->regs.ksp = (uintptr_t)c->kstack + offset;
	c->regs.ax = 0;
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

	if(unlikely(!prev)){
		_switch_to(NULL, &to->regs);
	}

	_switch_to(&prev->regs, &to->regs);

	if(prev && prev->state == TASK_ZOMBIE){
		if(prev->kstack){
			kfree(prev->kstack);
			prev->kstack = NULL;
		}
	}
}
