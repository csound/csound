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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_FFTLIB_H
#define CSOUND_FFTLIB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /**
   * Returns the amplitude scale that should be applied to the result of
   * an inverse complex FFT with a length of 'FFTsize' samples.
   */

  PUBLIC MYFLT csoundGetInverseComplexFFTScale(void *csound, int FFTsize);

  /**
   * Returns the amplitude scale that should be applied to the result of
   * an inverse real FFT with a length of 'FFTsize' samples.
   */

  PUBLIC MYFLT csoundGetInverseRealFFTScale(void *csound, int FFTsize);

  /**
   * Compute in-place complex FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize*2 MYFLT values,
   *          in interleaved real/imaginary format
   */

  PUBLIC void csoundComplexFFT(void *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place inverse complex FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize*2 MYFLT values,
   *          in interleaved real/imaginary format
   * Output should be scaled by the return value of
   * csoundGetInverseComplexFFTScale(csound, FFTsize).
   */

  PUBLIC void csoundInverseComplexFFT(void *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place real FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize MYFLT values; output is in interleaved
   *          real/imaginary format, except for buf[1] which is the real
   *          part for the Nyquist frequency
   */

  PUBLIC void csoundRealFFT(void *csound, MYFLT *buf, int FFTsize);

  /**
   * Compute in-place inverse real FFT
   * FFTsize: FFT length in samples
   * buf:     array of FFTsize MYFLT values; input is expected to be in
   *          interleaved real/imaginary format, except for buf[1] which
   *          is the real part for the Nyquist frequency
   * Output should be scaled by the return value of
   * csoundGetInverseRealFFTScale(csound, FFTsize).
   */

  PUBLIC void csoundInverseRealFFT(void *csound, MYFLT *buf, int FFTsize);

  /**
   * Multiply two arrays (buf1 and buf2) of complex data in the format
   * returned by csoundRealFFT(), and leave the result in outbuf, which
   * may be the same as either buf1 or buf2.
   * An amplitude scale of 'scaleFac' is also applied.
   * The arrays should contain 'FFTsize' MYFLT values.
   */

  PUBLIC void csoundRealFFTMult(void *csound,
                                MYFLT *outbuf, MYFLT *buf1, MYFLT *buf2,
                                int FFTsize, MYFLT scaleFac);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif      /* CSOUND_FFTLIB_H */

