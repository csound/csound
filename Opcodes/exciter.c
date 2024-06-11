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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
  MYFLT       *aout;
  MYFLT       *ain;
  MYFLT       *pfreq;
  MYFLT       *pceil;
  MYFLT       *pdrive;
  MYFLT       *pblend;
  // Internals
  MYFLT       freq_old, ceil_old;
  // biquad data
  double      hp1[7], hp2[7], hp3[7], hp4[7];
  double      lp1[7],  lp2[7];
  // resampler
  double      rs00[7], rs01[7], rs10[7], rs11[7];
  // distortion
  double      rdrive, rbdr, kpa, kpb, kna, knb, ap, an, imr, kc, srct, sq, pwrq;
  int32_t         over;
  double      prev_med, prev_out;
  double      blend_old, drive_old;
} EXCITER;

static inline double process(double st[7], double in/*, char *s */)
{
    double tmp = in - st[5] * st[3] - st[6] * st[4];
    double out = tmp * st[0] + st[5] * st[1] + st[6] * st[2];
    st[6] = st[5];
    st[5] = tmp;
    /* printf("%s: %f -> %f; %f %f\n", s, in, out, st[5], st[6]); */
    return out;
}

    /** Highpass filter based on Robert Bristow-Johnson's equations
     * @param fc     resonant frequency
     * @param q      resonance (gain at fc)
     */
static inline void set_hp_rbj(CSOUND *csound, double hp[7], double fc, double q, MYFLT sr)
{
    double omega= (TWOPI*fc/(double) sr);
    double sn=sin(omega);
    double cs=cos(omega);
    double alpha=(double)(sn/(2.0*q));
    double inv=(double)(1.0/(1.0+alpha));

    hp[2]/*a2*/ = hp[0]/*a0*/ =  (inv*(1.0 + cs)*0.5);
    hp[1]/*a1*/ =  -2.0 * hp[0];
    hp[3]/*b1*/ =  (-2.0*cs*inv);
    hp[4]/*b2*/ =  ((1.0 - alpha)*inv);
    /* printf("hp_rbj: %f %f %f %f %f\n", hp[0], hp[1], hp[2], hp[3], hp[4]); */
    return;
}

static inline void set_lp_rbj(double lp[7], double fc, double q, double sr)
{
    double omega=(TWOPI*fc/sr);
    double sn=sin(omega);
    double cs=cos(omega);
    double alpha=(sn/(2*q));
    double inv=(1.0/(1.0+alpha));

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
    p->freq_old =  p->ceil_old = FL(0.0);
    p->hp1[5] = p->hp2[5] = p->hp3[5] = p->hp4[5] = 0.0;
    p->hp1[6] = p->hp2[6] = p->hp3[6] = p->hp4[6] = 0.0;
    p->lp1[5] = p->lp2[5] = 0.0;
    p->lp1[6] = p->lp2[6] = 0.0;
    p->rs00[5] = p->rs01[5] = p->rs10[5] = p->rs11[5] = 0.0;
    p->rs00[6] = p->rs01[6] = p->rs10[6] = p->rs11[6] = 0.0;
    p->rdrive = p->rbdr = p->kpa = p->kpb = p->kna = p->knb = p->ap =
      p->an = p->imr = p->kc = p->srct = p->sq = p->pwrq = p->prev_med =
      p->prev_out = 0.0;
    p->over = CS_ESR * 2 > 96000 ? 1 : 2;
    p->blend_old = p->drive_old = -1.0;
    //resample_set_params(csound, p);
    {
      double srate = (double)CS_ESR;
      double ff = 25000.0;
      if (srate>50000) ff = srate*0.5;
      // set all filters
      set_lp_rbj(p->rs00, ff, 0.8, srate * 2);
      /* printf("resample filter: %f %f %f %f %f %f %f\n", */
      /*        p->rs00[0],p->rs00[1],p->rs00[2],p->rs00[3],p->rs00[4], */
      /*        p->rs00[5],p->rs00[6]); */
      memcpy(p->rs01, p->rs00, 5*sizeof(double));
      memcpy(p->rs10, p->rs00, 5*sizeof(double));
      memcpy(p->rs11, p->rs00, 5*sizeof(double));
    }
    return OK;
}

void upsample(EXCITER *p, double *tmp, double sample)
{
    double tt = process(p->rs00,sample);
    tmp[0] = process(p->rs01,tt);
    //printf("up0:%f -> %f -> %f\n", sample, tt, tmp[0]);
    tt = process(p->rs00,0.0);
    tmp[1] = process(p->rs01,tt);
    //printf("up1:%f -> %f -> %f\n", 0.0, tt, tmp[1]);
    return;
}

double downsample(EXCITER *p, double *sample)
{
    //printf("downsample: %f %f ->", sample[0], sample[1]);
    sample[0] = process(p->rs10, sample[0]);
    sample[0] = process(p->rs11, sample[0]);
    sample[1] = process(p->rs10, sample[1]);
    sample[1] = process(p->rs11, sample[1]);
    //printf(" %f\n", sample[0]);
    return sample[0];
}

static inline double M(double x)
{
    return (fabs(x) > 0.00000001) ? x : 0.0;
}

static inline double D(double x)
{
    x = fabs(x);
    return (x > 0.00000001) ? sqrt(x) : 0.0;
}

static inline double distort(EXCITER *p, double in)
{
    double samples[2], ans;
    int32_t i;
    double ap = p->ap, an = p->an, kpa = p->kpa, kna = p->kna,
          kpb = p->kpb, knb = p->knb, pwrq = p->pwrq;
    //printf("in: %f\n", in);
    upsample(p, samples, in);
    for (i = 0; i < p->over; i++) {
      double proc = samples[i];
      double med;
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
      double srate = CS_ESR;
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
      memcpy(p->hp2, p->hp1, 5*sizeof(double));
      memcpy(p->hp3, p->hp1, 5*sizeof(double));
      memcpy(p->hp4, p->hp1, 5*sizeof(double));
      p->freq_old = *p->pfreq;
    }
    // set the params of all filters
    if (UNLIKELY(*p->pceil != p->ceil_old)) {
      set_lp_rbj(p->lp1, *p->pceil, 0.707, (double)CS_ESR);
      memcpy(p->lp2, p->lp1, 5*sizeof(double));
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
    MYFLT zerodb = csound->Get0dBFS(csound);

    if (UNLIKELY(offset)) memset(p->aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    params_changed(csound, p);
   // process
    for (n = offset; n<nsmps; n++) {
      // cycle through samples
      double out, in, out1;
      in = (double)p->ain[n]/zerodb;
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
