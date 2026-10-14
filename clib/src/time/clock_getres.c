#include <syscall.h>
#include <time.h>

int clock_getres(clockid_t clk_id, struct timespec *res){
    return syscall(SYS_clock_getres, clk_id, res);
}