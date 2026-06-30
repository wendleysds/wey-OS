#ifndef _KERNEL_CLOCK_H
#define _KERNEL_CLOCK_H

#include <def/compile.h>
#include <stdint.h>
#include <sys/types.h>

#define NSEC_PER_SEC 1000000000ULL

struct clocksource {
	const char* name;
	time_ns_t (*read_ns)(void* data);
	void* data;
};

struct clockevent {
	const char* name;
	int (*start_periodic)(void* data, uint32_t hz);
	void (*stop)(void* data);
	void* data;
};

int clock_init(uint32_t timer_hz);
void clock_set_time_ns(time_ns_t time_ns);
void clock_set_realtime_ns(time_ns_t time_ns);

tick_t clock_get_ticks(void);
time_ns_t clock_get_monotonic_ns(void);
time_ns_t clock_get_realtime_ns(void);

int clocksource_register(const struct clocksource* source);

int clockevent_register(const struct clockevent* event);
int clockevent_start_periodic(uint32_t hz);
void clockevent_stop(void);

int clockevent_register_listener(void (*handler)(void* data), void* data);
void clockevent_fire(void);

#endif
