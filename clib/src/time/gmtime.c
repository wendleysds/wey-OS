#include <time.h>
#include <stdint.h>

static struct tm global_tm_buffer;

extern uint32_t do_div_32(uint64_t *n, uint32_t base);

static const int __mon_yday[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static inline int __is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

struct tm *gmtime_r(const time_t *timer, struct tm *result) {
    if (!timer || !result) return NULL;

    uint64_t t = (uint64_t)*timer;
    
    // do_div_32 divide 't' por 86400 e retorna o resto (segundos do dia)
    uint32_t day_seconds = do_div_32(&t, 86400);
    uint32_t days_32 = (uint32_t)t; // Quantidade de dias cabe em 32-bit

    // Decomposição de segundos no dia usando divisão simples de 32-bit
    result->tm_sec = day_seconds % 60;
    result->tm_min = (day_seconds / 60) % 60;
    result->tm_hour = day_seconds / 3600;

    // Dia da semana: Epoch (1970-01-01) foi Quinta-feira (4)
    result->tm_wday = (int)((days_32 + 4) % 7);

    // Algoritmo de Calendário Civil de Hinnant
    long days = (long)days_32 + 719468; // Shift até 0000-03-01
    long era = (days >= 0 ? days : days - 146096) / 146097;
    unsigned long doe = (unsigned long)(days - era * 146097);
    unsigned long yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    long y = (long)yoe + era * 400;
    unsigned long doy = doe - (365 * yoe + yoe/4 - yoe/100);
    unsigned long mp = (5 * doy + 2) / 153;
    
    unsigned long d = doy - (153 * mp + 2) / 5 + 1;
    unsigned long m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);

    result->tm_mday = (int)d;
    result->tm_mon = (int)m - 1;
    result->tm_year = (int)y - 1900;

    int leap = __is_leap(result->tm_year + 1900);
    result->tm_yday = __mon_yday[leap][result->tm_mon] + result->tm_mday - 1;
    result->tm_isdst = 0;

    return result;
}

/* --- gmtime --- */
struct tm *gmtime(const time_t *timer) {
    return gmtime_r(timer, &global_tm_buffer);
}