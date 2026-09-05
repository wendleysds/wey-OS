#include <def/compile.h>
#include <io/ports.h>
#include <stdbool.h>

#include "boot.h"

#include <uapi/headers.h>

struct rsdp_descriptor_v1 {
	char     signature[8];
	uint8_t  checksum;
	char     oemid[6];
	uint8_t  revision;
	uint32_t rsdt_address;
} __packed;

struct rsdp_descriptor_v2 {
	struct rsdp_descriptor_v1 v1;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t  extended_checksum;
	uint8_t  reserved[3];
} __packed;

typedef union {
	struct rsdp_descriptor_v1 v1;
	struct rsdp_descriptor_v2 v2;
} rsdp_descriptor_t;

static inline uint8_t acpi_checksum(const void* data, size_t size){
	uint8_t sum = 0;
	for(size_t i = 0; i < size; i++){
		sum += rdfs8((uint32_t)data + i);
	}

	return sum;
}

static bool acpi_validate_rsdp(const rsdp_descriptor_t *rsdp){
	if (memcmp_fs((uint32_t)rsdp->v1.signature, "RSD PTR ", 8) != 0)
		return false;

	if (acpi_checksum(&rsdp->v1, sizeof(struct rsdp_descriptor_v1)) != 0)
		return false;

	if (rsdp->v1.revision < 2)
		return true;

	if (rsdp->v2.length < sizeof(struct rsdp_descriptor_v2))
		return false;

	if (acpi_checksum(rsdp, rsdp->v2.length) != 0)
		return false;

	return true;
}

int acpi_find_rsdp(struct boot_tag_acpi* acpi){
	set_fs(0x0000);

	uint16_t ebda = rdfs16(0x40E);
	uint32_t ebda_base = 0;
	uint32_t addr;

	if(ebda != 0)
		ebda_base = ebda * 16;

	// EBDA
	for(addr = ebda_base; addr < ebda_base + 0x1000; addr += 16){
		struct rsdp_descriptor_v1 *v1 = (struct rsdp_descriptor_v1*)addr;

		if (!acpi_validate_rsdp((void*)v1))
			continue;

		goto found;
	}

	// BIOS ROM
	for(addr = 0xE0000; addr < 0x100000; addr += 16){
		struct rsdp_descriptor_v1 *v1 = (struct rsdp_descriptor_v1*)addr;

		if (!acpi_validate_rsdp((void*)v1))
			continue;

		goto found;
	}

	return 0;

found:
	acpi->rsdp = (uint64_t)addr;

	return 1;
}
