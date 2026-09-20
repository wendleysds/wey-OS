#include <kernel/printk.h>
#include <kernel/init.h>
#include <kernel/acpi.h>
#include <mm/kheap.h>

#include <lib/assert.h>
#include <lib/string.h>
#include <def/errno.h>

#include <asm/page.h>

#include "../internal.h"

struct acpi_madt_info *acpi_madt_info = NULL;

static __init void acpi_parse_madt_lapic(struct acpi_madt_lapic *entry, struct acpi_cpu *cpu) {
	cpu->acpi_id = entry->processor_id;
	cpu->apic_id = entry->apic_id;
	cpu->flags = entry->flags;
	cpu->is_x2apic = false;
}

static __init void acpi_parse_madt_ioapic(struct acpi_madt_ioapic *entry, struct acpi_ioapic *ioapic) {
	ioapic->id = entry->ioapic_id;
	ioapic->address = (vaddr_t)entry->ioapic_address;
	ioapic->gsi_base = entry->gsi_base;

	printk("ACPI: Found I/O APIC ID %u with GSI base %u at 0x%lx\n",
			ioapic->id, ioapic->gsi_base, (unsigned long)ioapic->address);
}

static __init void acpi_parse_madt_int_override(struct acpi_madt_int_override *entry, struct acpi_irq_override *override) {
	override->bus = entry->bus;
	override->source_irq = entry->source;
	override->gsi = entry->gsi;
	override->flags = entry->flags;

	printk("ACPI: Interrupt override for IRQ %u to GSI %u (bus %u, flags 0x%x)\n",
			override->source_irq, override->gsi, override->bus, override->flags);
}

static __init void acpi_parse_madt_nmi_source(struct acpi_madt_nmi_source *entry, struct acpi_nmi_source *nmi) {
	nmi->source_irq = entry->source;
	nmi->gsi = entry->gsi;
	nmi->flags = entry->flags;

	printk("ACPI: NMI source for IRQ %u mapping to GSI %u (flags 0x%x)\n",
			nmi->source_irq, nmi->gsi, nmi->flags);
}

static __init void acpi_parse_madt_local_nmi(struct acpi_madt_local_nmi *entry, struct acpi_local_nmi *nmi) {
	nmi->acpi_id = entry->processor_id;
	nmi->lint = entry->local_apic_lint;
	nmi->flags = entry->flags;

	if (nmi->acpi_id == MADT_ALL_PROCESSORS) {
		printk("ACPI: Local APIC NMI for all processors on LINT %u (flags 0x%x)\n",
				nmi->lint, nmi->flags);
	} else {
		printk("ACPI: Local APIC NMI for processor %u on LINT %u (flags 0x%x)\n",
				nmi->acpi_id, nmi->lint, nmi->flags);
	}
}

static __init void acpi_parse_madt_local_x2apic(struct acpi_madt_local_x2apic *entry, struct acpi_cpu *cpu) {
	cpu->acpi_id = entry->acpi_uid;
	cpu->apic_id = entry->x2apic_id;
	cpu->flags = entry->flags;
	cpu->is_x2apic = true;
}

static __init void acpi_parse_madt_local_x2apic_nmi(struct acpi_madt_local_x2apic_nmi *entry, struct acpi_local_nmi *nmi) {
	nmi->acpi_id = entry->acpi_uid;
	nmi->lint = entry->local_x2apic_lint;
	nmi->flags = entry->flags;

	printk("ACPI: Local x2APIC NMI for processor UID %u on LINT %u (flags 0x%x)\n",
			nmi->acpi_id, nmi->lint, nmi->flags);
}

void __init acpi_parse_madt(struct acpi_madt *table) {
	if (!table) return;

	struct acpi_madt_info *info = kzalloc(sizeof(struct acpi_madt_info));
	if (!info) {
		printk("ACPI: Failed to allocate memory for MADT info\n");
		return;
	}

	info->local_apic_address = (vaddr_t)table->local_apic_address;
	info->flags = table->flags;

	printk(
		"ACPI: Found Local APIC address %#lx (flags %#x)\n",
		(unsigned long)info->local_apic_address, info->flags
	);

	uintptr_t entries_start = (uintptr_t)table->entries;
	uintptr_t table_end = (uintptr_t)table + table->header.length;

	// Pass 1: Count entries
	struct acpi_madt_entry *entry = (struct acpi_madt_entry *)entries_start;
	while ((uintptr_t)entry + sizeof(struct acpi_madt_entry) <= table_end) {
		size_t len = entry->length;
		if (len < sizeof(struct acpi_madt_entry)) break;
		if ((uintptr_t)entry + len > table_end) break;

		switch (entry->type) {
			case MADT_TYPE_LOCAL_APIC:
				if (len >= sizeof(struct acpi_madt_lapic)) info->cpu_count++;
				break;
			case MADT_TYPE_IO_APIC:
				if (len >= sizeof(struct acpi_madt_ioapic)) info->ioapic_count++;
				break;
			case MADT_TYPE_INTERRUPT_OVERRIDE:
				if (len >= sizeof(struct acpi_madt_int_override)) info->override_count++;
				break;
			case MADT_TYPE_NMI_SOURCE:
				if (len >= sizeof(struct acpi_madt_nmi_source)) info->nmi_source_count++;
				break;
			case MADT_TYPE_LOCAL_NMI:
				if (len >= sizeof(struct acpi_madt_local_nmi)) info->local_nmi_count++;
				break;
			case MADT_TYPE_LOCAL_X2APIC:
				if (len >= sizeof(struct acpi_madt_local_x2apic)) info->cpu_count++;
				break;
			case MADT_TYPE_LOCAL_X2APIC_NMI:
				if (len >= sizeof(struct acpi_madt_local_x2apic_nmi)) info->local_nmi_count++;
				break;
			default:
				break;
		}

		entry = (struct acpi_madt_entry *)((uintptr_t)entry + len);
	}

	// Allocate array storage
	if (info->cpu_count > 0 && !(info->cpus = kcalloc(info->cpu_count, sizeof(struct acpi_cpu)))) goto out_err;
	if (info->ioapic_count > 0 && !(info->ioapics = kcalloc(info->ioapic_count, sizeof(struct acpi_ioapic)))) goto out_err;
	if (info->override_count > 0 && !(info->overrides = kcalloc(info->override_count, sizeof(struct acpi_irq_override)))) goto out_err;
	if (info->local_nmi_count > 0 && !(info->local_nmis = kcalloc(info->local_nmi_count, sizeof(struct acpi_local_nmi)))) goto out_err;
	if (info->nmi_source_count > 0 && !(info->nmi_sources = kcalloc(info->nmi_source_count, sizeof(struct acpi_nmi_source)))) goto out_err;

	// Pass 2: Populate entries
	size_t cpu_idx = 0;
	size_t ioapic_idx = 0;
	size_t override_idx = 0;
	size_t local_nmi_idx = 0;
	size_t nmi_src_idx = 0;

	entry = (struct acpi_madt_entry *)entries_start;
	while ((uintptr_t)entry + sizeof(struct acpi_madt_entry) <= table_end) {
		size_t len = entry->length;
		if (len < sizeof(struct acpi_madt_entry)) break;
		if ((uintptr_t)entry + len > table_end) break;

		switch (entry->type) {
			case MADT_TYPE_LOCAL_APIC:
				if (len >= sizeof(struct acpi_madt_lapic) && cpu_idx < info->cpu_count) {
					acpi_parse_madt_lapic((struct acpi_madt_lapic *)entry, &info->cpus[cpu_idx++]);
				}
				break;

			case MADT_TYPE_IO_APIC:
				if (len >= sizeof(struct acpi_madt_ioapic) && ioapic_idx < info->ioapic_count) {
					acpi_parse_madt_ioapic((struct acpi_madt_ioapic *)entry, &info->ioapics[ioapic_idx++]);
				}
				break;

			case MADT_TYPE_INTERRUPT_OVERRIDE:
				if (len >= sizeof(struct acpi_madt_int_override) && override_idx < info->override_count) {
					acpi_parse_madt_int_override((struct acpi_madt_int_override *)entry, &info->overrides[override_idx++]);
				}
				break;

			case MADT_TYPE_NMI_SOURCE:
				if (len >= sizeof(struct acpi_madt_nmi_source) && nmi_src_idx < info->nmi_source_count) {
					acpi_parse_madt_nmi_source((struct acpi_madt_nmi_source *)entry, &info->nmi_sources[nmi_src_idx++]);
				}
				break;

			case MADT_TYPE_LOCAL_NMI:
				if (len >= sizeof(struct acpi_madt_local_nmi) && local_nmi_idx < info->local_nmi_count) {
					acpi_parse_madt_local_nmi((struct acpi_madt_local_nmi *)entry, &info->local_nmis[local_nmi_idx++]);
				}
				break;

			case MADT_TYPE_LOCAL_APIC_OVERRIDE:
				if (len >= sizeof(struct acpi_madt_local_apic_address_override)) {
					struct acpi_madt_local_apic_address_override *ovr = (struct acpi_madt_local_apic_address_override *)entry;
					info->local_apic_address = (vaddr_t)ovr->local_apic_address;
					printk("ACPI: Local APIC address override: 0x%lx\n", (unsigned long)info->local_apic_address);
				}
				break;

			case MADT_TYPE_LOCAL_X2APIC:
				if (len >= sizeof(struct acpi_madt_local_x2apic) && cpu_idx < info->cpu_count) {
					acpi_parse_madt_local_x2apic((struct acpi_madt_local_x2apic *)entry, &info->cpus[cpu_idx++]);
				}
				break;

			case MADT_TYPE_LOCAL_X2APIC_NMI:
				if (len >= sizeof(struct acpi_madt_local_x2apic_nmi) && local_nmi_idx < info->local_nmi_count) {
					acpi_parse_madt_local_x2apic_nmi((struct acpi_madt_local_x2apic_nmi *)entry, &info->local_nmis[local_nmi_idx++]);
				}
				break;

			default:
				printk("ACPI: Unknown or unhandled MADT entry type %u (length %u)\n", entry->type, entry->length);
				break;
		}

		entry = (struct acpi_madt_entry *)((uintptr_t)entry + len);
	}

	acpi_madt_info = info;

	return;

out_err:
	printk("ACPI: Memory allocation failure while parsing MADT\n");
	if (info->cpus) kfree(info->cpus);
	if (info->ioapics) kfree(info->ioapics);
	if (info->overrides) kfree(info->overrides);
	if (info->local_nmis) kfree(info->local_nmis);
	if (info->nmi_sources) kfree(info->nmi_sources);
	kfree(info);
}

