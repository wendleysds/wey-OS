#include <wey/interrupt.h>
#include <wey/printk.h>
#include <lib/stdio.h>
#include <def/compile.h>

void __no_return panic(const char* fmt, ...){
	interrupts_disable();

	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	vsprintf(buffer,fmt, args);
	va_end(args);

	printk("\nPanic!\n    %s", buffer);

	while(1){
		__asm__ volatile ("hlt");
	}

	__builtin_unreachable();
}
