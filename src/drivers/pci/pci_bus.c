#include <kernel/device.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <device/pci.h>
#include <def/errno.h>
#include <def/bits.h>
#include <io/ports.h>
#include <mm/kheap.h>

//static uint8_t last_bus_id = 0;

#define PCI_BAR_IO_MASK      (~0x3u)
#define PCI_BAR_MEM_MASK     (~0xFULL)
#define PCI_BAR_TYPE_MASK    0x6

static void pci_parse_bar(struct pci_device* dev, struct pci_bar* bars, size_t count) {
	for (int i = 0; i < count; i++) {
		struct pci_bar *bar = &bars[i];
		uint32_t bar_offset = PCI_CONFIG_GENERAL_BAR0 + (i * sizeof(uint32_t));

		uint32_t low = pci_read32(dev, bar_offset);
		if (low == 0) {
			bar->type = PCI_BAR_UNUSED;
			continue;
		}

		bar->index = i;
		bar->prefetchable = !!(low & PCI_BAR_PREFETCHABLE);

		pci_write32(dev, bar_offset, 0xFFFFFFFF);
		uint32_t size_low = pci_read32(dev, bar_offset);
		uint64_t size_mask = 0;

		if (low & PCI_BAR_IO) {
			bar->type = PCI_BAR_IO;
			bar->base = low & PCI_BAR_IO_MASK;
			
			size_mask = size_low & PCI_BAR_IO_MASK;
			bar->size = (uint32_t)((~size_mask) + 1);

			pci_write32(dev, bar_offset, low);
			continue;
		}

		uint32_t mem_type = low & PCI_BAR_TYPE_MASK;

		if (mem_type == PCI_BAR_MEM64) {
			if (i + 1 >= count) {
				printk("PCI %04x:%04x: Invalid BAR64 configuration\n", 
					dev->vendor_id, dev->device_id);
				break;
			}

			uint32_t high_offset = PCI_CONFIG_GENERAL_BAR0 + ((i + 1) * sizeof(uint32_t));
			uint32_t high = pci_read32(dev, high_offset);

			pci_write32(dev, high_offset, 0xFFFFFFFF);
			uint32_t size_high = pci_read32(dev, high_offset);

			bar->type = PCI_BAR_MEM64;
			bar->base = ((uint64_t)high << 32) | (low & PCI_BAR_MEM_MASK);

			size_mask = ((uint64_t)size_high << 32) | (size_low & PCI_BAR_MEM_MASK);
			bar->size = (~size_mask) + 1;

			pci_write32(dev, high_offset, high);
			pci_write32(dev, bar_offset, low);

			bars[i + 1] = (struct pci_bar){
				.index = i + 1,
				.type = PCI_BAR_UNUSED,
				.base = 0,
				.size = 0
			};

			i++;
		} else {
			bar->type = PCI_BAR_MEM32;
			bar->base = low & PCI_BAR_MEM_MASK;

			size_mask = size_low & PCI_BAR_MEM_MASK;
			bar->size = (uint32_t)((~size_mask) + 1);

			pci_write32(dev, bar_offset, low);
		}
	}
}

static void pci_parse_header_general(struct pci_device* dev) {
	struct pci_header_general* header = &dev->header.general;

	pci_parse_bar(dev, header->bars, 6);

	header->cardbus_cis_pointer = pci_read32(dev, PCI_CONFIG_GENERAL_CIS_POINTER);

	// Dword 0 (Offset 0x2C): Subsystem ID [0:15], Subsystem Vendor ID [16:31]
	uint32_t dw0 = pci_read32(dev, 0x2C);
	header->subsystem_vendor_id = (uint16_t)(dw0 & 0xFFFF);
	header->subsystem_id	    = (uint16_t)(dw0 >> 16);
	
	header->expansion_rom_base_address = pci_read32(dev, PCI_CONFIG_GENERAL_EXPANSION_ROM);

	// Dword 1 (Offset 0x34): Reserved [31:8], Capabilities Pointer [7:0]
	uint32_t dw1 = pci_read32(dev, 0x34);
	header->reserved1 = (uint32_t)(dw1 & 0xFFFFFF00);
	header->capabilities_pointer = (uint8_t)(dw1 & 0xFF);

	header->reserved2 = pci_read32(dev, PCI_CONFIG_GENERAL_RESERVED_2);

	// Dword 2 (Offset 0x3C): Max Latency [31:24], Min Grant [23:16], Interrupt Pin [15:8], Interrupt Line [7:0]
	uint32_t dw2 = pci_read32(dev, 0x3C);
	header->max_latency = (uint8_t)((dw2 >> 24) & 0xFF);
	header->min_grant   = (uint8_t)((dw2 >> 16) & 0xFF);
	header->irq_pin     = (uint8_t)((dw2 >> 8) & 0xFF);
	header->irq_line    = (uint8_t)(dw2 & 0xFF);
}

static void pci_parse_header_bridge(struct pci_device* dev){
	struct pci_header_bridge* header = &dev->header.bridge;
	
	pci_parse_bar(dev, header->bars, 2);

	// Dword 0 (Offset 0x18): Secondary Latency Timer [31:24], Subordinate Bus Number [23:16], Secondary Bus Number [15:8], Primary Bus Number [7:0]
	uint32_t dw0 = pci_read32(dev, 0x18);
	header->secondary_latency_timer = (uint8_t)((dw0 >> 24) & 0xFF);
	header->subordinate_bus_number  = (uint8_t)((dw0 >> 16) & 0xFF);
	header->secondary_bus_number    = (uint8_t)((dw0 >> 8)  & 0xFF);
	header->primary_bus_number      = (uint8_t)(dw0 & 0xFF);

	// Dword 1 (Offset 0x1C): Secondary Status  [31:16], I/O Limit [15:8], I/O Base [7:0]
	uint32_t dw1 = pci_read32(dev, 0x1C);
	header->secondary_status = (uint16_t)((dw1 >> 16) & 0xFFFF);
	header->io_limit         = (uint8_t)((dw1 >> 8)   & 0xFF);
	header->io_base          = (uint8_t)(dw1 & 0xFF);

	// Dword 2 (Offset 0x20): Memory limit [31:16], Memory Base [15:0]
	uint32_t dw2 = pci_read32(dev, 0x20);
	header->memory_limit = (uint16_t)((dw2 >> 16) & 0xFFFF);
	header->memory_base  = (uint16_t)(dw2 & 0xFFFF);

	// Dword 3 (Offset 0x24): Prefetchable Memory Limit [31:16], Prefetchable Memory Base [15:0]
	uint32_t dw3 = pci_read32(dev, 0x24);
	header->prefetchable_memory_limit = (uint16_t)((dw3 >> 16) & 0xFFFF);
	header->prefetchable_memory_base  = (uint16_t)(dw3 & 0xFFFF);

	header->prefetchable_base_upper_32_bits = pci_read32(dev, PCI_CONFIG_BRIDGE_PREFETCH_BASE_UPPER);
	header->prefetchable_limit_upper_32_bits = pci_read32(dev, PCI_CONFIG_BRIDGE_PREFETCH_LIMIT_UPPER);

	// Dword 4 (Offset 0x30): I/O Limit [31:16], I/O Base [15:0]
	uint32_t dw4 = pci_read32(dev, 0x30);
	header->io_limit_upper_16_bits = (uint16_t)(dw4 & 0xFFFF);
	header->io_base_upper_16_bits = (uint16_t)((dw4 >> 16) & 0xFFFF);

	// Dword 5 (Offset 0x34): Reserved [31:8], Capabilities Pointer [7:0]
	uint32_t dw5 = pci_read32(dev, 0x34);
	header->reserved1 = (uint32_t)(dw5 & 0xFFFFFF00);
	header->capabilities_pointer = (uint8_t)(dw5 & 0xFF);

	header->expansion_rom_base_address = pci_read32(dev, PCI_CONFIG_BRIDGE_EXPANSION_ROM);

	// Dword 6 (Offset 0x3C): Bridge Control [31:16] Interrupt Pin [15:8] Interrupt Line [7:0]
	uint32_t dw6 = pci_read32(dev, 0x3C);
	header->bridge_control = (uint16_t)((dw6 >> 16) & 0xFFFF);
	header->irq_pin = (uint8_t)((dw6 >> 8) & 0xFF);
	header->irq_line = (uint8_t)(dw6 & 0xFF);
}

static void pci_parse_header_cardbus(struct pci_device* dev){
	struct pci_header_cardbus *header = &dev->header.cardbus;

	header->socket_exca_base_address = pci_read32(dev, PCI_CONFIG_CARDBUS_SOCKET_EXCA_BASE_ADDRESS);
	
	// Dword 1 (Offset 0x14): Secondary Status [31:16], Reserved [15:8], Offset of capabilities list [7:0]
	uint32_t dw1 = pci_read32(dev, 0x14);
	header->secondary_status = (uint16_t)((dw1 >> 16) & 0xFFFF);
	header->reserved1 = (uint16_t)((dw1 >> 8) & 0xFF);
	header->offset_of_capabilities_list = (uint8_t)(dw1 & 0xFF);

	// Dword 2 (Offset 0x18): Latency Timer [31:24], Subordinate Bus Number [23:16], Bus Number [15:8], PCI Bus Number [7:0]
	uint32_t dw2 = pci_read32(dev, 0x18);
	header->cardbus_latency_timer = (uint8_t)(dw2 & 0xFF);
	header->subordinate_bus_number = (uint8_t)((dw2 >> 8) & 0xFF);
	header->cardbus_bus_number = (uint8_t)((dw2 >> 16) & 0xFF);
	header->pci_bus_number = (uint8_t)((dw2 >> 24) & 0xFF);

	header->memory[0].base_address = pci_read32(dev, PCI_CONFIG_CARDBUS_MEMORY_BASE0);
	header->memory[0].limit = pci_read32(dev, PCI_CONFIG_CARDBUS_MEMORY_LIMIT0);

	header->memory[1].base_address = pci_read32(dev, PCI_CONFIG_CARDBUS_MEMORY_BASE1);
	header->memory[1].limit = pci_read32(dev, PCI_CONFIG_CARDBUS_MEMORY_LIMIT1);

	header->io[0].base_address = pci_read32(dev, PCI_CONFIG_CARDBUS_IO_BASE_ADDRESS0);
	header->io[0].limit = pci_read32(dev, PCI_CONFIG_CARDBUS_IO_LIMIT0);

	header->io[1].base_address = pci_read32(dev, PCI_CONFIG_CARDBUS_IO_BASE_ADDRESS1);
	header->io[1].limit = pci_read32(dev, PCI_CONFIG_CARDBUS_IO_LIMIT1);

	// Dword 3 (Offset 0x3C): Bridge Control [31:16], Interrupt Pin [15:8] Interrupt Line [7:0]
	uint32_t dw3 = pci_read32(dev, 0x3C);
	header->bridge_control = (uint16_t)((dw3 >> 16) & 0xFFFF);
	header->irq_pin = (uint8_t)((dw3 >> 8) & 0xFF);
	header->irq_line = (uint8_t)(dw3 & 0xFF);

	// Dword 4 (Offset 0x40): Subsystem Vendor ID [31:16] Subsystem Device ID [15:0], 
	uint32_t dw4 = pci_read32(dev, 0x40);
	header->subsystem_vendor_id = (uint16_t)((dw4 >> 16) & 0xFFFF);
	header->subsystem_device_id = (uint16_t)(dw4 & 0xFFFF);

	header->pc_card_legacy_mode_base_address_16bit = pci_read32(dev, PCI_CONFIG_CARDBUS_PC_CARD_LEGACY_MODE_BASE_ADDRESS);
}

static int pci_parse_capabilities(struct pci_device* dev){
	if(!BIT_CHECK(dev->status, PCI_STATUS_CAP_LIST)){
		return 0;
	}

	uint8_t capabilities_ptr;

	switch (dev->header_type) {
		case PCI_HEADER_TYPE_NORMAL:
			capabilities_ptr = dev->header.general.capabilities_pointer;
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			capabilities_ptr = dev->header.bridge.capabilities_pointer;
			break;
		case PCI_HEADER_TYPE_CARDBUS:
			return 0; // Not supported
		default:
			return -EINVAL;
	}

	capabilities_ptr &= ~3;

	while(capabilities_ptr && capabilities_ptr != 0xFF){
		uint8_t cap_id = pci_read8(dev, capabilities_ptr);
		uint8_t next_cap = pci_read8(dev, capabilities_ptr + 1);

		struct pci_capability* cap = (struct pci_capability*)kmalloc(sizeof(struct pci_capability));
		if(!cap){
			struct pci_capability* cap_next;
			list_for_each_entry_safe(cap, cap_next, &dev->capabilities, node){
				list_remove(&cap->node);
				kfree(cap);
			}

			return -ENOMEM;
		}

		INIT_LIST_HEAD(&cap->node);

		cap->id = cap_id;
		list_add(&cap->node, &dev->capabilities);

		capabilities_ptr = next_cap & ~3;
	}

	return SUCCESS;
}

static int pci_parse_device(struct pci_device* dev, uint8_t bus, uint8_t slot, uint8_t func) {
	const struct pci_config_ops* ops = dev->bus->config;

	// Dword 0 (Offset 0x00): Vendor ID [0:15], Device ID [16:31]
	uint32_t dw0 = ops->read32(bus, slot, func, 0x00);
	dev->vendor_id = (uint16_t)(dw0 & 0xFFFF);

	// Check if device exist
	if (dev->vendor_id == 0xFFFF || dev->vendor_id == 0x0000) return 1;

	dev->device_id = (uint16_t)(dw0 >> 16);
	if (dev->device_id == 0xFFFF || dev->device_id == 0x0000) return 1;

	dev->bus_num = bus;
	dev->devfn = PCI_DEVFN(slot, func);

	// Dword 1 (Offset 0x04): Command [0:15], Status [16:31]
	uint32_t dw1 = ops->read32(bus, slot, func, 0x04);
	dev->command = (uint16_t)(dw1 & 0xFFFF);
	dev->status  = (uint16_t)(dw1 >> 16);

	// Dword 2 (Offset 0x08): Revision [0:7], Prog IF [8:15], Subclass [16:23], Class [24:31]
	uint32_t dw2 = ops->read32(bus, slot, func, 0x08);
	dev->revision	= (uint8_t)(dw2 & 0xFF);
	dev->prog_if	= (uint8_t)((dw2 >> 8) & 0xFF);
	dev->subclass	= (uint8_t)((dw2 >> 16) & 0xFF);
	dev->class_code = (uint8_t)((dw2 >> 24) & 0xFF);

	// Dword 3 (Offset 0x0C): Cache Line Size [0:7], Latency Timer [8:15], Header Type [16:23], BIST [24:31]
	uint32_t dw3 = ops->read32(bus, slot, func, 0x0C);
	dev->cache_line_size = (uint8_t)(dw3 & 0xFF);
	dev->latency_timer   = (uint8_t)((dw3 >> 8) & 0xFF);
	dev->header_type     = (uint8_t)((dw3 >> 16) & 0xFF);
	dev->BIST	     = (uint8_t)((dw3 >> 24) & 0xFF);

	int header_type = dev->header_type & PCI_HEADER_TYPE_MASK;

	switch (header_type) {
		case PCI_HEADER_TYPE_NORMAL:
			pci_parse_header_general(dev);
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			pci_parse_header_bridge(dev);
			break;
		case PCI_HEADER_TYPE_CARDBUS:
			pci_parse_header_cardbus(dev);
			break;
		default: 
			printk("PCI: %d:%d.%d: unknown header type %d\n",
				dev->bus_num, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
				dev->header_type
			);

			return 1;
	}

	int res = pci_parse_capabilities(dev);
	if(res){
		printk("PCI: %d:%d.%d: failed to parse capabilities\n",
			dev->bus_num, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
		return res;
	}

	return 0;
}

static struct pci_device* alloc_pci_device(struct pci_bus *bus) {
	struct pci_device* dev = kzalloc(sizeof(struct pci_device));
	if (dev) {
		spinlock_init(&dev->lock);
		INIT_LIST_HEAD(&dev->node);
		INIT_LIST_HEAD(&dev->capabilities);
		dev->bus = bus;
	}
	return dev;
}

int pci_scan_bus(struct pci_bus *bus) {
	if (!bus) return -EINVAL;

	struct pci_device* device = alloc_pci_device(bus);

	int parsed = 0;

	for (uint8_t slot = 0; slot < 32; slot++) {
		if(!device) return -ENOMEM;

		if (pci_parse_device(device, bus->number, slot, 0) != 0) {
			continue;
		} parsed++;

		list_add_tail(&device->node, &bus->devices);

		uint8_t tmp = device->header_type;
		device = alloc_pci_device(bus);

		if (!BIT_CHECK(tmp, 7)) {
			continue;
		}

		for (uint8_t func = 1; func < 8; func++) {
			if(!device) return -ENOMEM;
			
			if (pci_parse_device(device, bus->number, slot, func) != 0) {
				continue;
			} parsed++;

			list_add_tail(&device->node, &bus->devices);

			device = alloc_pci_device(bus);
		}

		continue;
	}

	if (device) {
			kfree(device);
		}

	return parsed;
}

