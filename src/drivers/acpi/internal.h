#include <stddef.h>
#include <stdbool.h>

#include <kernel/acpi/tables.h>

bool acpi_checksum_ok(const void *ptr, size_t len);
int acpi_gas_to_io_region(const struct acpi_generic_address *gas, io_region_t *region);

void* acpi_map(paddr_t phys, size_t size);
void acpi_unmap(void __iomem* virt);

struct acpi_sdt_header *acpi_parse_sdt_header(paddr_t physaddr);
int acpi_add_table(paddr_t physaddr);
void acpi_remove_table(paddr_t physaddr);

struct aml_object;
void acpi_register_device(struct aml_object *obj);