#include <def/errno.h>

#include "internal.h"

const struct pci_device_id *pci_match_id(
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

int pci_driver_match(struct device *dev, struct device_driver *drv){
	struct pci_device *pdev = to_pci_device(dev);
	struct pci_driver *pdrv = to_pci_driver(drv);

	if (!pdrv->id_table){
		return 0;
    }

	const struct pci_device_id *id = pci_match_id(pdev, pdrv->id_table);
	return (id != NULL);
}
