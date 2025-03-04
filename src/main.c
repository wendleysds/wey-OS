#include "terminal/terminal.h"

void kmain(){
	terminal_clear();
	terminal_write("Hello, World!\n\n", 0x0F);
	terminal_write("Welcome to my OS! :D\n", 0x0F);

	while(1);
}


