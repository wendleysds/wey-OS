#include "terminal/terminal.h"
#include "kernel/kernel.h"

void kmain(){
	terminal_init();
	terminal_cursor_disable();
	
	terminal_clear();
	terminal_write("Hello, World!\n\n", 0x0F);
	terminal_write("Welcome to my OS! :D\n", 0x0F);
	
	init_kernel();

	while(1);
}


