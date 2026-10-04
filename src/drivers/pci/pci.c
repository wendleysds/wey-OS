#include <kernel/device.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <device/pci.h>
#include <def/errno.h>
#include <def/bits.h>
#include <io/ports.h>
#include <mm/kheap.h>

#include "internal.h"

/*
[x] PCI configuration access
[x] PCI enumeration
[x] pci_bus
[x] pci_device
[x] header parsing
[x] BAR detection
[x] BAR size
[x] capabilities
[x] find capabilities
[x] driver registry
[x] device/driver matching
[x] probe()
[x] pci_enable_device()
[x] PCI bridge recursion
[ ] bus mastering
[ ] MMIO mapping
[ ] IRQ
[ ] MSI
[ ] MSI-X
*/

static LIST_HEAD(pci_busses);

extern const struct pci_config_ops pci_config_ops_m1;
extern const struct pci_config_ops pci_config_ops_m2;
extern const struct pci_config_ops pci_config_ops_ecam;

const struct pci_config_ops *pci_config_ops = NULL;

struct bus_type pci_bus_type = {
	.name = "pci",
	.match = pci_driver_match,
	.probe = pci_device_probe,
	.remove = pci_device_remove,
};

struct pci_bus *pci_alloc_bus(const struct pci_config_ops *ops){
	struct pci_bus *bus = kzalloc(sizeof(struct pci_bus));
	if (bus) {
		INIT_LIST_HEAD(&bus->devices);
		INIT_LIST_HEAD(&bus->node);
		bus->config = ops;
	}

	return bus;
}

struct pci_bus *pci_find_bus(uint8_t number){
	struct pci_bus *bus;

	list_for_each_entry(bus, &pci_busses, node) {
		if (bus->number == number)
			return bus;
	}

	return NULL;
}

void pci_add_bus(struct pci_bus *bus){
	if (!bus || !list_empty(&bus->node))
		return;

	list_add_tail(&bus->node, &pci_busses);
}

int pci_register_driver(struct pci_driver *driver){
	if(!driver || !driver->id_table){
		return -EINVAL;
	}

	driver->driver.name = driver->name;
    driver->driver.bus = &pci_bus_type;
    driver->driver.probe = pci_device_probe;
    driver->driver.remove = pci_device_remove;

	driver_register(&driver->driver);
	return SUCCESS;
}

static __init int pci_init(void){
	// Check from the firmware the pci config mechanism
	// For now we just assume mechanism #1 (Ports)
	pci_set_config_ops(&pci_config_ops_m1);

	struct pci_bus *root = pci_alloc_bus(pci_config_ops);
	if (!root)
		return -ENOMEM;

	root->number = 0;
	pci_add_bus(root);

	int res = pci_scan_bus(root);
	if (res < 0)
		return res;

	return SUCCESS;
}

subsys_initcall(pci_init);

static int test_probe(
    struct pci_device *dev,
    const struct pci_device_id *id
) {
    printk(
        "TEST PCI DRIVER: found %04x:%04x\n",
        dev->vendor_id,
        dev->device_id
    );

    for (int i = 0; i < 6; i++) {

        struct pci_bar *bar =
            &dev->header.general.bars[i];

        if (bar->type == PCI_BAR_UNUSED)
            continue;

        printk(
            "  BAR%d base=%#llx size=%#llx\n",
            i,
            bar->base,
            bar->size
        );
    }

    return 0;
}

static const struct pci_device_id test_ids[] = {
    {
        .vendor = 0x8086,
        .device = 0x100E,

        .class = PCI_ANY_CLASS,
        .subclass = PCI_ANY_CLASS,
        .prog_if = PCI_ANY_CLASS,
    },

    { 0 }
};

static struct pci_driver test_driver = {
    .name = "pci-test",

    .id_table = test_ids,

    .probe = test_probe,
};

static __init int test_driver_init(void){
	return pci_register_driver(&test_driver);
}

device_initcall(test_driver_init);

