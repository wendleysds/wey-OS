#include <unistd.h>

pid_t setpgrp(void) {
    pid_t res = setpgid(0, 0);
    if(res < 0){
        return res;
    }
    return 0;
}