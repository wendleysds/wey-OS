#include <kernel/printk.h>
#include <kernel/init.h>
#include <def/config.h>

#include <uapi/headers.h>

static struct e820_table {
	struct e820_entry* entries;
	size_t length;
} e820_table;

static __init void e820_sort(void){
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

static __init void e820_merge(void){
	struct e820_entry* map = e820_table.entries;
	size_t* count = &e820_table.length;

	int merged = 0;
	size_t new_count = 0;

	while(1){
		// Resolve conflicts
		for (size_t i = 0; i < *count; i++) {
			struct e820_entry current = map[i];

			if (i + 1 < *count) {
				struct e820_entry* next = &map[i+1];

				if (current.base_addr + current.length > next->base_addr) {
					if (current.type == next->type) {
						uint64_t end = (current.base_addr + current.length > next->base_addr + next->length) 
									? current.base_addr + current.length 
									: next->base_addr + next->length;
						next->base_addr = current.base_addr;
						next->length = end - next->base_addr;
						continue;
					} 
					else {
						current.length = next->base_addr - current.base_addr;
					}
				}
			}

			if (current.length > 0) {
				map[new_count++] = current;
			}
		}
		*count = new_count;

		// Merge
		new_count = 0;
		for (size_t i = 0; i < *count; i++) {
			struct e820_entry current = map[i];

			if (i + 1 < *count) {
				struct e820_entry* next = &map[i+1];

				if (current.base_addr + current.length == next->base_addr &&
					current.type == next->type) {
					current.length += next->length;
					i++;
				}
				else if ((current.base_addr + current.length + 1) == next->base_addr &&
					current.type == next->type) {
					current.length += next->length + 1;
					i++;
				}
			}

			map[new_count++] = current;
		}
		*count = new_count;

		// Everything merged?
		merged = 0;
		for (size_t i = 0; i < *count - 1; i++) {
			struct e820_entry* current = &map[i];
			struct e820_entry* next = &map[i+1];
			if (current->base_addr + current->length == next->base_addr &&
				current->type == next->type) {
				merged = 1;
				break;
			}else if ((current->base_addr + current->length + 1) == next->base_addr &&
				current->type == next->type) {
				merged = 1;
				break;
			}
		}

		if (!merged) {
			break;
		}
	}
}

static __init inline void e820_sanitize(){
	e820_sort();
	e820_merge();
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
	e820_sanitize();
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