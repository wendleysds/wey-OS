#include <time.h>
#include <stdio.h>
#include <stddef.h>

/* Nomes de dias e meses no padrão POSIX C/LC_TIME */
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

/* --- strftime --- */
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

        format++; // Pula '%'
        int written = 0;

        switch (*format) {
            /* --- Formatos de Data Básicos e Estendidos --- */
            case 'Y': // Ano com 4 dígitos (ex: 2026)
                written = snprintf(out, remaining, "%04d", tm->tm_year + 1900);
                break;
            case 'y': // Ano com 2 dígitos [00,99]
                written = snprintf(out, remaining, "%02d", (tm->tm_year + 1900) % 100);
                break;
            case 'm': // Mês em dígitos [01,12]
                written = snprintf(out, remaining, "%02d", tm->tm_mon + 1);
                break;
            case 'd': // Dia do mês preenchido com zero [01,31]
                written = snprintf(out, remaining, "%02d", tm->tm_mday);
                break;
            case 'e': // Dia do mês preenchido com espaço [ 1,31]
                written = snprintf(out, remaining, "%2d", tm->tm_mday);
                break;
            case 'j': // Dia do ano [001,366]
                written = snprintf(out, remaining, "%03d", tm->tm_yday + 1);
                break;
            case 'F': // Atalho ISO 8601 para %Y-%m-%d (ex: 2026-10-24)
                written = snprintf(out, remaining, "%04d-%02d-%02d", 
                                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
                break;

            /* --- Formatos de Hora --- */
            case 'H': // Hora [00,23]
                written = snprintf(out, remaining, "%02d", tm->tm_hour);
                break;
            case 'I': // Hora [01,12]
                written = snprintf(out, remaining, "%02d", (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
                break;
            case 'M': // Minuto [00,59]
                written = snprintf(out, remaining, "%02d", tm->tm_min);
                break;
            case 'S': // Segundo [00,60]
                written = snprintf(out, remaining, "%02d", tm->tm_sec);
                break;
            case 'T': // Atalho para %H:%M:%S (ex: 14:30:00)
                written = snprintf(out, remaining, "%02d:%02d:%02d", 
                                   tm->tm_hour, tm->tm_min, tm->tm_sec);
                break;

            /* --- Textuais (Nomes de Dias e Meses) --- */
            case 'a': // Nome abreviado do dia da semana (ex: Sat)
                if (tm->tm_wday >= 0 && tm->tm_wday <= 6)
                    written = snprintf(out, remaining, "%s", __days_abbrev[tm->tm_wday]);
                break;
            case 'A': // Nome completo do dia da semana (ex: Saturday)
                if (tm->tm_wday >= 0 && tm->tm_wday <= 6)
                    written = snprintf(out, remaining, "%s", __days_full[tm->tm_wday]);
                break;
            case 'b':
            case 'h': // Nome abreviado do mês (ex: Oct)
                if (tm->tm_mon >= 0 && tm->tm_mon <= 11)
                    written = snprintf(out, remaining, "%s", __months_abbrev[tm->tm_mon]);
                break;
            case 'B': // Nome completo do mês (ex: October)
                if (tm->tm_mon >= 0 && tm->tm_mon <= 11)
                    written = snprintf(out, remaining, "%s", __months_full[tm->tm_mon]);
                break;
            case 'p': // AM/PM
                written = snprintf(out, remaining, "%s", (tm->tm_hour >= 12) ? "PM" : "AM");
                break;

            /* --- Caracteres Especiais --- */
            case 'n': // Nova linha '\n'
                written = snprintf(out, remaining, "\n");
                break;
            case 't': // Tabulação '\t'
                written = snprintf(out, remaining, "\t");
                break;
            case '%':
                written = snprintf(out, remaining, "%%");
                break;

            default:
                // Caso não reconhecido, copia o caractere literalmente
                written = snprintf(out, remaining, "%%%c", *format);
                break;
        }

        /* Se o snprintf falhar ou o buffer estourar, interrompe truncando */
        if (written < 0 || (size_t)written >= remaining) {
            *out = '\0';
            return 0; // POSIX especifica retornar 0 quando não couber no buffer
        }

        out += written;
        remaining -= written;
        format++;
    }

    *out = '\0';
    return (size_t)(out - s);
}