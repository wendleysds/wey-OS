#include <def/config.h>
#include <asm/ptrace.h>
#include <asm/cpuflags.h>
#include <wey/sched.h>
#include <lib/string.h>

void start_thread(struct registers* regs, void* entry_point, void* user_stack){
	memset(&regs, 0, sizeof(struct registers));
	regs->cs = GDT_USER_CODE;
	regs->ss = GDT_USER_DATA;
	regs->ip = (uint32_t)entry_point;
	regs->sp = (uint32_t)user_stack;
	regs->flags = (X86_EFLAGS_IF | X86_EFLAGS_FIXED); // enable interrupts
}
