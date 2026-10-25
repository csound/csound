/* Copyright (c) 2013  Julien Pommier ( pommier@modartt.com ) 

   Based on original fortran 77 code from FFTPACKv4 from NETLIB,
   authored by Dr Paul Swarztrauber of NCAR, in 1985.

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
*/
   
/*
   PFFFT : a Pretty Fast FFT.

   This is basically an adaptation of the single precision fftpack
   (v4) as found on netlib taking advantage of SIMD instruction found
   on cpus such as intel x86 (SSE1), powerpc (Altivec), and arm (NEON).
   
   For architectures where no SIMD instruction is available, the code
   falls back to a scalar version.  

   Restrictions: 

   - 1D transforms only, with 32-bit single precision.

   - supports only transforms for inputs of length N of the form
   N=(2^a)*(3^b)*(5^c), a >= 5, b >=0, c >= 0 (32, 48, 64, 96, 128,
   144, 160, etc are all acceptable lengths). Performance is best for
   128<=N<=8192.

   - all (float*) pointers in the functions below are expected to
   have an "simd-compatible" alignment, that is 16 bytes on x86 and
   powerpc CPUs.
  
   You can allocate such buffers with the functions
   pffft_aligned_malloc / pffft_aligned_free (or with stuff like
   posix_memalign..)

*/

#ifndef PFFFT_H
#define PFFFT_H

#include <stddef.h> /* for size_t */

#ifndef PFFFT_EXPORT
  #ifdef PFFFT_STATIC_DEFINE
    #define PFFFT_EXPORT
  #elif defined(_WIN32) || defined(__CYGWIN__)
    #ifdef PFFFT_EXPORTS
      #define PFFFT_EXPORT __declspec(dllexport)
    #else
      #define PFFFT_EXPORT __declspec(dllimport)
    #endif
  #elif defined(PFFFT_EXPORTS) && (defined(__GNUC__) || defined(__clang__))
    #define PFFFT_EXPORT __attribute__((visibility("default")))
  #else
    #define PFFFT_EXPORT
  #endif
#endif

#ifndef PFFFT_DEPRECATED
  #if defined(__GNUC__) || defined(__clang__)
    #define PFFFT_DEPRECATED __attribute__((deprecated))
  #elif defined(_MSC_VER)
    #define PFFFT_DEPRECATED __declspec(deprecated)
  #else
    #define PFFFT_DEPRECATED
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /* opaque struct holding internal stuff (precomputed twiddle factors)
     this struct can be shared by many threads as it contains only
     read-only data.
  */
  typedef struct PFFFT_Setup PFFFT_Setup;

#ifndef PFFFT_COMMON_ENUMS
#define PFFFT_COMMON_ENUMS

  /* direction of the transform */
  typedef enum { PFFFT_FORWARD, PFFFT_BACKWARD } pffft_direction_t;
  
  /* type of transform */
  typedef enum { PFFFT_REAL, PFFFT_COMPLEX } pffft_transform_t;

#endif

  /*
    prepare for performing transforms of size N -- the returned
    PFFFT_Setup structure is read-only so it can safely be shared by
    multiple concurrent threads. 
  */
  PFFFT_EXPORT PFFFT_Setup *pffft_new_setup(int N, pffft_transform_t transform);
  PFFFT_EXPORT void pffft_destroy_setup(PFFFT_Setup *);
  /* 
     Perform a Fourier transform , The z-domain data is stored in the
     most efficient order for transforming it back, or using it for
     convolution. If you need to have its content sorted in the
     "usual" way, that is as an array of interleaved complex numbers,
     either use pffft_transform_ordered , or call pffft_zreorder after
     the forward fft, and before the backward fft.

     Transforms are not scaled: PFFFT_BACKWARD(PFFFT_FORWARD(x)) = N*x.
     Typically you will want to scale the backward transform by 1/N.

     The 'work' pointer should point to an area of N (2*N for complex
     fft) floats, properly aligned. If 'work' is NULL, then stack will
     be used instead (this is probably the best strategy for small
     FFTs, say for N < 16384). Threads usually have a small stack, that
     there's no sufficient amount of memory, usually leading to a crash!
     Use the heap with pffft_aligned_malloc() in this case.

     For a real forward transform (PFFFT_REAL | PFFFT_FORWARD) with real
     input with input(=transformation) length N, the output array is
     'mostly' complex:
       index k in 1 .. N/2 -1  corresponds to frequency k * Samplerate / N
       index k == 0 is a special case:
         the real() part contains the result for the DC frequency 0,
         the imag() part contains the result for the Nyquist frequency Samplerate/2
     both 0-frequency and half frequency components, which are real,
     are assembled in the first entry as  F(0)+i*F(N/2).
     With the output size N/2 complex values (=N real/imag values), it is
     obvious, that the result for negative frequencies are not output,
     cause of symmetry.

     input and output may alias.
  */
  PFFFT_EXPORT void pffft_transform(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);

  /* 
     Similar to pffft_transform, but makes sure that the output is
     ordered as expected (interleaved complex numbers).  This is
     similar to calling pffft_transform and then pffft_zreorder.
     
     input and output may alias.
  */
  PFFFT_EXPORT void pffft_transform_ordered(const PFFFT_Setup *setup, const float *input, float *output, float *work, pffft_direction_t direction);

  /* 
     call pffft_zreorder(.., PFFFT_FORWARD) after pffft_transform(...,
     PFFFT_FORWARD) if you want to have the frequency components in
     the correct "canonical" order, as interleaved complex numbers.
     
     (for real transforms, both 0-frequency and half frequency
     components, which are real, are assembled in the first entry as
     F(0)+i*F(n/2+1). Note that the original fftpack did place
     F(n/2+1) at the end of the arrays).
     
     input and output should not alias.
  */
  PFFFT_EXPORT void pffft_zreorder(const PFFFT_Setup *setup, const float *input, float *output, pffft_direction_t direction);

  /* 
     Perform a multiplication of the frequency components of dft_a and
     dft_b and accumulate them into dft_ab. The arrays should have
     been obtained with pffft_transform(.., PFFFT_FORWARD) and should
     *not* have been reordered with pffft_zreorder (otherwise just
     perform the operation yourself as the dft coefs are stored as
     interleaved complex numbers).
     
     the operation performed is: dft_ab += (dft_a * fdt_b)*scaling
     
     The dft_a, dft_b and dft_ab pointers may alias.
  */
  PFFFT_EXPORT void pffft_zconvolve_accumulate(const PFFFT_Setup *setup, const float *dft_a, const float *dft_b, float *dft_ab, float scaling);

  /* 
     Perform a multiplication of the frequency components of dft_a and
     dft_b and put result in dft_ab. The arrays should have
     been obtained with pffft_transform(.., PFFFT_FORWARD) and should
     *not* have been reordered with pffft_zreorder (otherwise just
     perform the operation yourself as the dft coefs are stored as
     interleaved complex numbers).

     the operation performed is: dft_ab = (dft_a * fdt_b)*scaling

     The dft_a, dft_b and dft_ab pointers may alias.
  */
  PFFFT_EXPORT void pffft_zconvolve_scale(const PFFFT_Setup *setup, const float *dft_a, const float *dft_b, float *dft_ab, float scaling);
  
  /*
     Perform a multiplication of the frequency components of dft_a and
     dft_b and put result in dft_ab, without accumulation and without
     scaling. The arrays should have been obtained with
     pffft_transform(.., PFFFT_FORWARD) and should *not* have been
     reordered with pffft_zreorder (otherwise just perform the operation
     yourself as the dft coefs are stored as interleaved complex numbers).

     the operation performed is: dft_ab = dft_a * dft_b

     The dft_a, dft_b and dft_ab pointers may alias.
  */
  PFFFT_EXPORT void pffft_zconvolve(const PFFFT_Setup *setup, const float *dft_a, const float *dft_b, float *dft_ab);
  
  /*
     Convert an unordered PFFFT_REAL forward-transform result (as
     produced by pffft_transform(.., PFFFT_FORWARD)) into "zero-phase"
     form: imaginary vector lanes are zeroed, real lanes are multiplied
     by 'scaling' and normalized by 1/N. Useful for linear-phase FIR
     filters whose impulse response is centered at t = 0 with the
     left-hand taps wrapped around to the end of the transform block:
     their spectrum has no imaginary components. 'in' may alias 'out'.

     With scaling == 1 the pipeline transform(x, FORWARD) ->
     pffft_zconvolve_zp() -> transform(.., BACKWARD) yields the
     circular convolution of x with the filter at unit gain.
     'scaling' specifies an additional gain on top of that.

     Layout caveat: the unordered REAL layout packs the two real-valued
     boundary coefficients F(0) and F(N/2) into the first lanes of the
     first two vectors, where F(N/2) occupies an otherwise-imaginary
     lane. pffft_zconvert_zp() keeps that lane (scaled), so the output
     is NOT a plain spectrum: multiply it only with pffft_zconvolve_zp(),
     never with pffft_zconvolve_scale() or pffft_zconvolve(), which
     would cross-multiply the DC and Nyquist lanes.

     Returns 0 on success, nonzero if setup was not created for
     PFFFT_REAL.
  */
  PFFFT_EXPORT int pffft_zconvert_zp(const PFFFT_Setup *setup, const float *in, float *out, float scaling);

  /*
     Frequency-domain multiply of an unordered PFFFT_REAL forward
     spectrum dft_x with a zero-phase filter spectrum dft_hzp produced
     by pffft_zconvert_zp(): dft_ab = dft_x * dft_hzp. No accumulation.
     With a filter converted using scaling == 1, the pipeline
     transform(x, FORWARD) -> zconvolve_zp() -> transform(.., BACKWARD)
     yields the circular convolution at unit gain. All pointers may
     alias.

     Returns 0 on success, nonzero if setup was not created for
     PFFFT_REAL.
  */
  PFFFT_EXPORT int pffft_zconvolve_zp(const PFFFT_Setup *setup, const float *dft_x, const float *dft_hzp, float *dft_ab);

  /*
     deprecated synonym for pffft_zconvolve_scale()
  */
  PFFFT_EXPORT void PFFFT_DEPRECATED pffft_zconvolve_no_accu(const PFFFT_Setup *setup, const float *dft_a, const float *dft_b, float *dft_ab, float scaling);

  /* return 4 or 1 wether support SSE/NEON/Altivec instructions was enabled when building pffft.c */
  PFFFT_EXPORT int pffft_simd_size(void);

  /* return string identifier of used architecture (SSE/NEON/Altivec/..) */
  PFFFT_EXPORT const char * pffft_simd_arch(void);


  /* following functions are identical to the pffftd_ functions */

  /* simple helper to get minimum possible fft size */
  PFFFT_EXPORT int pffft_min_fft_size(pffft_transform_t transform);

  /* simple helper to determine next power of 2
     - without inexact/rounding floating point operations
  */
  PFFFT_EXPORT int pffft_next_power_of_two(int N);

  /* simple helper to determine if power of 2 - returns bool */
  PFFFT_EXPORT int pffft_is_power_of_two(int N);

  /* simple helper to determine size N is valid
     - factorizable to pffft_min_fft_size() with factors 2, 3, 5
     returns bool
  */
  PFFFT_EXPORT int pffft_is_valid_size(int N, pffft_transform_t cplx);

  /* determine nearest valid transform size  (by brute-force testing)
     - factorizable to pffft_min_fft_size() with factors 2, 3, 5.
     higher: bool-flag to find nearest higher value; else lower.
  */
  PFFFT_EXPORT int pffft_nearest_transform_size(int N, pffft_transform_t cplx, int higher);

  /*
    the float buffers must have the correct alignment (16-byte boundary
    on intel and powerpc). This function may be used to obtain such
    correctly aligned buffers.
  */
  PFFFT_EXPORT void *pffft_aligned_malloc(size_t nb_bytes);
  PFFFT_EXPORT void pffft_aligned_free(void *);

#ifdef __cplusplus
}
#endif

#endif /* PFFFT_H */

