#include <stddef.h>
#include <stdbool.h>

#include <kernel/acpi/tables.h>

bool acpi_checksum_ok(const void *ptr, size_t len);
int acpi_gas_to_io_region(const struct acpi_generic_address *gas, io_region_t *region);

void *acpi_find_table(const char *signature);
void acpi_parse_fadt(struct acpi_fadt *ptr);