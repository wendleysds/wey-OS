#include <def/init.h>
#include <drivers/terminal.h>
#include <mm/kheap.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern __init void setup_arch();

void kmain(){
	terminal_init();
	terminal_clear();
	
	setup_arch();

	interrupts_disable();

	init_kheap();

	mmu_init();

	interrupts_enable();

	terminal_write("ok");

	while(1) __asm__ volatile ("hlt");

	__builtin_unreachable();
}
