#include <kernel/device.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <device/pci.h>
#include <def/errno.h>
#include <def/bits.h>
#include <io/ports.h>
#include <mm/kheap.h>

/*

[x] PCI configuration access
[x] PCI enumeration
[x] pci_bus
[x] pci_device
[x] header parsing
[x] BAR detection
[x] BAR size
[x] capabilities
[ ] find capabilities
[x] driver registry
[x] device/driver matching
[x] probe()
[x] pci_enable_device()
[ ] bus mastering
[ ] MMIO mapping
[ ] IRQ
[ ] MSI
[ ] MSI-X
[ ] PCI bridge recursion -> pain (last)

*/

static LIST_HEAD(pci_busses);
static LIST_HEAD(pci_drivers);

extern const struct pci_config_ops pci_config_ops_m1;
extern const struct pci_config_ops pci_config_ops_m2;
extern const struct pci_config_ops pci_config_ops_ecam;

static struct pci_bus* alloc_pci_bus(const struct pci_config_ops* ops){
	struct pci_bus* bus = kzalloc(sizeof(struct pci_bus));
	if(bus){
		INIT_LIST_HEAD(&bus->devices);
		INIT_LIST_HEAD(&bus->node);
		bus->config = ops;
	}

	return bus;
}


static void _pci_dump_devices(void){
	struct pci_bar* bar;
	struct pci_bus* bus;
	struct pci_device* device;

	list_for_each_entry(bus, &pci_busses, node){
		printk("PCI: Bus (%d)\n", bus->number);
		list_for_each_entry(device, &bus->devices, node){
			printk("      '-- %d.%d ",
	  			PCI_SLOT(device->devfn), PCI_FUNC(device->devfn)
			);

			printk("Vendor: %#X Device: %#X\n",
				device->vendor_id, device->device_id
			);

			int count = 6;

			if(device->header_type == PCI_HEADER_TYPE_BRIDGE){
				count = 2;
			}

			for(int i = 0; i < count; i++){

				bar = device->header_type == PCI_HEADER_TYPE_BRIDGE ?
					&device->header.bridge.bars[i] :
					&device->header.general.bars[i];
				
				if(bar->type == PCI_BAR_UNUSED) continue;
				printk("            BAR[%d]: %s: %#llx - %#llx\n",
					i,
					(bar->type == PCI_BAR_IO) ? "IO" : "MEM",
					bar->base,
					bar->base + bar->size
				);
			}
		}

		printk("\n");
	}
}

static const struct pci_device_id *
pci_match_id(
    struct pci_device *dev,
    const struct pci_device_id *ids
) {
    for (; ids; ids++) {

        if (ids->vendor == 0 &&
            ids->device == 0 &&
            ids->class == 0)
            break;

        if (ids->vendor != PCI_ANY_ID &&
            ids->vendor != dev->vendor_id)
            continue;

        if (ids->device != PCI_ANY_ID &&
            ids->device != dev->device_id)
            continue;

        if (ids->class != PCI_ANY_CLASS &&
            ids->class != dev->class_code)
            continue;

        if (ids->subclass != PCI_ANY_CLASS &&
            ids->subclass != dev->subclass)
            continue;

        if (ids->prog_if != PCI_ANY_CLASS &&
            ids->prog_if != dev->prog_if)
            continue;

        return ids;
    }

    return NULL;
}

static int pci_probe_device(struct pci_device *dev){
	struct pci_driver* driver;
	const struct pci_device_id* id;

	list_for_each_entry(driver, &pci_drivers, node){
		id = pci_match_id(dev, driver->id_table);
		if(!id) continue;

		int ret = driver->probe(dev, id);

		if(ret){
			printk("PCI: %d:%d.%d: driver \"%s\" failed (%d)\n",
				dev->bus_num, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
				driver->name, ret
			);

			continue;
		}

		dev->driver = driver;

		printk("PCI: %d:%d.%d: driver \"%s\"\n",
			dev->bus_num, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
			driver->name, ret
		);

		return 0;
	}

	return -ENODEV;
}

int pci_register_driver(struct pci_driver *driver){
	if(!driver || !driver->id_table){
		return -EINVAL;
	}

	INIT_LIST_HEAD(&driver->node);
	list_add_tail(&driver->node, &pci_drivers);

	struct pci_bus* bus;
	struct pci_device* device;
	list_for_each_entry(bus, &pci_busses, node){
		list_for_each_entry(device, &bus->devices, node){
			if(device->driver) continue;

			pci_probe_device(device);
		}
	}


	return SUCCESS;
}

static __init int pci_init(void){
	// Check from the firmware the pci config mechanism
	// For now we just assume mechanism #1 (Ports)
	const struct pci_config_ops* ops = &pci_config_ops_m1;

	struct pci_bus* bus = alloc_pci_bus(ops);
	size_t total = 0;
	int res = 0;

	for(uint16_t busnum = 0; busnum < 256; busnum++){
		if(!bus) return -ENOMEM;
		bus->number = busnum;

		if((res = pci_scan_bus(bus)) < 0){
			return res;
		}

		if(res == 0){
			continue;
		} 

		total += res;
		
		list_add_tail(&bus->node, &pci_busses);

		bus = alloc_pci_bus(ops);
	}

	if(bus){
		kfree(bus);
	}

	printk("PCI: Parsed %d devices\n", total);

	
	return res;
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

