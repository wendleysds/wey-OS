#include "boot.h"
#include <def/compile.h>
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

	enable_a20();

	// mask all interrupts

	// setup idt

	// setup gdt

	// go to pm mode 
	
	printf("OK");
	while(1) asm volatile ("hlt");
}

