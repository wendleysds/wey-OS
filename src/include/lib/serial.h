#ifndef _SERIAL_H
#define _SERIAL_H

#include <io/ports.h>
#include <lib/utils.h>
#include <stdarg.h>

#define COM1 0x3F8

void serial_init() {
	outb(COM1 + 1, 0x00);    // Disable interrupts
	outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(COM1 + 0, 0x03);    // Set divisor to 3 (38400 baud)
	outb(COM1 + 1, 0x00);    //                  (high byte)
	outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int serial_is_transmit_empty() {
	return inb(COM1 + 5) & 0x20;
}

void serial_putchar(char a) {
	while (serial_is_transmit_empty() == 0);
	outb(COM1, a);
}

void serial_printf(const char* restrict format, ...){
	va_list args;
	va_start(args, format);
	char buffer[32];
	for (const char *ptr = format; *ptr; ptr++) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
				case 'c':
					serial_putchar(va_arg(args, int));
					break;
				case 's':
					for (char *s = va_arg(args, char*); *s; s++)
						serial_putchar(*s);
					break;
				case 'd':
					itoa(va_arg(args, int), buffer, 10);
					for (char *s = buffer; *s; s++)
						serial_putchar(*s);
					break;
				case 'x':
					itoa(va_arg(args, int), buffer, 16);
					for (char *s = buffer; *s; s++)
						serial_putchar(*s);
					break;
				default:
					serial_putchar('%');
					serial_putchar(*ptr);
					break;
			}
		} else {
			serial_putchar(*ptr);
		}
	}

	va_end(args);
}

#endif
