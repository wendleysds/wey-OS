#include <arch/i386/timer.h>
#include <arch/i386/idt.h>
#include <io/ports.h>
#include <def/config.h>
#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182

static uint64_t ticks = 0;

static void _timer_iqr_handler(struct InterruptFrame* frame){
	ticks++;
	outb(0x20, 0x20);
}

void pit_init(uint32_t frequency) {
	uint16_t divisor = (uint16_t)(PIT_FREQUENCY / frequency);

	outb(PIT_COMMAND, 0x36);                   // Channel 0, lobyte/hibyte
	outb(PIT_CHANNEL0, divisor & 0xFF);        // low end
	outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); // high end

	idt_register_callback(0x20, _timer_iqr_handler);
}

