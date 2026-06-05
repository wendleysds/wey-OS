#include <sync/spinlock.h>
#include <sync/barrier.h>

void spinlock_init(spinlock_t* lock) {
    atomic_set(&lock->locked, 0);
}

void spin_lock(spinlock_t* lock) {
    while (1) {
        if (atomic_cmpxchg(&lock->locked, 0, 1) == 0){
			break;
		}

        while (atomic_read(&lock->locked)){
			__asm__ volatile("pause");
		}
    }

    smp_mb(); // acquire barrier
}

void spin_unlock(spinlock_t* lock) {
    smp_mb(); // release barrier
    atomic_set(&lock->locked, 0);
}
