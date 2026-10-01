#ifndef _ACPI_TABLE_HPET
#define _ACPI_TABLE_HPET

#include <kernel/acpi.h>
#include <def/compile.h>

#define ACPI_HPET_SIGNATURE "HPET"

struct acpi_hpet {
	struct acpi_sdt_header header;
	uint32_t event_timer_block_id;
	struct acpi_generic_address base_address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __packed;

int hpet_init(void);
uint64_t hpet_get_ticks(void);
void hpet_udelay(uint64_t us);

#endif