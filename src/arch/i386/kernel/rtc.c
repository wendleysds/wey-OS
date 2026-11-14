#include <kernel/acpi/tables/fadt.h>
#include <kernel/clock.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <device/acpi.h>
#include <sync/spinlock.h>
#include <def/errno.h>

#include <asm/cpu.h>
#include <arch/i386/rtc.h>

#define CMOS_REG_SECONDS 0x00
#define CMOS_REG_MINUTES 0x02
#define CMOS_REG_HOURS   0x04
#define CMOS_REG_DAY     0x07
#define CMOS_REG_MONTH   0x08
#define CMOS_REG_YEAR    0x09
#define CMOS_REG_STATUSA 0x0A
#define CMOS_REG_STATUSB 0x0B
#define CMOS_REG_STATUSD 0x0D

#define CMOS_STATUSA_UIP    0x80
#define CMOS_STATUSB_24H    0x02
#define CMOS_STATUSB_BINARY 0x04
#define CMOS_STATUSD_VRT    0x80

#define CMOS_ADDRESS 0x00
#define CMOS_DATA    0x01

static io_region_t cmos_io_region;

static uint8_t cmos_century_reg = 0;

static DEFINE_SPINLOCK(cmos_lock);

static inline uint8_t cmos_read_unlocked(uint8_t reg){
	io_write8(&cmos_io_region, CMOS_ADDRESS, 0x80 | (reg & 0x7F));
	uint8_t val = io_read8(&cmos_io_region, CMOS_DATA);
	io_write8(&cmos_io_region, CMOS_ADDRESS, CMOS_REG_STATUSD);

	return val;
}

static uint8_t bcd_to_bin(uint8_t value){
	return (value & 0x0F) + ((value >> 4) * 10);
}

static int is_leap_year(uint16_t year){
	return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static uint8_t days_in_month(uint16_t year, uint8_t month){
	static const uint8_t month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if(month == 2 && is_leap_year(year)){
		return 29;
	}
	return month_days[month - 1];
}

static uint64_t rtc_days_since_epoch(const struct rtc_time* tm){
	uint64_t days = 0;

	for(uint16_t year = 1970; year < tm->year; year++){
		days += is_leap_year(year) ? 366 : 365;
	}

	for(uint8_t month = 1; month < tm->month; month++){
		days += days_in_month(tm->year, month);
	}

	days += tm->day - 1;
	return days;
}

int rtc_read(struct rtc_time* tm){
	if(!tm){
		return -EINVAL;
	}

	uint8_t status_b, status_d;
	uint8_t second, minute, hour, day, month, year;
	uint8_t century = 0;
	uint8_t hour_pm;
	uint32_t retries = 100000;

	unsigned long flags;
	spin_lock_irqsave(&cmos_lock, &flags);

	status_d = cmos_read_unlocked(CMOS_REG_STATUSD);
	if(!(status_d & CMOS_STATUSD_VRT)){
		spin_unlock_irqrestore(&cmos_lock, &flags);
		return -EIO;
	}

	do {
		while((cmos_read_unlocked(CMOS_REG_STATUSA) & CMOS_STATUSA_UIP) && --retries){
			cpu_relax();
		}
		if(retries == 0){
			spin_unlock_irqrestore(&cmos_lock, &flags);
			return -ETIMEDOUT;
		}

		status_b = cmos_read_unlocked(CMOS_REG_STATUSB);
		second   = cmos_read_unlocked(CMOS_REG_SECONDS);
		minute   = cmos_read_unlocked(CMOS_REG_MINUTES);
		hour     = cmos_read_unlocked(CMOS_REG_HOURS);
		day      = cmos_read_unlocked(CMOS_REG_DAY);
		month    = cmos_read_unlocked(CMOS_REG_MONTH);
		year     = cmos_read_unlocked(CMOS_REG_YEAR);
		if(cmos_century_reg != 0){
			century = cmos_read_unlocked(cmos_century_reg);
		}
	} while(
		second  != cmos_read_unlocked(CMOS_REG_SECONDS) ||
		minute  != cmos_read_unlocked(CMOS_REG_MINUTES) ||
		hour    != cmos_read_unlocked(CMOS_REG_HOURS)   ||
		day     != cmos_read_unlocked(CMOS_REG_DAY)     ||
		month   != cmos_read_unlocked(CMOS_REG_MONTH)   ||
		year    != cmos_read_unlocked(CMOS_REG_YEAR)    ||
		(cmos_century_reg != 0 && century != cmos_read_unlocked(cmos_century_reg))
	);

	spin_unlock_irqrestore(&cmos_lock, &flags);

	hour_pm = hour & 0x80;

	if(!(status_b & CMOS_STATUSB_BINARY)){
		second = bcd_to_bin(second);
		minute = bcd_to_bin(minute);
		hour   = bcd_to_bin(hour & 0x7F);
		day    = bcd_to_bin(day);
		month  = bcd_to_bin(month);
		year   = bcd_to_bin(year);
		if(cmos_century_reg != 0){
			century = bcd_to_bin(century);
		}
	}else{
		hour &= 0x7F;
	}

	if(!(status_b & CMOS_STATUSB_24H)){
		if(hour_pm && hour < 12){
			hour += 12;
		}else if(!hour_pm && hour == 12){
			hour = 0;
		}
	}

	if(cmos_century_reg == 0 || century < 19 || century > 21){
		century = (year < 70) ? 20 : 19;
	}

	uint16_t full_year = (uint16_t)(century * 100 + year);

	if(month < 1 || month > 12 ||
	day < 1   || day > days_in_month(full_year, month) ||
	hour > 23 || minute > 59 || second > 59){
		return -EINVAL;
	}

	tm->second = second;
	tm->minute = minute;
	tm->hour   = hour;
	tm->day    = day;
	tm->month  = month;
	tm->year   = full_year;

	return 0;
}

time_ns_t rtc_time_to_unix_ns(const struct rtc_time* tm){
	uint64_t seconds = rtc_days_since_epoch(tm) * 86400ULL;
	seconds += (uint64_t)tm->hour * 3600ULL;
	seconds += (uint64_t)tm->minute * 60ULL;
	seconds += (uint64_t)tm->second;

	return seconds * NSEC_PER_SEC;
}

static void rtc_sync_kernel_clock(void){
	struct rtc_time tm;
	if(rtc_read(&tm) == 0){
		clock_set_realtime_ns(rtc_time_to_unix_ns(&tm));

		printk(
			"RTC: sync clock to %04u-%02u-%02u %02u:%02u:%02u\n",
			tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second
		);

	}else{
		printk("RTC: sync clock failed\n");
	}
}

static int rtc_probe(struct device* dev){
	struct resource *resource = device_get_resource(dev, RESOURCE_TYPE_IO, 0);
	if(resource){
		cmos_io_region.type = IO_TYPE_PIO;
		cmos_io_region.pio_base = resource->io.base;
		goto found;
	}

	resource = device_get_resource(dev, RESOURCE_TYPE_MEMORY, 0);
	if(resource) {
		cmos_io_region.type = IO_TYPE_MMIO;
		cmos_io_region.mmio_base = resource->range.base;
		goto found;
	}

	return -EINVAL;
found:
	rtc_sync_kernel_clock();
	return 0;
}

static const struct acpi_device_id rtc_ids[] = {
	{"PNP0B00"},
	{"PNP0B0F"},
	{},
};

static struct acpi_driver rtc_driver = {
	.driver = {
		.name = "rtc",
		.probe = rtc_probe,
	},
	.id_table = rtc_ids,
};

static __init int rtc_driver_register(void){
	const struct acpi_fadt* fadt = (void*)acpi_find_table(ACPI_FADT_SIGNATURE);

	if (fadt) {
		cmos_century_reg = fadt->century;
		uint16_t flags = fadt_get_boot_arch_flags(fadt);

		if (!(flags & FADT_BOOT_CMOS_RTC_NOT_PRESENT)) {
			cmos_io_region.type = IO_TYPE_PIO;
			cmos_io_region.pio_base = 0x70;
			rtc_sync_kernel_clock();
			return 0;
		}
	}

	return acpi_register_driver(&rtc_driver);
}

device_initcall(rtc_driver_register);