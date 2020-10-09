/*
    fftlib.h:

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_FFTLIB_H
#define CSOUND_FFTLIB_H

#if !defined(__BUILDING_LIBCSOUND)
#  error "Csound plugins and host applications should not include fftlib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Returns the amplitude scale that should be applied to the result of
   * an inverse complex FFT with a length of 'FFTsize' samples.
   */
  MYFLT csoundGetInverseComplexFFTScale(CSOUND *csound, int FFTsize);

  /**
   * Returns the amplitude scale that should be applied to the result of
   * an inverse real FFT with a length of 'FFTsize' samples.
   */
  MYFLT csoundGetInverseRealFFTScale(CSOUND *csound, int FFTsize);

  /**
   * Compute in-place complex FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize*2 MYFLT values,
   *          in interleaved real/imaginary format
   */
  void csoundComplexFFT(CSOUND *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place inverse complex FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize*2 MYFLT values,
   *          in interleaved real/imaginary format
   * Output should be scaled by the return value of
   * csoundGetInverseComplexFFTScale(csound, FFTsize).
   */
  void csoundInverseComplexFFT(CSOUND *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place real FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize MYFLT values; output is in interleaved
   *          real/imaginary format, except for buf[1] which is the real
   *          part for the Nyquist frequency
   */
  void csoundRealFFT(CSOUND *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place inverse real FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize MYFLT values; input is expected to be in
   *          interleaved real/imaginary format, except for buf[1] which
   *          is the real part for the Nyquist frequency
   * Output should be scaled by the return value of
   * csoundGetInverseRealFFTScale(csound, FFTsize).
   */
  void csoundInverseRealFFT(CSOUND *csound, MYFLT *buf, int FFTsize);

  /**
   * Multiply two arrays (buf1 and buf2) of complex data in the format
   * returned by csoundRealFFT(), and leave the result in outbuf, which
   * may be the same as either buf1 or buf2.
   * An amplitude scale of 'scaleFac' is also applied.
   * The arrays should contain 'FFTsize' MYFLT values.
   */
  void csoundRealFFTMult(CSOUND *csound,
                         MYFLT *outbuf, MYFLT *buf1, MYFLT *buf2,
                         int FFTsize, MYFLT scaleFac);

  /**
   * Compute in-place real FFT, allowing non power of two FFT sizes.
   *
   * buf:     array of FFTsize + 2 MYFLT values; output is in interleaved
   *          real/imaginary format (note: the real part of the Nyquist
   *          frequency is stored in buf[FFTsize], and not in buf[1]).
   * FFTsize: FFT length in samples; not required to be an integer power of two,
   *          but should be even and not have too many factors.
   */
  void csoundRealFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place inverse real FFT, allowing non power of two FFT sizes.
   * The output does not need to be scaled.
   *
   * buf:     array of FFTsize + 2 MYFLT values, in interleaved real/imaginary
   *          format (note: the real part of the Nyquist frequency is stored
   *          in buf[FFTsize], and not in buf[1]).
   * FFTsize: FFT length in samples; not required to be an integer power of two,
   *          but should be even and not have too many factors.
   */
  void csoundInverseRealFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);


   /**
   * New Real FFT interface
   * Creates a setup for a series of FFT operations.
   *
   * FFTsize: FFT length in samples; not required to be an integer power of two,
   *          but should be even and not have too many factors.
   * d:       direction (FFT_FWD or FFT_INV). Scaling by 1/FFTsize is done on
   *          the inverse direction (as with the other RealFFT functions above).
   *
   *  returns: a pointer to the FFT setup.
   */
  void *csoundRealFFT2Setup(CSOUND *csound, int FFTsize, int d);

   /**
   * New Real FFT interface
   * Compute in-place real FFT.
   *
   * buf:     array of FFTsize + 2 MYFLT values, in interleaved real/imaginary
   *          format (note: the real part of the Nyquist frequency is stored
   *          in buf[FFTsize], and not in buf[1]).
   * setup:   an FFT setup created with csoundRealFFT2Setup()
   */
  void csoundRealFFT2(CSOUND *csound, void *setup, MYFLT *sig);

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_FFTLIB_H */
