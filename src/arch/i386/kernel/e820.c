#include <wey/printk.h>
#include <def/init.h>
#include <def/config.h>
#include <stddef.h>
#include <stdint.h>

#include <uapi/headers.h>

static struct e820_table {
	struct e820_entry* entries;
	size_t length;
} e820_table;

static __init void e820_sort(){
	size_t i, j;
	struct e820_entry* entries = e820_table.entries;
	for (i = 1; i < e820_table.length; i++) {
		struct e820_entry key = e820_table.entries[i];
		j = i;

		while (j > 0 &&
			entries[j - 1].base_addr > key.base_addr) {
			entries[j] = entries[j - 1];
			j--;
		}

		entries[j] = key;
	}
}

static __init void e820_print_type(enum e820_type type)
{
	switch (type) {
		case E820_TYPE_RAM:	
			printk(" System RAM");
			break;
		case E820_TYPE_RESERVED:
			printk(" Device reserved");
			break;
		case E820_TYPE_SOFT_RESERVED:
			printk(" Soft reserved");
			break;
		case E820_TYPE_ACPI:
			printk(" ACPI data");
			break;
		case E820_TYPE_NVS:
			printk(" ACPI NVS");
			break;
		case E820_TYPE_UNUSABLE:
			printk(" Unusable");
			break;
		case E820_TYPE_PMEM:
		case E820_TYPE_PRAM:
			printk(" Persistent RAM (type %u)", type);
			break;
		default:
			printk(" Type %u", type);
			break;
	}
}

__init void e820_init(struct e820_entry* entries, size_t length){
	e820_table.entries = entries;
	e820_table.length = length;
	e820_sort();
}

__init void e820_print(){
	uint64_t range_end_prev = 0;
	for (size_t idx = 0; idx < e820_table.length; idx++) {
		struct e820_entry* entry = &e820_table.entries[idx];
		uint64_t range_start, range_end;

		range_start = entry->base_addr;

		if (entry->length > UINT64_MAX - entry->base_addr) {
			printk("E820: entry overflow!\n");
			continue;
		}

		range_end = entry->base_addr + entry->length;

		/* Out of order E820 maps should not happen: */
		if (range_start < range_end_prev)
			printk("E820: out of order E820 entry!\n");

		if (range_start > range_end_prev) {
			printk("E820:    [gap %#018llx-%#018llx]\n",
				range_end_prev,
				range_start-1);
		}

		printk("E820:    [mem %#018llx-%#018llx] ", range_start, range_end-1);
		e820_print_type(entry->type);
		printk("\n");

		range_end_prev = range_end;
	}
}

__init size_t e820_end_ram_pfn(size_t limit_pfn) {
	size_t last_pfn = 0;
	size_t max_arch_pfn = MAX_ARCH_PFN;

	for (size_t i = 0; i < e820_table.length; i++) {
		struct e820_entry *entry = &e820_table.entries[i];
		size_t start_pfn;
		size_t end_pfn;

		if (entry->type != E820_TYPE_RAM &&
			entry->type != E820_TYPE_ACPI)
			continue;

		start_pfn = entry->base_addr >> PAGE_SHIFT;
		end_pfn = (entry->base_addr + entry->length) >> PAGE_SHIFT;

		if (start_pfn >= limit_pfn)
			continue;
		if (end_pfn > limit_pfn) {
			last_pfn = limit_pfn;
			break;
		}
		if (end_pfn > last_pfn)
			last_pfn = end_pfn;
	}

	if (last_pfn > max_arch_pfn)
		last_pfn = max_arch_pfn;

	printk("E820: last_pfn = %#lx; max_arch_pfn = %#lx\n", last_pfn, max_arch_pfn);
	return last_pfn;
}