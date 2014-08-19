/*
    exciter.c:

    Copyright (C) 2014 by John ffitch after Markus Schmidt

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

#include "csoundCore.h"       /*                    EXCITER.C         */
#include <math.h>

/**********************************************************************
 * EXCITER by Markus Schmidt
 **********************************************************************/

typedef struct {
  OPDS        h;
  MYFLT *aout;
  MYFLT *ain;
  MYFLT *pfreq;
  MYFLT *pceil;
  // ~Internals
  MYFLT freq_old, ceil_old;
  double hp1[7];
  double hp2[7];
  double hp3[7];
  double hp4[7];
  double lp1[7];
  double lp2[7];
  double rdrive, rbdr, kpa, kpb, kna, knb, ap, an, imr, kc, srct, sq, pwrq;
  int over;
  double prev_med, prev_out;
} EXCITER;

int exciter_init(CSOUND *csound, EXCITER *p)
{
    p->freq_old =  p->ceil_old = FL(0.0);
    p->hp1[5] = p->hp2[5] = p->hp3[5] = p->hp4[5] = 0.0;
    p->hp1[6] = p->hp2[6] = p->hp3[6] = p->hp4[6] = 0.0;
    p->lp1[5] = p->lp2[5] = 0.0;
    p->lp1[6] = p->lp2[6] = 0.0;
    p->rdrive = p->rbdr = p->kpa = p->kpb = p->kna = p->knb = p->ap =
      p->an = p->imr = p->kc = p->srct = p->sq = p->pwrq = p->prev_med =
      p->prev_out = 0.0;
    p->drive = p->blend = 0.0;
}

inline double pr,ocess(double st[7], double in)
{
    double tmp = in - st[5] * st[3] - st[6] * st[4];
    double out = tmp * st[0] + st[5] * st[1] + st[6] * st[2];
    st[6] = st[5];
    st[5] = tmp;
    return out;
}

    /** Highpass filter based on Robert Bristow-Johnson's equations
     * @param fc     resonant frequency
     * @param q      resonance (gain at fc)
     */
inline void set_hp_rbj(double hp[], double fc, double q)
{
    double omega=(double)(2*M_PI*fc/csound->esr);
    double sn=sin(omega);
    double cs=cos(omega);
    double alpha=(double)(sn/(2.0*q));

    double inv=(double)(1.0/(1.0+alpha));
    
    hp[2]/*a2*/ = hp[0]/*a0*/ =  (gain*inv*(1.0 + cs)/2.0);
    hp[1]/*a1*/ =  -2.0 * hp[0];
    hp[3]/*b1*/ =  (-2.0*cs*inv);
    hp[4]/*b2*/ =  ((1.0 - alpha)*inv);
    hp[5] = hp[6] = 0.0;
    return;
}

inline void set_lp_rbj(double lp[], double fc, double q)
    {
        double omega=(2.0*M_PI*fc/csound->sr);
        double sn=sin(omega);
        double cs=cos(omega);
        double alpha=(sn/(2*q));
        double inv=(1.0/(1.0+alpha));

        lp[2] = lp[0] =  (gain*inv*(1.0 - cs)*0.5);
        lp[1] =  lp[0]+lp[0];
        lp[3] =  (-2.0*cs*inv);
        lp[4] =  ((1.0 - alpha)*inv);
    }

static inline double M(float x)
{
    return (fabs(x) > 0.00000001f) ? x : 0.0f;
}

static inline double D(float x)
{
    x = fabs(x);
    return (x > 0.00000001f) ? sqrtf(x) : 0.0f;
}

inline double distort(EXCITER *p, double in)
{
    double *samples = resampler.upsample((double)in);
    
    for (int o = 0; o < over; o++) 
        double proc = samples[o];
        double med;
        if (proc >= 0.0) {
            med = (D(ap + proc * (kpa - proc)) + kpb) * pwrq;
        } else {
            med = (D(an - proc * (kna + proc)) + knb) * pwrq * -1.0f;
        }
        proc = srct * (med - prev_med + prev_out);
        prev_med = M(med);
        prev_out = M(proc);
        samples[o] = proc;
    }
    double out = (double)resampler.downsample(samples);
    return out;
}


void params_changed(CSOUND *csound, EXCITER *p)
{
    // set the params of all filters
    if (*p->pfreq != p->freq_old) {
      set_hp_rbj(p->hp1, *p->freq, 0.707);
      memcpy(p->hp2, p->hp1, 5*sizeof(double));
      memcpy(p->hp3, p->hp1, 5*sizeof(double));
      memcpy(p->hp4, p->hp1, 5*sizeof(double));
      p->freq_old = *p->freq;
    }
    // set the params of all filters
    if (*p->pceil != p->ceil_old) {
      set_lp_rbj(p->lp1, *p->ceil, 0.707);
      memcpy(p->lp2, p->lp1, 5*sizeof(double));
      p->ceil_old = *p->pceil;
    }
    // set distortion
    set_distort(EXCITER *p);
    p->blend = *p->pblend;
    p->drive = *p->pdrive;
}

int exciter_perf(CSOUND *csound, EXCITER *p)
//uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
 
     if (UNLIKELY(offset)) memset(p->out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout[nsmps], '\0', early*sizeof(MYFLT));
    }
   // process
    for (n = offset, n<nsmps, n++) {
      // cycle through samples
      double out, in;;
      in = (double)p->ain[n];
      // all pre filters in chain
      out = process(hp2, process(h1, in));
      out = distort(out);      // saturate
      // all post filters in chain
      out = process(hp4, process(hp3, in)); 
                
      if (*p->ceil_active > FL(0.5)) {
        // all H/P post filters in chain
        proc[i] = process(lp1, process(lp2), process(out));
      }
      maxDrive = dist[0].get_distortion_level() * *params[param_amount];
    
      p->aout[n] = out;
    } // cycle through samples
    return outputs_mask;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "exciter", S(EXCITER),   0, 5, "a", "akk",
                             (SUBR)exciter_init, NULL, (SUBR)exciter_perf },
}
