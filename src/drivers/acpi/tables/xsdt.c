#include <kernel/init.h>
#include "../internal.h"

void __init acpi_parse_xsdt(struct acpi_xsdt *xsdt){
	size_t entries = XSDT_ENTRIES_LEN(xsdt);
	for (size_t i = 0; i < entries; i++) {
		acpi_add_table(XSDT_ENTRY(xsdt, i));
	}
}