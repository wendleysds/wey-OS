#include <asm-generic/extable.h>
#include <def/linker.h>
#include <stddef.h>
#include <stdint.h>

static struct exception_entry* extable_start = (struct exception_entry*)&__exception_table_start;
static struct exception_entry* extable_end   = (struct exception_entry*)&__exception_table_end;

struct exception_entry* find_extable(uintptr_t addr){
	const size_t size = (uintptr_t)extable_end - (uintptr_t)extable_start;
	const size_t n = size / sizeof(struct exception_entry);

	for (size_t i = 0; i < n; i++){
		struct exception_entry* entry = extable_start + i;

		if (entry->fault == addr){
			return entry;
		}
	}

	return NULL;
}