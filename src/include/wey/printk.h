#ifndef _PRINT_KERNEL_H
#define _PRINT_KERNEL_H

typedef void (*printk_echo_function)(const char *s, int length);

void printk_show_buffer();

void printk_set_echo(printk_echo_function f);
int printk(const char* restrict fmt, ...);

#endif
