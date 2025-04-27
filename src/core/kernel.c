#include <core/kernel.h>

#include <drivers/terminal.h>
#include <drivers/keyboard.h>

#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/tss.h>

#include <lib/mem.h>

#include <memory/kheap.h>

#include <def/config.h>

struct TSS tss;
struct GDT gdt[TOTAL_GDT_SEGMENTS];

struct GDT_Structured gdt_ptr[TOTAL_GDT_SEGMENTS] = {
	{.base = 0x00, .limit = 0x00, .type = 0x00, .flags = 0x0},                     // NULL Segment
  {.base = 0x00, .limit = 0xFFFFF, .type = 0x9a, .flags = 0xC},                  // Kernel code segment
  {.base = 0x00, .limit = 0xFFFFF, .type = 0x92, .flags = 0xC},                  // Kernel data segment
  {.base = 0x00, .limit = 0xFFFFF, .type = 0xf8, .flags = 0xC},                  // User code segment
  {.base = 0x00, .limit = 0xFFFFF, .type = 0xf2, .flags = 0xC},                  // User data segment
  {.base = (uint32_t)&tss, .limit = sizeof(tss) - 1, .type = 0xE9, .flags = 0x0} // TSS Segment
};

void init_log(const char* msg, void (*init_method)(void)){
	terminal_write(TERMINAL_DEFAULT_COLOR, msg);
	init_method();
	terminal_write(0x0A, " OK\n");
}

void init_kernel(){
  terminal_write(0x0A, "\nINITIALIZING KERNEL...\n\n");

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);

	terminal_write(TERMINAL_DEFAULT_COLOR, "Loading Global Descriptor Table (GDT)...");
	gdt_load(gdt, sizeof(gdt) - 1);
	terminal_write(0x0A, " OK\n");

	init_log("Initializing kernel heap...", init_kheap);

	init_log("Initializing Interrupt Descriptor Table (IDT)...", init_idt);

	init_keyboard();
	terminal_cursor_enable();

	terminal_write(0x0A, "\nKERNEL READY!\n\n");

	while(1){
		__asm__ volatile ("hlt");
	}
}

void panic(const char* fmt, ...){
	terminal_write(TERMINAL_DEFAULT_COLOR, "\nPanic!\n  ");

	va_list args;
  va_start(args, fmt);
  terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
  va_end(args);

	while(1){
		__asm__ volatile ("hlt");
	}
}

