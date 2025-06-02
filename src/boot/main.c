#include <boot/bios.h>
#include <boot/video.h>
#include <stdint.h>

/*
 * Main module for the real-mode kernel code
 */

extern void go_to_protect_mode();

// TODO: Implement missing
void main(){

	// Check CPU

	// Check Memory

	setup_video();

	go_to_protect_mode();
}

