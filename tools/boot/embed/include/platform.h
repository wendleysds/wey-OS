#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <platform/fat.h>
#include <def/compile.h>
#include <stdint.h>
#include <file.h>
#include <mem.h>

int platform_init();
int platform_disk_init();

int platform_timeout(int secs);
void platform_putchar(char c);
int platform_getchar();

void platform_init_video();
void platform_putchar(char c);
void platform_clear_screen();
void platform_set_cursor(uint8_t x, uint8_t y);
void platform_get_cursor(uint8_t* x, uint8_t* y);
void platform_get_resolution(uint8_t* x, uint8_t* y);

int platform_get_memory_map(struct e820_entry* table, int tablesize, uint32_t *count);

void __no_return platform_die();
void __no_return platform_reboot();

fat_info_t* platform_parse_fat(struct disk* disk);
struct file* platform_open_file(fat_info_t* fat_info, const char* path);

#endif
