#include <core/kernel.h>
#include <core/sched.h>

#include <drivers/terminal.h>
#include <drivers/keyboard.h>

#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/timer.h>
#include <arch/i386/tss.h>

#include <lib/mem.h>
#include <lib/utils.h>
#include <lib/string.h>

#include <mmu.h>

#include <def/config.h>
#include <def/status.h>
#include <stdint.h>

#include <fs/fs.h>

#define _INIT(msg, init_func) \
	terminal_write("[    ] "); \
	terminal_write(msg); \
	init_func;\
	terminal_cwrite(0x00FF00, "\r  OK \n");

#define _INIT_MSGF(msg, init_func, ...) \
	terminal_write("[    ] "); \
	terminal_write(msg, __VA_ARGS__); \
	init_func; \
	terminal_cwrite(0x00FF00, "\r  OK \n");

/* 
 * Main module for the protected-mode kernel code
 */ 

// Kernel Page
static struct PagingDirectory* kernel_directory = 0x0;

struct TSS tss;
struct GDT gdt[TOTAL_GDT_SEGMENTS];

// Global Descriptor Table config
struct GDT_Structured gdt_ptr[TOTAL_GDT_SEGMENTS] = {
	{.base = 0x00, .limit = 0x00, .type = 0x00, .flags = 0x0},                     // NULL Segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0x9a, .flags = 0xC},                  // Kernel code segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0x92, .flags = 0xC},                  // Kernel data segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0xf8, .flags = 0xC},                  // User code segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0xf2, .flags = 0xC},                  // User data segment
	{.base = (uint32_t)&tss, .limit = sizeof(tss) - 1, .type = 0xE9, .flags = 0x0} // TSS Segment
};

void kmain(){
	terminal_init();
	terminal_clear();

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);

	_INIT(
		"Loading Global Descriptor Table (GDT)", 
		gdt_load(gdt, sizeof(gdt) - 1)
	);

	_INIT(
		"Initializing Interrupt Descriptor Table (IDT)", 
		init_idt()
	);

	_INIT(
		"Initializing Kernel Heap", 
		init_kheap()
	);

	_INIT(
		"Initializing Memory Manager Unit",
		mmu_init(&kernel_directory);
	)

	_INIT_MSGF(
		"Initializing PIT(IRQ 0) with %d.0hz", 
		pit_init(TIMER_FREQUENCY), 
		TIMER_FREQUENCY
	);

	_INIT(
		"Initializing File System", 
		fs_init()
	);

	_INIT(
		"Initializing Scheduler",
		scheduler_init();
	)

	terminal_cwrite(0x00FF00, "\nKERNEL READY!\n");

	//terminal_clear();
		
	// Start drivrers
	//init_keyboard();

	char* file = "/home/text.txt";
	int fd = open(file, O_RDWR | O_CREAT);

	if(fd < 0){
		panic("Error openinig %s: %d\n", file, fd);
	}

	int bytes = 0;
	if((bytes = write(fd, "Hello World!\n", 13)) < 0){
		panic("Error writing to %s: %d\n", file, bytes);
	}

	lseek(fd, 0, SEEK_SET);

	char buffer[64];
	if((bytes = read(fd, buffer, sizeof(buffer) - 1)) < 0){
		panic("Error reading from %s: %d\n", file, bytes);
	}

	buffer[bytes] = '\0';
	terminal_cwrite(0x00FF00, "\nContent: ");
	terminal_write(buffer);

	close(fd);

	// Main loop
	while(1){
		__asm__ volatile ("hlt");
	}
}

void panic(const char* fmt, ...){
	terminal_write("\nPanic!\n  ");

	va_list args;
	va_start(args, fmt);
	terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
	va_end(args);

	while(1){
		__asm__ volatile ("hlt");
	}
}

void kernel_page(){
	if(!kernel_directory){
		return;
	}

	kernel_registers();
	paging_switch(kernel_directory);
}