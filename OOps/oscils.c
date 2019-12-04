/*
    oscils.c:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/* ------ oscils, lphasor, and tablexkt by Istvan Varga (Jan 5 2002) ------ */

#include "csoundCore.h"
#include <math.h>
#define CSOUND_OSCILS_C 1
#include "oscils.h"

/* ------------- set up fast sine generator ------------- */
/* Input args:                                            */
/*   a: amplitude                                         */
/*   f: frequency (-PI - PI)                              */
/*   p: initial phase (0 - PI/2)                          */
/* Output args:                                           */
/*  *x: first output sample                               */
/*  *c, *v: coefficients for calculating next sample as   */
/*          shown below:                                  */
/*            v = v + c * x                               */
/*            x = x + v                                   */
/*          These values are calculated by:               */
/*            x = y[0]                                    */
/*            c = 2.0 * cos(f) - 2.0                      */
/*            v = y[1] - (c + 1.0) * y[0]                 */
/*          where y[0], and y[1] are the first, and       */
/*          second sample of the sine wave to be          */
/*          generated, respectively.                      */
/* -------- written by Istvan Varga, Jan 28 2002 -------- */

static void init_sine_gen(double a, double f, double p,
                           double *x, double *c, double *v)
{
    double  y0, y1;                 /* these should be doubles */

    y0 = sin(p);
    y1 = sin(p + f);
    *x = y0;
    *c = 2.0 * cos(f) - 2.0;
    *v = y1 - *c * y0 - y0;
    /* amp. scale */
    *x *= a; *v *= a;
}

/* -------- oscils set-up -------- */

int32_t oscils_set(CSOUND *csound, OSCILS *p)
{
    int32_t     iflg;

    iflg = (int32_t) (*(p->iflg) + FL(0.5)) & 0x07; /* check flags */
    if (UNLIKELY(iflg & 1)) return OK;          /* skip init, nothing to do */
    p->use_double = (iflg & 2 ? 1 : 0);         /* use doubles internally */
    init_sine_gen((double)*(p->iamp), (double)(*(p->icps) * csound->tpidsr),
                  (double)(*(p->iphs) * TWOPI_F),
                   &(p->xd), &(p->cd), &(p->vd));
    if (!(p->use_double)) {
      p->x = (MYFLT) p->xd;       /* use floats */
      p->c = (MYFLT) p->cd;
      p->v = (MYFLT) p->vd;
    }
    return OK;
}

/* -------- oscils performance -------- */

int32_t oscils(CSOUND *csound, OSCILS *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *ar, x, c, v;
    double  xd, cd, vd;

    /* copy object data to local variables */
    ar = p->ar;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->use_double) {            /* use doubles */
      xd = p->xd; cd = p->cd; vd = p->vd;
      for (n=offset; n<nsmps; n++) {
        ar[n] = (MYFLT) xd;
        vd += cd * xd;
        xd += vd;
      }
      p->xd = xd; p->vd = vd;
    }
    else {                          /* use floats */
      x = p->x; c = p->c; v = p->v;
      for (n=offset; n<nsmps; n++) {
        ar[n] = x;
        v += c * x;
        x += v;
      }
      p->x = x; p->v = v;
    }
    return OK;
}

/* -------- lphasor set-up -------- */

int32_t lphasor_set(CSOUND *csound, LPHASOR *p)
{
    IGN(csound);
    if (UNLIKELY(*(p->istor) != FL(0.0))) return OK;       /* nothing to do */

    p->phs = (double)*(p->istrt);                          /* start phase */
    p->lps = (double)*(p->ilps);                           /* loop start */
    p->lpe = (double)*(p->ilpe);                           /* loop end */
    p->loop_mode = (int32_t) (*(p->imode) + FL(0.5)) & 0x03;   /* loop mode */
    if (p->lpe <= p->lps) p->loop_mode = 0;                /* disable loop */
    p->dir = 1;                                            /* direction */
    return OK;
}

/* -------- lphasor performance -------- */

int32_t lphasor(CSOUND *csound, LPHASOR *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t loop_mode, dir;
    MYFLT   *ar, *xtrns;
    double  trns, phs, lps, lpe, lpt;
    int32_t     assxtr = IS_ASIG_ARG(p->xtrns);

    /* copy object data to local variables */
    ar = p->ar; xtrns = p->xtrns;
    phs = p->phs; lps = p->lps; lpe = p->lpe;
    lpt = lpe - lps;
    loop_mode = p->loop_mode;
    trns = (double)*xtrns;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (assxtr) trns = (double)xtrns[n];
      ar[n] = (MYFLT) phs;
      phs += (p->dir ? trns : -trns);
      if (loop_mode) {
        dir = (trns < 0.0 ? !(p->dir) : p->dir);
        if (dir && (phs >= lpe)) {
          phs += lpt * (double)((int32_t)((lps - phs) / lpt));
          if (loop_mode & 2) {
            phs = lps + lpe - phs;  /* reverse direction */
            p->dir = !(p->dir);
          }
        }
        else if (!dir && (phs <= lps)) {
          phs += lpt * (double)((int32_t)((lpe - phs) / lpt));
          if (loop_mode & 1) {
            phs = lps + lpe - phs;  /* reverse direction */
            p->dir = !(p->dir);
          }
        }
      }
    }
    /* store phase */
    p->phs = phs;
    return OK;
}

/* -------- tablexkt set-up -------- */

int32_t tablexkt_set(CSOUND *csound, TABLEXKT *p)
{
   IGN(csound);
    p->wsize = (int32_t)(*(p->iwsize) + 0.5);                  /* window size */
    if (UNLIKELY(p->wsize < 3)) {
      p->wsize = 2;
    }
    else {
      p->wsize = ((p->wsize + 2) >> 2) << 2;        /* round to nearest */
      if (p->wsize > 1024) p->wsize = 1024;         /* integer multiply of 4 */
    }
    /* constant for window calculation */
    p->win_fact = (FL(1.0) - POWER(p->wsize * FL(0.85172), -FL(0.89624)))
                   / ((MYFLT)((p->wsize * p->wsize) >> 2));

    p->ndx_scl = (*(p->ixmode) == FL(0.0) ? 0 : 1);         /* index mode */
    p->wrap_ndx = (*(p->iwrap) == FL(0.0) ? 0 : 1);         /* wrap index */
    /* use raw index values without scale / offset */
    if ((*(p->ixoff) != FL(0.0)) || p->ndx_scl) p->raw_ndx = 0;
    else p->raw_ndx = 1;
    return OK;
}

/* -------- tablexkt opcode -------- */

int32_t tablexkt(CSOUND *csound, TABLEXKT *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t i, wsize, wsized2, wrap_ndx, warp;
    double  ndx, d, x, c, v, flen_d, onedpi_d, pidwarp_d;
    int32_t ndx_i=0, flen;
    MYFLT   *ar, *xndx, ndx_f, a0, a1, a2, a3, v0, v1, v2, v3, *ftable;
    MYFLT   onedwarp, win_fact;
    FUNC    *ftp;
    int32_t asgx = IS_ASIG_ARG(p->xndx);

    /* window size */
    wsize = p->wsize;
    if (UNLIKELY((wsize < 2) || (wsize > 1024))) {
      return csound->PerfError(csound, &(p->h),
                               Str("tablexkt: not initialised"));
    }
    wsized2 = wsize >> 1;

    /* check ftable */
    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->kfn)) == NULL))
      return NOTOK;     /* invalid table */
    if (UNLIKELY((ftable = ftp->ftable) == NULL)) return NOTOK;
    flen = ftp->flen;               /* table length */
    flen_d = (double)flen;

    /* copy object data to local variables */
    ar = p->ar;
    xndx = p->xndx;
    wrap_ndx = p->wrap_ndx;
    if ((wsize > 4) && UNLIKELY((*(p->kwarp) > FL(1.001)))) {
      warp = 1;                     /* enable warp */
      onedwarp = FL(1.0) / *(p->kwarp);
      pidwarp_d = PI / (double)*(p->kwarp);
      /* correct window for kwarp */
      x = v = (double)wsized2; x *= x; x = 1.0 / x;
      v *= (double)onedwarp; v -= (double)((int32_t)v) + 0.5; v *= 4.0 * v;
      win_fact = (MYFLT)(((double)p->win_fact - x) * v + x);
    }
    else {
      warp = 0; onedwarp = FL(0.0); pidwarp_d = 0.0;
      win_fact = p->win_fact;
    }
    onedpi_d = 1.0 / PI;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      ndx = (double)*xndx;
      if (asgx) xndx++;
      /* calculate table index */
      if (!(p->raw_ndx)) {
        ndx += (double)*(p->ixoff);
        if (p->ndx_scl) ndx *= flen_d;
      }
      /* integer and fractional part of table index */
      ndx_i = (int32_t)ndx;
      ndx_f = (MYFLT) (ndx - (double)ndx_i);
      if (ndx_f < FL(0.0)) {
        ndx_f++; ndx_i--;
       }
      /* wrap or limit to allowed range */
      if (wrap_ndx) {
        while (ndx_i >= flen) ndx_i -= flen;
        while (ndx_i < 0) ndx_i += flen;
      }
      else {                        /* limit */
        if (UNLIKELY(ndx_i < 0)) {
          ndx_i = 0; ndx_f = FL(0.0);
        }
        else if (UNLIKELY(ndx_i >= flen)) {
          ndx_i = flen; ndx_f = FL(0.0);
        }
      }
      switch (wsize) {
        case 2:                     /* ---- linear interpolation ---- */
          ar[n] = ftable[ndx_i];
          if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
          ar[n] += (ftable[ndx_i] - ar[n]) * ndx_f;
          break;
        case 4:                     /* ---- cubic interpolation ---- */
          /* sample  0 */
          v1 = ftable[ndx_i];
          /* sample -1 */
          if (ndx_i) {
            v0 = ftable[ndx_i - 1L];
          } else {
            v0 = ftable[(wrap_ndx ? flen - 1L : 0L)];
          }
          /* sample +1 */
          if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
          v2 = ftable[ndx_i];
          /* sample +2 */
          if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
          v3 = ftable[ndx_i];
          a3 = ndx_f * ndx_f; a3--; a3 *= FL(0.1666666667);
          a2 = ndx_f; a2++; a0 = (a2 *= FL(0.5)); a0--;
          a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= ndx_f;
          ar[n] = (a0 * v0 + a1 * v1 + a2 * v2 + a3 * v3) * ndx_f + v1;
          break;
        default:                    /* ---- sinc interpolation ---- */
          ar[n] = FL(0.0);        /* clear output */
          ndx = (double)ndx_f;
          ndx_i += (int32_t)(1 - wsized2);
          d = (double)(1 - wsized2) - ndx;
          if (warp) {           /* ---- warp enabled ---- */
            init_sine_gen(onedpi_d, pidwarp_d, pidwarp_d * d, &x, &c, &v);
            /* samples -(window size / 2 - 1) to -1 */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
              a1 = a1 * a1 / (MYFLT) d;
              ar[n] += ftable[(ndx_i < 0L ? (wrap_ndx ? ndx_i + flen : 0L)
                                          : ndx_i)] * (MYFLT) x * a1;
              ndx_i++;
              d++; v += c * x; x += v;
            } while (--i);
            /* sample 0 */
            /* avoid division by zero */
            if (UNLIKELY(ndx < 0.00003)) ar[n] += onedwarp * ftable[ndx_i];
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
              a1 = a1 * a1 / (MYFLT) d;
              ar[n] += (MYFLT) x * a1 * ftable[ndx_i];
            }
            d++; v += c * x; x += v;
            if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
            /* sample 1 */
            /* avoid division by zero */
            if (ndx > 0.99997) ar[n] += onedwarp * ftable[ndx_i];
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
              a1 = a1 * a1 / (MYFLT) d;
              ar[n] += (MYFLT) x * a1 * ftable[ndx_i];
            }
            d++; v += c * x; x += v;
            if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
            /* samples 2 to (window size / 2) */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
              a1 = a1 * a1 / (MYFLT) d;
              ar[n] += (MYFLT) x * a1 * ftable[ndx_i];
              d++; v += c * x; x += v;
              if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
            } while (--i);
          }
          else {                /* ---- warp disabled ---- */
            /* avoid division by zero */
            if (UNLIKELY(ndx < 0.00001)) {
              ndx_i += (int32_t) (wsized2 - 1);    /* no need to check here */
              ar[n] = ftable[ndx_i];
            }
            else if (ndx > 0.99999) {
              ndx_i += (int32_t) wsized2;          /* does need range checking */
              if (ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
              ar[n] = ftable[ndx_i];
            }
            else {
              /* samples -(window size / 2 - 1) to 0 */
              i = wsized2 >> 1;
              do {
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
                a1 = a1 * a1 / (MYFLT) d;
                ar[n] += ftable[(ndx_i < 0L ? (wrap_ndx ? ndx_i + flen : 0L)
                                            : ndx_i)] * a1;
                d+=1.0; ndx_i++;
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
                a1 = a1 * a1 / (MYFLT) d;
                ar[n] -= ftable[(ndx_i < 0L ? (wrap_ndx ? ndx_i + flen : 0L)
                                            : ndx_i)] * a1;
                d+=1.0; ndx_i++;

              } while (--i);
              /* samples 1 to (window size / 2) */
              i = wsized2 >> 1;
              do {
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
                a1 = a1 * a1 / (MYFLT) d;
                ar[n] += a1 * ftable[ndx_i];
                d+=1.0;
                if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * win_fact;
                a1 = a1 * a1 / (MYFLT) d;
                ar[n] -=  a1 * ftable[ndx_i];
                d+=1.0;
                if (++ndx_i >= flen) ndx_i = (wrap_ndx ? ndx_i - flen : flen);
              } while (--i);
              ar[n] *= SIN(PI_F * ndx) / PI_F;
            }
          }
          break;
      }
    }
    return OK;
}
