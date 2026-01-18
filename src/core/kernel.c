#include <def/init.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>

#include <uapi/headers.h>

#define __hlt do {__asm__ volatile("hlt");}while(1)

extern __init void setup_arch();

extern struct boot_header boot_header;

__no_return void kmain(){
	/*terminal_init();
	terminal_clear();*/

	setup_arch();

	interrupts_disable();

	// vt_init();

	page_init(boot_header.e820_table, boot_header.e820_entries_count);

	slab_init();

	//mmu_init();
	
	// tty_init();

	char* test = slab_alloc(32);
	test[0] = 'O';
	test[1] = 'K';
	
	volatile char* mem = (volatile char*)0xB8000;
	mem[0] = test[0];
	mem[1] = 0xF;
	mem[2] = test[1];
	mem[3] = 0xF;

	while(1) __asm__ volatile ("hlt");

	mmu_init();

	interrupts_enable();

	__builtin_unreachable();
}
