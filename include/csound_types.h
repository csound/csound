/* Csound numeric types. See doc/numeric-types.md for the CS7 migration. */
#ifndef CSOUND_TYPES_H
#define CSOUND_TYPES_H

#if !defined(USE_DOUBLE) && !defined(USE_FLOAT)
#include "float-version.h"
#endif

#if defined(USE_DOUBLE) && defined(USE_FLOAT)
#error "USE_DOUBLE and USE_FLOAT cannot be enabled together"
#endif

/* Audio samples and opcode arguments; USE_DOUBLE keeps the CS7 default. */
#ifdef USE_DOUBLE
typedef double cs_float;
#else
typedef float cs_float;
#endif

/* Internal calculations that normally need double precision. */
#ifdef USE_FLOAT
typedef float cs_double;
#define CS_DOUBLE_SCAN "f"
#define cs_modf modff
#else
typedef double cs_double;
#define CS_DOUBLE_SCAN "lf"
#define cs_modf modf
#endif

/* Deprecated in CS7. Keep old host and opcode source code valid. */
#ifndef __MYFLT_DEF
#define __MYFLT_DEF
#if defined(SWIG)
typedef cs_float MYFLT;
#elif defined(_MSC_VER)
typedef __declspec(deprecated("MYFLT is deprecated in CS7; use cs_float")) cs_float MYFLT;
#elif defined(__GNUC__) || defined(__clang__)
typedef cs_float MYFLT __attribute__((deprecated("MYFLT is deprecated in CS7; use cs_float")));
#else
typedef cs_float MYFLT;
#endif
#endif

#endif /* CSOUND_TYPES_H */
