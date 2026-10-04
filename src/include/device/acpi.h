#ifndef _DEVICE_ACPI_H
#define _DEVICE_ACPI_H

#include <kernel/device.h>
#include <kernel/resource.h>
#include <lib/list.h>

/*
 * ACPI device
 *
 * Represents a device node discovered in the ACPI namespace (DSDT/SSDT).
 * Embeds struct device so it participates in the device subsystem.
 *
 * Generic drivers should use device_get_resource() on dev->dev for
 * bus-agnostic resource access.  The ACPI subsystem converts all
 * acpi_resource_t entries to struct resource during registration.
 */
struct acpi_device {
    struct device dev;  /* must be first */
    char hid[9];        /* Hardware ID  (_HID), e.g. "PNP0501" */
    char cid[9];        /* Compatible ID (_CID), optional       */
};

/*
 * Last entry must be zero-initialised (sentinel).
 */
struct acpi_device_id {
    char id[9];         /* HID or CID string, e.g. "PNP0501" */
};

struct acpi_driver {
    struct device_driver driver;
    const struct acpi_device_id *id_table;
};

/* Register / unregister an ACPI driver */
int  acpi_register_driver  (struct acpi_driver *drv);
void acpi_unregister_driver(struct acpi_driver *drv);

/* Upcast a generic struct device to acpi_device */
#define to_acpi_device(dev)  container_of(dev, struct acpi_device, dev)
#define to_acpi_driver(drv)  container_of(drv, struct acpi_driver, driver)

/* Driver private data helpers */
static inline void *acpi_get_drvdata(struct acpi_device *adev)
{
    return adev->dev.driver_data;
}

static inline void acpi_set_drvdata(struct acpi_device *adev, void *data)
{
    adev->dev.driver_data = data;
}

/*
 * Resource access helpers
 *
 * Generic drivers should prefer these over the ACPI-internal acpi_resource_t.
 * Thin wrappers around device_get_resource() for convenience.
 */
static inline struct resource *acpi_get_resource(
    struct acpi_device *adev,
    resource_type_t type,
    unsigned int idx
) {
    return device_get_resource(&adev->dev, type, idx);
}

#define acpi_for_each_resource(res, adev) \
    for ((res) = (adev)->dev.resources; (res) != NULL; (res) = (res)->sibling)

/* Global ACPI bus type (registered during acpi_bus_init) */
extern struct bus_type acpi_bus_type;

#endif
