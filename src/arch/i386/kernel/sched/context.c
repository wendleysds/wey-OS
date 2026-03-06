#include <wey/sched.h>
#include <asm/cpu.h>
#include <def/compile.h>

struct task* current = 0x0;

extern asmlinkage void _switch_to(struct registers* prev, struct registers* to);
extern asmlinkage __no_return void _ret_from_interrupt(struct registers* regs);

void context_switch(struct task* prev, struct task* to){
	if(prev == to || to == current){
		return;
	}

	// TODO: do some checks here, like if the tasks are valid or permissions

	current = to;
	current->state = TASK_RUNNING;

	get_cpu()->tss.esp0 = (unsigned long)(to->kstack + PROC_KERNEL_STACK_SIZE);

	if(unlikely(!prev)){
		_switch_to(NULL, &to->regs);
	}

	_switch_to(&prev->regs, &to->regs);
}
