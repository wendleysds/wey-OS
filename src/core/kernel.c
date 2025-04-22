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
  terminal_write("\nInitializing kernel...\n", TERMINAL_DEFAULT_COLOR);

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);

	terminal_write("Loading Global Descriptor Table (GDT)...", TERMINAL_DEFAULT_COLOR);
	gdt_load(gdt, sizeof(gdt) - 1);
	terminal_write(" OK\n", 0x0A);

	terminal_write("Loading Interrupt Descriptor Table (IDT)...", TERMINAL_DEFAULT_COLOR);
	init_idt();
	terminal_write(" OK\n", 0x0A);

	// TSS Setup
	memset(&tss, 0x00, sizeof(tss));
	tss.esp0 = 0x6000000;
	tss.ss0 = KERNEL_DATA_SELECTOR;

	terminal_write("Loading Task State Segment (TSS)...", TERMINAL_DEFAULT_COLOR);
	tss_load(0x28);
	terminal_write(" OK\n", 0x0A);

	panic("Kernel no implemented!");
}

void panic(const char* msg){
	terminal_write("\nPanic!\n  ", TERMINAL_DEFAULT_COLOR);
	terminal_write(msg, TERMINAL_DEFAULT_COLOR);
	while (1);
}

