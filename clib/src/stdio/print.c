#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PRINTF_BUF_SIZE 256

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
		puts(outbuf); \
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
				case 'u': {
					utoa(va_arg(args, unsigned int), toabuffer, 10);
					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = *s;
					}
					break;
				}
				case 'd': {
					itoa(va_arg(args, int), toabuffer, 10);
					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = *s;
					}
					break;
				}
				case 'x': {
					utoa(va_arg(args, unsigned int), toabuffer, 16);
					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = *s;
					}
					break;
				}
				case 'X': {
					utoa(va_arg(args, unsigned int), toabuffer, 16);
					for (char *s = toabuffer; *s; s++) {
						if (outpos >= PRINTF_BUF_SIZE) FLUSH_BUF();
						outbuf[outpos++] = CONVERT_CHAR_TO_UPPER(*s);
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