/*
    nlfilt.c:

    Copyright (C) 1996 John ffitch, Richard Dobson

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

/* Y{n} =a Y{n-1} + b Y{n-2} + d Y^2{n-L} + X{n} - C */

/***************************************************************\
*       nlfilt.c                                                *
*       Non-linear filter (Excitable region)                    *
*       5 June 1996 John ffitch                                 *
*       See paper by Dobson and ffitch, ICMC'96                 *
\***************************************************************/

#include "stdopcod.h"
#include "nlfilt.h"

typedef struct {
        OPDS    h;
        MYFLT   *ians;
        MYFLT   *index;
} PFIELD;

typedef struct {
        OPDS    h;
        STRINGDAT   *ians;
        MYFLT   *index;
} PFIELDSTR;

typedef struct {
        OPDS    h;
        MYFLT   *inits[24];
        MYFLT   *start;
        MYFLT   *end;
} PINIT;

typedef struct {
        OPDS    h;
        ARRAYDAT *inits;
        MYFLT   *start;
        MYFLT   *end;
} PAINIT;

#define MAX_DELAY   (1024)
#define MAXAMP      (FL(64000.0))

static int32_t nlfiltset(CSOUND *csound, NLFILT *p)
{
    if (p->delay.auxp == NULL ||
        p->delay.size<MAX_DELAY * sizeof(MYFLT)) {        /* get newspace    */
      csound->AuxAlloc(csound, MAX_DELAY * sizeof(MYFLT), &p->delay);
    }
    else {
      memset(p->delay.auxp, 0, MAX_DELAY * sizeof(MYFLT));
    }
    p->point = 0;
    return OK;
} /* end nlfset(p) */

static int32_t nlfilt(CSOUND *csound, NLFILT *p)
{
    MYFLT   *ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     point = p->point;
    int32_t     nm1 = point;
    int32_t     nm2 = point - 1;
    int32_t     nmL;
    MYFLT   ynm1, ynm2, ynmL;
    MYFLT   a = *p->a, b = *p->b, d = *p->d, C = *p->C;
    MYFLT   *in = p->in;
    MYFLT   *fp = (MYFLT*) p->delay.auxp;
    MYFLT   L = *p->L;
    MYFLT   maxamp, dvmaxamp, maxampd2;

    if (UNLIKELY(fp == NULL)) goto err1;                   /* RWD fix */
    ar   = p->ar;
                                        /* L is k-rate so need to check */
    if (L < FL(1.0))
      L = FL(1.0);
    else if (L >= MAX_DELAY) {
      L = (MYFLT) MAX_DELAY;
    }
    nmL = point - (int32_t) (L) - 1;
    if (UNLIKELY(nm1 < 0)) nm1 += MAX_DELAY;      /* Deal with the wrapping */
    if (UNLIKELY(nm2 < 0)) nm2 += MAX_DELAY;
    if (UNLIKELY(nmL < 0)) nmL += MAX_DELAY;
    ynm1 = fp[nm1];                     /* Pick up running values */
    ynm2 = fp[nm2];
    ynmL = fp[nmL];
    maxamp = csound->e0dbfs * FL(1.953125);     /* 64000 with default 0dBFS */
    dvmaxamp = FL(1.0) / maxamp;
    maxampd2 = maxamp * FL(0.5);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT yn;
      MYFLT out;
      yn = a * ynm1 + b * ynm2 + d * ynmL * ynmL - C;
      yn += in[n] * dvmaxamp;           /* Must work in small amplitudes  */
      out = yn * maxampd2;              /* Write output */
      if (out > maxamp)
        out = maxampd2;
      else if (out < -maxamp)
        out = -maxampd2;
      ar[n] = out;
      if (UNLIKELY(++point == MAX_DELAY)) {
        point = 0;
      }
      fp[point] = yn;                   /* and delay line */
      if (UNLIKELY(++nmL == MAX_DELAY)) {
        nmL = 0;
      }
      ynm2 = ynm1;                      /* Shuffle along */
      ynm1 = yn;
      ynmL = fp[nmL];
    }
    p->point = point;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("nlfilt: not initialised"));
} /* end nlfilt(p) */

/* Y{n} = a Y{n-1} + b Y{n-2} + d Y^2{n-L} + X{n} - C */

/* Revised version due to Risto Holopainen 12 Mar 2004 */
/* Y{n} =tanh(a Y{n-1} + b Y{n-2} + d Y^2{n-L} + X{n} - C) */

static int32_t nlfilt2(CSOUND *csound, NLFILT *p)
{
    MYFLT   *ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps;
    int32_t     point = p->point;
    int32_t     nm1 = point;
    int32_t     nm2 = point - 1;
    int32_t     nmL;
    MYFLT   ynm1, ynm2, ynmL;
    MYFLT   a = *p->a, b = *p->b, d = *p->d, C = *p->C;
    MYFLT   *in = p->in;
    MYFLT   *fp = (MYFLT*) p->delay.auxp;
    MYFLT   L = *p->L;
    MYFLT   maxamp, dvmaxamp, maxampd2;

    if (UNLIKELY(fp == NULL)) goto err1;                   /* RWD fix */
    ar   = p->ar;
                                        /* L is k-rate so need to check */
    if (L < FL(1.0))
      L = FL(1.0);
    else if (L >= MAX_DELAY) {
      L = (MYFLT) MAX_DELAY;
    }
    nmL = point - (int32_t) (L) - 1;
    if (UNLIKELY(nm1 < 0)) nm1 += MAX_DELAY;      /* Deal with the wrapping */
    if (UNLIKELY(nm2 < 0)) nm2 += MAX_DELAY;
    if (UNLIKELY(nmL < 0)) nmL += MAX_DELAY;
    ynm1 = fp[nm1];                     /* Pick up running values */
    ynm2 = fp[nm2];
    ynmL = fp[nmL];
    nsmps = CS_KSMPS;
    maxamp = csound->e0dbfs * FL(1.953125);     /* 64000 with default 0dBFS */
    dvmaxamp = FL(1.0) / maxamp;
    maxampd2 = maxamp * FL(0.5);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT yn;
      MYFLT out;
      yn = a * ynm1 + b * ynm2 + d * ynmL * ynmL - C;
      yn += in[n] * dvmaxamp;           /* Must work in small amplitudes  */
      out = yn * maxampd2;              /* Write output */
      if (out > maxamp)
        out = maxampd2;
      else if (out < -maxamp)
        out = -maxampd2;
      ar[n] = out;
      if (UNLIKELY(++point == MAX_DELAY)) {
        point = 0;
      }
      yn = TANH(yn);
      fp[point] = yn;                   /* and delay line */
      if (UNLIKELY(++nmL == MAX_DELAY)) {
        nmL = 0;
      }
      ynm2 = ynm1;                      /* Shuffle along */
      ynm1 = yn;
      ynmL = fp[nmL];
    }
    p->point = point;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("nlfilt2: not initialised"));
} /* end nlfilt2(p) */

/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************** */
/*     icnt    pcnt */
/*     ival    pfld indx */
int32_t pcount(CSOUND *csound, PFIELD *p)
{
    *p->ians = (MYFLT) csound->init_event->pcnt;
    return OK;
}

int32_t pvalue(CSOUND *csound, PFIELD *p)
{
    int32_t n = (int32_t)(*p->index);
    if (UNLIKELY(csound->init_event==NULL || n<1 || n>csound->init_event->pcnt)) {
      return csound->InitError(csound, Str("invalid p field index"));
    }
    *p->ians = csound->init_event->p[n];
    return OK;
}

int32_t pvaluestr(CSOUND *csound, PFIELDSTR *p)
{
    int32_t n = (int32_t)(*p->index);
    if (UNLIKELY(csound->init_event==NULL || n<1 || n>csound->init_event->pcnt)) {
      return csound->InitError(csound, Str("invalid p field index"));
    }

    if (p->ians->data!=NULL) csound->Free(csound, p->ians->data);

    if (LIKELY(csound->ISSTRCOD(csound->init_event->p[n]))) {
        p->ians->data = cs_strdup(csound,
                get_arg_string(csound, csound->init_event->p[n]));
        p->ians->size = strlen(p->ians->data) + 1;
    }
    return OK;
}

int32_t pinit(CSOUND *csound, PINIT *p)
{
    int32_t n;
    int32_t    nargs = p->OUTOCOUNT;
    int32_t    pargs = csound->init_event->pcnt;
    int32_t    start = (int32_t)(*p->start);
    /* Should check that inits exist> */
    int32_t    k = (int32_t)(*p->end);
    if (*p->end!=FL(0.0)) {
      if (k<pargs) pargs = k;
    }
    if (UNLIKELY(nargs>pargs))
      csound->Warning(csound, Str("More arguments than p fields"));
    pargs -= (int)*p->end;
    for (n=0; (n<nargs) && (n<=pargs-start); n++) {
      if (csound->ISSTRCOD(csound->init_event->p[n+start])) {
        ((STRINGDAT *)p->inits[n])->data =
          cs_strdup(csound, get_arg_string(csound, csound->init_event->p[n+start]));
        ((STRINGDAT *)p->inits[n])->size =
          strlen(((STRINGDAT *)p->inits[n])->data)+1;
      }
      else  *p->inits[n] = csound->init_event->p[n+start];
    }
    return OK;
}

#include "arrays.h"
int32_t painit(CSOUND *csound, PAINIT *p)
{
    int32_t n;
    int32_t    pargs = csound->init_event->pcnt;
    int32_t    start = (int32_t)(*p->start);
    int32_t    k = (int32_t)(*p->end);
    if (*p->end!=FL(0.0)) {
      if (k<pargs) pargs = k;
    }
    tabinit(csound, p->inits, pargs-start+1);
    for (n=0; n<=pargs-start; n++) {
      ((MYFLT*)p->inits->data)[n] = csound->init_event->p[n+start];
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "pcount", S(PFIELD),  0, 1, "i", "",       (SUBR)pcount,    NULL, NULL },
{ "pindex", S(PFIELD),  0, 1, "i", "i",      (SUBR)pvalue,    NULL, NULL },
{ "pindex.S", S(PFIELDSTR), 0, 1, "S", "i",  (SUBR)pvaluestr, NULL, NULL },
{ "passign", S(PINIT),  0, 1, "IIIIIIIIIIIIIIIIIIIIIIII", "po",
                                             (SUBR)pinit,     NULL, NULL },
{ "passign.i", S(PAINIT), 0, 1, "i[]", "po",  (SUBR)painit,    NULL, NULL },
{ "passign.k", S(PAINIT), 0, 1, "k[]", "po",  (SUBR)painit,    NULL, NULL },
{ "nlfilt",  S(NLFILT), 0, 3, "a", "akkkkk", (SUBR)nlfiltset, (SUBR)nlfilt },
{ "nlfilt2",  S(NLFILT), 0, 3, "a", "akkkkk", (SUBR)nlfiltset, (SUBR)nlfilt2 }
};

int32_t nlfilt_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
