#ifndef _ACPI_TABLE_RSDT
#define _ACPI_TABLE_RSDT

/* Root System Description Table */

#include <def/compile.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_RSDT_SIGNATURE "RSDT"

struct acpi_rsdt {
	struct acpi_sdt_header header;
	uint32_t entries[];
} __packed;

#endif