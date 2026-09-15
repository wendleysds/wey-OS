#include <kernel/stacktrace.h>
#include <asm/ptrace.h>
#include <def/config.h>

#define STACK_MAX_DEPTH 32

struct stack_frame {
	struct stack_frame *next;
	uintptr_t return_addr;
};

void arch_stack_walk(stack_trace_consume_fn consume_entry, void *cookie, struct registers *regs) {
	if (!consume_entry)
		return;

	uintptr_t ebp = 0;
	uintptr_t eip = 0;

	if (regs) {
		ebp = regs->bp;
		eip = regs->ip;
	} else {
		__asm__ volatile ("mov %%ebp, %0" : "=r" (ebp));
	}

	if (eip >= KERNEL_VIRT_BASE) {
		if (!consume_entry(cookie, eip))
			return;
	}

	struct stack_frame *frame = (struct stack_frame *)ebp;

	int depth = 0;
	while (frame && depth < STACK_MAX_DEPTH) {
		uintptr_t fp = (uintptr_t)frame;

		if (fp & 3) break;
		if (fp < KERNEL_VIRT_BASE) break;

		uintptr_t ret_addr = frame->return_addr;
		if (ret_addr < KERNEL_VIRT_BASE) break;

		if (!consume_entry(cookie, ret_addr))
			break;

		struct stack_frame *next_frame = frame->next;
		if ((uintptr_t)next_frame <= fp) break;

		frame = next_frame;
		depth++;
	}
}
