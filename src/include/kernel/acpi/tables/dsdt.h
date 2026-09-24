#ifndef _ACPI_TABLE_DSDT
#define _ACPI_TABLE_DSDT

/* Differentiated System Description Table */

#include <kernel/acpi.h>

#define ACPI_DSDT_SIGNATURE "DSDT"

struct acpi_dsdt {
	struct acpi_sdt_header header;
	uint8_t definition_block[];
};

#endif
