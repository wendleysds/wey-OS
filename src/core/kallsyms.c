#include <kernel/kallsyms.h>
#include <def/compile.h>
#include <def/errno.h>

__weak const struct kallsym_entry kallsyms_names[1] = { {0, NULL} };
__weak const size_t kallsyms_num_symbols = 0;

int kallsyms_lookup_symbol(uintptr_t addr, const char **name_out, uintptr_t *offset_out) {
	if (!name_out || !offset_out)
		return -EINVAL;

	if (kallsyms_num_symbols == 0)
		return -ENOENT;

	if (addr < kallsyms_names[0].addr)
		return -ENOENT;

	size_t low = 0;
	size_t high = kallsyms_num_symbols;

	while (low < high) {
		size_t mid = low + (high - low) / 2;
		if (kallsyms_names[mid].addr <= addr) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	size_t idx = low - 1;
	*name_out = kallsyms_names[idx].name;
	*offset_out = addr - kallsyms_names[idx].addr;
	return 0;
}
