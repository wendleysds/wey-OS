#ifndef _CLIB_TIME_H
#define _CLIB_TIME_H

#include <sys/types.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3

#define TIMER_ABSTIME 0x001

typedef long clock_t;
typedef long long timer_t;

typedef long suseconds_t;
typedef long long time_t;

typedef struct timespec {
	time_t    tv_sec;
	long long tv_nsec;
} timespec;

typedef struct tm {
    int tm_sec;   // Seconds [0,60].
    int tm_min;   // Minutes [0,59].
    int tm_hour;  // Hour [0,23].
    int tm_mday;  // Day of month [1,31].
    int tm_mon;   // Month of year [0,11].
    int tm_year;  // Years since 1900.
    int tm_wday;  // Day of week [0,6] (Sunday =0).
    int tm_yday;  // Day of year [0,365].
    int tm_isdst; // Daylight Savings flag.
} tm;

typedef struct timezone {
  int tz_minuteswest;    /* minutes west of UTC */
  int tz_dsttime;        /* type of DST correction to apply */
} timezone;

typedef struct itimerspec {
    struct timespec it_interval; // timer interval: if non-zero, the timer reloads and counts down from it_value
    struct timespec it_value;    // initial expiration value
} itimerspec;

#define CLOCKS_PER_SEC  1000000l

/**
 * @brief Get current cpu time.
 * @return clock_t The current cpu time.
 */
clock_t clock(void);

/**
 * @brief Get the difference between two time_t values.
 * @param time1 The first time_t value.
 * @param time0 The second time_t value.
 * @return double The difference between time1 and time0.
 */
double difftime(time_t time1, time_t time0);

/**
 * @brief Convert time_t to struct tm in UTC.
 * @param timer The time_t value to convert.
 * @return struct tm* The struct tm in UTC.
 */
struct tm *gmtime(const time_t *timer);

/**
 * @brief Convert time_t to struct tm in UTC in a thread-safe manner.
 * @param timer The time_t value to convert.
 * @param result The struct tm to store the result.
 * @return struct tm* The struct tm in UTC.
 */
struct tm *gmtime_r(const time_t *timer, struct tm *result);

/**
 * @brief Convert time_t to struct tm in local time.
 * @param timer The time_t value to convert.
 * @return struct tm* The struct tm in local time.
 */
struct tm *localtime(const time_t *timer);

/**
 * @brief Convert struct tm to time_t.
 * @param tm The struct tm to convert.
 * @return time_t The time_t value.
 */
time_t mktime(struct tm *tm);

/**
 * @brief Format a time_t value into a string.
 * @param s The string to store the result.
 * @param max The maximum size of the string.
 * @param format The format string.
 * @param tm The struct tm to format.
 * @return size_t The number of characters written to the string.
 */
size_t strftime(char *restrict s, size_t max, const char *restrict format, const struct tm *restrict tm);

/**
 * @brief Get current time.
 * @param timer The time_t value to store the result.
 * @return time_t The current time.
 */
time_t time(time_t *timer);

/**
 * @brief Get the current time.
 * @param clk_id The clock ID to use.
 * @param tp The struct timespec to store the result.
 * @return int 0 on success, -1 on error.
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp);

/**
 * @brief Get the resolution of the clock.
 * @param clk_id The clock ID to use.
 * @param res The struct timespec to store the result.
 * @return int 0 on success, -1 on error.
 */
int clock_getres(clockid_t clk_id, struct timespec *res);

/**
 * @brief Set the current time.
 * @param clk_id The clock ID to use.
 * @param tp The struct timespec to use.
 * @return int 0 on success, -1 on error.
 */
int clock_settime(clockid_t clk_id, const struct timespec *tp);

#endif