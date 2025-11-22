#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <def/compile.h>
#include <mem.h>
#include <stdint.h>

int platform_init();

void platform_putchar(char c);
int platform_get_memory_map(struct e820_entry* table, int tablesize, uint32_t *count);

void __no_return platform_die();
void __no_return platform_reboot();

#endif