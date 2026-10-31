#ifndef _UAPI_TIME_H
#define _UAPI_TIME_H

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3

typedef long long time_t;

struct timespec {
	time_t    tv_sec;
	long long tv_nsec;
};

#endif