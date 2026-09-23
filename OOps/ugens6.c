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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h" /*                              UGENS6.C        */
#include "ugens6.h"
#include <math.h>

#define log001 (-FL(6.9078))    /* log(.001) */

int32_t downset(CSOUND *csound, DOWNSAMP *p)
{
    cs_double length = (cs_double)*p->ilen;

    /* Check the truncated length before converting to an unsigned integer. */
    if (UNLIKELY(!(length > -1.0 && length < (cs_double)CS_KSMPS + 1.0)))
      return csound->InitError(csound, "%s",
                               Str("downsamp: window length out of range"));
    p->len = (uint32_t)length;
    return OK;
}

int32_t downsamp(CSOUND *csound, DOWNSAMP *p)
{
    IGN(csound);
    cs_float       *asig, sum;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t end = CS_KSMPS - early;
    uint32_t len, n;

    if (UNLIKELY(offset >= end)) {
      *p->kr = FL(0.0);
      return OK;
    }
    if (p->len <= 1)
      *p->kr = p->asig[offset];
    else {
      asig = p->asig;
      sum = FL(0.0);
      len = p->len;
      /* Start the window at the first active sample and average only
         the samples available in this block. */
      if (len > end - offset) len = end - offset;
      for (n=offset; n<offset+len; n++) {
        sum += asig[n];
      }
      *p->kr = sum / len;
    }
    return OK;
}

int32_t upsamp(CSOUND *csound, UPSAMP *p)
{
    IGN(csound);
    cs_float kval = *p->ksig;
    cs_float *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
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
    cs_float *ar, val, incr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->rslt;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
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
    cs_float       *rslt, *asig, sum;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    rslt = p->rslt;
    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(cs_float));
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
    cs_float       tmp;
    tmp = *p->xsig;             /* IV - Sep 5 2002: fix to make */
    *p->rslt = tmp - p->prev;   /* diff work when the input and */
    p->prev = tmp;              /* output argument is the same  */
    return OK;
}

int32_t diff(CSOUND *csound, INDIFF *p)
{
    IGN(csound);
    cs_float       *ar, *asig, prev, tmp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->rslt;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
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
    cs_float       *ar, *asig, *agate, state;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ar = p->xr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
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
    if (UNLIKELY((npts = CS_FLOAT2LRND(*p->idlt * CS_ESR)) <= 0)) {
      return csound->InitError(csound, Str("illegal delay time"));
    }

    if ((auxp = p->auxch.auxp) == NULL ||
        npts != p->npts) { /* new space if reqd */
      csound->AuxAlloc(csound, (int32_t)npts*sizeof(cs_float), &p->auxch);
      auxp = p->auxch.auxp;
      p->npts = npts;
    }
    else if (!(*p->istor)) {                    /* else if requested */
      memset(auxp, '\0', npts*sizeof(cs_float));
    }
    p->curp = (cs_float *) auxp;

    return OK;
}

int32_t delrset(CSOUND *csound, DELAYR *p)
{
    uint32_t    npts;
    cs_float       *auxp;

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
      *(p->indx) = (cs_float)-(csound->delayr_stack_depth);
    }

    if (UNLIKELY(*p->istor != FL(0.0) && p->auxch.auxp != NULL))
      return OK;
    /* ksmps is min dely */
    if (UNLIKELY((npts=(uint32_t)CS_FLOAT2LRND(*p->idlt*CS_ESR)) < CS_KSMPS)) {
      return csound->InitError(csound, Str("illegal delay time"));
    }
    if ((auxp = (cs_float*)p->auxch.auxp) == NULL ||       /* new space if reqd */
        npts != p->npts) {
      csound->AuxAlloc(csound, (int32_t)npts*sizeof(cs_float), &p->auxch);
      auxp = (cs_float*)p->auxch.auxp;
      p->npts = npts;
    }
    else if (*p->istor == FL(0.0)) {            /* else if requested */
      memset(auxp, 0, npts*sizeof(cs_float));
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

static DELAYR *delayr_find(CSOUND *csound, cs_float *ndx)
{
    DELAYR  *d = (DELAYR*) csound->first_delayr;
    int32_t     n = (int32_t)CS_FLOAT2LRND(*ndx);

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
                        (cs_double)*ndx);
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
    cs_float       *ar, *asig, *curp, *endp;
    uint32_t offset = 0;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;  /* RWD fix */
    ar = p->ar;
    if (csound->oparms->sampleAccurate) {
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      offset = p->h.insdshead->ksmps_offset;

      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(cs_float));
      }
    }
    asig = p->asig;
    curp = p->curp;
    endp = (cs_float *) p->auxch.endp;
    for (n=offset; n<nsmps; n++) {
      cs_float in = asig[n];       /* Allow overwriting form */
      ar[n] = *curp;
      *curp = in;
      if (UNLIKELY(++curp >= endp))
        curp = (cs_float *) p->auxch.auxp;
    }
    p->curp = curp;             /* sav the new curp */

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("delay: not initialised"));
}

int32_t delayr(CSOUND *csound, DELAYR *p)
{
    cs_float       *ar, *curp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    curp = p->curp;
    endp = (cs_float *) p->auxch.endp;
    for (n=offset; n<nsmps; n++) {
      ar[n] = *curp++;
      if (UNLIKELY(curp >= endp))
        curp = (cs_float *) p->auxch.auxp;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("delayr: not initialised"));
}

int32_t delayw(CSOUND *csound, DELAYW *p)
{
    DELAYR      *q = p->delayr;
    cs_float       *asig, *curp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1; /* RWD fix */
    asig = p->asig;
    curp = q->curp;
    endp = (cs_float *) q->auxch.endp;
    if (UNLIKELY(early)) nsmps -= early;
    for (n=offset; n<nsmps; n++) {
      *curp = asig[n];
      if (UNLIKELY(++curp >= endp))
        curp = (cs_float *) q->auxch.auxp;
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
    cs_float       *ar, *tap, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1; /* RWD fix */
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    tap = q->curp - CS_FLOAT2LRND(*p->xdlt * CS_ESR);
    while (tap < (cs_float *) q->auxch.auxp)
      tap += q->npts;
    endp = (cs_float *) q->auxch.endp;
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
    cs_float       *ar, *tap, *prv, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t       idelsmps;
    cs_float       delsmps, delfrac;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    begp = (cs_float *) q->auxch.auxp;
    endp = (cs_float *) q->auxch.endp;
    if (!IS_ASIG_ARG(p->xdlt)) {
      if (*p->xdlt == INFINITY) goto err2;
      delsmps = *p->xdlt * CS_ESR;
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
      cs_float *timp = p->xdlt, *curq = q->curp;
      for (n=offset; n<nsmps; n++) {
        if (timp[n] == INFINITY) goto err2;
        delsmps = timp[n] * CS_ESR;
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
    cs_float  *ar, *tap, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  idelsmps;
    cs_float  delsmps;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    begp = (cs_float *) q->auxch.auxp;
    endp = (cs_float *) q->auxch.endp;
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
      cs_float *timp = p->xdlt, *curq = q->curp;
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
    cs_float       *ar, *tap, *prv, *begp, *endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t       idelsmps;
    cs_float       delsmps, delfrac;

    if (UNLIKELY(q->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    begp = (cs_float *) q->auxch.auxp;
    endp = (cs_float *) q->auxch.endp;
    if (!IS_ASIG_ARG(p->xdlt)) {
      if (*p->xdlt == INFINITY) goto err2;
      delsmps = *p->xdlt * CS_ESR;
      idelsmps = (int32_t)delsmps;
      delfrac = delsmps - idelsmps;
      tap = q->curp - idelsmps;
      while (tap < begp) tap += q->npts;
      for (n=offset; n<nsmps; n++) {
        cs_float ym1, y0, y1, y2;
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
          cs_float w, x, y, z;
          z = delfrac * delfrac; z--; z *= FL(0.16666666666667);
          y = delfrac; y++; w = (y *= FL(0.5)); w--;
          x = FL(3.0) * z; y -= x; w -= z; x -= delfrac;
          ar[n] = (w*ym1 + x*y0 + y*y1 + z*y2) * delfrac + y0;
        }
        tap++;
      }
    }
    else {
      cs_float *timp = p->xdlt, *curq = q->curp;
      for (n=offset; n<nsmps; n++) {
        cs_float ym1, y0, y1, y2;
        if (timp[n] == INFINITY) goto err2;
        delsmps = *timp++ * CS_ESR;
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
          cs_float w, x, y, z;
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
    p->d2x = 1.0 - pow((cs_double)p->wsize * 0.85172, -0.89624);
    p->d2x /= (cs_double)((p->wsize * p->wsize) >> 2);
    return OK;
}

int32_t deltapx(CSOUND *csound, DELTAPX *p)                 /* deltapx opcode */
{
    DELAYR  *q = p->delayr;
    cs_float   *out1, *del, *buf1, *bufp, *bufend;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   indx, maxd, xpos;

    if (UNLIKELY(q->auxch.auxp == NULL)) goto err1; /* RWD fix */
    out1 = p->ar; del = p->adlt;
    if (UNLIKELY(offset)) memset(out1, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(cs_float));
    }
    buf1 = (cs_float *) q->auxch.auxp;
    indx = (int32_t) (q->curp - buf1);
    maxd = q->npts; bufend = buf1 + maxd;

    if (p->wsize != 4) {                /* window size >= 8 */
      cs_double  d, x1, n1, w, d2x;
      int32_t     i2, i;
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = p->d2x;
      for (n=offset; n<nsmps; n++) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (cs_double)indx - (cs_double)*(del++) * (cs_double)CS_ESR;
        while (x1 < 0.0) x1 += (cs_double)maxd;
        xpos = (int32_t)x1; x1 -= (cs_double)xpos;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 > 0.00000001 && x1 < 0.99999999) {
          xpos -= i2;
          while (xpos < 0) xpos += maxd;
          d = (cs_double)(1 - i2) - x1;
          bufp = buf1 + xpos;
          i = i2;
          n1 = 0.0;
          do {
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            n1 += w * w * (cs_double)*bufp / d; d++;
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            n1 -= w * w * (cs_double)*bufp / d; d++;
          } while (--i);
          out1[n] = (cs_float)(n1 * sin(PI * x1) / PI);
        }
        else {                                          /* integer sample */
          xpos = CS_FLOAT2LRND((cs_double)xpos + x1);         /* position */
          if (xpos >= maxd) xpos -= maxd;
          out1[n] = buf1[xpos];
        }
        indx++;
      }
    }
    else {                          /* window size = 4, cubic interpolation */
      cs_double  x, am1, a0, a1, a2;
      for (n=offset; n<nsmps; n++) {
        am1 = (cs_double)indx - (cs_double)*(del++) * (cs_double)CS_ESR;
        while (am1 < 0.0) am1 += (cs_double)maxd;
        xpos = (int32_t) am1; am1 -= (cs_double)xpos;

        a0  = am1 * am1; a2 = 0.16666667 * (am1 * a0 - am1);    /* sample +2 */
        a1  = 0.5 * (a0 + am1) - 3.0 * a2;                      /* sample +1 */
        am1 = 0.5 * (a0 - am1) - a2;                            /* sample -1 */
        a0  = 3.0 * a2 - a0; a0++;                              /* sample 0  */

        bufp = (xpos ? (buf1 + (xpos - 1L)) : (bufend - 1));
        while (bufp >= bufend) bufp -= maxd;
        x = am1 * (cs_double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a0 * (cs_double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a1 * (cs_double)*bufp;   if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        x += a2 * (cs_double)*bufp;

        indx++; out1[n] = (cs_float)x;
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
    cs_float   *in1, *del, *buf1, *bufp, *bufend;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   indx, maxd, xpos;

    if (UNLIKELY(q->auxch.auxp == NULL)) goto err1; /* RWD fix */
    in1 = p->ar; del = p->adlt;
    if (UNLIKELY(early)) nsmps -= early;
    buf1 = (cs_float *) q->auxch.auxp;
    indx = (int32_t) (q->curp - buf1);
    maxd = q->npts; bufend = buf1 + maxd;

    if (p->wsize != 4) {                /* window size >= 8 */
      cs_double  d, x1, n1, w, d2x;
      int32_t     i2, i;
      i2 = (p->wsize >> 1);
      /* wsize = 4: d2x = 1 - 1/3, wsize = 64: d2x = 1 - 1/36 */
      d2x = p->d2x;
      for (n=offset; n<nsmps; n++) {
        /* x1: fractional part of delay time */
        /* x2: sine of x1 (for interpolation) */
        /* xpos: integer part of delay time (buffer position to read from) */

        x1 = (cs_double)indx - (cs_double)*(del++) * (cs_double)CS_ESR;
        while (x1 < 0.0) x1 += (cs_double)maxd;
        xpos = (int32_t) x1; x1 -= (cs_double)xpos;
        while (xpos >= maxd) xpos -= maxd;

        if (x1 > 0.00000001 && x1 < 0.99999999) {
          n1 = (cs_double)*in1 * (sin(PI * x1) / PI);
          xpos -= i2;
          while (xpos < 0) xpos += maxd;
          d = (cs_double)(1 - i2) - x1;
          bufp = buf1 + xpos;
          i = i2;
          do {
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            *bufp = (cs_float)((cs_double)*bufp + w * w * n1 / d); d++;
            w = 1.0 - d * d * d2x;
            if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
            *bufp = (cs_float)((cs_double)*bufp - w * w * n1 / d); d++;
          } while (--i);
        }
        else {                                          /* integer sample */
          xpos = CS_FLOAT2LRND((cs_double)xpos + x1);         /* position */
          if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
          buf1[xpos] += in1[n];
        }
        indx++;
      }
    }
    else {                          /* window size = 4, cubic interpolation */
      cs_double  x, am1, a0, a1, a2;
      for (n=offset; n<nsmps; n++) {
        am1 = (cs_double)indx - (cs_double)*(del++) * (cs_double)CS_ESR;
        while (am1 < 0.0) am1 += (cs_double)maxd;
        xpos = (int32_t) am1; am1 -= (cs_double)xpos;

        a0  = am1 * am1; a2 = 0.16666667 * (am1 * a0 - am1);    /* sample +2 */
        a1  = 0.5 * (a0 + am1) - 3.0 * a2;                      /* sample +1 */
        am1 = 0.5 * (a0 - am1) - a2;                            /* sample -1 */
        a0  = 3.0 * a2 - a0; a0++;                              /* sample 0  */

        x = (cs_double)in1[n];
        bufp = (xpos ? (buf1 + (xpos - 1L)) : (bufend - 1));
        while (bufp >= bufend) bufp -= maxd;
        *bufp += (cs_float)(am1 * x); if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (cs_float)(a0 * x);  if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (cs_float)(a1 * x);  if (UNLIKELY(++bufp >= bufend)) bufp = buf1;
        *bufp += (cs_float)(a2 * x);

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
    cs_float       *ar, *asig;
    cs_float       last;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;

    ar = p->ar;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    if (UNLIKELY(offset >= nsmps)) return OK;
    /* Preserve both endpoints when input and output share the buffer. */
    last = asig[nsmps-1];
    memmove(&ar[offset+1], &asig[offset], sizeof(cs_float)*(nsmps-1-offset));
    ar[offset] = p->sav1;
    p->sav1 = last;
    return OK;
}

int32_t cmbset(CSOUND *csound, COMB *p)
{
    int32_t       lpsiz, nbytes;

    if (*p->insmps != 0) {
      if (UNLIKELY((lpsiz = CS_FLOAT2LRND(*p->ilpt))) <= 0) {
        return csound->InitError(csound, Str("illegal loop time"));
      }
    }
    else if (UNLIKELY((lpsiz = CS_FLOAT2LRND(*p->ilpt * CS_ESR)) <= 0)) {
      return csound->InitError(csound, Str("illegal loop time"));
    }
    nbytes = lpsiz * sizeof(cs_float);
    if (p->auxch.auxp == NULL || (uint32_t)nbytes != p->auxch.size) {
      csound->AuxAlloc(csound, (int32_t)nbytes, &p->auxch);
      p->pntr = (cs_float *) p->auxch.auxp;
      p->prvt = FL(0.0);
      p->coef = FL(0.0);
    }
    else if (!(*p->istor)) {
      p->pntr = (cs_float *) p->auxch.auxp;
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
    cs_float       *ar, *asig, *xp, *endp;
    cs_float       coef = p->coef;
    cs_float loopTime = *p->insmps != 0 ? *p->ilpt * CS_ONEDSR : *p->ilpt;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      /*
       * The argument to exp() in the following is sometimes a small
       * enough negative number to result in a denormal (or worse)
       * on Alpha. So if the result would be less than 1.0e-16, we
       * just say it's zero and don't call exp().  heh 981101
       */
      cs_double exp_arg = (cs_double)(log001 * loopTime / p->prvt);
      if (UNLIKELY(exp_arg < -36.8413615))    /* ln(1.0e-16) */
        coef = p->coef = FL(0.0);
      else
        coef = p->coef = (cs_float)exp(exp_arg);
    }
    xp = p->pntr;
    endp = (cs_float *) p->auxch.endp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      cs_float out = *xp;
      *xp *= coef;
      *xp += asig[n];
      ar[n] = out;
      if (UNLIKELY(++xp >= endp))
        xp = (cs_float *) p->auxch.auxp;
    }
    p->pntr = xp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("comb: not initialised"));
}

int32_t invcomb(CSOUND *csound, COMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float       *ar, *asig, *xp, *endp;
    cs_float       coef = p->coef;
    cs_float loopTime = *p->insmps != 0 ? *p->ilpt * CS_ONEDSR : *p->ilpt;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      /*
       * The argument to exp() in the following is sometimes a small
       * enough negative number to result in a denormal (or worse)
       * on Alpha. So if the result would be less than 1.0e-16, we
       * just say it is zero and do not call exp().  heh 981101
       */
      cs_double exp_arg = (cs_double)(log001 * loopTime / p->prvt);
      if (UNLIKELY(exp_arg < -36.8413615))    /* ln(1.0e-16) */
        coef = p->coef = FL(0.0);
      else
        coef = p->coef = (cs_float)exp(exp_arg);
    }
    xp = p->pntr;
    endp = (cs_float *) p->auxch.endp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    asig = p->asig;
    cs_float out;
    for (n=offset; n<nsmps; n++) {
      out = *xp;
      ar[n] = (*xp = asig[n])-coef*out;
      if (UNLIKELY(++xp >= endp))
        xp = (cs_float *) p->auxch.auxp;
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
    cs_float       *ar, *asig, *xp, *endp;
    cs_float       y, z;
    cs_float       coef = p->coef;
    cs_float loopTime = *p->insmps != 0 ? *p->ilpt * CS_ONEDSR : *p->ilpt;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    int32_t audioRvt = IS_ASIG_ARG(p->krvt);
    if (!audioRvt && p->prvt != *p->krvt) {
      p->prvt = *p->krvt;
      coef = p->coef = EXP(log001 * loopTime / p->prvt);
    }
    xp = p->pntr;
    endp = (cs_float *) p->auxch.endp;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    asig = p->asig;
    if (audioRvt) {
      for (n=offset; n<nsmps; n++) {
        if (p->prvt != p->krvt[n]) {
          p->prvt = p->krvt[n];
          coef = p->coef = EXP(log001 * loopTime / p->prvt);
        }
        y = *xp;
        *xp++ = z = coef * y + asig[n];
        ar[n] = y - coef * z;
        if (UNLIKELY(xp >= endp))
          xp = (cs_float *) p->auxch.auxp;
      }
    }
    else {
      for (n=offset; n<nsmps; n++) {
        y = *xp;
        *xp++ = z = coef * y + asig[n];
        ar[n] = y - coef * z;
        if (UNLIKELY(xp >= endp))
          xp = (cs_float *) p->auxch.auxp;
      }
    }
    p->pntr = xp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("alpass: not initialised"));
}

static const cs_float revlptimes[6] = {FL(0.0297), FL(0.0371), FL(0.0411),
                                    FL(0.0437), FL(0.0050), FL(0.0017)};

void reverbinit(CSOUND *csound, REVERB *p)
{
    const cs_float *lptimp = revlptimes;
    int32_t     *lpsizp = p->revlpsiza;
    int32_t n = 6;
    p->revlpsum = 0;
    for (n=0; n<6; n++) {
        lpsizp[n] = CS_FLOAT2LRND(lptimp[n] * CS_ESR);
        p->revlpsum += lpsizp[n];
      }
}

int32_t rvbset(CSOUND *csound, REVERB *p)
{
    if (p->auxch.auxp == NULL) {                        /* if no space yet, */
      int32      *sizp = p->revlpsiza;               /*    allocate it   */
      reverbinit(csound, p);
      csound->AuxAlloc(csound, p->revlpsum * sizeof(cs_float), &p->auxch);
      p->adr1 = p->p1 = (cs_float *) p->auxch.auxp;
      p->adr2 = p->p2 = p->adr1 + *sizp++;
      p->adr3 = p->p3 = p->adr2 + *sizp++;              /*    & init ptrs   */
      p->adr4 = p->p4 = p->adr3 + *sizp++;
      p->adr5 = p->p5 = p->adr4 + *sizp++;
      p->adr6 = p->p6 = p->adr5 + *sizp++;
      if (UNLIKELY(p->adr6 + *sizp != (cs_float *) p->auxch.endp)) {
        return csound->InitError(csound, Str("revlpsiz inconsistent\n"));
      }
      p->prvt = FL(0.0);
    }
    else if (!(*p->istor)) {                    /* else if istor = 0 */
      memset(p->adr1, '\0', p->revlpsum * sizeof(cs_float));
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
    cs_float       *asig, *p1, *p2, *p3, *p4, *p5, *p6, *ar, *endp;
    cs_float       c1,c2,c3,c4,c5,c6;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (UNLIKELY(p->prvt != *p->krvt)) {          /* People rarely change rvt */
      const cs_float *lptimp = revlptimes;
      cs_float       logdrvt = log001 / *p->krvt;
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
    endp = (cs_float *) p->auxch.endp;

    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      cs_float     cmbsum, y1, y2, z;
      cs_float     sig = asig[n];
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

    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
      return NOTOK;
    p->ftp = ftp;
    p->xmul = (*p->imode == FL(0.0) ? FL(1.0) : (cs_float)ftp->flen);
    p->xoff = (*p->ioffset == FL(0.0) ? (cs_float)ftp->flen * FL(0.5) : FL(0.0));

    return OK;
}

int32_t pan(CSOUND *csound, PAN *p)
{
    cs_float   flend2, xndx_f, yndx_f, xt, yt, ch1, ch2, ch3, ch4;
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
    flend2 = (cs_float)flen * FL(0.5);
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
    xndx = CS_FLOAT2LRND(xndx_f);
    yndx = CS_FLOAT2LRND(yndx_f);
    xndx = (xndx >= 0L ? (xndx < flen ? xndx : flen) : 0L);
    yndx = (yndx >= 0L ? (yndx < flen ? yndx : flen) : 0L);
    ch1 = ftp->ftable[flen - xndx] * ftp->ftable[yndx];
    ch2 = ftp->ftable[xndx]        * ftp->ftable[yndx];
    ch3 = ftp->ftable[flen - xndx] * ftp->ftable[flen - yndx];
    ch4 = ftp->ftable[xndx]        * ftp->ftable[flen - yndx];

    if (UNLIKELY(offset)) {
      memset(p->r1, '\0', offset*sizeof(cs_float));
      memset(p->r2, '\0', offset*sizeof(cs_float));
      memset(p->r3, '\0', offset*sizeof(cs_float));
      memset(p->r4, '\0', offset*sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r1[nsmps], '\0', early*sizeof(cs_float));
      memset(&p->r2[nsmps], '\0', early*sizeof(cs_float));
      memset(&p->r3[nsmps], '\0', early*sizeof(cs_float));
      memset(&p->r4[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset; n<nsmps; n++) {
      cs_float sig = p->asig[n];
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
