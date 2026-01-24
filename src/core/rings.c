#include <def/config.h>

extern void _set_resgisters_segments(int);

void user_registers(){
    _set_resgisters_segments(GDT_USER_DATA);
}

void kernel_registers(){
    _set_resgisters_segments(GDT_KERNEL_DATA);
}