#include "boot.h"
#include <def/compile.h>
#include <stdint.h>

struct boot_header* header __section(".header");

/*
 * Main module for the real-mode kernel code
 */

// TODO: Implement missing
void main(){

	// Check CPU

	// Check Memory

	setup_video();

	while(1) asm volatile ("hlt");
}

