/* Compile the real queue so a test can advance it between counter loads. */
#include "csoundCore.h"

static int32_t count_mutex_calls;
static int32_t mutex_calls[3];

static void checked_mutex_lock(void *mutex)
{
    if (count_mutex_calls)
        ++mutex_calls[0];
    csoundLockMutex(mutex);
}

static int32_t checked_mutex_try_lock(void *mutex)
{
    if (count_mutex_calls)
        ++mutex_calls[1];
    return csoundLockMutexNoWait(mutex);
}

static void checked_mutex_unlock(void *mutex)
{
    if (count_mutex_calls)
        ++mutex_calls[2];
    csoundUnlockMutex(mutex);
}

static CSOUND *checked_csound;
static int32_t write_loads;
static void (*after_write_load)(CSOUND *);

static long original_load(volatile long *value)
{
    return ATOMIC_GET(*value);
}

static long checked_load(volatile long *value)
{
    long result = original_load(value);
    if (checked_csound != NULL && value == &checked_csound->msg_queue_write &&
        --write_loads == 0) {
        CSOUND *csound = checked_csound;
        checked_csound = NULL;
        after_write_load(csound);
    }
    return result;
}

#undef ATOMIC_GET
#define ATOMIC_GET(value) checked_load(&(value))
#define csoundLockMutex checked_mutex_lock
#define csoundLockMutexNoWait checked_mutex_try_lock
#define csoundUnlockMutex checked_mutex_unlock
#include "../../../Top/threadsafe.c"
#undef csoundLockMutex
#undef csoundLockMutexNoWait
#undef csoundUnlockMutex
#undef ATOMIC_GET

void csound_test_api_mutex_begin(void)
{
    /* Keep the interceptor available even when production has no try-lock. */
    (void) checked_mutex_try_lock;
    memset(mutex_calls, 0, sizeof(mutex_calls));
    count_mutex_calls = 1;
}

void csound_test_api_mutex_end(int32_t *locks, int32_t *tries, int32_t *unlocks)
{
    count_mutex_calls = 0;
    *locks = mutex_calls[0];
    *tries = mutex_calls[1];
    *unlocks = mutex_calls[2];
}

void csound_test_after_api_write_load(CSOUND *csound, int32_t load,
                                     void (*callback)(CSOUND *))
{
    checked_csound = csound;
    write_loads = load;
    after_write_load = callback;
}
