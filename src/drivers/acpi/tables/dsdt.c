#include <kernel/init.h>
#include <kernel/printk.h>

#include <device/acpi.h>

#include "../internal.h"

static void acpi_enumerate_device(aml_object_t *obj)
{
	if (obj->type != AML_OBJ_TYPE_DEVICE)
		return;

	/* Devices without any identity cannot be matched by a driver */
	if (obj->device.hid[0] == '\0' && obj->device.cid[0] == '\0')
		return;

	acpi_register_device(obj);
}

void __init acpi_parse_dsdt(struct acpi_dsdt *dsdt)
{
	const uint8_t *aml    = dsdt->definition_block;
	const size_t   length = dsdt->header.length - sizeof(struct acpi_sdt_header);

	acpi_load_namespace(aml, length);

	acpi_namespace_walk(acpi_enumerate_device);
}
