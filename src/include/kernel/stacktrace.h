#ifndef _KERNEL_STACKTRACE_H
#define _KERNEL_STACKTRACE_H

#include <stdbool.h>
#include <stdint.h>

struct registers;

typedef bool (*stack_trace_consume_fn)(void *cookie, uintptr_t ip);

void dump_stack(struct registers *regs);
void arch_stack_walk(stack_trace_consume_fn consume_entry, void *cookie, struct registers *regs);

#endif
