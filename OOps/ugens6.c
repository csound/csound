/*
    ugens6.c:

    Copyright (C) 1991-2000 Barry Vercoe, John ffitch, Jens Groh,
                            Hans Mikelson, Istvan Varga

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

#include "csoundCore.h" /*                              UGENS6.C        */
#include "ugens6.h"
#include <math.h>

#define log001 (-FL(6.9078))    /* log(.001) */

int32_t downset(CSOUND *csound, DOWNSAMP *p)
{
    if (UNLIKELY((p->len = (uint32_t)*p->ilen) > CS_KSMPS))
      return csound->InitError(csound, "ilen > ksmps");
    return OK;
}

int32_t downsamp(CSOUND *csound, DOWNSAMP *p)
{
    IGN(csound);
    MYFLT       *asig, sum;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t len, n;

    if (p->len <= 1)
      *p->kr = p->asig[offset];
    else {
      asig = p->asig;
      sum = FL(0.0);
      len = p->len;
      if (len>(int32_t)(CS_KSMPS-early)) len = early;
      for (n=offset; n<len; n++) {
        sum += asig[n];
      }
      *p->kr = sum / p->len;
    }
    return OK;
}

int32_t upsamp(CSOUND *csound, UPSAMP *p)
{
    IGN(csound);
    MYFLT kval = *p->ksig;
    MYFLT *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++)
      ar[n] = kval;
    return OK;
}

int32_t a_k_set(CSOUND *csound, INTERP *p)
{
    IGN(csound);
    p->prev = FL(0.0);
    p->init_k = 0;              /* IV - Sep 5 2002 */
    return OK;
}

int32_t interpset(CSOUND *csound, INTERP *p)
{
    IGN(csound);
    if (*p->istor == FL(0.0)) {
      p->prev = (*p->imode == FL(0.0) ? *p->istart : FL(0.0));
      p->init_k = (*p->imode == FL(0.0) ? 0 : 1);       /* IV - Sep 5 2002 */
    }

    return OK;
}

int32_t interp(CSOUND *csound, INTERP *p)
{
    IGN(csound);
    MYFLT *ar, val, incr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->rslt;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->init_k) {
      p->init_k = 0;
      p->prev = *p->xsig;
    }
    val = p->prev;
    incr = (*p->xsig - val) / (nsmps-offset);
    for (n=offset; n<nsmps; n++) {
      ar[n] = val += incr;
    }
    p->prev = val;
    return OK;
}

int32_t indfset(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    if (*p->istor == FL(0.0))   /* IV - Sep 5 2002 */
      p->prev = FL(0.0);
    return OK;
}

int32_t kntegrate(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    *p->rslt = p->prev += *p->xsig;
    return OK;
}

int32_t integrate(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    MYFLT       *rslt, *asig, sum;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    rslt = p->rslt;
    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->xsig;
    sum = p->prev;
    for (n=offset; n<nsmps; n++) {
      rslt[n] = sum += asig[n];
    }
    p->prev = sum;
    return OK;
}

int32_t kdiff(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    MYFLT       tmp;
    tmp = *p->xsig;             /* IV - Sep 5 2002: fix to make */
    *p->rslt = tmp - p->prev;   /* diff work when the input and */
    p->prev = tmp;              /* output argument is the same  */
    return OK;
}

int32_t diff(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    MYFLT       *ar, *asig, prev, tmp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->rslt;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->xsig;
    prev = p->prev;
    for (n=offset; n<nsmps; n++) {
      tmp = asig[n];            /* IV - Sep 5 2002: fix to make */
      ar[n] = tmp - prev;       /* diff work when the input and */
      prev = tmp;               /* output argument is the same  */
    }
    p->prev = prev;
    return OK;
}

int32_t samphset(CSOUND *csound, SAMPHOLD *p)
{
    IGN(csound);
    if (!(*p->istor))
      p->state = *p->ival;
    p->audiogate = IS_ASIG_ARG(p->xgate) ? 1 : 0;
    return OK;
}

int32_t ksmphold(CSOUND *csound, SAMPHOLD *p)
{
    IGN(csound);
    if (*p->xgate > FL(0.0))
      p->state = *p->xsig;
    *p->xr = p->state;
    return OK;
}

int32_t samphold(CSOUND *csound, SAMPHOLD *p)
{
    IGN(csound);
    MYFLT       *ar, *asig, *agate, state;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->xr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->xsig;
    state = p->state;
    if (p->audiogate) {
      agate = p->xgate;
      for (n=offset; n<nsmps; n++) {
        if (agate[n] > FL(0.0))
          state = asig[n];
        ar[n] = state;
      }
    }
    else {
      if (*p->xgate > FL(0.0)) {
        for (n=offset; n<nsmps; n++) {
          ar[n] = state = asig[n];
        }
      }
      else {
        for (n=offset; n<nsmps; n++) {
          ar[n] = state;
        }
      }
    }
    p->state = state;
    return OK;
}

int32_t delset(CSOUND *csound, DELAY *p)
{
    int32_t      npts;
    char        *auxp;

    if (UNLIKELY(*p->istor && p->auxch.auxp != NULL))
      return OK;
    /* Round not truncate */
    if (UNLIKELY((npts = MYFLT2LRND(*p->idlt * csound->esr)) <= 0)) {
      return csound->InitError(csound, Str("illegal delay time"));
    }

    if ((auxp = p->auxch.auxp) == NULL ||
        npts != p->npts) { /* new space if reqd */
      csound->AuxAlloc(csound, (int32_t)npts*sizeof(MYFLT), &p->auxch);
      auxp = p->auxch.auxp;
      p->npts = npts;
    }
    else if (!(*p->istor)) {                    /* else if requested */
      memset(auxp, '\0', npts*sizeof(MYFLT));
    }
    p->curp = (MYFLT *) auxp;

    return OK;
}

int32_t delrset(CSOUND *csound, DELAYR *p)
{
    uint32_t    npts;
    MYFLT       *auxp;

    if (UNLIKELY(!IS_ASIG_ARG(p->ar)))
      return csound->InitError(csound, Str("delayr: invalid outarg type"));
    /* fifo for delayr pointers by Jens Groh: */
    /* append structadr for delayw to fifo: */
    if (csound->first_delayr != NULL)       /* fifo not empty */
      ((DELAYR*) csound->last_delayr)->next_delayr = p;
    else                                    /* fifo empty */
      csound->first_delayr = (void*) p;
    csound->last_delayr = (void*) p;
    csound->delayr_stack_depth++;
    p->next_delayr = NULL;
    if (p->OUTOCOUNT > 1) {
      /* set optional output arg if specified */
      *(p->indx) = (MYFLT)-(csound->delayr_stack_depth);
    }

    if (UNLIKELY(*p->istor != FL(0.0) && p->auxch.auxp != NULL))
      return OK;
    /* ksmps is min dely */
    if (UNLIKELY((npts=(uint32_t)MYFLT2LRND(*p->idlt*csound->esr)) < CS_KSMPS)) {
      return csound->InitError(csound, Str("illegal delay time"));
    }
    if ((auxp = (MYFLT*)p->auxch.auxp) == NULL ||       /* new space if reqd */
        npts != p->npts) {
      csound->AuxAlloc(csound, (int32_t)npts*sizeof(MYFLT), &p->auxch);
      auxp = (MYFLT*)p->auxch.auxp;
      p->npts = npts;
    }
    else if (*p->istor == FL(0.0)) {            /* else if requested */
      memset(auxp, 0, npts*sizeof(MYFLT));
    }
    p->curp = auxp;
    return OK;
}

int32_t delwset(CSOUND *csound, DELAYW *p)
{
   /* fifo for delayr pointers by Jens Groh: */
    if (UNLIKELY(csound->first_delayr == NULL)) {
      return csound->InitError(csound,
                               Str("delayw: associated delayr not found"));
    }
    p->delayr = (DELAYR*) csound->first_delayr;         /* adr delayr struct */
    /* remove structadr from fifo */
    if (csound->last_delayr == csound->first_delayr) {  /* fifo will be empty */
      csound->first_delayr = NULL;
    }
    else    /* fifo will not be empty */
      csound->first_delayr = ((DELAYR*) csound->first_delayr)->next_delayr;
    csound->delayr_stack_depth--;
    return OK;
}

static DELAYR *delayr_find(CSOUND *csound, MYFLT *ndx)
{
    DELAYR  *d = (DELAYR*) csound->first_delayr;
    int32_t     n = (int32_t)MYFLT2LRND(*ndx);

    if (UNLIKELY(d == NULL)) {
      csound->InitError(csound, Str("deltap: associated delayr not found"));
      return NULL;
    }
    if (!n)
      return (DELAYR*) csound->last_delayr;     /* default: use last delayr */
    else if (n > 0)
      n = csound->delayr_stack_depth - n;       /* ndx > 0: LIFO index mode */
    else
      n = -n;                                   /* ndx < 0: FIFO index mode */
    if (UNLIKELY(n < 1 || n > csound->delayr_stack_depth)) {
      csound->InitError(csound,
                        Str("deltap: delayr index %.0f is out of range"),
                        (double)*ndx);
      return NULL;
    }
    /* find delay line */
    while (--n)
      d = d->next_delayr;
    return d;
}

int32_t tapset(CSOUND *csound, DELTAP *p)
{
    p->delayr = delayr_find(csound, p->indx);
    return (p->delayr != NULL ? OK : NOTOK);
}

int32_t delay(CSOUND *csound, DELAY *p)
{
    MYFLT       *ar, *asig, *curp, *endp;
    uint32_t offset = 0;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;  /* RWD fix */
    ar = p->ar;
    if (csound->oparms->sampleAccurate) {
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      offset = p->h.insdshead->ksmps_offset;

      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    asig = p->asig;
    curp = p->curp;
    endp = (MYFLT *) p->auxch.endp;
    for (n=offset; n<nsmps; n++) {
      MYFLT in = asig[n];       /* Allow overwriting form */
      ar[n] = *curp;
      *curp = in;
      if (UNLIKELY(++curp >= endp))
        curp = (MYFLT *) p->auxch.auxp;
    }
    p->curp = curp;             /* sav the new curp */

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("delay: not initialised"));
}

int32_t delayr(CSOUND *csound, DELAYR *p)
{
    MYFLT       *ar, *curp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    curp = p->curp;
    endp = (MYFLT *) p->auxch.endp;
    for (n=offset; n<nsmps; n++) {
      ar[n] = *curp++;
      if (UNLIKELY(curp >= endp))
        curp = (MYFLT *) p->auxch.auxp;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("delayr: not initialised"));
}

int32_t delayw(CSOUND *csound, DELAYW *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *asig, *curp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1; /* RWD fix */
    asig = p->asig;
    curp = q->curp;
    endp = (MYFLT *) q->auxch.endp;
    if (UNLIKELY(early)) nsmps -= early;
    for (n=offset; n<nsmps; n++) {
      *curp = asig[n];
      if (UNLIKELY(++curp >= endp))
        curp = (MYFLT *) q->auxch.auxp;
    }
    q->curp = curp;                                     /* now sav new curp */
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("delayw: not initialised"));
}

int32_t deltap(CSOUND *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1; /* RWD fix */
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    tap = q->curp - MYFLT2LRND(*p->xdlt * csound->esr);
    while (tap < (MYFLT *) q->auxch.auxp)
      tap += q->npts;
    endp = (MYFLT *) q->auxch.endp;
    for (n=offset; n<nsmps; n++) {
      if (UNLIKELY(tap >= endp))
        tap -= q->npts;
      ar[n] = *tap++;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("deltap: not initialised"));
}

int32_t deltapi(CSOUND *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *prv, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t       idelsmps;
    MYFLT       delsmps, delfrac;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!IS_ASIG_ARG(p->xdlt)) {
      if (*p->xdlt == INFINITY) goto err2;
      delsmps = *p->xdlt * csound->esr;
      idelsmps = (int32_t)delsmps;
      delfrac = delsmps - idelsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      for (n=offset; n<nsmps; n++) {
        if (UNLIKELY(tap >= endp))
          tap -= q->npts;
        if (UNLIKELY((prv = tap - 1) < begp))
          prv += q->npts;
        ar[n] = *tap + (*prv - *tap) * delfrac;
        tap++;
      }
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      for (n=offset; n<nsmps; n++) {
        if (timp[n] == INFINITY) goto err2;
        delsmps = timp[n] * csound->esr;
        idelsmps = (int32_t)delsmps;
        delfrac = delsmps - idelsmps;
        tap = curq++ - idelsmps;
        if (UNLIKELY(tap < begp)) tap += q->npts;
        else if (UNLIKELY(tap >= endp))
          tap -= q->npts;
        if (UNLIKELY((prv = tap - 1) < begp))
          prv += q->npts;
        ar[n] = *tap + (*prv - *tap) * delfrac;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                              Str("deltapi: not initialised"));
  err2:
    return csound->PerfError(csound, &(p->h),
                              Str("deltapi: INF delaytime"));
}

/* ***** From Hans Mikelson ************* */
/* Delay N samples */
int32_t deltapn(CSOUND *csound, DELTAP *p)
{
    DELAYR *q = p->delayr;
    MYFLT  *ar, *tap, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  idelsmps;
    MYFLT  delsmps;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!IS_ASIG_ARG(p->xdlt)) {
      delsmps = *p->xdlt;
      idelsmps = (int32_t)delsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      for (n=offset; n<nsmps; n++) {
        while (UNLIKELY(tap >= endp ))
          tap -= q->npts;
        while (UNLIKELY(tap < begp))
          tap += q->npts;
        ar[n] = *tap;
        tap++;
      }
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      for (n=offset; n<nsmps; n++) {
        delsmps = timp[n];
        idelsmps = (int32_t)delsmps;
        if (UNLIKELY((tap = curq++ - idelsmps) < begp))
          tap += q->npts;
        else if (UNLIKELY(tap >= endp))
          tap -= q->npts;
        ar[n] = *tap;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("deltapn: not initialised"));
}

/* **** JPff **** */
int32_t deltap3(CSOUND *csound, DELTAP *p)
{
    DELAYR      *q = p->delayr;
    MYFLT       *ar, *tap, *prv, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t       idelsmps;
    MYFLT       delsmps, delfrac;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    begp = (MYFLT *) q->auxch.auxp;
    endp = (MYFLT *) q->auxch.endp;
    if (!IS_ASIG_ARG(p->xdlt)) {
      if (*p->xdlt == INFINITY) goto err2;
      delsmps = *p->xdlt * csound->esr;
      idelsmps = (int32_t)delsmps;
      delfrac = delsmps - idelsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      for (n=offset; n<nsmps; n++) {
        MYFLT ym1, y0, y1, y2;
        if (UNLIKELY(tap >= endp))
          tap -= q->npts;
        if (UNLIKELY((prv = tap - 1) < begp))
          prv += q->npts;
        if (UNLIKELY(prv - 1 < begp))
          y2 = *(prv-1+q->npts);
        else
          y2 = *(prv-1);
        if (UNLIKELY(tap + 1 >= endp))
          ym1 = *(tap+1-q->npts);
        else
          ym1 = *(tap+1);
        y0 = *tap; y1 = *prv;
        {
          MYFLT w, x, y, z;
          z = delfrac * delfrac; z--; z *= FL(0.16666666666667);
          y = delfrac; y++; w = (y *= FL(0.5)); w--;
          x = FL(3.0) * z; y -= x; w -= z; x -= delfrac;
          ar[n] = (w*ym1 + x*y0 + y*y1 + z*y2) * delfrac + y0;
        }
        tap++;
      }
    }
    else {
      MYFLT *timp = p->xdlt, *curq = q->curp;
      for (n=offset; n<nsmps; n++) {
        MYFLT ym1, y0, y1, y2;
        if (timp[n] == INFINITY) goto err2;
        delsmps = *timp++ * csound->esr;
        idelsmps = (int32_t)delsmps;
        delfrac = delsmps - idelsmps;
        if (UNLIKELY((tap = curq++ - idelsmps) < begp))
          tap += q->npts;
        else if (UNLIKELY(tap >= endp))
          tap -= q->npts;
        if (UNLIKELY((prv = tap - 1) < begp))
          prv += q->npts;
        if (UNLIKELY(prv - 1 < begp)) y2 = *(prv-1+q->npts);
        else                          y2 = *(prv-1);
        if (UNLIKELY(tap + 1 >= endp)) ym1 = *(tap+1-q->npts);
        else                           ym1 = *(tap+1);
        y0 = *tap; y1 = *prv;
        {
          MYFLT w, x, y, z;
          z = delfrac * delfrac; z--; z *= FL(0.1666666667);
          y = delfrac; y++; w = (y *= FL(0.5)); w--;
          x = FL(3.0) * z; y -= x; w -= z; x -= delfrac;
          ar[n] = (w*ym1 + x*y0 + y*y1 + z*y2) * delfrac + y0;
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("deltap3: not initialised"));
  err2:
    return csound->PerfError(csound, &(p->h),
                              Str("deltapi: INF delaytime"));

}


/* deltapx and deltapxw opcodes by Istvan Varga */

int32_t tapxset(CSOUND *csound, DELTAPX *p)
{
    p->delayr = delayr_find(csound, p->indx);
    if (UNLIKELY(p->delayr == NULL))
      return NOTOK;
    p->wsize = (int32_t)(*(p->iwsize) + FL(0.5));          /* window size */
    p->wsize = ((p->wsize + 2) >> 2) << 2;
    if (UNLIKELY(p->wsize < 4)) p->wsize = 4;
    if (UNLIKELY(p->wsize > 1024)) p->wsize = 1024;
    /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
    p->d2x = 1.0 - pow((double)p->wsize * 0.85172, -0.89624);
    p->d2x /= (double)((p->wsize * p->wsize) >> 2);
    return OK;
}

int32_t deltapx(CSOUND *csound, DELTAPX *p)                 /* deltapx opcode */
{
    DELAYR  *q = p->delayr;
    MYFLT   *out1, *del, *buf1, *bufp, *bufend;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   indx, maxd, xpos;

    if (UNLIKELY(q->auxch.auxp == NULL)) goto err1; /* RWD fix */
    out1 = p->ar; del = p->adlt;
    if (UNLIKELY(offset)) memset(out1, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    }
    buf1 = (MYFLT *) q->auxch.auxp;
    indx = (int32_t) (q->curp - buf1);
    maxd = q->npts; bufend = buf1 + maxd;

    if (p->wsize != 4) {                /* window size >= 8 */
      double  d, x1, n1, w, d2x;
      int32_t     i2, i;
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = p->d2x;
      for (n=offset; n<nsmps; n++) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (double)indx - (double)*(del++) * (double)csound->esr;
        while (x1 < 0.0) x1 += (double)maxd;
        xpos = (int32_t)x1; x1 -= (double)xpos;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 > 0.00000001 && x1 < 0.99999999) {
          xpos -= i2;
          while (xpos < 0) xpos += maxd;
          d = (double)(1 - i2) - x1;
          bufp = buf1 + xpos;
          i = i2;
          n1 = 0.0;
          do {
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            n1 += w * w * (double)*bufp / d; d++;
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            n1 -= w * w * (double)*bufp / d; d++;
          } while (--i);
          out1[n] = (MYFLT)(n1 * sin(PI * x1) / PI);
        }
        else {                                          /* integer sample */
          xpos = MYFLT2LRND((double)xpos + x1);         /* position */
          if (xpos >= maxd) xpos -= maxd;
          out1[n] = buf1[xpos];
        }
        indx++;
      }
    }
    else {                          /* window size = 4, cubic interpolation */
      double  x, am1, a0, a1, a2;
      for (n=offset; n<nsmps; n++) {
        am1 = (double)indx - (double)*(del++) * (double)csound->esr;
        while (am1 < 0.0) am1 += (double)maxd;
        xpos = (int32_t) am1; am1 -= (double)xpos;

        a0  = am1 * am1; a2 = 0.16666667 * (am1 * a0 - am1);    /* sample +2 */
        a1  = 0.5 * (a0 + am1) - 3.0 * a2;                      /* sample +1 */
        am1 = 0.5 * (a0 - am1) - a2;                            /* sample -1 */
        a0  = 3.0 * a2 - a0; a0++;                              /* sample 0  */

        bufp = (xpos ? (buf1 + (xpos - 1L)) : (bufend - 1));
        while (bufp >= bufend) bufp -= maxd;
        x = am1 * (double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a0 * (double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a1 * (double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a2 * (double)*bufp;

        indx++; out1[n] = (MYFLT)x;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("deltap: not initialised"));
}

int32_t deltapxw(CSOUND *csound, DELTAPX *p)                /* deltapxw opcode */
{
    DELAYR  *q = p->delayr;
    MYFLT   *in1, *del, *buf1, *bufp, *bufend;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   indx, maxd, xpos;

    if (UNLIKELY(q->auxch.auxp == NULL)) goto err1; /* RWD fix */
    in1 = p->ar; del = p->adlt;
    if (UNLIKELY(early)) nsmps -= early;
    buf1 = (MYFLT *) q->auxch.auxp;
    indx = (int32_t) (q->curp - buf1);
    maxd = q->npts; bufend = buf1 + maxd;

    if (p->wsize != 4) {                /* window size >= 8 */
      double  d, x1, n1, w, d2x;
      int32_t     i2, i;
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = p->d2x;
      for (n=offset; n<nsmps; n++) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (double)indx - (double)*(del++) * (double)csound->esr;
        while (x1 < 0.0) x1 += (double)maxd;
        xpos = (int32_t) x1; x1 -= (double)xpos;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 > 0.00000001 && x1 < 0.99999999) {
          n1 = (double)*in1 * (sin(PI * x1) / PI);
          xpos -= i2;
          while (xpos < 0) xpos += maxd;
          d = (double)(1 - i2) - x1;
          bufp = buf1 + xpos;
          i = i2;
          do {
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            *bufp = (MYFLT)((double)*bufp + w * w * n1 / d); d++;
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            *bufp = (MYFLT)((double)*bufp - w * w * n1 / d); d++;
          } while (--i);
        }
        else {                                          /* integer sample */
          xpos = MYFLT2LRND((double)xpos + x1);         /* position */
          if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
          buf1[xpos] += in1[n];
        }
        indx++;
      }
    }
    else {                          /* window size = 4, cubic interpolation */
      double  x, am1, a0, a1, a2;
      for (n=offset; n<nsmps; n++) {
        am1 = (double)indx - (double)*(del++) * (double)csound->esr;
        while (am1 < 0.0) am1 += (double)maxd;
        xpos = (int32_t) am1; am1 -= (double)xpos;

        a0  = am1 * am1; a2 = 0.16666667 * (am1 * a0 - am1);    /* sample +2 */
        a1  = 0.5 * (a0 + am1) - 3.0 * a2;                      /* sample +1 */
        am1 = 0.5 * (a0 - am1) - a2;                            /* sample -1 */
        a0  = 3.0 * a2 - a0; a0++;                              /* sample 0  */

        x = (double)in1[n];
        bufp = (xpos ? (buf1 + (xpos - 1L)) : (bufend - 1));
        while (bufp >= bufend) bufp -= maxd;
        *bufp += (MYFLT)(am1 * x); if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (MYFLT)(a0 * x);  if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (MYFLT)(a1 * x);  if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (MYFLT)(a2 * x);

        indx++;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("deltap: not initialised"));
}

int32_t del1set(CSOUND *csound, DELAY1 *p)
{
    IGN(csound);
    if (!(*p->istor))
      p->sav1 = FL(0.0);
    return OK;
}

int32_t delay1(CSOUND *csound, DELAY1 *p)
{
    IGN(csound);
    MYFLT       *ar, *asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    ar = p->ar;
    /* asig = p->asig - 1; */
    asig = p->asig;
    ar[offset] = p->sav1;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    memmove(&ar[offset+1], &asig[offset], sizeof(MYFLT)*(nsmps-1-offset));
    p->sav1 = asig[nsmps-1];
    return OK;
}

int32_t cmbset(CSOUND *csound, COMB *p)
{
    int32_t       lpsiz, nbytes;

    if (*p->insmps != 0) {
      if (UNLIKELY((lpsiz = MYFLT2LRND(*p->ilpt))) <= 0) {
        return csound->InitError(csound, Str("illegal loop time"));
      }
    }
    else if (UNLIKELY((lpsiz = MYFLT2LRND(*p->ilpt * csound->esr)) <= 0)) {
      return csound->InitError(csound, Str("illegal loop time"));
    }
    nbytes = lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || (uint32_t)nbytes != p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      p->prvt = FL(0.0);
      p->coef = FL(0.0);
    }
    else if (!(*p->istor)) {
      /* Does this assume sizeof(MYFLT)==sizeof(int32_t)?? */
      p->pntr = (MYFLT *) p->auxch.auxp;
      memset(p->auxch.auxp, '\0', nbytes);
      p->prvt = FL(0.0);
      p->coef = FL(0.0);
    }
    return OK;
}

int32_t comb(CSOUND *csound, COMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig, *xp, *endp;
    MYFLT       coef = p->coef;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      /*
       * The argument to exp() in the following is sometimes a small
       * enough negative number to result in a denormal (or worse)
       * on Alpha. So if the result would be less than 1.0e-16, we
       * just say it's zero and don't call exp().  heh 981101
       */
      double exp_arg = (double)(log001 * *p->ilpt / p->prvt);
      if (UNLIKELY(exp_arg < -36.8413615))    /* ln(1.0e-16) */
        coef = p->coef = FL(0.0);
      else
        coef = p->coef = (MYFLT)exp(exp_arg);
    }
    xp = p->pntr;
    endp = (MYFLT *) p->auxch.endp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      MYFLT out = *xp;
      *xp *= coef;
      *xp += asig[n];
      ar[n] = out;
      if (UNLIKELY(++xp >= endp))
        xp = (MYFLT *) p->auxch.auxp;
    }
    p->pntr = xp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("comb: not initialised"));
}

int32_t invcomb(CSOUND *csound, COMB *p)
{
    int32_t n, nsmps = csound->ksmps;
    MYFLT       *ar, *asig, *xp, *endp;
    MYFLT       coef = p->coef;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      /*
       * The argument to exp() in the following is sometimes a small
       * enough negative number to result in a denormal (or worse)
       * on Alpha. So if the result would be less than 1.0e-16, we
       * just say it is zero and do not call exp().  heh 981101
       */
      double exp_arg = (double)(log001 * *p->ilpt / p->prvt);
      if (UNLIKELY(exp_arg < -36.8413615))    /* ln(1.0e-16) */
        coef = p->coef = FL(0.0);
      else
        coef = p->coef = (MYFLT)exp(exp_arg);
    }
    xp = p->pntr;
    endp = (MYFLT *) p->auxch.endp;
    ar = p->ar;
    asig = p->asig;
    MYFLT out;
    for (n=0; n<nsmps; n++) {
      out = *xp;
      ar[n] = (*xp = asig[n])-coef*out;
      if (UNLIKELY(++xp >= endp))
        xp = (MYFLT *) p->auxch.auxp;
    }
    p->pntr = xp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("combinv: not initialised"));
}

int32_t alpass(CSOUND *csound, COMB *p)
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig, *xp, *endp;
    MYFLT       y, z;
    MYFLT       coef = p->coef;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      coef = p->coef = EXP(log001 * *p->ilpt / p->prvt);
    }
    xp = p->pntr;
    endp = (MYFLT *) p->auxch.endp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      y = *xp;
      *xp++ = z = coef * y + asig[n];
      ar[n] = y - coef * z;
      if (UNLIKELY(xp >= endp))
        xp = (MYFLT *) p->auxch.auxp;
    }
    p->pntr = xp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("alpass: not initialised"));
}

static const MYFLT revlptimes[6] = {FL(0.0297), FL(0.0371), FL(0.0411),
                                    FL(0.0437), FL(0.0050), FL(0.0017)};

void reverbinit(CSOUND *csound)         /* called once by oload */
{                                       /*  to init reverb data */
    const MYFLT *lptimp = revlptimes;
    int32_t     *lpsizp = csound->revlpsiz;
    int32_t n = 6;

    if (csound->revlpsum==0) {
      csound->revlpsum = 0;
      for (n=0; n<6; n++) {
        lpsizp[n] = MYFLT2LRND(lptimp[n] * csound->esr);
        csound->revlpsum += lpsizp[n];
      }
    }
}

int32_t rvbset(CSOUND *csound, REVERB *p)
{
    if (p->auxch.auxp == NULL) {                        /* if no space yet, */
      int32      *sizp = csound->revlpsiz;               /*    allocate it   */
      csound->AuxAlloc(csound, csound->revlpsum * sizeof(MYFLT), &p->auxch);
      p->adr1 = p->p1 = (MYFLT *) p->auxch.auxp;
      p->adr2 = p->p2 = p->adr1 + *sizp++;
      p->adr3 = p->p3 = p->adr2 + *sizp++;              /*    & init ptrs   */
      p->adr4 = p->p4 = p->adr3 + *sizp++;
      p->adr5 = p->p5 = p->adr4 + *sizp++;
      p->adr6 = p->p6 = p->adr5 + *sizp++;
      if (UNLIKELY(p->adr6 + *sizp != (MYFLT *) p->auxch.endp)) {
        return csound->InitError(csound, Str("revlpsiz inconsistent\n"));
      }
      p->prvt = FL(0.0);
    }
    else if (!(*p->istor)) {                    /* else if istor = 0 */
      memset(p->adr1, '\0', csound->revlpsum * sizeof(MYFLT));
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

int32_t reverb(CSOUND *csound, REVERB *p)
{
    MYFLT       *asig, *p1, *p2, *p3, *p4, *p5, *p6, *ar, *endp;
    MYFLT       c1,c2,c3,c4,c5,c6;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (UNLIKELY(p->prvt != *p->krvt)) {          /* People rarely change rvt */
      const MYFLT *lptimp = revlptimes;
      MYFLT       logdrvt = log001 / *p->krvt;
      c1=p->c1 = EXP(logdrvt * *lptimp++);
      c2=p->c2 = EXP(logdrvt * *lptimp++);
      c3=p->c3 = EXP(logdrvt * *lptimp++);
      c4=p->c4 = EXP(logdrvt * *lptimp++);
      c5=p->c5 = EXP(logdrvt * *lptimp++);
      c6=p->c6 = EXP(logdrvt * *lptimp++);
      p->prvt = *p->krvt;       /* JPff optimisation?? */
    }
    else {
      c1=p->c1;
      c2=p->c2;
      c3=p->c3;
      c4=p->c4;
      c5=p->c5;
      c6=p->c6;
   }

    p1 = p->p1;
    p2 = p->p2;
    p3 = p->p3;
    p4 = p->p4;
    p5 = p->p5;
    p6 = p->p6;
    endp = (MYFLT *) p->auxch.endp;

    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      MYFLT     cmbsum, y1, y2, z;
      MYFLT     sig = asig[n];
      cmbsum = *p1 + *p2 + *p3 + *p4;
      *p1 = c1 * *p1 + sig;
      *p2 = c2 * *p2 + sig;
      *p3 = c3 * *p3 + sig;
      *p4 = c4 * *p4 + sig;
      p1++; p2++; p3++; p4++;
      y1 = *p5;
      *p5++ = z = c5 * y1 + cmbsum;
      y1 -= c5 * z;
      y2 = *p6;
      *p6++ = z = c6 * y2 + y1;
      ar[n] = y2 - c6 * z;
      if (UNLIKELY(p1 >= p->adr2)) p1 = p->adr1;
      if (UNLIKELY(p2 >= p->adr3)) p2 = p->adr2;
      if (UNLIKELY(p3 >= p->adr4)) p3 = p->adr3;
      if (UNLIKELY(p4 >= p->adr5)) p4 = p->adr4;
      if (UNLIKELY(p5 >= p->adr6)) p5 = p->adr5;
      if (UNLIKELY(p6 >= endp))    p6 = p->adr6;
    }
    p->p1 = p1;
    p->p2 = p2;
    p->p3 = p3;
    p->p4 = p4;
    p->p5 = p5;
    p->p6 = p6;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("reverb: not initialised"));
}

int32_t panset(CSOUND *csound, PAN *p)
{
    FUNC  *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->ifn)) == NULL))
      return NOTOK;
    p->ftp = ftp;
    p->xmul = (*p->imode == FL(0.0) ? FL(1.0) : (MYFLT)ftp->flen);
    p->xoff = (*p->ioffset == FL(0.0) ? (MYFLT)ftp->flen * FL(0.5) : FL(0.0));

    return OK;
}

int32_t pan(CSOUND *csound, PAN *p)
{
    MYFLT   flend2, xndx_f, yndx_f, xt, yt, ch1, ch2, ch3, ch4;
    int32   xndx, yndx, flen;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    FUNC    *ftp;

    ftp = p->ftp;
    if (UNLIKELY(ftp == NULL)) goto err1;          /* RWD fix */
    xndx_f = (*p->kx * p->xmul) - p->xoff;
    yndx_f = (*p->ky * p->xmul) - p->xoff;
    flen = ftp->flen;
    flend2 = (MYFLT)flen * FL(0.5);
    xt = FABS(xndx_f);
    yt = FABS(yndx_f);
    if (xt > flend2 || yt > flend2) {
      if (xt > yt)
        yndx_f *= (flend2 / xt);
      else
        xndx_f *= (flend2 / yt);
    }
    xndx_f += flend2;
    yndx_f += flend2;
    xndx = MYFLT2LRND(xndx_f);
    yndx = MYFLT2LRND(yndx_f);
    xndx = (xndx >= 0L ? (xndx < flen ? xndx : flen) : 0L);
    yndx = (yndx >= 0L ? (yndx < flen ? yndx : flen) : 0L);
    ch1 = ftp->ftable[flen - xndx] * ftp->ftable[yndx];
    ch2 = ftp->ftable[xndx]        * ftp->ftable[yndx];
    ch3 = ftp->ftable[flen - xndx] * ftp->ftable[flen - yndx];
    ch4 = ftp->ftable[xndx]        * ftp->ftable[flen - yndx];

    if (UNLIKELY(offset)) {
      memset(p->r1, '\0', offset*sizeof(MYFLT));
      memset(p->r2, '\0', offset*sizeof(MYFLT));
      memset(p->r3, '\0', offset*sizeof(MYFLT));
      memset(p->r4, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->r2[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->r3[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->r4[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT sig = p->asig[n];
      p->r1[n] = sig * ch1;
      p->r2[n] = sig * ch2;
      p->r3[n] = sig * ch3;
      p->r4[n] = sig * ch4;
    }

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("pan: not initialised"));
}
