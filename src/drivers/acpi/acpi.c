#include <kernel/acpi.h>
#include <kernel/init.h>

#include <kernel/printk.h>
#include <kernel/clock.h>

#include <lib/string.h>
#include <lib/list.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <asm/page.h>
#include <asm/cpu.h>

#include "internal.h"

extern unsigned long acpi_rsdp;

static struct acpi_rsdt *rsdt;

struct acpi_pm_info acpi_pm;

struct acpi_table{
	struct acpi_sdt_header *sdt;
	struct list_head node;
};

static LIST_HEAD(acpi_tables);

void acpi_parse_tables(void){
	const size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
	for (size_t i = 0; i < entries; i++){
		struct acpi_sdt_header *tmp = acpi_map(rsdt->entries[i], sizeof(struct acpi_sdt_header));
		if(!tmp) continue;

		struct acpi_sdt_header *sdt = acpi_map(rsdt->entries[i], tmp->length);
		acpi_unmap(tmp);

		if(!sdt) continue;

		if(!acpi_checksum_ok(sdt, sdt->length)) {
			printk("ACPI: Invalid table '%.4s' checksum\n", sdt->signature);
			acpi_unmap(sdt);
			continue;
		}

		struct acpi_table* table = kmalloc(sizeof(struct acpi_table));
		if(!table){
			printk("ACPI: allocation failed for table '%.4s'\n", sdt->signature);
			acpi_unmap(sdt);
			continue;
		}

		table->sdt = sdt;
		INIT_LIST_HEAD(&table->node);
		list_add(&table->node, &acpi_tables);
	}
}

void *acpi_find_table(const char *signature){
	struct acpi_table* pos;
	list_for_each_entry(pos, &acpi_tables, node){
		if(memcmp(pos->sdt->signature, signature, 4) == 0) {
			return pos->sdt;
		}
	}

	return NULL;
}

static void acpi_print_sdt(const struct acpi_sdt_header *sdt){
	printk("ACPI: Table '%.4s' at 0x%lx\n", sdt->signature, (uintptr_t)sdt);
	printk("    length: 0x%x, revision: 0x%x\n", sdt->length, sdt->revision);
	printk("    OEM ID: %c%c%c%c%c%c\n", sdt->oemid[0], sdt->oemid[1], sdt->oemid[2], sdt->oemid[3], sdt->oemid[4], sdt->oemid[5]);
	printk("    OEM Table ID: 0x%llx\n", sdt->oem_table_id);
	printk("    OEM revision: 0x%x\n", sdt->oem_revision);
	printk("    Creator ID: 0x%x\n", sdt->creator_id);
	printk("    Creator revision: 0x%x\n", sdt->creator_revision);
}

static rsdp_descriptor_t *acpi_parse_rsdp(paddr_t paddr) {
	rsdp_descriptor_t *rsdp = acpi_map(paddr, sizeof(rsdp_descriptor_t));
	if(!rsdp) return NULL;

	if (memcmp(rsdp->v1.signature, "RSD PTR ", 8) != 0) {
		printk("ACPI: Invalid RSDP signature\n");
		goto out_invalid;
	}

	/* ACPI 1.0 checksum */
	if (!acpi_checksum_ok(rsdp, sizeof(struct rsdp_descriptor_v1))) {
		printk("ACPI: Invalid RSDP checksum\n");
		goto out_invalid;
	}

	/* ACPI 2.0+ extended checksum */
	if (rsdp->v1.revision >= 2) {
		if (rsdp->v2.length < sizeof(struct rsdp_descriptor_v2)) {
			printk("ACPI: Invalid RSDP length\n");
			goto out_invalid;
		}

		if (!acpi_checksum_ok(rsdp, rsdp->v2.length)) {
			printk("ACPI: Invalid extended RSDP checksum\n");
			goto out_invalid;
		}
	}

	return rsdp;

out_invalid:
	acpi_unmap(rsdp);
	return NULL;
}

static struct acpi_rsdt *acpi_parse_rsdt(rsdp_descriptor_t *rsdp) {
	if (rsdp->v1.rsdt_address == 0) return NULL;

	struct acpi_sdt_header *sdt = acpi_map(rsdp->v1.rsdt_address, sizeof(struct acpi_sdt_header));
	if(!sdt) return NULL;

	struct acpi_rsdt *rsdt = acpi_map(rsdp->v1.rsdt_address, sdt->length);
	acpi_unmap(sdt);

	if(!rsdt) return NULL;

	if (memcmp(rsdt->header.signature, "RSDT", 4) != 0) {
		printk("ACPI: Invalid RSDT signature\n");
		goto out_invalid;
	}

	if (rsdt->header.length < sizeof(struct acpi_sdt_header)) {
		printk("ACPI: Invalid RSDT length\n");
		goto out_invalid;
	}

	if (!acpi_checksum_ok(rsdt, rsdt->header.length)) {
		printk("ACPI: Invalid RSDT checksum\n");
		goto out_invalid;
	}

	return rsdt;

out_invalid:
	acpi_unmap(rsdt);
	return NULL;
}

static inline void ndelay(uint64_t ns){
	const uint64_t start = clock_get_realtime_ns();
	const uint64_t end = start + ns;

	while(clock_get_realtime_ns() < end) cpu_relax();
}

static int acpi_enable(void){
	if(!acpi_pm.acpi_enable || !acpi_pm.acpi_disable) return -ENODEV;
	
	uint16_t value;

	if (acpi_pm.smi_cmd.pio_base == 0) {
		return -ENODEV;
	}

	if (acpi_pm.pm1a_cnt.pio_base == 0) {
		return -ENODEV;
	}

	value = io_read16(&acpi_pm.pm1a_cnt, 0);

	if (value & (1 << 0)) {
		printk("ACPI: SCI already enabled\n");
		return 0;
	}

	if (acpi_pm.acpi_enable == 0) {
		printk("ACPI: ACPI_ENABLE command unavailable\n");
		return -ENODEV;
	}

	io_write8(&acpi_pm.smi_cmd, 0, acpi_pm.acpi_enable);

	for (unsigned int i = 0; i < 300; i++) {
		value = io_read16(&acpi_pm.pm1a_cnt, 0);

		if (value & (1 << 0)) {
			printk("ACPI: ACPI mode enabled\n");
			return 0;
		}

		ndelay(1000000);
	}

	printk("ACPI: timeout waiting for SCI_EN\n");

	return -ETIMEDOUT;
}

static __init int acpi_init(void) {
	if(!acpi_rsdp) return 0;
	memset(&acpi_pm, 0, sizeof(acpi_pm));

	rsdp_descriptor_t *rsdp = acpi_parse_rsdp(acpi_rsdp);
	if(!rsdp) return -ENOENT;

	rsdt = acpi_parse_rsdt(rsdp);
	if(!rsdt) return -ENOENT;

	INIT_LIST_HEAD(&acpi_tables);

	acpi_parse_tables();

	struct acpi_madt *madt = acpi_find_table(ACPI_MADT_SIGNATURE);
	if(madt){
		acpi_parse_madt(madt);
		acpi_unmap(madt);
	}
	
	struct acpi_fadt *fadt = acpi_find_table(ACPI_FADT_SIGNATURE);
	if(fadt){
		acpi_parse_fadt(fadt);
		acpi_unmap(fadt);
		return acpi_enable();
	}

	return 0;
}

core_initcall(acpi_init);