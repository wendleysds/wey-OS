#include <kernel/clock.h>
#include <def/errno.h>
#include <lib/div64.h>
#include <lib/list.h>
#include <mm/kheap.h>
#include <sync/spinlock.h>

/*
 * TODO:
 * Make SMP-safe readings.
 * Callbacks currently run under clockevent_lock.
 */

static volatile tick_t clock_ticks;
static volatile time_ns_t clock_monotonic_ns;
static volatile int64_t clock_realtime_offset_ns;
static time_ns_t clock_tick_ns;
static time_ns_t clock_tick_ns_remainder;
static time_ns_t clock_remainder_accum;
static uint32_t clock_hz;

static spinlock_t clockevent_lock;
static spinlock_t clocksource_lock;

struct clockevent_listener {
	void (*handler)(void* data);
	void* data;
	struct list_head node;
};

static LIST_HEAD(clockevent_listeners);
static const struct clockevent* current_clockevent;
static const struct clocksource* current_clocksource;

static time_ns_t _clocksource_read_default(void* data){
	return clock_monotonic_ns;
}

static const struct clocksource default_clocksource = {
	.name = "software-tick",
	.read_ns = _clocksource_read_default,
	.data = 0x0,
};

int clock_init(uint32_t timer_hz){
	if(!timer_hz){
		return -EINVAL;
	}

	spinlock_init(&clockevent_lock);
	spinlock_init(&clocksource_lock);
	INIT_LIST_HEAD(&clockevent_listeners);

	clock_tick_ns = NSEC_PER_SEC;
	clock_tick_ns_remainder = do_div(clock_tick_ns, timer_hz);
	clock_remainder_accum = 0;
	clock_hz = timer_hz;
	clock_ticks = 0;
	clock_monotonic_ns = 0;
	clock_realtime_offset_ns = 0;
	current_clocksource = &default_clocksource;
	current_clockevent = 0x0;

	return SUCCESS;
}

void clock_set_realtime_ns(time_ns_t time_ns){
	clock_realtime_offset_ns = (int64_t)time_ns - (int64_t)clock_monotonic_ns;
}

static void clock_tick(void){
	clock_ticks++;
	clock_monotonic_ns += clock_tick_ns;

	if(clock_tick_ns_remainder){
		clock_remainder_accum += clock_tick_ns_remainder;

		if(clock_remainder_accum >= clock_hz){
			clock_monotonic_ns++;
			clock_remainder_accum -= clock_hz;
		}
	}
}

tick_t clock_get_ticks(void){
	return clock_ticks;
}

time_ns_t clock_get_monotonic_ns(void){
	const struct clocksource* source = current_clocksource;
	if(!source){
		return clock_monotonic_ns;
	}

	return source->read_ns(source->data);
}

time_ns_t clock_get_realtime_ns(void)
{
    return (time_ns_t)(
        (int64_t)clock_get_monotonic_ns() +
        clock_realtime_offset_ns
    );
}

int clocksource_register(const struct clocksource* source){
	if(!source || !source->read_ns){
		return -EINVAL;
	}

	spin_lock(&clocksource_lock);
	current_clocksource = source;
	spin_unlock(&clocksource_lock);

	return SUCCESS;
}

int clockevent_register(const struct clockevent* event){
	if(!event || !event->start_periodic){
		return -EINVAL;
	}

	spin_lock(&clockevent_lock);
	current_clockevent = event;
	spin_unlock(&clockevent_lock);

	return SUCCESS;
}

int clockevent_start_periodic(uint32_t hz){
	const struct clockevent* event = current_clockevent;
	if(!event || !event->start_periodic){
		return -EINVAL;
	}

	return event->start_periodic(event->data, hz);
}

void clockevent_stop(void){
	const struct clockevent* event = current_clockevent;
	if(event && event->stop){
		event->stop(event->data);
	}
}

int clockevent_register_listener(void (*handler)(void* data), void* data){
	if(!handler){
		return -EINVAL;
	}

	struct clockevent_listener* listener = kmalloc(sizeof(*listener));
	if(!listener){
		return -ENOMEM;
	}

	listener->handler = handler;
	listener->data = data;
	INIT_LIST_HEAD(&listener->node);

	spin_lock(&clockevent_lock);
	list_add_tail(&listener->node, &clockevent_listeners);
	spin_unlock(&clockevent_lock);

	return SUCCESS;
}

void clockevent_fire(void){
	struct list_head *pos, *next;

	clock_tick();

	spin_lock(&clockevent_lock);
	for(pos = clockevent_listeners.next; pos != &clockevent_listeners; pos = next){
		next = pos->next;
		struct clockevent_listener* listener = list_entry(pos, struct clockevent_listener, node);
		listener->handler(listener->data);
	}
	spin_unlock(&clockevent_lock);
}
