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

extern struct acpi_pm_info *acpi_pm;
extern struct acpi_madt_info *acpi_madt_info;

struct acpi_table{
	struct acpi_sdt_header *sdt;
	paddr_t physaddr;
	struct list_head node;
};

static LIST_HEAD(acpi_tables);

static bool __init acpi_table_exists(paddr_t physaddr) {
	struct acpi_table *pos;
	list_for_each_entry(pos, &acpi_tables, node){
		if(pos->physaddr == physaddr) return true;
	}

	return false;
}

int __init acpi_add_table(paddr_t physaddr){
	if(physaddr == 0) return -EINVAL;
	if(acpi_table_exists(physaddr)) return -EEXIST;

	struct acpi_sdt_header *sdt = acpi_parse_sdt_header(physaddr);
	if(!sdt) return -ENOMEM;

	struct acpi_table *table = kmalloc(sizeof(struct acpi_table));
	if(!table){
		printk("ACPI: allocation failed for table '%.4s'\n", sdt->signature);
		acpi_unmap(sdt);
		return -ENOMEM;
	}

	table->sdt = sdt;
	table->physaddr = physaddr;
	INIT_LIST_HEAD(&table->node);
	list_add(&table->node, &acpi_tables);

	printk("ACPI: Table '%.4s' at 0x%llx\n", sdt->signature, (vaddr_t)sdt);

	return 0;
}

void __init acpi_remove_table(paddr_t physaddr){
	struct acpi_table *pos, *tmp;
	list_for_each_entry_safe(pos, tmp, &acpi_tables, node){
		if(pos->physaddr == physaddr) {
			acpi_unmap(pos->sdt);
			list_remove(&pos->node);
			kfree(pos);
			return;
		}
	}
}

struct acpi_sdt_header *acpi_find_table(const char *signature){
	struct acpi_table *pos;
	list_for_each_entry(pos, &acpi_tables, node){
		if(memcmp(pos->sdt->signature, signature, 4) == 0) {
			return pos->sdt;
		}
	}

	return NULL;
}

static __init void acpi_print_sdt(const struct acpi_sdt_header *sdt){
	printk("ACPI: Table '%.4s' at 0x%lx\n", sdt->signature, (uintptr_t)sdt);
	printk("    length: 0x%x, revision: 0x%x\n", sdt->length, sdt->revision);
	printk("    OEM ID: %c%c%c%c%c%c\n", sdt->oemid[0], sdt->oemid[1], sdt->oemid[2], sdt->oemid[3], sdt->oemid[4], sdt->oemid[5]);
	printk("    OEM Table ID: 0x%llx\n", sdt->oem_table_id);
	printk("    OEM revision: 0x%x\n", sdt->oem_revision);
	printk("    Creator ID: 0x%x\n", sdt->creator_id);
	printk("    Creator revision: 0x%x\n", sdt->creator_revision);
}

static __init rsdp_descriptor_t *acpi_parse_rsdp(paddr_t paddr) {
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

static __init inline void ndelay(uint64_t ns){
	const uint64_t start = clock_get_realtime_ns();
	const uint64_t end = start + ns;

	while(clock_get_realtime_ns() < end) cpu_relax();
}

static __init int acpi_enable(void){
	if(!acpi_pm) return -ENODEV;

	if(!acpi_pm->acpi_enable || !acpi_pm->acpi_disable) return -ENODEV;
	
	uint16_t value;

	if (acpi_pm->smi_cmd.pio_base == 0) {
		return -ENODEV;
	}

	if (acpi_pm->pm1a_cnt.pio_base == 0) {
		return -ENODEV;
	}

	value = io_read16(&acpi_pm->pm1a_cnt, 0);

	if (value & (1 << 0)) {
		printk("ACPI: SCI already enabled\n");
		return 0;
	}

	if (acpi_pm->acpi_enable == 0) {
		printk("ACPI: ACPI_ENABLE command unavailable\n");
		return -ENODEV;
	}

	io_write8(&acpi_pm->smi_cmd, 0, acpi_pm->acpi_enable);

	for (unsigned int i = 0; i < 300; i++) {
		value = io_read16(&acpi_pm->pm1a_cnt, 0);

		if (value & (1 << 0)) {
			printk("ACPI: ACPI mode enabled\n");
			return 0;
		}

		ndelay(1000000);
	}

	printk("ACPI: timeout waiting for SCI_EN\n");

	return -ETIMEDOUT;
}

static __init int acpi_get_all_tables(void){
	if(!acpi_rsdp) return 0;

	rsdp_descriptor_t *rsdp = acpi_parse_rsdp(acpi_rsdp);
	if(!rsdp) return -ENOENT;

	paddr_t rsdp_paddr = rsdp->v1.revision >= 2 ?
		rsdp->v2.xsdt_address : rsdp->v1.rsdt_address;

	int res = acpi_add_table(rsdp_paddr);
	if(res) return res;

	struct acpi_sdt_header *sdt;

	if((sdt = acpi_find_table(ACPI_RSDT_SIGNATURE))){
		acpi_parse_rsdt((void*)sdt);
	}
	else if((sdt = acpi_find_table(ACPI_XSDT_SIGNATURE))){
		acpi_parse_xsdt((void*)sdt);
	}else{
		acpi_remove_table(rsdp_paddr);
		return -ENOENT;
	}

	return 0;
}

static __init int acpi_init(void) {
	if(list_empty(&acpi_tables)) return 0;
	acpi_madt_info = NULL;
	acpi_pm = NULL;

	struct acpi_madt *madt = (void*)acpi_find_table(ACPI_MADT_SIGNATURE);
	if(madt){
		acpi_parse_madt(madt);
		acpi_unmap(madt);
	}

	struct acpi_fadt *fadt = (void*)acpi_find_table(ACPI_FADT_SIGNATURE);
	if(fadt){
		acpi_parse_fadt(fadt);
		acpi_unmap(fadt);
	}
	
	acpi_enable();

	return 0;
}

pure_initcall(acpi_get_all_tables);
core_initcall(acpi_init);
