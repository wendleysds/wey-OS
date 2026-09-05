#include <kernel/printk.h>
#include <device/pci.h>
#include <def/errno.h>
#include <def/bits.h>

static inline void pci_command_set_bits(struct pci_device *dev, uint16_t bits) {
	if (!(dev->command & bits)) {
		dev->command |= bits;
		pci_write16(dev, PCI_CONFIG_COMMAND, dev->command);
	}
}

static inline void pci_command_clear_bits(struct pci_device *dev, uint16_t bits) {
	if (dev->command & bits) {
		dev->command &= ~bits;
		pci_write16(dev, PCI_CONFIG_COMMAND, dev->command);
	}
}

int pci_enable_device(struct pci_device *dev){
	if (!dev) return -EINVAL;

	unsigned long irqflag;
	spin_lock_irqsave(&dev->lock, &irqflag);
	pci_command_set_bits(dev, PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
	spin_unlock_irqrestore(&dev->lock, &irqflag);
	
	return 0;
}

void pci_disable_device(struct pci_device *dev){
	if (!dev) return;

	unsigned long irqflag;
	spin_lock_irqsave(&dev->lock, &irqflag);
	pci_command_clear_bits(dev, PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
	spin_unlock_irqrestore(&dev->lock, &irqflag);
}

int pci_enable_bus_master(struct pci_device *dev){
	if (!dev) return -EINVAL;

	unsigned long irqflag;
	spin_lock_irqsave(&dev->lock, &irqflag);
	pci_command_set_bits(dev, PCI_COMMAND_BUS_MASTER);
	spin_unlock_irqrestore(&dev->lock, &irqflag);

	return 0;
}

void pci_disable_bus_master(struct pci_device *dev){
	if (!dev) return;

	unsigned long irqflag;
	spin_lock_irqsave(&dev->lock, &irqflag);
	pci_command_clear_bits(dev, PCI_COMMAND_BUS_MASTER);
	spin_unlock_irqrestore(&dev->lock, &irqflag);
}
