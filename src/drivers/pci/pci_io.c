#include <device/pci.h>
#include <io/ports.h>

// Mechanism #1
#define PCI_M1_ADDRESS_PORT 0xCF8
#define PCI_M1_DATA_PORT    0xCFC
#define PCI_M1_CONFIG_ADDRESS_ENABLE BIT(31)

static inline void pciIOCommon(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset){
	uint32_t address = (uint32_t)(
		((uint32_t)bus << 16) |
		((uint32_t)(device & 0x1F) << 11) |
		((uint32_t)(function & 0x07) << 8) |
		(offset & 0xFC) |
		((uint32_t)0x80000000)
	);
	
	outl(PCI_M1_ADDRESS_PORT, address);
}

static uint32_t pci_config_m1_read_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	pciIOCommon(bus, slot, func, offset);
	return inl(PCI_M1_DATA_PORT);
}

static uint16_t pci_config_m1_read_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	pciIOCommon(bus, slot, func, offset);
	return inw(PCI_M1_DATA_PORT);
}

static uint8_t pci_config_m1_read_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	pciIOCommon(bus, slot, func, offset);
	return inb(PCI_M1_DATA_PORT);
}	

static void pci_config_m1_write_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value){
	pciIOCommon(bus, slot, func, offset);
	outl(PCI_M1_DATA_PORT, value);
}

static void pci_config_m1_write_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value){
	pciIOCommon(bus, slot, func, offset);
	outw(PCI_M1_DATA_PORT, value);
}

static void pci_config_m1_write_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value){
	pciIOCommon(bus, slot, func, offset);
	outb(PCI_M1_DATA_PORT, value);
}

const struct pci_config_ops pci_config_ops_m1 = {
	.read8   = pci_config_m1_read_8,
	.read16  = pci_config_m1_read_16,
	.read32  = pci_config_m1_read_32,
	.write8  = pci_config_m1_write_8,
	.write16 = pci_config_m1_write_16,
	.write32 = pci_config_m1_write_32
};

// Mechanism #2

// ...

// ECAM

// ...

// Wrapper functions

uint8_t pci_read8(struct pci_device *dev, uint16_t offset){
	return dev->bus->config->read8(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset
	);
}

uint16_t pci_read16(struct pci_device *dev, uint16_t offset){
	return dev->bus->config->read16(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset
	);
}

uint32_t pci_read32(struct pci_device *dev, uint16_t offset){
	return dev->bus->config->read32(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset
	);
}

void pci_write8(struct pci_device *dev, uint16_t offset, uint8_t value){
	dev->bus->config->write8(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset,
		value
	);
}

void pci_write16(struct pci_device *dev, uint16_t offset, uint16_t value){
	dev->bus->config->write16(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset,
		value
	);
}

void pci_write32(struct pci_device *dev, uint16_t offset, uint32_t value){
	dev->bus->config->write32(
		dev->bus_num,
		PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn),
		offset,
		value
	);
}