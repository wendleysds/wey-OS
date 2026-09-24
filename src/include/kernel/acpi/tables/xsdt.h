#ifndef _ACPI_TABLE_XSDT
#define _ACPI_TABLE_XSDT

/* eXtended System Descriptor Table */

#include <def/compile.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_XSDT_SIGNATURE "XSDT"

#define XSDT_ENTRY_SIZE sizeof(uint64_t)

#define XSDT_ENTRIES_LEN(ptr) \
	((((ptr)->header.length) - sizeof(struct acpi_sdt_header)) / XSDT_ENTRY_SIZE)
#define XSDT_ENTRY(ptr, index) \
	(((ptr)->entries)[(index)])

struct acpi_xsdt {
	struct acpi_sdt_header header;
	uint64_t entries[];
} __packed;

#endif