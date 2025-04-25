#include <drivers/terminal.h>
#include <core/kernel.h>

void kmain(){
	terminal_init();
	terminal_cursor_disable();
	
	terminal_clear();
	terminal_write(TERMINAL_DEFAULT_COLOR, "Hello, World!\n");
	terminal_write(TERMINAL_DEFAULT_COLOR, "Welcome to my OS! :D\n");
	terminal_write(TERMINAL_DEFAULT_COLOR, "\n--------------------------\n");
	
	init_kernel();

	while(1);
}


