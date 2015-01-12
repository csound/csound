/*
 fareygen.c:

 Copyright (C) 2010 Georg Boenn

 This file is part of Csound.

 The Csound Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
*/
extern "C" {
#include "csdl.h"
}
#include <cmath>
#include <complex>
#include <random>

static MYFLT profile(MYFLT fi, MYFLT bwi)
{
    MYFLT x = fi / bwi;
    x *= x;
    // Avoids computing the e^(-x^2) where it's results are very close to zero.
    if (x > 14.71280603) {
        return 0.0;
    }
    return std::exp(-x) / bwi;
};

extern "C" {
#define ROOT2 FL(1.41421356237309504880168872421)
  
  /**
   * This function computes a Csound "sound sample" function table
   * using Nasca's "padsynth" algorithm implemented in C++.
   */
  static int padsynth_gen (FGDATA *ff, FUNC *ftp)
  {
      CSOUND *csound = ff->csound;
      MYFLT p1_function_table_number = ff->fno;
      csound->Message(csound, "p1_function_table_number: %9.4f\n",
                      p1_function_table_number);
      MYFLT p2_score_time = ff->e.p[2];
      csound->Message(csound, "p2_score_time: %9.4f\n", p2_score_time);
      int N = ff->flen;
      csound->Message(csound, "N: %9.4f\n", N);
      //const char *p4_gen_id = ((STRINGDAT *)(&ff->e.p[4]))->data;
      csound->Message(csound, "p4_gen_id: %d\n", (int)(ff->e.p[4]));
      MYFLT p5_fundamental_frequency = ff->e.p[5];
      csound->Message(csound, "p5_fundamental_frequency: %9.4f\n",
                      p5_fundamental_frequency);
      MYFLT p6_partial_bandwith = ff->e.p[6];
      csound->Message(csound, "p6_partial_bandwith: %9.4f\n", p6_partial_bandwith);
      MYFLT p7_partial_bandwidth_scale_factor = ff->e.p[7];
      csound->Message(csound, "p7_partial_bandwidth_scale_factor: %9.4f\n",
                      p7_partial_bandwidth_scale_factor);
      MYFLT p8_harmonic_stretch = ff->e.p[8];
      csound->Message(csound, "p8_harmonic_stretch: %9.4f\n", p8_harmonic_stretch);
      MYFLT samplerate = csound->GetSr(csound);
      csound->Message(csound, "samplerate: %9.4f\n", samplerate);
      // The amplitudes of each partial are in pfield 9 and higher.
      // N.B.: The partials are indexed starting from 1.
      int partialN = ff->e.pcnt - 8;
      std::vector<MYFLT> A(partialN + 1);
      for (int partialI = 1; partialI <= partialN; ++partialI) {
        A[partialI] = ff->e.p[9 + partialI - 1];
        csound->Message(csound, "Partial[%3d]: %9.4f\n", partialI, A[partialI]);
      }
      for (int i = 0; i < N; ++i) {
        ftp->ftable[i] = FL(0.0);
      }
      // The algorithm computes a spectrum based on the partials and their
      // bandwidths.
      // The inverse FFT of this spectrum is then taken to obtain a sound sample --
      // not the IFFT of the partials!
      // N.B.: An in-place IFFT of N/2 complex to N real samples is used.
      // ftable[1] contains the real part of the Nyquist frequency; we make it 0.
      std::complex<MYFLT> *spectrum = (std::complex<MYFLT> *)ftp->ftable;
      int complexN = int(N / 2.0);
      for (int partialI = 1; partialI <= partialN; ++partialI) {
        MYFLT bw_Hz;//bandwidth of the current harmonic measured in Hz
        MYFLT bwi;
        MYFLT fi;
        MYFLT rF = p5_fundamental_frequency * p8_harmonic_stretch * partialI;
        // bw_Hz=(pow(2.0,bw/1200.0)-1.0)*f*pow(relF(nh),bwscale);
        bw_Hz = (std::pow(2.0, p6_partial_bandwith / 1200.0) - 1.0) *
          p5_fundamental_frequency * std::pow(p8_harmonic_stretch * partialI,
                                              p7_partial_bandwidth_scale_factor);
        // bwi=bw_Hz/(2.0*samplerate);
        bwi = bw_Hz / (2.0 * samplerate);
        // fi=rF/samplerate;
        fi = rF / samplerate;
        for (int complexI = 0; complexI < complexN; ++complexI) {
          // Here you can optimize, by avoiding to compute the profile for
          // the full frequency (usually it's zero or very close to zero).
          // hprofile=profile((i/(REALTYPE)N)-fi,bwi);
          MYFLT hprofile = profile((complexI / (MYFLT) N) - fi, bwi);
          // freq_amp[i]+=hprofile*A[nh];
          MYFLT real = hprofile * A[partialI];
          spectrum[complexI] += real;
        };
      };
      std::default_random_engine generator;
      std::uniform_real_distribution<double> distribution(0.0, 6.28318530718);
      for (int complexI = 0; complexI < complexN; ++complexI) {
        MYFLT phase = distribution(generator);
        MYFLT real = spectrum[complexI].real();
        spectrum[complexI].real(real * std::cos(phase));
        spectrum[complexI].imag(real * std::sin(phase));
      };
      spectrum[0].imag(0);
      csound->InverseComplexFFT(csound, ftp->ftable, complexN);
      // Normalize,
      MYFLT maximum = FL(0.0);
      for (int i = 0; i < N; ++i) {
        if (std::fabs(ftp->ftable[i]) > maximum) {
          maximum = std::fabs(ftp->ftable[i]);
          csound->Message(csound, "maximum at %d: %f\n", i, maximum);
        }
      }
      if (maximum < 1e-5) {
        maximum = 1e-5;
      }
      for (int i = 0; i < N; ++i) {
        ftp->ftable[i] /= maximum * ROOT2;
      }
      return OK;
  }

  static NGFENS padsynth_gens[] = {
    { "padsynth", padsynth_gen },
    { NULL, NULL }
  };

  FLINKAGE_BUILTIN(padsynth_gens)

};
