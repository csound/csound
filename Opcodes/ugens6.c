/*
    ugens6.c:

    Copyright (C) 1991-2000 Barry Vercoe, John ffitch, Jens Groh, Hans Mikelson, Istvan Varga

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

#include "cs.h" /*                                      UGENS6.C        */
#include "ugens6.h"
#include <math.h>

#define log001 (-FL(6.9078))    /* log(.001) */

int downset(ENVIRON *csound, DOWNSAMP *p)
{
    if ((p->len = (int)*p->ilen) > ksmps)
      return initerror(Str("ilen > ksmps"));
    return OK;
}

int downsamp(ENVIRON *csound, DOWNSAMP *p)
{
    MYFLT       *asig, sum;
    int len;

    if (p->len <= 1)
      *p->kr = *p->asig;
    else {
      asig = p->asig;
      sum = FL(0.0);
      len = p->len;
      do {
        sum += *asig++;
      } while (--len);
      *p->kr = sum / p->len;
    }
    return OK;
}

int upsamp(ENVIRON *csound, UPSAMP *p)
{
    MYFLT       *ar, kval;
    int nsmps = ksmps;

    ar = p->ar;
    kval = *p->ksig;
    do {
      *ar++ = kval;
    } while (--nsmps);
    return OK;
}

int a_k_set(ENVIRON *csound, INTERP *p)
{
    p->prev = FL(0.0);
    p->init_k = 0;              /* IV - Sep 5 2002 */
    return OK;
}

int interpset(ENVIRON *csound, INTERP *p)
{
    if (*p->istor == FL(0.0)) {
      p->prev = FL(0.0);
      p->init_k = (*p->imode == FL(0.0) ? 0 : 1);       /* IV - Sep 5 2002 */
    }
    return OK;
}

int interp(ENVIRON *csound, INTERP *p)
{
    MYFLT       *ar, val, incr;
    int nsmps = ksmps;

    ar = p->rslt;
    if (p->init_k) {            /* IV - Sep 5 2002 */
      p->init_k = 0;
      p->prev = *p->xsig;
    }
    val = p->prev;
    incr = (*p->xsig - val) / ensmps;
    do {
      *ar++ = val += incr;
    } while (--nsmps);
    p->prev = val;
    return OK;
}

int indfset(ENVIRON *csound, INDIFF *p)
{
    if (*p->istor == FL(0.0))   /* IV - Sep 5 2002 */
      p->prev = FL(0.0);
    return OK;
}

int kntegrate(ENVIRON *csound, INDIFF *p)
{
    *p->rslt = p->prev += *p->xsig;
    return OK;
}

int integrate(ENVIRON *csound, INDIFF *p)
{
    MYFLT       *rslt, *asig, sum;
    int nsmps = ksmps;

    rslt = p->rslt;
    asig = p->xsig;
    sum = p->prev;
    do {
      *rslt++ = sum += *asig++;
    } while (--nsmps);
    p->prev = sum;
    return OK;
}

int kdiff(ENVIRON *csound, INDIFF *p)
{
    MYFLT       tmp;
    tmp = *p->xsig;             /* IV - Sep 5 2002: fix to make */
    *p->rslt = tmp - p->prev;   /* diff work when the input and */
    p->prev = tmp;              /* output argument is the same  */
    return OK;
}

int diff(ENVIRON *csound, INDIFF *p)
{
    MYFLT       *ar, *asig, prev, tmp;
    int nsmps = ksmps;

    ar = p->rslt;
    asig = p->xsig;
    prev = p->prev;
    do {
      tmp = *asig++;            /* IV - Sep 5 2002: fix to make */
      *ar++ = tmp - prev;       /* diff work when the input and */
      prev = tmp;               /* output argument is the same  */
    } while (--nsmps);
    p->prev = prev;
    return OK;
}

int samphset(ENVIRON *csound, SAMPHOLD *p)
{
    if (!(*p->istor))
      p->state = *p->ival;
    p->audiogate = (p->XINCODE & 01) ? 1 : 0;
    return OK;
}

int ksmphold(ENVIRON *csound, SAMPHOLD *p)
{
    if (*p->xgate > 0.)
      p->state = *p->xsig;
    *p->xr = p->state;
    return OK;
}

int samphold(ENVIRON *csound, SAMPHOLD *p)
{
    MYFLT       *ar, *asig, *agate, state;
    int nsmps = ksmps;

    ar = p->xr;
    asig = p->xsig;
    state = p->state;
    if (p->audiogate) {
      agate = p->xgate;
      do {
        if (*agate++ > FL(0.0))
          state = *asig;
        *ar++ = state;
        asig++;
      }
      while (--nsmps);
    }
    else {
      if (*p->xgate > FL(0.0)) {
        do {
          *ar++ = state = *asig++;
        } while (--nsmps);
      }
      else {
        do {
          *ar++ = state;
        } while (--nsmps);
      }
    }
    p->state = state;
    return OK;
}

int delset(ENVIRON *csound, DELAY *p)
{
    long        npts;
    char        *auxp;

    if (*p->istor && p->auxch.auxp != NULL)
      return OK;
    if ((npts = (long)(*p->idlt * esr)) <= 0) {
      return initerror(Str("illegal delay time"));
    }
    if ((auxp = p->auxch.auxp) == NULL ||
        npts != p->npts) { /* new space if reqd */
      auxalloc((long)npts*sizeof(MYFLT), &p->auxch);
      auxp = p->auxch.auxp;
      p->npts = npts;
    }
    else if (!(*p->istor)) {                    /* else if requested */
      long *lp = (long *)auxp;
      do {
        *lp++ = 0;                            /*   clr old to zero */
      } while (--npts);
    }
    p->curp = (MYFLT *) auxp;
    return OK;
}

/* fifo for delayr pointers by Jens Groh: */
static   DELAYR   *first_delayr = NULL;   /* fifo anchor */
static   DELAYR   *last_delayr = NULL;   /* fifo anchor */

int delrset(ENVIRON *csound, DELAYR *p)
{
    long        npts;
    char        *auxp;

    /* fifo for delayr pointers by Jens Groh: */
    /* append structadr for delayw to fifo: */
    if (first_delayr != NULL)    /* fifo not empty */
      last_delayr->next_delayr = p;
    else    /* fifo empty */
      first_delayr = p;
    last_delayr = p;

    if (*p->istor && p->auxch.auxp != NULL)
      return OK;
    if ((npts = (long)(*p->idlt * esr)) < ksmps) {      /* ksmps is min dely */
      return initerror(Str("illegal delay time"));
    }
    if ((auxp = p->auxch.auxp) == NULL ||       /* new space if reqd */
        npts != p->npts) {
      auxalloc((long)npts*sizeof(MYFLT), &p->auxch);
      auxp = p->auxch.auxp;
      p->npts = npts;
    }
    else if (!(*p->istor)) {                    /* else if requested */
      long *lp = (long *)auxp;
      do {
        *lp++ = 0;                            /*   clr old to zero */
      } while (--npts);
    }
    p->curp = (MYFLT *) auxp;
    return OK;
}

int delwset(ENVIRON *csound, DELAYW *p)
{
   /* fifo for delayr pointers by Jens Groh: */
    if (first_delayr == NULL) {
      return initerror(Str("delayw: associated delayr not found"));
   }
    p->delayr = first_delayr;     /* adr delayr struct */
    /* remove structadr from fifo */
    if (last_delayr == first_delayr) {   /* fifo will be empty */
      first_delayr = NULL;
    }
    else    /* fifo will not be empty */
      first_delayr = first_delayr->next_delayr;
    return OK;
}

int tapset(ENVIRON *csound, DELTAP *p)
{
    /* fifo for delayr pointers by Jens Groh: */
    if (last_delayr == NULL) {
      return initerror(Str("deltap: associated delayr not found"));
    }
    p->delayr = last_delayr;      /* adr delayr struct */
    return OK;
}

int delay(ENVIRON *csound, DELAY *p)
{
    MYFLT       *ar, *asig, *curp, *endp;
    int nsmps = ksmps;

    if (p->auxch.auxp==NULL) {  /* RWD fix */
      return perferror(Str("delay: not initialised"));
    }
    ar = p->ar;
    asig = p->asig;
    curp = p->curp;
    endp = (MYFLT *) p->auxch.endp;
    do {
      MYFLT in = *asig++;       /* Allow overwriting form */
      *ar++ = *curp;
      *curp = in;
      if (++curp >= endp)
        curp = (MYFLT *) p->auxch.auxp;
    } while (--nsmps);
    p->curp = curp;             /* sav the new curp */
    return OK;
}

int delayr(ENVIRON *csound, DELAYR *p)
{
    MYFLT       *ar, *curp, *endp;
    int nsmps = ksmps;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("delayr: not initialised"));
    }
    ar = p->ar;
    curp = p->curp;
    endp = (MYFLT *) p->auxch.endp;
    do {
      *ar++ = *curp++;
      if (curp >= endp)
        curp = (MYFLT *) p->auxch.auxp;
    } while (--nsmps);
    return OK;
}                                                       /* leave old curp */

int delayw(ENVIRON *csound, DELAYW *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *asig, *curp, *endp;
    int nsmps = ksmps;

    if (q->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("delayw: not initialised"));
    }
    asig = p->asig;
    curp = q->curp;
    endp = (MYFLT *) q->auxch.endp;
    do {
      *curp = *asig++;
      if (++curp >= endp)
        curp = (MYFLT *) q->auxch.auxp;
    } while (--nsmps);
    q->curp = curp;                                     /* now sav new curp */
    return OK;
}

int deltap(ENVIRON *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *endp;
    int nsmps = ksmps;

    if (q->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("deltap: not initialised"));
    }
    ar = p->ar;
    tap = q->curp - (long)(*p->xdlt * esr);
    while (tap < (MYFLT *) q->auxch.auxp)
      tap += q->npts;
    endp = (MYFLT *) q->auxch.endp;
    do {
      if (tap >= endp)
        tap -= q->npts;
      *ar++ = *tap++;
    } while (--nsmps);
    return OK;
}

int deltapi(ENVIRON *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *prv, *begp, *endp;
    int nsmps = ksmps;
    long        idelsmps;
    MYFLT       delsmps, delfrac;

    if (q->auxch.auxp==NULL) {
      return perferror(Str("deltapi: not initialised"));
    }
    ar = p->ar;
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!p->XINCODE) {
      delsmps = *p->xdlt * esr;
      idelsmps = (long)delsmps;
      delfrac = delsmps - idelsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      do {
        if (tap >= endp)
          tap -= q->npts;
        if ((prv = tap - 1) < begp)
          prv += q->npts;
        *ar++ = *tap + (*prv - *tap) * delfrac;
        tap++;
      } while (--nsmps);
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      do {
        delsmps = *timp++ * esr;
        idelsmps = (long)delsmps;
        delfrac = delsmps - idelsmps;
        tap = curq++ - idelsmps;
        if (tap < begp) tap += q->npts;
        else if (tap >= endp)
          tap -= q->npts;
        if ((prv = tap - 1) < begp)
          prv += q->npts;
        *ar++ = *tap + (*prv - *tap) * delfrac;
      } while (--nsmps);
    }
    return OK;
}

/* ***** From Hans Mikelson ************* */
/* Delay N samples */
int deltapn(ENVIRON *csound, DELTAP *p)
{
    DELAYR *q = p->delayr;
    MYFLT *ar, *tap, *begp, *endp;
    int nsmps = ksmps;
    long idelsmps;
    MYFLT delsmps;

    if (q->auxch.auxp==NULL) {
      return perferror(Str("deltapn: not initialised"));
    }
    ar = p->ar;
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!p->XINCODE) {
      delsmps = *p->xdlt;
      idelsmps = (long)delsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      do {
        if (tap >= endp )
          tap -= q->npts;
        if (tap < begp)
          tap += q->npts;
        *ar++ = *tap;
        tap++;
      } while (--nsmps);
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      do {
        delsmps = *timp++;
        idelsmps = (long)delsmps;
        if ((tap = curq++ - idelsmps) < begp)
          tap += q->npts;
        else if (tap >= endp)
          tap -= q->npts;
        *ar++ = *tap;
      } while (--nsmps);
    }
    return OK;
}

/* **** JPff **** */
int deltap3(ENVIRON *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *prv, *begp, *endp;
    int nsmps = ksmps;
    long        idelsmps;
    MYFLT       delsmps, delfrac;

    if (q->auxch.auxp==NULL) {
      return perferror(Str("deltap3: not initialised"));
    }
    ar = p->ar;
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!p->XINCODE) {
      delsmps = *p->xdlt * esr;
      idelsmps = (long)delsmps;
      delfrac = delsmps - idelsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      do {
        MYFLT ym1, y0, y1, y2;
        if (tap >= endp)
          tap -= q->npts;
        if ((prv = tap - 1) < begp)
          prv += q->npts;
        if (prv - 1 < begp) y2 = *(prv-1+q->npts);
        else                y2 = *(prv-1);
        if (tap + 1 >= endp) ym1 = *(tap+1-q->npts);
        else                ym1 = *(tap+1);
        y0 = *tap; y1 = *prv;
        {
          MYFLT w, x, y, z;
          z = delfrac * delfrac; z--; z *= FL(0.1666666667);
          y = delfrac; y++; w = (y *= FL(0.5)); w--;
          x = FL(3.0) * z; y -= x; w -= z; x -= delfrac;
          *(ar++) = (w*ym1 + x*y0 + y*y1 + z*y2) * delfrac + y0;
        }
        tap++;
      } while (--nsmps);
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      do {
        MYFLT ym1, y0, y1, y2;
        delsmps = *timp++ * esr;
        idelsmps = (long)delsmps;
        delfrac = delsmps - idelsmps;
        if ((tap = curq++ - idelsmps) < begp)
          tap += q->npts;
        else if (tap >= endp)
          tap -= q->npts;
        if ((prv = tap - 1) < begp)
          prv += q->npts;
        if (prv - 1 < begp) y2 = *(prv-1+q->npts);
        else                y2 = *(prv-1);
        if (tap + 1 >= endp) ym1 = *(tap+1-q->npts);
        else                ym1 = *(tap+1);
        y0 = *tap; y1 = *prv;
        {
          MYFLT w, x, y, z;
          z = delfrac * delfrac; z--; z *= FL(0.1666666667);
          y = delfrac; y++; w = (y *= FL(0.5)); w--;
          x = FL(3.0) * z; y -= x; w -= z; x -= delfrac;
          *(ar++) = (w*ym1 + x*y0 + y*y1 + z*y2) * delfrac + y0;
        }
      } while (--nsmps);
    }
    return OK;
}

/* deltapx and deltapxw opcodes by Istvan Varga */

int tapxset(ENVIRON *csound, DELTAPX *p)
{
    /* fifo for delayr pointers by Jens Groh: */
    if (last_delayr == NULL) {
      return initerror(Str("deltap: associated delayr not found"));
    }
    p->wsize = (int) (*(p->iwsize) + FL(0.5));          /* window size */
    p->wsize = ((p->wsize + 2) >> 2) << 2;
    if (p->wsize < 4) p->wsize = 4;
    if (p->wsize > 1024) p->wsize = 1024;
    p->delayr = last_delayr;      /* adr delayr struct */
    return OK;
}

int deltapx(ENVIRON *csound, DELTAPX *p)                /* deltapx opcode */
{
    DELAYR      *q = p->delayr;
    MYFLT       *out1, *del, *buf1;
    int         nn = ksmps, i2, i;
    double      x1, x2, w, d, d2x, n1;
    long        indx, maxd, xpos;

    if (q->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("deltap: not initialised"));
    }
    out1 = p->ar; del = p->adlt;
    buf1 = (MYFLT *) q->auxch.auxp;
    indx = (long) (q->curp - buf1); maxd = q->npts;

    if (p->wsize != 4) {                /* window size >= 8 */
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = 1.0 - pow ((double) p->wsize * 0.85172, -0.89624);
      d2x /= (double) (i2*i2);
      while (nn--) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (double) indx - (double) *(del++) * (double) esr;
        while (x1 < 0.0) x1 += (double) maxd;
        xpos = (long) x1; x1 -= (double) xpos;
        x2 = sin (PI * x1) / PI;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 * (1.0 - x1) > 0.00000001) {
          n1 = 0.0;
          xpos += (1 - i2);
          while (xpos < 0) xpos += maxd;
          d = (double) (1 - i2) - x1;
          for (i = i2; i--;) {
            w = 1.0 - d*d*d2x; w *= (w / d++);
            n1 += (double) buf1[xpos] * w;
            if (++xpos >= maxd) xpos -= maxd;
            w = 1.0 - d*d*d2x; w *= (w / d++);
            n1 -= (double) buf1[xpos] * w;
            if (++xpos >= maxd) xpos -= maxd;
          }
          *out1 = (MYFLT) (n1 * x2);
        } else {                                        /* integer sample */
          xpos = (long) ((double) xpos + x1 + 0.5);     /* position */
          if (xpos >= maxd) xpos -= maxd;
          *out1 = buf1[xpos];
        }
        out1++; indx++;
      }
    } else {                    /* window size = 4, cubic interpolation */
      while (nn--) {
        x1 = (double) indx - (double) *(del++) * (double) esr;
        while (x1 < 0.0) x1 += (double) maxd;
        xpos = (long) x1; x1 -= (double) xpos;
        w = x1 * x1; d2x = FL(0.16666667) * (x1 * w - x1);      /* sample +2 */
        d = FL(0.5) * (w + x1) - FL(3.0) * d2x;                 /* sample +1 */
        x1 = FL(0.5) * (w - x1) - d2x;                          /* sample -1 */
        w = FL(3.0) * d2x - w; w++;                             /* sample 0  */

        if (--xpos < 0) xpos += maxd;
        while (xpos >= maxd) xpos -= maxd;
        n1 = x1 * (double) buf1[xpos]; if (++xpos >= maxd) xpos = 0;
        n1 += w * (double) buf1[xpos]; if (++xpos >= maxd) xpos = 0;
        n1 += d * (double) buf1[xpos]; if (++xpos >= maxd) xpos = 0;
        n1 += d2x * (double) buf1[xpos];

        *(out1++) = (MYFLT) n1; indx++;
      }
    }
    return OK;
}

int deltapxw(ENVIRON *csound, DELTAPX *p)               /* deltapxw opcode */
{
    DELAYR      *q = p->delayr;
    MYFLT       *in1, *del, *buf1;
    int         nn = ksmps, i2, i;
    double      x1, x2, w, d, d2x, n1;
    long        indx, maxd, xpos;

    if (q->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("deltap: not initialised"));
    }
    in1 = p->ar; del = p->adlt;
    buf1 = (MYFLT *) q->auxch.auxp;
    indx = (long) (q->curp - buf1); maxd = q->npts;

    if (p->wsize != 4) {                /* window size >= 8 */
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = 1.0 - pow ((double) p->wsize * 0.85172, -0.89624);
      d2x /= (double) (i2*i2);
      while (nn--) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (double) indx - (double) *(del++) * (double) esr;
        while (x1 < 0.0) x1 += (double) maxd;
        xpos = (long) x1; x1 -= (double) xpos;
        x2 = sin (PI * x1) / PI;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 * (1.0 - x1) > 0.00000001) {
          n1 = (double) *in1 * x2;
          xpos += (1 - i2);
          while (xpos < 0) xpos += maxd;
          d = (double) (1 - i2) - x1;
          for (i = i2; i--;) {
            w = 1.0 - d*d*d2x; w *= (w / d++);
            buf1[xpos] += (MYFLT) (n1 * w);
            if (++xpos >= maxd) xpos -= maxd;
            w = 1.0 - d*d*d2x; w *= (w / d++);
            buf1[xpos] -= (MYFLT) (n1 * w);
            if (++xpos >= maxd) xpos -= maxd;
          }
        } else {                                        /* integer sample */
          xpos = (long) ((double) xpos + x1 + 0.5);     /* position */
          if (xpos >= maxd) xpos -= maxd;
          buf1[xpos] += *in1;
        }
        in1++; indx++;
      }
    }
    else {                    /* window size = 4, cubic interpolation */
      while (nn--) {
        x1 = (double) indx - (double) *(del++) * (double) esr;
        while (x1 < 0.0) x1 += (double) maxd;
        xpos = (long) x1; x1 -= (double) xpos;
        w = x1 * x1; d2x = FL(0.16666667) * (x1 * w - x1);      /* sample +2 */
        d = FL(0.5) * (w + x1) - FL(3.0) * d2x;                 /* sample +1 */
        x1 = FL(0.5) * (w - x1) - d2x;                          /* sample -1 */
        w = FL(3.0) * d2x - w; w++;                             /* sample 0  */

        n1 = (double) *(in1++);
        if (--xpos < 0) xpos += maxd;
        while (xpos >= maxd) xpos -= maxd;
        buf1[xpos] += (MYFLT) (x1 * n1); if (++xpos >= maxd) xpos = 0;
        buf1[xpos] += (MYFLT) (w * n1); if (++xpos >= maxd) xpos = 0;
        buf1[xpos] += (MYFLT) (d * n1); if (++xpos >= maxd) xpos = 0;
        buf1[xpos] += (MYFLT) (d2x * n1);

        indx++;
      }
    }
    return OK;
}

int del1set(ENVIRON *csound, DELAY1 *p)
{
    if (!(*p->istor))
      p->sav1 = FL(0.0);
    return OK;
}

int delay1(ENVIRON *csound, DELAY1 *p)
{
    MYFLT       *ar, *asig;
    int nsmps = ksmps - 1;

    ar = p->ar;
    asig = p->asig;
    *ar++ = p->sav1;
    if (nsmps) {
      do {
        *ar++ = *asig++;
      } while (--nsmps);
    }
    p->sav1 = *asig;
    return OK;
}


int cmbset(ENVIRON *csound, COMB *p)
{
    long        lpsiz, nbytes;

    if (*p->insmps != 0) {
      if ((lpsiz = (long)(FL(0.5)+*p->ilpt)) <= 0) {
        return initerror(Str("illegal loop time"));
      }
    }
    else if ((lpsiz = (long)(*p->ilpt * esr)) <= 0) {
      return initerror(Str("illegal loop time"));
    }
    nbytes = lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || nbytes != p->auxch.size) {
      auxalloc((long)nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      p->prvt = FL(0.0);
      p->coef = FL(0.0);
    }
    else if (!(*p->istor)) {
      long *fp = (long *) p->auxch.auxp;
      p->pntr = (MYFLT *) fp;
      do  *fp++ = 0;
      while (--lpsiz);
      p->prvt = FL(0.0);
      p->coef = FL(0.0);
    }
    return OK;
}

int comb(ENVIRON *csound, COMB *p)
{
    int nsmps = ksmps;
    MYFLT       *ar, *asig, *xp, *endp;
    MYFLT       coef = p->coef;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("comb: not initialised"));
    }
    if (p->prvt != *p->krvt) {
#ifdef __alpha__
      /*
       * The argument to exp() in the following is sometimes a small
       * enough negative number to result in a denormal (or worse)
       * on Alpha. So if the result would be less than 1.0e-16, we
       * just say it's zero and don't call exp().  heh 981101
       */
      double exp_arg = (double)(log001 * *p->ilpt / *p->krvt);
      if (exp_arg < -36.8413615)    /* ln(1.0e-16) */
        coef = p->coef = FL(0.0);
      else
        coef = p->coef = (MYFLT)exp(exp_arg);
#else
      coef = p->coef = (MYFLT)exp((double)(log001 * *p->ilpt / *p->krvt));
#endif
      p->prvt = *p->krvt;
    }
    xp = p->pntr;
    endp = (MYFLT *) p->auxch.endp;
    ar = p->ar;
    asig = p->asig;
    do {
      *ar++ = *xp;
      *xp *= coef;
      *xp += *asig++;
      if (++xp >= endp)
        xp = (MYFLT *) p->auxch.auxp;
    } while (--nsmps);
    p->pntr = xp;
    return OK;
}

int alpass(ENVIRON *csound, COMB *p)
{
    int nsmps = ksmps;
    MYFLT       *ar, *asig, *xp, *endp;
    MYFLT       y, z;
    MYFLT       coef = p->coef;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("alpass: not initialised"));
    }
    if (p->prvt != *p->krvt) {
      coef = p->coef = (MYFLT)exp((double)(log001 * *p->ilpt / *p->krvt));
      p->prvt = *p->krvt;
    }
    xp = p->pntr;
    endp = (MYFLT *) p->auxch.endp;
    ar = p->ar;
    asig = p->asig;
    do {
      y = *xp;
      *xp++ = z = coef * y + *asig++;
      *ar++ = y - coef * z;
      if (xp >= endp)
        xp = (MYFLT *) p->auxch.auxp;
    } while (--nsmps);
    p->pntr = xp;
    return OK;
}

static  MYFLT   revlptimes[6] ={FL(0.0297), FL(0.0371), FL(0.0411),
                                FL(0.0437), FL(0.005), FL(0.0017)};
static  long    revlpsiz[6];
static  long    revlpsum;

void reverbinit(void)                   /* called once by oload */
{                                       /*  to init reverb data */
    MYFLT       *lptimp = revlptimes;
    long        *lpsizp = revlpsiz;
    int n = 6;

    revlpsum = 0;
    do {
      *lpsizp = (long)(*lptimp++ * esr);
      revlpsum += *lpsizp++;
    } while (--n);
}

int rvbset(ENVIRON *csound, REVERB *p)
{
    if (p->auxch.auxp == NULL) {                        /* if no space yet, */
      long      *sizp = revlpsiz;
      auxalloc(revlpsum*sizeof(MYFLT),&p->auxch);       /*    allocate it   */
      p->adr1 = p->p1 = (MYFLT *) p->auxch.auxp;
      p->adr2 = p->p2 = p->adr1 + *sizp++;
      p->adr3 = p->p3 = p->adr2 + *sizp++;              /*    & init ptrs   */
      p->adr4 = p->p4 = p->adr3 + *sizp++;
      p->adr5 = p->p5 = p->adr4 + *sizp++;
      p->adr6 = p->p6 = p->adr5 + *sizp++;
      if (p->adr6 + *sizp != (MYFLT *) p->auxch.endp) {
        printf(Str("revlpsiz inconsistent\n"));
        longjmp(pcglob->exitjmp,1);
      }
      p->prvt = FL(0.0);
    }
    else if (!(*p->istor)) {                    /* else if istor = 0 */
      MYFLT     *fp = p->adr1;
      long      nn = revlpsum;
      do  *fp++ = FL(0.0);                      /*  clr existing spc */
      while (--nn);
      p->p1 = p->adr1;                          /*  and reset   */
      p->p2 = p->adr2;
      p->p3 = p->adr3;
      p->p4 = p->adr4;
      p->p5 = p->adr5;
      p->p6 = p->adr6;
      p->prvt = FL(0.0);
    }
    return OK;
}

int reverb(ENVIRON *csound, REVERB *p)
{
    MYFLT       *asig, *p1, *p2, *p3, *p4, *p5, *p6, *ar, *endp;
    int nsmps = ksmps;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return perferror(Str("reverb: not intialised"));
    }
    if (p->prvt != *p->krvt) {
      MYFLT     *lptimp = revlptimes;
      MYFLT     logdrvt = log001 / *p->krvt;
      p->c1 = (MYFLT)exp(logdrvt * *lptimp++);
      p->c2 = (MYFLT)exp(logdrvt * *lptimp++);
      p->c3 = (MYFLT)exp(logdrvt * *lptimp++);
      p->c4 = (MYFLT)exp(logdrvt * *lptimp++);
      p->c5 = (MYFLT)exp(logdrvt * *lptimp++);
      p->c6 = (MYFLT)exp(logdrvt * *lptimp++);
    }
    p1 = p->p1;
    p2 = p->p2;
    p3 = p->p3;
    p4 = p->p4;
    p5 = p->p5;
    p6 = p->p6;
    endp = (MYFLT *) p->auxch.endp;

    ar = p->ar;
    asig = p->asig;
    do {
      MYFLT     cmbsum, y1, y2, z;
      cmbsum = *p1 + *p2 + *p3 + *p4;
      *p1 = p->c1 * *p1 + *asig;
      *p2 = p->c2 * *p2 + *asig;
      *p3 = p->c3 * *p3 + *asig;
      *p4 = p->c4 * *p4 + *asig++;
      p1++; p2++; p3++; p4++;
      y1 = *p5;
      *p5++ = z = p->c5 * y1 + cmbsum;
      y1 -= p->c5 * z;
      y2 = *p6;
      *p6++ = z = p->c6 * y2 + y1;
      *ar++ = y2 - p->c6 * z;
      if (p1 >= p->adr2)        p1 = p->adr1;
      if (p2 >= p->adr3)        p2 = p->adr2;
      if (p3 >= p->adr4)        p3 = p->adr3;
      if (p4 >= p->adr5)        p4 = p->adr4;
      if (p5 >= p->adr6)        p5 = p->adr5;
      if (p6 >= endp)   p6 = p->adr6;
    } while (--nsmps);
    p->p1 = p1;
    p->p2 = p2;
    p->p3 = p3;
    p->p4 = p4;
    p->p5 = p5;
    p->p6 = p6;
    return OK;
}

int panset(ENVIRON *csound, PAN *p)
{
    FUNC *ftp;

    if ((ftp = ftfind(csound, p->ifn)) == NULL)
      return NOTOK;
    p->ftp = ftp;
    if (*p->imode)
      p->xmul = (MYFLT)ftp->flen;
    else        p->xmul = FL(1.0);
    if (*p->ioffset)
      p->xoff = ftp->flen >> 1;
    else        p->xoff = 0;
    return OK;
}

int pan(ENVIRON *csound, PAN *p)
{
    MYFLT       *r1, *r2, *r3, *r4, *sigp, ch1, ch2, ch3, ch4;
    long        xndx, yndx, flen;
    int nsmps = ksmps;

    FUNC        *ftp;

    ftp = p->ftp;
    if (ftp==NULL) {        /* RWD fix */
      return perferror(Str("pan: not initialised"));
    }
    flen = ftp->flen;
    xndx = (long)(*p->kx * p->xmul) - p->xoff;
    yndx = (long)(*p->ky * p->xmul) - p->xoff;
    if (xndx < 0L || xndx > flen || yndx < 0L || yndx > flen) {
      long xt, yt, off = flen >>1;
      xt = xndx - off;
      yt = yndx - off;
      if (xt*xt > yt*yt) {
        if (xt < 0) xt = -xt;
        yndx = yt * off / xt + off;
      }
      else {
        if (yt < 0) yt = -yt;
        xndx = xt * off / yt + off;
      }
      if (xndx < 0)             xndx = 0;
      else if (xndx > flen)     xndx = flen;
      if (yndx < 0)             yndx = 0;
      else if (yndx > flen)     yndx = flen;
    }
    ch2 = *(ftp->ftable + xndx) * *(ftp->ftable + yndx);
    ch4 = *(ftp->ftable + xndx) * *(ftp->ftable + flen - yndx);
    ch1 = *(ftp->ftable + flen - xndx) * *(ftp->ftable + yndx);
    ch3 = *(ftp->ftable + flen - xndx) * *(ftp->ftable + flen - yndx);
    r1 = p->r1;
    r2 = p->r2;
    r3 = p->r3;
    r4 = p->r4;
    sigp = p->asig;
    do {
      *r1++ = *sigp * ch1;
      *r2++ = *sigp * ch2;
      *r3++ = *sigp * ch3;
      *r4++ = *sigp * ch4;
      sigp++;
    } while (--nsmps);
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "downsamp",S(DOWNSAMP),3, "k", "ao",   (SUBR)downset,(SUBR)downsamp        },
{ "upsamp", S(UPSAMP),  4,  "a", "k",    NULL,   NULL,   (SUBR)upsamp        },
/* IV - Sep 5 2002 */
{ "interp", S(INTERP),  5,  "a", "koo",  (SUBR)interpset,NULL, (SUBR)interp  },
{ "a.k",    S(INTERP),  5,  "a", "k",    (SUBR)a_k_set,NULL,   (SUBR)interp  },
{ "integ", S(INDIFF), 7, "s", "xo", (SUBR)indfset,(SUBR)kntegrate,(SUBR)integrate},
{ "diff",   S(INDIFF),  7,  "s", "xo",   (SUBR)indfset,(SUBR)kdiff, (SUBR)diff },
{ "samphold",S(SAMPHOLD),7, "s", "xxoo", (SUBR)samphset,(SUBR)ksmphold,(SUBR)samphold},
{ "delay",  S(DELAY),   5,  "a", "aio",  (SUBR)delset, NULL,   (SUBR)delay   },
{ "delayr", S(DELAYR),  5,  "a", "io",   (SUBR)delrset,NULL,   (SUBR)delayr  },
{ "delayw", S(DELAYW),  5,  "",  "a",    (SUBR)delwset,NULL,   (SUBR)delayw  },
{ "delay1", S(DELAY1),  5,  "a", "ao",   (SUBR)del1set,NULL,   (SUBR)delay1  },
{ "deltap", S(DELTAP),  5,  "a", "k",    (SUBR)tapset, NULL,   (SUBR)deltap  },
{ "deltapi",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltapi },
{ "deltapn",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltapn },
{ "deltap3",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltap3 },
{ "deltapx", S(DELTAPX),5,  "a", "ai",   (SUBR)tapxset, NULL,  (SUBR)deltapx },
{ "deltapxw", S(DELTAPX),5, "",  "aai",  (SUBR)tapxset, NULL, (SUBR)deltapxw },
{ "pan",    S(PAN),   5, "aaaa", "akkioo",(SUBR)panset,NULL,   (SUBR)pan     },
{ "reverb", S(REVERB),  5, "a",  "ako",  (SUBR)rvbset, NULL,   (SUBR)reverb  },
};

LINKAGE
