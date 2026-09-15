#ifndef _KERNEL_KALLSYMS_H
#define _KERNEL_KALLSYMS_H

#include <stdint.h>
#include <stddef.h>

struct kallsym_entry {
	uintptr_t addr;
	const char *name;
};

extern const struct kallsym_entry kallsyms_names[];
extern const size_t kallsyms_num_symbols;

int kallsyms_lookup_symbol(uintptr_t addr, const char **name_out, uintptr_t *offset_out);

#endif
