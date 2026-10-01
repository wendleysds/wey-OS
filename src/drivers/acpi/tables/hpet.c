#include <kernel/acpi/tables/hpet.h>
#include <kernel/printk.h>
#include <mm/iomem.h>
#include <def/errno.h>
#include <io/region.h>

#include <lib/div64.h>

#include <asm/cpu.h>
#include <def/config.h>

#include "../internal.h"

#define HPET_GEN_CAP_ID     0x000
#define HPET_GEN_CONFIG     0x010
#define HPET_MAIN_COUNTER   0x0F0

static io_region_t hpet_region;
static uint32_t hpet_period_fs = 0;

static inline uint64_t hpet_read64(uint32_t reg){
	uint32_t low = io_read32(&hpet_region, reg);
	uint32_t high = io_read32(&hpet_region, reg + 0x4);
    return low | ((uint64_t)high << 32);
}

static inline void hpet_write64(uint32_t reg, uint64_t val){
    io_write32(&hpet_region, reg, (uint32_t)val);
    io_write32(&hpet_region, reg + 0x4, (uint32_t)(val >> 32));
}

int hpet_init(void){
	struct acpi_hpet *hpet = (void*)acpi_find_table(ACPI_HPET_SIGNATURE);
	if(!hpet) return -ENODEV;

	acpi_gas_to_io_region(&hpet->base_address, &hpet_region);

	if(hpet_region.type == IO_TYPE_MMIO){
		vaddr_t hpet_base = (vaddr_t)ioremap(
			hpet->base_address.address, 
			PAGE_SIZE
		);

		if(!hpet_base) return -ENOMEM;

		hpet_region.mmio_base = hpet_base;
	}else{
		return -ENOTSUP;
	}

	uint64_t cap = hpet_read64(HPET_GEN_CAP_ID);
    hpet_period_fs = (uint32_t)(cap >> 32);

	uint64_t config = hpet_read64(HPET_GEN_CONFIG);
    config |= 1;
    hpet_write64(HPET_GEN_CONFIG, config);

	printk("HPET: period_fs %u, cap %llx, config %llx\n", hpet_period_fs, cap, config);

	return 0;
}

uint64_t hpet_get_ticks(void) {
    return hpet_read64(HPET_MAIN_COUNTER);
}

void hpet_udelay(uint64_t us) {
    // fs = us * 10^9. Ticks = (us * 10^9) / period_fs
    uint64_t ticks = (us * 1000000000ULL);
	do_div(ticks, hpet_period_fs);

    uint64_t start = hpet_get_ticks();
    while ((hpet_get_ticks() - start) < ticks) {
        cpu_relax();
    }
}

