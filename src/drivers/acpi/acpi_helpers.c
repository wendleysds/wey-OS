#include <kernel/acpi.h>
#include <kernel/printk.h>
#include <kernel/init.h>

#include <def/config.h>
#include <def/errno.h>
#include <mm/iomem.h>
#include <asm/page.h>

#include <stdint.h>
#include <stddef.h>

bool __init acpi_checksum_ok(const void *addr, size_t len){
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

void* acpi_map(paddr_t phys, size_t size){
	if(size > UINTPTR_MAX - phys) return NULL;

	paddr_t end = phys + size;
	if(end < KERNEL_DIRECTMAP_SIZE){
		return (void*)__va(phys);
	}

	return ioremap(phys, size);
}

void acpi_unmap(void __iomem *virt){
	vaddr_t vaddr = (vaddr_t)virt;
	if (vaddr >= KERNEL_DIRECTMAP_START && vaddr < KERNEL_DIRECTMAP_END) {
		return;
	}

	iounmap(virt);
}

__init struct acpi_sdt_header* acpi_parse_sdt_header(paddr_t physaddr){
	if(physaddr == 0) return NULL;

	struct acpi_sdt_header *tmp = acpi_map(physaddr, sizeof(struct acpi_sdt_header));
	if(!tmp) return NULL;

	struct acpi_sdt_header *sdt = acpi_map(physaddr, tmp->length);
	acpi_unmap(tmp);

	if(!sdt) return NULL;

	if(!acpi_checksum_ok(sdt, sdt->length)) {
		printk("ACPI: Invalid table '%.4s' checksum\n", sdt->signature);
		acpi_unmap(sdt);
		return NULL;
	}

	return sdt;
}
