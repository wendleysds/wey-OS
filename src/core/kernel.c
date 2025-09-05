#include <core/kernel.h>
#include <core/sched.h>

#include <drivers/terminal.h>

#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/pic.h>
#include <arch/i386/tss.h>

#include <lib/mem.h>
#include <lib/utils.h>
#include <lib/string.h>

#include <mmu.h>
#include <memory/kheap.h>

#include <def/config.h>
#include <def/err.h>
#include <stdint.h>

#include <fs/vfs.h>

#define _INIT_PANIC(msg, pmsg, init_func) \
	terminal_cwrite(0x00FF00, "[   ] "); \
	terminal_write(msg); \
	if(IS_STAT_ERR((res = init_func))){ \
		terminal_cwrite(0xFF0000, "\r[%d]\n", res); \
		panic(pmsg); \
	} else \
	terminal_cwrite(0x00FF00, "\r[ 0 ]\n")

#define _INIT(msg, init_func) \
	terminal_cwrite(0x00FF00, "[   ] "); \
	terminal_write(msg); \
	init_func;\
	terminal_cwrite(0x00FF00, "\r[ 0 ]\n")

#define _INIT_MSGF(init_func, msg, ...) \
	terminal_cwrite(0x00FF00, "[   ] "); \
	terminal_write(msg, __VA_ARGS__); \
	init_func; \
	terminal_cwrite(0x00FF00, "\r[ 0 ]\n")

/* 
 * Main module for the protected-mode kernel code
 */ 

extern void load_drivers();

static struct TSS tss;
static struct GDT gdt[TOTAL_GDT_SEGMENTS];

// Global Descriptor Table config
struct GDT_Structured gdt_ptr[TOTAL_GDT_SEGMENTS] = {
	{.base = 0x00, .limit = 0x00, .type = 0x00, .flags = 0x0},                     // NULL Segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0x9a, .flags = 0xC},                  // Kernel code segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0x92, .flags = 0xC},                  // Kernel data segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0xf8, .flags = 0xC},                  // User code segment
	{.base = 0x00, .limit = 0xFFFFF, .type = 0xf2, .flags = 0xC},                  // User data segment
	{.base = (uint32_t)&tss, .limit = sizeof(tss) - 1, .type = 0xE9, .flags = 0x0} // TSS Segment
};

extern void pcb_set(struct Task* t);
static int8_t kernel_userland_init(){
	struct Process* kernel_p = process_create("init", 0, 0, 0, 0, 0);
	if(IS_ERR(kernel_p)){
		return PTR_ERR(kernel_p);
	}

	struct Task* kernel_t = task_new(kernel_p, (void*)1);
	if(IS_ERR(kernel_t)){
		process_terminate(kernel_p);
		return PTR_ERR(kernel_t);
	}

	process_add_task(kernel_p, kernel_t);

	int res;

	pcb_set(kernel_t);
	scheduler_add_task(kernel_t); // Prepare task inside the scheduler whem ready*/

	const char* bin1args[] = { "/bin/bash", NULL };
	const char* bin2args[] = { "/bash", NULL };

	const char* envp[] = { "HOME=/home", "PATH=/bin", NULL };

	if((res = kernel_exec(bin1args[0], bin1args, envp)) == SUCCESS){
		return res;
	}

	if(res != FILE_NOT_FOUND){
		return res;
	}

	warning("Bash not found in '%s' trying '%s'\n", bin1args[0], bin2args[0]);

	return kernel_exec(bin2args[0], bin2args, envp);
}

static void _load_tss(){
	memset(&tss, 0x0, sizeof(tss));
	tss.ss0 = KERNEL_DATA_SELECTOR;
	tss.esp0 = 0x600000;
	tss.iopb = sizeof(tss);

	tss_load(0x28); // TSS segment is the 6th entry in the GDT (index 5), so selector is 0x28
}

void kmain(){
	terminal_init();
	terminal_clear();

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);

	int res;

	_INIT(
		"Loading Global Descriptor Table (GDT)", 
		gdt_load(gdt, sizeof(gdt) - 1)
	);

	_INIT_MSGF(
		pic_init(TIMER_FREQUENCY), 
		"Initializing PIT(IRQ 0x20). Divisor = %d", 
		TIMER_FREQUENCY
	);

	_INIT(
		"Initializing Interrupt Descriptor Table (IDT)", 
		init_idt()
	);

	_INIT(
		"Loading Task State Segment (TSS)", 
		_load_tss();
	);

	disable_interrupts();
	
	_INIT_PANIC(
		"Initializing Kernel Heap",
		"Failed to create kernel heap!",
		init_kheap()
	);

	_INIT_PANIC(
		"Initializing Memory Manager Unit",
		"Failed to initializing Memory Manager Unit!",
		mmu_init()
	);

	enable_interrupts();

	_INIT(
		"Initializing Drivers",
		load_drivers()
	);

	_INIT_PANIC(
		"Mounting root",
		"Failed to mount root!",
		vfs_mount(device_get_name("hda"), "/", "vfat")
	);

	_INIT(
		"Initializing Scheduler",
		scheduler_init()
	);

	_INIT(
		"Initializing Syscalls",
		syscalls_init()
	);

	while(1){
		__asm__ volatile ("hlt");
	}

	_INIT_PANIC(
		"Starting userland",
		"userland init failed!",
		kernel_userland_init()
	);

	terminal_clear();

	pcb_set(0x0);
	scheduler_start();

	while(1){
		__asm__ volatile ("hlt");
	}
}

void panic(const char* fmt, ...){
	disable_interrupts();
	terminal_write("\nPanic!\n  ");

	va_list args;
	va_start(args, fmt);
	terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
	va_end(args);

	while(1){
		__asm__ volatile ("hlt");
	}
}

void warning(const char* fmt, ...){
	terminal_cwrite(0xF0FF00, "\nWarning! ");

	va_list args;
	va_start(args, fmt);
	terminal_vwrite(TERMINAL_DEFAULT_COLOR, fmt, args);
	va_end(args);
}
