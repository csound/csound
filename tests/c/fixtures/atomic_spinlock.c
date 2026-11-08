/* Exercise the generic backend even on hosts that select an OS lock. */
#include <stdint.h>
typedef int32_t spin_lock_t;
#include "atomic_spinlock.h"

void csoundTestAtomicSpinLock(int32_t *lock) { csoundAtomicSpinLock(lock); }
void csoundTestAtomicSpinUnlock(int32_t *lock) { csoundAtomicSpinUnlock(lock); }
int csoundTestAtomicSpinTryLock(int32_t *lock) { return csoundAtomicSpinTryLock(lock); }
