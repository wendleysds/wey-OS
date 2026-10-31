#ifndef X86_TIMER_CAPS
#define X86_TIMER_CAPS

#include <stdbool.h>

struct x86_timer_caps {
    bool hpet_available;
    bool pit_available;
    bool lapic_available;
    bool ioapic_available;
    bool pic_legacy;
};

#endif