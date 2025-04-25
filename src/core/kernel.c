#include <core/kernel.h>
#include <drivers/terminal.h>
#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/tss.h>
#include <lib/mem.h>

#include <config/config.h>

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

void init_kernel(){
  terminal_write(TERMINAL_DEFAULT_COLOR, "\nInitializing kernel...\n");

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);

	terminal_write(TERMINAL_DEFAULT_COLOR, "Loading Global Descriptor Table (GDT)...");
	gdt_load(gdt, sizeof(gdt) - 1);
	terminal_write(0x0A, " OK\n");

	terminal_write(TERMINAL_DEFAULT_COLOR, "Initializing Interrupt Descriptor Table (IDT)...");
	init_idt();
	terminal_write(0x0A, " OK\n");

	terminal_write(TERMINAL_DEFAULT_COLOR, "Initializing PIT...");
	terminal_write(0x0A, " OK\n");

	/*
	// TSS Setup
	memset(&tss, 0x00, sizeof(tss));
	tss.esp0 = 0x6000000;
	tss.ss0 = KERNEL_DATA_SELECTOR;

	terminal_write(TERMINAL_DEFAULT_COLOR, "Loading Task State Segment (TSS)...");
	tss_load(0x28);
	terminal_write(0x0A, " OK\n");
	*/

	panic("Kernel no implemented!");
}

void panic(const char* msg){
	terminal_write(TERMINAL_DEFAULT_COLOR, "\nPanic!\n  %s", msg);
	while (1);
}

