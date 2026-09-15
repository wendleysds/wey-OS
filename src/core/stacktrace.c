#include <kernel/stacktrace.h>
#include <kernel/kallsyms.h>
#include <kernel/printk.h>

static bool print_trace_address(void *cookie, uintptr_t ip) {
	const char *name = NULL;
	uintptr_t offset = 0;

	if (kallsyms_lookup_symbol(ip, &name, &offset) == 0) {
		printk("  [<%08lx>] %s+0x%lx\n", (unsigned long)ip, name, (unsigned long)offset);
	} else {
		printk("  [<%08lx>] <unknown>\n", (unsigned long)ip);
	}
	return true;
}

void dump_stack(struct registers *regs) {
	printk("Call Trace:\n");
	arch_stack_walk(print_trace_address, NULL, regs);
}
