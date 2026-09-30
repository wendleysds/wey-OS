#include <kernel/init.h>
#include <kernel/printk.h>

#include "../internal.h"

static void list_devices(aml_object_t* obj) {
	if(obj->type != AML_OBJ_TYPE_DEVICE) return;
	if(!obj->device.resources) return;
	
	if(obj->device.hid[0] != '\0'){
		printk("ACPI:  HID: %s\n", obj->device.hid);
	}
	if(obj->device.cid[0] != '\0'){
		printk("ACPI:  CID: %s\n", obj->device.cid);
	}

	acpi_resource_t* res = obj->device.resources;
	while (res) {
		if (res->type == ACPI_RESOURCE_TYPE_MMIO) {
			printk("ACPI:    MMIO: 0x%lx - 0x%lx\n", res->mmio.base, res->mmio.base + res->mmio.length);
		} else if (res->type == ACPI_RESOURCE_TYPE_IO) {
			printk("ACPI:    IO: 0x%x - 0x%x\n", res->io.base, res->io.base + res->io.length);
		} else if (res->type == ACPI_RESOURCE_TYPE_IRQ) {
			printk("ACPI:    IRQ: 0x%x\n", res->irq_mask);
		}
		res = res->next;
	}
}

void __init acpi_parse_dsdt(struct acpi_dsdt *dsdt) {
	const uint8_t *aml = dsdt->definition_block;
	const size_t length = dsdt->header.length - sizeof(struct acpi_sdt_header);

	acpi_load_namespace(aml, length);

	printk("ACPI: DSDT parsed\n");
	printk("ACPI: Devices with resources:\n");
	acpi_namespace_walk(list_devices);
}
