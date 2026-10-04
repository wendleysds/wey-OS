#include <kernel/device.h>
#include <kernel/resource.h>
#include <kernel/acpi.h>
#include <kernel/init.h>
#include <kernel/printk.h>

#include <device/acpi.h>

#include <mm/kheap.h>
#include <lib/string.h>
#include <def/errno.h>

static int acpi_bus_match (struct device *dev, struct device_driver *drv);

struct bus_type acpi_bus_type = {
    .name = "acpi",
    .match = acpi_bus_match,
};

static const struct acpi_device_id *acpi_match_id(
    const struct acpi_device *adev,
    const struct acpi_device_id *ids
){
    for (; ids->id[0] != '\0'; ids++) {
        if (adev->hid[0] != '\0' && strcmp(adev->hid, ids->id) == 0)
            return ids;
        if (adev->cid[0] != '\0' && strcmp(adev->cid, ids->id) == 0)
            return ids;
    }
    return NULL;
}

static int acpi_bus_match(struct device *dev, struct device_driver *drv)
{
    struct acpi_device *adev = to_acpi_device(dev);
    struct acpi_driver *adrv = to_acpi_driver(drv);

    if (!adrv->id_table)
        return 0;

    return acpi_match_id(adev, adrv->id_table) != NULL;
}

int acpi_register_driver(struct acpi_driver *adrv)
{
    if (!adrv || !adrv->id_table)
        return -EINVAL;

    adrv->driver.bus = &acpi_bus_type;

    driver_register(&adrv->driver);
    return 0;
}

void acpi_unregister_driver(struct acpi_driver *adrv)
{
    if (!adrv)
        return;
    driver_unregister(&adrv->driver);
}

/*
 * acpi_build_resources - Convert the ACPI-internal acpi_resource_t linked
 *   list into generic struct resource objects and attach them to dev via
 *   device_add_resource().
 *
 * Each struct resource is kzalloc'd here and lives for the device's lifetime.
 */
static void acpi_build_resources(struct device *dev, acpi_resource_t *list)
{
    for (acpi_resource_t *ar = list; ar; ar = ar->next) {
        struct resource *res = kzalloc(sizeof(struct resource));
        if (!res)
            return;

        switch (ar->type) {
        case ACPI_RESOURCE_TYPE_IO:
            res->type  = RESOURCE_TYPE_IO;
            res->flags = RESOURCE_FLAG_IO | RESOURCE_FLAG_READABLE |
                         RESOURCE_FLAG_WRITABLE;
            res->io.base  = ar->io.base;
            res->io.size  = ar->io.length;
            res->name  = "ACPI IO";
            break;

        case ACPI_RESOURCE_TYPE_MMIO:
            res->type  = RESOURCE_TYPE_MEMORY;
            res->flags = RESOURCE_FLAG_MMIO | RESOURCE_FLAG_READABLE |
                         (ar->mmio.writeable ? RESOURCE_FLAG_WRITABLE : 0);
            res->range.base  = ar->mmio.base;
            res->range.size  = ar->mmio.length;
            res->name  = "ACPI MMIO";
            break;

        case ACPI_RESOURCE_TYPE_IRQ:
            res->type  = RESOURCE_TYPE_IRQ;
            res->flags = 0;
            res->name  = "ACPI IRQ";
            res->irq = ar->irq_mask; // Mask
            break;

        default:
            kfree(res);
            continue;
        }

        device_add_resource(dev, res);
    }
}

void acpi_register_device(aml_object_t *obj)
{
    if (!obj || obj->type != AML_OBJ_TYPE_DEVICE)
        return;

    if (obj->device.hid[0] == '\0' && obj->device.cid[0] == '\0')
        return;

    if (!obj->device.resources) {
        printk("ACPI: device %s has no \"static\" resources, skipping...\n",
               obj->device.hid[0] ? obj->device.hid : obj->device.cid);
        return;
    }

    struct acpi_device *adev = kzalloc(sizeof(struct acpi_device));
    if (!adev) {
        printk("ACPI: failed to allocate acpi_device for %s\n",
               obj->device.hid[0] ? obj->device.hid : obj->device.cid);
        return;
    }

    strncpy(adev->hid, obj->device.hid, sizeof(adev->hid) - 1);
    strncpy(adev->cid, obj->device.cid, sizeof(adev->cid) - 1);

    device_initialize(&adev->dev);
    adev->dev.bus    = &acpi_bus_type;
    adev->dev.parent = NULL;

    const char *name = adev->hid[0] ? adev->hid : adev->cid;
    adev->dev.name = strdup(name);
    if (!adev->dev.name) {
        kfree(adev);
        return;
    }

    /* Convert ACPI resources -> generic struct resource list */
    if (obj->device.resources)
        acpi_build_resources(&adev->dev, obj->device.resources);

    int ret = device_register(&adev->dev);
    if (ret) {
        printk("ACPI: failed to register device %s (%d)\n", name, ret);
        kfree((void *)adev->dev.name);
        kfree(adev);
        return;
    }

    printk("ACPI: registered device %s", name);
    if (adev->cid[0] && strcmp(adev->hid, adev->cid) != 0)
        printk(" (CID: %s)", adev->cid);
    printk("\n");
}

static __init int acpi_bus_init(void)
{
    bus_register(&acpi_bus_type);
    return 0;
}

postcore_initcall(acpi_bus_init);
