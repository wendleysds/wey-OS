#include <kernel/acpi/tables/hpet.h>
#include <kernel/printk.h>
#include <mm/iomem.h>
#include <def/errno.h>
#include <io/region.h>
#include <sync/spinlock.h>

#include <lib/div64.h>

#include <asm/cpu.h>
#include <def/config.h>

#include "../internal.h"

#define HPET_GEN_CAP_ID     0x000
#define HPET_GEN_CONFIG     0x010
#define HPET_MAIN_COUNTER   0x0F0

#define HPET_CAP_COUNT_SIZE (1ULL << 13)   /* 1 = main counter de 64 bits */
#define HPET_CFG_ENABLE     (1ULL << 0)
#define HPET_CFG_LEG_RT     (1ULL << 1)    /* legacy replacement mapping */
#define HPET_MAX_PERIOD_FS  0x05F5E100U    /* limite da spec (100 ns) */

static io_region_t hpet_region;
static uint32_t hpet_period_fs = 0;
static bool hpet_initialized = false;
static bool hpet_is_64bit = false;
static uint64_t hpet_counter_mask = 0xFFFFFFFFULL;

static spinlock_t hpet_lock;
static uint64_t hpet_last_raw = 0;
static uint64_t hpet_ext_ticks = 0;

static inline uint64_t hpet_read64(uint32_t reg){
	uint32_t low = io_read32(&hpet_region, reg);
	uint32_t high = io_read32(&hpet_region, reg + 0x4);
	return low | ((uint64_t)high << 32);
}

static inline void hpet_write64(uint32_t reg, uint64_t val){
	io_write32(&hpet_region, reg, (uint32_t)val);
	io_write32(&hpet_region, reg + 0x4, (uint32_t)(val >> 32));
}

bool hpet_is_initialized(void){
	return hpet_initialized;
}

static uint64_t hpet_read_counter_raw(void){
	if(!hpet_is_64bit)
		return io_read32(&hpet_region, HPET_MAIN_COUNTER);

	uint32_t hi, lo, hi2;
	do {
		hi  = io_read32(&hpet_region, HPET_MAIN_COUNTER + 4);
		lo  = io_read32(&hpet_region, HPET_MAIN_COUNTER);
		hi2 = io_read32(&hpet_region, HPET_MAIN_COUNTER + 4);
	} while(hi != hi2);

	return ((uint64_t)hi << 32) | lo;
}

uint64_t hpet_get_ticks(void){
	if(hpet_is_64bit)
		return hpet_read_counter_raw();

	uint64_t ticks;

	unsigned long flags;
	spin_lock_irqsave(&hpet_lock, &flags);
	uint64_t raw = hpet_read_counter_raw();
	hpet_ext_ticks += (raw - hpet_last_raw) & hpet_counter_mask;
	hpet_last_raw = raw;
	ticks = hpet_ext_ticks;
	spin_unlock_irqrestore(&hpet_lock, &flags);

	return ticks;
}

int hpet_init(void){
	if(hpet_initialized) return 0;

	struct acpi_hpet *hpet = (void*)acpi_find_table(ACPI_HPET_SIGNATURE);
	if(!hpet) return -ENODEV;

	acpi_gas_to_io_region(&hpet->base_address, &hpet_region);

	if(hpet_region.type != IO_TYPE_MMIO)
		return -ENOTSUP;

	vaddr_t hpet_base = (vaddr_t)ioremap(hpet->base_address.address, PAGE_SIZE);
	if(!hpet_base) return -ENOMEM;

	hpet_region.mmio_base = hpet_base;

	uint64_t cap = hpet_read64(HPET_GEN_CAP_ID);
	uint32_t period = (uint32_t)(cap >> 32);

	if(period == 0 || period > HPET_MAX_PERIOD_FS){
		printk("HPET: invalid period %u fs\n", period);
		return -ENODEV;
	}

	hpet_period_fs = period;
	hpet_is_64bit = !!(cap & HPET_CAP_COUNT_SIZE);
	hpet_counter_mask = hpet_is_64bit ? ~0ULL : 0xFFFFFFFFULL;

	spinlock_init(&hpet_lock);

	uint64_t config = hpet_read64(HPET_GEN_CONFIG);
	config &= ~(HPET_CFG_ENABLE | HPET_CFG_LEG_RT);
	hpet_write64(HPET_GEN_CONFIG, config);

	hpet_write64(HPET_MAIN_COUNTER, 0);
	hpet_last_raw = 0;
	hpet_ext_ticks = 0;

	config |= HPET_CFG_ENABLE;
	hpet_write64(HPET_GEN_CONFIG, config);

	printk("HPET: period_fs %u, %s counter, cap %llx, config %llx\n",
		hpet_period_fs, hpet_is_64bit ? "64-bit" : "32-bit", cap, config);

	hpet_initialized = true;
	return 0;
}

void hpet_udelay(uint64_t us){
	if(!hpet_initialized) return;

	/* fs = us * 10^9. Ticks = (us * 10^9) / period_fs */
	uint64_t ticks = us * 1000000000ULL;
	do_div(ticks, hpet_period_fs);

	uint64_t start = hpet_read_counter_raw();
	while(((hpet_read_counter_raw() - start) & hpet_counter_mask) < ticks){
		cpu_relax();
	}
}

static inline time_ns_t hpet_ticks_to_ns(uint64_t ticks){
	uint64_t q = ticks;
	uint32_t r = do_div(q, 1000000U);

	uint64_t ns = q * (uint64_t)hpet_period_fs;
	uint64_t frac = (uint64_t)r * (uint64_t)hpet_period_fs;
	do_div(frac, 1000000U);

	return (time_ns_t)(ns + frac);
}

static time_ns_t hpet_get_time_ns(void* data){
	(void)data;

	if(!hpet_initialized)
		return 0;

	return hpet_ticks_to_ns(hpet_get_ticks());
}

const struct clocksource hpet_clocksource = {
	.name = "hpet",
	.read_ns = hpet_get_time_ns,
	.data = 0x0,
};