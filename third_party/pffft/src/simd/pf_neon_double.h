/*
   Copyright (c) 2020  Dario Mambro ( dario.mambro@gmail.com )
*/

/* Copyright (c) 2013  Julien Pommier ( pommier@modartt.com )

   Redistribution and use of the Software in source and binary forms,
   with or without modification, is permitted provided that the
   following conditions are met:

   - Neither the names of NCAR's Computational and Information Systems
   Laboratory, the University Corporation for Atmospheric Research,
   nor the names of its sponsors or contributors may be used to
   endorse or promote products derived from this Software without
   specific prior written permission.

   - Redistributions of source code must retain the above copyright
   notices, this list of conditions, and the disclaimer below.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions, and the disclaimer below in the
   documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
   SOFTWARE.
*/

#ifndef PF_NEON_DBL_H
#define PF_NEON_DBL_H

/*
  NEON 64bit support macros — native AArch64 implementation.
  Packs 4 doubles into a pair of float64x2_t (2x 128-bit NEON registers).
*/
#if !defined(SIMD_SZ) && !defined(PFFFT_SIMD_DISABLE) && defined(PFFFT_ENABLE_NEON) && (defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64))

#pragma message (__FILE__ ": NEON double macros are defined" )

#include <arm_neon.h>

/* v4sf holds 4 doubles as two NEON 128-bit registers */
typedef struct {
  float64x2_t lo;
  float64x2_t hi;
} v4sf;

/* 4 doubles by simd vector */
#  define SIMD_SZ 4

typedef union v4sf_union {
  v4sf  v;
  double f[SIMD_SZ];
} v4sf_union;

#  define VARCH "NEON"
#  define VREQUIRES_ALIGN 0

#  define VZERO() ((v4sf){ vdupq_n_f64(0), vdupq_n_f64(0) })
#  define VMUL(a,b) ((v4sf){ vmulq_f64((a).lo, (b).lo), vmulq_f64((a).hi, (b).hi) })
#  define VADD(a,b) ((v4sf){ vaddq_f64((a).lo, (b).lo), vaddq_f64((a).hi, (b).hi) })
#  define VMADD(a,b,c) ((v4sf){ vfmaq_f64((c).lo, (a).lo, (b).lo), vfmaq_f64((c).hi, (a).hi, (b).hi) })
#  define VMSUB(a,b,c) ((v4sf){ vfmsq_f64((c).lo, (a).lo, (b).lo), vfmsq_f64((c).hi, (a).hi, (b).hi) })
#  define VSUB(a,b) ((v4sf){ vsubq_f64((a).lo, (b).lo), vsubq_f64((a).hi, (b).hi) })
#  define LD_PS1(p) ((v4sf){ vdupq_n_f64(p), vdupq_n_f64(p) })
#  define VLOAD_UNALIGNED(ptr)  ((v4sf){ vld1q_f64((const double*)(ptr)), vld1q_f64((const double*)(ptr) + 2) })
#  define VLOAD_ALIGNED(ptr)    VLOAD_UNALIGNED(ptr)

/* INTERLEAVE2 (in1, in2, out1, out2) pseudo code:
   out1 = [ in1[0], in2[0], in1[1], in2[1] ]
   out2 = [ in1[2], in2[2], in1[3], in2[3] ]
*/
#  define INTERLEAVE2(in1, in2, out1, out2) {                                  \
    v4sf tmp__;                                                                \
    tmp__.lo = vtrn1q_f64((in1).lo, (in2).lo);                                \
    tmp__.hi = vtrn2q_f64((in1).lo, (in2).lo);                                \
    out2 = (v4sf){ vtrn1q_f64((in1).hi, (in2).hi),                            \
                   vtrn2q_f64((in1).hi, (in2).hi) };                          \
    out1 = tmp__;                                                              \
  }

/* UNINTERLEAVE2(in1, in2, out1, out2) pseudo code:
   out1 = [ in1[0], in1[2], in2[0], in2[2] ]
   out2 = [ in1[1], in1[3], in2[1], in2[3] ]
*/
#  define UNINTERLEAVE2(in1, in2, out1, out2) {                                \
    v4sf tmp__;                                                                \
    tmp__.lo = vtrn1q_f64((in1).lo, (in1).hi);                                \
    tmp__.hi = vtrn1q_f64((in2).lo, (in2).hi);                                \
    out2 = (v4sf){ vtrn2q_f64((in1).lo, (in1).hi),                            \
                   vtrn2q_f64((in2).lo, (in2).hi) };                          \
    out1 = tmp__;                                                              \
  }

/* VTRANSPOSE4 pseudo code: 4x4 matrix transpose
   Uses trn1/trn2.2d to select low/high 64-bit lanes from each pair,
   avoiding vcombine_f64(vget_low/high_f64(...)) verbose patterns.
*/
#  define VTRANSPOSE4(row0, row1, row2, row3) {                                \
    float64x2_t r0lo__ = (row0).lo, r0hi__ = (row0).hi;                       \
    float64x2_t r1lo__ = (row1).lo, r1hi__ = (row1).hi;                       \
    float64x2_t r2lo__ = (row2).lo, r2hi__ = (row2).hi;                       \
    float64x2_t r3lo__ = (row3).lo, r3hi__ = (row3).hi;                       \
    /* trn1.2d picks low lanes, trn2.2d picks high lanes */                    \
    float64x2_t t0lo__ = vtrn1q_f64(r0lo__, r1lo__);                          \
    float64x2_t t2lo__ = vtrn2q_f64(r0lo__, r1lo__);                          \
    float64x2_t t0hi__ = vtrn1q_f64(r0hi__, r1hi__);                          \
    float64x2_t t2hi__ = vtrn2q_f64(r0hi__, r1hi__);                          \
    float64x2_t t1lo__ = vtrn1q_f64(r2lo__, r3lo__);                          \
    float64x2_t t3lo__ = vtrn2q_f64(r2lo__, r3lo__);                          \
    float64x2_t t1hi__ = vtrn1q_f64(r2hi__, r3hi__);                          \
    float64x2_t t3hi__ = vtrn2q_f64(r2hi__, r3hi__);                          \
    (row0) = (v4sf){ t0lo__, t1lo__ };                                         \
    (row1) = (v4sf){ t2lo__, t3lo__ };                                         \
    (row2) = (v4sf){ t0hi__, t1hi__ };                                         \
    (row3) = (v4sf){ t2hi__, t3hi__ };                                         \
  }

/* VSWAPHL(a, b) pseudo code:
   return [ b[0], b[1], a[2], a[3] ]
*/
#  define VSWAPHL(a,b) ((v4sf){ (b).lo, (a).hi })

/* reverse/flip all doubles: [0,1,2,3] -> [3,2,1,0] */
#  define VREV_S(a) ((v4sf){ vextq_f64((a).hi, (a).hi, 1), vextq_f64((a).lo, (a).lo, 1) })

/* reverse/flip complex doubles: [0,1,2,3] -> [2,3,0,1] */
#  define VREV_C(a) ((v4sf){ (a).hi, (a).lo })

#  define VALIGNED(ptr) ((((uintptr_t)(ptr)) & 0xF) == 0)

#endif

#endif /* PF_NEON_DBL_H */
