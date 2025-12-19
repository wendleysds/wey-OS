#include "boot.h"
#include <def/compile.h>
#include <def/err.h>
#include <io/ports.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <stdint.h>

struct boot_header boot_header;

/*
 * Main module for the real-mode kernel code
 */

// TODO: Implement missing
void main(){
	memcpy(&boot_header.setup_header, &hdr, sizeof(hdr));
	
	// Check CPU

	detect_memory();

	setup_video();

	if(IS_STAT_ERR(enable_a20())){
		printf("Failed to enable A20 line!\n");
		while(1) asm volatile ("hlt");
	}

	// mask all interrupts
	outb_p(0xFF, 0x21);
	outb_p(0xFF, 0xA1);

	// setup idt
	static const struct gdt_descriptor idt_null = {0, 0};
	asm volatile ("lidt %0" :: "m"(idt_null) : "memory");

	// setup gdt
	static const struct gdt_entry gdt[] = {
		GDT_ENTRY(0, 0, 0, 0),                                    // Null
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING0, GDT_FLAGS_DEFAULT), // Kernel code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING0, GDT_FLAGS_DEFAULT), // Kernel data
	};

	static struct gdt_descriptor gdt_descriptor;
	gdt_descriptor.size = sizeof(gdt) - 1;
	gdt_descriptor.addr = (uintptr_t)gdt + (ds() << 4);

	gdt_load(&gdt_descriptor);

	go_to_protect_mode(
		hdr.code32_start,
		(uint32_t)&boot_header + (ds() << 4)
	);
}

