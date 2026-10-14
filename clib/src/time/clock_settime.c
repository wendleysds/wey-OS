#include <syscall.h>
#include <time.h>

int clock_settime(clockid_t clk_id, const struct timespec *tp) {
    return syscall(SYS_clock_settime, clk_id, tp);
}
