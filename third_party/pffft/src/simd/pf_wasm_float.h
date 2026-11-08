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

#ifndef PF_WASM_FLT_H
#define PF_WASM_FLT_H

/*
  WebAssembly SIMD 128-bit support macros (4x float32)
*/
#if !defined(SIMD_SZ) && !defined(PFFFT_SIMD_DISABLE) && defined(PFFFT_ENABLE_WASM) && defined(__wasm_simd128__)
#pragma message( __FILE__ ": WASM SIMD float macros are defined" )

#  include <wasm_simd128.h>
typedef v128_t v4sf;

#  define SIMD_SZ 4

typedef union v4sf_union {
  v4sf  v;
  float f[SIMD_SZ];
} v4sf_union;

#  define VARCH "WASM-SIMD"
#  define VREQUIRES_ALIGN 0
#  define VZERO() wasm_f32x4_splat(0)
#  define VMUL(a,b) wasm_f32x4_mul(a,b)
#  define VADD(a,b) wasm_f32x4_add(a,b)
#  define VSUB(a,b) wasm_f32x4_sub(a,b)
/* Use relaxed FMA when available, otherwise emulate with mul+add/sub */
#ifdef __wasm_relaxed_simd__
#  define VMADD(a,b,c) wasm_f32x4_relaxed_madd(a,b,c)
#  define VMSUB(a,b,c) wasm_f32x4_relaxed_nmadd(a,b,c)
#else
#  define VMADD(a,b,c) wasm_f32x4_add(c, wasm_f32x4_mul(a,b))
#  define VMSUB(a,b,c) wasm_f32x4_sub(c, wasm_f32x4_mul(a,b))
#endif
#  define LD_PS1(p) wasm_f32x4_splat(p)
#  define VLOAD_UNALIGNED(ptr)  wasm_v128_load((const float*)(ptr))
#  define VLOAD_ALIGNED(ptr)    wasm_v128_load((const float*)(ptr))

#  define INTERLEAVE2(in1, in2, out1, out2) {              \
    v128_t t1__ = wasm_i32x4_shuffle(in1, in2, 0, 4, 1, 5); \
    v128_t t2__ = wasm_i32x4_shuffle(in1, in2, 2, 6, 3, 7); \
    out1 = t1__; out2 = t2__;                                \
  }
#  define UNINTERLEAVE2(in1, in2, out1, out2) {              \
    v128_t t1__ = wasm_i32x4_shuffle(in1, in2, 0, 2, 4, 6); \
    v128_t t2__ = wasm_i32x4_shuffle(in1, in2, 1, 3, 5, 7); \
    out1 = t1__; out2 = t2__;                                \
  }

#  define VTRANSPOSE4(x0, x1, x2, x3) {                         \
    v128_t t0lo_ = wasm_i32x4_shuffle(x0, x2, 0, 4, 1, 5);     \
    v128_t t0hi_ = wasm_i32x4_shuffle(x0, x2, 2, 6, 3, 7);     \
    v128_t t1lo_ = wasm_i32x4_shuffle(x1, x3, 0, 4, 1, 5);     \
    v128_t t1hi_ = wasm_i32x4_shuffle(x1, x3, 2, 6, 3, 7);     \
    x0 = wasm_i32x4_shuffle(t0lo_, t1lo_, 0, 4, 1, 5);         \
    x1 = wasm_i32x4_shuffle(t0lo_, t1lo_, 2, 6, 3, 7);         \
    x2 = wasm_i32x4_shuffle(t0hi_, t1hi_, 0, 4, 1, 5);         \
    x3 = wasm_i32x4_shuffle(t0hi_, t1hi_, 2, 6, 3, 7);         \
  }

#  define VSWAPHL(a,b) __builtin_shufflevector((b), (a), 0, 1, 6, 7)

/* reverse/flip all floats: [0,1,2,3] -> [3,2,1,0] */
#  define VREV_S(a) wasm_i32x4_shuffle(a, a, 3, 2, 1, 0)
/* reverse/flip complex floats: [0,1,2,3] -> [2,3,0,1] */
#  define VREV_C(a) wasm_i64x2_shuffle(a, a, 1, 0)

#  define VALIGNED(ptr) ((((uintptr_t)(ptr)) & 0x3) == 0)

#else
/* #pragma message( __FILE__ ": WASM SIMD float macros are not defined" ) */
#endif

#endif /* PF_WASM_FLT_H */
