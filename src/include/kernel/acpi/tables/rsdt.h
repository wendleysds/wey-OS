#ifndef _ACPI_TABLE_RSDT
#define _ACPI_TABLE_RSDT

/* Root System Description Table */

#include <def/compile.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_RSDT_SIGNATURE "RSDT"
#define RSDT_ENTRY_SIZE sizeof(uint32_t)

#define RSDT_ENTRIES_LEN(ptr) \
	((ptr)->header.length - sizeof(struct acpi_sdt_header)) / RSDT_ENTRY_SIZE
#define RSDT_ENTRY(ptr, index) \
	(((ptr)->entries)[(index)])

struct acpi_rsdt {
	struct acpi_sdt_header header;
	uint32_t entries[];
} __packed;

#endif