#ifndef CSOUND_ATOMIC_SPINLOCK_H
#define CSOUND_ATOMIC_SPINLOCK_H

/* The caller supplies spin_lock_t so tests can exercise this backend on
   hosts whose sysdep.h selects a platform lock with a different type. */
static inline void csoundAtomicSpinLock(spin_lock_t *lock)
{
    for (;;) {
        /* CAS replaces expected on failure; reset it before each retry. */
        spin_lock_t expected = 0;
        if (__atomic_compare_exchange_n(lock, &expected, 1, 0,
                                        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return;
    }
}

static inline void csoundAtomicSpinUnlock(spin_lock_t *lock)
{
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

static inline int csoundAtomicSpinTryLock(spin_lock_t *lock)
{
    spin_lock_t expected = 0;
    return __atomic_compare_exchange_n(lock, &expected, 1, 0,
                                       __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

#endif
