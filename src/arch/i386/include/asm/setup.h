#ifndef _X86_SETUP_H
#define _X86_SETUP_H

#include <stddef.h>
#include <uapi/headers.h>

extern struct boot_header boot_header;

#define LOWMEMSIZE() (0x9f000)

extern unsigned long _brk_end;
void* extend_brk(size_t size, size_t align);

#define RESERVE_BRK(name, size)					\
	__section(".bss..brk") __aligned(1) __used	\
	static char __brk_##name[size]

#endif