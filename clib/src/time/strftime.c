#include <time.h>
#include <stdio.h>
#include <stddef.h>

static const char *const __days_abbrev[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *const __days_full[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char *const __months_abbrev[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *const __months_full[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
    if (!s || !max || !format || !tm) return 0;

    char *out = s;
    size_t remaining = max;

    while (*format && remaining > 1) {
        if (*format != '%') {
            *out++ = *format++;
            remaining--;
            continue;
        }

        format++;
        int written = 0;

        switch (*format) {
            case 'Y':
                written = snprintf(out, remaining, "%04d", tm->tm_year + 1900);
                break;
            case 'y':
                written = snprintf(out, remaining, "%02d", (tm->tm_year + 1900) % 100);
                break;
            case 'm':
                written = snprintf(out, remaining, "%02d", tm->tm_mon + 1);
                break;
            case 'd':
                written = snprintf(out, remaining, "%02d", tm->tm_mday);
                break;
            case 'e':
                written = snprintf(out, remaining, "%2d", tm->tm_mday);
                break;
            case 'j':
                written = snprintf(out, remaining, "%03d", tm->tm_yday + 1);
                break;
            case 'F':
                written = snprintf(out, remaining, "%04d-%02d-%02d", 
                                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
                break;
            case 'H':
                written = snprintf(out, remaining, "%02d", tm->tm_hour);
                break;
            case 'I':
                written = snprintf(out, remaining, "%02d", (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
                break;
            case 'M':
                written = snprintf(out, remaining, "%02d", tm->tm_min);
                break;
            case 'S':
                written = snprintf(out, remaining, "%02d", tm->tm_sec);
                break;
            case 'T':
                written = snprintf(out, remaining, "%02d:%02d:%02d", 
                                   tm->tm_hour, tm->tm_min, tm->tm_sec);
                break;
            case 'a':
                if (tm->tm_wday >= 0 && tm->tm_wday <= 6)
                    written = snprintf(out, remaining, "%s", __days_abbrev[tm->tm_wday]);
                break;
            case 'A':
                if (tm->tm_wday >= 0 && tm->tm_wday <= 6)
                    written = snprintf(out, remaining, "%s", __days_full[tm->tm_wday]);
                break;
            case 'b':
            case 'h':
                if (tm->tm_mon >= 0 && tm->tm_mon <= 11)
                    written = snprintf(out, remaining, "%s", __months_abbrev[tm->tm_mon]);
                break;
            case 'B':
                if (tm->tm_mon >= 0 && tm->tm_mon <= 11)
                    written = snprintf(out, remaining, "%s", __months_full[tm->tm_mon]);
                break;
            case 'p':
                written = snprintf(out, remaining, "%s", (tm->tm_hour >= 12) ? "PM" : "AM");
                break;

            case 'n':
                written = snprintf(out, remaining, "\n");
                break;
            case 't':
                written = snprintf(out, remaining, "\t");
                break;
            case '%':
                written = snprintf(out, remaining, "%%");
                break;

            default:
                written = snprintf(out, remaining, "%%%c", *format);
                break;
        }

        if (written < 0 || (size_t)written >= remaining) {
            *out = '\0';
            return 0;
        }

        out += written;
        remaining -= written;
        format++;
    }

    *out = '\0';
    return (size_t)(out - s);
}