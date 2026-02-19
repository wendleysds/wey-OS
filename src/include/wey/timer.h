#ifndef _TIMER_H
#define _TIMER_H

struct registers;

typedef void (*timer_callback_t)(struct registers*);

extern int timer_init();
extern void timer_register_callback(timer_callback_t callback);

#endif
