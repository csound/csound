/* Compile the real queue code so these tests can count platform lock calls. */
#include "csoundCore.h"

static _Thread_local int32_t checking_locks;
static _Thread_local int32_t lock_calls[3];
static _Thread_local void (*before_log_lock)(void *);
static _Thread_local void *before_log_lock_data;
static _Thread_local void (*on_log_wait)(void *);
static _Thread_local void *on_log_wait_data;

static int32_t checked_compare_exchange(volatile long *value,
                                        long desired, long expected)
{
    void (*hook)(void *) = before_log_lock;
    before_log_lock = NULL;
    if (hook != NULL)
        hook(before_log_lock_data);
    return ATOMIC_CMP_XCH(value, desired, expected);
}

static void checked_sleep(size_t milliseconds)
{
    void (*hook)(void *) = on_log_wait;
    on_log_wait = NULL;
    if (hook != NULL)
        hook(on_log_wait_data);
    csoundSleep(milliseconds);
}

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

#undef ATOMIC_CMP_XCH
#define ATOMIC_CMP_XCH(value, desired, expected) \
    checked_compare_exchange(value, desired, expected)
#define csoundSleep checked_sleep
#define csoundSpinLock checked_spin_lock
#define csoundSpinTryLock checked_spin_try_lock
#define csoundSpinUnLock checked_spin_unlock
#include "../../../Engine/insert.c"
#undef csoundSpinLock
#undef csoundSpinTryLock
#undef csoundSpinUnLock
#undef csoundSleep

/* Stop during formatting, after message dispatch has selected its branch. */
static _Thread_local void (*after_log_format)(void *);
static _Thread_local void *after_log_format_data;

static int32_t checked_vsnprintf(char *str, size_t size, const char *format,
                                 va_list args)
{
    int32_t result = vsnprintf(str, size, format, args);
    void (*hook)(void *) = after_log_format;
    after_log_format = NULL;
    if (hook != NULL)
        hook(after_log_format_data);
    return result;
}

#ifdef vsnprintf
#undef vsnprintf
#endif
#define vsnprintf checked_vsnprintf
#include "../../../Top/csound_messages.c"
#undef vsnprintf

void csound_test_realtime_after_log_format(void (*hook)(void *), void *data)
{
    after_log_format = hook;
    after_log_format_data = data;
}

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

void csound_test_realtime_before_log_lock(void (*hook)(void *), void *data)
{
    before_log_lock = hook;
    before_log_lock_data = data;
}

void csound_test_realtime_on_log_wait(void (*hook)(void *), void *data)
{
    on_log_wait = hook;
    on_log_wait_data = data;
}

int32_t csound_test_realtime_log_dequeue(CSOUND *csound, unsigned long *rp,
                                        message_string_t *message)
{
    return message_string_dequeue(csound, rp, message);
}
