/* Copyright (c) 2013  Julien Pommier ( pommier@modartt.com )
   Copyright (c) 2020  Hayati Ayguen ( h_ayguen@web.de )
   Copyright (c) 2020  Dario Mambro ( dario.mambro@gmail.com )

   Based on original fortran 77 code from FFTPACKv4 from NETLIB
   (http://www.netlib.org/fftpack), authored by Dr Paul Swarztrauber
   of NCAR, in 1985.

   As confirmed by the NCAR fftpack software curators, the following
   FFTPACKv5 license applies to FFTPACKv4 sources. My changes are
   released under the same terms.

   FFTPACK license:

   http://www.cisl.ucar.edu/css/software/fftpack5/ftpk.html

   Copyright (c) 2004 the University Corporation for Atmospheric
   Research ("UCAR"). All rights reserved. Developed by NCAR's
   Computational and Information Systems Laboratory, UCAR,
   www.cisl.ucar.edu.

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


   PFFFT : a Pretty Fast FFT.

   This file is largerly based on the original FFTPACK implementation, modified in
   order to take advantage of SIMD instructions of modern CPUs.
*/

/*
   NOTE: This file is adapted from Julien Pommier's original PFFFT,
   which works on 32 bit floating point precision using SSE instructions,
   to work with 64 bit floating point precision using AVX instructions.
   Author: Dario Mambro @ https://github.com/unevens/pffft
*/

#include "pffft_double.h"

/* detect compiler flavour */
#if defined(_MSC_VER)
#  define COMPILER_MSVC
#elif defined(__GNUC__)
#  define COMPILER_GCC
#endif

#ifdef COMPILER_MSVC
#  ifndef _USE_MATH_DEFINES
#    define _USE_MATH_DEFINES
#  endif
#  include <malloc.h>
#elif defined(__MINGW32__) || defined(__MINGW64__)
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#if defined(COMPILER_GCC)
#  define ALWAYS_INLINE(return_type) inline return_type __attribute__ ((always_inline))
#  define NEVER_INLINE(return_type) return_type __attribute__ ((noinline))
#  define RESTRICT __restrict
#  define VLA_ARRAY_ON_STACK(type__, varname__, size__) type__ varname__[size__];
#elif defined(COMPILER_MSVC)
#  define ALWAYS_INLINE(return_type) __forceinline return_type
#  define NEVER_INLINE(return_type) __declspec(noinline) return_type
#  define RESTRICT __restrict
#  define VLA_ARRAY_ON_STACK(type__, varname__, size__) type__ *varname__ = (type__*)_alloca(size__ * sizeof(type__))
#endif


#ifdef COMPILER_MSVC
#pragma warning( disable : 4244 4305 4204 4456 )
#endif

/* 
   vector support macros: the rest of the code is independant of
   AVX -- adding support for other platforms with 4-element
   vectors should be limited to these macros 
*/
#include "simd/pf_double.h"

/* have code comparable with this definition */
#define float double
#define SETUP_STRUCT               PFFFTD_Setup
#define FUNC_NEW_SETUP             pffftd_new_setup
#define FUNC_DESTROY               pffftd_destroy_setup
#define FUNC_TRANSFORM_UNORDRD     pffftd_transform
#define FUNC_TRANSFORM_ORDERED     pffftd_transform_ordered
#define FUNC_ZREORDER              pffftd_zreorder
#define FUNC_ZCONVOLVE_ACCUMULATE  pffftd_zconvolve_accumulate
#define FUNC_ZCONVOLVE              pffftd_zconvolve
#define FUNC_ZCONVERT_ZP            pffftd_zconvert_zp
#define FUNC_ZCONVOLVE_ZP           pffftd_zconvolve_zp
#define FUNC_ZCONVOLVE_SCALE       pffftd_zconvolve_scale

#define FUNC_ALIGNED_MALLOC        pffftd_aligned_malloc
#define FUNC_ALIGNED_FREE          pffftd_aligned_free
#define FUNC_SIMD_SIZE             pffftd_simd_size
#define FUNC_MIN_FFT_SIZE          pffftd_min_fft_size
#define FUNC_IS_VALID_SIZE         pffftd_is_valid_size
#define FUNC_NEAREST_SIZE          pffftd_nearest_transform_size
#define FUNC_SIMD_ARCH             pffftd_simd_arch
#define FUNC_VALIDATE_SIMD_A       validate_pffftd_simd
#define FUNC_VALIDATE_SIMD_EX      validate_pffftd_simd_ex

#define FUNC_CPLX_FINALIZE         pffftd_cplx_finalize
#define FUNC_CPLX_PREPROCESS       pffftd_cplx_preprocess
#define FUNC_REAL_PREPROCESS_4X4   pffftd_real_preprocess_4x4
#define FUNC_REAL_PREPROCESS       pffftd_real_preprocess
#define FUNC_REAL_FINALIZE_4X4     pffftd_real_finalize_4x4
#define FUNC_REAL_FINALIZE         pffftd_real_finalize
#define FUNC_TRANSFORM_INTERNAL    pffftd_transform_internal

#define FUNC_COS  cos
#define FUNC_SIN  sin


#include "pffft_priv_impl.h"



/* deprecated synonym for pffftd_zconvolve_scale() -- kept for ABI compatibility */
void pffftd_zconvolve_no_accu(const PFFFTD_Setup *setup, const double *dft_a, const double *dft_b, double *dft_ab, double scaling) {
  pffftd_zconvolve_scale(setup, dft_a, dft_b, dft_ab, scaling);
}
