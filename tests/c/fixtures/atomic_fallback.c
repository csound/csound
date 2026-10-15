/* Exercise sysdep's serialized fallback even on hosts with real atomics. */
#undef HAVE_ATOMIC_BUILTIN
#undef MSVC
#include "sysdep.h"

int csoundTestPlainCompareExchange(long initial, long expected, long desired,
                                   long *after)
{
    volatile long value = initial;
    int result = ATOMIC_CMP_XCH(&value, desired, expected);
    *after = value;
    return result;
}
