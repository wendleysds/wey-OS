#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>

#undef getc

enum {
    MOD_NONE = 0,
    MOD_HH,
    MOD_H,
    MOD_L,
    MOD_LL
};

struct stream_source {
    void *ctx;
    int (*getc)(void *);
    int (*ungetc)(int, void *);
};

int vscanfmt(struct stream_source *restrict stream, const char *restrict fmt, va_list ap) {
    int count = 0;
    int c;

    while (*fmt) {
        if (isspace((unsigned char)*fmt)) {
            while (isspace((unsigned char)*fmt)) fmt++;
            while ((c = stream->getc(stream->ctx)) != EOF && isspace(c));
            if (c != EOF) stream->ungetc(c, stream->ctx);
            continue;
        }

        if (*fmt != '%') {
            c = stream->getc(stream->ctx);
            if (c != *fmt) {
                if (c != EOF) stream->ungetc(c, stream->ctx);
                return (count == 0 && c == EOF) ? EOF : count;
            }
            fmt++;
            continue;
        }

        fmt++;

        if (*fmt == '%') {
            c = stream->getc(stream->ctx);
            if (c != '%') {
                if (c != EOF) stream->ungetc(c, stream->ctx);
                return (count == 0 && c == EOF) ? EOF : count;
            }
            fmt++;
            continue;
        }

        int suppress = 0;
        if (*fmt == '*') {
            suppress = 1;
            fmt++;
        }

        int width = -1;
        if (isdigit((unsigned char)*fmt)) {
            width = 0;
            while (isdigit((unsigned char)*fmt)) {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        int mod = MOD_NONE;
        if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') {
                mod = MOD_HH;
                fmt++;
            } else {
                mod = MOD_H;
            }
        } else if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                mod = MOD_LL;
                fmt++;
            } else {
                mod = MOD_L;
            }
        } else if (*fmt == 'L') {
            mod = MOD_LL;
            fmt++;
        }

        char spec = *fmt++;
        if (!spec) break;

        if (spec != 'c' && spec != '[') {
            while ((c = stream->getc(stream->ctx)) != EOF && isspace(c));
            if (c != EOF) stream->ungetc(c, stream->ctx);
        }

        if (spec == 'c') {
            int max_chars = (width == -1) ? 1 : width;
            char *dest = suppress ? NULL : va_arg(ap, char *);
            int chars_read = 0;

            while (chars_read < max_chars) {
                c = stream->getc(stream->ctx);
                if (c == EOF) break;
                if (!suppress) dest[chars_read] = (char)c;
                chars_read++;
            }
            if (chars_read == 0) return (count == 0) ? EOF : count;
            if (!suppress) count++;
        }
        else if (spec == 's') {
            char *dest = suppress ? NULL : va_arg(ap, char *);
            int chars_read = 0;

            while (width == -1 || chars_read < width) {
                c = stream->getc(stream->ctx);
                if (c == EOF || isspace(c)) {
                    if (c != EOF) stream->ungetc(c, stream->ctx);
                    break;
                }
                if (!suppress) dest[chars_read] = (char)c;
                chars_read++;
            }
            if (chars_read == 0) return (count == 0) ? EOF : count;
            if (!suppress) {
                dest[chars_read] = '\0';
                count++;
            }
        }
        else if (spec == 'd' || spec == 'i' || spec == 'u' || spec == 'x') {
            char buf[64];
            int i = 0;
            int max_len = (width == -1 || width >= (int)sizeof(buf)) ? (int)sizeof(buf) - 1 : width;

            while (i < max_len) {
                c = stream->getc(stream->ctx);
                if (c == EOF) break;

                int valid = 0;
                if (i == 0 && (c == '+' || c == '-')) valid = 1;
                else if (isdigit(c)) valid = 1;
                else if ((spec == 'x' || spec == 'i') && (isxdigit(c) || c == 'x' || c == 'X')) valid = 1;

                if (valid) {
                    buf[i++] = (char)c;
                } else {
                    stream->ungetc(c, stream->ctx);
                    break;
                }
            }

            if (i == 0) return (count == 0 && c == EOF) ? EOF : count;
            buf[i] = '\0';

            if (!suppress) {
                int base = 10;
                if (spec == 'x') base = 16;
                else if (spec == 'i') base = 0;

                if (spec == 'd' || spec == 'i') {
                    long long val = strtoll(buf, NULL, base);
                    switch (mod) {
                        case MOD_HH: *va_arg(ap, signed char *) = (signed char)val; break;
                        case MOD_H:  *va_arg(ap, short *)       = (short)val;       break;
                        case MOD_L:  *va_arg(ap, long *)        = (long)val;        break;
                        case MOD_LL: *va_arg(ap, long long *)   = val;              break;
                        default:     *va_arg(ap, int *)         = (int)val;         break;
                    }
                } else { // %u, %x
                    unsigned long long val = strtoull(buf, NULL, base);
                    switch (mod) {
                        case MOD_HH: *va_arg(ap, unsigned char *)      = (unsigned char)val;      break;
                        case MOD_H:  *va_arg(ap, unsigned short *)     = (unsigned short)val;     break;
                        case MOD_L:  *va_arg(ap, unsigned long *)      = (unsigned long)val;      break;
                        case MOD_LL: *va_arg(ap, unsigned long long *) = val;                     break;
                        default:     *va_arg(ap, unsigned int *)       = (unsigned int)val;       break;
                    }
                }
                count++;
            }
        }
    }

    return count;
}