/*
    sysdep.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_SYSDEP_H
#define CSOUND_SYSDEP_H

/* check for the presence of a modern compiler (for use of certain features) */
#if defined(WIN32)
#if !defined(locale_t)
typedef void *locale_t;
#endif
#endif

#include <limits.h>
/* this checks for 64BIT builds */
#if defined(__MACH__) || defined(LINUX)
#if ( __WORDSIZE == 64 ) || defined(__x86_64__) || defined(__amd64__)
#define B64BIT
#endif
#endif

#if defined(WIN32)
#if _WIN64
#define B64BIT
#endif
#endif



#ifdef HAVE_GCC3
#  undef HAVE_GCC3
#endif
#ifdef HAVE_C99
#  undef HAVE_C99
#endif
#if (defined(__GNUC__) && (__GNUC__ >= 3))
#  define HAVE_C99 1
#  if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)
#    ifndef _ISOC99_SOURCE
#      define _ISOC99_SOURCE  1
#    endif
#    ifndef _ISOC9X_SOURCE
#      define _ISOC9X_SOURCE  1
#    endif
#  endif
#  if !(defined(__MACH__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2))
#    define HAVE_GCC3 1
#  endif
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#  define HAVE_C99 1
#endif

#if defined(__GNUC__)
# if defined(__GNUC_PATCHLEVEL__)
#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
                            + __GNUC_MINOR__ * 100 \
                            + __GNUC_PATCHLEVEL__)
# else
#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
                            + __GNUC_MINOR__ * 100)
# endif
#endif

#ifndef CABBAGE
#ifdef MSVC
typedef __int32 int32;
typedef __int16 int16;
typedef unsigned __int32 uint32;
typedef unsigned __int16 uint16;
#else
#include <stdint.h>
#if defined(__HAIKU__) && defined(__HAIKU_CONFLICT)
 /* Haiku has different typedefs -- relevant to rthaiku and network*/
 #include <SupportDefs.h>
#else
typedef int_least32_t int32;
typedef int_least64_t int64;
typedef int_least16_t int16;
typedef uint_least32_t uint32;
typedef uint_least16_t uint16;
#endif
#endif
#endif

#if defined(HAVE_PTHREAD_SPIN_LOCK)
#include <pthread.h>
#endif

#ifdef __MACH__
#include <AvailabilityMacros.h>
#endif

#if !defined(USE_DOUBLE)
#if !defined(_MSC_VER)
#include "float-version.h"
#else
#define USE_DOUBLE
#endif
#endif

#ifdef USE_DOUBLE
/* Defined here as Android does not have log2 functions */
#define MYRECIPLN2  1.442695040888963407359924681001892137426 /* 1.0/log(2) */
#define LOG2(a) (MYRECIPLN2*log(a))       /* floating point logarithm base 2 */

  #define ACOS acos
  #define ASIN asin
  #define ATAN atan
  #define ATAN2 atan2
  #define COS cos
  #define SIN sin
  #define TAN tan
  #define COSH cosh
  #define SINH sinh
  #define TANH tanh
  #define ACOSH acosh
  #define ASINH asinh
  #define ATANH atanh
  #define EXP exp
  #define LOG log
  #define LOG10 log10
  /* #define LOG2 log2 */
  #define POWER pow
  #define SQRT sqrt
  #define HYPOT hypot
  #define FABS fabs
  #define FLOOR floor
  #define CEIL ceil
  #define FMOD fmod
  #define MODF modf
#else
/* Defined here as Android does not have log2 functions */
#define MYRECIPLN2  1.442695040888963407359924681001892137426 /* 1.0/log(2) */
#define LOG2(a) (MYRECIPLN2*logf(a))       /* floating point logarithm base 2 */

  #define ACOS acosf
  #define ASIN asinf
  #define ATAN atanf
  #define ATAN2 atan2f
  #define COS cosf
  #define SIN sinf
  #define TAN tanf
  #define COSH coshf
  #define SINH sinhf
  #define TANH tanhf
  #define ACOSH acoshf
  #define ASINH asinhf
  #define ATANH atanhf
  #define EXP expf
  #define LOG logf
  #define LOG10 log10f
  /* #define LOG2 log2f */
  #define POWER powf
  #define SQRT sqrtf
  #define HYPOT hypotf
  #define FABS(x) fabsf(FL(x))
  #define FLOOR floorf
  #define CEIL ceilf
  #define FMOD fmodf
  #define MODF modff
#endif

// #include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if defined(HAVE_FCNTL_H) || defined(__unix) || defined(__unix__)
#include <fcntl.h>
#endif
#if defined(HAVE_UNISTD_H) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#endif

/* Experiment with doubles or floats */

#ifndef __MYFLT_DEF
#  define __MYFLT_DEF
#  ifndef USE_DOUBLE
#    define MYFLT float
#  else
#    define MYFLT double
#  endif
#endif

/* Aligning to double boundaries, should work with MYFLT as float or double */
#define CS_FLOAT_ALIGN(x) ((int)(x + sizeof(MYFLT)-1) & (~(sizeof(MYFLT)-1)))

#if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)

#define FL(x) ((MYFLT) (x))

/* find out operating system if not specified on the command line */

#if defined(_WIN32) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32 1
#  endif
#elif (defined(linux) || defined(__linux)) && !defined(LINUX)
#  define LINUX 1
#endif

#if defined(WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
#  ifndef MSVC
#    define MSVC 1
#  endif
#elif defined(MSVC)
#  undef MSVC
#endif

/* inline keyword: always available in C++, C99, and GCC 3.x and above */
/* add any other compiler that supports 'inline' */

#if !(defined(__cplusplus) || defined(inline))
#  if defined(HAVE_C99) || defined(HAVE_GCC3)
#    if defined(__GNUC__) && defined(__STRICT_ANSI__)
#      define inline __inline__
#    endif
#  elif defined(MSVC)
#    define inline  __inline
#  else
#    define inline
#  endif
#endif

#define DIRSEP '/'
#ifdef WIN32
#  undef  DIRSEP
#  define DIRSEP '\\'
#  if !defined(O_NDELAY)
#    define  O_NDELAY (0)
#  endif
#  include <io.h>
#else
#  ifdef DOSGCC
#    if !defined(O_NDELAY)
#      define  O_NDELAY (0)
#    endif
#  endif
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif
/*  RWD for WIN32 on VC++ */
#endif
#ifndef MSVC
#  include <sys/file.h>
#endif
#include <sys/stat.h>

#endif  /* __BUILDING_LIBCSOUND || CSOUND_CSDL_H */

#ifdef WIN32
#  define ENVSEP ';'
#else
#  define ENVSEP ':'
#endif
/* standard integer types */

#if defined(USE_GUSI2)
/* When compiling with GUSI on MacOS 9 (for Python),  */
/* all of the other integer types are already defined */
typedef int64_t             int_least64_t;
typedef uint64_t            uint_least64_t;
#elif defined(HAVE_STDINT_H) || defined(HAVE_C99)
#  include <stdint.h>

#    if defined(__CYGWIN__)
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long
#    endif
#else
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
#  if defined(__GNUC__) || !defined(WIN32)
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
typedef long long           int_least64_t;
typedef unsigned long long  uint_least64_t;
#  else
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
typedef __int64             int_least64_t;
typedef unsigned __int64    uint_least64_t;
#  endif
#if !defined(_MSC_VER)
typedef long                intptr_t;
typedef unsigned long       uintptr_t;
#endif
#endif      /* !(USE_GUSI2 || HAVE_STDINT_H || HAVE_C99) */



/* function attributes */

#if defined(HAVE_GCC3) && !defined(SWIG)
/* deprecated function, variable, or type that is to be removed eventually */
#  define CS_DEPRECATED __attribute__ ((__deprecated__))
/* a function that should not be inlined */
#  define CS_NOINLINE   __attribute__ ((__noinline__))
/* a function that never returns (e.g. csoundDie()) */
#  define CS_NORETURN   __attribute__ ((__noreturn__))
/* printf-style function with first argument as format string */
#  define CS_PRINTF1    __attribute__ ((__format__ (__printf__, 1, 2)))
/* printf-style function with second argument as format string */
#  define CS_PRINTF2    __attribute__ ((__format__ (__printf__, 2, 3)))
/* printf-style function with third argument as format string */
#  define CS_PRINTF3    __attribute__ ((__format__ (__printf__, 3, 4)))
/* a function with no side effects or dependencies on volatile data */
#  define CS_PURE       __attribute__ ((__pure__))
#else
#  define CS_DEPRECATED
#  define CS_NOINLINE
#  define CS_NORETURN
#  define CS_PRINTF1
#  define CS_PRINTF2
#  define CS_PRINTF3
#  define CS_PURE
#endif
#if defined(__clang__) ||  defined(HAVE_GCC3)
#  define LIKELY(x)     __builtin_expect(!!(x),1)
#  define UNLIKELY(x)   __builtin_expect(!!(x),0)
#else
#  define LIKELY(x)     x
#  define UNLIKELY(x)   x
#endif

#if defined(__BUILDING_LIBCSOUND) || defined(CSOUND_CSDL_H)

/* macros for converting floats to integers */
/* MYFLT2LONG: converts with unspecified rounding */
/* MYFLT2LRND: rounds to nearest integer */

#ifdef USE_LRINT
#  ifndef USE_DOUBLE
#    define MYFLT2LONG(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int32) lrintf((float) (x)) : 0)
#    define MYFLT2LRND(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int32) lrintf((float) (x)) : 0)
#  else
#    define MYFLT2LONG(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int32) lrint((double) (x)) : 0)
#    define MYFLT2LRND(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int32) lrint((double) (x)) : 0)
#    define MYFLT2LONG64(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int64_t) lrintl((double) (x)) : 0)
#    define MYFLT2LRND64(x) (x > LONG_MIN && x < LONG_MAX ? \
                           (int64_t) lrintl((double) (x)) : 0)
#  endif
#elif defined(MSVC)
#include <emmintrin.h>
#  ifndef USE_DOUBLE
// From Agner Fog optimisation manuals p.144
static inline int MYFLT2LONG (float const x) {
    return _mm_cvtss_si32 (_mm_load_ss (&x));
}

static inline int MYFLT2LRND (float const x) {
    return _mm_cvtss_si32 (_mm_load_ss (&x));
}

#  else
static inline int MYFLT2LONG (double const x) {
    return _mm_cvtsd_si32 (_mm_load_sd (&x));
}

static inline int MYFLT2LRND (double const x) {
    return _mm_cvtsd_si32 (_mm_load_sd (&x));
}
#  endif
#else
#  ifndef USE_DOUBLE
#    define MYFLT2LONG(x) ((int32) (x))
#    if defined(HAVE_GCC3) && defined(__i386__) && !defined(__ICC)
#      define MYFLT2LRND(x) ((int32) lrintf((float) (x)))
#    else
static inline int32 MYFLT2LRND(float fval)
{
    return ((int32) (fval + (fval < 0.0f ? -0.5f : 0.5f)));
}
#    endif
#  else
#    define MYFLT2LONG(x) ((int32) (x))
#    if defined(HAVE_GCC3) && defined(__i386__) && !defined(__ICC)
#      define MYFLT2LRND(x) ((int32) lrint((double) (x)))
#    else

static inline int32 MYFLT2LRND(double fval)
{
    return ((int32) (fval + (fval < 0.0 ? -0.5 : 0.5)));
}

static inline int64 MYFLT2LRND64(double fval)
{
    return ((int64) (fval + (fval < 0.0 ? -0.5 : 0.5)));
}

#    endif
#  endif
#endif

/* inline functions and macros for clamping denormals to zero */

#if defined(__i386__) || defined(MSVC)
static inline float csoundUndenormalizeFloat(float x)
{
    volatile float  tmp = 1.0e-30f;
    return ((x + 1.0e-30f) - tmp);
}

static inline double csoundUndenormalizeDouble(double x)
{
    volatile double tmp = 1.0e-200;
    return ((x + 1.0e-200) - tmp);
}
#else
#  define csoundUndenormalizeFloat(x)   x
#  define csoundUndenormalizeDouble(x)  x
#endif

#ifndef USE_DOUBLE
#  define csoundUndenormalizeMYFLT      csoundUndenormalizeFloat
#else
#  define csoundUndenormalizeMYFLT      csoundUndenormalizeDouble
#endif

#endif  /* __BUILDING_LIBCSOUND || CSOUND_CSDL_H */

// This is wrong.....  needs thought
/* #ifdef HAVE_SPRINTF_L */
/* # define CS_SPRINTF sprintf_l */
/* #elseif HAVE__SPRINT_L */
/*   /\* this would be the case for the Windows locale aware function *\/ */
/* # define CS_SPRINTF _sprintf_l */
/* #else */
# define CS_SPRINTF cs_sprintf
# define CS_SSCANF cs_sscanf
/* #endif */

#if !defined(HAVE_STRLCAT) && !defined(strlcat)
size_t strlcat(char *dst, const char *src, size_t siz);
#endif
char *strNcpy(char *dst, const char *src, size_t siz);

/* atomics */
#if defined(MSVC)
#define ATOMIC_SET(var, val)  InterlockedExchange(&var, val);
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_SET(var, val) __atomic_store_n(&var, val, __ATOMIC_SEQ_CST);
#else
#define ATOMIC_SET(var, val) var = val;
#endif

#if defined(MSVC)
#define ATOMIC_SET8(var, val)  InterlockedExchange8(&var, val);
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_SET8(var, val) __atomic_store_n(&var, val,__ATOMIC_SEQ_CST);
#else
#define ATOMIC_SET8(var, val) var = val;
#endif

#ifdef MSVC
#define ATOMIC_GET(var) InterlockedExchangeAdd(&var, 0)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_GET(var) __atomic_load_n(&var, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_GET(var) var
#endif

#ifdef MSVC
#define ATOMIC_GET8(var) InterlockedExchangeAdd8(&var, 0)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_GET8(var) __atomic_load_n(&var, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_GET8(var) var
#endif

#ifdef MSVC
#define ATOMIC_DECR(var) InterlockedExchangeAdd(&var, -1)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_DECR(var) __atomic_sub_fetch(&var, 1, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_DECR(var) var -= 1
#endif

#ifdef MSVC
#define ATOMIC_INCR(var) InterlockedExchangeAdd(&var, 1)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_INCR(var) __atomic_add_fetch(&var, 1, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_INCR(var) var += 1
#endif

#ifdef MSVC
#define ATOMIC_SUB(var, val) InterlockedExchangeAdd(&var, -val)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_SUB(var, val) __atomic_sub_fetch(&var, val, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_SUB(var, val) var -= val
#endif

#ifdef MSVC
#define ATOMIC_ADD(var, val) InterlockedExchangeAdd(&var, val)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_ADD(var, val) __atomic_add_fetch(&var, val, __ATOMIC_SEQ_CST)
#else
#define ATOMIC_ADD(var, val) var += val
#endif

#if defined(MSVC)
#define ATOMIC_CMP_XCH(val, newVal, oldVal) \
  (InterlockedCompareExchange(val, newVal, oldVal) != oldVal)
#elif defined(HAVE_ATOMIC_BUILTIN)
#define ATOMIC_CMP_XCH(val, newVal, oldVal) \
  !(__atomic_compare_exchange(val, (long *) &oldVal, &newVal, 0,        \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
#else /* FIXME: no atomics, what to do? */
#define ATOMIC_CMP_XCH(val, newVal, oldVal) (*val = newVal) != oldVal
#endif

#if defined(WIN32)
typedef int32_t spin_lock_t;
#define SPINLOCK_INIT 0

#elif defined(MACOSX)
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
#include <os/lock.h>
typedef struct os_unfair_lock_s spin_lock_t;
#define SPINLOCK_INIT {0}
#else
#include <libkern/OSAtomic.h>
typedef int32_t spin_lock_t;
#define SPINLOCK_INIT 0
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#elif defined(__GNUC__) && defined(HAVE_PTHREAD_SPIN_LOCK)
typedef pthread_spinlock_t spin_lock_t;
#define SPINLOCK_INIT PTHREAD_SPINLOCK_INITIALIZER
#elif defined(__GNUC__) && defined(HAVE_ATOMIC_BUILTIN)
typedef char spin_lock_t;
#define SPINLOCK_INIT 0

#else
typedef int32_t spin_lock_t;
#define SPINLOCK_INIT 0
#endif

/* The ignore_value() macro is taken from GNULIB ignore-value.h,
   licensed under the terms of the LGPLv2+
   Normally casting an expression to void discards its value, but GCC
   versions 3.4 and newer have __attribute__ ((__warn_unused_result__))
   which may cause unwanted diagnostics in that case.  Use __typeof__
   and __extension__ to work around the problem, if the workaround is
   known to be needed.  */
#if 3 < __GNUC__ + (4 <= __GNUC_MINOR__)
# define ignore_value(x) \
    (__extension__ ({ __typeof__ (x) __x = (x); (void) __x; }))
#else
# define ignore_value(x) ((void) (x))
#endif


#endif  /* CSOUND_SYSDEP_H */
