#ifndef _X86_RTC_H
#define _X86_RTC_H

#include <stdint.h>
#include <sys/types.h>

struct rtc_time {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

int rtc_read(struct rtc_time* tm);
time_ns_t rtc_time_to_unix_ns(const struct rtc_time* tm);

#endif
