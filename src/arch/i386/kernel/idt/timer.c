#include <wey/interrupt.h>
#include <wey/timer.h>
#include <def/init.h>
#include <def/config.h>
#include <lib/string.h>
#include <arch/i386/pic.h>

#define TIMER_CALLBACKS 10

static timer_callback_t timer_callbacks[TIMER_CALLBACKS] = {0x0};
static uint8_t timer_callback_count = 0;

static void _timer_dispatch(struct registers* regs){
	uint8_t count = timer_callback_count;

	for(int i = 0; i < count; i++) {
		if(timer_callbacks[i] != 0x0){
			timer_callbacks[i](regs);
		}
	}
}

int __init timer_init(){
	pic_init(TIMER_FREQUENCY);
	memset(timer_callbacks, 0x0, sizeof(timer_callbacks));
	timer_callback_count = 0;

	interrupt_register(0x20, _timer_dispatch);

	return 0;
}

void timer_register_callback(timer_callback_t callback){
	if(timer_callback_count >= TIMER_CALLBACKS){
		return;
	}

	timer_callbacks[timer_callback_count++] = callback;
}

void timer_unregister_callback(timer_callback_t callback){
	for(int i = 0; i < timer_callback_count; i++){
		if(timer_callbacks[i] == callback){
			// Shift remaining callbacks down
			for(int j = i; j < timer_callback_count - 1; j++){
				timer_callbacks[j] = timer_callbacks[j + 1];
			}

			timer_callbacks[--timer_callback_count] = 0;

			return;
		}
	}
}
