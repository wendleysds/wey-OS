#include <def/config.h>
#include <asm/ptrace.h>
#include <asm/cpuflags.h>
#include <asm/cpu.h>
#include <wey/sched.h>
#include <lib/string.h>


extern asmlinkage void _switch_to(struct registers* prev, struct registers* to);
extern asmlinkage __no_return void _ret_from_interrupt(struct registers* regs);

static void start_thread_common(
	struct registers* regs, void* entry_point, void* user_stack, 
	unsigned long cs, unsigned long ss
){
	memset(regs, 0, sizeof(struct registers));
	regs->cs = GDT_USER_CODE;
	regs->ss = GDT_USER_DATA;
	regs->ip = (uint32_t)entry_point;
	regs->sp = (uint32_t)user_stack;
	regs->flags = (X86_EFLAGS_IF | X86_EFLAGS_FIXED); // enable interrupts
}

void start_thread_user(struct registers* regs, void* entry_point, void* user_stack){
	start_thread_common(regs, entry_point, user_stack, GDT_USER_CODE, GDT_USER_DATA);
}

void start_thread_kernel(struct registers* regs, void* entry_point, void* user_stack){
	start_thread_common(regs, entry_point, user_stack, GDT_KERNEL_CODE, GDT_KERNEL_DATA);
}

int copy_thread(struct task *p, struct task *c);

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
}
