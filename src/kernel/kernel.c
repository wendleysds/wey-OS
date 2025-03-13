#include "kernel.h"
#include "../terminal/terminal.h"
#include "../gdt/gdt.h"
#include "../idt/idt.h"
#include "../task/tss.h"
#include "../memory/memory.h"

#include "../config.h"

struct TSS tss;
struct GDT gdt[TOTAL_GDT_SEGMENTS];
struct GDT_Structured gdt_ptr[TOTAL_GDT_SEGMENTS] = {
		{.base = 0x00, .limit = 0x00, .type = 0x00},                 // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},           // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},           // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},           // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}   // TSS Segment
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

		panic("Kernel not implemented alery!");
}

void panic(const char* msg){
		terminal_write("\nPanic!\n  ", TERMINAL_DEFAULT_COLOR);
    terminal_write(msg, TERMINAL_DEFAULT_COLOR);
    while (1);
}

void kernel_page(){

}
