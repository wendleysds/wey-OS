#include <syscall.h>
#include <time.h>

time_t time(time_t *timer){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    if(timer){
        *timer = ts.tv_sec;
    }

    return ts.tv_sec;
}
