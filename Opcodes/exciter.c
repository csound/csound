/*
    exciter.c:

    Copyright (C) 2014 by John ffitch after Markus Schmidt (calf)

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif       /*                    EXCITER.C         */
#include <math.h>

/**********************************************************************
 * EXCITER by Markus Schmidt
 **********************************************************************/

typedef struct {
  OPDS        h;
  cs_float       *aout;
  cs_float       *ain;
  cs_float       *pfreq;
  cs_float       *pceil;
  cs_float       *pdrive;
  cs_float       *pblend;
  // Internals
  cs_float       freq_old, ceil_old;
  // biquad data
  cs_double      hp1[7], hp2[7], hp3[7], hp4[7];
  cs_double      lp1[7],  lp2[7];
  // resampler
  cs_double      rs00[7], rs01[7], rs10[7], rs11[7];
  // distortion
  cs_double      rdrive, rbdr, kpa, kpb, kna, knb, ap, an, imr, kc, srct, sq, pwrq;
  cs_double      prev_med, prev_out;
  cs_double      blend_old, drive_old;
} EXCITER;

static inline cs_double process(cs_double st[7], cs_double in/*, char *s */)
{
    cs_double tmp = in - st[5] * st[3] - st[6] * st[4];
    cs_double out = tmp * st[0] + st[5] * st[1] + st[6] * st[2];
    st[6] = st[5];
    st[5] = tmp;
    /* printf("%s: %f -> %f; %f %f\n", s, in, out, st[5], st[6]); */
    return out;
}

    /** Highpass filter based on Robert Bristow-Johnson's equations
     * @param fc     resonant frequency
     * @param q      resonance (gain at fc)
     */
static inline void set_hp_rbj(CSOUND *csound, cs_double hp[7], cs_double fc, cs_double q, cs_float sr)
{
    cs_double omega= (TWOPI*fc/(cs_double) sr);
    cs_double sn=sin(omega);
    cs_double cs=cos(omega);
    cs_double alpha=(cs_double)(sn/(2.0*q));
    cs_double inv=(cs_double)(1.0/(1.0+alpha));

    hp[2]/*a2*/ = hp[0]/*a0*/ =  (inv*(1.0 + cs)*0.5);
    hp[1]/*a1*/ =  -2.0 * hp[0];
    hp[3]/*b1*/ =  (-2.0*cs*inv);
    hp[4]/*b2*/ =  ((1.0 - alpha)*inv);
    /* printf("hp_rbj: %f %f %f %f %f\n", hp[0], hp[1], hp[2], hp[3], hp[4]); */
    return;
}

static inline void set_lp_rbj(cs_double lp[7], cs_double fc, cs_double q, cs_double sr)
{
    cs_double omega=(TWOPI*fc/sr);
    cs_double sn=sin(omega);
    cs_double cs=cos(omega);
    cs_double alpha=(sn/(2*q));
    cs_double inv=(1.0/(1.0+alpha));

    /* printf("fc = %f q = %f sr = %f: %f\n", fc, q, sr, TWOPI*fc/sr); */
    /* printf("omega = %f, sn = %f, cs = %f, alpha = %f, inv = %f\n", */
    /*        omega, sn, cs, alpha, inv); */
    lp[2] = lp[0] =  inv*(1.0 - cs)*0.5;
    lp[1] =  lp[0]+lp[0];
    lp[3] =  (-2.0*cs*inv);
    lp[4] =  ((1.0 - alpha)*inv);
    /* printf("lp_rbj: %f %f %f %f %f\n", lp[0], lp[1], lp[2], lp[3], lp[4]); */
}

static int32_t exciter_init(CSOUND *csound, EXCITER *p)
{
    /* Zero is a valid cutoff and must also rebuild coefficients on reinit. */
    p->freq_old = p->ceil_old = FL(-1.0);
    p->hp1[5] = p->hp2[5] = p->hp3[5] = p->hp4[5] = 0.0;
    p->hp1[6] = p->hp2[6] = p->hp3[6] = p->hp4[6] = 0.0;
    p->lp1[5] = p->lp2[5] = 0.0;
    p->lp1[6] = p->lp2[6] = 0.0;
    p->rs00[5] = p->rs01[5] = p->rs10[5] = p->rs11[5] = 0.0;
    p->rs00[6] = p->rs01[6] = p->rs10[6] = p->rs11[6] = 0.0;
    p->rdrive = p->rbdr = p->kpa = p->kpb = p->kna = p->knb = p->ap =
      p->an = p->imr = p->kc = p->srct = p->sq = p->pwrq = p->prev_med =
      p->prev_out = 0.0;
    p->blend_old = p->drive_old = -1.0;
    //resample_set_params(csound, p);
    {
      cs_double srate = (cs_double)CS_ESR;
      cs_double ff = 25000.0;
      /* The resampling filters run at 2 * srate, with Nyquist at srate. */
      if (ff >= srate || srate > 50000) ff = srate*0.5;
      // set all filters
      set_lp_rbj(p->rs00, ff, 0.8, srate * 2);
      /* printf("resample filter: %f %f %f %f %f %f %f\n", */
      /*        p->rs00[0],p->rs00[1],p->rs00[2],p->rs00[3],p->rs00[4], */
      /*        p->rs00[5],p->rs00[6]); */
      memcpy(p->rs01, p->rs00, 5*sizeof(cs_double));
      memcpy(p->rs10, p->rs00, 5*sizeof(cs_double));
      memcpy(p->rs11, p->rs00, 5*sizeof(cs_double));
    }
    return OK;
}

void upsample(EXCITER *p, cs_double *tmp, cs_double sample)
{
    cs_double tt = process(p->rs00,sample);
    tmp[0] = process(p->rs01,tt);
    //printf("up0:%f -> %f -> %f\n", sample, tt, tmp[0]);
    tt = process(p->rs00,0.0);
    tmp[1] = process(p->rs01,tt);
    //printf("up1:%f -> %f -> %f\n", 0.0, tt, tmp[1]);
    return;
}

cs_double downsample(EXCITER *p, cs_double *sample)
{
    //printf("downsample: %f %f ->", sample[0], sample[1]);
    sample[0] = process(p->rs10, sample[0]);
    sample[0] = process(p->rs11, sample[0]);
    sample[1] = process(p->rs10, sample[1]);
    sample[1] = process(p->rs11, sample[1]);
    //printf(" %f\n", sample[0]);
    return sample[0];
}

static inline cs_double M(cs_double x)
{
    return (fabs(x) > 0.00000001) ? x : 0.0;
}

static inline cs_double D(cs_double x)
{
    x = fabs(x);
    return (x > 0.00000001) ? sqrt(x) : 0.0;
}

static inline cs_double distort(EXCITER *p, cs_double in)
{
    cs_double samples[2], ans;
    int32_t i;
    cs_double ap = p->ap, an = p->an, kpa = p->kpa, kna = p->kna,
          kpb = p->kpb, knb = p->knb, pwrq = p->pwrq;
    //printf("in: %f\n", in);
    upsample(p, samples, in);
    /* Upsampling and downsampling always process two samples. */
    for (i = 0; i < 2; i++) {
      cs_double proc = samples[i];
      cs_double med;
      //printf("%d: %f-> ", i, proc);
      if (proc >= 0.0) {
        med = (D(ap + proc * (kpa - proc)) + kpb) * pwrq;
      } else {
        med = - (D(an - proc * (kna + proc)) + knb) * pwrq;
      }
      proc = p->srct * (med - p->prev_med + p->prev_out);
      //printf("%f\n", proc);
      p->prev_med = M(med);
      p->prev_out = M(proc);
      samples[i] = proc;
    }
    ans = downsample(p, samples);
    //printf("out: %f\n", ans);
    return ans;
}









static inline void set_distort(CSOUND *csound, EXCITER *p)
{
    // set distortion coeffs
    if ((p->drive_old != *p->pdrive) || (p->blend_old != *p->pblend)) {
      cs_double srate = CS_ESR;
      /* printf("drive %f->%f; blend %f->%f\n", */
      /*        p->drive_old, *p->pdrive, p->blend_old, *p->pblend); */
      p->drive_old = *p->pdrive;
      p->blend_old = *p->pblend;
      p->rdrive = 12.0 / p->drive_old;
      p->rbdr = p->rdrive / (10.5 - p->blend_old) * 780.0 / 33.0;
      p->kpa = D(2.0 * (p->rdrive*p->rdrive) - 1.0) + 1.0;
      p->kpb = (2.0 - p->kpa) / 2.0;
      p->ap = ((p->rdrive*p->rdrive) - p->kpa + 1.0) / 2.0;
      p->kc = p->kpa / D(2.0 * D(2.0 * (p->rdrive*p->rdrive) - 1.0) -
                         2.0 * p->rdrive*p->rdrive);
      p->srct = (0.1 * srate) / (0.1 * srate + 1.0);
      p->sq = p->kc*p->kc + 1.0;
      p->knb = -1.0 * p->rbdr / D(p->sq);
      p->kna = 2.0 * p->kc * p->rbdr / D(p->sq);
      p->an = p->rbdr*p->rbdr / p->sq;
      p->imr = 2.0 * p->knb + D(2.0 * p->kna + 4.0 * p->an - 1.0);
      p->pwrq = 2.0 / (p->imr + 1.0);
      /* printf("params: rdrive\trbdr\tkpa\tkpb\tkna\tknb\tap\tan"
                "\timr\tkc\tsrct\tsq\tpwrq\n"); */
      /* printf("\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", */
      /*        p->rdrive, p->rbdr, p->kpa, p->kpb, p->kna, p->knb, p->ap, p->an, */
      /*        p->imr, p->kc, p->srct, p->sq, p->pwrq); */
    }
}


static inline void params_changed(CSOUND *csound, EXCITER *p)
{
    // set the params of all filters
    if (UNLIKELY(*p->pfreq != p->freq_old)) {
      set_hp_rbj(csound, p->hp1, *p->pfreq, 0.707, CS_ESR);
      memcpy(p->hp2, p->hp1, 5*sizeof(cs_double));
      memcpy(p->hp3, p->hp1, 5*sizeof(cs_double));
      memcpy(p->hp4, p->hp1, 5*sizeof(cs_double));
      p->freq_old = *p->pfreq;
    }
    // set the params of all filters
    if (UNLIKELY(*p->pceil != p->ceil_old)) {
      set_lp_rbj(p->lp1, *p->pceil, 0.707, (cs_double)CS_ESR);
      memcpy(p->lp2, p->lp1, 5*sizeof(cs_double));
      p->ceil_old = *p->pceil;
    }
    // set distortion
    set_distort(csound, p);
}

int32_t exciter_perf(CSOUND *csound, EXCITER *p)
//uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float zerodb = csound->Get0dBFS(csound);

    if (UNLIKELY(offset)) memset(p->aout, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout[nsmps], '\0', early*sizeof(cs_float));
    }
    params_changed(csound, p);
   // process
    for (n = offset; n<nsmps; n++) {
      // cycle through samples
      cs_double out, in, out1;
      in = (cs_double)p->ain[n]/zerodb;
      // all pre filters in chain
      //printf("**** %f ****\n", in);
      out1 = process(p->hp2, process(p->hp1, in));
      out = distort(p, out1);      // saturate
      //printf("after distort %f -> %f -> %f\n", in, out1, out);
      // all post filters in chain
      out = process(p->hp4, process(p->hp3, out));

      // all H/P post filters in chain (surely LP - JPff)
      out = process(p->lp1, process(p->lp2, out));
      p->aout[n] = out*zerodb;
    } // cycle through samples
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY exciter_localops[] = {
  { "exciter", S(EXCITER),   0, "a", "akkkk",
                             (SUBR)exciter_init, (SUBR)exciter_perf },
};

LINKAGE_BUILTIN(exciter_localops)
