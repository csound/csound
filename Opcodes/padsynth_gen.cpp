/*
 padsynt_gen.cpp:

 Copyright (C) 2015 Michael Gogins after Nasca O Paul

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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301 USA
*/
extern "C" {

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
  
}
#include <cmath>
#include <complex>
#include <random>

/**

Paul Octavian Nasca's "padsynth algorithm" adds bandwidth to each partial of a
periodic weaveform. This bandwidth is heard as color, movement, and additional
richness of sound.

First, the waveform is defined by the user as a series of harmonic partials.
Then, bandwidth is added by independently spreading each partial of the
original waveform from a single frequency across neighboring frequencies,
according to a "profile" function: a Gaussian curve, a square, or a rising
and then falling expontential.

The partials of the original waveform may be considered to be samples in a
discrete Fourier transform of the waveform. Normally there is not an exact
one-to-one correspondence between the frequencies of the samples (frequency
bins) of the discrete Fourier transform with the frequencies of the partials
of the original waveform, because any frequency in the inverse of the discrete
Fourier transform might be synthesized by interference between any number of
bins. However, the padsynth algorithm uses a simple trick to create this
correspondence. The discrete Fourier transform is simply made so large that
the frequency of any partial of the original waveform will be very close to
the frequency of the corresponding bin in the Fourier transform. Once this
correspondence has been created, the bandwidth profile can be applied by
centering it over the frequency bin of the original partial, scaling the
profile by the bandwidth, and simply multiplying the original partial by each
sample of the profile and adding the product to the corresponding bin of the
Fourier transform.

As the frequencies of the partials increase, their bandwidth may optionally
become wider or (less often) narrower.

Once each partial has been spread out in this way, the discrete Fourier
transform may be given random phases, and is then simply inverted to
synthesize the desired waveform, which may be used as the wavetable for
a digital oscillator.

N.B.: The size of the function table does NOT necessarily reflect one periodic
cycle of the waveform that it contains. The fundamental frequency must be used
to generate the desired pitch from an oscillator using the function table, e.g.

    oscillator_hz = desired_hz * (sr / padsynth_size / fundamental_hz)

The parameters of the function table statement are:

    p1      "padsynth"

    p2      Score time (usually 0).

    p3      Function table size (must be a power of 2, should be large,
            e.g. 2^18 == 262144).

    p4      Function table number (auto-generated if 0).

    p5      Fundamental frequency of the generated waveform
            (cycles per second).

    p6      Bandwidth of the partials (cents).

    p7      Scaling factor for partial bandwidth (log of increase/decrease
            with partial frequency, 0 is no stretch or shrink).

    p8      Harmonic stretch/shrink of the partial (1 is harmonic).

    p9      Number specifying the shape of the bandwidth profile:

             1  Gaussian
             2  Square
             3  Exponential

    p10     Profile function parameter.

    p11-pN  The amplitudes of the partials (may be 0).


*/

static void log(CSOUND *csound, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (csound) {
    if (csound->GetMessageLevel(csound) & CS_WARNMSG)
      csound->MessageV(csound, 0, format, args);
  } else {
    vfprintf(stdout, format, args);
  }
  va_end(args);
}

static void warn(CSOUND *csound, const char *format, ...) {
  if (csound) {
    if (csound->GetMessageLevel(csound) & CS_WARNMSG) {
      va_list args;
      va_start(args, format);
      csound->MessageV(csound, CSOUNDMSG_WARNING, format, args);
      va_end(args);
    }
  } else {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
  }
}

/* unused
static MYFLT profile_original(MYFLT fi, MYFLT bwi)
{
    MYFLT x=fi/bwi;
    x*=x;
    if (x>14.71280603) {
        return 0.0;    //this avoids computing the e^(-x^2) where it's results
are very close to zero
    }
    return exp(-x)/bwi;
};
*/

// profile(p9_profile_shape, profile_sample_index_normalized, bandwidth_samples,
// p10_profile_parameter);
static MYFLT profile(int32_t shape, MYFLT fi, MYFLT bwi, MYFLT a) {
  MYFLT x = fi / bwi;
  MYFLT y = 0;
  switch (shape) {
  case 1:
    y = std::exp(-(x * x * a));
    break;
  case 2:
    // The idea is to take profile 1 and simply say y goes to 0 if below a and
    // to 1 if above a.
    y = std::exp(-(x * x * a));
    if (a < 0.00001) {
      a = 0.00001;
    } else if (a > 0.99999) {
      a = 0.99999;
    }
    if (y < a) {
      y = 0;
    } else {
      y = 1;
    }
    break;
  case 3:
    y = std::exp(-(std::fabs(x) * std::sqrt(a)));
    break;
  }
  return y / bwi;
}

#if 0
// Keep this stuff around, it might come in handy later.

#define FUNC(b) MYFLT base_function_##b(MYFLT x, MYFLT a)

static MYFLT base_function_pulse(MYFLT x, MYFLT a)
{
    return (std::fmod(x, 1.0) < a) ? -1.0 : 1.0;
}

FUNC(saw)
{
    if(a < 0.00001f) {
        a = 0.00001f;
    } else if(a > 0.99999f) {
        a = 0.99999f;
    }
    x = fmod(x, 1);
    if(x < a) {
        return x / a * 2.0f - 1.0f;
    } else {
        return (1.0f - x) / (1.0f - a) * 2.0f - 1.0f;
    }
}

FUNC(triangle)
{
    x = fmod(x + 0.25f, 1);
    a = 1 - a;
    if(a < 0.00001f) {
        a = 0.00001f;
    }
    if(x < 0.5f) {
        x = x * 4 - 1.0f;
    } else {
        x = (1.0f - x) * 4 - 1.0f;
    }
    x /= -a;
    if(x < -1.0f) {
        x = -1.0f;
    }
    if(x > 1.0f) {
        x = 1.0f;
    }
    return x;
}

FUNC(power)
{
    x = fmod(x, 1);
    if(a < 0.00001f) {
        a = 0.00001f;
    } else if(a > 0.99999f) {
        a = 0.99999f;
    }
    return powf(x, expf((a - 0.5f) * 10.0f)) * 2.0f - 1.0f;
}

FUNC(gauss)
{
    x = fmod(x, 1) * 2.0f - 1.0f;
    if(a < 0.00001f) {
        a = 0.00001f;
    }
    return expf(-x * x * (expf(a * 8) + 5.0f)) * 2.0f - 1.0f;
}

FUNC(diode)
{
    if(a < 0.00001f) {
        a = 0.00001f;
    } else if(a > 0.99999f) {
        a = 0.99999f;
    }
    a = a * 2.0f - 1.0f;
    x = cosf((x + 0.5f) * 2.0f * PI) - a;
    if(x < 0.0f) {
        x = 0.0f;
    }
    return x / (1.0f - a) * 2 - 1.0f;
}

FUNC(abssine)
{
    x = fmod(x, 1);
    if(a < 0.00001f) {
        a = 0.00001f;
    } else if(a > 0.99999f) {
        a = 0.99999f;
    }
    return sinf(powf(x, expf((a - 0.5f) * 5.0f)) * PI) * 2.0f - 1.0f;
}

FUNC(pulsesine)
{
    if(a < 0.00001f) {
        a = 0.00001f;
    }
    x = (fmod(x, 1) - 0.5f) * expf((a - 0.5f) * logf(128));
    if(x < -0.5f) {
        x = -0.5f;
    } else if(x > 0.5f) {
        x = 0.5f;
    }
    x = sinf(x * PI * 2.0f);
    return x;
}

FUNC(stretchsine)
{
    x = fmod(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 4;
    if(a > 0.0f) {
        a *= 2;
    }
    a = powf(3.0f, a);
    float b = powf(fabs(x), a);
    if(x < 0) {
        b = -b;
    }
    return -sinf(b * PI);
}

FUNC(chirp)
{
    x = fmod(x, 1.0f) * 2.0f * PI;
    a = (a - 0.5f) * 4;
    if(a < 0.0f) {
        a *= 2.0f;
    }
    a = powf(3.0f, a);
    return sinf(x / 2.0f) * sinf(a * x * x);
}

FUNC(absstretchsine)
{
    x = fmod(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 9;
    a = powf(3.0f, a);
    float b = powf(fabs(x), a);
    if(x < 0) {
        b = -b;
    }
    return -powf(sinf(b * PI), 2);
}

FUNC(chebyshev)
{
    a = a * a * a * 30.0f + 1.0f;
    return cosf(acosf(x * 2.0f - 1.0f) * a);
}

FUNC(sqr)
{
    a = a * a * a * a * 160.0f + 0.001f;
    return -atanf(sinf(x * 2.0f * PI) * a);
}

FUNC(spike)
{
    float b = a * 0.66666; // the width of the range: if a == 0.5, b == 0.33333

    if(x < 0.5) {
        if(x < (0.5 - (b / 2.0))) {
            return 0.0;
        } else {  // shift to zero, and expand to range from 0 to 1
            x = (x + (b / 2) - 0.5) * (2.0 / b);
            return x * (2.0 / b); // this is the slope: 1 / (b / 2)
        }
    } else {
        if(x > (0.5 + (b / 2.0))) {
            return 0.0;
        } else {
            x = (x - 0.5) * (2.0 / b);
            return (1 - x) * (2.0 / b);
        }
    }
}

FUNC(circle)
{
    // a is parameter: 0 -> 0.5 -> 1 // O.5 = circle
    float b, y;

    b = 2 - (a * 2); // b goes from 2 to 0
    x = x * 4;

    if(x < 2) {
        x = x - 1; // x goes from -1 to 1
        if((x < -b) || (x > b)) {
            y = 0;
        } else {
            y = sqrt(1 - (x*x) / (b*b));    // normally * a^2, but a stays 1
        }
    } else {
        x = x - 3; // x goes from -1 to 1 as well
        if((x < -b) || (x > b)) {
            y = 0;
        } else {
            y = -sqrt(1 - (x*x) / (b*b));
        }
    }
    return y;
}

typedef MYFLT (*base_function_t)(MYFLT, MYFLT);

static base_function_t get_base_function(int32_t index)
{
    if(!index) {
        return NULL;
    }

    if(index == 127) { //should be the custom wave
        return NULL;
    }

    index--;
    base_function_t functions[] = {
        base_function_triangle,comment
        base_function_pulse,
        base_function_saw,
        base_function_power,
        base_function_gauss,
        base_function_diode,
        base_function_abssine,
        base_function_pulsesine,
        base_function_stretchsine,
        base_function_chirp,
        base_function_absstretchsine,
        base_function_chebyshev,
        base_function_sqr,
        base_function_spike,
        base_function_circle,
    };
    return functions[index];
}

#endif

extern "C" {

/*
Original code:

    MYFLT PADsynth::profile(MYFLT fi, MYFLT bwi)
    {
        MYFLT x=fi/bwi;
        x*=x;
        if (x>14.71280603) {
            return 0.0;    //this avoids computing the e^(-x^2) where it's
results are very close to zero
        }
        return exp(-x)/bwi;
    };

    for (nh=1; nh<number_harmonics; nh++) { //for each harmonic
        MYFLT bw_Hz;//bandwidth of the current harmonic measured in Hz
        MYFLT bwi;
        MYFLT fi;
        MYFLT rF=f*relF(nh);

        bw_Hz=(pow(2.0,bw/1200.0)-1.0)*f*pow(relF(nh),bwscale);

        bwi=bw_Hz/(2.0*samplerate);
        fi=rF/samplerate;
        for (i=0; i<N/2; i++) { //here you can optimize, by avoiding to
                               // compute the profile for the full frequency
                               // (usually it's zero or very close to zero)
            MYFLT hprofile;
            hprofile=profile((i/(MYFLT)N)-fi,bwi);
            freq_amp[i]+=hprofile*A[nh];
        };
    };
*/

//#define ROOT2 FL(1.41421356237309504880168872421)

/**
 * This function computes a Csound function table
 * using Nasca's "padsynth" algorithm..
 */
static int32_t padsynth_gen(FGDATA *ff, FUNC *ftp) {
  CSOUND *csound = ff->csound;
  MYFLT p1_function_table_number = ff->fno;
  MYFLT p2_score_time = ff->e.p[2];
  void *setup;
  int32_t N = ff->flen;
  if (N <= 0) return csound->FtError(ff, Str("Illegal table size %d"), N);

  MYFLT p5_fundamental_frequency = ff->e.p[5];
  MYFLT p6_partial_bandwidth = ff->e.p[6];
  MYFLT p7_partial_bandwidth_scale_factor = ff->e.p[7];
  MYFLT p8_harmonic_stretch = ff->e.p[8];
  int32_t p9_profile_shape = (int)ff->e.p[9];
  // base_function_t base_function = get_base_function(p9_profile_shape);
  MYFLT p10_profile_parameter = ff->e.p[10];
  MYFLT samplerate = ftp->sr;
  log(csound, "samplerate:                  %12d\n", (int)samplerate);
  log(csound, "p1_function_table_number:            %9.4f\n",
      p1_function_table_number);
  log(csound, "p2_score_time:                       %9.4f\n", p2_score_time);
  log(csound, "p3_ftable_size               %12d\n", N);
  log(csound, "p4_gen_id:                   %12d\n", (int)(ff->e.p[4]));
  log(csound, "p5_fundamental_frequency:            %9.4f\n",
      p5_fundamental_frequency);
  log(csound, "p6_partial_bandwidth:                %9.4f\n",
      p6_partial_bandwidth);
  log(csound, "p7_partial_bandwidth_scale_factor:   %9.4f\n",
      p7_partial_bandwidth_scale_factor);
  log(csound, "p8_harmonic_stretch:                 %9.4f\n",
      p8_harmonic_stretch);
  log(csound, "p9_profile_shape:            %12d\n", p9_profile_shape);
  // log(csound, "profile_function:   0x%16p\n", base_function);
  log(csound, "p10_profile_parameter:               %9.4f\n",
      p10_profile_parameter);
  // The amplitudes of each partial are in pfield 11 and higher.
  // N.B.: The partials are indexed starting from 1.
  int32_t partialN = ff->e.pcnt - 10;
  std::vector<MYFLT> A(partialN + 1);
  A[0] = FL(0.0);
  for (int32_t partialI = 1; partialI <= partialN; ++partialI) {
    A[partialI] = ff->e.p[11 + partialI - 1];
  }
  for (int32_t i = 0; i < N; ++i) {
    ftp->ftable[i] = FL(0.0);
  }
  // N.B.: An in-place IFFT of N/2 complex to N real samples is used.
  // ftable[1] contains the real part of the Nyquist frequency; we make it 0.
  std::complex<MYFLT> *spectrum = (std::complex<MYFLT> *)ftp->ftable;
  int32_t complexN = int(N / 2.0);
  for (int32_t partialI = 1; partialI <= partialN; ++partialI) {
    MYFLT partial_Hz =
        p5_fundamental_frequency * p8_harmonic_stretch * ((MYFLT)partialI);
    MYFLT frequency_sample_index_normalized = partial_Hz / ((MYFLT)samplerate);
    int32_t partial_frequency_index =
        frequency_sample_index_normalized * ((MYFLT)N);
    MYFLT bandwidth_Hz = (std::pow(2.0, p6_partial_bandwidth / 1200.0) - 1.0) *
                         p5_fundamental_frequency *
                         std::pow(p8_harmonic_stretch * ((MYFLT)partialI),
                                  p7_partial_bandwidth_scale_factor);
    MYFLT bandwidth_samples = bandwidth_Hz / (2.0 * samplerate);
    log(csound, "partial[%3d]:                        %9.4f\n", partialI,
        A[partialI]);
    warn(csound, "  partial_Hz:                        %9.4f\n", partial_Hz);
    warn(csound, "  frequency_sample_index_normalized: %9.4f\n",
         frequency_sample_index_normalized);
    warn(csound, "  partial_frequency_index:   %12d\n",
         partial_frequency_index);
    warn(csound, "  bandwidth_Hz:                      %9.4f\n", bandwidth_Hz);
    warn(csound, "  bandwidth_samples:                  %12.8f\n",
         bandwidth_samples);
    for (int32_t fft_sample_index = 0; fft_sample_index < complexN;
         ++fft_sample_index) {
      MYFLT fft_sample_index_normalized =
          ((MYFLT)fft_sample_index) / ((MYFLT)N);
      MYFLT profile_sample_index_normalized =
          fft_sample_index_normalized - frequency_sample_index_normalized;
      MYFLT profile_sample =
          profile(p9_profile_shape, profile_sample_index_normalized,
                  bandwidth_samples, p10_profile_parameter);
      // MYFLT profile_sample =
      // profile_original(profile_sample_index_normalized, bandwidth_samples);
      MYFLT real = profile_sample * A[partialI];
      spectrum[fft_sample_index] += real;
    };
  };
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(0.0, 6.28318530718);
  for (int32_t complexI = 0; complexI < complexN; ++complexI) {
    MYFLT random_phase = distribution(generator);
    MYFLT real = spectrum[complexI].real();
    spectrum[complexI].real(real * std::cos(random_phase));
    spectrum[complexI].imag(real * std::sin(random_phase));
  };
  spectrum[0].imag(0);
  setup = csound->RealFFTSetup(csound,N,FFT_INV);
  csound->RealFFT(csound,setup,ftp->ftable);
  // Normalize,
  MYFLT maximum = FL(0.0);
  for (int32_t i = 0; i < N; ++i) {
    if (std::fabs(ftp->ftable[i]) > maximum) {
      maximum = std::fabs(ftp->ftable[i]);
      // warn(csound, "maximum at %d: %f\n", i, maximum);
    }
  }
  for (int32_t i = 0; i < N; ++i) {
    ftp->ftable[i] /= maximum * ROOT2;
  }
  return OK;
}

extern "C" {
  
static NGFENS padsynth_gens[] = {{(char *)"padsynth", padsynth_gen},
                                 {NULL, NULL}};

PUBLIC NGFENS *padsyn_fgen_init(CSOUND *csound) {
  IGN(csound);
  return padsynth_gens;
}
};
}
