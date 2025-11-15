#include <def/init.h>
#include <def/config.h>
#include <asm/tss.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <arch/i386/pic.h>
#include <lib/string.h>

static struct TSS tss;
static struct gdt_entry gdt[TOTAL_GDT_SEGMENTS];
static struct gdt_descriptor gdt_descriptor;

__init void setup_arch(){
	struct gdt_entry _gdt[TOTAL_GDT_SEGMENTS] = {
		GDT_ENTRY(0, 0, 0, 0),                              // Null
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING0, GDT_FLAGS_DEFAULT), // Kernel code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING0, GDT_FLAGS_DEFAULT), // Kernel data
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING3, GDT_FLAGS_DEFAULT), // User code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING3, GDT_FLAGS_DEFAULT), // User data
		GDT_ENTRY((unsigned long)&tss, sizeof(tss)-1, GDT_TSS_AVAILABLE, GDT_FLAG_32BIT) // TSS
	};

	memcpy(gdt, _gdt, sizeof(gdt));

	gdt_descriptor.addr = (uintptr_t)gdt;
	gdt_descriptor.size = sizeof(gdt) - 1;

	gdt_load(&gdt_descriptor);

	memset(&tss, 0x0, sizeof(tss));
	tss.ss0 = KERNEL_DATA_SELECTOR;
	tss.esp0 = PROC_KERNEL_STACK_VIRTUAL_TOP;
	tss.iopb = sizeof(tss);

	tss_load(0x28); // TSS segment is the 6th entry in the GDT (index 5), so selector is 0x28

	pic_init(TIMER_FREQUENCY);

	idt_init();
}