#include <kernel/acpi.h>
#include <kernel/init.h>
#include <kernel/printk.h>

#include <lib/string.h>
#include <lib/list.h>
#include <mm/kheap.h>

#include <def/errno.h>

#include "../internal.h"

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