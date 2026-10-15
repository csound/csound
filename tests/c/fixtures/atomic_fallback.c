/* Exercise sysdep's serialized fallback even on hosts with real atomics.
   This target omits __BUILDING_LIBCSOUND and CSOUND_CSDL_H, so sysdep.h
   does not restore MSVC from the platform macros. */
#undef HAVE_ATOMIC_BUILTIN
#undef MSVC
#include "sysdep.h"
#if defined(MSVC) || defined(HAVE_ATOMIC_BUILTIN)
#error "atomic_fallback.c must use the serialized atomic fallback"
#endif

int csoundTestPlainCompareExchange(long initial, long expected, long desired,
                                   long *after)
{
    volatile long value = initial;
    int result = ATOMIC_CMP_XCH(&value, desired, expected);
    *after = value;
    return result;
}
