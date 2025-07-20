#include <def/config.h>

extern void _set_resgisters_segments(int);

void user_registers(){
    _set_resgisters_segments(USER_DATA_SEGMENT);
}

void kernel_registers(){
    _set_resgisters_segments(KERNEL_DATA_SELECTOR);
}