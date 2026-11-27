#ifndef _RW_LOCK_H
#define _RW_LOCK_H

#include <sync/spinlock.h>
#include <kernel/wait.h>
#include <kernel/sched.h>
#include <stdbool.h>

struct rw_lock {
    int readers;
    bool writer_active;
    int writers_waiting;
    spinlock_t lock;
    struct wait_queue_head wait_list;
};

#define INIT_RWLOCK(lock) \
    { \
        .readers = 0, \
        .writer_active = false, \
        .writers_waiting = 0, \
        .lock = SPINLOCK_INIT(0), \
        .wait_list = WAIT_QUEUE_HEAD_INIT(lock.wait_list) \
    }

#define DEFINE_RWLOCK(name) \
    struct rw_lock name = INIT_RWLOCK(name)

void init_rwlock(struct rw_lock *sem);

void down_read(struct rw_lock *sem);
bool down_read_trylock(struct rw_lock *sem);
void up_read(struct rw_lock *sem);

void down_write(struct rw_lock *sem);
bool down_write_trylock(struct rw_lock *sem);
void up_write(struct rw_lock *sem);

#endif