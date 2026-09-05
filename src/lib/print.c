#include <lib/string.h>
#include <lib/div64.h>
#include <def/bits.h>
#include <stdarg.h>

#define ZERODAP_FLAG      BIT(0)
#define SHOW_SIGN_FLAG    BIT(1)
#define PLUS_PREFIX_FLAG  BIT(2)
#define SPACE_PREFIX_FLAG BIT(3)
#define SPECIAL_FLAG      BIT(4)
#define LOWERCASE_FLAG    BIT(5)
#define LEFTPAD_FLAG      BIT(6)

#define __isdigit(c) ((c) >= '0' && (c) <= '9')
#define __tolow(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 32 : (c))

static int skip_stoi(const char **s) {
	int i = 0;
	while (__isdigit(**s)) {
		i = i * 10 + *((*s)++) - '0';
	}
	return i;
}

#define EMIT(c) do { putc((c), ctx); count++; } while(0)

static int print_num(void (*putc)(char c, void *ctx), void *ctx,
					unsigned long long number, int base, int size,
					int precision, int flag) {

	static const char digits[] = "0123456789ABCDEF";
	char tmp[128];
	int i = 0;
	int count = 0;

	if (base < 2 || base > 16) {
		return 0;
	}

	uint8_t lowercase = (flag & LOWERCASE_FLAG) ? 0x20 : 0;

	if (flag & LEFTPAD_FLAG) {
		flag &= ~ZERODAP_FLAG;
	}

	char c = (flag & ZERODAP_FLAG) ? '0' : ' ';
	
	// Handle sign
	uint8_t sign = 0;
	if (flag & SHOW_SIGN_FLAG) {
		if ((signed long long)number < 0) {
			sign = '-';
			number = -(signed long long)number;
			size--;
		} else if (flag & PLUS_PREFIX_FLAG) {
			sign = '+';
			size--;
		} else if (flag & SPACE_PREFIX_FLAG) {
			sign = ' ';
			size--;
		}
	}

	// Handle special prefix (0x, 0, 0b)
	if (flag & SPECIAL_FLAG) {
		size -= (base == 16 || base == 2) ? 2 : (base == 8) ? 1 : 0;
	}

	// Convert number to string (preenche de trás pra frente no tmp)
	if (number == 0) {
		tmp[i++] = '0';
	} else {
		if (base == 16) {
			while (number) {
				tmp[i++] = digits[number & 0xF];
				number >>= 4;
			}
		} else if (base == 8) {
			while (number) {
				tmp[i++] = digits[number & 0x7];
				number >>= 3;
			}
		} else if (base == 2) {
			while (number) {
				tmp[i++] = digits[number & 0x1];
				number >>= 1;
			}
		} else {
			while (number) {
				// assumindo que do_div é uma macro que faz divisão e retorna o resto
				tmp[i++] = digits[do_div(number, base)] | lowercase;
			}
		}
	}

	precision = (i > precision) ? i : precision;
	size -= precision;

	// Left padding with spaces
	if (!(flag & (ZERODAP_FLAG | LEFTPAD_FLAG))) {
		while (size-- > 0) EMIT(' ');
	}

	if (sign) EMIT(sign);

	// Special prefix
	if (flag & SPECIAL_FLAG) {
		if (base == 8) EMIT('0');
		else if (base == 16) {
			EMIT('0');
			EMIT('X' | lowercase);
		} else if (base == 2) {
			EMIT('0');
			EMIT('b');
		}
	}

	// Zero padding
	if (!(flag & LEFTPAD_FLAG)) {
		while (size-- > 0) EMIT(c);
	}

	// Precision padding
	while (i < precision--) EMIT('0');

	// Copy digits in reverse
	while (i--) EMIT(tmp[i]);

	// Right padding
	while (size-- > 0) EMIT(' ');

	return count;
}

int vprintfmt(void (*putc)(char c, void *ctx), void *ctx, const char *fmt, va_list args) {
	int count = 0;

	for (; *fmt; ++fmt) {
		if (*fmt != '%') {
			EMIT(*fmt);
			continue;
		}

		/* parse format specifier */
		int flags = 0;
		while (++fmt) {
			if (*fmt == '-') flags |= LEFTPAD_FLAG;
			else if (*fmt == '+') flags |= PLUS_PREFIX_FLAG;
			else if (*fmt == ' ') flags |= SPACE_PREFIX_FLAG;
			else if (*fmt == '#') flags |= SPECIAL_FLAG;
			else if (*fmt == '0') flags |= ZERODAP_FLAG;
			else break;
		}

		/* field width */
		int field_width = -1;
		if (__isdigit(*fmt))
			field_width = skip_stoi(&fmt);
		else if (*fmt == '*') {
			fmt++;
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFTPAD_FLAG;
			}
		}

		/* precision */
		int precision = -1;
		if (*fmt == '.') {
			fmt++;
			precision = (__isdigit(*fmt)) ? skip_stoi(&fmt) : 0;
			if (*fmt == '*') {
				fmt++;
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* qualifier */
		int qualifier = -1;
		if (*fmt == 'l' && *(fmt + 1) == 'l') {
			qualifier = 'q';
			fmt += 2;
		} else if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z') {
			qualifier = *fmt;
			++fmt;
		}

		int base = 10;
		switch (*fmt) {
			case 'c':
				if (!(flags & LEFTPAD_FLAG))
					while (--field_width > 0) EMIT(' ');
				EMIT((unsigned char)va_arg(args, int));
				while (--field_width > 0) EMIT(' ');
				continue;
			case 's': {
				const char *s = va_arg(args, char *);
				if (!s) s = "(null)";
				int len = (precision < 0) ? strlen(s) : strnlen(s, precision);
				if (!(flags & LEFTPAD_FLAG))
					while (len < field_width--) EMIT(' ');
				for (int j = 0; j < len; ++j) EMIT(s[j]);
				while (len < field_width--) EMIT(' ');
				continue;
			}
			case 'p':
				if (field_width == -1) {
					field_width = 2 * sizeof(void *);
					flags |= ZERODAP_FLAG;
				}
				count += print_num(putc, ctx, (unsigned long)va_arg(args, void *), 16,
								field_width, precision, flags);
				continue;
			case 'n':
				if (qualifier == 'l') {
					long *ip = va_arg(args, long *);
					*ip = count;
				} else {
					int *ip = va_arg(args, int *);
					*ip = count;
				}
				continue;
			case '%':
				EMIT('%');
				continue;
			case 'b': base = 2; break;
			case 'o': base = 8; break;
			case 'x': flags |= LOWERCASE_FLAG;
			case 'X': base = 16; break;
			case 'd':
			case 'i': flags |= SHOW_SIGN_FLAG; break;
			case 'u': break;
			default:
				EMIT('%');
				if (*fmt) EMIT(*fmt);
				else --fmt;
				continue;
		}

		unsigned long long num;

		if (flags & SHOW_SIGN_FLAG) {
			// signed
			if (qualifier == 'l')
				num = va_arg(args, long);
			else if (qualifier == 'q')
				num = va_arg(args, long long);
			else if (qualifier == 'h')
				num = (short)va_arg(args, int);
			else
				num = va_arg(args, int);
		} else {
			// unsigned
			if (qualifier == 'l')
				num = va_arg(args, unsigned long);
			else if (qualifier == 'q')
				num = va_arg(args, unsigned long long);
			else if (qualifier == 'h')
				num = (unsigned short)va_arg(args, unsigned int);
			else
				num = va_arg(args, unsigned int);
		}
		
		count += print_num(putc, ctx, num, base, field_width, precision, flags);
	}

	return count;
}
#undef EMIT

struct buffer_ctx {
	char *buf;
	size_t pos;
	size_t size;
};

static void buffer_putc(char c, void *ctx) {
	struct buffer_ctx *b = ctx;

	if (b->pos < b->size - 1) {
		b->buf[b->pos] = c;
	}

	b->pos++;
}

int vsnprintf(char *restrict buf, unsigned int size, const char *fmt, va_list args) {
	if (size == 0) {
		return 0;
	}

	struct buffer_ctx ctx = {
		.buf = buf,
		.pos = 0,
		.size = size
	};

	vprintfmt(buffer_putc, &ctx, fmt, args);

	if (ctx.pos < size) {
		buf[ctx.pos] = '\0';
	} else {
		buf[size - 1] = '\0';
	}

	return ctx.pos;
}

int snprintf(char* restrict buf, unsigned int size, const char* restrict fmt, ...){
	va_list args;
	va_start(args, fmt);
	int count = vsnprintf(buf, size, fmt, args);
	va_end(args);
	
	return count;
}
