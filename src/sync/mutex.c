#include <sync/mutex.h>
#include <kernel/sched.h>
#include <lib/assert.h>

void mutex_init(struct mutex *lock) {
    atomic_set(&lock->count, 1);
    spinlock_init(&lock->wait_lock);
    wait_queue_head_init(&lock->wait_list);
    lock->owner = NULL;
}

void mutex_lock(struct mutex *lock) {
    if (atomic_dec_and_test(&lock->count)) {
        lock->owner = current;
        return;
    }

    unsigned long flags;
    struct wait_queue_entry wait;
    wait_queue_entry_init(&wait, current, task_default_wakeup);

    spin_lock_irqsave(&lock->wait_lock, &flags);

    wait_queue_add(&lock->wait_list, &wait);

    while (1) {
        task_sleep_uninterruptible(current);

        spin_unlock_irqrestore(&lock->wait_lock, &flags);

        schedule();

        spin_lock_irqsave(&lock->wait_lock, &flags);

        if (atomic_read(&lock->count) == 1) {
            atomic_dec(&lock->count);
            lock->owner = current;
            break;
        }
    }

    wait_queue_remove(&lock->wait_list, &wait);
    spin_unlock_irqrestore(&lock->wait_lock, &flags);
}

void mutex_unlock(struct mutex *lock) {
    unsigned long flags;
    spin_lock_irqsave(&lock->wait_lock, &flags);

    BUG_ON(lock->owner != current);
    lock->owner = NULL;
    atomic_set(&lock->count, 1);

    wake_up(&lock->wait_list);
    spin_unlock_irqrestore(&lock->wait_lock, &flags);
}

bool mutex_trylock(struct mutex *lock) {
    if (atomic_dec_and_test(&lock->count)) {
        lock->owner = current;
        return true;
    }

    atomic_inc(&lock->count);
    return false;
}