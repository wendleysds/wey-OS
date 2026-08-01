#ifndef _ASM_GENERIC_EXTABLE_H
#define _ASM_GENERIC_EXTABLE_H

#include <stdint.h>

struct exception_entry {
	uintptr_t fault;
	uintptr_t fixup;
};

struct exception_entry* find_extable(uintptr_t addr);

#endif