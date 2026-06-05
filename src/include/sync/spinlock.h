#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <sync/atomic.h>

typedef struct {
    atomic_t locked;
} spinlock_t;

void spinlock_init(spinlock_t* lock);
void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);

#endif