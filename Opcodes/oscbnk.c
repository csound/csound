/*  
    oscbnk.c:

    Copyright (C) 2002 Istvan Varga

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


/* ---- oscbnk, grain2, and grain3 - written by Istvan Varga, 2001 ---- */

#include "csdl.h"
#include <math.h>
#include <time.h>
#include "fft.h"        /* IV - Sep 17 2002 */
#include "oscbnk.h"

static unsigned long    oscbnk_seed = 0UL;      /* global seed value */
static long             rnd31i_seed = 0L;       /* seed for rnd31i   */

/* update random seed */

void oscbnk_rand31(long *seed)
{
    unsigned long       x, xl, xh;

    /* x = (16807 * x) % 0x7FFFFFFF */
    x = (unsigned long) *seed;
    xl = (x & 0xFFFFUL) * 16807UL; xh = (x >> 16) * 16807UL;
    x = xl + ((xh & 0x7FFFUL) << 16) + (xh >> 15);
    if (x >= 0x7FFFFFFFUL) x = (x + 1UL) & 0x7FFFFFFFUL;
    *seed = (long) x;
}

/* initialise random seed */

void oscbnk_seedrand(long *seed, MYFLT seedval)
{
    *seed = (long) (seedval + FL(0.5));
    if (*seed < 1L) {                   /* seed from current time */
      if (oscbnk_seed > 0UL) {
        oscbnk_seed += 11UL;
      }
      else {
        oscbnk_seed = (unsigned long) time (NULL);
      }
      oscbnk_seed = ((oscbnk_seed - 1UL) % 0x7FFFFFFEUL) + 1UL;
      *seed = (long) oscbnk_seed;
    }
    else {
      *seed = ((*seed - 1L) % 0x7FFFFFFEL) + 1L;
    }
    oscbnk_rand31 (seed); oscbnk_rand31 (seed);
}

/* return a random phase value between 0 and OSCBNK_PHSMAX */

unsigned long oscbnk_rnd_phase(long *seed)
{
    /* update random seed */

    oscbnk_rand31(seed);

    /* convert seed to phase */

    return ((unsigned long) *seed >> OSCBNK_RNDPHS);
}

/* return a random value between -1 and 1 */

MYFLT oscbnk_rnd_bipolar(long *seed, MYFLT rpow, int rmode)
{
    double      x;
    MYFLT       s;

    /* update random seed */

    oscbnk_rand31(seed);

    /* convert to floating point */

    x = ((double) *seed - (double) 0x3FFFFFFFL) / (double) 0x3FFFFFFFL;

    if (!(rmode)) return ((MYFLT) x);   /* uniform distribution */

    /* change distribution */

    s = (x < 0.0 ? FL(-1.0) : FL(1.0)); /* sign                 */
    x = fabs(x);                               /* absolute value       */
    if (rmode == 2) x = fabs(1.0 - x);
    x = pow(x, (double) rpow);
    if (rmode == 2) x = 1.0 - x;

    return ((MYFLT) x * s);
}

/* set ftable parameters (mask etc.) according to table length */

void oscbnk_flen_setup(long flen, unsigned long *mask,
                       unsigned long *lobits, MYFLT *pfrac);


/* Update random seed, and return next value from parameter table (if   */
/* enabled) or random value between 0 and 1. If output table is present */
/* store value in table.                                                */

static MYFLT oscbnk_rand(OSCBNK *p)
{
    MYFLT           y;

    /* update random seed */

    oscbnk_rand31(&(p->seed));

    /* convert to float */

    y = (MYFLT) (p->seed - 1L) / (MYFLT) 0x7FFFFFFDL;

    /* read from parameter table (if exists) */

    if ((p->tabl_cnt < p->tabl_len) && (p->tabl[p->tabl_cnt] >= FL(0.0)))
      y = p->tabl[p->tabl_cnt];
    switch (p->tabl_cnt % 5) {
    case 0:                                 /* wrap phase */
    case 1:
    case 3: y -= (MYFLT) ((long) y); break;
    default: if (y > FL(1.0)) y = FL(1.0);  /* limit frequency */
    }

    /* store in output table */

    if (p->tabl_cnt < p->outft_len) p->outft[p->tabl_cnt] = y;

    p->tabl_cnt++;
    return y;
}

/* Read from ft at phase with linear interpolation. flen is the table   */
/* length. Phase is limited to the range 0 - 1.                         */

static MYFLT oscbnk_interp_read_limit(MYFLT phase, MYFLT *ft, long flen)
{
    static MYFLT    x;
    static long     n;

    if (phase < FL(0.0)) return ft[0];
    else phase *= (MYFLT) flen;
    n = (long) phase; phase -= (MYFLT) n;
    if (n >= flen) return ft[flen];
    else x = ft[n]; x += phase * (ft[++n] - x);

    return x;
}

/* LFO / modulation */

static void     oscbnk_lfo(OSCBNK *p, OSCBNK_OSC *o)
{
    unsigned long   n;
    int     eqmode;
    MYFLT   f, l, q, k, kk, vk, vkk, vkdq, sq;
    static MYFLT    lfo1val = FL(0.0), lfo2val = FL(0.0);

    /* lfo1val = LFO1 output, lfo2val = LFO2 output */

    if (p->ilfomode & 0xF0) {                       /* LFO 1 */
      n = o->LFO1phs >> p->l1t_lobits; lfo1val = p->l1t[n++];
      lfo1val += (p->l1t[n] - lfo1val)
        * (MYFLT) ((long) (o->LFO1phs & p->l1t_mask)) * p->l1t_pfrac;
      /* update phase */
      f = o->LFO1frq * p->lf1_scl + p->lf1_ofs;
      o->LFO1phs = (o->LFO1phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    }

    if (p->ilfomode & 0x0F) {                       /* LFO 2 */
      n = o->LFO2phs >> p->l2t_lobits; lfo2val = p->l2t[n++];
      lfo2val += (p->l2t[n] - lfo2val)
        * (MYFLT) ((long) (o->LFO2phs & p->l2t_mask)) * p->l2t_pfrac;
      /* update phase */
      f = o->LFO2frq * p->lf2_scl + p->lf2_ofs;
      o->LFO2phs = (o->LFO2phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
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

    /* EQ code taken from biquad.c */

    sq = (MYFLT) sqrt(2.0 * (double) l);                   /* level     */
    /* frequency */
    k = (MYFLT) tan((double) ((eqmode == 2 ? (PI_F - f) : f) * FL(0.5)));
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
}

/* ---------------- oscbnk set-up ---------------- */

int    oscbnkset(OSCBNK *p)
{
    long    i;
    FUNC    *ftp;
    MYFLT   x;

    p->init_k = 1;
    p->nr_osc = (int) (*(p->args[5]) + FL(0.5));    /* number of oscs */
    if (p->nr_osc <= 0) p->nr_osc = -1;     /* no output */
    oscbnk_seedrand(&(p->seed), *(p->args[6]));        /* random seed    */
    p->ilfomode = (int) (*(p->args[11]) + FL(0.5)) & 0xFF;  /* LFO mode */
    p->eq_interp = 0;                                       /* EQ mode */
    if (*(p->args[18]) < FL(-0.5)) {
      p->ieqmode = -1; p->ilfomode &= 0xEE;   /* disable EQ */
    }
    else {
      p->ieqmode = (int) (*(p->args[18]) + FL(0.5));
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
      ftp = ftfind (p->args[20]);             /* LFO 1 */
      if ((ftp == NULL) || ((p->l1t = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->l1t_mask), &(p->l1t_lobits),
                         &(p->l1t_pfrac));
    }
    else {
      p->l1t = NULL;          /* LFO1 not used */
      p->l1t_lobits = p->l1t_mask = 0UL; p->l1t_pfrac = FL(0.0);
    }

    if (p->ilfomode & 0x0F) {
      ftp = ftfind (p->args[21]);             /* LFO 2 */
      if ((ftp == NULL) || ((p->l2t = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->l2t_mask), &(p->l2t_lobits),
                         &(p->l2t_pfrac));
    }
    else {
      p->l2t = NULL;          /* LFO2 not used */
      p->l2t_lobits = p->l2t_mask = 0UL; p->l2t_pfrac = FL(0.0);
    }

    if (p->ieqmode >= 0) {
      ftp = ftfind(p->args[22]);             /* EQ frequency */
      if ((ftp == NULL) || ((p->eqft = ftp->ftable) == NULL)) return NOTOK;
      p->eqft_len = ftp->flen;

      ftp = ftfind(p->args[23]);             /* EQ level */
      if ((ftp == NULL) || ((p->eqlt = ftp->ftable) == NULL)) return NOTOK;
      p->eqlt_len = ftp->flen;

      ftp = ftfind(p->args[24]);             /* EQ Q */
      if ((ftp == NULL) || ((p->eqqt = ftp->ftable) == NULL)) return NOTOK;
      p->eqqt_len = ftp->flen;
    }
    else {
      p->eqft = p->eqlt = p->eqqt = NULL;     /* EQ disabled */
      p->eqft_len = p->eqlt_len = p->eqqt_len = 0L;
    }

    if (*(p->args[25]) >= FL(1.0)) {        /* parameter table */
      ftp = ftfind(p->args[25]);
      if ((ftp == NULL) || ((p->tabl = ftp->ftable) == NULL)) return NOTOK;
      p->tabl_len = ftp->flen;
    }
    else {
      p->tabl = NULL; p->tabl_len = 0L;
    }
    p->tabl_cnt = 0L;   /* table ptr. */

    if (*(p->args[26]) >= FL(1.0)) {        /* output table */
      ftp = ftfind(p->args[26]);
      if ((ftp == NULL) || ((p->outft = ftp->ftable) == NULL)) return NOTOK;
      p->outft_len = ftp->flen;
    }
    else {
      p->outft = NULL; p->outft_len = 0L;
    }

    /* allocate space */

    if (p->nr_osc < 1) return OK;
    i = (long) p->nr_osc * (long) sizeof (OSCBNK_OSC);
    if ((p->auxdata.auxp == NULL) || (p->auxdata.size < i))
      auxalloc(i, &(p->auxdata));
    p->osc = (OSCBNK_OSC *) p->auxdata.auxp;

    i = 0; while (i++ < p->outft_len)       /* clear output ftable */
      p->outft[i] = FL(0.0);

    /* initialise oscillators */

    for (i = 0; i < p->nr_osc; i++) {
      /* oscillator phase */
      x = oscbnk_rand(p); p->osc[i].osc_phs = OSCBNK_PHS2INT(x);
      /* LFO1 phase */
      x = oscbnk_rand(p); p->osc[i].LFO1phs = OSCBNK_PHS2INT(x);
      /* LFO1 frequency */
      p->osc[i].LFO1frq = oscbnk_rand(p);
      /* LFO2 phase */
      x = oscbnk_rand(p); p->osc[i].LFO2phs = OSCBNK_PHS2INT(x);
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

int    oscbnk(OSCBNK *p)
{
    int     nn, osc_cnt, pm_enabled, am_enabled;
    FUNC    *ftp;
    MYFLT   *ft;
    unsigned long   n, lobits, mask, ph, f_i;
    MYFLT   onedksmps, pfrac, pm, a, f, a1, a2, b0, b1, b2;
    MYFLT   k, a_d = FL(0.0), a1_d, a2_d, b0_d, b1_d, b2_d;
    MYFLT   yn, xnm1, xnm2, ynm1, ynm2;
    OSCBNK_OSC      *o;

    /* clear output signal */

    for (nn = 0; nn < ksmps_; nn++) p->args[0][nn] = FL(0.0);

    if (p->nr_osc == -1) {
      return OK;         /* nothing to render */
    }
    else if ((p->seed == 0L) || (p->osc == NULL)) {
      return perferror(Str(X_1665,"oscbnk: not initialised"));
    }

    /* check oscillator ftable */

    ftp = ftfindp(p->args[19]);
    if ((ftp == NULL) || ((ft = ftp->ftable) == NULL))
      return NOTOK;
    oscbnk_flen_setup(ftp->flen, &(mask), &(lobits), &(pfrac));

    /* some constants */
    onedksmps = FL(1.0) / (MYFLT) ksmps_;
    pm_enabled = (p->ilfomode & 0x22 ? 1 : 0);
    am_enabled = (p->ilfomode & 0x44 ? 1 : 0);
    p->frq_scl = onedsr;                              /* osc. freq.   */
    p->lf1_scl = (*(p->args[8]) - *(p->args[7])) / ekr_;
    p->lf1_ofs = *(p->args[7]) / ekr_;                 /* LFO1 freq.   */
    p->lf2_scl = (*(p->args[10]) - *(p->args[9])) / ekr_;
    p->lf2_ofs = *(p->args[9]) / ekr_;                 /* LFO2 freq.   */
    if (p->ieqmode >= 0) {
      p->eqo_scl = (*(p->args[13]) - *(p->args[12])) * tpidsr;
      p->eqo_ofs = *(p->args[12]) * tpidsr;           /* EQ omega */
      p->eql_scl = *(p->args[15]) - (p->eql_ofs= *(p->args[14]));/* EQ level */
      p->eqq_scl = *(p->args[17]) - (p->eqq_ofs= *(p->args[16]));/* EQ Q     */
    }

    for (osc_cnt = 0, o = p->osc; osc_cnt < p->nr_osc; osc_cnt++, o++) {
      if (p->init_k) oscbnk_lfo(p, o);
      ph = o->osc_phs;                        /* phase        */
      pm = o->osc_phm;                        /* phase mod.   */
      if ((p->init_k) && (pm_enabled)) {
        f = pm - (MYFLT) ((long) pm);
        ph = (ph + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
      }
      a = o->osc_amp;                         /* amplitude    */
      f = o->osc_frq;                         /* frequency    */
      if (p->ieqmode < 0) {           /* EQ disabled */
        oscbnk_lfo(p, o);
        /* initialise ramps */
        f = ((o->osc_frq + f) * FL(0.5) + *(p->args[1])) * p->frq_scl;
        if (pm_enabled) {
          f += (MYFLT) ((double) o->osc_phm - (double) pm) * onedksmps;
          f -= (MYFLT) ((long) f);
        }
        f_i = OSCBNK_PHS2INT(f);
        if (am_enabled) a_d = (o->osc_amp - a) * onedksmps;
        /* oscillator */
        for (nn = 0; nn < ksmps_; nn++) {
          /* read from table */
          n = ph >> lobits; k = ft[n++];
          k += (ft[n] - k) * (MYFLT) ((long) (ph & mask)) * pfrac;
          /* amplitude modulation */
          if (am_enabled) k *= (a += a_d);
          /* mix to output */
          p->args[0][nn] += k;
          /* update phase */
          ph = (ph + f_i) & OSCBNK_PHSMSK;
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
          f += (MYFLT) ((double) o->osc_phm - (double) pm)
            * onedksmps;
          f -= (MYFLT) ((long) f);
        }
        f_i = OSCBNK_PHS2INT(f);
        if (am_enabled) a_d = (o->osc_amp - a) * onedksmps;
        if (p->eq_interp) {     /* EQ w/ interpolation */
          a1_d = (o->a1 - a1) * onedksmps;
          a2_d = (o->a2 - a2) * onedksmps;
          b0_d = (o->b0 - b0) * onedksmps;
          b1_d = (o->b1 - b1) * onedksmps;
          b2_d = (o->b2 - b2) * onedksmps;
          /* oscillator */
          for (nn = 0; nn < ksmps_; nn++) {
            /* update ramps */
            a1 += a1_d; a2 += a2_d;
            b0 += b0_d; b1 += b1_d; b2 += b2_d;
            /* read from table */
            n = ph >> lobits; k = ft[n++];
            k += (ft[n] - k) * (MYFLT) ((long) (ph & mask)) * pfrac;
            /* amplitude modulation */
            if (am_enabled) k *= (a += a_d);
            /* EQ */
            yn = b2 * xnm2; yn += b1 * (xnm2 = xnm1); yn += b0 * (xnm1 = k);
            yn -= a2 * ynm2; yn -= a1 * (ynm2 = ynm1); ynm1 = yn;
            /* mix to output */
            p->args[0][nn] += yn;
            /* update phase */
            ph = (ph + f_i) & OSCBNK_PHSMSK;
          }
          /* save EQ coeffs */
          o->a1 = a1; o->a2 = a2;
          o->b0 = b0; o->b1 = b1; o->b2 = b2;
        }
        else {                /* EQ w/o interpolation */
          /* oscillator */
          for (nn = 0; nn < ksmps_; nn++) {
            /* read from table */
            n = ph >> lobits; k = ft[n++];
            k += (ft[n] - k) * (MYFLT) ((long) (ph & mask)) * pfrac;
            /* amplitude modulation */
            if (am_enabled) k *= (a += a_d);
            /* EQ */
            yn = b2 * xnm2; yn += b1 * (xnm2 = xnm1); yn += b0 * (xnm1 = k);
            yn -= a2 * ynm2; yn -= a1 * (ynm2 = ynm1); ynm1 = yn;
            /* mix to output */
            p->args[0][nn] += yn;
            /* update phase */
            ph = (ph + f_i) & OSCBNK_PHSMSK;
          }
        }
        o->xnm1 = xnm1; o->xnm2 = xnm2; /* save EQ state */
        o->ynm1 = ynm1; o->ynm2 = ynm2;
      }
      /* save amplitude and phase */
      o->osc_amp = a;
      o->osc_phs = ph;
    }
    p->init_k = 0;
    return OK;
}

/* ---------------- grain2 set-up ---------------- */

int grain2set(GRAIN2 *p)
{
    int i;
    FUNC        *ftp;
    long        n;
    double      x, y;

    /* check opcode params */

    i = (int) (*(p->imode) + FL(0.5));  /* mode */
    if (i & 1) return OK;                  /* skip initialisation */
    p->init_k = 1;
    p->mode = i & 0x0E;
    p->nr_osc = (int) (*(p->iovrlp) + FL(0.5)); /* nr of oscillators */
    if (p->nr_osc < 1) p->nr_osc = -1;
    oscbnk_seedrand(&(p->seed), *(p->iseed));  /* initialise seed */
    p->rnd_pow = *(p->irpow);           /* random distribution */
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
    ftp = ftfind(p->iwfn);                     /* window table */
    if ((ftp == NULL) || ((p->wft = ftp->ftable) == NULL)) return NOTOK;
    oscbnk_flen_setup(ftp->flen, &(p->wft_mask), &(p->wft_lobits),
                       &(p->wft_pfrac));

    /* allocate space */

    if (p->nr_osc == -1) return OK;                /* no oscillators */
    n = (long) p->nr_osc * (long) sizeof(GRAIN2_OSC);
    if ((p->auxdata.auxp == NULL) || (p->auxdata.size < n))
      auxalloc(n, &(p->auxdata));
    p->osc = (GRAIN2_OSC *) p->auxdata.auxp;

    /* initialise oscillators */

    y = (double) OSCBNK_PHSMAX / (double) p->nr_osc;
    x = (double) OSCBNK_PHSMAX + 0.5;
    for (i = 0; i < p->nr_osc; i++) {
      if ((x -= y) < 0.0) x = 0.0;
      p->osc[i].window_phs = (unsigned long) x;
    }
    return OK;
}

/* ---------------- grain2 performance ---------------- */

/* set initial phase of grains with start time less than zero */

void grain2_init_grain_phase(GRAIN2_OSC *o, unsigned long frq,
                              unsigned long w_frq, MYFLT frq_scl,
                              int f_nolock)
{
    double      d;
    MYFLT       f;

    if (!(w_frq)) return;
    if (f_nolock) {
      d = (double) o->grain_frq_flt * (double) frq_scl
        * (double) OSCBNK_PHSMAX + (double) frq;
    }
    else {
      d = (double) o->grain_frq_int;
    }
    d *= (double) o->window_phs / ((double) w_frq * (double) OSCBNK_PHSMAX);
    d -= (double) ((long) d);
    f = (MYFLT) d;
    o->grain_phs = (o->grain_phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
}

/* initialise grain */

void grain2_init_grain(GRAIN2 *p, GRAIN2_OSC *o)
{
    MYFLT       f;

    /* random phase */

    o->grain_phs = oscbnk_rnd_phase(&(p->seed));

    /* random frequency */

    f = oscbnk_rnd_bipolar(&(p->seed), p->rnd_pow, p->rnd_mode);
    if (p->mode & 2) {
      o->grain_frq_flt = f;
    }
    else {                              /* lock frequency */
      f = p->grain_frq + p->frq_scl * f;
      o->grain_frq_int = OSCBNK_PHS2INT(f);
    }
}

/* ---- grain2 opcode ---- */

int grain2(GRAIN2 *p)
{
    int i, nn, w_interp, g_interp, f_nolock;
    MYFLT       *aout, *ft, *w_ft, grain_frq, frq_scl, pfrac, w_pfrac, f, a, k;
    unsigned long       n, mask, lobits, w_mask, w_lobits;
    unsigned long       g_frq, w_frq;
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

    for (nn = 0; nn < ksmps_; nn++) aout[nn] = FL(0.0);

    if (p->nr_osc == -1) {
      return OK;                   /* nothing to render */
    }
    else if ((p->seed == 0L) || (p->osc == NULL)) {
      return perferror(Str(X_1668,"grain2: not initialised"));
    }

    /* check grain ftable */

    ftp = ftfindp(p->kfn);
    if ((ftp == NULL) || ((ft = ftp->ftable) == NULL)) return NOTOK;
    oscbnk_flen_setup(ftp->flen, &mask, &lobits, &pfrac);

    p->grain_frq = grain_frq = *(p->kcps) * onedsr;     /* grain frequency */
    p->frq_scl   = frq_scl = *(p->kfmd) * onedsr;

    f            = onedsr / *(p->kgdur);           /* window frequency     */
    w_frq        = OSCBNK_PHS2INT(f);

    /* initialisation */

    if (p->init_k) {
      g_frq = OSCBNK_PHS2INT(grain_frq);
      for (i = 0; i < p->nr_osc; i++) {
        grain2_init_grain(p, o + i);
        grain2_init_grain_phase(o + i, g_frq, w_frq, frq_scl, f_nolock);
      }
      p->init_k = 0;
    }

    for (i = 0; i < p->nr_osc; i++) {   /* calculate grain frequency */
      if (f_nolock) {
        f = grain_frq + frq_scl * o[i].grain_frq_flt;
        o[i].grain_frq_int = OSCBNK_PHS2INT(f);
      }
    }
    aout = p->ar;                       /* audio output         */
    nn = ksmps_;
    do {
      i = p->nr_osc;
      do {
        /* grain waveform */
        n = o->grain_phs >> lobits; k = ft[n++];
        if (g_interp)
          k += (ft[n] - k) * (MYFLT) ((long) (o->grain_phs & mask)) * pfrac;
        o->grain_phs += o->grain_frq_int;
        o->grain_phs &= OSCBNK_PHSMSK;
        /* window waveform */
        n = o->window_phs >> w_lobits; a = w_ft[n++];
        if (w_interp)
          a += (w_ft[n] - a) * (MYFLT) ((long) (o->window_phs & w_mask))
            * w_pfrac;
        o->window_phs += w_frq;
        /* mix to output */
        *aout += a * k;
        if (o->window_phs >= OSCBNK_PHSMAX) {
          o->window_phs &= OSCBNK_PHSMSK;       /* new grain    */
          grain2_init_grain(p, o);
          /* grain frequency */
          if (f_nolock) {
            f = grain_frq + frq_scl * o->grain_frq_flt;
            o->grain_frq_int = OSCBNK_PHS2INT(f);
          }
        }
        o++;            /* next grain */
      } while (--i);
      o -= p->nr_osc;
      aout++;
    } while (--nn);
    return OK;
}

/* ---------------- grain3 set-up ---------------- */

int grain3set(GRAIN3 *p)
{
    int i;
    FUNC        *ftp;
    long        n;

    /* check opcode params */

    i = (int) (*(p->imode) + FL(0.5));  /* mode */
    if (i & 1) return OK;                  /* skip initialisation */
    p->init_k = 1;
    p->mode = i & 0x7E;
    p->x_phs = OSCBNK_PHSMAX;

    p->ovrlap = (int) (*(p->imaxovr) + FL(0.5));        /* max. overlap */
    p->ovrlap = (p->ovrlap < 1 ? 1 : p->ovrlap) + 1;

    oscbnk_seedrand(&(p->seed), *(p->iseed));  /* initialise seed */

    ftp = ftfind(p->iwfn);                     /* window table */
    if ((ftp == NULL) || ((p->wft = ftp->ftable) == NULL)) return NOTOK;
    oscbnk_flen_setup(ftp->flen, &(p->wft_mask), &(p->wft_lobits),
                       &(p->wft_pfrac));

    /* allocate space */

    n = ((long) ksmps_ + 1L) * (long) sizeof(unsigned long);
    n += (long) p->ovrlap * (long) sizeof(GRAIN2_OSC);
    if ((p->auxdata.auxp == NULL) || (p->auxdata.size < n))
      auxalloc(n, &(p->auxdata));
    p->phase = (unsigned long *) p->auxdata.auxp;
    p->osc = (GRAIN2_OSC *) ((unsigned long *) p->phase + ksmps_ + 1);
    p->osc_start = p->osc;
    p->osc_end = p->osc;
    p->osc_max = p->osc + (p->ovrlap - 1);
    return OK;
}

/* ---------------- grain3 performance ---------------- */

/* initialise grain */

void grain3_init_grain(GRAIN3 *p, GRAIN2_OSC *o,
                        unsigned long w_ph, unsigned long g_ph)
{
    MYFLT       f;

    /* start phase */

    f = oscbnk_rnd_bipolar(&(p->seed), p->p_rnd_pow, p->p_rnd_mode);
    f *= *(p->kpmd); if (p->pm_wrap) f -= (MYFLT) ((long) f);
    o->grain_phs = (g_ph + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    o->window_phs = w_ph;

    /* frequency */

    f = oscbnk_rnd_bipolar(&(p->seed), p->f_rnd_pow, p->f_rnd_mode);
    if (p->mode & 2) {
      o->grain_frq_flt = f;
    }
    else {                              /* lock frequency */
      f *= p->frq_scl;
      o->grain_frq_int = (p->grain_frq + OSCBNK_PHS2INT(f))
        & OSCBNK_PHSMSK;
    }
}

/* ---- grain3 opcode ---- */

int grain3(GRAIN3 *p)
{
    int i, nn, w_interp, g_interp, f_nolock;
    MYFLT       *aout0, *aout, *ft, *w_ft, frq_scl, pfrac, w_pfrac, f, a, k;
    MYFLT       wfdivxf, w_frq_f, x_frq_f;
    unsigned long       n, mask, lobits, w_mask, w_lobits;
    unsigned long       *phs, frq, x_ph, x_frq, g_ph, g_frq, w_ph, w_frq;
    GRAIN2_OSC  *o;
    FUNC        *ftp;

    /* clear output */

    for (nn = 0; nn < ksmps_; nn++) p->ar[nn] = FL(0.0);

    if ((p->seed == 0L) || (p->osc == NULL)) {
      return perferror(Str(X_1669,"grain3: not initialised"));
    }

    /* assign object data to local variables */

    aout0 = p->ar;                              /* audio output         */
    w_interp = (p->mode & 8 ? 1 : 0);   /* interpolate window   */
    g_interp = (p->mode & 4 ? 0 : 1);   /* interpolate grain    */
    f_nolock = (p->mode & 2 ? 1 : 0);   /* don't lock grain frq */
    w_ft = p->wft;                              /* window ftable        */
    w_mask = p->wft_mask; w_lobits = p->wft_lobits; w_pfrac = p->wft_pfrac;
    phs = p->phase;                             /* grain phase offset   */
    x_ph = p->x_phs;

    ftp = ftfindp(p->kfn);             /* check grain ftable           */
    if ((ftp == NULL) || ((ft = ftp->ftable) == NULL)) return NOTOK;
    oscbnk_flen_setup(ftp->flen, &mask, &lobits, &pfrac);

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
      f = *(p->kphs); g_ph = OSCBNK_PHS2INT(f);
    }
    else {
      f = p->phs0; g_ph = phs[ksmps_];
    }
    p->phs0 = *(p->kphs);
    /* convert phase modulation to frequency modulation */
    f = (MYFLT) ((double) p->phs0 - (double) f) / (MYFLT) ksmps_;
    f -= (MYFLT) ((long) f); g_frq = OSCBNK_PHS2INT(f);
    f = *(p->kcps) * onedsr;            /* grain frequency      */
    frq = (g_frq + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
    if (p->mode & 0x40) g_frq = frq;    /* phase sync   */
    /* calculate phase offset values for this k-cycle */
    for (nn = 0; nn <= ksmps_; nn++) {
      phs[nn] = g_ph; g_ph = (g_ph + g_frq) & OSCBNK_PHSMSK;
    }

    w_frq_f = onedsr / *(p->kgdur);             /* window frequency     */
    if ((w_frq_f < (FL(1.0) / (MYFLT) OSCBNK_PHSMAX)) ||
        (w_frq_f >= FL(1.0))) {
      return perferror(Str(X_1670,"grain3: invalid grain duration"));
    }
    w_frq = OSCBNK_PHS2INT(w_frq_f);
    x_frq_f = onedsr * *(p->kdens);             /* density              */
    if ((x_frq_f < (FL(1.0) / (MYFLT) OSCBNK_PHSMAX)) ||
        (x_frq_f >= FL(1.0))) {
      return perferror(Str(X_1671,"grain3: invalid grain density"));
    }
    x_frq = OSCBNK_PHS2INT(x_frq_f);
    wfdivxf = w_frq_f / ((MYFLT) OSCBNK_PHSMAX * x_frq_f);
    p->grain_frq = frq;                 /* grain frequency      */
    p->frq_scl = frq_scl = *(p->kfmd) * onedsr;
    p->pm_wrap = (fabs((double) *(p->kpmd)) > 0.9 ? 1 : 0);

    /* initialise grains (if enabled) */

    if ((p->init_k) && (!(p->mode & 0x10))) {
      f = w_frq_f / x_frq_f;
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
        if (p->osc_end == p->osc_start) {
          return perferror(Str(X_1672,"grain3 needs more overlaps"));
        }
        g_ph -= g_frq;
      }
    }
    p->init_k = 0;

    nn = ksmps_; o = p->osc_start;
    while (nn) {
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
        if (p->osc_end == p->osc_start) {
          return perferror(Str(X_1672,"grain3 needs more overlaps"));
        }
      }

      if (o == p->osc_end) {            /* no active grains     */
        x_ph += x_frq; nn--; aout0++; phs++; continue;
      }

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

      /* render grain */
      aout = aout0; i = nn;
      while (i--) {
        /* window waveform */
        n = w_ph >> w_lobits; a = w_ft[n++];
        if (w_interp) a += (w_ft[n] - a) * w_pfrac
                        * (MYFLT) ((long) (w_ph & w_mask));
        /* grain waveform */
        n = g_ph >> lobits; k = ft[n++];
        if (g_interp) k += (ft[n] - k) * pfrac
                        * (MYFLT) ((long) (g_ph & mask));
        /* mix to output */
        *(aout++) += a * k;
        /* update phase */
        g_ph = (g_ph + g_frq) & OSCBNK_PHSMSK;
        /* check for end of grain */
        if ((w_ph += w_frq) >= OSCBNK_PHSMAX) {
          if (++(p->osc_start) > p->osc_max)
            p->osc_start = p->osc;
          break;
        }
      }
      /* save phase */
      o->grain_phs = g_ph; o->window_phs = w_ph;
      /* next grain */
      if (++o > p->osc_max) o = p->osc;
    }
    p->x_phs = x_ph;
    return OK;
}

/* ----------------------------- rnd31 opcode ------------------------------ */

int rnd31set(RND31 *p)
{
    /* initialise random seed */
    oscbnk_seedrand(&(p->seed), *(p->iseed));
    return OK;
}

/* ---- rnd31 / i-rate ---- */

int rnd31i(RND31 *p)
{
    MYFLT       rpow;
    int         rmode;

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
    if (*(p->iseed) < FL(0.5)) { /* seed from current time      */
      if (rnd31i_seed <= 0L)     /* check if already initialised        */
        oscbnk_seedrand(&rnd31i_seed, FL(0.0));
    }
    else {                       /* explicit seed value         */
      oscbnk_seedrand(&rnd31i_seed, *(p->iseed));
    }

    *(p->out) = *(p->scl) * oscbnk_rnd_bipolar(&rnd31i_seed, rpow, rmode);
    return OK;
}

/* ---- rnd31 / k-rate ---- */

int rnd31k(RND31 *p)
{
    MYFLT       rpow;
    int rmode;

    if ((p->seed < 1L) || (p->seed > 0x7FFFFFFEL)) {
      return perferror(Str(X_1673,"rnd31: not initialised"));
    }

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
}

/* ---- rnd31 / a-rate ---- */

int rnd31a(RND31 *p)
{
    MYFLT       scl, *out, rpow;
    int rmode, nn;

    if ((p->seed < 1L) || (p->seed > 0x7FFFFFFEL)) {
      return perferror(Str(X_1673,"rnd31: not initialised"));
    }

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

    scl = *(p->scl); out = p->out;
    nn = ksmps_;
    while (nn--) {
      *(out++) = scl * oscbnk_rnd_bipolar(&(p->seed), rpow, rmode);
    }
    return OK;
}

/* ---- oscilikt initialisation ---- */

int oscktset(OSCKT *p)
{
    MYFLT   phs;

    if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
    /* initialise table parameters */
    p->oldfn = FL(-1.0);
    p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
    /* initial phase */
    phs = *(p->iphs) - (MYFLT) ((long) *(p->iphs));
    p->phs = OSCBNK_PHS2INT(phs);
    return OK;
}

/* ---- oscilikt performance ---- */

int kosclikt(OSCKT *p)
{
    FUNC    *ftp;
    unsigned long   n, phs;
    MYFLT   v, *ft;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs;
    /* read from table with interpolation */
    n = phs >> p->lobits; v = (MYFLT) ((long) (phs & p->mask)) * p->pfrac;
    *(p->sr) = (p->ft[n] + (p->ft[n + 1] - p->ft[n]) * v) * *(p->xamp);
    /* update phase */
    v = *(p->xcps) * onedkr;
    p->phs = (phs + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
    return OK;
}

int osckkikt(OSCKT *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask, frq;
    MYFLT   pfrac, *ft, v, a, *ar;
    int     nn;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs; a = *(p->xamp); ar = p->sr;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    /* read from table with interpolation */
    v = *(p->xcps) * onedsr; frq = OSCBNK_PHS2INT(v);
    nn = ksmps_;
    do {
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      phs = (phs + frq) & OSCBNK_PHSMSK;
      *(ar++) = v * a;
    } while (--nn);
    /* save new phase */
    p->phs = phs;
    return OK;
}

int osckaikt(OSCKT *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask;
    MYFLT   pfrac, *ft, v, a, *ar, *xcps;
    int     nn;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs; a = *(p->xamp); ar = p->sr; xcps = p->xcps;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    /* read from table with interpolation */
    nn = ksmps_;
    do {
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      *(ar++) = v * a;
      v = *(xcps++) * onedsr;
      phs = (phs + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
    } while (--nn);
    /* save new phase */
    p->phs = phs;
    return OK;
}

void oscbnk_flen_setup(long flen, unsigned long *mask,
                       unsigned long *lobits, MYFLT *pfrac)
{
    unsigned long       n;

    n = (unsigned long) flen;
    *lobits = 0UL; *mask = 1UL; *pfrac = FL(0.0);
    if (n < 2UL) return;
    while (n < OSCBNK_PHSMAX) {
      n <<= 1; *mask <<= 1; (*lobits)++;
    }
    *pfrac = FL(1.0) / (MYFLT) *mask; (*mask)--;
}

int oscakikt(OSCKT *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask, frq;
    MYFLT   pfrac, *ft, v, *ar, *xamp;
    int     nn;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs; xamp = p->xamp; ar = p->sr;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    /* read from table with interpolation */
    v = *(p->xcps) * onedsr; frq = OSCBNK_PHS2INT(v);
    nn = ksmps_;
    do {
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      phs = (phs + frq) & OSCBNK_PHSMSK;
      *(ar++) = v * *(xamp++);
    } while (--nn);
    /* save new phase */
    p->phs = phs;
    return OK;
}

int oscaaikt(OSCKT *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask;
    MYFLT   pfrac, *ft, v, *ar, *xcps, *xamp;
    int     nn;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs; ar = p->sr; xcps = p->xcps; xamp = p->xamp;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    /* read from table with interpolation */
    nn = ksmps_;
    do {
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      *(ar++) = v * *(xamp++);
      v = *(xcps++) * onedsr;
      phs = (phs + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
    } while (--nn);
    /* save new phase */
    p->phs = phs;
    return OK;
}

/* ---- osciliktp initialisation ---- */

int oscktpset(OSCKTP *p)
{
    if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
    /* initialise table parameters */
    p->oldfn = FL(-1.0);
    p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
    /* initial phase */
    p->phs = 0UL; p->old_phs = FL(0.0);
    p->init_k = 1;
    p->dv_ksmps = FL(1.0) / (MYFLT) ksmps_;
    return OK;
}

/* ---- osciliktp performance ---- */

int oscktp(OSCKTP *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask, frq;
    MYFLT   pfrac, *ft, v, *ar;
    int     nn;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft; phs = p->phs; ar = p->ar;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    v = *(p->kcps) * onedsr;
    frq = OSCBNK_PHS2INT(v);
    /* initialise phase if 1st k-cycle */
    if (p->init_k) {
      p->init_k = 0;
      p->old_phs = *(p->kphs);
      v = *(p->kphs) - (MYFLT) ((long) *(p->kphs));
      phs = OSCBNK_PHS2INT(v);
    }
    /* convert phase modulation to frequency modulation */
    v = (MYFLT) ((double) *(p->kphs) - (double) p->old_phs) * p->dv_ksmps;
    p->old_phs = *(p->kphs);
    frq = (frq + OSCBNK_PHS2INT(v)) & OSCBNK_PHSMSK;
    /* read from table with interpolation */
    nn = ksmps_;
    do {
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      phs = (phs + frq) & OSCBNK_PHSMSK;
      *(ar++) = v;
    } while (--nn);
    /* save new phase */
    p->phs = phs;
    return OK;
}

/* ---- oscilikts initialisation ---- */

int oscktsset(OSCKTS *p)
{
    if (*(p->istor) != FL(0.0)) return OK;         /* skip initialisation */
    /* initialise table parameters */
    p->oldfn = FL(-1.0);
    p->lobits = p->mask = 0UL; p->pfrac = FL(0.0); p->ft = NULL;
    /* initial phase */
    p->phs = 0UL;
    p->init_k = 1;
    return OK;
}

/* ---- oscilikts performance ---- */

int osckts(OSCKTS *p)
{
    FUNC    *ftp;
    unsigned long   n, phs, lobits, mask, frq = 0UL;
    MYFLT   pfrac, *ft, v, *ar, *xcps, *xamp, *async;
    int     nn, a_amp, a_cps;

    /* check if table number was changed */
    if (*(p->kfn) != p->oldfn || p->ft == NULL) {
      p->oldfn = *(p->kfn);
      ftp = ftfindp(p->kfn);           /* new table parameters */
      if ((ftp == NULL) || ((p->ft = ftp->ftable) == NULL)) return NOTOK;
      oscbnk_flen_setup(ftp->flen, &(p->mask), &(p->lobits), &(p->pfrac));
    }

    /* copy object data to local variables */
    ft = p->ft;
    a_amp = (XINARG1 ? 1 : 0); a_cps = (XINARG2 ? 1 : 0);
    phs = p->phs; ar = p->ar; xcps = p->xcps; xamp = p->xamp; async = p->async;
    lobits = p->lobits; mask = p->mask; pfrac = p->pfrac;
    if (!a_cps) {
      v = *xcps * onedsr;
      frq = OSCBNK_PHS2INT(v);
    }
    /* initialise phase if 1st k-cycle */
    if (p->init_k) {
      p->init_k = 0;
      v = *(p->kphs) - (MYFLT) ((long) *(p->kphs));
      phs = OSCBNK_PHS2INT(v);
    }
    /* read from table with interpolation */
    nn = ksmps_;
    do {
      if (*(async++) > FL(0.0)) {               /* re-initialise phase */
        v = *(p->kphs) - (MYFLT) ((long) *(p->kphs));
        phs = OSCBNK_PHS2INT(v);
      }
      n = phs >> lobits;
      v = ft[n++]; v += (ft[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
      *(ar++) = v * *xamp;
      if (a_amp) xamp++;
      if (a_cps) {
        v = *(xcps++) * onedsr;
        frq = OSCBNK_PHS2INT(v);
      }
      phs = (phs + frq) & OSCBNK_PHSMSK;
    } while (--nn);
    /* save new phase */
    p->phs = phs;
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

static int vco2_nr_table_arrays = 0;    /* number of available table arrays */
static VCO2_TABLE_ARRAY **vco2_tables = NULL;

#define VCO2_MAX_NPART  4096    /* maximum number of harmonic partials */

typedef struct {
    int     waveform;           /* waveform number (< 0: user defined)       */
    int     w_npart;            /* nr of partials in user specified waveform */
    double  npart_mul;          /* multiplier for number of partials         */
    int     min_size, max_size; /* minimum and maximum table size            */
    complex *w_fftbuf;          /* FFT of user specified waveform            */
} VCO2_TABLE_PARAMS;

/* remove table array for the specified waveform */

static void vco2_delete_table_array(int w)
{
    int j;
    /* table array does not exist: nothing to do */
    if (vco2_tables == NULL || w >= vco2_nr_table_arrays
        || vco2_tables[w] == NULL)
      return;
#ifdef VCO2FT_USE_TABLE
    /* free number of partials -> table list, */
    mfree(vco2_tables[w]->nparts_tabl);
#else
    /* free number of partials list, */
    mfree(vco2_tables[w]->nparts);
#endif
    /* table data (only if not shared as standard Csound ftables), */
    for (j = 0; j < vco2_tables[w]->ntabl; j++) {
      if (vco2_tables[w]->base_ftnum < 1)
        mfree(vco2_tables[w]->tables[j].ftable);
    }
    /* table list, */
    mfree(vco2_tables[w]->tables);
    /* and table array structure */
    mfree(vco2_tables[w]);
    vco2_tables[w] = NULL;
}

/* free memory used by all vco2 table arrays (called by RESET routines) */

void vco2_tables_destroy(void)
{
    int i;

    if (vco2_tables == NULL) return;    /* no tables */
    for (i = 0; i < vco2_nr_table_arrays; i++) vco2_delete_table_array(i);
    mfree(vco2_tables);
    vco2_tables = NULL;
    vco2_nr_table_arrays = 0;
}

/* generate a table using the waveform specified in tp */

static void vco2_calculate_table(VCO2_TABLE *table, VCO2_TABLE_PARAMS *tp)
{
    complex *fftbuf, *ex;
    int     i, minh;

    /* allocate memory for FFT */
    ex = AssignBasis(NULL, (long) table->size);
    fftbuf = (complex*) mmalloc(sizeof(complex) * ((table->size >> 1) + 1));
    if (tp->waveform >= 0) {                    /* no DC offset for built-in */
      minh = 1; fftbuf[0].re = fftbuf[0].im = FL(0.0);  /* waveforms */
    }
    else minh = 0;
    /* calculate FFT of the requested waveform */
    for (i = minh; i <= (table->size >> 1); i++) {
      fftbuf[i].re = fftbuf[i].im = FL(0.0);
      if (i > table->npart) continue;
      switch (tp->waveform) {
      case 0:                                   /* sawtooth */
        fftbuf[i].im = FL(-2.0) / (PI_F * (MYFLT) i);
        break;
      case 1:                                   /* 4 * x * (1 - x) */
        fftbuf[i].re = FL(-4.0) / (PI_F * PI_F * (MYFLT) i * (MYFLT) i);
        break;
      case 2:                                   /* pulse */
        fftbuf[i].re = FL(1.0);
        break;
      case 3:                                   /* square */
        fftbuf[i].im = (i & 1 ? (FL(-4.0) / (PI_F * (MYFLT) i)) : FL(0.0));
        break;
      case 4:                                   /* triangle */
        fftbuf[i].im = (i & 1 ? ((i & 2 ? FL(8.0) : FL(-8.0))
                                 / (PI_F * PI_F * (MYFLT) i * (MYFLT) i))
                                : FL(0.0));
        break;
      default:                                  /* user defined */
        if (i <= tp->w_npart) {
          fftbuf[i].re = tp->w_fftbuf[i].re;
          fftbuf[i].im = tp->w_fftbuf[i].im;
        }
      }
    }
    /* inverse FFT */
    FFT2torlpacked(fftbuf, (long) table->size, FL(0.5), ex);
    /* copy to table */
    i = 0;
    do {
      table->ftable[i] = fftbuf[i >> 1].re; i++;
      table->ftable[i] = fftbuf[i >> 1].im; i++;
    } while (i < table->size);
    /* write guard point */
    table->ftable[table->size] = fftbuf[0].re;
    /* free memory used by temporary buffers */
    mfree(fftbuf);
}

/* set default table parameters depending on waveform */

static void vco2_default_table_params(int w, VCO2_TABLE_PARAMS *tp)
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

static int vco2_table_size(int npart, VCO2_TABLE_PARAMS *tp)
{
    int     n;

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

extern FUNC* hfgens(EVTBLK*);

static int vco2_tables_create(int waveform, int base_ftable,
                              VCO2_TABLE_PARAMS *tp)
{
    int     i, npart, ntables;
    double  npart_f;
    VCO2_TABLE_ARRAY    *tables;
    VCO2_TABLE_PARAMS   tp2;

    /* set default table parameters if not specified in tp */
    if (tp == NULL) {
      if (waveform < 0) return -1;
      vco2_default_table_params(waveform, &tp2);
      tp = &tp2;
    }
    waveform = (waveform < 0 ? 4 - waveform : waveform);
    if (waveform >= vco2_nr_table_arrays) {
      /* extend space for table arrays */
      ntables = ((waveform >> 4) + 1) << 4;
      vco2_tables = (VCO2_TABLE_ARRAY**)
        mrealloc(vco2_tables, sizeof(VCO2_TABLE_ARRAY*) * ntables);
      for (i = vco2_nr_table_arrays; i < ntables; i++) vco2_tables[i] = NULL;
      vco2_nr_table_arrays = ntables;
    }
    /* clear table array if already initialised */
    if (vco2_tables[waveform] != NULL) {
      vco2_delete_table_array(waveform);
      if (O.msglevel & WARNMSG)
        printf(errmsg, "WARNING: redefined table array for waveform %d\n",
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
    tables = vco2_tables[waveform] =
      (VCO2_TABLE_ARRAY*) mcalloc(sizeof(VCO2_TABLE_ARRAY));
      /* ... and all tables */
#ifdef VCO2FT_USE_TABLE
    tables->nparts_tabl =
      (VCO2_TABLE**) mmalloc(sizeof(VCO2_TABLE*) * (VCO2_MAX_NPART + 1));
#else
    tables->nparts = (MYFLT*) mmalloc(sizeof(MYFLT) * (ntables * 3));
    for (i = 0; i < ntables; i++) {
      tables->nparts[i] = FL(-1.0);     /* padding for number of partials */
      tables->nparts[(ntables << 1) + i] = FL(1.0e24);  /* list */
    }
#endif
    tables->tables = (VCO2_TABLE*) mcalloc(sizeof(VCO2_TABLE) * ntables);
      /* generate tables */
    tables->ntabl = ntables;            /* store number of tables */
    tables->base_ftnum = base_ftable;   /* and base ftable number */
    npart_f = 0.0; i = 0;
      do {
        /* store number of partials, */
      npart = tables->tables[i].npart = (int) (npart_f + 0.5);
#ifndef VCO2FT_USE_TABLE
      tables->nparts[ntables + i] = (MYFLT) npart;
#endif
        /* table size, */
      tables->tables[i].size = vco2_table_size(npart, tp);
        /* and other parameters */
      oscbnk_flen_setup((long) tables->tables[i].size,
                        &(tables->tables[i].mask),
                        &(tables->tables[i].lobits),
                        &(tables->tables[i].pfrac));
        /* if base ftable was specified, generate empty table ... */
        if (base_ftable > 0) {
          EVTBLK  e;
          FUNC    *ftp;

          e.strarg = NULL;                      /* create "f" event */
          e.opcod = 'f';
          e.pcnt = 5;
          e.p[1] = (MYFLT) base_ftable;
          e.p[2] = e.p2orig = e.offtim = FL(0.0);
        e.p[3] = e.p3orig = (MYFLT) tables->tables[i].size;
          e.p[4] = FL(-2.0);           /* GEN02 */
          e.p[5] = FL(0.0);
          if ((ftp = hfgens(&e)) == NULL) return -1;
        tables->tables[i].ftable = ftp->ftable;
          base_ftable++;                /* next table number */
        }
        else    /* ... else allocate memory (cannot be accessed as a       */
        tables->tables[i].ftable =      /* standard Csound ftable) */
          (MYFLT*) mmalloc(sizeof(MYFLT) * (tables->tables[i].size + 1));
        /* now calculate the table */
      vco2_calculate_table(&(tables->tables[i]), tp);
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

int vco2init(VCO2INIT *p)
{
    int     waveforms, base_ftable, ftnum, i, w;
    VCO2_TABLE_PARAMS   tp;
    FUNC    *ftp;
    complex *ex;

    /* check waveform number */
    waveforms = (int) (*(p->iwaveforms)
                       + (*(p->iwaveforms) < FL(0.0) ? FL(-0.5) : FL(0.5)));
    if (waveforms < -1000000 || waveforms > 31) {
      sprintf(errmsg, "vco2init: invalid waveform number: %f",
                      *(p->iwaveforms));
      return initerror(errmsg);
    }
    /* base ftable number (required by user defined waveforms except -1) */
    ftnum = base_ftable = (int) (*(p->iftnum) + FL(0.5));
    if (ftnum < 1) ftnum = base_ftable = -1;
    if ((waveforms < -1 && ftnum < 1) || ftnum > 1000000) {
      return initerror(Str(X_1761,"vco2init: invalid base ftable number"));
    }
    *(p->ift) = (MYFLT) ftnum;
    if (!waveforms) return OK;     /* nothing to do */
    w = (waveforms < 0 ? waveforms : 0);
    do {
    /* set default table parameters, */
      vco2_default_table_params(w, &tp);
    /* and override with user specified values (if there are any) */
    if (*(p->ipmul) > FL(0.0)) {
      if (*(p->ipmul) < FL(1.00999) || *(p->ipmul) > FL(2.00001)) {
        return initerror(Str(X_1763,"vco2init: invalid partial number multiplier"));
      }
      tp.npart_mul = (double) *(p->ipmul);
    }
    if (*(p->iminsiz) > FL(0.0)) {
      i = (int) (*(p->iminsiz) + FL(0.5));
      if (i < 16 || i > 262144 || (i & (i - 1))) {
        return initerror(Str(X_1764,"vco2init: invalid min table size"));
      }
      tp.min_size = i;
    }
    if (*(p->imaxsiz) > FL(0.0)) {
      i = (int) (*(p->imaxsiz) + FL(0.5));
      if (i < 16 || i > 16777216 || (i & (i - 1)) || i < tp.min_size) {
        return initerror(Str(X_1765,"vco2init: invalid max table size"));
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
          ftnum = vco2_tables_create(w, ftnum, &tp);
          if (base_ftable > 0 && ftnum <= 0) {
            return initerror(Str(X_787,"ftgen error"));
          }
        }
      }
    else {                      /* user defined, requires source ftable */
      if ((ftp = ftfind(p->isrcft)) == NULL || ftp->flen < 4) {
        return initerror(Str(X_1766,"vco2init: invalid source ftable"));
      }
      /* analyze source table, and store results in table params structure */
      i = ftp->flen;
      tp.w_npart = i >> 1;
      ex = AssignBasis(NULL, (long) i);
      tp.w_fftbuf = (complex*) mmalloc(sizeof(complex) * ((i >> 1) + 1));
      for (i = 0; i < ftp->flen; i++) {
        tp.w_fftbuf[i >> 1].re = ftp->ftable[i] / (MYFLT) (ftp->flen >> 1);
        i++;
        tp.w_fftbuf[i >> 1].im = ftp->ftable[i] / (MYFLT) (ftp->flen >> 1);
      }
      FFT2realpacked(tp.w_fftbuf, ftp->flen, ex);
      /* generate table array */
      ftnum = vco2_tables_create(waveforms, ftnum, &tp);
        /* free memory used by FFT buffer */
        mfree(tp.w_fftbuf);
      if (base_ftable > 0 && ftnum <= 0) {
        return initerror(Str(X_787,"ftgen error"));
      }
    }
    *(p->ift) = (MYFLT) ftnum;
      w++;
    } while (w > 0 && w < 5);
    return OK;
}

/* ---- vco2ft / vco2ift opcode (initialisation) ---- */

int vco2ftp(VCO2FT*);

int vco2ftset(VCO2FT *p)
{
    int     w;

    w = (int) (*(p->iwave) + (*(p->iwave) < FL(0.0) ? FL(-0.5) : FL(0.5)));
    if (w > 4) w = 0x7FFFFFFF;
    if (w < 0) w = 4 - w;
    if (w >= vco2_nr_table_arrays || vco2_tables[w] == NULL
        || vco2_tables[w]->base_ftnum < 1) {
      return initerror(Str(X_1721,"vco2ft: table array not found for this waveform"));
    }
#ifdef VCO2FT_USE_TABLE
    p->nparts_tabl = vco2_tables[w]->nparts_tabl;
    p->tab0 = vco2_tables[w]->tables;
#else
    /* address of number of partials list (with offset for padding) */
    p->nparts = vco2_tables[w]->nparts + vco2_tables[w]->ntabl;
    p->npart_old = p->nparts + (vco2_tables[w]->ntabl >> 1);
#endif
    p->base_ftnum = vco2_tables[w]->base_ftnum;
    if (*(p->inyx) > FL(0.5))
      p->p_scl = FL(0.5) * esr_;
    else if (*(p->inyx) < FL(0.001))
      p->p_scl = FL(0.001) * esr_;
    else
      p->p_scl = *(p->inyx) * esr_;
    p->p_min = p->p_scl / (MYFLT) VCO2_MAX_NPART;
    /* in case of vco2ift opcode, find table number now */
    if (!strcmp(p->h.optext->t.opcod, "vco2ift"))
      vco2ftp(p);
    else                                /* else set perf routine to avoid */
      p->h.opadr = (SUBR) vco2ftp;      /* "not initialised" error */
    return OK;
}

/* ---- vco2ft opcode (performance) ---- */

int vco2ftp(VCO2FT *p)
{
#ifdef VCO2FT_USE_TABLE
    MYFLT   npart;
    int     n;
#else
    MYFLT   npart, *nparts;
    int     nn;
#endif

    npart = (MYFLT)fabs(*(p->kcps)); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
    n = (int) (p->nparts_tabl[(int) (p->p_scl / npart)] - p->tab0);
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
    *(p->kft) = (MYFLT) ((int) (nparts - p->nparts) + p->base_ftnum);
#endif
    return OK;
}

int vco2ft(VCO2FT *p)
{
    return perferror(Str(X_1722,"vco2ft: not initialised"));
}

/* ---- vco2 opcode (initialisation) ---- */

int vco2set(VCO2 *p)
{
    int     mode, min_args, tnum;
    int     tnums[8] = { 0, 0, 1, 2, 1, 3, 4, 5 };
    int     modes[8] = { 0, 1, 2, 0, 0, 0, 0, 0 };
    MYFLT   x;

    /* check number of args */
    if (p->INOCOUNT > 6) {
      return initerror(Str(X_1723,"vco2: too many input arguments"));
    }
    mode = (int) (*(p->imode) + FL(0.5)) & 0x1F;
    if (mode & 1) return OK;               /* skip initialisation */
    /* more checks */
    min_args = 2;
    if ((mode & 14) == 2 || (mode & 14) == 4) min_args = 4;
    if (mode & 16) min_args = 5;
    if (p->INOCOUNT < min_args) {
      return initerror(Str(X_1724,"vco2: insufficient required arguments"));
    }
    if (p->XINCODE) {
      return initerror(Str(X_1725,"vco2: invalid argument type"));
    }
    /* select table array and algorithm, according to waveform */
    tnum = tnums[(mode & 14) >> 1];
    p->mode = modes[(mode & 14) >> 1];
    /* initialise tables if not done yet */
    if (tnum >= vco2_nr_table_arrays || vco2_tables[tnum] == NULL) {
      if (tnum < 5)
        vco2_tables_create(tnum, -1, NULL);
      else {
        return initerror(Str(X_1768, "vco2: table array not found for user defined waveform"));
      }
    }
#ifdef VCO2FT_USE_TABLE
    p->nparts_tabl = vco2_tables[tnum]->nparts_tabl;
#else
    /* address of number of partials list (with offset for padding) */
    p->nparts = vco2_tables[tnum]->nparts + vco2_tables[tnum]->ntabl;
    p->npart_old = p->nparts + (vco2_tables[tnum]->ntabl >> 1);
    p->tables = vco2_tables[tnum]->tables;
#endif
    /* set misc. parameters */
    p->init_k = 1;
    p->pm_enabled = (mode & 16 ? 1 : 0);
    if ((mode & 16) || (p->INOCOUNT < 5))
      p->phs = 0UL;
    else {
      x = *(p->kphs); x -= (MYFLT) ((long) x);
      p->phs = OSCBNK_PHS2INT(x);
    }
    p->f_scl = onedsr;
    x = (p->INOCOUNT < 6 ? FL(0.5) : *(p->inyx));
    if (x < FL(0.001)) x = FL(0.001);
    if (x > FL(0.5)) x = FL(0.5);
    p->p_min = x / (MYFLT) VCO2_MAX_NPART;
    p->p_scl = x;
    p->dv_ksmps = FL(1.0) / ensmps;
    return OK;
}

/* ---- vco2 opcode (performance) ---- */

int vco2(VCO2 *p)
{
    int     nn, n;
    VCO2_TABLE      *tabl;
    unsigned long   phs, phs2, frq, frq2, lobits, mask;
#ifdef VCO2FT_USE_TABLE
    MYFLT   f, f1, npart, pfrac, v, *ftable, kamp, *ar;
    if (p->nparts_tabl == NULL) {
#else
    MYFLT   f, f1, npart, *nparts, pfrac, v, *ftable, kamp, *ar;
    if (p->tables == NULL) {
#endif
      return perferror(Str(X_1726,"vco2: not initialised"));
    }
    /* if 1st k-cycle, initialise now */
    if (p->init_k) {
      p->init_k = 0;
      if (p->pm_enabled) {
        f = p->kphs_old = *(p->kphs); f -= (MYFLT) ((long) f);
        p->phs = OSCBNK_PHS2INT(f);
      }
      if (p->mode) {
        p->kphs2_old = -(*(p->kpw));
        f = p->kphs2_old; f -= (MYFLT) ((long) f);
        p->phs2 = (p->phs + OSCBNK_PHS2INT(f)) & OSCBNK_PHSMSK;
      }
    }
    /* calculate frequency (including phase modulation) */
    f = *(p->kcps) * p->f_scl;
    frq = OSCBNK_PHS2INT(f);
    if (p->pm_enabled) {
      f1 = (MYFLT) ((double) *(p->kphs) - (double) p->kphs_old) * p->dv_ksmps;
      p->kphs_old = *(p->kphs);
      frq = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      f += f1;
    }
    /* find best table for current frequency */
    npart = (MYFLT)fabs(f); if (npart < p->p_min) npart = p->p_min;
#ifdef VCO2FT_USE_TABLE
    tabl = p->nparts_tabl[(int) (p->p_scl / npart)];
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
    tabl = p->tables + (int) (nparts - p->nparts);
#endif
    /* copy object data to local variables */
    ar = p->ar;
    kamp = *(p->kamp);
    phs = p->phs;
    lobits = tabl->lobits; mask = tabl->mask; pfrac = tabl->pfrac;
    ftable = tabl->ftable;

    nn = ksmps_;
    if (!p->mode) {                     /* - mode 0: simple table playback - */
      do {
        n = phs >> lobits;
        v = ftable[n++];
        v += (ftable[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
        phs = (phs + frq) & OSCBNK_PHSMSK;
        *(ar++) = v * kamp;
      } while (--nn);
    }
    else {
      v = -(*(p->kpw));                                 /* pulse width */
      f1 = (MYFLT) ((double) v - (double) p->kphs2_old) * p->dv_ksmps;
      f = p->kphs2_old; f -= (MYFLT) ((long) f); if (f < FL(0.0)) f++;
      p->kphs2_old = v;
      phs2 = p->phs2;
      frq2 = (frq + OSCBNK_PHS2INT(f1)) & OSCBNK_PHSMSK;
      if (p->mode == 1) {               /* - mode 1: PWM - */
        /* DC correction offset */
        f = FL(1.0) - FL(2.0) * f;
        f1 *= FL(-2.0);
        do {
          n = phs >> lobits;
          v = ftable[n++];
          *ar = v + (ftable[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((long) (phs2 & mask)) * pfrac;
          *ar = (*ar - v + f) * kamp;
          ar++;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        } while (--nn);
      }
      else {                            /* - mode 2: saw / triangle ramp - */
        do {
          n = phs >> lobits;
          v = ftable[n++];
          *ar = v + (ftable[n] - v) * (MYFLT) ((long) (phs & mask)) * pfrac;
          n = phs2 >> lobits;
          v = ftable[n++];
          v += (ftable[n] - v) * (MYFLT) ((long) (phs2 & mask)) * pfrac;
          *ar = (*ar - v) * (FL(0.25) / (f - f * f)) * kamp;
          ar++;
          phs = (phs + frq) & OSCBNK_PHSMSK;
          phs2 = (phs2 + frq2) & OSCBNK_PHSMSK;
          f += f1;
        } while (--nn);
      }
      p->phs2 = phs2;
    }
    /* save oscillator phase */
    p->phs = phs;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "oscbnk",   S(OSCBNK), 5, "a", "kkkkiikkkkikkkkkkikooooooo",
                                 (SUBR)oscbnkset, NULL, (SUBR)oscbnk   },
{ "grain2",   S(GRAIN2), 5, "a", "kkkikiooo", 
                                 (SUBR)grain2set, NULL, (SUBR)grain2   },
{ "grain3",   S(GRAIN3), 5, "a", "kkkkkkikikkoo", 
                                 (SUBR)grain3set, NULL, (SUBR)grain3   },
{ "rnd31",    0xFFFF                                                          },
{ "rnd31_i",  S(RND31),  1, "i", "iio",  (SUBR)rnd31i, NULL, NULL             },
{ "rnd31_k",  S(RND31),  3, "k", "kko",  (SUBR)rnd31set, (SUBR)rnd31k, NULL   },
{ "rnd31_a",  S(RND31),  5, "a", "kko",  (SUBR)rnd31set, NULL, (SUBR)rnd31a   },
/* IV - Aug 23 2002, IV - Sep 5 2002 */
{ "oscilikt", 0xFFFE                                                          },
{ "oscilikt_kk",S(OSCKT), 7,"s", "kkkoo",  (SUBR)oscktset, (SUBR)kosclikt, (SUBR)osckkikt   },
{ "oscilikt_ka",S(OSCKT), 5,"a", "kakoo",  (SUBR)oscktset, NULL,  (SUBR)osckaikt },
{ "oscilikt_ak",S(OSCKT), 5,"a", "akkoo",  (SUBR)oscktset, NULL,  (SUBR)oscakikt },
{ "oscilikt_aa",S(OSCKT), 5,"a", "aakoo",  (SUBR)oscktset, NULL,  (SUBR)oscaaikt },
{ "osciliktp", S(OSCKTP),5, "a", "kkko",   (SUBR)oscktpset, NULL, (SUBR)oscktp   },
{ "oscilikts", S(OSCKTS),5, "a", "xxkako", (SUBR)oscktsset, NULL, (SUBR)osckts   },
/* IV - Sep 25 2002 -- new opcodes: vco2init, vco2ft, vco2 */
{ "vco2init", S(VCO2INIT), 1,   "i", "ijjjjj",   (SUBR)vco2init, NULL, NULL      },
{ "vco2ift",    S(VCO2FT),1,    "i", "iov",      (SUBR)vco2ftset, NULL, NULL     },
{ "vco2ft",     S(VCO2FT),3,    "k", "kov",      (SUBR)vco2ftset, (SUBR)vco2ft, NULL        },
{ "vco2",       S(VCO2),  5,    "a", "kkoM",     (SUBR)vco2set, NULL, (SUBR)vco2 },
};

LINKAGE

void RESET(void)
{
    vco2_tables_destroy();
}
