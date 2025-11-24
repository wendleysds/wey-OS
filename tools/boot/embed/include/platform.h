#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <platform/fat.h>
#include <def/compile.h>
#include <stdint.h>
#include <file.h>
#include <mem.h>
j
int platform_init();
int platform_disk_init();

void platform_putchar(char c);
int platform_get_memory_map(struct e820_entry* table, int tablesize, uint32_t *count);

void __no_return platform_die();
void __no_return platform_reboot();

fat_info_t* platform_parse_fat(struct disk* disk);
struct file* platform_open_file(fat_info_t* fat_info, const char* path);

#endif
