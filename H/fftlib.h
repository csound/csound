/*
    fftlib.h:

    FFT library
    based on public domain code by John Green <green_jt@vsdec.npt.nuwc.navy.mil>
    original version is available at
      http://hyperarchive.lcs.mit.edu/
            /HyperArchive/Archive/dev/src/ffts-for-risc-2-c.hqx
    ported to Csound by Istvan Varga, 2005

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

  /**
   * Compute in-place complex FFT on the rows of the input array
   * INPUTS
   *   *buf = input data array
   *   FFTsize = FFT size, in samples, or log2 of 1 / (FFT size) if negative
   *   nRows = number of rows in buf array (use 1 for nRows for a single FFT)
   * OUTPUTS
   *   *buf = output data array
   */
  PUBLIC void csoundComplexFFT(void *csound, MYFLT *buf, int FFTsize,
                               int nRows);

  /**
   * Compute in-place inverse complex FFT on the rows of the input array
   * INPUTS
   *   *buf = input data array
   *   FFTsize = FFT size, in samples, or log2 of 1 / (FFT size) if negative
   *   nRows = number of rows in buf array (use 1 for nRows for a single FFT)
   * OUTPUTS
   *   *buf = output data array
   */
  PUBLIC void csoundInverseComplexFFT(void *csound, MYFLT *buf,
                                      int FFTsize, int nRows);

  /**
   * Compute in-place real FFT on the rows of the input array
   * The result is the complex spectra of the positive frequencies
   * except the location for the first complex number contains the real
   * values for DC and Nyquist
   * See csoundRSpectProd for multiplying two of these spectra together
   * - ex. for fast convolution
   * INPUTS
   *   *buf = real input data array
   *   FFTsize = FFT size, in samples, or log2 of 1 / (FFT size) if negative
   *   nRows = number of rows in buf array (use 1 for nRows for a single FFT)
   * OUTPUTS
   *   *buf = output data array, in the following order:
   *            Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]), Re(x[2]), Im(x[2]),
   *            ... Re(x[N/2-1]), Im(x[N/2-1]).
   */
  PUBLIC void csoundRealFFT(void *csound, MYFLT *buf, int FFTsize, int nRows);

  /**
   * Compute in-place real iFFT on the rows of the input array
   * data order as from csoundRealFFT()
   * INPUTS
   *   *buf = input data array in the following order:
   *            Re(x[0]), Re(x[N/2]), Re(x[1]), Im(x[1]), Re(x[2]), Im(x[2]),
   *            ... Re(x[N/2-1]), Im(x[N/2-1]).
   *   FFTsize = FFT size, in samples, or log2 of 1 / (FFT size) if negative
   *   nRows = number of rows in buf array (use 1 for nRows for a single FFT)
   * OUTPUTS
   *   *buf = real output data array
   */
  PUBLIC void csoundInverseRealFFT(void *csound, MYFLT *buf,
                                   int FFTsize, int nRows);

  /**
   * When multiplying a pair of spectra from csoundRealFFT() care must be
   * taken to multiply the two real values seperately from the complex ones.
   * This routine does it correctly.
   * the result can be stored in-place over one of the inputs
   * INPUTS
   *   *buf1 = input data array    first spectra
   *   *buf2 = input data array    second spectra
   *   FFTsize = FFT size, in samples, or log2 of 1 / (FFT size) if negative
   * OUTPUTS
   *   *outbuf = output data array spectra
   */
  PUBLIC void csoundRSpectProd(void *csound, MYFLT *buf1, MYFLT *buf2,
                               MYFLT *outbuf, int FFTsize);

#endif      /* CSOUND_FFTLIB_H */

