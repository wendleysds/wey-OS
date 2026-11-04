#include <time.h>
#include <stdint.h>

clock_t clock(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            return (clock_t)-1;
        }
    }
    
    // ts.tv_sec * CLOCKS_PER_SEC + (ts.tv_nsec * CLOCKS_PER_SEC) / 1000000000LL
    uint64_t sec_clocks = (uint64_t)ts.tv_sec * CLOCKS_PER_SEC;
    uint64_t nsec_clocks = ((uint64_t)ts.tv_nsec * CLOCKS_PER_SEC) / 1000000000ULL;

    return (clock_t)(sec_clocks + nsec_clocks);
}