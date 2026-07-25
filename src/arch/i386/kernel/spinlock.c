#include <sync/spinlock.h>
#include <sync/barrier.h>
#include <asm/cpuflags.h>

static __always_inline void  local_irq_enable(){
	__asm__ volatile ("sti" ::: "memory");
}

static __always_inline void local_irq_disable(){
	__asm__ volatile ("cli" ::: "memory");
}

static __always_inline unsigned long local_save_flags(void) {
	unsigned long flags;

	asm volatile(
		"pushf ; pop %0"
		: "=rm" (flags)
		:: "memory"
	);

	return flags;
}

static __always_inline void local_restore_flags(unsigned long flags){
	if(flags & X86_EFLAGS_IF){
		local_irq_enable();
	}
}

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

void spin_lock_irqsave(spinlock_t* lock, unsigned long* flags){
    *flags = local_save_flags();
    local_irq_disable();
    spin_lock(lock);
}

void spin_unlock_irqrestore(spinlock_t* lock, unsigned long* flags){
    spin_unlock(lock);
    local_restore_flags(*flags);
}
