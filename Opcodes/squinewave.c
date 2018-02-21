/* SQUINEWAVE.C: Sine-Square-Pulse-Saw oscillator
* by rasmus ekman 2017, for Csound.
* This code is released under the Csound license,
* GNU Lesser General Public License version 2.1.
*/
/*
    Copyright (C) 2017 rasmus ekman 2017

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

#include <math.h>

#include "csoundCore.h"


/* ================================================================== */

typedef struct {
    OPDS h;
    MYFLT *aout, *async_out, *acps, *aclip, *askew, *async_in, *iminsweep, *iphase;

    // phase and warped_phase range 0-2.
    //This makes skew/clip into simple proportions
    double phase;
    double warped_phase;
    double hardsync_phase;
    double hardsync_inc;

    // Const inited from environment
    double Min_Sweep;
    double Maxphase_By_sr;
    double Max_Warp_Freq;

    MYFLT *sync_sig;        // holds async_in if a-rate
    int32_t init_phase;
} SQUINEWAVE;

/* ================================================================== */

static inline int32_t find_sync(const MYFLT* sync_sig, const uint32_t first,
                                const uint32_t last)
{
    uint32_t i;
    if (sync_sig == 0)
        return -1;

    for (i = first; i < last; ++i) {
        if (sync_sig[i] >= (MYFLT)1)
            return i;
    }
    return -1;
}

/* ================================================================== */

static void hardsync_init(SQUINEWAVE *p, const double freq,
                          const double warped_phase)
{
    if (p->hardsync_phase)
        return;

    // If we're in last flat part, we're just done now
    if (warped_phase == 2.0) {
        p->phase = 2.0;
        return;
    }

    if (freq > p->Max_Warp_Freq)
        return;

    p->hardsync_inc = (PI / p->Min_Sweep);
    p->hardsync_phase = p->hardsync_inc * 0.5;
}


/* ================================================================== */

static inline MYFLT Clamp(const MYFLT x, const MYFLT minval, const MYFLT maxval) {
    return (x < minval) ? minval : (x > maxval) ? maxval : x;
}


/* ================================================================== */


int32_t squinewave_init(CSOUND* csound, SQUINEWAVE *p)
{
    const double sr = csound->GetSr(csound);

    // Skip setting phase only if we have been inited at least once
    p->init_phase = (*p->iphase < 0 && p->Min_Sweep > 1.0) ? 0 : 1;
    p->Min_Sweep = *p->iminsweep;

    // Allow range 4-sr/100
    if (p->Min_Sweep < 4.0 || p->Min_Sweep > sr * 0.01) {
      const int32_t minsweep_default = (int32_t)Clamp(sr / 3000.0, 8.0, sr * 0.01);
      if (p->Min_Sweep != 0.0) {
        csound->Warning(csound,
                        Str("squinewave iminsweep range 4 to sr/100. "
                            "Set to default %d"), minsweep_default);
      }
      p->Min_Sweep = minsweep_default;
    }

    p->Maxphase_By_sr = 2.0 / sr;
    p->Max_Warp_Freq = sr / (2.0 * p->Min_Sweep);

    p->sync_sig = IS_ASIG_ARG(p->async_in) ? p->async_in : 0;

    return OK;
}


/* ================================================================== */

int32_t squinewave_gen(CSOUND* csound, SQUINEWAVE *p)
{
    IGN(csound);
    const uint32_t nsmps = CS_KSMPS;
    uint32_t n;

    // Clear parts of output outside event
    const uint32_t ksmps_offset = p->h.insdshead->ksmps_offset;
    const uint32_t ksmps_end = nsmps - p->h.insdshead->ksmps_no_end;
    if (UNLIKELY(ksmps_offset)) memset(p->aout, 0, ksmps_offset * sizeof(MYFLT));
    if (UNLIKELY(ksmps_end < nsmps)) {
      memset(&p->aout[ksmps_end], 0, p->h.insdshead->ksmps_no_end * sizeof(MYFLT));
    }

    const double Maxphase_By_sr = p->Maxphase_By_sr;
    const double Max_Warp_Freq = p->Max_Warp_Freq;
    const double Max_Warp = 1.0 / p->Min_Sweep;
    const double Min_Sweep = p->Min_Sweep;

    MYFLT *aout = &p->aout[0];
    const MYFLT * const freq_sig = p->acps;
    const MYFLT * const clip_sig = p->aclip;
    const MYFLT * const skew_sig = p->askew;

    double phase = p->phase;
    double warped_phase = p->warped_phase;

    double hardsync_phase = p->hardsync_phase;
    double hardsync_inc = p->hardsync_inc;
    int32_t sync = find_sync(p->sync_sig, ksmps_offset, ksmps_end);

    // Set main phase so it matches warp
    if (p->init_phase) {
      const double freq = fmax(freq_sig[0], 0.0);
      const double phase_inc = Maxphase_By_sr * freq;
      const double min_sweep = phase_inc * Min_Sweep;
      const double skew = 1.0 - Clamp(skew_sig[0], -1.0, 1.0);
      const double clip = 1.0 - Clamp(clip_sig[0], 0.0, 1.0);
      const double midpoint = Clamp(skew, min_sweep, 2.0 - min_sweep);

      // Init phase range 0-2, has 4 segment parts (sweep down,
      // flat -1, sweep up, flat +1)
      warped_phase = *p->iphase;
      if (warped_phase < 0.0) {
        // "up" 0-crossing
        warped_phase = 1.25;
      }
      if (warped_phase > 2.0)
        warped_phase = fmod(warped_phase, 2.0);

      // Select segment and scale within
      if (warped_phase < 1.0) {
        const double sweep_length = fmax(clip * midpoint, min_sweep);
        if (warped_phase < 0.5) {
          phase = sweep_length * (warped_phase * 2.0);
          warped_phase *= 2.0;
        }
        else {
          const double flat_length = midpoint - sweep_length;
          phase = sweep_length + flat_length * ((warped_phase - 0.5) * 2.0);
          warped_phase = 1.0;
        }
      }
      else {
        const double sweep_length = fmax(clip * (2.0 - midpoint), min_sweep);
        if (warped_phase < 1.5) {
          phase = midpoint + sweep_length * ((warped_phase - 1.0) * 2.0);
          warped_phase = 1.0 + (warped_phase - 1.0) * 2.0;
        }
        else {
          const double flat_length = 2.0 - (midpoint + sweep_length);
          phase = midpoint + sweep_length +
            flat_length * ((warped_phase - 1.5) * 2.0);
          warped_phase = 2.0;
        }
      }

      p->init_phase = 0;
    }

    if (p->async_out)
      memset(p->async_out, 0, nsmps * sizeof(MYFLT));


    for (n = ksmps_offset; n < ksmps_end; ++n)
    {
      double freq = fmax(freq_sig[n], 0.0);

      if (sync == (int32_t)n) {
        p->phase = phase;
        p->hardsync_phase = hardsync_phase;
        p->hardsync_inc = hardsync_inc;
        hardsync_init(p, freq, warped_phase);
        phase = p->phase;
        hardsync_phase = p->hardsync_phase;
        hardsync_inc = p->hardsync_inc;
      }

      if (hardsync_phase) {
        const double syncsweep = 0.5 * (1.0 - cos(hardsync_phase));
        freq += syncsweep * ((2.0 * Max_Warp_Freq) - freq);
        hardsync_phase += hardsync_inc;
        if (hardsync_phase > PI) {
          hardsync_phase = PI;
          hardsync_inc = 0.0;
        }
      }

      const double phase_inc = Maxphase_By_sr * freq;

      // Pure sine if freq > sr/(2*Min_Sweep)
      if (freq >= Max_Warp_Freq)
        {
          // Continue from warped
          *aout++ = cos(PI * warped_phase);
          phase = warped_phase;
          warped_phase += phase_inc;
        }
      else
        {
          const double min_sweep = phase_inc * Min_Sweep;
          const double skew = 1.0 - Clamp(skew_sig[n], -1.0, 1.0);
          const double clip = 1.0 - Clamp(clip_sig[n], 0.0, 1.0);
          const double midpoint = Clamp(skew, min_sweep, 2.0 - min_sweep);

          // 1st half: Sweep down to cos(warped_phase <= Pi) then
          // flat -1 until phase >= midpoint
          if (warped_phase < 1.0 || (warped_phase == 1.0 && phase < midpoint))
            {
              if (warped_phase < 1.0) {
                const double sweep_length = fmax(clip * midpoint, min_sweep);

                *aout++ = cos(PI * warped_phase);
                warped_phase += fmin(phase_inc / sweep_length, Max_Warp);

                // Handle fractional warped_phase overshoot after sweep ends
                if (warped_phase > 1.0) {
                  /* Tricky here: phase and warped may disagree where
                   * we are in waveform (due to FM + skew/clip
                   * changes).  Warped dominates to keep waveform
                   * stable, waveform (flat part) decides where we
                   * are.
                   */
                  const double flat_length = midpoint - sweep_length;
                  // warp overshoot scaled to main phase rate
                  const double phase_overshoot =
                    (warped_phase - 1.0) * sweep_length;

                  // phase matches shape
                  phase = midpoint - flat_length + phase_overshoot - phase_inc;

                  // Flat if next samp still not at midpoint
                  if (flat_length >= phase_overshoot) {
                    warped_phase = 1.0;
                    // phase may be > midpoint here (which means
                    // actually no flat part), if so it will be
                    // corrected in 2nd half (since warped == 1.0)
                  }
                  else {
                    const double next_sweep_length =
                      fmax(clip * (2.0 - midpoint), min_sweep);
                    warped_phase =
                      1.0 + (phase_overshoot - flat_length) / next_sweep_length;
                  }
                }
              }
              else {
                // flat up to midpoint
                *aout++ = -1.0;
                warped_phase = 1.0;
              }
            }
            // 2nd half: Sweep up to cos(warped_phase <= 2.Pi) then
            // flat +1 until phase >= 2
            else {
              if (warped_phase < 2.0) {
                const double sweep_length =
                  fmax(clip * (2.0 - midpoint), min_sweep);
                if (warped_phase == 1.0) {
                  // warped_phase overshoot after flat part
                  warped_phase = 1.0 + fmin( fmin(phase - midpoint, phase_inc) /
                                             sweep_length, Max_Warp);
                }
                *aout++ = cos(PI * warped_phase);
                warped_phase += fmin(phase_inc / sweep_length, Max_Warp);
                if (warped_phase > 2.0) {
                  const double flat_length = 2.0 - (midpoint + sweep_length);
                  const double phase_overshoot =
                    (warped_phase - 2.0) * sweep_length;

                  phase = 2.0 - flat_length + phase_overshoot - phase_inc;

                  if (flat_length >= phase_overshoot) {
                    warped_phase = 2.0;
                  }
                  else {
                    const double next_sweep_length =
                      fmax(clip * midpoint, min_sweep);
                    warped_phase =
                      2.0 + (phase_overshoot - flat_length) / next_sweep_length;
                  }
                }
              }
              else {
                *aout++ = 1.0;
                warped_phase = 2.0;
              }
            }
        }

      phase += phase_inc;
      if (warped_phase >= 2.0 && phase >= 2.0)
        {
            if (hardsync_phase) {
              warped_phase = phase = 0.0;
              hardsync_phase = hardsync_inc = 0.0;

              sync = find_sync(p->sync_sig, n + 1, ksmps_end);
            }
            else {
              phase -= 2.0;
              if (phase > phase_inc) {
                // wild aliasing freq - just reset
                phase = phase_inc * 0.5;
              }
              if (freq < Max_Warp_Freq) {
                const double min_sweep = phase_inc * Min_Sweep;
                const double skew = 1.0 - Clamp(skew_sig[n], -1.0, 1.0);
                const double clip = 1.0 - Clamp(clip_sig[n], 0.0, 1.0);
                const double midpoint = Clamp(skew, min_sweep, 2.0 - min_sweep);
                const double next_sweep_length = fmax(clip * midpoint, min_sweep);
                warped_phase = fmin(phase / next_sweep_length, Max_Warp);
              }
              else
                warped_phase = phase;
            }

            if (p->async_out)
              p->async_out[n] = 1.0;
        }
    }

    p->phase = phase;
    p->warped_phase = warped_phase;
    p->hardsync_phase = hardsync_phase;
    p->hardsync_inc = hardsync_inc;
    return OK;
}



/* ================================================================== */


/* ar[, aSyncOut] squinewave   aFreq, aClip, aSkew [, aSyncIn, aMinSweep, iphase] */

static OENTRY squinewave_localops[] =
  {
   { "squinewave", sizeof(SQUINEWAVE), 0, 3, "am", "aaaxoj",
     (SUBR)squinewave_init, (SUBR)squinewave_gen },
};

LINKAGE_BUILTIN(squinewave_localops)
