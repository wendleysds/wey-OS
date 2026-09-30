#include <arch/i386/ioapic.h>
#include <arch/i386/lapic.h>
#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <kernel/acpi/tables/madt.h>
#include <def/bits.h>
#include <def/errno.h>
#include <io/mmio.h>
#include <mm/iomem.h>
#include <lib/string.h>
#include <asm/page.h>

#define IOREGSEL           0x00
#define IOWIN              0x10

#define IOAPIC_ID          0x00
#define IOAPIC_VER         0x01
#define IOAPIC_ARB         0x02
#define IOAPIC_REDTBL(n)   (0x10 + 2 * (n))

#define IOAPIC_MASK_BIT    BIT(16)

enum irq_delivery_mode {
    IRQ_DELIVERY_MODE_FIXED  = 0,
    IRQ_DELIVERY_MODE_LOW    = 1,
    IRQ_DELIVERY_MODE_SMI    = 2,
    IRQ_DELIVERY_MODE_NMI    = 4,
    IRQ_DELIVERY_MODE_INIT   = 5,
    IRQ_DELIVERY_MODE_EXTINT = 7,
};

enum irq_trigger_mode {
    IRQ_TRIGGER_MODE_EDGE  = 0,
    IRQ_TRIGGER_MODE_LEVEL = 1,
};

static vaddr_t __iomem ioapic_base = 0;
static uint32_t ioapic_max_entries = 0;
static uint32_t ioapic_id = 0;
static uint32_t ioapic_version = 0;
static uint8_t default_target_apic_id = 0;

#define MAX_ISA_IRQS 16
static uint8_t isa_irq_to_gsi[MAX_ISA_IRQS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

uint32_t ioapic_read(uint32_t reg) {
    if (!ioapic_base) return 0;
    mmio_write32(ioapic_base + IOREGSEL, reg & 0xFF);
    return mmio_read32(ioapic_base + IOWIN);
}

void ioapic_write(uint32_t reg, uint32_t value) {
    if (!ioapic_base) return;
    mmio_write32(ioapic_base + IOREGSEL, reg & 0xFF);
    mmio_write32(ioapic_base + IOWIN, value);
}

/*
 * Redirection Table Entry (64-bit):
 * Bits 0-7:   Interrupt Vector (0x10 - 0xFE)
 * Bits 8-10:  Delivery Mode (0=Fixed, 1=Lowest Priority, 2=SMI, 4=NMI, 5=INIT, 7=ExtINT)
 * Bit 11:     Destination Mode (0=Physical, 1=Logical)
 * Bit 12:     Delivery Status (0=Idle, 1=Send Pending) [RO]
 * Bit 13:     Polarity (0=Active High, 1=Active Low)
 * Bit 14:     Remote IRR (for level-triggered interrupts: 0=EOI received, 1=Accepted) [RO]
 * Bit 15:     Trigger Mode (0=Edge sensitive, 1=Level sensitive)
 * Bit 16:     Interrupt Mask (0=Unmasked, 1=Masked)
 * Bits 17-55: Reserved
 * Bits 56-63: Destination APIC ID / processor bitmap
 */

struct ioapic_redtbl_entry {
    uint8_t vector;
    uint8_t destination_mode;
    uint8_t delivery_status;
    enum irq_delivery_mode delv_mode;
    enum irq_polarity polarity;
    uint8_t remote_irr;
    enum irq_trigger_mode trigger_mode;
    uint8_t mask;
    uint8_t dest;
} __packed;

static void ioapic_decode_entry(struct ioapic_redtbl_entry *entry, int idx) {
    uint32_t low = ioapic_read(IOAPIC_REDTBL(idx));
    uint32_t high = ioapic_read(IOAPIC_REDTBL(idx) + 1);
    uint64_t value = ((uint64_t)high << 32) | low;

    entry->vector = (uint8_t)(value & 0xFF);
    entry->delv_mode = (enum irq_delivery_mode)((value >> 8) & 0x7);
    entry->destination_mode = (uint8_t)((value >> 11) & 0x1);
    entry->delivery_status = (uint8_t)((value >> 12) & 0x1);
    entry->polarity = (enum irq_polarity)((value >> 13) & 0x1);
    entry->remote_irr = (uint8_t)((value >> 14) & 0x1);
    entry->trigger_mode = (enum irq_trigger_mode)((value >> 15) & 0x1);
    entry->mask = (uint8_t)((value >> 16) & 0x1);
    entry->dest = (uint8_t)((value >> 56) & 0xFF);
}

static void ioapic_encode_entry(const struct ioapic_redtbl_entry *entry, int idx) {
    uint64_t value = 0;

    value |= (uint64_t)(entry->vector & 0xFF);
    value |= (uint64_t)(entry->delv_mode & 0x7) << 8;
    value |= (uint64_t)(entry->destination_mode & 0x1) << 11;
    value |= (uint64_t)(entry->polarity & 0x1) << 13;
    value |= (uint64_t)(entry->trigger_mode & 0x1) << 15;
    value |= (uint64_t)(entry->mask & 0x1) << 16;
    value |= (uint64_t)(entry->dest & 0xFF) << 56;

    // Write upper 32-bit register (destination), then lower 32-bit register
    ioapic_write(IOAPIC_REDTBL(idx) + 1, (uint32_t)(value >> 32));
    ioapic_write(IOAPIC_REDTBL(idx), (uint32_t)(value & 0xFFFFFFFF));
}

void ioapic_set_routing(uint32_t pin, uint8_t vector, bool edge_triggered, bool no_poll) {
    if (pin >= ioapic_max_entries)
        return;

    struct ioapic_redtbl_entry entry;
    memset(&entry, 0, sizeof(entry));

    entry.vector = vector;
    entry.delv_mode = IRQ_DELIVERY_MODE_FIXED;
    entry.destination_mode = 0; // Physical mode
    entry.dest = default_target_apic_id;
    entry.polarity = no_poll ? IRQ_POLARITY_LOW : IRQ_POLARITY_HIGH;
    entry.trigger_mode = edge_triggered ? IRQ_TRIGGER_MODE_EDGE : IRQ_TRIGGER_MODE_LEVEL;
    entry.mask = 1; // Masked by default

    ioapic_encode_entry(&entry, pin);
}

void ioapic_mask(uint32_t pin) {
    if (pin >= ioapic_max_entries)
        return;

    uint32_t low = ioapic_read(IOAPIC_REDTBL(pin));
    low |= IOAPIC_MASK_BIT;
    ioapic_write(IOAPIC_REDTBL(pin), low);
}

void ioapic_unmask(uint32_t pin) {
    if (pin >= ioapic_max_entries)
        return;

    uint32_t low = ioapic_read(IOAPIC_REDTBL(pin));
    low &= ~IOAPIC_MASK_BIT;
    ioapic_write(IOAPIC_REDTBL(pin), low);
}

void ioapic_mask_all(void) {
    for (uint32_t i = 0; i < ioapic_max_entries; i++) {
        ioapic_mask(i);
    }
}

void ioapic_send_eoi(int vector) {
    lapic_eoi();
}

int ioapic_irq_to_pin(uint32_t irq) {
    if (irq < MAX_ISA_IRQS) {
        return isa_irq_to_gsi[irq];
    }
    return (int)irq;
}

void ioapic_mask_irq(uint32_t irq) {
    int pin = ioapic_irq_to_pin(irq);
    if (pin >= 0 && (uint32_t)pin < ioapic_max_entries) {
        ioapic_mask((uint32_t)pin);
    }
}

void ioapic_unmask_irq(uint32_t irq) {
    int pin = ioapic_irq_to_pin(irq);
    if (pin >= 0 && (uint32_t)pin < ioapic_max_entries) {
        ioapic_unmask((uint32_t)pin);
    }
}

int ioapic_init(uint32_t local_apic_id) {
    if (!acpi_madt_info || acpi_madt_info->ioapic_count <= 0) {
        return -ENODEV;
    }

	default_target_apic_id = (uint8_t)local_apic_id;

    paddr_t phys_addr = acpi_madt_info->ioapics[0].address;
    uint32_t gsi_base = acpi_madt_info->ioapics[0].gsi_base;

    ioapic_base = (vaddr_t)ioremap(phys_addr, PAGE_SIZE);
    if (!ioapic_base) {
        printk("IOAPIC: Failed to ioremap base address 0x%lx\n", (unsigned long)phys_addr);
        return -ENOMEM;
    }

    ioapic_id = (ioapic_read(IOAPIC_ID) >> 24) & 0x0F;
    uint32_t ver_reg = ioapic_read(IOAPIC_VER);
    ioapic_version = ver_reg & 0xFF;
    ioapic_max_entries = ((ver_reg >> 16) & 0xFF) + 1;

    printk("IOAPIC: ID %u, Version %u, Max Redirection Entries %u at 0x%lx (GSI base %u)\n",
           ioapic_id, ioapic_version, ioapic_max_entries, (unsigned long)phys_addr, gsi_base);

    // Initialize default ISA IRQ mappings (0-15)
    for (uint32_t i = 0; i < MAX_ISA_IRQS; i++) {
        isa_irq_to_gsi[i] = (uint8_t)i;
    }

    // Process ACPI MADT Interrupt Source Overrides
    if (acpi_madt_info && acpi_madt_info->override_count > 0) {
        for (size_t i = 0; i < acpi_madt_info->override_count; i++) {
            struct acpi_irq_override *ovr = &acpi_madt_info->overrides[i];
            if (ovr->source_irq < MAX_ISA_IRQS) {
                isa_irq_to_gsi[ovr->source_irq] = (uint8_t)ovr->gsi;
            }
        }
    }

    // Mask all entries initially
    ioapic_mask_all();

    // Configure redirection table entries for legacy ISA IRQs (0-15)
    for (uint8_t irq = 0; irq < MAX_ISA_IRQS; irq++) {
        uint32_t gsi = isa_irq_to_gsi[irq];
        if (gsi < gsi_base || (gsi - gsi_base) >= ioapic_max_entries)
            continue;

        uint32_t pin = gsi - gsi_base;
        enum irq_polarity polarity = IRQ_POLARITY_HIGH;
        enum irq_trigger_mode trigger = IRQ_TRIGGER_MODE_EDGE;

        // Check if this IRQ has specific flags in overrides
        if (acpi_madt_info) {
            for (size_t j = 0; j < acpi_madt_info->override_count; j++) {
                struct acpi_irq_override *ovr = &acpi_madt_info->overrides[j];
                if (ovr->source_irq == irq) {
                    if ((ovr->flags & MADT_INT_FLAGS_POLARITY_MASK) == MADT_INT_POLARITY_LOW) {
                        polarity = IRQ_POLARITY_LOW;
                    }
                    if ((ovr->flags & MADT_INT_FLAGS_TRIGGER_MASK) == MADT_INT_TRIGGER_LEVEL) {
                        trigger = IRQ_TRIGGER_MODE_LEVEL;
                    }
                    break;
                }
            }
        }

        struct ioapic_redtbl_entry entry;
        memset(&entry, 0, sizeof(entry));
        entry.vector = 0x20 + irq;
        entry.delv_mode = IRQ_DELIVERY_MODE_FIXED;
        entry.destination_mode = 0; // Physical mode
        entry.dest = default_target_apic_id;
        entry.polarity = polarity;
        entry.trigger_mode = trigger;
        entry.mask = 1; // Masked

        ioapic_encode_entry(&entry, pin);
    }

    irq_domain_create_legacy("ioapic", 16, 0x20, 0, &ioapic_chip, &ioapic_controller);

	return SUCCESS;
}

static void ioapic_chip_mask(struct irq_desc *desc) {
    if (!desc) return;
    ioapic_mask_irq(desc->irq);
}

static void ioapic_chip_unmask(struct irq_desc *desc) {
    if (!desc) return;
    ioapic_unmask_irq(desc->irq);
}

static int ioapic_chip_set_type(struct irq_desc *desc, enum irq_trigger_type type) {
    if (!desc) return -EINVAL;
    int pin = ioapic_irq_to_pin(desc->irq);
    if (pin < 0 || (uint32_t)pin >= ioapic_max_entries) return -EINVAL;

    struct ioapic_redtbl_entry entry;
    ioapic_decode_entry(&entry, pin);
    entry.trigger_mode = (type == IRQ_TYPE_LEVEL) ? IRQ_TRIGGER_MODE_LEVEL : IRQ_TRIGGER_MODE_EDGE;
    ioapic_encode_entry(&entry, pin);
    return 0;
}

static int ioapic_chip_set_affinity(struct irq_desc *desc, unsigned int cpu) {
    if (!desc) return -EINVAL;
    int pin = ioapic_irq_to_pin(desc->irq);
    if (pin < 0 || (uint32_t)pin >= ioapic_max_entries) return -EINVAL;

    struct ioapic_redtbl_entry entry;
    ioapic_decode_entry(&entry, pin);
    entry.destination_mode = 0; // Physical mode
    entry.dest = (uint8_t)cpu;
    ioapic_encode_entry(&entry, pin);
    return 0;
}

static void ioapic_ctrl_eoi(struct irq_desc *desc) {
    lapic_eoi();
}

const struct irq_chip ioapic_chip = {
    .name = "IOAPIC",
    .mask = ioapic_chip_mask,
    .unmask = ioapic_chip_unmask,
    .set_type = ioapic_chip_set_type,
    .set_affinity = ioapic_chip_set_affinity,
};

const struct irq_controller ioapic_controller = {
    .name = "IOAPIC",
    .eoi = ioapic_ctrl_eoi,
};