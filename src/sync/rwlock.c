#include <sync/rwlock.h>
#include <kernel/sched.h>
#include <lib/assert.h>

void init_rwlock(struct rw_lock *sem) {
    sem->readers = 0;
    sem->writer_active = false;
    sem->writers_waiting = 0;
    spinlock_init(&sem->lock);
    wait_queue_head_init(&sem->wait_list);
}

void down_read(struct rw_lock *sem) {
    unsigned long flags;
    struct wait_queue_entry wait;
    wait_queue_entry_init(&wait, current, task_default_wakeup);

    spin_lock_irqsave(&sem->lock, &flags);

    while (sem->writer_active || sem->writers_waiting > 0) {
        wait_queue_add(&sem->wait_list, &wait);
        task_sleep_uninterruptible(current);

        spin_unlock_irqrestore(&sem->lock, &flags);
        schedule();

        spin_lock_irqsave(&sem->lock, &flags);
        wait_queue_remove(&sem->wait_list, &wait);
    }

    sem->readers++;

    spin_unlock_irqrestore(&sem->lock, &flags);
}

void up_read(struct rw_lock *sem) {
    unsigned long flags;
    spin_lock_irqsave(&sem->lock, &flags);

    sem->readers--;
    BUG_ON(sem->readers < 0);

    if (sem->readers == 0) {
        wake_up_all(&sem->wait_list);
    }

    spin_unlock_irqrestore(&sem->lock, &flags);
}

void down_write(struct rw_lock *sem) {
    unsigned long flags;
    struct wait_queue_entry wait;
    wait_queue_entry_init(&wait, current, task_default_wakeup);

    spin_lock_irqsave(&sem->lock, &flags);
    sem->writers_waiting++;

    while (sem->readers > 0 || sem->writer_active) {
        wait_queue_add(&sem->wait_list, &wait);
        task_sleep_uninterruptible(current);

        spin_unlock_irqrestore(&sem->lock, &flags);
        schedule();
        spin_lock_irqsave(&sem->lock, &flags);

        wait_queue_remove(&sem->wait_list, &wait);
    }

    sem->writers_waiting--;
    sem->writer_active = true;

    spin_unlock_irqrestore(&sem->lock, &flags);
}

void up_write(struct rw_lock *sem) {
    unsigned long flags;
    spin_lock_irqsave(&sem->lock, &flags);

    BUG_ON(!sem->writer_active);
    sem->writer_active = false;

    wake_up_all(&sem->wait_list);

    spin_unlock_irqrestore(&sem->lock, &flags);
}