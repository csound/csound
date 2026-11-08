/*
   Copyright (c) 2026  Marton Danoczy ( marton78@gmail.com )
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

#ifndef PF_WASM_DBL_H
#define PF_WASM_DBL_H

/*
  WebAssembly SIMD 128-bit double support macros.
  Packs 4 doubles into a pair of v128_t (2x 128-bit WASM SIMD registers).
*/
#if !defined(SIMD_SZ) && !defined(PFFFT_SIMD_DISABLE) && defined(PFFFT_ENABLE_WASM) && defined(__wasm_simd128__)

#pragma message (__FILE__ ": WASM SIMD double macros are defined" )

#include <wasm_simd128.h>

/* v4sf holds 4 doubles as two WASM SIMD 128-bit registers */
typedef struct {
  v128_t lo;
  v128_t hi;
} v4sf;

/* 4 doubles per simd vector */
#  define SIMD_SZ 4

typedef union v4sf_union {
  v4sf  v;
  double f[SIMD_SZ];
} v4sf_union;

#  define VARCH "WASM-SIMD"
#  define VREQUIRES_ALIGN 0

#  define VZERO() ((v4sf){ wasm_f64x2_splat(0), wasm_f64x2_splat(0) })
#  define VMUL(a,b) ((v4sf){ wasm_f64x2_mul((a).lo, (b).lo), wasm_f64x2_mul((a).hi, (b).hi) })
#  define VADD(a,b) ((v4sf){ wasm_f64x2_add((a).lo, (b).lo), wasm_f64x2_add((a).hi, (b).hi) })
#  define VSUB(a,b) ((v4sf){ wasm_f64x2_sub((a).lo, (b).lo), wasm_f64x2_sub((a).hi, (b).hi) })
/* Use relaxed FMA when available, otherwise emulate with mul+add/sub */
#ifdef __wasm_relaxed_simd__
#  define VMADD(a,b,c) ((v4sf){ wasm_f64x2_relaxed_madd((a).lo,(b).lo,(c).lo), wasm_f64x2_relaxed_madd((a).hi,(b).hi,(c).hi) })
#  define VMSUB(a,b,c) ((v4sf){ wasm_f64x2_relaxed_nmadd((a).lo,(b).lo,(c).lo), wasm_f64x2_relaxed_nmadd((a).hi,(b).hi,(c).hi) })
#else
#  define VMADD(a,b,c) ((v4sf){ wasm_f64x2_add((c).lo, wasm_f64x2_mul((a).lo,(b).lo)), wasm_f64x2_add((c).hi, wasm_f64x2_mul((a).hi,(b).hi)) })
#  define VMSUB(a,b,c) ((v4sf){ wasm_f64x2_sub((c).lo, wasm_f64x2_mul((a).lo,(b).lo)), wasm_f64x2_sub((c).hi, wasm_f64x2_mul((a).hi,(b).hi)) })
#endif
#  define LD_PS1(p) ((v4sf){ wasm_f64x2_splat(p), wasm_f64x2_splat(p) })
#  define VLOAD_UNALIGNED(ptr)  ((v4sf){ wasm_v128_load((const double*)(ptr)), wasm_v128_load((const double*)(ptr) + 2) })
#  define VLOAD_ALIGNED(ptr)    VLOAD_UNALIGNED(ptr)

/* INTERLEAVE2 (in1, in2, out1, out2) pseudo code:
   out1 = [ in1[0], in2[0], in1[1], in2[1] ]
   out2 = [ in1[2], in2[2], in1[3], in2[3] ]
*/
#  define INTERLEAVE2(in1, in2, out1, out2) {                                \
    v4sf t1__ = { wasm_i64x2_shuffle((in1).lo, (in2).lo, 0, 2),             \
                  wasm_i64x2_shuffle((in1).lo, (in2).lo, 1, 3) };           \
    v4sf t2__ = { wasm_i64x2_shuffle((in1).hi, (in2).hi, 0, 2),             \
                  wasm_i64x2_shuffle((in1).hi, (in2).hi, 1, 3) };           \
    out1 = t1__; out2 = t2__;                                               \
  }

/* UNINTERLEAVE2(in1, in2, out1, out2) pseudo code:
   out1 = [ in1[0], in1[2], in2[0], in2[2] ]
   out2 = [ in1[1], in1[3], in2[1], in2[3] ]
*/
#  define UNINTERLEAVE2(in1, in2, out1, out2) {                              \
    v4sf t1__ = { wasm_i64x2_shuffle((in1).lo, (in1).hi, 0, 2),             \
                  wasm_i64x2_shuffle((in2).lo, (in2).hi, 0, 2) };           \
    v4sf t2__ = { wasm_i64x2_shuffle((in1).lo, (in1).hi, 1, 3),             \
                  wasm_i64x2_shuffle((in2).lo, (in2).hi, 1, 3) };           \
    out1 = t1__; out2 = t2__;                                               \
  }

/* VTRANSPOSE4 pseudo code: 4x4 matrix transpose */
#  define VTRANSPOSE4(row0, row1, row2, row3) {                              \
    v128_t r0lo__ = (row0).lo, r0hi__ = (row0).hi;                          \
    v128_t r1lo__ = (row1).lo, r1hi__ = (row1).hi;                          \
    v128_t r2lo__ = (row2).lo, r2hi__ = (row2).hi;                          \
    v128_t r3lo__ = (row3).lo, r3hi__ = (row3).hi;                          \
    (row0) = (v4sf){ wasm_i64x2_shuffle(r0lo__, r1lo__, 0, 2),              \
                     wasm_i64x2_shuffle(r2lo__, r3lo__, 0, 2) };            \
    (row1) = (v4sf){ wasm_i64x2_shuffle(r0lo__, r1lo__, 1, 3),              \
                     wasm_i64x2_shuffle(r2lo__, r3lo__, 1, 3) };            \
    (row2) = (v4sf){ wasm_i64x2_shuffle(r0hi__, r1hi__, 0, 2),              \
                     wasm_i64x2_shuffle(r2hi__, r3hi__, 0, 2) };            \
    (row3) = (v4sf){ wasm_i64x2_shuffle(r0hi__, r1hi__, 1, 3),              \
                     wasm_i64x2_shuffle(r2hi__, r3hi__, 1, 3) };            \
  }

/* VSWAPHL(a, b) pseudo code:
   return [ b[0], b[1], a[2], a[3] ]
*/
#  define VSWAPHL(a,b) ((v4sf){ (b).lo, (a).hi })

/* reverse/flip all doubles: [0,1,2,3] -> [3,2,1,0] */
#  define VREV_S(a) ((v4sf){ wasm_i64x2_shuffle((a).hi, (a).hi, 1, 0), wasm_i64x2_shuffle((a).lo, (a).lo, 1, 0) })

/* reverse/flip complex doubles: [0,1,2,3] -> [2,3,0,1] */
#  define VREV_C(a) ((v4sf){ (a).hi, (a).lo })

#  define VALIGNED(ptr) ((((uintptr_t)(ptr)) & 0xF) == 0)

#endif

#endif /* PF_WASM_DBL_H */
