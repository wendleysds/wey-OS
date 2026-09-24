#include <kernel/init.h>
#include "../internal.h"

void __init acpi_parse_rsdt(struct acpi_rsdt *rsdt) {
	size_t entries = RSDT_ENTRIES_LEN(rsdt);
	for (size_t i = 0; i < entries; i++) {
		acpi_add_table(RSDT_ENTRY(rsdt, i));
	}
}
