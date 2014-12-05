/*
    lowpassr.c:

    Copyright (C) 1998 Gabriel Maldonado

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

/* Resonant Lowpass filters by G.Maldonado */

#include "stdopcod.h"
#include "lowpassr.h"
#include <math.h>

static int lowpr_set(CSOUND *csound, LOWPR *p)
{
    if (*p->istor==FL(0.0))
      p->ynm1 = p->ynm2 = 0.0;
    p->okf = 0.0;
    p->okr = 0.0;
    p->k = 0.0;
    return OK;
}

static int lowpr(CSOUND *csound, LOWPR *p)
{
    double b, k = p->k;
    MYFLT *ar, *asig;
    double yn, ynm1, ynm2 ;
    MYFLT kfco = *p->kfco;
    MYFLT kres = *p->kres;
    double coef1 = p->coef1, coef2 = p->coef2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (p->okf != kfco || p->okr != kres) { /* Only if changed */
      if (kfco<=FL(0.0))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Cutoff parameter must be positive"));
      b = 10.0 / (kres * sqrt((double)kfco)) - 1.0;
      p->k = k = 1000.0 / (double)kfco;
      p->coef1 = coef1 = (b+2.0 * k);
      p->coef2 = coef2 = 1.0/(1.0 + b + k);
      p->okf = kfco; p->okr = kres; /* remember to save recalculation */
    }
    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps;n++) {
      ar[n] = (MYFLT)(yn = (coef1 * ynm1 - k * ynm2 + (double)asig[n]) * coef2);
      ynm2 = ynm1;
      ynm1 =  yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */

    return OK;
}

static int lowpraa(CSOUND *csound, LOWPR *p)
{
    double b, k = p->k;
    MYFLT *ar, *asig;
    double yn, ynm1, ynm2 ;
    MYFLT *fco = p->kfco;
    MYFLT *res = p->kres;
    MYFLT okf = p->okf, okr = p->okr;
    double coef1 = p->coef1, coef2 = p->coef2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (okf!= fco[0] || okr != res[0]) { /* Only if changed */
      if (fco[0]<=FL(0.0))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Cutoff parameter must be positive"));
      b = 10.0 / (res[0] * sqrt((double)fco[0])) - 1.0;
      p->k = k = 1000.0 / (double)fco[0];
      p->coef1 = coef1 = (b+2.0 * k);
      p->coef2 = coef2 = 1.0/(1.0 + b + k);
      okf = fco[0]; okr = res[0];
      /* remember to save recalculation */
    }
    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps;n++) {
      if (okf!= fco[n] || okr != res[n]) { /* Only if changed */
        if (fco[n]<=FL(0.0))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Cutoff parameter must be positive"));
        b = 10.0 / (res[n] * sqrt((double)fco[n])) - 1.0;
        p->k = k = 1000.0 / (double)fco[0];
        p->coef1 = coef1 = (b+2.0 * k);
        p->coef2 = coef2 = 1.0/(1.0 + b + k);
        okf = fco[n]; okr = res[n];
        /* remember to save recalculation */
      }
      ar[n] = (MYFLT)(yn = (coef1 * ynm1 - k * ynm2 + (double)asig[n]) * coef2);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;
    return OK;
}

static int lowprak(CSOUND *csound, LOWPR *p)
{
    double b, k = p->k;
    MYFLT *ar, *asig;
    double yn, ynm1, ynm2 ;
    MYFLT *fco = p->kfco;
    MYFLT kres = *p->kres;
    MYFLT okf = p->okf, okr = p->okr;
    double coef1 = p->coef1, coef2 = p->coef2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (okf != fco[0] || okr != kres) { /* Only if changed */
      if (fco[0]<=FL(0.0))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("Cutoff parameter must be positive"));
      b = 10.0 / (kres * sqrt((double)fco[0])) - 1.0;
      p->k = k = 1000.0 / (double)fco[0];
      p->coef1 = coef1 = (b+2.0 * k);
      p->coef2 = coef2 = 1.0/(1.0 + b + k);
      okf = fco[0]; okr = kres; /* remember to save recalculation */
    }
    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps;n++) {
      if (okf != fco[n]) { /* Only if changed */
        if (fco[n]<=FL(0.0))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("Cutoff parameter must be positive"));
        b = 10.0 / (kres * sqrt((double)fco[n])) - 1.0;
        p->k = k = 1000.0 / (double)fco[n];
        p->coef1 = coef1 = (b+2.0 * k);
        p->coef2 = coef2 = 1.0/(1.0 + b + k);
        okf = fco[n]; /* remember to save recalculation */
      }
      ar[n] = (MYFLT)(yn = (coef1 * ynm1 - k * ynm2 + (double)asig[n]) * coef2);
      ynm2 = ynm1;
      ynm1 =  yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;

    return OK;
}

static int lowprka(CSOUND *csound, LOWPR *p)
{
    double b, k = p->k;
    MYFLT *ar, *asig;
    double yn, ynm1, ynm2 ;
    MYFLT fco = *p->kfco;
    MYFLT *res = p->kres;
    MYFLT okr = p->okr, okf = p->okf;
    double coef1 = p->coef1, coef2 = p->coef2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (okf!= fco || okr != res[0]) { /* Only if changed */
        if (fco<=FL(0.0))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("Cutoff parameter must be positive"));
      b = 10.0 / (res[0] * sqrt((double)fco)) - 1.0;
      p->k = k = 1000.0 / (double)fco;
      p->coef1 = coef1 = (b+2.0 * k);
      p->coef2 = coef2 = 1.0/(1.0 + b + k);
      okf = fco; okr = res[0]; /* remember to save recalculation */
    }
    ar = p->ar;
    asig = p->asig;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps;n++) {
      // ****Optimise by remembering okf/okr
      if (okr != res[n]) { /* Only if changed */
        b = 10.0 / (res[n] * sqrt((double)fco)) - 1.0;
        p->k = k = 1000.0 / (double)fco;
        p->coef1 = coef1 = (b+2.0 * k);
        p->coef2 = coef2 = 1.0/(1.0 + b + k);
        okr = res[n]; /* remember to save recalculation */
      }
      ar[n] = (MYFLT)(yn = (coef1 * ynm1 - k * ynm2 + (double)asig[n]) * coef2);
      ynm2 = ynm1;
      ynm1 = yn;
    }
    p->ynm1 = ynm1;
    p->ynm2 = ynm2;             /* And save */
    p->okf = okf; p->okr = okr;

    return OK;
}

static int lowpr_setx(CSOUND *csound, LOWPRX *p)
{
    int j;
    if ((p->loop = (int) MYFLT2LONG(*p->ord)) < 1) p->loop = 4; /*default value*/
    else if (UNLIKELY(p->loop > 10)) {
      return csound->InitError(csound, Str("illegal order num. (min 1, max 10)"));
    }
    if (*p->istor == FL(0.0))
      for (j=0; j< p->loop; j++)  p->ynm1[j] = p->ynm2[j] = FL(0.0);
    p->k = p->okf = p->okr = -FL(1.0);
    return OK;
}

static int lowprx(CSOUND *csound, LOWPRX *p)
{
    MYFLT    b, k = p->k;
    MYFLT   *ar, *asig, yn,*ynm1, *ynm2 ;
    MYFLT    coef1 = p->coef1, coef2 = p->coef2;
    MYFLT    *kfco = p->kfco, *kres = p->kres;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j;


    ynm1 = p->ynm1;
    ynm2 = p->ynm2;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (j=0; j< p->loop; j++) {
      ar = p->ar;

      for (n=offset;n<nsmps;n++) {
        MYFLT fco = (IS_ASIG_ARG(p->kfco) ? kfco[n] : *kfco);
        MYFLT res = (IS_ASIG_ARG(p->kres) ? kres[n] : *kres);
        if (p->okf != fco || p->okr != res) { /* Only if changed */
          b = FL(10.0) / (res * SQRT(fco)) - FL(1.0);
          k = FL(1000.0) / fco;
          coef1 = (b+FL(2.0) * k);
          coef2 = FL(1.0)/(FL(1.0) + b + k);
          p->okf = fco; p->okr = res; /* remember to save recalculation */
        }
        ar[n] = yn = (coef1 * ynm1[j] - k * ynm2[j] + asig[n]) * coef2;
        ynm2[j] = ynm1[j];
        ynm1[j] = yn;
      }
      asig= p->ar;
    }
    p->k = k;
    p->coef1 = coef1;
    p->coef2 = coef2;
    return OK;
}

static int lowpr_w_sep_set(CSOUND *csound, LOWPR_SEP *p)
{
    int j;
    if ((p->loop = (int) MYFLT2LONG(*p->ord)) < 1)
      p->loop = 4; /*default value*/
    else if (UNLIKELY(p->loop > 10)) {
      return csound->InitError(csound, Str("illegal order num. (min 1, max 10)"));
    }
    for (j=0; j< p->loop; j++)  p->ynm1[j] = p->ynm2[j] = FL(0.0);
    return OK;
}

static int lowpr_w_sep(CSOUND *csound, LOWPR_SEP *p)
{
    MYFLT    b, k;
    MYFLT   *ar, *asig, yn,*ynm1, *ynm2 ;
    MYFLT    coef1, coef2;
    MYFLT    kfcobase = *p->kfco;
    MYFLT    sep = (*p->sep / p->loop);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j;

    MYFLT kres = *p->kres;
    MYFLT kfco;
    ynm1 = p->ynm1;
    ynm2 = p->ynm2;
    asig = p->asig;

    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    ar = p->ar;
    for (j=0; j< p->loop; j++) {
      MYFLT lynm1 = ynm1[j];
      MYFLT lynm2 = ynm2[j];
                /*
                linfco=log((double) kfco)*ONEtoLOG2     ;
                linfco = linfco + (sep / p->loop)*j;
                kfco = (MYFLT) pow(2.0,linfco);
                */
      kfco = kfcobase * (FL(1.0) + (sep * j));

      b = FL(10.0) / ( kres * (MYFLT)sqrt((double)kfco)) - FL(1.0);
      k = FL(1000.0) / kfco;
      coef1 = (b+FL(2.0) *k);
      coef2 = FL(1.0)/(FL(1.0) + b + k);

      for (n=offset;n<nsmps; n++) {
        /* This can be speeded up avoiding indirection */
        ar[n] = yn = (coef1 * lynm1 - k * lynm2 + asig[n]) * coef2;
        lynm2 = lynm1;
        lynm1 =  yn;
      }
      ynm1[j] = lynm1;
      ynm2[j] = lynm2;
      asig= p->ar;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "lowres.kk",   S(LOWPR),   0, 5, "a", "akko",
                          (SUBR)lowpr_set, NULL,   (SUBR)lowpr   },
{ "lowres.aa",   S(LOWPR),   0, 5, "a", "aaao",
                          (SUBR)lowpr_set, NULL,   (SUBR)lowpraa },
{ "lowres.ak",   S(LOWPR),   0, 5, "a", "aako",
                          (SUBR)lowpr_set, NULL,   (SUBR)lowprak },
{ "lowres.ka",   S(LOWPR),   0, 5, "a", "akao",
                          (SUBR)lowpr_set, NULL,   (SUBR)lowprka },
{ "lowresx",  S(LOWPRX),  0, 5, "a", "axxoo",
                          (SUBR)lowpr_setx, NULL, (SUBR)lowprx   },
{ "vlowres", S(LOWPR_SEP),0, 5, "a", "akkik",
                          (SUBR)lowpr_w_sep_set, NULL, (SUBR)lowpr_w_sep }
};

int lowpassr_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}
