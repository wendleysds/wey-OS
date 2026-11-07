#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <sync/atomic.h>

typedef struct {
    atomic_t locked;
} spinlock_t;

#define DEFINE_SPINLOCK(name) spinlock_t name = { .locked = { 0 } }

void spinlock_init(spinlock_t* lock);
void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);

void spin_lock_irqsave(spinlock_t* lock, unsigned long* flags);
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long* flags);

#endif