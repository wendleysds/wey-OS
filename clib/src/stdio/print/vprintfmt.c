#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#define ZEROPAD_FLAG      (1 << 0)
#define SHOW_SIGN_FLAG    (1 << 1)
#define PLUS_PREFIX_FLAG  (1 << 2)
#define SPACE_PREFIX_FLAG (1 << 3)
#define SPECIAL_FLAG      (1 << 4)
#define LOWERCASE_FLAG    (1 << 5)
#define LEFTPAD_FLAG      (1 << 6)

#define __isdigit(c) ((c) >= '0' && (c) <= '9')

static int skip_stoi(const char **s) {
    int i = 0;
    while (__isdigit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

static uint32_t do_div_32(uint64_t *n, uint32_t base) {
    uint32_t rem;
    
    #if defined(__i386__) || defined(__i686__)
    uint32_t high = (uint32_t)(*n >> 32);
    uint32_t low = (uint32_t)(*n);

    if (high == 0) {
        asm("divl %4"
            : "=a"(low), "=d"(rem)
            : "a"(low), "d"(0), "r"(base));
        *n = low;
    } else {
        asm("divl %4"
            : "=a"(high), "=d"(rem)
            : "a"(high), "d"(0), "r"(base));

        asm("divl %4"
            : "=a"(low), "=d"(rem)
            : "a"(low), "d"(rem), "r"(base));

        *n = ((uint64_t)high << 32) | low;
    }
    #else
    rem = (uint32_t)(*n % base);
    *n = *n / base;
    #endif

    return rem;
}

#define EMIT(c) do { putc((c), ctx); count++; } while(0)

static int print_float(void (*putc)(char c, void *ctx), void *ctx,
                       double value, int width, int precision, int flags, char fmt_char) {
    int count = 0;
    
    union { double f; uint64_t u; } cast;
    cast.f = value;
    bool is_negative = (cast.u >> 63) != 0;
    uint16_t exp_bits = (cast.u >> 52) & 0x7FF;
    uint64_t mant = cast.u & ((1ULL << 52) - 1);
    
    if (exp_bits == 0x7FF) {
        const char* s = (mant == 0) ? ((fmt_char >= 'a') ? "inf" : "INF") 
                                    : ((fmt_char >= 'a') ? "nan" : "NAN");
        int len = 3;
        char sign = 0;
        if (is_negative) sign = '-';
        else if (flags & SHOW_SIGN_FLAG) sign = (flags & PLUS_PREFIX_FLAG) ? '+' : ' ';
        
        if (sign) width--;
        width -= len;
        
        if (!(flags & LEFTPAD_FLAG)) while (width-- > 0) { EMIT(' '); }
        if (sign) EMIT(sign);
        while (*s) { EMIT(*s++); }
        if (flags & LEFTPAD_FLAG) while (width-- > 0) { EMIT(' '); }
        return count;
    }

    int p = (precision == -1) ? 6 : (precision == 0 && (fmt_char == 'g' || fmt_char == 'G') ? 1 : precision);
    
    int e10 = 0;
    double v = is_negative ? -value : value;
    
    if (v > 0.0) {
        while (v >= 10.0) { v /= 10.0; e10++; }
        while (v < 1.0) { v *= 10.0; e10--; }
    }
    
    bool use_e = false;
    if (fmt_char == 'g' || fmt_char == 'G') {
        if (e10 < -4 || e10 >= p) {
            use_e = true;
            precision = (p > 0) ? p - 1 : 0;
        } else {
            precision = p - 1 - e10;
        }
    } else if (fmt_char == 'e' || fmt_char == 'E') {
        use_e = true;
        if (precision == -1) precision = 6;
    } else { // 'f' ou 'F'
        if (precision == -1) precision = 6;
    }
    
    double rounding = 0.5;
    for (int j = 0; j < precision; ++j) rounding /= 10.0;
    
    double abs_val = is_negative ? -value : value;
    if (use_e) abs_val = v;
    
    abs_val += rounding;
    if (use_e && abs_val >= 10.0) {
        abs_val /= 10.0;
        e10++;
    }
    
    uint64_t int_part = (uint64_t)abs_val;
    double frac_part = abs_val - (double)int_part;
    
    char int_buf[64];
    int int_len = 0;
    if (int_part == 0) {
        int_buf[int_len++] = '0';
    } else {
        while (int_part > 0) {
            int_buf[int_len++] = '0' + do_div_32(&int_part, 10);
        }
    }
    
    char frac_buf[64];
    int frac_len = 0;
    for (int j = 0; j < precision; ++j) {
        frac_part *= 10.0;
        int digit = (int)frac_part;
        if (digit > 9) digit = 9;
        frac_buf[frac_len++] = '0' + digit;
        frac_part -= digit;
    }
    
    if ((fmt_char == 'g' || fmt_char == 'G') && !(flags & SPECIAL_FLAG)) {
        while (frac_len > 0 && frac_buf[frac_len - 1] == '0') {
            frac_len--;
        }
    }
    
    char exp_buf[8];
    int exp_len = 0;
    if (use_e) {
        int abs_e10 = e10 >= 0 ? e10 : -e10;
        char temp_exp[4];
        int te_len = 0;
        do {
            temp_exp[te_len++] = '0' + (abs_e10 % 10);
            abs_e10 /= 10;
        } while (abs_e10 > 0);
        if (te_len < 2) temp_exp[te_len++] = '0';
        
        exp_buf[exp_len++] = (fmt_char >= 'a') ? 'e' : 'E';
        exp_buf[exp_len++] = (e10 >= 0) ? '+' : '-';
        while (te_len > 0) {
            exp_buf[exp_len++] = temp_exp[--te_len];
        }
    }
    
    char sign = 0;
    if (is_negative) sign = '-';
    else if (flags & SHOW_SIGN_FLAG) sign = (flags & PLUS_PREFIX_FLAG) ? '+' : ' ';
    
    int total_len = (sign ? 1 : 0) + int_len + (frac_len > 0 || (flags & SPECIAL_FLAG) ? 1 + frac_len : 0) + exp_len;
    int pad_len = width - total_len;
    if (pad_len < 0) pad_len = 0;
    
    if (!(flags & (LEFTPAD_FLAG | ZEROPAD_FLAG))) {
        while (pad_len > 0) { EMIT(' '); pad_len--; }
    }
    if (sign) EMIT(sign);
    if (!(flags & LEFTPAD_FLAG) && (flags & ZEROPAD_FLAG)) {
        while (pad_len > 0) { EMIT('0'); pad_len--; }
    }
    
    while (int_len > 0) { EMIT(int_buf[--int_len]); }
    
    if (frac_len > 0 || (flags & SPECIAL_FLAG)) {
        EMIT('.');
        for (int j = 0; j < frac_len; ++j) { EMIT(frac_buf[j]); }
    }
    
    for (int j = 0; j < exp_len; ++j) { EMIT(exp_buf[j]); }
    
    if (flags & LEFTPAD_FLAG) {
        while (pad_len > 0) { EMIT(' '); pad_len--; }
    }
    
    return count;
}

static int print_num(void (*putc)(char c, void *ctx), void *ctx,
                     uint64_t number, int base, int width,
                     int precision, int flags, bool is_negative) {

    static const char digits_upper[] = "0123456789ABCDEF";
    static const char digits_lower[] = "0123456789abcdef";
    const char *digits = (flags & LOWERCASE_FLAG) ? digits_lower : digits_upper;

    char tmp[65];
    int i = 0;
    int count = 0;

    if (base < 2 || base > 16) return 0;

    if (flags & LEFTPAD_FLAG) {
        flags &= ~ZEROPAD_FLAG;
    }

    char sign = 0;
    if (is_negative) {
        sign = '-';
        width--;
    } else if (flags & SHOW_SIGN_FLAG) {
        if (flags & PLUS_PREFIX_FLAG) {
            sign = '+';
            width--;
        } else if (flags & SPACE_PREFIX_FLAG) {
            sign = ' ';
            width--;
        }
    }

    int spec_len = 0;
    if (flags & SPECIAL_FLAG) {
        if (base == 16 || base == 2) spec_len = 2;
        else if (base == 8) spec_len = 1;
        width -= spec_len;
    }

    if (number == 0) {
        if (precision != 0) {
            tmp[i++] = '0';
        }
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
                tmp[i++] = digits[do_div_32(&number, base)];
            }
        }
    }

    if (precision >= 0) {
        flags &= ~ZEROPAD_FLAG;
    }

    int num_digits = i;
    int zero_padding = (precision > num_digits) ? (precision - num_digits) : 0;
    int pad_len = width - (num_digits + zero_padding);
    if (pad_len < 0) pad_len = 0;

    if (!(flags & (ZEROPAD_FLAG | LEFTPAD_FLAG))) {
        while (pad_len > 0) { EMIT(' '); pad_len--; }
    }

    if (sign) EMIT(sign);

    if (flags & SPECIAL_FLAG) {
        if (base == 8) EMIT('0');
        else if (base == 16) {
            EMIT('0');
            EMIT((flags & LOWERCASE_FLAG) ? 'x' : 'X');
        } else if (base == 2) {
            EMIT('0');
            EMIT('b');
        }
    }

    if (!(flags & LEFTPAD_FLAG) && (flags & ZEROPAD_FLAG)) {
        while (pad_len > 0) { EMIT('0'); pad_len--; }
    }

    while (zero_padding > 0) { EMIT('0'); zero_padding--; }
    while (i > 0) { EMIT(tmp[--i]); }

    if (flags & LEFTPAD_FLAG) {
        while (pad_len > 0) { EMIT(' '); pad_len--; }
    }

    return count;
}

int vprintfmt(void (*putc)(char c, void *ctx), void *ctx, const char *fmt, va_list args) {
    int count = 0;

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            EMIT(*fmt);
            continue;
        }

        int flags = 0;
        while (++fmt) {
            if (*fmt == '-') flags |= LEFTPAD_FLAG;
            else if (*fmt == '+') flags |= PLUS_PREFIX_FLAG;
            else if (*fmt == ' ') flags |= SPACE_PREFIX_FLAG;
            else if (*fmt == '#') flags |= SPECIAL_FLAG;
            else if (*fmt == '0') flags |= ZEROPAD_FLAG;
            else break;
        }

        int field_width = -1;
        if (__isdigit(*fmt)) {
            field_width = skip_stoi(&fmt);
        } else if (*fmt == '*') {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFTPAD_FLAG;
            }
        }

        int precision = -1;
        if (*fmt == '.') {
            fmt++;
            if (__isdigit(*fmt)) {
                precision = skip_stoi(&fmt);
            } else if (*fmt == '*') {
                fmt++;
                precision = va_arg(args, int);
            } else {
                precision = 0;
            }
        }

        int qualifier = 0;
        if (*fmt == 'h') {
            qualifier = 'h';
            if (*(fmt + 1) == 'h') { qualifier = 'H'; fmt++; }
            fmt++;
        } else if (*fmt == 'l') {
            qualifier = 'l';
            if (*(fmt + 1) == 'l') { qualifier = 'q'; fmt++; }
            fmt++;
        } else if (*fmt == 'z' || *fmt == 't') {
            qualifier = *fmt;
            fmt++;
        }

        int base = 10;
        bool is_signed = false;

        switch (*fmt) {
            case 'c': {
                char c = (char)va_arg(args, int);
                if (!(flags & LEFTPAD_FLAG))
                    while (--field_width > 0) EMIT(' ');
                EMIT(c);
                while (--field_width > 0) EMIT(' ');
                continue;
            }
            case 's': {
                const char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                
                int len = 0;
                while (s[len] && (precision < 0 || len < precision)) len++;

                if (!(flags & LEFTPAD_FLAG))
                    while (field_width-- > len) EMIT(' ');
                for (int j = 0; j < len; ++j) EMIT(s[j]);
                while (field_width-- > len) EMIT(' ');
                continue;
            }
            case 'p': {
                uint32_t ptr = (uint32_t)va_arg(args, void *);
                flags |= SPECIAL_FLAG | LOWERCASE_FLAG;
                if (field_width == -1) field_width = 8;
                count += print_num(putc, ctx, (uint64_t)ptr, 16, field_width, precision, flags, false);
                continue;
            }
            case '%':
                EMIT('%');
                continue;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                double num = va_arg(args, double);
                count += print_float(putc, ctx, num, field_width, precision, flags, *fmt);
                continue;
            }
            case 'b': base = 2; break;
            case 'o': base = 8; break;
            case 'x': flags |= LOWERCASE_FLAG; /* fallthrough */
            case 'X': base = 16; break;
            case 'd':
            case 'i': is_signed = true; flags |= SHOW_SIGN_FLAG; break;
            case 'u': break;
            default:
                EMIT('%');
                if (*fmt) EMIT(*fmt);
                else --fmt;
                continue;
        }

        uint64_t num;
        bool is_negative = false;

        if (is_signed) {
            int64_t snum;
            if (qualifier == 'q') snum = va_arg(args, long long);
            else if (qualifier == 'l') snum = va_arg(args, long);
            else if (qualifier == 'z') snum = va_arg(args, int);
            else if (qualifier == 'h') snum = (short)va_arg(args, int);
            else if (qualifier == 'H') snum = (signed char)va_arg(args, int);
            else snum = va_arg(args, int);

            if (snum < 0) {
                is_negative = true;
                num = (uint64_t)(-snum);
            } else {
                num = (uint64_t)snum;
            }
        } else {
            if (qualifier == 'q') num = va_arg(args, unsigned long long);
            else if (qualifier == 'l') num = va_arg(args, unsigned long);
            else if (qualifier == 'z') num = va_arg(args, uint32_t);
            else if (qualifier == 'h') num = (unsigned short)va_arg(args, unsigned int);
            else if (qualifier == 'H') num = (unsigned char)va_arg(args, unsigned int);
            else num = va_arg(args, unsigned int);
        }

        count += print_num(putc, ctx, num, base, field_width, precision, flags, is_negative);
    }

    return count;
}
#undef EMIT
