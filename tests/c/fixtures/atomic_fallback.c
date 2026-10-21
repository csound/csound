/* Exercise the serial implementation directly on every platform. */
#include "sysdep.h"

int csoundTestPlainCompareExchange(long initial, long expected, long desired,
                                   long *after)
{
    volatile long value = initial;
    int result = csoundCompareExchangeSerial(&value, desired, expected);
    *after = value;
    return result;
}
