#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>

enum e820_type {
	E820_TYPE_RAM		= 1,
	E820_TYPE_RESERVED	= 2,
	E820_TYPE_ACPI		= 3,
	E820_TYPE_NVS		= 4,
	E820_TYPE_UNUSABLE	= 5,
	E820_TYPE_PMEM		= 7,
	E820_TYPE_PRAM		= 12,
	E820_TYPE_SOFT_RESERVED	= 0xefffffff,
};

struct e820_entry{
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
	uint32_t acpi_attrs;
} __packed;

#endif