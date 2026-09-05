#ifndef _PCI_H
#define _PCI_H

#include <sync/spinlock.h>
#include <lib/list.h>

#include <stdint.h>
#include <stdbool.h>


/*
* PCI configuration space
*/

#define PCI_CONFIG_VENDOR_ID        0x00
#define PCI_CONFIG_DEVICE_ID        0x02
#define PCI_CONFIG_COMMAND          0x04
#define PCI_CONFIG_STATUS           0x06
#define PCI_CONFIG_REVISION         0x08
#define PCI_CONFIG_PROG_IF          0x09
#define PCI_CONFIG_SUBCLASS         0x0A
#define PCI_CONFIG_CLASS            0x0B
#define PCI_CONFIG_CACHE_LINE       0x0C
#define PCI_CONFIG_LATENCY_TIMER    0x0D
#define PCI_CONFIG_HEADER_TYPE      0x0E
#define PCI_CONFIG_BIST             0x0F

// General header type offsets

#define PCI_CONFIG_GENERAL_BAR0                 0x10
#define PCI_CONFIG_GENERAL_BAR1                 0x14
#define PCI_CONFIG_GENERAL_BAR2                 0x18
#define PCI_CONFIG_GENERAL_BAR3                 0x1C
#define PCI_CONFIG_GENERAL_BAR4                 0x20
#define PCI_CONFIG_GENERAL_BAR5                 0x24
#define PCI_CONFIG_GENERAL_CIS_POINTER          0x28
#define PCI_CONFIG_GENERAL_SUBSYSTEM_VENDOR_ID  0x2C
#define PCI_CONFIG_GENERAL_SUBSYSTEM_ID         0x2E
#define PCI_CONFIG_GENERAL_EXPANSION_ROM        0x30
#define PCI_CONFIG_GENERAL_RESERVED_1           0x34
#define PCI_CONFIG_GENERAL_CAP_PTR              0x37
#define PCI_CONFIG_GENERAL_RESERVED_2           0x38
#define PCI_CONFIG_GENERAL_INTERRUPT_LINE       0x3C
#define PCI_CONFIG_GENERAL_INTERRUPT_PIN        0x3D
#define PCI_CONFIG_GENERAL_MIN_GRANT            0x3E
#define PCI_CONFIG_GENERAL_MAX_LATENCY          0x3F

// Bridge header type offsets

#define PCI_CONFIG_BRIDGE_BAR0                  0x10
#define PCI_CONFIG_BRIDGE_BAR1                  0x14
#define PCI_CONFIG_BRIDGE_SECONDARY_LATENCY     0x18
#define PCI_CONFIG_BRIDGE_SUBORDINATE_BUS       0x19
#define PCI_CONFIG_BRIDGE_SECONDARY_BUS         0x1A
#define PCI_CONFIG_BRIDGE_PRIMARY_BUS           0x1B
#define PCI_CONFIG_BRIDGE_SECONDARY_STATUS      0x1C
#define PCI_CONFIG_BRIDGE_IO_LIMIT              0x1D
#define PCI_CONFIG_BRIDGE_IO_BASE               0x1E
#define PCI_CONFIG_BRIDGE_MEMORY_LIMIT          0x20
#define PCI_CONFIG_BRIDGE_MEMORY_BASE           0x22
#define PCI_CONFIG_BRIDGE_PREFETCH_MEMORY_LIMIT 0x24
#define PCI_CONFIG_BRIDGE_PREFETCH_MEMORY_BASE  0x26
#define PCI_CONFIG_BRIDGE_PREFETCH_BASE_UPPER   0x28
#define PCI_CONFIG_BRIDGE_PREFETCH_LIMIT_UPPER  0x2C
#define PCI_CONFIG_BRIDGE_IO_LIMIT_UPPER        0x30
#define PCI_CONFIG_BRIDGE_IO_BASE_UPPER         0x32
#define PCI_CONFIG_BRIDGE_CAP_PTR               0x34
#define PCI_CONFIG_BRIDGE_EXPANSION_ROM         0x38
#define PCI_CONFIG_BRIDGE_BRIDGE_CONTROL        0x3E
#define PCI_CONFIG_BRIDGE_INTERRUPT_LINE        0x3F
#define PCI_CONFIG_BRIDGE_INTERRUPT_PIN         0x3D

// Cardbus header type offsets

#define PCI_CONFIG_CARDBUS_SOCKET_EXCA_BASE_ADDRESS         0x10
#define PCI_CONFIG_CARDBUS_SECONDARY_STATUS                 0x14
#define PCI_CONFIG_CARDBUS_RESERVED                         0x16
#define PCI_CONFIG_CARDBUS_OFFSET_CAP_LIST                  0x17
#define PCI_CONFIG_CARDBUS_LATENCY_TIMER                    0x18
#define PCI_CONFIG_CARDBUS_SUBORDINATE_BUS_NUMBER           0x19
#define PCI_CONFIG_CARDBUS_BUS_NUMBER                       0x1A
#define PCI_CONFIG_CARDBUS_PCI_BUS_NUMBER                   0x1B
#define PCI_CONFIG_CARDBUS_MEMORY_BASE0                     0x1C
#define PCI_CONFIG_CARDBUS_MEMORY_LIMIT0                    0x20
#define PCI_CONFIG_CARDBUS_MEMORY_BASE1                     0x24
#define PCI_CONFIG_CARDBUS_MEMORY_LIMIT1                    0x28
#define PCI_CONFIG_CARDBUS_IO_BASE_ADDRESS0                 0x2C
#define PCI_CONFIG_CARDBUS_IO_LIMIT0                        0x30
#define PCI_CONFIG_CARDBUS_IO_BASE_ADDRESS1                 0x34
#define PCI_CONFIG_CARDBUS_IO_LIMIT1                        0x38
#define PCI_CONFIG_CARDBUS_BRIDGE_CONTROL                   0x3C
#define PCI_CONFIG_CARDBUS_INTERRUPT_LINE                   0x3E
#define PCI_CONFIG_CARDBUS_INTERRUPT_PIN                    0x3F
#define PCI_CONFIG_CARDBUS_SUBSISTEM_VENDOR_ID              0x40
#define PCI_CONFIG_CARDBUS_SUBSISTEM_ID                     0x42
#define PCI_CONFIG_CARDBUS_PC_CARD_LEGACY_MODE_BASE_ADDRESS 0x44

/*
* PCI command register
*/

#define PCI_COMMAND_IO              BIT(0)
#define PCI_COMMAND_MEMORY          BIT(1)
#define PCI_COMMAND_BUS_MASTER      BIT(2)
#define PCI_COMMAND_SPECIAL         BIT(3)
#define PCI_COMMAND_MEM_WRITE_INV   BIT(4)
#define PCI_COMMAND_VGA_PALETTE     BIT(5)
#define PCI_COMMAND_PARITY          BIT(6)
#define PCI_COMMAND_WAIT            BIT(7)
#define PCI_COMMAND_SERR            BIT(8)
#define PCI_COMMAND_FAST_BACK       BIT(9)
#define PCI_COMMAND_INTX_DISABLE    BIT(10)


/*
* PCI status register
*/

#define PCI_STATUS_INTERRUPT         BIT(3)
#define PCI_STATUS_CAP_LIST          BIT(4)
#define PCI_STATUS_66MHZ             BIT(5)
#define PCI_STATUS_FAST_BACK         BIT(7)
#define PCI_STATUS_PARITY            BIT(8)
#define PCI_STATUS_DEVSEL_MASK       (3 << 9)
#define PCI_STATUS_SIG_TARGET_ABORT  BIT(11)
#define PCI_STATUS_REC_TARGET_ABORT  BIT(12)
#define PCI_STATUS_REC_MASTER_ABORT  BIT(13)
#define PCI_STATUS_SIG_SYSTEM_ERROR  BIT(14)
#define PCI_STATUS_DETECTED_PARITY   BIT(15)


/*
* PCI header types
*/

#define PCI_HEADER_TYPE_NORMAL       0x00
#define PCI_HEADER_TYPE_BRIDGE       0x01
#define PCI_HEADER_TYPE_CARDBUS      0x02

#define PCI_HEADER_TYPE_MASK         0x7F
#define PCI_HEADER_MULTIFUNCTION     0x80

/*
* PCI Classes
*/		

#define PCI_CLASS_CODE_UNCLASSIFIED                   0x00
#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER        0x01
#define PCI_CLASS_CODE_NETWORK_CONTROLLER             0x02
#define PCI_CLASS_CODE_DISPLAY_CONTROLLER             0x03
#define PCI_CLASS_CODE_MULTIMEDIA_CONTROLLER          0x04
#define PCI_CLASS_CODE_MEMORY_CONTROLLER              0x05
#define PCI_CLASS_CODE_BRIDGE_CONTROLLER              0x06
#define PCI_CLASS_CODE_SIMPLE_COMM_CONTROLLER         0x07
#define PCI_CLASS_CODE_BASE_SYSTEM_PERIPHERAL         0x08
#define PCI_CLASS_CODE_INPUT_DEVICE_CONTROLLER        0x09
#define PCI_CLASS_CODE_DOCKING_SYSTEM                 0x0A
#define PCI_CLASS_CODE_PROCESSOR                      0x0B
#define PCI_CLASS_CODE_SERIAL_BUS_CONTROLLER          0x0C
#define PCI_CLASS_CODE_WIRELESS_CONTROLLER            0x0D
#define PCI_CLASS_CODE_INTELLIGENT_CONTROLLER         0x0E
#define PCI_CLASS_CODE_SATELLITE_COMM_CONTROLLER      0x0F
#define PCI_CLASS_CODE_ENCRYPTION_CONTROLLER          0x10
#define PCI_CLASS_CODE_SIGNAL_PROCESSING_CONTROLLER   0x11
#define PCI_CLASS_CODE_PROCESSING_ACCELERATOR         0x12
#define PCI_CLASS_CODE_NON_ESSENTIAL_INSTRUMENTATION  0x13
#define PCI_CLASS_CODE_COPROCESSOR                    0x40
#define PCI_CLASS_CODE_UNASSIGNED                     0xFF


/*
 * Matching
 */

#define PCI_ANY_ID       0xFFFF
#define PCI_ANY_CLASS    0xFF

/*
* PCI BAR
*/

#define PCI_BAR_IO_T                 0x01
#define PCI_BAR_MEM_T                0x00

#define PCI_BAR_MEM_TYPE_MASK        0x06
#define PCI_BAR_MEM_TYPE_32          0x00
#define PCI_BAR_MEM_TYPE_64          0x04

#define PCI_BAR_PREFETCHABLE         0x08


enum pci_bar_type {
    PCI_BAR_UNUSED = 0,
    PCI_BAR_IO,
    PCI_BAR_MEM32,
    PCI_BAR_MEM64,
};


struct pci_bar {
    enum pci_bar_type type;

    uint8_t index;

    uint64_t base;
    uint64_t size;

    bool prefetchable;
};


/*
* PCI configuration access
*/

struct pci_config_ops {
    uint8_t  (*read8) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint16_t (*read16) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t (*read32) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void (*write8) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);
    void (*write16) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void (*write32) (uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
};


/*
* PCI capability
*/

struct pci_capability {
    uint8_t id;
    uint8_t offset;

    struct list_head node;
};


/*
* PCI device identification
*/

struct pci_device_id {
    uint16_t vendor;
    uint16_t device;

    uint8_t class;
    uint8_t subclass;
    uint8_t prog_if;
};


/*
* PCI bus
*/

struct pci_bus {
    uint8_t number;

    struct pci_bus *parent;

    const struct pci_config_ops *config;

    struct list_head devices;

    struct list_head node;

    void *sysdata;
};


struct pci_header_general {
    struct pci_bar bars[6];
    uint32_t cardbus_cis_pointer;

    uint16_t subsystem_id;
    uint16_t subsystem_vendor_id;

    uint32_t expansion_rom_base_address;

    uint32_t reserved1;
    uint8_t capabilities_pointer;
    uint32_t reserved2;

    uint8_t max_latency;
    uint8_t min_grant;
    uint8_t irq_pin;
    uint8_t irq_line;
};

struct pci_header_bridge {
    struct pci_bar bars[2];
    uint8_t secondary_latency_timer;
    uint8_t subordinate_bus_number;
    uint8_t secondary_bus_number;
    uint8_t primary_bus_number;

    uint16_t secondary_status;
    uint8_t io_limit;
    uint8_t io_base;

    uint16_t memory_limit;
    uint16_t memory_base;

    uint16_t prefetchable_memory_limit;
    uint16_t prefetchable_memory_base;

    uint32_t prefetchable_base_upper_32_bits;
    uint32_t prefetchable_limit_upper_32_bits;

    uint16_t io_limit_upper_16_bits;
    uint16_t io_base_upper_16_bits;

    uint32_t reserved1;
    uint8_t capabilities_pointer;

    uint32_t expansion_rom_base_address;

    uint16_t bridge_control;
    uint8_t irq_pin;
    uint8_t irq_line;
};

struct pci_header_cardbus {
    uint32_t socket_exca_base_address;

    uint16_t secondary_status;
    uint8_t reserved1;
    uint8_t offset_of_capabilities_list;

    uint8_t cardbus_latency_timer;
    uint8_t subordinate_bus_number;
    uint8_t cardbus_bus_number;
    uint8_t pci_bus_number;

    struct {
        uint32_t base_address;
        uint32_t limit;
    } memory[2];

    struct {
        uint32_t base_address;
        uint32_t limit;
    } io[2];

    uint16_t bridge_control;
    uint8_t irq_pin;
    uint8_t irq_line;

    uint16_t subsystem_device_id;
    uint16_t subsystem_vendor_id;

    uint32_t pc_card_legacy_mode_base_address_16bit;
};

/*
* PCI device
*
* One pci_device represents one PCI function.
*
* Example:
*
*     00:03.0
*     ^  ^  ^
*     |  |  function
*     |  device
*     bus
*/

struct pci_device {
    struct pci_bus *bus;
	spinlock_t lock;

    uint8_t bus_num;
    uint8_t devfn;

    uint16_t device_id;
    uint16_t vendor_id;

    uint16_t status;
    uint16_t command;

    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;

    // 7   -> capable
    // 6   -> start
    // 4,5 -> reserved
    // 0,3 -> completion Code
    uint8_t BIST;

    uint8_t latency_timer;
    uint8_t cache_line_size;
    
    uint8_t header_type;
    union {
        struct pci_header_general general;
        struct pci_header_bridge bridge;
        struct pci_header_cardbus cardbus;
    } header;

    struct list_head capabilities;

    struct pci_driver *driver;

    void *driver_data;

    struct list_head node;
};

/*
* PCI driver
*/

struct pci_driver {
    const char *name;

    const struct pci_device_id *id_table;

    int (*probe)(
        struct pci_device *dev,
        const struct pci_device_id *id
    );

    void (*remove)(
        struct pci_device *dev
    );

    struct list_head node;
};



/*
* PCI address helpers
*/

#define PCI_DEVFN(device, function) \
    (((device) << 3) | ((function) & 0x07))

#define PCI_SLOT(devfn) \
    (((devfn) >> 3) & 0x1F)

#define PCI_FUNC(devfn) \
    ((devfn) & 0x07)


/*
* Configuration space access
*/

uint8_t pci_read8(struct pci_device *dev, uint16_t offset);
uint16_t pci_read16(struct pci_device *dev, uint16_t offset);
uint32_t pci_read32(struct pci_device *dev, uint16_t offset);

void pci_write8(struct pci_device *dev, uint16_t offset, uint8_t value);
void pci_write16(struct pci_device *dev, uint16_t offset, uint16_t value);
void pci_write32(struct pci_device *dev, uint16_t offset, uint32_t value);


/*
* Device management
*/

int pci_register_driver(struct pci_driver *driver);
void pci_unregister_driver(struct pci_driver *driver);

int pci_enable_device(struct pci_device *dev);
void pci_disable_device(struct pci_device *dev);

int pci_enable_bus_master(struct pci_device *dev);
void pci_disable_bus_master(struct pci_device *dev);


/*
* BAR helpers
*/

struct pci_bar *pci_get_bar(struct pci_device *dev, unsigned int index);


/*
* Capability helpers
*/

struct pci_capability *pci_find_capability(struct pci_device *dev, uint8_t id);


/*
* PCI enumeration
*/

int pci_scan_bus(struct pci_bus *bus);

#endif
