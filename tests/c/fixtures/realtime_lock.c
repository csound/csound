/* Compile the real queue code so these tests can count platform lock calls. */
#include "csoundCore.h"

static _Thread_local int32_t checking_locks;
static _Thread_local int32_t lock_calls[3];

static void checked_spin_lock(spin_lock_t *lock)
{
    if (checking_locks)
        ++lock_calls[0];
    csoundSpinLock(lock);
}

static int32_t __attribute__((unused)) checked_spin_try_lock(spin_lock_t *lock)
{
    if (checking_locks)
        ++lock_calls[1];
    return csoundSpinTryLock(lock);
}

static void checked_spin_unlock(spin_lock_t *lock)
{
    if (checking_locks)
        ++lock_calls[2];
    csoundSpinUnLock(lock);
}

#define csoundSpinLock checked_spin_lock
#define csoundSpinTryLock checked_spin_try_lock
#define csoundSpinUnLock checked_spin_unlock
#include "../../../Engine/insert.c"
#undef csoundSpinLock
#undef csoundSpinTryLock
#undef csoundSpinUnLock

void csound_test_realtime_locks_begin(void)
{
    memset(lock_calls, 0, sizeof(lock_calls));
    checking_locks = 1;
}

void csound_test_realtime_locks_end(int32_t *locks, int32_t *tries,
                                   int32_t *unlocks)
{
    checking_locks = 0;
    *locks = lock_calls[0];
    *tries = lock_calls[1];
    *unlocks = lock_calls[2];
}
