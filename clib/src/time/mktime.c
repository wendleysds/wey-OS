#include <time.h>

time_t mktime(struct tm *timeptr) {
    if (!timeptr) return (time_t)-1;

    int year = timeptr->tm_year + 1900;
    int mon = timeptr->tm_mon;

    if (mon >= 12) {
        year += mon / 12;
        mon %= 12;
    } else if (mon < 0) {
        int years_less = (11 - mon) / 12;
        year -= years_less;
        mon += years_less * 12;
    }

    year -= (mon < 2);
    long era = (year >= 0 ? year : year - 399) / 400;
    unsigned yoe = (unsigned)(year - era * 400);
    int mp = mon + (mon >= 2 ? -2 : 10);
    
    unsigned doy = (153 * mp + 2) / 5 + timeptr->tm_mday - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long days = era * 146097 + (long)doe - 719468;

    time_t t = (time_t)days * 86400ULL + timeptr->tm_hour * 3600 + timeptr->tm_min * 60 + timeptr->tm_sec;

    // Normaliza os membros da struct tm
    gmtime_r(&t, timeptr);

    return t;
}