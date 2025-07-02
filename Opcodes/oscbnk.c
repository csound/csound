/*
  oscbnk.c:

  Copyright (C) 2002, 2005 Istvan Varga

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

#include "stdopcod.h"
#include "oscbnk.h"
#include <math.h>

static inline STDOPCOD_GLOBALS *get_oscbnk_globals(CSOUND *csound)
{
  return (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
}

/* ---- oscbnk, grain2, and grain3 - written by Istvan Varga, 2001 ---- */

/* update random seed */

static CS_PURE int32 oscbnk_rand31(int32 seed)
{
  uint64_t tmp1;
  uint32_t tmp2;

  /* x = (16807 * x) % 0x7FFFFFFF */
  tmp1 = (uint64_t) ((int32_t) seed * (int64_t) 16807);
  tmp2 = (uint32_t) tmp1 & (uint32_t) 0x7FFFFFFF;
  tmp2 += (uint32_t) (tmp1 >> 31);
  if ((int32_t) tmp2 < (int32_t) 0)
    tmp2 = (tmp2 + (uint32_t) 1) & (uint32_t) 0x7FFFFFFF;
  return (int32) tmp2;
}

/* initialise random seed */

static void oscbnk_seedrand(CSOUND *csound, int32 *seed, MYFLT seedval)
{
  *seed = (int32) ((double) seedval + 0.5);
  if (*seed < 1L) {                   /* seed from current time */
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    if (pp->oscbnk_seed > 0UL)
      pp->oscbnk_seed += 23UL;
    else
      pp->oscbnk_seed = (uint32) csound->GetRandomSeedFromTime();
    pp->oscbnk_seed = ((pp->oscbnk_seed - 1UL) % 0x7FFFFFFEUL) + 1UL;
    *seed = (int32) pp->oscbnk_seed;
  }
  else {
    *seed = ((*seed - 1L) % 0x7FFFFFFEL) + 1L;
  }
  *seed = oscbnk_rand31(*seed);
  *seed = oscbnk_rand31(*seed);
}

/* return a random phase value between 0 and OSCBNK_PHSMAX */

static uint32 oscbnk_rnd_phase(int32 *seed)
{
  /* update random seed */
  *seed = oscbnk_rand31(*seed);
  /* convert seed to phase */
  return ((uint32) *seed >> OSCBNK_RNDPHS);
}

/* return a random value between -1 and 1 */

static MYFLT oscbnk_rnd_bipolar(int32_t *seed, MYFLT rpow, int32_t rmode)
{
  double      x;
  MYFLT       s;

  /* update random seed */

  *seed = oscbnk_rand31(*seed);

  /* convert to floating point */

  x = (double) (*seed - 0x3FFFFFFFL) * (1.0 / 1073741823.015625);

  if (!(rmode)) return ((MYFLT) x);           /* uniform distribution */

  /* change distribution */

  s = (x < 0.0 ? FL(-1.0) : FL(1.0));         /* sign                 */
  x = fabs(x);                                /* absolute value       */
  if (rmode == 2) x = fabs(1.0 - x);
  x = pow(x, (double) rpow);
  if (rmode == 2) x = 1.0 - x;

  return ((MYFLT) x * s);
}

/* set ftable parameters (mask etc.) according to table length */

static void oscbnk_flen_setup(int32 flen, uint32 *mask,
                              uint32 *lobits, MYFLT *pfrac);

/* Update random seed, and return next value from parameter table (if   */
/* enabled) or random value between 0 and 1. If output table is present */
/* store value in table.                                                */

static MYFLT oscbnk_rand(OSCBNK *p)
{
  MYFLT           y;

  /* update random seed */

  p->seed = oscbnk_rand31(p->seed);

  /* convert to float */

  y = (MYFLT) (p->seed - 1L) / (MYFLT) 0x7FFFFFFDL;

  /* read from parameter table (if exists) */

  if ((p->tabl_cnt < p->tabl_len) && (p->tabl[p->tabl_cnt] >= FL(0.0)))
    y = p->tabl[p->tabl_cnt];
  switch (p->tabl_cnt % 5) {
  case 0:                                 /* wrap phase */
  case 1:
  case 3:
    y -= (MYFLT) ((int32) y); break;
  default:
    if (y > FL(1.0)) y = FL(1.0);  /* limit frequency */
  }

  /* store in output table */

  if (p->tabl_cnt < p->outft_len) p->outft[p->tabl_cnt] = y;

  p->tabl_cnt++;
  return y;
}

/* Read from ft at phase with linear interpolation. flen is the table   */
/* length. Phase is limited to the range 0 - 1.                         */

static MYFLT oscbnk_interp_read_limit(MYFLT phase, MYFLT *ft, int32_t flen)
{
  MYFLT   x;
  int32_t n;

  if (phase < FL(0.0)) return ft[0];
  else phase *= (MYFLT) flen;
  n = (int32) phase; phase -= (MYFLT) n;
  if (UNLIKELY(n >= flen)) return ft[flen];
  else { x = ft[n]; }
  x += phase * (ft[++n] - x);
  //printf("**** (%d) x = %f\n", __LINE__, x);
  return x;
}

/* LFO / modulation */

static void oscbnk_lfo(OSCBNK *p, OSCBNK_OSC *o)
{
  uint32   n;
  int32_t  eqmode;
  MYFLT   f, l, q, k, kk, vk, vkk, vkdq, sq;
  MYFLT   lfo1val = FL(0.0), lfo2val = FL(0.0);

  /* lfo1val = LFO1 output, lfo2val = LFO2 output */
  if(p->floatph) {
    if (p->ilfomode & 0xF0) {
      MYFLT frac, pos = o->LFO1phsf*p->flen1;
      n = (int32_t) pos;
      frac = pos - n;
      lfo1val = p->l1t[n] + frac*(p->l1t[n+1] - p->l1t[n+1]); 
      /* update phase */
      f = o->LFO1frq * p->lf1_scl + p->lf1_ofs;
      o->LFO1phsf = PHMOD1(o->LFO1phsf + f);
    }
    if (p->ilfomode & 0x0F) {                       /* LFO 2 */
      MYFLT frac, pos = o->LFO2phsf*p->flen2;
      n = (int32_t) pos;
      frac = pos - n;
      lfo2val = p->l2t[n] + frac*(p->l2t[n+1] - p->l2t[n+1]); 
      /* update phase */
      f = o->LFO2frq * p->lf2_scl + p->lf2_ofs;
      o->LFO2phsf = PHMOD1(o->LFO2phsf + f);
    }
  } else {  
    if (p->ilfomode & 0xF0) {                       /* LFO 1 */
      n = o->LFO1phs >> p->l1t_lobits; lfo1val = p->l1t[n++];
      lfo1val += (p->l1t[n] - lfo1val)
        * (MYFLT) ((int32) (o->LFO1phs & p->l1t_mask)) * p->l1t_pfrac;
      /* update phase */
      f = o->LFO1frq * p->lf1_scl + p->lf1_ofs;
      o->LFO1phs = (o->LFO1phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    }

    if (p->ilfomode & 0x0F) {                       /* LFO 2 */
      n = o->LFO2phs >> p->l2t_lobits; lfo2val = p->l2t[n++];
      lfo2val += (p->l2t[n] - lfo2val)
        * (MYFLT) ((int32) (o->LFO2phs & p->l2t_mask)) * p->l2t_pfrac;
      /* update phase */
      f = o->LFO2frq * p->lf2_scl + p->lf2_ofs;
      o->LFO2phs = (o->LFO2phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    }
  }

  /* modulate phase, frequency, and amplitude */

  o->osc_frq = FL(0.0);
  if (p->ilfomode & 0x88) {               /* FM */
    if (p->ilfomode & 0x80) o->osc_frq += lfo1val;
    if (p->ilfomode & 0x08) o->osc_frq += lfo2val;
    o->osc_frq = o->osc_frq * *(p->args[3]);
  }

  if (p->ilfomode & 0x44) {               /* AM */
    o->osc_amp = FL(0.0);
    if (p->ilfomode & 0x40) o->osc_amp += lfo1val;
    if (p->ilfomode & 0x04) o->osc_amp += lfo2val;
    o->osc_amp--; o->osc_amp *= *(p->args[2]); o->osc_amp++;
  }
  else {
    o->osc_amp = FL(1.0);
  }

  o->osc_phm = FL(0.0);                   /* PM */
  if (p->ilfomode & 0x22) {
    if (p->ilfomode & 0x20) o->osc_phm += lfo1val;
    if (p->ilfomode & 0x02) o->osc_phm += lfo2val;
    o->osc_phm *= *(p->args[4]);
  }

  if ((eqmode = p->ieqmode) < 0) return;          /* EQ disabled  */
  /* modulate EQ */

  f = l = q = FL(0.0);
  lfo1val = lfo1val * FL(0.5) + FL(0.5);
  lfo2val = lfo2val * FL(0.5) + FL(0.5);
  if (p->ilfomode & 0x10) {               /* LFO1 to EQ */
    f += oscbnk_interp_read_limit(lfo1val, p->eqft, p->eqft_len);
    l += oscbnk_interp_read_limit(lfo1val, p->eqlt, p->eqlt_len);
    q += oscbnk_interp_read_limit(lfo1val, p->eqqt, p->eqqt_len);
  }
  if (p->ilfomode & 0x01) {               /* LFO2 to EQ */
    f += oscbnk_interp_read_limit(lfo2val, p->eqft, p->eqft_len);
    l += oscbnk_interp_read_limit(lfo2val, p->eqlt, p->eqlt_len);
    q += oscbnk_interp_read_limit(lfo2val, p->eqqt, p->eqqt_len);
  }
  /* calculate EQ frequency, level, and Q */
  f *= p->eqo_scl; f += p->eqo_ofs;
  l *= p->eql_scl; l += p->eql_ofs;
  q *= p->eqq_scl; q += p->eqq_ofs;
  f  = FABS(f); q = FABS(q);
  /* EQ code taken from biquad.c */

  sq = l<FL(0.0) ? FL(0.0) : SQRT(l+l);                   /* level     */
  /* frequency */
  k = TAN(((eqmode == 2 ? (PI_F - f) : f) * FL(0.5)));
  kk = k * k; vk = l * k; vkk = l * kk; vkdq = vk / q;    /* Q         */

  if (eqmode != 0) {
    o->b0 = FL(1.0) + sq * k + vkk;
    o->b1 = FL(2.0) * (vkk - FL(1.0));
    o->b2 = FL(1.0) - sq * k + vkk;
  }
  else {
    o->b0 = FL(1.0) + vkdq + kk;
    o->b1 = FL(2.0) * (kk - FL(1.0));
    o->b2 = FL(1.0) - vkdq + kk;
  }
  l = FL(1.0) + (k / q) + kk;                 /* l = a0 */
  o->a1 = FL(2.0) * (kk - FL(1.0));
  o->a2 = FL(1.0) - (k / q) + kk;
  if (eqmode == 2) {
    o->a1 = -(o->a1);
    o->b1 = -(o->b1);
  }
  l = FL(1.0) / l;
  o->a1 *= l; o->a2 *= l; o->b0 *= l; o->b1 *= l; o->b2 *= l;
  //printf("**** (%d) a1, a2 = %f, %f\n", __LINE__, o->a1, o->a2);
}

/* ---------------- oscbnk set-up ---------------- */

static int32_t oscbnkset(CSOUND *csound, OSCBNK *p)
{
  uint32_t i;
  FUNC     *ftp;
  MYFLT    x;

  p->init_k = 1;
  p->nr_osc = (int32_t) MYFLT2LONG(*(p->args[5]));        /* number of oscs */
  if (p->nr_osc <= 0) p->nr_osc = -1;                     /* no output */
  oscbnk_seedrand(csound, &(p->seed), *(p->args[6]));     /* random seed */
  p->ilfomode = (int32_t) MYFLT2LONG(*(p->args[11])) & 0xFF;  /* LFO mode */
  p->eq_interp = 0;                                       /* EQ mode */
  if (*(p->args[18]) < FL(-0.5)) {
    p->ieqmode = -1; p->ilfomode &= 0xEE;                 /* disable EQ */
  }
  else {
    p->ieqmode = (int32_t) MYFLT2LONG(*(p->args[18]));
    if (p->ieqmode > 2) {
      p->ieqmode -= 3;
    }
    else {
      p->eq_interp = 1;       /* enable interpolation */
    }
    if (p->ieqmode > 2) p->ieqmode = 2;
  }

  /* set up ftables */

  if (p->ilfomode & 0xF0) {
    ftp = csound->FTFind(csound, p->args[20]);    /* LFO 1 */
    if ((ftp == NULL) || ((p->l1t = ftp->ftable) == NULL)) return NOTOK;
    oscbnk_flen_setup(ftp->flen, &(p->l1t_mask), &(p->l1t_lobits),
                      &(p->l1t_pfrac));
    p->flen1 = ftp->flen;
  }
  else {
    p->l1t = NULL;          /* LFO1 not used */
    p->l1t_lobits = p->l1t_mask = 0UL; p->l1t_pfrac = FL(0.0);
  }

  if (p->ilfomode & 0x0F) {
    ftp = csound->FTFind(csound, p->args[21]);    /* LFO 2 */
    if (UNLIKELY((ftp == NULL) || ((p->l2t = ftp->ftable) == NULL)))
      return NOTOK;
    oscbnk_flen_setup(ftp->flen, &(p->l2t_mask), &(p->l2t_lobits),
                      &(p->l2t_pfrac));
    p->flen2 = ftp->flen;
  }
  else {
    p->l2t = NULL;          /* LFO2 not used */
    p->l2t_lobits = p->l2t_mask = 0UL; p->l2t_pfrac = FL(0.0);
  }

  if (p->ieqmode >= 0) {
    ftp = csound->FTFind(csound, p->args[22]);    /* EQ frequency */
    if (UNLIKELY((ftp == NULL) || ((p->eqft = ftp->ftable) == NULL)))
      return NOTOK;
    p->eqft_len = ftp->flen;

    ftp = csound->FTFind(csound, p->args[23]);    /* EQ level */
    if (UNLIKELY((ftp == NULL) || ((p->eqlt = ftp->ftable) == NULL)))
      return NOTOK;
    p->eqlt_len = ftp->flen;

    ftp = csound->FTFind(csound, p->args[24]);    /* EQ Q */
    if (UNLIKELY((ftp == NULL) || ((p->eqqt = ftp->ftable) == NULL)))
      return NOTOK;
    p->eqqt_len = ftp->flen;
  }
  else {
    p->eqft = p->eqlt = p->eqqt = NULL;     /* EQ disabled */
    p->eqft_len = p->eqlt_len = p->eqqt_len = 0L;
  }

  if (*(p->args[25]) >= FL(1.0)) {        /* parameter table */
    ftp = csound->FTFind(csound, p->args[25]);
    if (UNLIKELY((ftp == NULL) || ((p->tabl = ftp->ftable) == NULL)))
      return NOTOK;
    p->tabl_len = ftp->flen;
  }
  else {
    p->tabl = NULL; p->tabl_len = 0L;
  }
  p->tabl_cnt = 0L;   /* table ptr. */

  if (*(p->args[26]) >= FL(1.0)) {        /* output table */
    ftp = csound->FTFind(csound, p->args[26]);
    if (UNLIKELY((ftp == NULL) || ((p->outft = ftp->ftable) == NULL)))
      return NOTOK;
    p->outft_len = ftp->flen;
  }
  else {
    p->outft = NULL; p->outft_len = 0L;
  }

  /* allocate space */

  if (p->nr_osc < 1) return OK;
  i = (uint32_t) p->nr_osc * (int32) sizeof (OSCBNK_OSC);
  if ((p->auxdata.auxp == NULL) || (p->auxdata.size < i))
    csound->AuxAlloc(csound, i, &(p->auxdata));
  p->osc = (OSCBNK_OSC *) p->auxdata.auxp;

  memset(p->outft, 0, p->outft_len*sizeof(MYFLT));

  p->floatph = (!IS_POW_TWO(p->flen1)) | (!IS_POW_TWO(p->flen2)); 
  /* initialise oscillators */

  for (i = 0; i < (uint32_t)p->nr_osc; i++) {
    /* oscillator phase */
    x = oscbnk_rand(p);
    p->osc[i].osc_phsf = x;
    p->osc[i].osc_phs = OSCBNK_PHS2INT(x);
    /* LFO1 phase */
    p->osc[i].LFO1phsf = x;
    p->osc[i].LFO1phs = OSCBNK_PHS2INT(x);
    /* LFO1 frequency */
    p->osc[i].LFO1frq = oscbnk_rand(p);
    /* LFO2 phase */
    p->osc[i].LFO2phsf = x;
    p->osc[i].LFO2phs = OSCBNK_PHS2INT(x);     
    /* LFO2 frequency */
    p->osc[i].LFO2frq = oscbnk_rand(p);
    /* EQ data */
    p->osc[i].xnm1 = p->osc[i].xnm2 = FL(0.0);
    p->osc[i].ynm1 = p->osc[i].ynm2 = FL(0.0);
    p->osc[i].b0 = FL(1.0);
    p->osc[i].a1 = p->osc[i].b1 = FL(0.0);
    p->osc[i].a2 = p->osc[i].b2 = FL(0.0);
  }
  return OK;
}

/* ---------------- oscbnk performance ---------------- */

static int32_t oscbnk(CSOUND *csound, OSCBNK *p)
{
  int32_t osc_cnt, pm_enabled, am_enabled;
  FUNC    *ftp;
  MYFLT   *ft;
  uint32  n, lobits, mask, ph, f_i, flen;
  MYFLT   pfrac, pm, a, f, a1, a2, b0, b1, b2, phf;
  MYFLT   k, a_d = FL(0.0), a1_d = FL(0.0), a2_d = FL(0.0),
    b0_d = FL(0.0), b1_d = FL(0.0), b2_d = FL(0.0);
  MYFLT   yn, xnm1 = FL(0.0), xnm2 = FL(0.0), ynm1 = FL(0.0), ynm2 = FL(0.0);
  OSCBNK_OSC      *o;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t floatph;

  /* clear output signal */
  memset(p->args[0], '\0', nsmps*sizeof(MYFLT));

  if (p->nr_osc == -1) {
    return OK;         /* nothing to render */
  }
  else if (UNLIKELY((p->seed == 0L) || (p->osc == NULL))) goto err1;

  /* check oscillator ftable */
  ftp = csound->FTFind(csound, p->args[19]);
  if (UNLIKELY((ftp == NULL) || ((ft = ftp->ftable) == NULL)))
    return NOTOK;
  flen = ftp->flen;
  floatph = !(IS_POW_TWO(flen));
  if(!floatph) 
    oscbnk_flen_setup(ftp->flen, &(mask), &(lobits), &(pfrac));
  
 
  /* some constants */
  pm_enabled = (p->ilfomode & 0x22 ? 1 : 0);
  am_enabled = (p->ilfomode & 0x44 ? 1 : 0);
  p->frq_scl = CS_ONEDSR;                 /* osc. freq.   */
  p->lf1_scl = (*(p->args[8]) - *(p->args[7])) * CS_ONEDKR;
  p->lf1_ofs = *(p->args[7]) * CS_ONEDKR;      /* LFO1 freq.   */
  p->lf2_scl = (*(p->args[10]) - *(p->args[9])) * CS_ONEDKR;
  p->lf2_ofs = *(p->args[9]) * CS_ONEDKR;      /* LFO2 freq.   */
  if (p->ieqmode >= 0) {
    MYFLT fmax =  *(p->args[13]);
    MYFLT fmin =  *(p->args[12]);

    /* VL: min freq cannot be > max freq */
    fmin = fmin < fmax ? fmin : fmax;
    p->eqo_scl = (fmax - fmin) * CS_TPIDSR;
    p->eqo_ofs = fmin * CS_TPIDSR;   /* EQ omega */
    p->eql_scl = *(p->args[15]) - (p->eql_ofs= *(p->args[14]));/* EQ level */
    p->eqq_scl = *(p->args[17]) - (p->eqq_ofs= *(p->args[16]));/* EQ Q     */
  }

  if (UNLIKELY(early)) nsmps -= early;
  for (osc_cnt = 0, o = p->osc; osc_cnt < p->nr_osc; osc_cnt++, o++) {
    if (p->init_k) oscbnk_lfo(p, o);
    ph = o->osc_phs;   /* phase        */
    phf = o->osc_phsf;
    pm = o->osc_phm;                        /* phase mod.   */
    if ((p->init_k) && (pm_enabled)) {
      f = pm - (MYFLT) ((int32) pm);
      if(floatph) phf = PHMOD1(phf + f);
      else ph = (ph + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    }
    a = o->osc_amp;                         /* amplitude    */
    f = o->osc_frq;                         /* frequency    */
    if (p->ieqmode < 0) {           /* EQ disabled */
      oscbnk_lfo(p, o);
      /* initialise ramps */
      f = ((o->osc_frq + f) * FL(0.5) + *(p->args[1])) * p->frq_scl;
      if (pm_enabled) {
        f += (MYFLT) ((double) o->osc_phm - (double) pm) / (nsmps-offset);
        f -= (MYFLT) ((int32) f);
      }
      f_i = OSCBNK_PHS2INT(f);
      if (am_enabled) a_d = (o->osc_amp - a)  / (nsmps-offset);
      /* oscillator */
      for (nn = offset; nn < nsmps; nn++) {
        /* read from table */
        if(floatph) {
          MYFLT frac;
          MYFLT pos = phf*flen;
          n = (int32_t) pos;
          frac = pos - n;
          k = ft[n] + frac*(ft[n+1] - ft[n]);
          phf = PHMOD1(phf + f);
        } else {
          n = ph >> lobits; k = ft[n++];
          k += (ft[n] - k) * (MYFLT) ((int32) (ph & mask)) * pfrac;
          /* update phase */
          ph = (ph + f_i) & OSCBNK_PHSMSK;
        }
        /* amplitude modulation */
        if (am_enabled) k *= (a += a_d);
        /* mix to output */
        p->args[0][nn] += k;

      }
    }
    else {                        /* EQ enabled */
      a1 = o->a1; a2 = o->a2;         /* EQ coeffs    */
      b0 = o->b0; b1 = o->b1; b2 = o->b2;
      xnm1 = o->xnm1; xnm2 = o->xnm2;
      ynm1 = o->ynm1; ynm2 = o->ynm2;
      oscbnk_lfo(p, o);
      /* initialise ramps */
      f = ((o->osc_frq + f) * FL(0.5) + *(p->args[1])) * p->frq_scl;
      if (pm_enabled) {
        f += (MYFLT) ((double) o->osc_phm - (double) pm) / (nsmps-offset);
        f -= (MYFLT) ((int32) f);
      }
      f_i = OSCBNK_PHS2INT(f);
      if (am_enabled) a_d = (o->osc_amp - a) / (nsmps-offset);
      if (p->eq_interp) {     /* EQ w/ interpolation */
        a1_d = (o->a1 - a1) / (nsmps-offset);
        a2_d = (o->a2 - a2) / (nsmps-offset);
        b0_d = (o->b0 - b0) / (nsmps-offset);
        b1_d = (o->b1 - b1) / (nsmps-offset);
        b2_d = (o->b2 - b2) / (nsmps-offset);
        /* oscillator */
        for (nn = offset; nn < nsmps; nn++) {
          /* update ramps */
          a1 += a1_d; a2 += a2_d;
          b0 += b0_d; b1 += b1_d; b2 += b2_d;
          /* read from table */
          if(floatph) {
            MYFLT frac;
            MYFLT pos = phf*flen;
            n = (int32_t) pos;
            frac = pos - n;
            k = ft[n] + frac*(ft[n+1] - ft[n]);
            phf = PHMOD1(phf + f);
          } else {
            n = ph >> lobits; k = ft[n++];
            k += (ft[n] - k) * (MYFLT) ((int32) (ph & mask)) * pfrac;
            /* update phase */
            ph = (ph + f_i) & OSCBNK_PHSMSK;
          }
          /* amplitude modulation */
          if (am_enabled) k *= (a += a_d);
          /* EQ */
          yn = b2 * xnm2; yn += b1 * (xnm2 = xnm1); yn += b0 * (xnm1 = k);
          yn -= a2 * ynm2; yn -= a1 * (ynm2 = ynm1); ynm1 = yn;
          /* mix to output */
          //if (yn>1) {
          //  printf("**** (%d) yn = %f\n", __LINE__, yn);
          // printf("**** a1 = %f a2 = %f; %f\n",
          //         a1, a2, 0.5*(-a1+ sqrt(a1*a1-4*a2)/a2));
          //}
          p->args[0][nn] += yn;
          //if (p->args[0][nn]>1)
          //  printf("**** (%d) out%d = %f\n", __LINE__, nn, p->args[0][nn]);
        }
        /* save EQ coeffs */
        o->a1 = a1; o->a2 = a2;
        o->b0 = b0; o->b1 = b1; o->b2 = b2;
      }
      else {                /* EQ w/o interpolation */
        /* oscillator */
        a1 = o->a1; a2 = o->a2;         /* EQ coeffs    */
        b0 = o->b0; b1 = o->b1; b2 = o->b2;
        for (nn = offset; nn < nsmps; nn++) {
          /* read from table */
          if(floatph) {
            MYFLT frac;
            MYFLT pos = phf*flen;
            n = (int32_t) pos;
            frac = pos - n;
            k = ft[n] + frac*(ft[n+1] - ft[n]);
            phf = PHMOD1(phf + f);
          } else {            
            n = ph >> lobits; k = ft[n++];
            k += (ft[n] - k) * (MYFLT) ((int32) (ph & mask)) * pfrac;
            /* update phase */
            ph = (ph + f_i) & OSCBNK_PHSMSK;
          }
          /* amplitude modulation */
          if (am_enabled) k *= (a += a_d);
          /* EQ */
          yn = b2 * xnm2; yn += b1 * (xnm2 = xnm1); yn += b0 * (xnm1 = k);
          yn -= a2 * ynm2; yn -= a1 * (ynm2 = ynm1); ynm1 = yn;
          /* mix to output */
          p->args[0][nn] += yn;
        }
        /* save EQ coeffs */
        o->a1 = a1; o->a2 = a2;
        o->b0 = b0; o->b1 = b1; o->b2 = b2;
      }
    }
    o->xnm1 = xnm1; o->xnm2 = xnm2; /* save EQ state */
    o->ynm1 = ynm1; o->ynm2 = ynm2;
    /* save amplitude and phase */
    o->osc_amp = a;
    o->osc_phs = ph;
  }
  p->init_k = 0;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("oscbnk: not initialised"));
}

/* ---------------- grain2 set-up ---------------- */

static int32_t grain2set(CSOUND *csound, GRAIN2 *p)
{
  int32_t  i;
  FUNC     *ftp;
  uint32_t n;
  double   x, y;

  /* check opcode params */

  i = (int32_t) MYFLT2LONG(*(p->imode));  /* mode */
  if (i & 1) return OK;               /* skip initialisation */
  p->init_k = 1;
  p->mode = i & 0x0E;
  p->nr_osc = (int32_t) MYFLT2LONG(*(p->iovrlp));   /* nr of oscillators */
  if (p->nr_osc < 1) p->nr_osc = -1;
  oscbnk_seedrand(csound, &(p->seed), *(p->iseed)); /* initialise seed */
  p->rnd_pow = *(p->irpow);                         /* random distribution */
  if ((p->rnd_pow == FL(0.0)) || (p->rnd_pow == FL(-1.0)) ||
      (p->rnd_pow == FL(1.0))) {
    p->rnd_pow = FL(1.0); p->rnd_mode = 0;
  }
  else if (p->rnd_pow < FL(0.0)) {
    p->rnd_pow = -(p->rnd_pow); p->rnd_mode = 2;
  }
  else {
    p->rnd_mode = 1;
  }
  ftp = csound->FTFind(csound, p->iwfn);          /* window table */
  if (UNLIKELY((ftp == NULL) || ((p->wft = ftp->ftable) == NULL))) return NOTOK;
  oscbnk_flen_setup(ftp->flen, &(p->wft_mask), &(p->wft_lobits),
                    &(p->wft_pfrac));
  p->wflen = ftp->flen;
  p->floatph = !IS_POW_TWO(ftp->flen);
  /* allocate space */

  if (p->nr_osc == -1) return OK;                 /* no oscillators */
  n = (uint32_t) p->nr_osc * (int32) sizeof(GRAIN2_OSC);
  if ((p->auxdata.auxp == NULL) || (p->auxdata.size < n))
    csound->AuxAlloc(csound, n, &(p->auxdata));
  p->osc = (GRAIN2_OSC *) p->auxdata.auxp;

  /* initialise oscillators */
  if(p->floatph) {
    y = 1. / (double) p->nr_osc;
    x = 1.;  
    for (i = 0; i < p->nr_osc; i++) {
      if ((x -= y) < 0.0) x = 0.0;
      p->osc[i].window_phsf = (uint32) x;
    }
  } else {
    y = (double) OSCBNK_PHSMAX / (double) p->nr_osc;
    x = (double) OSCBNK_PHSMAX + 0.5;
    for (i = 0; i < p->nr_osc; i++) {
      if ((x -= y) < 0.0) x = 0.0;
      p->osc[i].window_phs = (uint32) x;
    }
  }
  return OK;
}

/* ---------------- grain2 performance ---------------- */

/* set initial phase of grains with start time less than zero */
static void grain2_init_grain_phase(GRAIN2_OSC *o, uint32 frq,
                                    uint32 w_frq, MYFLT frq_scl,
                                    int32_t f_nolock)
{
  double  d;
  MYFLT   f;

  if (!(w_frq)) return;
  if (f_nolock) {
    d = (double) o->grain_frq_flt * (double) frq_scl
      * (double) OSCBNK_PHSMAX + (double) frq;
  }
  else {
    d = (double) o->grain_frq_int;
  }
  d *= (double) o->window_phs / ((double) w_frq * (double) OSCBNK_PHSMAX);
  d -= (double) ((int32) d);
  f = (MYFLT) d;
  o->grain_phs = (o->grain_phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
}


/* set initial phase of grains with start time less than zero */
static void grain2_init_grain_phase_f(GRAIN2_OSC *o, MYFLT frq,
                                      MYFLT w_frq, MYFLT frq_scl,
                                      int32_t f_nolock){ 
  double  d;
  MYFLT   f;

  if (!(w_frq)) return;
  if (f_nolock) {
    d = o->grain_frq_flt * frq_scl + frq;
  }
  else {
    d = (double) o->grain_frq_flt ;
  }
  d *= o->window_phsf / w_frq;
  d -= (double) ((int32) d);
  f = (MYFLT) d;
  o->grain_phsf = PHMOD1(o->grain_phsf + f);
}

  

/* initialise grain */
static void grain2_init_grain(GRAIN2 *p, GRAIN2_OSC *o)
{
  MYFLT   f;
  f = oscbnk_rnd_bipolar(&(p->seed), p->rnd_pow, p->rnd_mode);
  if(p->floatph) {
    o->grain_phsf = (double) oscbnk_rnd_phase(&(p->seed))/OSCBNK_PHSMAX; 
    if (p->mode & 2) {
      o->grain_frq_flt = f;
    } else {
      f = p->grain_frq + p->frq_scl * f;
      o->grain_frq = f;
    }
  } else {
    o->grain_phs = oscbnk_rnd_phase(&(p->seed));
    if (p->mode & 2) {
      o->grain_frq_flt = f;
    }
    else {                              /* lock frequency */
      f = p->grain_frq + p->frq_scl * f;
      o->grain_frq_int = OSCBNK_PHS2INT(f);
    }
  }
}

/* ---- grain2 opcode ---- */

static int32_t grain2(CSOUND *csound, GRAIN2 *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t  i, w_interp, g_interp, f_nolock, floatph = 0;
  MYFLT    *aout, *ft, *w_ft, grain_frq, frq_scl, pfrac, w_pfrac, f, a, k, wf;
  uint32   n, mask, lobits, w_mask, w_lobits, flen, wflen = p->wflen;
  uint32   g_frq, w_frq;
  GRAIN2_OSC  *o;
  FUNC        *ftp;

  /* assign object data to local variables */

  aout     = p->ar;                   /* audio output                 */
  o        = p->osc;                  /* oscillator array             */
  w_interp = (p->mode & 8 ? 1 : 0);   /* interpolate window   */
  g_interp = (p->mode & 4 ? 0 : 1);   /* interpolate grain    */
  f_nolock = (p->mode & 2 ? 1 : 0);   /* don't lock grain frq */
  w_ft     = p->wft;                  /* window ftable                */
  w_mask   = p->wft_mask; w_lobits = p->wft_lobits; w_pfrac = p->wft_pfrac;

  /* clear output signal */
  memset(aout, 0, nsmps*sizeof(MYFLT));
  if (UNLIKELY(early)) nsmps -= early;

  if (p->nr_osc == -1) {
    return OK;                   /* nothing to render */
  }
  else if (UNLIKELY((p->seed == 0L) || (p->osc == NULL))) goto err1;

  /* check grain ftable */

  ftp = csound->FTFind(csound, p->kfn);
  if (UNLIKELY((ftp == NULL) || ((ft = ftp->ftable) == NULL))) return NOTOK;
  flen = ftp->flen;
  floatph |= (((!IS_POW_TWO(flen))) | p->floatph);
  if(!floatph)
    oscbnk_flen_setup(ftp->flen, &mask, &lobits, &pfrac);
  p->floatph = floatph;

  p->grain_frq = grain_frq = *(p->kcps) *  CS_ONEDSR; /* grain freq. */
  p->frq_scl   = frq_scl = *(p->kfmd) *  CS_ONEDSR;

  wf            =  CS_ONEDSR / *(p->kgdur);    /* window frequency    */
  w_frq        = OSCBNK_PHS2INT(wf);

  /* initialisation */

  if (p->init_k) {
    g_frq = OSCBNK_PHS2INT(grain_frq);
    for (i = 0; i < p->nr_osc; i++) {
      grain2_init_grain(p, o + i);
      if(!floatph) grain2_init_grain_phase(o + i, g_frq, w_frq, frq_scl, f_nolock);
      else grain2_init_grain_phase_f(o + i, grain_frq, wf, frq_scl, f_nolock); 
    }
    p->init_k = 0;
  }

  for (i = 0; i < p->nr_osc; i++) {   /* calculate grain frequency */
    if (f_nolock) {
      if(!floatph) {
        f = grain_frq + frq_scl * o[i].grain_frq_flt;
        o[i].grain_frq_int = OSCBNK_PHS2INT(f);
      }
      else {
        o[i].grain_frq = grain_frq + frq_scl * o[i].grain_frq_flt;
      }
    }
  }
  aout = p->ar;                       /* audio output         */
  for (nn = offset; nn<nsmps; nn++) {
    i = p->nr_osc;
    do {
      /* grain waveform */
      if(!floatph) {
        n = o->grain_phs >> lobits; k = ft[n++];
        if (g_interp)
          k += (ft[n] - k) * (MYFLT) ((int32) (o->grain_phs & mask)) * pfrac;
        o->grain_phs += o->grain_frq_int;
        o->grain_phs &= OSCBNK_PHSMSK;
        /* window waveform */
        n = o->window_phs >> w_lobits; a = w_ft[n++];
        if (w_interp)
          a += (w_ft[n] - a) * (MYFLT) ((int32) (o->window_phs & w_mask))
            * w_pfrac;
        o->window_phs += w_frq;
        if (o->window_phs >= OSCBNK_PHSMAX) {
          o->window_phs &= OSCBNK_PHSMSK;       /* new grain    */
          grain2_init_grain(p, o);
          /* grain frequency */
          if (f_nolock) {
            f = grain_frq + frq_scl * o->grain_frq_flt;
            o->grain_frq_int = OSCBNK_PHS2INT(f);
          }
        }
      } else {
        MYFLT pos = o->grain_phsf*flen;
        n = (int32_t) pos;
        k = ft[n];
        if (g_interp) k += (pos - n)*(ft[n+1] - k);
        o->grain_phsf = PHMOD1(o->grain_phsf + o->grain_frq);
        /* window waveform */
        pos = o->window_phsf*wflen;
        n = (int32_t) pos;
        a = w_ft[n];
        if (w_interp) a += (pos - n)*(w_ft[n+1] - k);
        o->window_phsf += wf;
        if (o->window_phsf >= FL(1.0)) {
          o->window_phs = PHMOD1(o->window_phs);       /* new grain    */
          grain2_init_grain(p, o);
          /* grain frequency */
          if (f_nolock) {
            o->grain_frq = grain_frq + frq_scl * o->grain_frq_flt;   
          }
        }
      }
      /* mix to output */
      aout[nn] += a * k;
      o++;            /* next grain */
    } while (--i);
    o -= p->nr_osc;
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("grain2: not initialised"));
}

/* ---------------- grain3 set-up ---------------- */
static int32_t grain3set(CSOUND *csound, GRAIN3 *p)
{
  int32_t   i;
  FUNC      *ftp;
  uint32_t  n;

  /* check opcode params */

  i = (int32_t) MYFLT2LONG(*(p->imode)); /* mode */
  if (i & 1) return OK;                  /* skip initialisation */
  p->init_k = 1;
  p->mode = i & 0x7E;
  p->x_phs = OSCBNK_PHSMAX;

  p->ovrlap = (int32_t) MYFLT2LONG(*(p->imaxovr));        /* max. overlap */
  p->ovrlap = (p->ovrlap < 1 ? 1 : p->ovrlap) + 1;

  oscbnk_seedrand(csound, &(p->seed), *(p->iseed));   /* initialise seed */

  ftp = csound->FTFind(csound, p->iwfn);              /* window table */
  if (UNLIKELY((ftp == NULL) || ((p->wft = ftp->ftable) == NULL))) return NOTOK;
  oscbnk_flen_setup(ftp->flen, &(p->wft_mask), &(p->wft_lobits),
                    &(p->wft_pfrac));
  p->wflen = ftp->flen;
  p->floatph = !IS_POW_TWO(ftp->flen);
  
  /* allocate space */
  n = (uint32_t) p->ovrlap * (int32) sizeof(GRAIN2_OSC);
  n += ((uint32_t) CS_KSMPS + 1L) * (int32) sizeof(double);
  if ((p->auxdata.auxp == NULL) || (p->auxdata.size < n))
    csound->AuxAlloc(csound, n, &(p->auxdata));
  p->phase = (uint32 *) p->auxdata.auxp;
  p->phasef = (double *) p->auxdata.auxp;
  p->osc = (GRAIN2_OSC *) ((uint32 *) p->phase + CS_KSMPS + 1);
  p->osc_start = p->osc;
  p->osc_end = p->osc;
  p->osc_max = p->osc + (p->ovrlap - 1);
  return OK;
}

/* ---------------- grain3 performance ---------------- */

/* initialise grain */

static void grain3_init_grain(GRAIN3 *p, GRAIN2_OSC *o,
                              uint32 w_ph, uint32 g_ph)
{
  MYFLT f;

  /* start phase */
  f = oscbnk_rnd_bipolar(&(p->seed), p->p_rnd_pow, p->p_rnd_mode);
  f *= *(p->kpmd); if (p->pm_wrap) f -= (MYFLT) ((int32) f);
  o->grain_phs = (g_ph + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
  o->window_phs = w_ph;
  /* frequency */
  f = oscbnk_rnd_bipolar(&(p->seed), p->f_rnd_pow, p->f_rnd_mode);
  if (p->mode & 2) {
    o->grain_frq_flt = f;
  }
  else {                              /* lock frequency */
    f *= p->frq_scl;
    o->grain_frq_int = (p->grain_frq + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
  }
}

static void grain3_init_grain_f(GRAIN3 *p, GRAIN2_OSC *o,
                                MYFLT w_ph, MYFLT g_ph)
{
  MYFLT f;

  /* start phase */
  f = oscbnk_rnd_bipolar(&(p->seed), p->p_rnd_pow, p->p_rnd_mode);
  f *= *(p->kpmd); if (p->pm_wrap) f -= (MYFLT) ((int32) f);
  o->grain_phsf = PHMOD1(g_ph + f);
  o->window_phsf = w_ph;
  /* frequency */
  f = oscbnk_rnd_bipolar(&(p->seed), p->f_rnd_pow, p->f_rnd_mode);
  if (p->mode & 2) {
    o->grain_frq_flt = f;
  }
  else {                              /* lock frequency */
    o->grain_frq = p->grain_frqf + f*p->frq_scl;
  }
}


/* ---- grain3 opcode ---- */

static int32_t grain3(CSOUND *csound, GRAIN3 *p)
{
  int32_t           i, w_interp, g_interp, f_nolock;
  MYFLT         *aout0, *aout, *ft, *w_ft, frq_scl, pfrac, w_pfrac, f, a, k;
  MYFLT         wfdivxf, w_frq_f, x_frq_f;
  uint32        n, mask, lobits, w_mask, w_lobits;
  uint32        *phs, frq, x_ph, x_frq, g_ph, g_frq, w_ph, w_frq;
  GRAIN2_OSC    *o;
  FUNC          *ftp;
  uint32_t      offset = p->h.insdshead->ksmps_offset;
  uint32_t      early  = p->h.insdshead->ksmps_no_end;
  uint32_t      nn, nsmps = CS_KSMPS;
  double        *phsf, x_phf, g_phf, g_frqf, frqf, w_phf;
  int32_t       flen, wflen = p->wflen, floatph = 0;

  /* clear output */
  memset(p->ar, 0, nsmps*sizeof(MYFLT));
  if (UNLIKELY(early)) nsmps -= early;

  if (UNLIKELY((p->seed == 0L) || (p->osc == NULL))) goto err1;

  /* assign object data to local variables */

  aout0 = p->ar;                              /* audio output         */
  w_interp = (p->mode & 8 ? 1 : 0);   /* interpolate window   */
  g_interp = (p->mode & 4 ? 0 : 1);   /* interpolate grain    */
  f_nolock = (p->mode & 2 ? 1 : 0);   /* do not lock grain frq */
  w_ft = p->wft;                              /* window ftable        */
  w_mask = p->wft_mask; w_lobits = p->wft_lobits; w_pfrac = p->wft_pfrac;
  x_ph = p->x_phs;
  x_phf = p->x_phsf;

  ftp = csound->FTFind(csound, p->kfn); /* check grain ftable  */
  if (UNLIKELY((ftp == NULL) || ((ft = ftp->ftable) == NULL))) return NOTOK;
  flen = ftp->flen;
  floatph |= (((!IS_POW_TWO(flen))) | p->floatph);
  if(!floatph)
    oscbnk_flen_setup(ftp->flen, &mask, &lobits, &pfrac);
  p->floatph = floatph;
  if(!floatph)
    phs = p->phase;                             /* grain phase offset   */
  else phsf = p->phasef;

  p->f_rnd_pow = *(p->kfrpow);        /* random distribution (frequency) */
  if ((p->f_rnd_pow == FL(0.0)) || (p->f_rnd_pow == FL(-1.0)) ||
      (p->f_rnd_pow == FL(1.0))) {
    p->f_rnd_pow = FL(1.0); p->f_rnd_mode = 0;
  }
  else if (p->f_rnd_pow < FL(0.0)) {
    p->f_rnd_pow = -(p->f_rnd_pow); p->f_rnd_mode = 2;
  }
  else {
    p->f_rnd_mode = 1;
  }

  p->p_rnd_pow = *(p->kprpow);        /* random distribution (phase)  */
  if ((p->p_rnd_pow == FL(0.0)) || (p->p_rnd_pow == FL(-1.0)) ||
      (p->p_rnd_pow == FL(1.0))) {
    p->p_rnd_pow = FL(1.0); p->p_rnd_mode = 0;
  }
  else if (p->p_rnd_pow < FL(0.0)) {
    p->p_rnd_pow = -(p->p_rnd_pow); p->p_rnd_mode = 2;
  }
  else {
    p->p_rnd_mode = 1;
  }

  if (p->init_k) {                    /* initial phase        */
    f = *(p->kphs);
    if(!floatph) g_ph = OSCBNK_PHS2INT(f);
    else g_phf = f;
  }
  else {
    f = p->phs0;
    if(!floatph) g_ph = phs[nsmps];
    else g_phf = (double) phsf[nsmps];
  }
  p->phs0 = *(p->kphs);
  /* convert phase modulation to frequency modulation */
  f = (MYFLT) ((double) p->phs0 - (double) f) / (nsmps-offset);
  f -= (MYFLT) ((int32) f);
  if(!floatph) g_frq = OSCBNK_PHS2INT(f);
  else g_frqf = f;
  f = *(p->kcps) * CS_ONEDSR;            /* grain frequency      */
  if(!floatph) frq = (g_frq + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
  else frqf = PHMOD1(g_frqf + f);
  if (p->mode & 0x40) {
    g_frq = frq;            /* phase sync   */
    g_frqf = frqf;
  }
  /* calculate phase offset values for this k-cycle */
  for (nn = offset; nn <= nsmps; nn++) {
    if(!floatph) {
      phs[nn] = g_ph; g_ph = (g_ph + g_frq) & OSCBNK_PHSMSK;
    } else {
      phsf[nn] = g_phf;
      g_phf = PHMOD1(g_phf + g_frqf);
    }
  }

  w_frq_f = CS_ONEDSR / *(p->kgdur);     /* window frequency     */
  if (UNLIKELY((w_frq_f < (FL(1.0) / (MYFLT) OSCBNK_PHSMAX)) ||
               (w_frq_f >= FL(1.0)))) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("grain3: invalid grain duration"));
  }
  w_frq = OSCBNK_PHS2INT(w_frq_f);
  x_frq_f = CS_ONEDSR * *(p->kdens);     /* density              */
  if (UNLIKELY((x_frq_f < (FL(1.0) / (MYFLT) OSCBNK_PHSMAX)) ||
               (x_frq_f >= FL(1.0)))) {
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("grain3: invalid grain density"));
  }
  x_frq = OSCBNK_PHS2INT(x_frq_f);
  if(floatph)
    wfdivxf = w_frq_f / x_frq_f;  
  else
    wfdivxf = w_frq_f / ((MYFLT) OSCBNK_PHSMAX * x_frq_f);
  p->grain_frq = frq;                 /* grain frequency      */
  p->grain_frqf = frqf;
  p->frq_scl = frq_scl = *(p->kfmd) * CS_ONEDSR;
  p->pm_wrap = (fabs((double) *(p->kpmd)) > 0.9 ? 1 : 0);


  /* initialise grains (if enabled) */
  if ((p->init_k) && (!(p->mode & 0x10))) {
    f = w_frq_f / x_frq_f;
    if(!floatph) {
      g_frq = (f > FL(0.99999) ? OSCBNK_PHSMAX : OSCBNK_PHS2INT(f));
      /* initial window phase */
      g_ph = OSCBNK_PHSMAX % g_frq;
      if (g_ph < (OSCBNK_PHSMAX >> 16)) g_ph += g_frq;
      g_ph = OSCBNK_PHSMAX - g_ph;
      while (g_ph) {
        grain3_init_grain(p, p->osc_end, g_ph, *phs);
        if (!(p->mode & 0x40))  /* init. grain phase    */
          grain2_init_grain_phase(p->osc_end, frq, w_frq,
                                  frq_scl, f_nolock);
        if (++(p->osc_end) > p->osc_max) p->osc_end = p->osc;
        if (UNLIKELY(p->osc_end == p->osc_start)) goto err2;
        g_ph -= g_frq;
      }
    }
    else {
      g_frqf = (f > FL(0.99999) ? 1. : f);
      g_phf = fmod(1., g_frqf);
      if(g_phf < 0.5) g_phf += g_frqf; /* VL ??? */
      g_phf = 1. - g_phf;
      while (g_phf != 0.) {
        grain3_init_grain_f(p, p->osc_end, g_phf, *phsf);
        if (!(p->mode & 0x40))  /* init. grain phase    */
          grain2_init_grain_phase_f(p->osc_end, frqf, w_frq_f,
                                    frq_scl, f_nolock);
        if (++(p->osc_end) > p->osc_max) p->osc_end = p->osc;
        if (UNLIKELY(p->osc_end == p->osc_start)) goto err2;
        g_phf -= g_frqf;
      }
    }
  }
  p->init_k = 0;

  nn = nsmps; o = p->osc_start;
  while (nn>offset) {
    if(!floatph) {    
      if (x_ph >= OSCBNK_PHSMAX) {      /* check for new grain  */
        x_ph &= OSCBNK_PHSMSK;
        if (!(p->mode & 0x20)) {
          f = (MYFLT) x_ph * wfdivxf;
          w_ph = OSCBNK_PHS2INT(f);
        }
        else {
          w_ph = 0UL;
        }
        grain3_init_grain(p, p->osc_end, w_ph, *phs);
        if (++(p->osc_end) > p->osc_max) p->osc_end = p->osc;
        if (UNLIKELY(p->osc_end == p->osc_start)) goto err2;
      }
    } else {
      if (x_phf >= FL(1.0)) {      /* check for new grain  */
        x_phf = PHMOD1(x_phf);
        if (!(p->mode & 0x20)) {
          f = (MYFLT) x_phf * wfdivxf;
          w_phf = f;
        }
        else {
          w_phf = FL(0.0);
        }
        grain3_init_grain_f(p, p->osc_end, w_phf, *phs);
        if (++(p->osc_end) > p->osc_max) p->osc_end = p->osc;
        if (UNLIKELY(p->osc_end == p->osc_start)) goto err2;
      }

      if (o == p->osc_end) {            /* no active grains     */
        x_ph += x_frq; nn--; aout0++; phs++; continue;
        x_phf += x_frq_f;
      }

      if(!floatph) {
        g_ph = o->grain_phs;              /* grain phase          */
        if (f_nolock) {
          /* grain frequency */
          f = o->grain_frq_flt * frq_scl;
          g_frq = OSCBNK_PHS2INT(f);
          g_frq = (g_frq + frq) & OSCBNK_PHSMSK;
        }
        else {                    /* lock frequency       */
          g_frq = o->grain_frq_int;
        }
        w_ph = o->window_phs;             /* window phase         */
      } else {
        g_phf = o->grain_phsf;              /* grain phase          */
        if (f_nolock) {
          /* grain frequency */
          f = o->grain_frq_flt * frq_scl;
          g_frqf = f;
          g_frq = PHMOD1(g_frqf + frqf);
        }
        else {                    /* lock frequency       */
          g_frqf = o->grain_frq;
        }
        w_phf = o->window_phsf;             /* window phase         */
      }
      /* render grain */
      aout = aout0; i = nn;
      while (i--) {
        if(!floatph) {
          /* window waveform */
          n = w_ph >> w_lobits; a = w_ft[n++];
          if (w_interp) a += (w_ft[n] - a) * w_pfrac
                          * (MYFLT) ((int32) (w_ph & w_mask));
          /* grain waveform */
          n = g_ph >> lobits; k = ft[n++];
          if (g_interp) k += (ft[n] - k) * pfrac
                          * (MYFLT) ((int32) (g_ph & mask));
          /* update phase */
          g_ph = (g_ph + g_frq) & OSCBNK_PHSMSK;
          /* check for end of grain */
          if ((w_ph += w_frq) >= OSCBNK_PHSMAX) {
            if (++(p->osc_start) > p->osc_max)
              p->osc_start = p->osc;
            break;
          }
        }
        else {
          /* window waveform */
          MYFLT pos = w_phf*wflen;
          n = (int32_t) pos;
          a = w_ft[n];
          if (w_interp) a += (pos - n)*(w_ft[n+1] - k);
          /* grain waveform */
          pos = g_phf*flen;
          n = (int32_t) pos;
          k = ft[n];
          if (g_interp) k += (pos - n)*(ft[n+1] - k);
          g_phf = PHMOD1(g_phf+ g_frqf);
          /* check for end of grain */
          if ((w_phf += w_frq_f) >= FL(1.0)) {
            if (++(p->osc_start) > p->osc_max)
              p->osc_start = p->osc;
            break;
          }
        }
        /* mix to output */
        *(aout++) += a * k;
      }
    }
    /* save phase */
    o->grain_phs = g_ph; o->window_phs = w_ph;
    o->grain_phsf = g_phf; o->window_phsf = w_phf;
    /* next grain */
    if (++o > p->osc_max) o = p->osc;
  }
  p->x_phs = x_ph;
  p->x_phsf = x_phf;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("grain3: not initialised"));
 err2:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("grain3 needs more overlaps"));
}

/* ----------------------------- rnd31 opcode ------------------------------ */

static int32_t rnd31set(CSOUND *csound, RND31 *p)
{
  /* initialise random seed */
  oscbnk_seedrand(csound, &(p->seed), *(p->iseed));
  return OK;
}

/* ---- rnd31 / i-rate ---- */

static int32_t rnd31i(CSOUND *csound, RND31 *p)
{
  MYFLT rpow;
  int32_t   rmode;

  /* random distribution */
  rpow = *(p->rpow);
  if ((rpow == FL(0.0)) || (rpow == FL(-1.0)) || (rpow == FL(1.0))) {
    rpow = FL(1.0); rmode = 0;
  }
  else if (rpow < FL(0.0)) {
    rpow = -(rpow); rmode = 2;
  }
  else {
    rmode = 1;
  }

  /* initialise seed */
  if (p->rnd31i_seed == NULL) {
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    p->rnd31i_seed = &(pp->rnd31i_seed);
  }
  if (*(p->iseed) < FL(0.5)) {    /* seed from current time       */
    if (*(p->rnd31i_seed) <= 0L)  /* check if already initialised */
      oscbnk_seedrand(csound, p->rnd31i_seed, FL(0.0));
  }
  else {                          /* explicit seed value          */
    oscbnk_seedrand(csound, p->rnd31i_seed, *(p->iseed));
  }

  *(p->out) = *(p->scl) * oscbnk_rnd_bipolar(p->rnd31i_seed, rpow, rmode);
  return OK;
}

/* ---- rnd31 / k-rate ---- */

static int32_t rnd31k(CSOUND *csound, RND31 *p)
{
  MYFLT rpow;
  int32_t   rmode;

  if (UNLIKELY(!p->seed)) goto err1;

  /* random distribution */
  rpow = *(p->rpow);
  if ((rpow == FL(0.0)) || (rpow == FL(-1.0)) || (rpow == FL(1.0))) {
    rpow = FL(1.0); rmode = 0;
  }
  else if (rpow < FL(0.0)) {
    rpow = -(rpow); rmode = 2;
  }
  else {
    rmode = 1;
  }

  *(p->out) = *(p->scl) * oscbnk_rnd_bipolar(&(p->seed), rpow, rmode);
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("rnd31: not initialised"));
}

/* ---- rnd31 / a-rate ---- */

static int32_t rnd31a(CSOUND *csound, RND31 *p)
{
  MYFLT   scl, *out, rpow;
  int32_t     rmode;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;

  if (UNLIKELY(!p->seed)) goto err1;

  scl = *(p->scl); out = p->out;
  /* random distribution */
  rpow = *(p->rpow);
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  if ((rpow == FL(0.0)) || (rpow == FL(-1.0)) || (rpow == FL(1.0))) {
    /* IV - Jan 30 2003: optimised code for uniform distribution */
    scl *= (MYFLT) (1.0 / 1073741823.015625);
    for (nn=offset; nn<nsmps; nn++) {
      p->seed = oscbnk_rand31(p->seed);
      out[nn] = scl * (MYFLT) (p->seed - 0x3FFFFFFFL);
    }
    return OK;
  }
  else if (rpow < FL(0.0)) {
    rpow = -(rpow); rmode = 2;
  }
  else {
    rmode = 1;
  }

  for (nn=offset; nn<nsmps; nn++) {
    out[nn] = scl * oscbnk_rnd_bipolar(&(p->seed), rpow, rmode);
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("rnd31: not initialised"));
}

/* ---- oscilikt initialisation ---- */

static int32_t oscktset(CSOUND *csound, OSCKT *p)
{
  IGN(csound);
  MYFLT   phs;

  if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
  /* initialise table parameters */
  p->oldfn = FL(-1.0);
  p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
  /* initial phase */
  phs = *(p->iphs) - (MYFLT) ((int32) *(p->iphs));
  p->phs = OSCBNK_PHS2INT(phs);
  p->phsf = PHMOD1(phs);
  p->floatph = 0; // reset flag
  return OK;
}

/* ---- oscilikt performance ---- */

static int32_t kosclikt(CSOUND *csound, OSCKT *p)
{
  FUNC    *ftp;
  uint32   n, phs;
  MYFLT   v, *ft;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn); /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
    p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));        
  }

  /* copy object data to local variables */
  ft = p->ft;
  if(!p->floatph) {
  phs = p->phs;
  /* read from table with interpolation */
  n = phs >> p->lobits; v = (MYFLT) ((int32) (phs & p->mask)) * p->pfrac;
  *(p->sr) = (ft[n] + (ft[n + 1] - ft[n]) * v) * *(p->xamp);
  /* update phase */
  v = *(p->xcps) * CS_ONEDKR;
  p->phs = (phs + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
  } else {
    MYFLT pos = p->phsf * p->flen;
    n = (int32_t) pos;
    *(p->sr) = *p->xamp*(ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    p->phsf = PHMOD1(p->phsf + (*p->xcps * CS_ONEDKR));
  }
  return OK;
}

static int32_t osckkikt(CSOUND *csound, OSCKT *p)
{
  FUNC    *ftp;
  uint32   n, phs, lobits, mask, frq;
  MYFLT   pfrac, *ft, v, a, *ar, phsf, xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t floatph = p->floatph, flen;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn); /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;
  /* copy object data to local variables */
  ft = p->ft; a = *(p->xamp); ar = p->sr;
  /* read from table with interpolation */
  if(floatph) {
   phsf = p->phsf;
   xcps = *(p->xcps) * CS_ONEDSR;
  }
  else {
  v = *(p->xcps) * CS_ONEDSR;  
  frq = OSCBNK_PHS2INT(v);  
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  phs = p->phs;
  }
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (nn=offset; nn<nsmps; nn++) {
    if(!floatph) {
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    phs = (phs + frq) & OSCBNK_PHSMSK;
    } else {
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + xcps);
    }
    ar[nn] = v * a;
  }
  /* save new phase */
  p->phsf = phsf;
  p->phs = phs;
  return OK;
}

static int32_t osckaikt(CSOUND *csound, OSCKT *p)
{
  FUNC    *ftp;
  uint32   n, phs, lobits, mask;
  MYFLT   pfrac, *ft, v, a, *ar, *xcps, phsf;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps=CS_KSMPS;
  int32_t floatph = p->floatph, flen;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn);    /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;

  /* copy object data to local variables */
  ft = p->ft; a = *(p->xamp); ar = p->sr; xcps = p->xcps;
  if(floatph)
   phsf = p->phsf;
  else {
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  phs = p->phs;
  }
  
  /* read from table with interpolation */
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (nn=offset; nn<nsmps; nn++) {
    MYFLT fcps = *(xcps++) * CS_ONEDSR;
    if(!floatph) {
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    phs = (phs + OSCBNK_PHS2INT(fcps)) & OSCBNK_PHSMSK;
    }
    else {
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + fcps);
    }
    ar[nn] = v * a;
  }
  /* save new phase */
  p->phsf = phsf;
  p->phs = phs;
  return OK;
}

static void oscbnk_flen_setup(int32 flen, uint32 *mask,
                              uint32 *lobits, MYFLT *pfrac)
{
  uint32       n;

  n = (uint32) flen;
  *lobits = 0UL; *mask = 1UL; *pfrac = FL(0.0);
  if (n < 2UL) return;
  while (n < OSCBNK_PHSMAX) {
    n <<= 1; *mask <<= 1; (*lobits)++;
  }
  *pfrac = FL(1.0) / (MYFLT) *mask; (*mask)--;
}

static int32_t oscakikt(CSOUND *csound, OSCKT *p)
{
  FUNC    *ftp;
  uint32   n, phs, lobits, mask, frq;
  MYFLT   pfrac, *ft, v, *ar, *xamp, phsf, xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t floatph = p->floatph, flen;

   /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn);    /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;

  /* copy object data to local variables */
  ft = p->ft; phs = p->phs; xamp = p->xamp; ar = p->sr;
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;

    /* copy object data to local variables */
  ft = p->ft; xamp = p->xamp; ar = p->sr; 
  if(floatph) {
   xcps = *(p->xcps) * CS_ONEDSR;  
   phsf = p->phsf;
  }
  else {
  v = *(p->xcps) * CS_ONEDSR;  
  frq = OSCBNK_PHS2INT(v);  
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  phs = p->phs;
  }

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (nn=offset; nn<nsmps; nn++) {
    if(!floatph) {
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    phs = (phs + frq) & OSCBNK_PHSMSK;
    } else {
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + xcps);
    }
    ar[nn] = v * *(xamp++);
  }
  /* save new phase */
  p->phsf = phsf;
  p->phs = phs;
  return OK;
}

static int32_t oscaaikt(CSOUND *csound, OSCKT *p)
{
  FUNC    *ftp;
  uint32   n, phs, lobits, mask;
  MYFLT   pfrac, *ft, v, *ar, *xcps, phsf, *xamp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps=CS_KSMPS;
  int32_t floatph = p->floatph, flen;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn);    /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;

  /* copy object data to local variables */
  ft = p->ft; xamp = p->xamp; ar = p->sr; xcps = p->xcps;
  if(floatph)
   phsf = p->phsf;
  else {
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  phs = p->phs;
  }
  
  /* read from table with interpolation */
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (nn=offset; nn<nsmps; nn++) {
    MYFLT fcps = *(xcps++) * CS_ONEDSR;
    if(!floatph) {
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    phs = (phs + OSCBNK_PHS2INT(fcps)) & OSCBNK_PHSMSK;
    }
    else {
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + fcps);
    }
    ar[nn] = v * xamp[nn];
  }
  /* save new phase */
  p->phsf = phsf;
  p->phs = phs;
  return OK;
}

/* ---- osciliktp initialisation ---- */

static int32_t oscktpset(CSOUND *csound, OSCKTP *p)
{
  IGN(csound);
  if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
  /* initialise table parameters */
  p->oldfn = FL(-1.0);
  p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
  /* initial phase */
  p->phs = 0UL; p->old_phs = FL(0.0);
  p->phsf = 0.;
  p->floatph = 0;
  p->init_k = 1;
  return OK;
}

/* ---- osciliktp performance ---- */

static int32_t oscktp(CSOUND *csound, OSCKTP *p)
{
  FUNC    *ftp;
  uint32_t   n, phs, lobits, mask, frq;
  MYFLT   pfrac, *ft, v, *ar, frqf, phsf;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
 int32_t floatph = p->floatph, flen;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn);    /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;
  
  /* copy object data to local variables */
  ft = p->ft; ar = p->sr;
  /* read from table with interpolation */
  if(floatph) {
   phsf = p->phsf;
   frqf = *(p->xcps) * CS_ONEDSR;
  }
  else {
  v = *(p->xcps) * CS_ONEDSR;  
  frq = OSCBNK_PHS2INT(v);  
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  phs = p->phs;
  }
  
  /* initialise phase if 1st k-cycle */
  if (p->init_k) {
    p->init_k = 0;
    p->old_phs = *(p->kphs);
    v = *(p->kphs) - (MYFLT) ((int32) *(p->kphs));
    if(floatph) phsf = v;
    else phs = OSCBNK_PHS2INT(v);
  }
  
  /* convert phase modulation to frequency modulation */
  /* VL moved the line from below to here */
  v = (MYFLT) ((double) *(p->kphs) - (double) p->old_phs) / (nsmps-offset);
  p->old_phs = *(p->kphs);
  if(floatph) frqf = PHMOD1(frqf + v);
  else frq = (frq + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
  
  /* read from table with interpolation */
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (nn=offset; nn<nsmps; nn++) {
    if(!floatph) {
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    phs = (phs + frq) & OSCBNK_PHSMSK;
    } else {
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + frqf);
    }
    ar[nn] = v;
  }
  /* save new phase */
  p->phsf = phsf;
  p->phs = phs;
  return OK;
}

/* ---- oscilikts initialisation ---- */

static int32_t oscktsset(CSOUND *csound, OSCKTS *p)
{
  IGN(csound);
  if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
  /* initialise table parameters */
  p->oldfn = FL(-1.0);
  p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
  /* initial phase */
  p->phs = 0UL;
  p->init_k = 1;
  p->phsf = 0.;
  p->floatph = 0;
  return OK;
}

/* ---- oscilikts performance ---- */

static int32_t osckts(CSOUND *csound, OSCKTS *p)
{
  FUNC    *ftp;
  uint32_t   n, phs, lobits, mask, frq = 0UL;
  MYFLT   pfrac, *ft, v, *ar, *xcps, *xamp, *async, cpsf, phsf;
  int32_t     a_amp, a_cps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t floatph = p->floatph, flen;

  /* check if table number was changed */
  if (*(p->kfn) != p->oldfn || p->ft == NULL) {
    p->oldfn = *(p->kfn);
    ftp = csound->FTFind(csound, p->kfn);    /* new table parameters */
    if (UNLIKELY((ftp == NULL) || ((p->ft = ftp->ftable) == NULL))) return NOTOK;
      p->flen = ftp->flen;
    p->floatph |= !(IS_POW_TWO(p->flen)); // once a np2 table is used, floatph is set.
    if(!p->floatph)
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    floatph = p->floatph;
  }
  flen = p->flen;

  /* copy object data to local variables */
  ft = p->ft;
  a_amp = (IS_ASIG_ARG(p->xamp) ? 1 : 0); a_cps = (IS_ASIG_ARG(p->xcps) ? 1 : 0);
  ar = p->ar; xcps = p->xcps; xamp = p->xamp; async = p->async;
  
  if(!floatph) {
  phs = p->phs;  
  lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
  if (!a_cps) {
    v = *xcps * CS_ONEDSR;
    frq = OSCBNK_PHS2INT(v);
  }
  } else {
    phsf = p->phsf;
  if (!a_cps) {
    cpsf = *xcps * CS_ONEDSR;
   }
  }
  /* initialise phase if 1st k-cycle */
  if (p->init_k) {
    p->init_k = 0;
    cpsf = *(p->kphs) - (MYFLT) ((int32) *(p->kphs));
    if(!floatph) phs = OSCBNK_PHS2INT(cpsf);
    else phsf = v;
  }
  /* read from table with interpolation */
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (nn=offset; nn<nsmps; nn++) {
    if (a_cps) 
      cpsf = xcps[nn] * CS_ONEDSR;
    
    if(!floatph) {
    if (async[nn] > FL(0.0)) {               /* re-initialise phase */
      phsf = *(p->kphs) - (MYFLT) ((int32) *(p->kphs));
      phs = OSCBNK_PHS2INT(phsf);
    }
    n = phs >> lobits;
    v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
    if (a_cps) 
      frq = OSCBNK_PHS2INT(cpsf);
     phs = (phs + frq) & OSCBNK_PHSMSK;
    } else {
    if (async[nn] > FL(0.0)) {               /* re-initialise phase */
      phsf = *(p->kphs) - (MYFLT) ((int32) *(p->kphs));
    }
    MYFLT pos = phsf * flen;
    n = (int32_t) pos;
    v = (ft[n] + (pos - n)*(ft[n+1] - ft[n]));
    phsf = PHMOD1(phsf + cpsf);
    } 
    ar[nn] = v * *xamp;
    if (a_amp) xamp++;
  }
  /* save new phase */
  p->phs = phs;
  p->phsf = phsf;
  return OK;
}

/* ---- vco2init, vco2ft, and vco2 opcodes by Istvan Varga, Sep 2002 ---- */

/* table arrays for vco2 opcode */
/*   0: sawtooth                */
/*   1: 4 * x * (1 - x)         */
/*   2: pulse (not normalised)  */
/*   3: square                  */
/*   4: triangle                */
/*   5 and above: user defined  */

#define VCO2_MAX_NPART  4096    /* maximum number of harmonic partials */

typedef struct {
  int32_t     waveform;           /* waveform number (< 0: user defined)       */
  int32_t     w_npart;            /* nr of partials in user specified waveform */
  double  npart_mul;          /* multiplier for number of partials         */
  int32_t     min_size, max_size; /* minimum and maximum table size            */
  MYFLT   *w_fftbuf;          /* FFT of user specified waveform            */
} VCO2_TABLE_PARAMS;

/* remove table array for the specified waveform */

static void vco2_delete_table_array(CSOUND *csound, int32_t w)
{
  STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
  int32_t               j;

  /* table array does not exist: nothing to do */
  if (pp->vco2_tables == (VCO2_TABLE_ARRAY**) NULL ||
      w >= pp->vco2_nr_table_arrays ||
      pp->vco2_tables[w] == (VCO2_TABLE_ARRAY*) NULL)
    return;
#ifdef VCO2FT_USE_TABLE
  /* free number of partials -> table list, */
  csound->Free(csound, pp->vco2_tables[w]->nparts_tabl);
#else
  /* free number of partials list, */
  csound->Free(csound, pp->vco2_tables[w]->nparts);
#endif
  /* table data (only if not shared as standard Csound ftables), */
  for (j = 0; j < pp->vco2_tables[w]->ntabl; j++) {
    if (pp->vco2_tables[w]->base_ftnum < 1)
      csound->Free(csound, pp->vco2_tables[w]->tables[j].ftable);
  }
  /* table list, */
  csound->Free(csound, pp->vco2_tables[w]->tables);
  /* and table array structure */
  csound->Free(csound, pp->vco2_tables[w]);
  pp->vco2_tables[w] = NULL;
}

/* generate a table using the waveform specified in tp */

static void vco2_calculate_table(CSOUND *csound,
                                 VCO2_TABLE *table, VCO2_TABLE_PARAMS *tp)
{
  MYFLT   scaleFac;
  MYFLT   *fftbuf;
  int32_t     i, minh;
    void *setup;

  if (UNLIKELY(table->ftable == NULL)) {
    csound->InitError(csound,
                      "%s", Str("function table is NULL, check that ibasfn is "
                          "available\n"));
    return;
  }

  /* allocate memory for FFT */
  fftbuf = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (table->size + 2));
  if (tp->waveform >= 0) {                        /* no DC offset for   */
    minh = 1; fftbuf[0] = fftbuf[1] = FL(0.0);    /* built-in waveforms */
  }
  else
    minh = 0;
  scaleFac = csound->GetInverseRealFFTScale(csound, (int32_t) table->size);
  scaleFac *= (FL(0.5) * (MYFLT) table->size);
  switch (tp->waveform) {
  case 0: scaleFac *= (FL(-2.0) / PI_F);          break;
  case 1: scaleFac *= (FL(-4.0) / (PI_F * PI_F)); break;
  case 3: scaleFac *= (FL(-4.0) / PI_F);          break;
  case 4: scaleFac *= (FL(8.0) / (PI_F * PI_F));  break;
  }
  /* calculate FFT of the requested waveform */
  for (i = minh; i <= (table->size >> 1); i++) {
    fftbuf[i << 1] = fftbuf[(i << 1) + 1] = FL(0.0);
    if (i > table->npart) continue;
    switch (tp->waveform) {
    case 0:                                   /* sawtooth */
      fftbuf[(i << 1) + 1] = scaleFac / (MYFLT) i;
      break;
    case 1:                                   /* 4 * x * (1 - x) */
      fftbuf[i << 1] = scaleFac / ((MYFLT) i * (MYFLT) i);
      break;
    case 2:                                   /* pulse */
      fftbuf[i << 1] = scaleFac;
      break;
    case 3:                                   /* square */
      fftbuf[(i << 1) + 1] = (i & 1 ? (scaleFac / (MYFLT) i) : FL(0.0));
      break;
    case 4:                                   /* triangle */
      fftbuf[(i << 1) + 1] = (i & 1 ? ((i & 2 ? scaleFac : (-scaleFac))
                                       / ((MYFLT) i * (MYFLT) i))
                              : FL(0.0));
      break;
    default:                                  /* user defined */
      if (i <= tp->w_npart) {
        fftbuf[i << 1] = scaleFac * tp->w_fftbuf[i << 1];
        fftbuf[(i << 1) + 1] = scaleFac * tp->w_fftbuf[(i << 1) + 1];
      }
    }
  }
  /* inverse FFT */
  fftbuf[1] = fftbuf[table->size];
  fftbuf[table->size] = fftbuf[(int32_t) table->size + 1] = FL(0.0);
  setup = csound->RealFFTSetup(csound,table->size,FFT_INV);
  csound->RealFFT(csound,setup,fftbuf);
  /* copy to table */
  for (i = 0; i < table->size; i++)
    table->ftable[i] = fftbuf[i];
  /* write guard point */
  table->ftable[table->size] = fftbuf[0];
  /* free memory used by temporary buffers */
  csound->Free(csound, fftbuf);
}

/* set default table parameters depending on waveform */

static void vco2_default_table_params(int32_t w, VCO2_TABLE_PARAMS *tp)
{
  tp->waveform = w;
  tp->w_npart = -1;
  tp->npart_mul = 1.05;
  tp->min_size = (w == 2 ? 256 : 128);
  tp->max_size = (w == 2 ? 16384 : 8192);
  tp->w_fftbuf = NULL;
}

/* return number of partials for next table */

static void vco2_next_npart(double *npart, VCO2_TABLE_PARAMS *tp)
{
  double  n;
  n = *npart * tp->npart_mul;
  if ((n - *npart) < 1.0)
    (*npart)++;
  else
    *npart = n;
}

/* return optimal table size for a given number of partials */

static int32_t vco2_table_size(int32_t npart, VCO2_TABLE_PARAMS *tp)
{
  int32_t     n;

  if (npart < 1)
    return 16;        /* empty table, size is always 16 */
  else if (npart == 1)
    n = 1;
  else if (npart <= 4)
    n = 2;
  else if (npart <= 16)
    n = 4;
  else if (npart <= 64)
    n = 8;
  else if (npart <= 256)
    n = 16;
  else if (npart <= 1024)
    n = 32;
  else
    n = 64;
  /* set table size according to min and max value */
  n *= tp->min_size;
  if (n > tp->max_size) n = tp->max_size;

  return n;
}

/* Generate table array for the specified waveform (< 0: user defined).  */
/* The tables can be accessed also as standard Csound ftables, starting  */
/* from table number "base_ftable" if it is greater than zero.           */
/* The return value is the first ftable number that is not allocated.    */

static int32_t vco2_tables_create(CSOUND *csound, int32_t waveform,
                                  int32_t base_ftable,
                                  VCO2_TABLE_PARAMS *tp)
{
  STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
  int32_t               i, npart, ntables;
  double            npart_f;
  VCO2_TABLE_ARRAY  *tables;
  VCO2_TABLE_PARAMS tp2;
 

  /* set default table parameters if not specified in tp */
  if (tp == NULL) {
    if (waveform < 0) return -1;
    vco2_default_table_params(waveform, &tp2);
    tp = &tp2;
  }
  waveform = (waveform < 0 ? 4 - waveform : waveform);
  if (waveform >= pp->vco2_nr_table_arrays) {
    /* extend space for table arrays */
    ntables = ((waveform >> 4) + 1) << 4;
    pp->vco2_tables = (VCO2_TABLE_ARRAY**)
      csound->ReAlloc(csound, pp->vco2_tables, sizeof(VCO2_TABLE_ARRAY*)
                      * ntables);
    for (i = pp->vco2_nr_table_arrays; i < ntables; i++)
      pp->vco2_tables[i] = NULL;
    pp->vco2_nr_table_arrays = ntables;
   }
  /* clear table array if already initialised */
  if (pp->vco2_tables[waveform] != NULL) {
    vco2_delete_table_array(csound, waveform);
    csound->Warning(csound,
                    Str("redefined table array for waveform %d\n"),
                    (waveform > 4 ? 4 - waveform : waveform));
  }
  /* calculate number of tables */
  i = tp->max_size >> 1;
  if (i > VCO2_MAX_NPART) i = VCO2_MAX_NPART; /* max number of partials */
  npart_f = 0.0; ntables = 0;
  do {
    ntables++;
    vco2_next_npart(&npart_f, tp);
  } while (npart_f <= (double) i);
  /* allocate memory for the table array ... */
  tables = pp->vco2_tables[waveform] =
    (VCO2_TABLE_ARRAY*) csound->Calloc(csound, sizeof(VCO2_TABLE_ARRAY));
  /* ... and all tables */
#ifdef VCO2FT_USE_TABLE
  tables->nparts_tabl =
    (VCO2_TABLE**) csound->Malloc(csound, sizeof(VCO2_TABLE*)
                                  * (VCO2_MAX_NPART + 1));
#else
  tables->nparts =
    (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (ntables * 3));
  for (i = 0; i < ntables; i++) {
    tables->nparts[i] = FL(-1.0);     /* padding for number of partials */
    tables->nparts[(ntables << 1) + i] = FL(1.0e24);  /* list */
  }
#endif
  tables->tables =
    (VCO2_TABLE*) csound->Calloc(csound, sizeof(VCO2_TABLE) * ntables);
  /* generate tables */
  tables->ntabl = ntables;            /* store number of tables */
  tables->base_ftnum = base_ftable;   /* and base ftable number */
  npart_f = 0.0; i = 0;
  do {
    /* store number of partials, */
    npart = tables->tables[i].npart = (int32_t) (npart_f + 0.5);
#ifndef VCO2FT_USE_TABLE
    tables->nparts[ntables + i] = (MYFLT) npart;
#endif
    /* table size, */
    tables->tables[i].size = vco2_table_size(npart, tp);
    /* and other parameters */
    oscbnk_flen_setup((int32) tables->tables[i].size,
                      &(tables->tables[i].mask),
                      &(tables->tables[i].lobits),
                      &(tables->tables[i].pfrac));
    /* if base ftable was specified, generate empty table ... */
    if (base_ftable > 0) {
      FUNC *ftp;
      MYFLT ftable = (MYFLT) base_ftable;
      csound->FTAlloc(csound, base_ftable, (int32_t) tables->tables[i].size);
      ftp = csound->FTFind(csound, &ftable);
      tables->tables[i].ftable = ftp->ftable;
      base_ftable++;                /* next table number */
    }
    else    /* ... else allocate memory (cannot be accessed as a       */
      tables->tables[i].ftable =      /* standard Csound ftable) */
        (MYFLT*) csound->Malloc(csound, sizeof(MYFLT)
                                * (tables->tables[i].size + 1));
    /* now calculate the table */
    vco2_calculate_table(csound, &(tables->tables[i]), tp);
    /* next table */
    vco2_next_npart(&npart_f, tp);
  } while (++i < ntables);
#ifdef VCO2FT_USE_TABLE
  /* build table for number of harmonic partials -> table lookup */
  i = npart = 0;
  do {
    tables->nparts_tabl[npart++] = &(tables->tables[i]);
    if (i < (ntables - 1) && npart >= tables->tables[i + 1].npart) i++;
  } while (npart <= VCO2_MAX_NPART);
#endif

  return base_ftable;
}

/* ---- vco2init opcode ---- */

static int32_t vco2init(CSOUND *csound, VCO2INIT *p)
{
  int32_t     waveforms, base_ftable, ftnum, i, w;
  VCO2_TABLE_PARAMS   tp;
  FUNC    *ftp;
  uint32_t j;
  void *setup;
  /* check waveform number */
  waveforms = (int32_t) MYFLT2LRND(*(p->iwaveforms));
  if (UNLIKELY(waveforms < -1000000 || waveforms > 31)) {
    return csound->InitError(csound,
                             Str("vco2init: invalid waveform number: %f"),
                             *(p->iwaveforms));
  }
  /* base ftable number (required by user defined waveforms except -1) */
  ftnum = base_ftable = (int32_t) MYFLT2LONG(*(p->iftnum));
  if (ftnum < 1) ftnum = base_ftable = -1;
  if (UNLIKELY((waveforms < -1 && ftnum < 1) || ftnum > 1000000)) {
    return csound->InitError(csound,  "%s", 
                             Str("vco2init: invalid base ftable number"));
  }
  *(p->ift) = (MYFLT) ftnum;
  if (!waveforms) return OK;     /* nothing to do */
  w = (waveforms < 0 ? waveforms : 0);
  do {
    /* set default table parameters, */
    vco2_default_table_params(w, &tp);
    /* and override with user specified values (if there are any) */
    if (*(p->ipmul) > FL(0.0)) {
      if (UNLIKELY(*(p->ipmul) < FL(1.00999) || *(p->ipmul) > FL(2.00001))) {
        return csound->InitError(csound, "%s", Str("vco2init: invalid "
                                             "partial number multiplier"));
      }
      tp.npart_mul = (double) *(p->ipmul);
    }
    if (*(p->iminsiz) > FL(0.0)) {
      i = (int32_t) MYFLT2LONG(*(p->iminsiz));
      if (UNLIKELY(i < 16 || i > 262144 || (i & (i - 1)))) {
        return csound->InitError(csound,
                                 "%s", Str("vco2init: invalid min table size"));
      }
      tp.min_size = i;
    }
    if (*(p->imaxsiz) > FL(0.0)) {
      i = (int32_t) MYFLT2LONG(*(p->imaxsiz));
      if (UNLIKELY(i < 16 || i > 16777216 || (i & (i - 1)) || i < tp.min_size)) {
        return csound->InitError(csound,
                                 "%s", Str("vco2init: invalid max table size"));
      }
      tp.max_size = i;
    }
    else {
      tp.max_size = tp.min_size << 6;           /* default max size */
      if (tp.max_size > 16384) tp.max_size = 16384;
      if (tp.max_size < tp.min_size) tp.max_size = tp.min_size;
    }

    if (w >= 0) {             /* built-in waveforms */
      if (waveforms & (1 << w)) {
        ftnum = vco2_tables_create(csound, w, ftnum, &tp);
        if (UNLIKELY(base_ftable > 0 && ftnum <= 0)) {
          return csound->InitError(csound, "%s", Str("ftgen error"));
        }
      }
    }
    else {                      /* user defined, requires source ftable */
      if (UNLIKELY((ftp = csound->FTFind(csound, p->isrcft)) == NULL ||
                   ftp->flen < 4)) {
        return csound->InitError(csound,
                                 "%s", Str("vco2init: invalid source ftable"));
      }
      if(!IS_POW_TWO(ftp->flen))
                return csound->InitError(csound,
                                 "%s", Str("vco2init FFT requires power-of-two size source table"));
         
      /* analyze source table, and store results in table params structure */
      i = ftp->flen;
      tp.w_npart = i >> 1;
      tp.w_fftbuf = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT) * (i + 2));
      for (j = 0; j < ftp->flen; j++)
        tp.w_fftbuf[j] = ftp->ftable[j] / (MYFLT) (ftp->flen >> 1);
      setup = csound->RealFFTSetup(csound, ftp->flen, FFT_FWD);
      csound->RealFFT(csound, setup, tp.w_fftbuf);
      tp.w_fftbuf[ftp->flen] = tp.w_fftbuf[1];
      tp.w_fftbuf[1] = tp.w_fftbuf[(int32_t) ftp->flen + 1] = FL(0.0);
      /* generate table array */
      ftnum = vco2_tables_create(csound,waveforms, ftnum, &tp);
      /* free memory used by FFT buffer */
      csound->Free(csound, tp.w_fftbuf);
      if (UNLIKELY(base_ftable > 0 && ftnum <= 0)) {
        return csound->InitError(csound, "%s", Str("ftgen error"));
      }
    }
    *(p->ift) = (MYFLT) ftnum;
    w++;
  } while (w > 0 && w < 5);
  return OK;
}

/* ---- vco2ft / vco2ift opcode (initialisation) ---- */

static int32_t vco2ftp(CSOUND *, VCO2FT *);

static int32_t vco2ftset(CSOUND *csound, VCO2FT *p)
{
  int32_t     w;

  if (p->vco2_nr_table_arrays == NULL || p->vco2_tables == NULL) {
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    p->vco2_nr_table_arrays = &(pp->vco2_nr_table_arrays);
    p->vco2_tables = &(pp->vco2_tables);
  }
  w = (int32_t) MYFLT2LRND(*(p->iwave));
  if (w > 4) w = 0x7FFFFFFF;
  if (w < 0) w = 4 - w;
  if (UNLIKELY(w >= *(p->vco2_nr_table_arrays) || (*(p->vco2_tables))[w] == NULL
               || (*(p->vco2_tables))[w]->base_ftnum < 1)) {
    return csound->InitError(csound, "%s", Str("vco2ft: table array "
                                         "not found for this waveform"));
  }
#ifdef VCO2FT_USE_TABLE
  p->nparts_tabl = (*(p->vco2_tables))[w]->nparts_tabl;
  p->tab0 = (*(p->vco2_tables))[w]->tables;
#else
  /* address of number of partials list (with offset for padding) */
  p->nparts = (*(p->vco2_tables))[w]->nparts
    + (*(p->vco2_tables))[w]->ntabl;
  p->npart_old = p->nparts + ((*(p->vco2_tables))[w]->ntabl >> 1);
#endif
    p->base_ftnum = (*(p->vco2_tables))[w]->base_ftnum;
    if (*(p->inyx) > FL(0.5))
      p->p_scl = FL(0.5) * CS_ESR;
    else if (*(p->inyx) < FL(0.001))
      p->p_scl = FL(0.001) * CS_ESR;
    else
      p->p_scl = *(p->inyx) * CS_ESR;
    p->p_min = p->p_scl / (MYFLT) VCO2_MAX_NPART;
    /* in case of vco2ift opcode, find table number now */
    if (!strcmp(p->h.optext->t.opcod, "vco2ift"))
      vco2ftp(csound, p);
    else                                /* else set perf routine to avoid */
      p->h.perf = (SUBR) vco2ftp;      /* "not initialised" error */
    return OK;
}

/* ---- vco2ft opcode (performance) ---- */

static int32_t vco2ftp(CSOUND *csound, VCO2FT *p)
{
  IGN(csound);
#ifdef VCO2FT_USE_TABLE
  MYFLT   npart;
  int32_t     n;
#else
  MYFLT   npart, *nparts;
  int32_t     nn;
#endif

  npart = (MYFLT)fabs(*(p->kcps)); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
  n = (int32_t) (p->nparts_tabl[(int32_t) (p->p_scl / npart)] - p->tab0);
  *(p->kft) = (MYFLT) (n + p->base_ftnum);
#else
  npart = p->p_scl / npart;
  nparts = p->npart_old;
  if (npart < *nparts) {
    do {
      nparts--; nn = 1;
      while (npart < *(nparts - nn)) {
        nparts = nparts - nn; nn <<= 1;
      }
    } while (nn > 1);
  }
  else if (npart >= *(nparts + 1)) {
    do {
      nparts++; nn = 1;
      while (npart >= *(nparts + nn + 1)) {
        nparts = nparts + nn; nn <<= 1;
      }
    } while (nn > 1);
  }
  p->npart_old = nparts;
  *(p->kft) = (MYFLT) ((int32_t) (nparts - p->nparts) + p->base_ftnum);
#endif
  return OK;
}

static int32_t vco2ft(CSOUND *csound, VCO2FT *p)
{
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("vco2ft: not initialised"));
}

/* ---- vco2 opcode (initialisation) ---- */

static int32_t vco2set(CSOUND *csound, VCO2 *p)
{
  int32_t     mode, tnum;
  int32_t     tnums[8] = { 0, 0, 1, 2, 1, 3, 4, 5 };
  int32_t     modes[8] = { 0, 1, 2, 0, 0, 0, 0, 0 };
  MYFLT   x;
  uint32_t min_args;

  if (p->vco2_nr_table_arrays == NULL || p->vco2_tables == NULL) {
    STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
    p->vco2_nr_table_arrays = &(pp->vco2_nr_table_arrays);
    p->vco2_tables = &(pp->vco2_tables);
  }
  /* check number of args */
  if (UNLIKELY(p->INOCOUNT > 6)) {
    return csound->InitError(csound, "%s", Str("vco2: too many input arguments"));
  }
  mode = (int32_t) MYFLT2LONG(*(p->imode)) & 0x1F;
  if (mode & 1) return OK;               /* skip initialisation */
  /* more checks */
  min_args = 2;
  if ((mode & 14) == 2 || (mode & 14) == 4) min_args = 4;
  if (mode & 16) min_args = 5;
  if (UNLIKELY(p->INOCOUNT < min_args)) {
    return csound->InitError(csound,
                             "%s", Str("vco2: insufficient required arguments"));
  }

  //FIXME

  //    if (UNLIKELY(p->XINCODE)) {
  //      return csound->InitError(csound, "%s", Str("vco2: invalid argument type"));
  //    }

  /* select table array and algorithm, according to waveform */
  tnum = tnums[(mode & 14) >> 1];
  p->mode = modes[(mode & 14) >> 1];
  /* initialise tables if not done yet */
  if (tnum >= *(p->vco2_nr_table_arrays) ||
      (*(p->vco2_tables))[tnum] == NULL) {
    if (LIKELY(tnum < 5))
      vco2_tables_create(csound, tnum, -1, NULL);
    else {
      return csound->InitError(csound, "%s", Str("vco2: table array not found for "
                                           "user defined waveform"));
    }
  }
#ifdef VCO2FT_USE_TABLE
  p->nparts_tabl = (*(p->vco2_tables))[tnum]->nparts_tabl;
#else
  /* address of number of partials list (with offset for padding) */
  p->nparts = (*(p->vco2_tables))[tnum]->nparts
    + (*(p->vco2_tables))[tnum]->ntabl;
  p->npart_old = p->nparts + ((*(p->vco2_tables))[tnum]->ntabl >> 1);
  p->tables = (*(p->vco2_tables))[tnum]->tables;
#endif
  /* set misc. parameters */
  p->init_k = 1;
  p->pm_enabled = (mode & 16 ? 1 : 0);
  if ((mode & 16) || (p->INOCOUNT < 5))
    p->phs = 0UL;
  else {
    x = *(p->kphs); x -= (MYFLT) ((int32) x);
    p->phs = OSCBNK_PHS2INT(x);
  }
  p->f_scl = CS_ONEDSR;
  x = (p->INOCOUNT < 6 ? FL(0.5) : *(p->inyx));
  if (x < FL(0.001)) x = FL(0.001);
  if (x > FL(0.5)) x = FL(0.5);
  p->p_min = x / (MYFLT) VCO2_MAX_NPART;
  p->p_scl = x;
  return OK;
}

/* ---- vco2 opcode (performance) ---- */

static int32_t vco2(CSOUND *csound, VCO2 *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn, nsmps = CS_KSMPS;
  int32_t      n;
  VCO2_TABLE      *tabl;
  uint32  phs, phs2, frq, frq2, lobits, mask;
#ifdef VCO2FT_USE_TABLE
  MYFLT   f, f1, npart, pfrac, v, *ftable, kamp, *ar;
  if (UNLIKELY(p->nparts_tabl == NULL)) {
#else
    MYFLT   f, f1, npart, *nparts, pfrac, v, *ftable, kamp, *ar;
    if (UNLIKELY(p->tables == NULL)) {
#endif
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vco2: not initialised"));
    }
    /* if 1st k-cycle, initialise now */
    if (p->init_k) {
      p->init_k = 0;
      if (p->pm_enabled) {
        f = p->kphs_old = *(p->kphs); f -= (MYFLT) ((int32) f);
        p->phs = OSCBNK_PHS2INT(f);
      }
      if (p->mode) {
        p->kphs2_old = -(*(p->kpw));
        f = p->kphs2_old; f -= (MYFLT) ((int32) f);
        p->phs2 = (p->phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
      }
    }
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* calculate frequency (including phase modulation) */
    f = *(p->kcps) * p->f_scl;
    frq = OSCBNK_PHS2INT(f);
    if (p->pm_enabled) {
      f1 = (MYFLT) ((double) *(p->kphs) - (double) p->kphs_old)
        / (nsmps-offset);
      p->kphs_old = *(p->kphs);
      frq = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      f += f1;
    }
    /* find best table for current frequency */
    npart = (MYFLT)fabs(f); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
    tabl = p->nparts_tabl[(int32_t) (p->p_scl / npart)];
#else
    npart = p->p_scl / npart;
    nparts = p->npart_old;
    if (npart < *nparts) {
      do {
        nparts--; nn = 1;
        while (npart < *(nparts - nn)) {
          nparts = nparts - nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    else if (npart >= *(nparts + 1)) {
      do {
        nparts++; nn = 1;
        while (npart >= *(nparts + nn + 1)) {
          nparts = nparts + nn; nn <<= 1;
        }
      } while (nn > 1);
    }
    p->npart_old = nparts;
    tabl = p->tables + (int32_t) (nparts - p->nparts);
#endif
    /* copy object data to local variables */
    kamp = *(p->kamp);
    phs = p->phs;
    lobits = tabl->lobits; mask = tabl->mask; pfrac = tabl->pfrac;
    ftable = tabl->ftable;

    if (!p->mode) {                     /* - mode 0: simple table playback - */
      for (nn=offset; nn<nsmps; nn++) {
        n = phs >> lobits;
        v = ftable[n++];
        v += (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
        phs = (phs + frq) & OSCBNK_PHSMSK;
        ar[nn] = v * kamp;
      }
    }
    else {
      v = -(*(p->kpw));                                 /* pulse width */
      f1 = (MYFLT) ((double) v - (double) p->kphs2_old) / (nsmps-offset);
      f = p->kphs2_old; f -= (MYFLT) ((int32) f); if (f < FL(0.0)) f++;
      p->kphs2_old = v;
      phs2 = p->phs2;
      frq2 = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      if (p->mode == 1) {               /* - mode 1: PWM - */
        /* DC correction offset */
        f = FL(1.0) - FL(2.0) * f;
        f1 *= FL(-2.0);
        for (nn=offset; nn<nsmps; nn++) {
          n = phs >> lobits;
          v = ftable[n++];
          ar[nn] = v + (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((int32) (phs2 & mask)) * pfrac;
          ar[nn] = (ar[nn] - v + f) * kamp;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        }
      }
      else {                            /* - mode 2: saw / triangle ramp - */
        for (nn=offset; nn<nsmps; nn++) {
          n = phs >> lobits;
          v = ftable[n++];
          ar[nn] = v + (ftable[n] - v) * (MYFLT) ((int32) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((int32) (phs2 & mask)) * pfrac;
          ar[nn] = (ar[nn] - v) * (FL(0.25) / (f - f * f)) * kamp;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        }
      }
      p->phs2 = phs2;
    }
    /* save oscillator phase */
    p->phs = phs;
    return OK;
  }

  /* ---- denorm opcode ---- */

#ifndef USE_DOUBLE
#define DENORM_RND  ((MYFLT) ((*seed = (*seed * 15625 + 1) & 0xFFFF) - 0x8000) \
                     * FL(1.0e-24))
#else
#define DENORM_RND  ((MYFLT) ((*seed = (*seed * 15625 + 1) & 0xFFFF) - 0x8000) \
                     * FL(1.0e-60))
#endif

  static int32_t denorms(CSOUND *csound, DENORMS *p)
  {
    MYFLT   r, *ar, **args = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t     n = p->INOCOUNT, *seed;

    seed = p->seedptr;
    if (seed == NULL) {
      STDOPCOD_GLOBALS  *pp = get_oscbnk_globals(csound);
      seed = p->seedptr = &(pp->denorm_seed);
    }
    if (UNLIKELY(early)) nsmps -= early;
    do {
      r = DENORM_RND;
      ar = *args++;
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      for (nn=offset; nn<nsmps; nn++) {
        ar[nn] += r;
      }
    } while (--n);
    return OK;
  }

  /* ---- delayk and vdel_k opcodes ---- */

  static int32_t delaykset(CSOUND *csound, DELAYK *p)
  {
    int32_t npts, mode = (int32_t) MYFLT2LONG(*p->imode) & 3;

    if (mode & 1) return OK;            /* skip initialisation */
    p->mode = mode;
    /* calculate delay time */
    npts = (int32_t) (*p->idel * CS_EKR + FL(1.5));
    if (UNLIKELY(npts < 1))
      return csound->InitError(csound, "%s", Str("delayk: invalid delay time "
                                           "(must be >= 0)"));
    p->readp = 0; p->npts = npts;
    /* allocate space for delay buffer */
    if (p->aux.auxp == NULL ||
        (uint32_t)(npts * sizeof(MYFLT)) > p->aux.size) {
      csound->AuxAlloc(csound, (int32) (npts * sizeof(MYFLT)), &p->aux);
    }
    p->init_k = npts - 1;
    return OK;
  }

  static int32_t delayk(CSOUND *csound, DELAYK *p)
  {
    MYFLT   *buf = (MYFLT*) p->aux.auxp;

    if (UNLIKELY(!buf))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("delayk: not initialised"));
    buf[p->readp++] = *(p->ksig);           /* write input signal to buffer */
    if (p->readp >= p->npts)
      p->readp = 0;                         /* wrap index */
    if (p->init_k) {
      *(p->ar) = (p->mode & 2 ? *(p->ksig) : FL(0.0));  /* initial delay */
      p->init_k--;
    }
    else
      *(p->ar) = buf[p->readp];             /* read output signal */
    return OK;
  }

  static int32_t vdelaykset(CSOUND *csound, VDELAYK *p)
  {
    int32_t npts, mode = (int32_t) MYFLT2LONG(*p->imode) & 3;

    if (mode & 1)
      return OK;                /* skip initialisation */
    p->mode = mode;
    /* calculate max. delay time */
    npts = (int32_t) (*p->imdel * CS_EKR + FL(1.5));
    if (UNLIKELY(npts < 1))
      return csound->InitError(csound, "%s", Str("vdel_k: invalid max delay time "
                                           "(must be >= 0)"));
    p->wrtp = 0; p->npts = npts;
    /* allocate space for delay buffer */
    if (p->aux.auxp == NULL ||
        (uint32_t)(npts * sizeof(MYFLT)) > p->aux.size) {
      csound->AuxAlloc(csound, (int32) (npts * sizeof(MYFLT)), &p->aux);
    }
    p->init_k = npts;           /* not -1 this time ! */
    return OK;
  }

  static int32_t vdelayk(CSOUND *csound, VDELAYK *p)
  {
    MYFLT   *buf = (MYFLT*) p->aux.auxp;
    int32_t     n, npts = p->npts;

    if (UNLIKELY(!buf))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vdel_k: not initialised"));
    buf[p->wrtp] = *(p->ksig);              /* write input signal to buffer */
    /* calculate delay time */
    n = (int32_t) MYFLT2LONG(*(p->kdel) * CS_EKR);
    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vdel_k: invalid delay time "
                                   "(must be >= 0)"));
    n = p->wrtp - n;
    if (++p->wrtp >= npts) p->wrtp = 0;         /* wrap index */
    if (p->init_k) {
      if (p->mode & 2) {
        if (npts == p->init_k)
          p->frstkval = *(p->ksig);             /* save first input value */
        *(p->ar) = (n < 0 ? p->frstkval : buf[n]);      /* initial delay */
      }
      else {
        *(p->ar) = (n < 0 ? FL(0.0) : buf[n]);
      }
      p->init_k--;
    }
    else {
      while (n < 0) n += npts;
      *(p->ar) = buf[n];                        /* read output signal */
    }
    return OK;
  }

  /* ------------ rbjeq opcode ------------ */

  /* original algorithm by Robert Bristow-Johnson */
  /* Csound orchestra version by Josep M Comajuncosas, Aug 1999 */
  /* ported to C (and optimised) by Istvan Varga, Dec 2002 */

  /* ar rbjeq asig, kfco, klvl, kQ, kS[, imode] */

  /* IV - Dec 28 2002: according to the original version by JMC, the formula */
  /*   alpha = sin(omega) * sinh(1 / (2 * Q))                                */
  /* should be used to calculate Q. However, according to my tests, it seems */
  /* to be wrong with low Q values, where this simplified code               */
  /*   alpha = sin(omega) / (2 * Q)                                          */
  /* was measured to be more accurate. It also makes the Q value for no      */
  /* resonance exactly sqrt(0.5) (as it would be expected), while the old    */
  /* version required a Q setting of about 0.7593 for no resonance.          */
  /* With Q >= 1, there is not much difference.                              */
  /* N.B.: the above apply to the lowpass and highpass filters only. For     */
  /* bandpass, band-reject, and peaking EQ, the modified formula is          */
  /*   alpha = tan(omega / (2 * Q))                                          */

  /* Defining this macro selects the revised version, while commenting it    */
  /* out enables the original.                                               */

  /* #undef IV_Q_CALC */
#define IV_Q_CALC 1

  static int32_t rbjeqset(CSOUND *csound, RBJEQ *p)
  {
    IGN(csound);
    int32_t mode = (int32_t) MYFLT2LONG(*p->imode) & 0xF;

    if (mode & 1)
      return OK;                /* skip initialisation */
    /* filter type */
    p->ftype = mode >> 1;
    /* reset filter */
    p->old_kcps = p->old_klvl = p->old_kQ = p->old_kS = FL(-1.12123e35);
    p->b0 = p->b1 = p->b2 = p->a1 = p->a2 = FL(0.0);
    p->xnm1 = p->xnm2 = p->ynm1 = p->ynm2 = FL(0.0);
    return OK;
  }

  static int32_t rbjeq(CSOUND *csound, RBJEQ *p)
  {
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     new_frq;
    MYFLT   b0, b1, b2, a1, a2, tmp;
    MYFLT   xnm1, xnm2, ynm1, ynm2;
    MYFLT   *ar, *asig;
    double  dva0;

    if (*(p->kcps) != p->old_kcps) {
      /* frequency changed */
      new_frq = 1;
      p->old_kcps = *(p->kcps);
      /* calculate variables that depend on freq., and are used by all modes */
      p->omega = (double) p->old_kcps * TWOPI / (double) CS_ESR;
      p->cs = cos(p->omega);
      p->sn = sqrt(1.0 - p->cs * p->cs);
      //printf("**** (%d) p->cs = %f\n", __LINE__, p->cs);
    }
    else
      new_frq = 0;
    /* copy object data to local variables */
    ar = p->ar; asig = p->asig;
    xnm1 = p->xnm1; xnm2 = p->xnm2; ynm1 = p->ynm1; ynm2 = p->ynm2;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->ftype) {
    case 0:                                     /* lowpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = p->sn * 0.5 / (double) p->old_kQ;       /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (0.5 * (dva0 - dva0 * p->cs));
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp + xnm1 + xnm1 + xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 1:                                     /* highpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = p->sn * 0.5 / (double) p->old_kQ;       /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (0.5 * (dva0 + dva0 * p->cs));
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp - xnm1 - xnm1 + xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 2:                                     /* bandpass filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) (dva0 * alpha);
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp - xnm2) - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 3:                                     /* band-reject (notch) filter */
      if (new_frq || *(p->kQ) != p->old_kQ) {
        double  alpha;
        p->old_kQ = *(p->kQ);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        dva0 = 1.0 / (1.0 + alpha);
        p->b2 = (MYFLT) dva0;
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * alpha);
      }
      b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b2 * (tmp + xnm2) - a1 * (ynm1 - xnm1) - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 4:                                     /* peaking EQ */
      if (new_frq || *(p->kQ) != p->old_kQ || *(p->klvl) != p->old_klvl) {
        double  sq, alpha, tmp1, tmp2;
        p->old_kQ = *(p->kQ);
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        //printf("*** (%d) p->old_klvl\n", __LINE__, p->old_klvl);
#ifdef IV_Q_CALC
        alpha = tan(p->omega * 0.5 / (double) p->old_kQ); /* IV - Dec 28 2002 */
#else
        alpha = p->sn * sinh(0.5 / (double) p->old_kQ);
#endif
        /* recalculate all coeffs */
        tmp1 = alpha / sq;
        dva0 = 1.0 / (1.0 + tmp1);
        tmp2 = alpha * sq * dva0;
        p->b0 = (MYFLT) (dva0 + tmp2);
        p->b2 = (MYFLT) (dva0 - tmp2);
        p->a1 = (MYFLT) (-2.0 * dva0 * p->cs);
        p->a2 = (MYFLT) (dva0 - dva0 * tmp1);
      }
      b0 = p->b0; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b2 * xnm2 - a1 * (ynm1 - xnm1) - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 5:                                     /* low shelf */
      if (new_frq || *(p->klvl) != p->old_klvl || *(p->kS) != p->old_kS) {
        double sq, beta, tmp1, tmp2, tmp3, tmp4;
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        p->old_kS = *(p->kS);
        beta = p->sn * sqrt(((double) p->old_klvl + 1.0) / p->old_kS
                            - (double) p->old_klvl + sq + sq - 1.0);
        /* recalculate all coeffs */
        tmp1 = sq + 1.0;
        tmp2 = sq - 1.0;
        tmp3 = tmp1 * p->cs;
        tmp4 = tmp2 * p->cs;
        dva0 = 1.0 / (tmp1 + tmp4 + beta);
        p->a1 = (MYFLT) (-2.0 * dva0 * (tmp2 + tmp3));
        p->a2 = (MYFLT) (dva0 * (tmp1 + tmp4 - beta));
        dva0 *= sq;
        p->b0 = (MYFLT) (dva0 * (tmp1 - tmp4 + beta));
        p->b1 = (MYFLT) ((dva0 + dva0) * (tmp2 - tmp3));
        p->b2 = (MYFLT) (dva0 * (tmp1 - tmp4 - beta));
      }
      b0 = p->b0; b1 = p->b1; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b1 * xnm1 + b2 * xnm2 - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    case 6:                                     /* high shelf */
      if (new_frq || *(p->klvl) != p->old_klvl || *(p->kS) != p->old_kS) {
        double sq, beta, tmp1, tmp2, tmp3, tmp4;
        sq = sqrt((double) (p->old_klvl = *(p->klvl)));
        p->old_kS = *(p->kS);
        beta = p->sn * sqrt(((double) p->old_klvl + 1.0) / p->old_kS
                            - (double) p->old_klvl + sq + sq - 1.0);
        /* recalculate all coeffs */
        tmp1 = sq + 1.0;
        tmp2 = sq - 1.0;
        tmp3 = tmp1 * p->cs;
        tmp4 = tmp2 * p->cs;
        dva0 = 1.0 / (tmp1 - tmp4 + beta);
        p->a1 = (MYFLT) ((dva0 + dva0) * (tmp2 - tmp3));
        p->a2 = (MYFLT) (dva0 * (tmp1 - tmp4 - beta));
        dva0 *= sq;
        p->b0 = (MYFLT) (dva0 * (tmp1 + tmp4 + beta));
        p->b1 = (MYFLT) (-2.0 * dva0 * (tmp2 + tmp3));
        p->b2 = (MYFLT) (dva0 * (tmp1 + tmp4 - beta));
      }
      b0 = p->b0; b1 = p->b1; b2 = p->b2; a1 = p->a1; a2 = p->a2;
      for (n=offset; n<nsmps; n++) {
        tmp = asig[n];
        ar[n] = b0 * tmp + b1 * xnm1 + b2 * xnm2 - a1 * ynm1 - a2 * ynm2;
        xnm2 = xnm1; xnm1 = tmp;
        ynm2 = ynm1; ynm1 = ar[n];
      }
      break;
    default:
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("rbjeq: invalid filter type"));
      break;
    }
    /* save filter state */
    p->xnm1 = xnm1; p->xnm2 = xnm2; p->ynm1 = ynm1; p->ynm2 = ynm2;
    return OK;
  }

  /* ------------------------------------------------------------------------- */

  static const OENTRY localops[] =
    {
   { "oscbnk",     sizeof(OSCBNK),     TR,  "a",  "kkkkiikkkkikkkkkkikooooooo",
     (SUBR) oscbnkset, (SUBR) oscbnk                },
   { "grain2",     sizeof(GRAIN2),     TR,      "a",    "kkkikiooo",
            (SUBR) grain2set, (SUBR) grain2                },
   { "grain3",     sizeof(GRAIN3),     TR,      "a",    "kkkkkkikikkoo",
            (SUBR) grain3set, (SUBR) grain3                },
    { "rnd31.i",    sizeof(RND31),  0,           "i",    "iio",
            (SUBR) rnd31i, (SUBR) NULL, (SUBR) NULL                     },
    { "rnd31.k",    sizeof(RND31),  0,          "k",    "kko",
            (SUBR) rnd31set, (SUBR) rnd31k, (SUBR) NULL                 },
   { "rnd31.a",    sizeof(RND31),  0,          "a",    "kko",
            (SUBR) rnd31set, (SUBR) rnd31a                 },
    { "oscilikt",   0xFFFE,   TR                                       },
   { "oscilikt.a", sizeof(OSCKT),   0,        "a",    "kkkoo",
      (SUBR) oscktset, (SUBR)osckkikt      },
    { "oscilikt.kk", sizeof(OSCKT),   0,        "k",    "kkkoo",
            (SUBR) oscktset, (SUBR) kosclikt, NULL          },
   { "oscilikt.ka", sizeof(OSCKT),   0,        "a",    "kakoo",
            (SUBR) oscktset, (SUBR) osckaikt               },
   { "oscilikt.ak", sizeof(OSCKT),   0,        "a",    "akkoo",
            (SUBR) oscktset, (SUBR) oscakikt               },
   { "oscilikt.aa", sizeof(OSCKT),   0,        "a",    "aakoo",
            (SUBR) oscktset, (SUBR) oscaaikt               },
   { "osciliktp",  sizeof(OSCKTP),     TR,      "a",    "kkko",
            (SUBR) oscktpset, (SUBR) oscktp                },
   { "oscilikts",  sizeof(OSCKTS),     TR,      "a",    "xxkako",
            (SUBR) oscktsset, (SUBR) osckts                },
    { "vco2init",   sizeof(VCO2INIT),   TW,       "i",    "ijjjjj",
            (SUBR) vco2init, (SUBR) NULL, (SUBR) NULL                   },
    { "vco2ift",    sizeof(VCO2FT),     TW,       "i",    "iov",
            (SUBR) vco2ftset, (SUBR) NULL, (SUBR) NULL                  },
    { "vco2ft",     sizeof(VCO2FT),     TW,      "k",    "kov",
            (SUBR) vco2ftset, (SUBR) vco2ft, (SUBR) NULL                },
//    { "vco2",       sizeof(VCO2),       TR,      "a",    "kkoM",
   { "vco2",       sizeof(VCO2),       TR,      "a",    "kkoOOo",
     (SUBR) vco2set, (SUBR) vco2                    },
    { "denorm",     sizeof(DENORMS),   WI,        "",     "y",
            (SUBR) NULL, (SUBR) denorms                    },
    { "delayk",     sizeof(DELAYK),    0,       "k",    "kio",
            (SUBR) delaykset, (SUBR) delayk, (SUBR) NULL                },
    { "vdel_k",     sizeof(VDELAYK),   0,       "k",    "kkio",
            (SUBR) vdelaykset, (SUBR) vdelayk, (SUBR) NULL              },
   { "rbjeq",      sizeof(RBJEQ),     0,       "a",    "akkkko",
            (SUBR) rbjeqset, (SUBR) rbjeq                  }
};

int32_t oscbnk_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
  }
