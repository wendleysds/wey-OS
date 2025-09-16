#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>

#define PRINTF_BUF_SIZE 256

static void toa_base(long long value, char* result, int base, int is_unsigned) {
	const char* digits = "0123456789abcdef";
	char buffer[32];
	int i = 0, j = 0;
	int max_result_len = sizeof(buffer);

	if (base < 2 || base > 16) {
		if (result && max_result_len > 0)
			result[0] = '\0';
		return;
	}

	unsigned long long uvalue;
	if (!is_unsigned && value < 0 && base == 10) {
		if (j < max_result_len - 1)
			result[j++] = '-';
		uvalue = (unsigned long long)(-value);
	} else {
		uvalue = (unsigned long long)value;
	}

	do {
		if (i < (int)sizeof(buffer) - 1)
			buffer[i++] = digits[uvalue % base];
		uvalue /= base;
	} while(uvalue);

	while(i > 0 && j < max_result_len - 1){
		result[j++] = buffer[--i];
	}

	result[j] = '\0';
}

static inline void itoa(long long value, char* result, int base){
	return toa_base(value, result, base, 0);
}

static inline void utoa(long long value, char* result, int base){
	return toa_base(value, result, base, 1);
}

int printf(const char *restrict fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char toabuffer[32];
	char outbuf[PRINTF_BUF_SIZE];
	short outpos = 0;

	unsigned int char_count = 0;

	// Helper to flush buffer
	#define FLUSH_BUF() do { \
		outbuf[outpos] = '\0'; \
		syscall(SYS_write, (long)outbuf, outpos, 0, 0); \
		char_count += outpos; \
		outpos = 0; \
	} while (0)

	#define CONVERT_CHAR_TO_UPPER(c) ((c) >= 'a' && (c) <= 'f' ? (c) - 32 : (c))

	for (const char *ptr = fmt; *ptr; ptr++) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
				case 'c': {
					char c = (char)va_arg(args, int);
					if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
					outbuf[outpos++] = c;
					break;
				}
				case 's': {
					char *s = va_arg(args, char*);
					while (*s) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = *s++;
					}
					break;
				}
				case 'u':
				case 'd':
				case 'i': {
					if(*ptr == 'u')
						utoa(va_arg(args, unsigned int), toabuffer, 10);
					else
						itoa(va_arg(args, int), toabuffer, 10);

					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = *s;
					}
					break;
				}
				case 'x':
				case 'X': {
					utoa(va_arg(args, unsigned int), toabuffer, 16);
					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = (*ptr == 'X') ? CONVERT_CHAR_TO_UPPER(*s) : *s;
					}
					break;
				}
				default: {
					if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
					outbuf[outpos++] = '%';
					if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
					outbuf[outpos++] = *ptr;
					break;
				}
			}
		} else {
			if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
			outbuf[outpos++] = *ptr;
		}
	}

	if (outpos > 0) FLUSH_BUF();
	va_end(args);
	#undef FLUSH_BUF
	#undef CONVERT_CHAR_TO_UPPER

	return char_count;
}
