#include <kernel/acpi.h>
#include <kernel/printk.h>
#include <def/errno.h>
#include <stdint.h>
#include <stddef.h>

bool acpi_checksum_ok(const void *addr, size_t len){
    u8 sum = 0;

    for (size_t i = 0; i < len; i++){
        sum += ((u8*)addr)[i];
    }

    return sum == 0;
}

int acpi_gas_to_io_region(const struct acpi_generic_address *gas, io_region_t *region){
	switch (gas->address_space_id) {
		case ACPI_ADDRESS_SPACE_SYSTEM_MEMORY:
			region->type = IO_TYPE_MMIO;
			region->mmio_base = (uintptr_t)gas->address;
			return 0;

		case ACPI_ADDRESS_SPACE_SYSTEM_IO:
			region->type = IO_TYPE_PIO;
			region->pio_base = (uint16_t)gas->address;
			return 0;

		default:
            printk("ACPI: Unsupported address space id %d\n", gas->address_space_id);
			return -EOPNOTSUPP;
	}
}