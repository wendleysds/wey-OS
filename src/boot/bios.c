#include <boot/bios.h>
#include <stdarg.h>
#include <stdint.h>

extern void bios_int10h(const struct biosreg *ireg, struct biosreg *oreg);

extern void bios_putchar(char c);

static void itoa(int value, char *result, int base)
{
	char *digits = "0123456789ABCDEF";
	char buffer[32];
	int i = 0, j = 0;

	if (value < 0 && base == 10)
	{
		result[j++] = '-';
		value = -value;
	}

	do
	{
		buffer[i++] = digits[value % base];
		value /= base;
	} while (value);

	while (i > 0)
	{
		result[j++] = buffer[--i];
	}

	result[j] = '\0';
}

void bios_printf(const char *fmt, ...)
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
				bios_putchar(va_arg(args, int));
				break;
			case 's':
				for (char *s = va_arg(args, char *); *s; s++)
					bios_putchar(*s);
				break;
			case 'd':
				itoa(va_arg(args, int), buffer, 10);
				for (char *s = buffer; *s; s++)
					bios_putchar(*s);
				break;
			case 'x':
				itoa(va_arg(args, int), buffer, 16);
				for (char *s = buffer; *s; s++)
					bios_putchar(*s);
				break;
			default:
				bios_putchar('%');
				bios_putchar(*ptr);
				break;
			}
		}
		else
		{
			bios_putchar(*ptr);
		}
	}

	va_end(args);
}

/*
 * bios interrupt call
 */
void bios_intcall(int int_no, const struct biosreg *inReg, struct biosreg *outReg)
{
	switch (int_no)
	{
	case 0x10:
		bios_int10h(inReg, outReg);
		return;
	default:
		bios_printf("bios int 0x%x not defined!\r\n", int_no);
		return;
	}
}
