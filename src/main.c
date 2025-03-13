#include "terminal/terminal.h"
#include "kernel/kernel.h"

void kmain(){
	terminal_init();
	terminal_cursor_disable();
	
	terminal_clear();
	terminal_write("Hello, World!\n", TERMINAL_DEFAULT_COLOR);
	terminal_write("Welcome to my OS! :D\n", TERMINAL_DEFAULT_COLOR);
	terminal_write("\n--------------------------\n", TERMINAL_DEFAULT_COLOR);
	
	init_kernel();

	while(1);
}


