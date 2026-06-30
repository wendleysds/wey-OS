#include <arch/i386/rtc.h>
#include <def/errno.h>
#include <io/ports.h>
#include <asm/cpu.h>
#include <kernel/clock.h>

#define CMOS_ADDR_PORT   0x70
#define CMOS_DATA_PORT   0x71
#define CMOS_REG_SECONDS 0x00
#define CMOS_REG_MINUTES 0x02
#define CMOS_REG_HOURS   0x04
#define CMOS_REG_DAY     0x07
#define CMOS_REG_MONTH   0x08
#define CMOS_REG_YEAR    0x09
#define CMOS_REG_STATUSA 0x0A
#define CMOS_REG_STATUSB 0x0B
#define CMOS_REG_CENTURY 0x32

static uint8_t cmos_read(uint8_t reg){
	outb(CMOS_ADDR_PORT, 0x80 | reg);
	return inb(CMOS_DATA_PORT);
}

static uint8_t bcd_to_bin(uint8_t value){
	return (value & 0x0F) + ((value >> 4) * 10);
}

static int is_leap_year(uint16_t year){
	return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static uint64_t rtc_days_since_epoch(const struct rtc_time* tm){
	static const uint8_t month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint64_t days = 0;

	for(uint16_t year = 1970; year < tm->year; year++){
		days += is_leap_year(year) ? 366 : 365;
	}

	for(uint8_t month = 1; month < tm->month; month++){
		days += month_days[month - 1];
		if(month == 2 && is_leap_year(tm->year)){
			days++;
		}
	}

	days += tm->day - 1;
	return days;
}

int rtc_read(struct rtc_time* tm){
	if(!tm){
		return -EINVAL;
	}

	uint8_t status_b;
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t century;
	uint8_t hour_pm;

	do {
		while(cmos_read(CMOS_REG_STATUSA) & 0x80){
			cpu_relax();
		}

		status_b = cmos_read(CMOS_REG_STATUSB);
		second = cmos_read(CMOS_REG_SECONDS);
		minute = cmos_read(CMOS_REG_MINUTES);
		hour = cmos_read(CMOS_REG_HOURS);
		day = cmos_read(CMOS_REG_DAY);
		month = cmos_read(CMOS_REG_MONTH);
		year = cmos_read(CMOS_REG_YEAR);
		century = cmos_read(CMOS_REG_CENTURY);
		hour_pm = hour & 0x80;
	} while(
		second != cmos_read(CMOS_REG_SECONDS) ||
		minute != cmos_read(CMOS_REG_MINUTES) ||
		hour != cmos_read(CMOS_REG_HOURS) ||
		day != cmos_read(CMOS_REG_DAY) ||
		month != cmos_read(CMOS_REG_MONTH) ||
		year != cmos_read(CMOS_REG_YEAR)
	);

	if(!(status_b & 0x04)){
		second = bcd_to_bin(second);
		minute = bcd_to_bin(minute);
		hour = bcd_to_bin(hour & 0x7F);
		day = bcd_to_bin(day);
		month = bcd_to_bin(month);
		year = bcd_to_bin(year);
		century = bcd_to_bin(century);
	}

	if(!(status_b & 0x02)){
		if(hour_pm && hour < 12){
			hour += 12;
		}else if(!hour_pm && hour == 12){
			hour = 0;
		}
	}

	if(century == 0x00 || century == 0xFF){
		century = (year < 70) ? 20 : 19;
	}

	tm->second = second;
	tm->minute = minute;
	tm->hour = hour;
	tm->day = day;
	tm->month = month;
	tm->year = (uint16_t)(century * 100 + year);

	return 0;
}

time_ns_t rtc_time_to_unix_ns(const struct rtc_time* tm){
	uint64_t seconds = rtc_days_since_epoch(tm) * 86400ULL;
	seconds += (uint64_t)tm->hour * 3600ULL;
	seconds += (uint64_t)tm->minute * 60ULL;
	seconds += (uint64_t)tm->second;

	return seconds * NSEC_PER_SEC;
}
