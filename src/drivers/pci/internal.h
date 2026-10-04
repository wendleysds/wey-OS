#ifndef _PCI_INTERNAL_H
#define _PCI_INTERNAL_H

#include <device/pci.h>

extern const struct pci_config_ops *pci_config_ops;
extern struct bus_type pci_bus_type;

static inline void pci_set_config_ops(const struct pci_config_ops *ops){
	pci_config_ops = ops;
}

struct pci_bus *pci_alloc_bus(const struct pci_config_ops *ops);
struct pci_bus *pci_find_bus(uint8_t number);
void pci_add_bus(struct pci_bus *bus);

int pci_scan_bridge(struct pci_bus *parent, struct pci_device *dev);

static inline void pci_config_write8(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off, uint8_t val){
	pci_config_ops->write8(bus, dev, fn, off, val);
}

static inline void pci_config_write16(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off, uint16_t val){
	pci_config_ops->write16(bus, dev, fn, off, val);
}

static inline void pci_config_write32(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off, uint32_t val){
	pci_config_ops->write32(bus, dev, fn, off, val);
}

static inline uint8_t pci_config_read8(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off){
	return pci_config_ops->read8(bus, dev, fn, off);
}

static inline uint16_t pci_config_read16(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off){
	return pci_config_ops->read16(bus, dev, fn, off);
}

static inline uint32_t pci_config_read32(unsigned char bus, unsigned char dev, unsigned char fn, unsigned char off){
	return pci_config_ops->read32(bus, dev, fn, off);
}

const struct pci_device_id *pci_match_id(
	struct pci_device *dev,
	const struct pci_device_id *ids
);

int pci_driver_match(struct device *dev, struct device_driver *drv);

#endif