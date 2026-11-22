#ifndef _X86_FPU_H
#define _X86_FPU_H

#include <stdint.h>
#include <stdbool.h>

struct task;

struct fpu_state {
    uint8_t data[108];
};

int  fpu_init(void);
void fpu_switch_to(struct task *to);
void fpu_flush(struct task *t);
void fpu_copy(struct task *dst, struct task *src);
void fpu_reset(struct task *t);
void fpu_release(struct task *t);
void fpu_set_state(struct task *t, const struct fpu_state *s, bool used);

#endif