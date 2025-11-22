#include <platform.h>
#include <utils.h>
#include <stdarg.h>
#include <stdint.h>

#define TO_LOWER(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 32 : (c))

static void itoa_base(long value, char* result, int base, int is_unsigned){
	const char* digits = "0123456789ABCDEF";
	char buffer[32];
	int i = 0, j = 0;

	if (!is_unsigned && base == 10 && value < 0){
		result[j++] = '-';
		value = -value;
	}

	unsigned long uvalue = (unsigned long)value;

	do {
		buffer[i++] = digits[uvalue % base];
		uvalue /= base;
	} while(uvalue);

	while(i > 0){
		result[j++] = buffer[--i];
	}

	result[j] = '\0';
}

void printf(const char* restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[32];

	for (const char *ptr = fmt; *ptr; ptr++)
	{
		if (*ptr == '%')
		{
			ptr++;
			switch (*ptr)
			{
			case 'c':
				platform_putchar(va_arg(args, int));
				break;
			case 's':
				for (char *s = va_arg(args, char *); *s; s++)
					platform_putchar(*s);
				break;
			case 'u':
			case 'd':
				itoa_base(va_arg(args, int), buffer, 10, *ptr == 'u');
				for (char *s = buffer; *s; s++)
					platform_putchar(*s);
				break;
			case 'p':
			case 'x':
			case 'X':
				itoa_base(va_arg(args, int), buffer, 16, 0);
				for (char *s = buffer; *s; s++)
					platform_putchar(*ptr == 'x' ? TO_LOWER(*s) : *s);
				break;
			default:
				platform_putchar('%');
				platform_putchar(*ptr);
				break;
			}
		}
		else
		{
			if(*ptr == '\n')
				platform_putchar('\r');

			platform_putchar(*ptr);
		}
	}

	va_end(args);
}

