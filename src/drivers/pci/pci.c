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

    driver->driver.bus = &pci_bus_type;

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

static int test_probe(struct device *dev) {
    struct pci_device *pdev = to_pci_device(dev);
    
    printk(
        "TEST PCI DRIVER: found %04x:%04x\n",
        pdev->vendor_id,
        pdev->device_id
    );

	struct resource *res = dev->resources;
	while(res){
		const char* type;
		uint64_t base, size;
		if(res->type == RESOURCE_TYPE_MEMORY){
			type = "MEM";
			base = res->range.base;
			size = res->range.size;
		} else if(res->type == RESOURCE_TYPE_IO){
			type = "IO";
			base = res->io.base;
			size = res->io.size;
		} else if(res->type == RESOURCE_TYPE_IRQ){
			type = "IRQ";
			base = res->irq;
			size = 1;
		}else{
			type = "UNKNOWN";
			base = 0;
			size = 0;
		}

		printk("     Resource %s: %#llx - %#llx\n", type, base, size);
		res = res->sibling;
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
	.id_table = test_ids,

	.driver = {
		.name = "pci-test",
		.probe = test_probe,
	},
};

static __init int test_driver_init(void){
	return pci_register_driver(&test_driver);
}

device_initcall(test_driver_init);

