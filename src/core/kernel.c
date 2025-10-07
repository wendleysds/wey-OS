#include <wey/panic.h>
#include <wey/sched.h>
#include <wey/pid.h>
#include <wey/syscall.h>

#include <drivers/terminal.h>

#include <arch/i386/gdt.h>
#include <arch/i386/idt.h>
#include <arch/i386/pic.h>
#include <arch/i386/tss.h>

#include <lib/utils.h>
#include <lib/string.h>

#include <wey/mmu.h>
#include <mm/kheap.h>

#include <def/config.h>
#include <def/err.h>
#include <stdint.h>

#include <wey/vfs.h>

#define _INIT_PANIC(msg, pmsg, init_func) \
	terminal_cwrite(0xFFFF00, "[...] "); \
	terminal_write(msg); \
	if(IS_STAT_ERR((res = init_func))){ \
		terminal_cwrite(0xFF0000, "\n[%d] %s\n", res, msg); \
		panic(pmsg); \
	} else \
	terminal_cwrite(0x00FF00, "\n[ 0 ] "); \
	terminal_write("%s\n", msg)

/* 
 * Main module for the protected-mode kernel code
 */ 

extern void load_drivers();
extern void pcb_set(struct Task* t);

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

static const char* argv_init[] = { NULL, NULL };
static const char* envp_init[] = { "HOME=/", "PATH=/bin", NULL };

static int try_run_init(const char* filename){
	argv_init[0] = filename;

	terminal_write("Running ");
	terminal_cwrite(0xFF1A1A, "%s ", filename);
	terminal_write("as init process ", filename);

	argv_init[0] = filename;

	int res = kernel_exec(filename, argv_init, envp_init);
	terminal_write("%d\n", res);

	return res;
}

static int8_t kernel_prepare_userland(){
	struct Process* kernel_p = process_create("init", 0, 0, 0, 0, 0);
	if(IS_ERR(kernel_p)){
		return PTR_ERR(kernel_p);
	}

	struct Task* kernel_t = task_new(kernel_p, (void*)1);
	if(IS_ERR(kernel_t)){
		process_terminate(kernel_p);
		return PTR_ERR(kernel_t);
	}

	kfree(kernel_t->userStack);
	kfree(kernel_t->kernelStack);

	pcb_set(kernel_t); // Set current for the exec replace
	scheduler_add_task(kernel_t); // Prepare task inside the scheduler whem ready

	return SUCCESS;
}

static void _load_tss(){
	memset(&tss, 0x0, sizeof(tss));
	tss.ss0 = KERNEL_DATA_SELECTOR;
	tss.esp0 = PROC_KERNEL_STACK_VIRTUAL_TOP;
	tss.iopb = sizeof(tss);

	tss_load(0x28); // TSS segment is the 6th entry in the GDT (index 5), so selector is 0x28
}

void kmain(){
	terminal_init();
	terminal_clear();

	// GDT Setup
	memset(gdt, 0x00, sizeof(gdt));
	gdt_structured_to_gdt(gdt, gdt_ptr, TOTAL_GDT_SEGMENTS);
	gdt_load(gdt, sizeof(gdt) - 1);

	pic_init(TIMER_FREQUENCY);

	init_idt();

	_load_tss();

	disable_interrupts();

	int res;
	
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

	load_drivers();

	pid_restart();

	scheduler_init();

	syscalls_init();

	_INIT_PANIC(
		"Mounting root",
		"Failed to mount root!",
		vfs_mount("/dev/hda", "/", "vfat", NULL)
	);

	_INIT_PANIC(
		"Preparing userland",
		"failed!",
		kernel_prepare_userland()
	);

	terminal_putchar('\n', 0);

	if(!try_run_init("/bin/init") ||
	   !try_run_init("/init") ||
	   !try_run_init("/bin/bash") ||
	   !try_run_init("/bash"))
	{
		const char* const* p;
		terminal_write("  with args:\n");
		for (p = argv_init; *p; p++)
			terminal_write("    %s\n", *p);
		terminal_write("  with envp:\n");
		for (p = envp_init; *p; p++)
			terminal_write("    %s\n", *p);

		terminal_clear();

		scheduler_start();

		while(1){
			__asm__ volatile ("hlt"); // Halt until the scheduler pick the task
		}
	}

	panic("No init found!");

	__builtin_unreachable();
}

void __no_return panic(const char* fmt, ...){
	disable_interrupts();
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
