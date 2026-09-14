#include <kernel/printk.h>
#include <kernel/acpi.h>

#include <lib/assert.h>
#include <lib/string.h>
#include <def/errno.h>

#include <asm/page.h>

#include "../internal.h"

struct acpi_cpu {
	uint32_t acpi_id;
	uint32_t apid_id;
	uint32_t flags;
};

struct acpi_ioapic {
	uint8_t id;
	vaddr_t address;
	uint32_t gsi_base;
};

struct acpi_irq_override {
	uint8_t bus;
	uint8_t source_irq;
	uint16_t flags;
	uint32_t gsi;
};

struct acpi_nmi_source {
	uint8_t acpi_id;
	uint8_t lint;
	uint16_t flags;
};

struct acpi_madt_info {
	vaddr_t local_apic_address;
	uint32_t flags;

	struct acpi_cpu *cpus;
	size_t cpu_count;

	struct acpi_ioapic *ioapics;
	size_t ioapic_count;
	
	struct acpi_irq_override *overrides;
	size_t override_count;

	struct acpi_nmi_sources *nmis;
	size_t nmi_count;
};

static inline void acpi_parse_madt_lapic(struct acpi_madt_lapic *entry) {
	if (!(entry->flags & MADT_LAPIC_FLAG_ENABLED)) return;

	printk("ACPI: Found CPU with APIC ID %u, Processor ID %u, Flags: 0x%x\n",
			entry->apic_id, entry->processor_id, entry->flags);
}

static inline void acpi_parse_madt_ioapic(struct acpi_madt_ioapic *entry) {
	printk("ACPI: Found I/O APIC with GSI base %u at %p\n",
			entry->gsi_base, (void *)entry->ioapic_address);
}

static inline void acpi_parse_madt_int_override(struct acpi_madt_int_override *entry) {
	printk("ACPI: Interrupt override for IRQ %u to GSI %u (bus %u)\n",
			entry->source, entry->gsi, entry->bus);
}

static inline void acpi_parse_madt_local_nmi(struct acpi_madt_local_nmi *entry) {
	printk("ACPI: Local APIC NMI for processor %u on LINT %u\n",
			entry->processor_id, entry->local_apic_lint);
}

static inline void acpi_parse_madt_local_x2apic(struct acpi_madt_local_x2apic *entry) {
	printk("ACPI: Found CPU with x2APIC ID %u\n", entry->x2apic_id);
}

void acpi_parse_madt(struct acpi_madt *table){
	struct acpi_madt_entry *entry = table->entries;

	printk("ACPI: MADT parsed. Local APIC address: 0x%x, flags: 0x%x\n",
			table->local_apic_address, table->flags);

	while ((uintptr_t)entry < (uintptr_t)table + table->header.length) {
		switch (entry->type) {
			case MADT_TYPE_LOCAL_APIC:
				if (entry->length < sizeof(struct acpi_madt_lapic)) break;
				acpi_parse_madt_lapic((struct acpi_madt_lapic *)entry);
				break;

			case MADT_TYPE_IO_APIC:
				if (entry->length < sizeof(struct acpi_madt_ioapic)) break;
				acpi_parse_madt_ioapic((struct acpi_madt_ioapic *)entry);
				break;

			case MADT_TYPE_INTERRUPT_OVERRIDE:
				if (entry->length < sizeof(struct acpi_madt_int_override)) break;
				acpi_parse_madt_int_override((struct acpi_madt_int_override *)entry);
				break;

			case MADT_TYPE_NMI_SOURCE:
				if (entry->length < sizeof(struct acpi_madt_nmi_source)) break;
				break;

			case MADT_TYPE_LOCAL_NMI:
				if (entry->length < sizeof(struct acpi_madt_local_nmi)) break;
				acpi_parse_madt_local_nmi((struct acpi_madt_local_nmi *)entry);
				break;

			case MADT_TYPE_LOCAL_APIC_OVERRIDE:
				break;

			case MADT_TYPE_LOCAL_X2APIC:
				if (entry->length < sizeof(struct acpi_madt_local_x2apic)) break;
				acpi_parse_madt_local_x2apic((struct acpi_madt_local_x2apic *)entry);
				break;

			default:
				printk("ACPI: Unknown MADT entry type %u (length %u)\n",
							entry->type, entry->length);
				break;
		}

		const size_t next_len = (entry->length == 0) ? 2 : entry->length;
		entry = (struct acpi_madt_entry *)((uintptr_t)entry + next_len);
	}
}
