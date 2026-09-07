#include <kernel/acpi.h>
#include <kernel/init.h>

#include <kernel/printk.h>

#include <lib/string.h>
#include <def/errno.h>
#include <asm/page.h>

#include "internal.h"

extern unsigned long acpi_rsdp;

static struct acpi_rsdt *rsdt;

void *acpi_find_table(const char *signature){
	const size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
	for (size_t i = 0; i < entries; i++){
		struct acpi_sdt_header *sdt = (struct acpi_sdt_header *)__va(rsdt->entries[i]);

		if(memcmp(sdt->signature, signature, 4) == 0) {
			if(!acpi_checksum_ok(sdt, sdt->length)) {
				printk("ACPI: Invalid table '%.4s' checksum\n", sdt->signature);
				continue;
			}

			return sdt;
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

static rsdp_descriptor_t *acpi_parse_rsdp(void) {
	rsdp_descriptor_t *rsdp = (rsdp_descriptor_t *)acpi_rsdp;

	if (memcmp(rsdp->v1.signature, "RSD PTR ", 8) != 0) {
		printk("ACPI: Invalid RSDP signature\n");
		return NULL;
	}

	/* ACPI 1.0 checksum */
	if (!acpi_checksum_ok(rsdp, sizeof(struct rsdp_descriptor_v1))) {
		printk("ACPI: Invalid RSDP checksum\n");
		return NULL;
	}

	/* ACPI 2.0+ extended checksum */
	if (rsdp->v1.revision >= 2) {
		if (rsdp->v2.length < sizeof(struct rsdp_descriptor_v2)) {
			printk("ACPI: Invalid RSDP length\n");
			return NULL;
		}

		if (!acpi_checksum_ok(rsdp, rsdp->v2.length)) {
			printk("ACPI: Invalid extended RSDP checksum\n");
			return NULL;
		}
	}

	return rsdp;
}

static struct acpi_rsdt *acpi_parse_rsdt(rsdp_descriptor_t *rsdp) {
	if (rsdp->v1.rsdt_address == 0) return NULL;

	struct acpi_rsdt *rsdt = (struct acpi_rsdt *)__va(rsdp->v1.rsdt_address);

	if (memcmp(rsdt->header.signature, "RSDT", 4) != 0) {
		printk("ACPI: Invalid RSDT signature\n");
		return NULL;
	}

	if (rsdt->header.length < sizeof(struct acpi_sdt_header)) {
		printk("ACPI: Invalid RSDT length\n");
		return NULL;
	}

	if (!acpi_checksum_ok(rsdt, rsdt->header.length)) {
		printk("ACPI: Invalid RSDT checksum\n");
		return NULL;
	}

	return rsdt;
}

static __init int acpi_init(void) {
	if(!acpi_rsdp) return 0;

	rsdp_descriptor_t *rsdp = acpi_parse_rsdp();
	if(!rsdp) return -ENOENT;

	rsdt = acpi_parse_rsdt(rsdp);
	if(!rsdt) return -ENOENT;
	
	struct acpi_fadt *fadt = acpi_find_table("FACP");
	if(!fadt) return -ENODEV;
	
	acpi_parse_fadt(fadt);
	
	return 0;
}

core_initcall(acpi_init);