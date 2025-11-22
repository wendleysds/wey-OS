#include <wey/interrupt.h>
#include <drivers/terminal.h>
#include <def/compile.h>

void __no_return panic(const char* fmt, ...){
	interrupts_disable();
	terminal_write("\nPanic!\n  ");

	va_list args;
	va_start(args, fmt);
	terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
	va_end(args);

	while(1){
		__asm__ volatile ("hlt");
	}

	__builtin_unreachable();
}

void warning(const char* fmt, ...){
	terminal_cwrite(0xF0FF00, "\nWarning! ");

	va_list args;
	va_start(args, fmt);
	terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
	va_end(args);
}