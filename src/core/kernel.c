#include <def/init.h>
#include <drivers/terminal.h>
#include <mm/kheap.h>
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

	init_kheap();

	mmu_init();

	interrupts_enable();

	mmu_mmap(
		NULL,
		(void*)0xB8000,
		(void*)0xD0000000,
		32,
		(MEM_READ | MEM_WRITE | MEM_KERNEL),
		MAP_PRIVATE
	);

	volatile char* vga = (volatile char*)0xD0000000;
	vga[0] = 'O';
	vga[1] = 0xF;
	vga[2] = 'K';
	vga[3] = 0xF;
	
	//terminal_write("ok");

	while(1) __asm__ volatile ("hlt");

	__builtin_unreachable();
}
