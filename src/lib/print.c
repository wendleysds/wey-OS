#include <lib/string.h>
#include <stdarg.h>
#include <stdint.h>

#define ZERODAP_FLAG      0x01
#define SHOW_SIGN_FLAG    0x02
#define PLUS_PREFIX_FLAG  0x04
#define SPACE_PREFIX_FLAG 0x08
#define SPECIAL_FLAG      0x10
#define LOWERCASE_FLAG    0x20
#define LEFTPAD_FLAG      0x40

#define __isdigit(c) ((c) >= '0' && (c) <= '9')
#define __tolow(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 32 : (c))

uint32_t __div(uint64_t *n, uint32_t base){
	uint64_t rem = *n;
	uint64_t b = base;
	uint64_t res, d = 1;
	uint32_t high = rem >> 32;

	res = 0;
	if (high >= base) {
		high /= base;
		res = (uint64_t) high << 32;
		rem -= (uint64_t) (high*base) << 32;
	}

	while ((int64_t)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

static int skip_stoi(const char **s){
	int i = 0;
	while (__isdigit(**s)){
		i = i * 10 + *((*s)++) - '0';
	}
		
	return i;
}

static char* stoi(char* str, unsigned long long number, int base, int size, int precision, int flag){
	static const char digits[] = "0123456789ABCDEF";

	char tmp[128];
	int i = 0;

	if(base < 2 || base > 16){
		return NULL;
	}

	uint8_t lowercase = (flag & LOWERCASE_FLAG);

	if(flag & LEFTPAD_FLAG){
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

	// Handle special prefix (0x, 0)
	if(flag & SPECIAL_FLAG){
		size -= (base == 16) ? 2 : (base == 8) ? 1 : 0;
	}

	// Convert number to string
	if(number == 0){
		tmp[i++] = '0';
	} else {
		while (number){
			tmp[i++] = (digits[__div(&number, base)] | lowercase);
		}
	}

	precision = (i > precision) ? i : precision;
	size -= precision;

	// Left padding with spaces
	if(!(flag & (ZERODAP_FLAG | LEFTPAD_FLAG))){
		while(size-- > 0) *str++ = ' ';
	}

	if(sign) *str++ = sign;

	// Special prefix
	if(flag & SPECIAL_FLAG){
		if(base == 8) *str++ = '0';
		else if(base == 16){
			*str++ = '0';
			*str++ = ('X' | lowercase);
		}
	}

	// Zero padding
	if(!(flag & LEFTPAD_FLAG)){
		while (size-- > 0) *str++ = c;
	}

	// Precision padding
	while(i < precision--) *str++ = '0';

	// Copy digits in reverse
	while(i--) *str++ = tmp[i];

	// Right padding
	while(size-- > 0) *str++ = ' ';

	return str;
}

int vsprintf(char* restrict buf, const char* fmt, va_list args){
	char *str = buf;
	
	for (; *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
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
		else if (*fmt == '*'){
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
			if (*fmt == '*'){
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
		} else if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L'
			|| *fmt == 'Z') {
			qualifier = *fmt;
			++fmt;
		}

		int base = 10;
		switch (*fmt) {
			case 'c':
				if (!(flags & LEFTPAD_FLAG))
					while (--field_width > 0) *str++ = ' ';
				*str++ = (unsigned char)va_arg(args, int);
				while (--field_width > 0) *str++ = ' ';
				continue;
			case 's': {
					const char *s = va_arg(args, char *);
					int len = (precision < 0) ? strlen(s) : strnlen(s, precision);
				if (!(flags & LEFTPAD_FLAG))
					while (len < field_width--) *str++ = ' ';
				for (int i = 0; i < len; ++i) *str++ = *s++;
				while (len < field_width--) *str++ = ' ';
				continue;
			}
			case 'p':
				if (field_width == -1) {
					field_width = 2 * sizeof(void *);
					flags |= ZERODAP_FLAG;
				}
				str = stoi(str, (unsigned long)va_arg(args, void *), 16,
						field_width, precision, flags);
				continue;
			case 'n':
				if (qualifier == 'l') {
					long *ip = va_arg(args, long *);
					*ip = (str - buf);
				} else {
					int *ip = va_arg(args, int *);
					*ip = (str - buf);
				}
				continue;
			case '%':
				*str++ = '%';
				continue;
			case 'o': base = 8; break;
			case 'x': flags |= LOWERCASE_FLAG;
			case 'X': base = 16; break;
			case 'd':
			case 'i': flags |= SHOW_SIGN_FLAG; break;
			case 'u': break;
			default:
				*str++ = '%';
				if (*fmt) *str++ = *fmt;
				else --fmt;
				continue;
		}

		unsigned long long num;
		if (qualifier == 'l')
			num = (flags & SHOW_SIGN_FLAG) ? (signed long)va_arg(args, int) 
				: (unsigned long)va_arg(args, long);
		else if(qualifier == 'q')
			num = (flags & SHOW_SIGN_FLAG) ? (signed long long)va_arg(args, int) 
				: (unsigned long long)va_arg(args, long long);
		else if (qualifier == 'h')
			num = (flags & SHOW_SIGN_FLAG) ? (signed short)va_arg(args, int) 
				: (unsigned short)va_arg(args, int);
		else
			num = (flags & SHOW_SIGN_FLAG) ? va_arg(args, signed int) 
				: va_arg(args, unsigned int);
		
		str = stoi(str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str - buf;
}

int vsnprintf(char* restrict buf, unsigned int size, const char* restrict fmt, va_list args){
	if (size == 0) {
		return 0; // No space to write
	}

	char temp[4096];
	int count = vsprintf(temp, fmt, args);
	
	if (size > 0) {
		unsigned int copy_len = (count < (int)size - 1) ? count : size - 1;
		memcpy(buf, temp, copy_len);
		buf[copy_len] = '\0';
	}
	
	return count;
}

int sprintf(char* restrict buf, const char* restrict fmt, ...){
	va_list args;

	va_start(args, fmt);
	int count = vsprintf(buf, fmt, args);
	va_end(args);

	return count;
}

int snprintf(char* restrict buf, unsigned int size, const char* restrict fmt, ...){
	va_list args;
	va_start(args, fmt);
	
	char temp[4096];
	int count = vsprintf(temp, fmt, args);
	va_end(args);
	
	if (size > 0) {
		unsigned int copy_len = (count < (int)size - 1) ? count : size - 1;
		memcpy(buf, temp, copy_len);
		buf[copy_len] = '\0';
	}
	
	return count;
}
