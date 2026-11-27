#ifndef _MUTEX_H
#define _MUTEX_H

#include <sync/atomic.h>
#include <kernel/wait.h>

struct task;

struct mutex {
    atomic_t count;
    spinlock_t wait_lock;
    struct wait_queue_head wait_list;
    struct task *owner;
};

#define MUTEX_INIT(mutex) \
    { \
        .count = ATOMIC_INIT(1), \
        .wait_lock = SPINLOCK_INIT(0), \
        .wait_list = WAIT_QUEUE_HEAD_INIT(mutex.wait_list), \
        .owner = NULL \
    }

#define DEFINE_MUTEX(name) \
    struct mutex name = MUTEX_INIT(name)

void mutex_init(struct mutex *lock);
void mutex_lock(struct mutex *lock);
bool mutex_trylock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);

#endif