#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <lib/stdio.h>
#include <def/compile.h>

asmlinkage __no_return void panic(const char* fmt, ...){
	interrupts_disable();

	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	printk("\nPanic!\n    %s", buffer);

	while(1){
		__asm__ volatile ("hlt");
	}

	__builtin_unreachable();
}
