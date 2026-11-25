/*
    This file is part of the Csound Library.
    Distributed under the GNU Lesser General Public License, version 2.1
    or later.
*/

#ifndef CSOUND_FILTER_MATH_H
#define CSOUND_FILTER_MATH_H

#include <math.h>

/* Scalar and cascaded filters must round the same operations. Use hardware
   FMA where it is fast. Else keep the source order without a library call. */
#if defined(USE_FLOAT) && defined(FP_FAST_FMAF)
  #define CS_FILTER_MADD(a, b, c) fmaf((a), (b), (c))
#else
  #define CS_FILTER_MADD(a, b, c) ((a) * (b) + (c))
#endif

#if defined(USE_FLOAT) && !defined(FP_FAST_FMAF) && defined(__clang__)
  #define CS_FILTER_ORDERED \
    _Pragma("clang fp reassociate(off)") _Pragma("clang fp contract(off)")
#else
  #define CS_FILTER_ORDERED
#endif

#if defined(USE_FLOAT) && !defined(FP_FAST_FMAF) && \
    defined(__GNUC__) && !defined(__clang__)
  #define CS_FILTER_FUNCTION \
    __attribute__((optimize("no-associative-math", "fp-contract=off")))
#else
  #define CS_FILTER_FUNCTION
#endif

#ifdef USE_FLOAT
  #define CS_RESON_STEP(input, y1, y2, c1, c2, c3) \
    CS_FILTER_MADD((c1), (input), CS_FILTER_MADD((c2), (y1), -(c3) * (y2)))
#else
  #define CS_RESON_STEP(input, y1, y2, c1, c2, c3) \
    ((c1) * (input) + (c2) * (y1) - (c3) * (y2))
#endif

#endif
