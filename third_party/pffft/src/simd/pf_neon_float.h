
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

#ifndef PF_NEON_FLT_H
#define PF_NEON_FLT_H

/*
  ARM NEON support macros
*/
#if !defined(SIMD_SZ) && !defined(PFFFT_SIMD_DISABLE) && defined(PFFFT_ENABLE_NEON) && (defined(__arm__) || defined(__aarch64__) || defined(__arm64__))
#pragma message( __FILE__ ": ARM NEON macros are defined" )

#  include <arm_neon.h>
typedef float32x4_t v4sf;

#  define SIMD_SZ 4

typedef union v4sf_union {
  v4sf  v;
  float f[SIMD_SZ];
} v4sf_union;

#  define VARCH "NEON"
#  define VREQUIRES_ALIGN 0  /* usually no alignment required */
#  define VZERO() vdupq_n_f32(0)
#  define VMUL(a,b) vmulq_f32(a,b)
#  define VADD(a,b) vaddq_f32(a,b)
/* Use true fused multiply-add when available (AArch64 always, ARMv7 with VFPv4+) */
#if defined(__aarch64__) || defined(__arm64__) || defined(__ARM_FEATURE_FMA)
#  define VMADD(a,b,c) vfmaq_f32(c,a,b)
#  define VMSUB(a,b,c) vfmsq_f32(c,a,b)
#else
#  define VMADD(a,b,c) vmlaq_f32(c,a,b)
#  define VMSUB(a,b,c) vmlsq_f32(c,a,b)
#endif
#  define VSUB(a,b) vsubq_f32(a,b)
#  define LD_PS1(p) vld1q_dup_f32(&(p))
#  define VLOAD_UNALIGNED(ptr)  vld1q_f32((const float*)(ptr))
#  define VLOAD_ALIGNED(ptr)    vld1q_f32((const float*)(ptr))
#  define INTERLEAVE2(in1, in2, out1, out2) { float32x4x2_t tmp__ = vzipq_f32(in1,in2); out1=tmp__.val[0]; out2=tmp__.val[1]; }
#  define UNINTERLEAVE2(in1, in2, out1, out2) { float32x4x2_t tmp__ = vuzpq_f32(in1,in2); out1=tmp__.val[0]; out2=tmp__.val[1]; }
/* ARMv7: use inline asm (vtrn+vswp) which is faster than intrinsics.
   AArch64: use intrinsics (the 32-bit asm syntax is invalid there). */
#if defined(__arm__) && !defined(__aarch64__) && !defined(__arm64__) && (defined(__GNUC__) || defined(__clang__))
#  define VTRANSPOSE4(x0,x1,x2,x3) { __asm__("vtrn.32 %q0, %q1;\n vtrn.32 %q2,%q3\n vswp %f0,%e2\n vswp %f1,%e3" : "+w"(x0), "+w"(x1), "+w"(x2), "+w"(x3)::); }
#elif defined(__aarch64__) || defined(__arm64__)
/* AArch64: use trn1/trn2 at 32-bit and 64-bit granularity.
   This avoids float32x4x2_t struct intermediates from vzipq, giving
   the compiler single-vector results and better register allocation.
   Step 1: trn1/trn2.4s transpose pairs of 32-bit elements.
   Step 2: trn1/trn2.2d (via f64 reinterpret) swap 64-bit halves. */
#  define VTRANSPOSE4(x0,x1,x2,x3) {                                          \
    float32x4_t t0_ = vtrn1q_f32(x0, x1);                                     \
    float32x4_t t1_ = vtrn2q_f32(x0, x1);                                     \
    float32x4_t t2_ = vtrn1q_f32(x2, x3);                                     \
    float32x4_t t3_ = vtrn2q_f32(x2, x3);                                     \
    x0 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t0_),         \
                                           vreinterpretq_f64_f32(t2_)));       \
    x2 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t0_),         \
                                           vreinterpretq_f64_f32(t2_)));       \
    x1 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t1_),         \
                                           vreinterpretq_f64_f32(t3_)));       \
    x3 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t1_),         \
                                           vreinterpretq_f64_f32(t3_)));       \
  }
#else
/* Fallback for compilers without the ARMv7 inline asm or AArch64 intrinsics paths */
#  define VTRANSPOSE4(x0,x1,x2,x3) {                                    \
    float32x4x2_t t0_ = vzipq_f32(x0, x2);                              \
    float32x4x2_t t1_ = vzipq_f32(x1, x3);                              \
    float32x4x2_t u0_ = vzipq_f32(t0_.val[0], t1_.val[0]);              \
    float32x4x2_t u1_ = vzipq_f32(t0_.val[1], t1_.val[1]);              \
    x0 = u0_.val[0]; x1 = u0_.val[1]; x2 = u1_.val[0]; x3 = u1_.val[1]; \
  }
#endif
#  define VSWAPHL(a,b) vcombine_f32(vget_low_f32(b), vget_high_f32(a))

/* reverse/flip all floats */
#  define VREV_S(a)    vcombine_f32(vrev64_f32(vget_high_f32(a)), vrev64_f32(vget_low_f32(a)))
/* reverse/flip complex floats */
#  define VREV_C(a)    vextq_f32(a, a, 2)

#  define VALIGNED(ptr) ((((uintptr_t)(ptr)) & 0x3) == 0)

#else
/* #pragma message( __FILE__ ": ARM NEON macros are not defined" ) */
#endif

#endif /* PF_NEON_FLT_H */

