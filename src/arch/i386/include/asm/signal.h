#ifndef ASM_SIGNAL_H
#define ASM_SIGNAL_H

#include <kernel/signal.h>

int arch_setup_signal_frame(int sig, const struct sigaction *act, struct registers *regs);

#endif /* ASM_SIGNAL_H */