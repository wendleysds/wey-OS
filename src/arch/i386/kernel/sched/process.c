#include <def/config.h>
#include <asm/ptrace.h>
#include <wey/sched.h>
#include <lib/string.h>

void start_thread(struct registers* regs, void* entry_point, void* user_stack){
	memset(&regs, 0, sizeof(struct registers));
	regs->cs = USER_CODE_SEGMENT;
	regs->ss = USER_DATA_SEGMENT;
	regs->ip = (uint32_t)entry_point;
	regs->sp = (uint32_t)user_stack;
	regs->flags = (9 << 9); // enable interrupts
}
