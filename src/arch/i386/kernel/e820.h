#ifndef _x86_E820_H
#define _x86_E820_H

#include <stddef.h>
#include <uapi/headers.h>

void e820_init(struct e820_entry* entries, size_t length);
void e820_print();
void e820_sort();
size_t e820_end_ram_pfn(size_t limit_pfn);

#endif
