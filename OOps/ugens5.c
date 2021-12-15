/*
    ugens5.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado

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

#include "csoundCore.h"         /*                      UGENS5.C        */
#include "ugens5.h"
#include <math.h>
#include <inttypes.h>

/*
 * LPC storage slots
 */

#define MAX_LPC_SLOT 20


int32_t porset(CSOUND *csound, PORT *p)
{
   IGN(csound);
    p->c2 = pow(0.5, (double)CS_ONEDKR / *p->ihtim);
    p->c1 = 1.0 - p->c2;
    if (LIKELY(*p->isig >= FL(0.0)))
      p->yt1 = (double)(*p->isig);
    p->ihtim_old = *p->ihtim;
    return OK;
}
int32_t kporset(CSOUND *csound, PORT *p) { return porset(csound, p); }

int32_t port(CSOUND *csound, PORT *p)
{
    IGN(csound);
    p->yt1 = p->c1 * (double)*p->ksig + p->c2 * p->yt1;
    *p->kr = (MYFLT)p->yt1;
    return OK;
}

int32_t kport(CSOUND *csound, KPORT *p)
{
    IGN(csound);
    if (p->ihtim_old != *p->ihtim) {
      p->c2 = pow(0.5, (double)CS_ONEDKR / *p->ihtim);
      p->c1 = 1.0 - p->c2;
      p->ihtim_old = *p->ihtim;
    }
    p->yt1 = p->c1 * (double)*p->ksig + p->c2 * p->yt1;
    *p->kr = (MYFLT)p->yt1;
    return OK;
}

int32_t tonset(CSOUND *csound, TONE *p)
{
    double b;
    p->prvhp = (double)*p->khp;
    b = 2.0 - cos((double)(p->prvhp * csound->tpidsr));
    p->c2 = b - sqrt(b * b - 1.0);
    p->c1 = 1.0 - p->c2;

    if (LIKELY(!(*p->istor)))
      p->yt1 = 0.0;
    return OK;
}

int32_t ktonset(CSOUND *csound, TONE *p) {
    IGN(csound);
    double b;
    p->prvhp = (double)*p->khp;
    b = 2.0 - cos((double)(p->prvhp * CS_ONEDKR *TWOPI));
    p->c2 = b - sqrt(b * b - 1.0);
    p->c1 = 1.0 - p->c2;

    if (LIKELY(!(*p->istor)))
      p->yt1 = 0.0;
    return OK;
 }

int32_t ktone(CSOUND *csound, TONE *p)
{
    IGN(csound);
    double      c1 = p->c1, c2 = p->c2;
    double      yt1 = p->yt1;

    if (*p->khp != (MYFLT)p->prvhp) {
      double b;
      p->prvhp = (double)*p->khp;
      b = 2.0 - cos((double)(p->prvhp * CS_ONEDKR *TWOPI));
      p->c2 = c2 = b - sqrt(b * b - 1.0);
      p->c1 = c1 = 1.0 - c2;
    }
    yt1 = c1 * (double)(*p->asig) + c2 * yt1;
    *p->ar = (MYFLT)yt1;
    p->yt1 = yt1;
    return OK;
}

int32_t tone(CSOUND *csound, TONE *p)
{
    IGN(csound);
    MYFLT       *ar, *asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double      c1 = p->c1, c2 = p->c2;
    double      yt1 = p->yt1;

    if (*p->khp != (MYFLT)p->prvhp) {
      double b;
      p->prvhp = (double)*p->khp;
      b = 2.0 - cos((double)(p->prvhp * csound->tpidsr));
      p->c2 = c2 = b - sqrt(b * b - 1.0);
      p->c1 = c1 = 1.0 - c2;
    }
    ar = p->ar;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      yt1 = c1 * (double)(asig[n]) + c2 * yt1;
      ar[n] = (MYFLT)yt1;
    }
    p->yt1 = yt1;
    return OK;
}

int32_t tonsetx(CSOUND *csound, TONEX *p)
{                   /* From Gabriel Maldonado, modified for arbitrary order */
    {
      double b;
      p->prvhp = *p->khp;
      b = 2.0 - cos((double)(*p->khp * csound->tpidsr));
      p->c2 = b - sqrt(b * b - 1.0);
      p->c1 = 1.0 - p->c2;
    }
    if (UNLIKELY((p->loop = (int32_t) (*p->ord + FL(0.5))) < 1)) p->loop = 4;
    if (!*p->istor && (p->aux.auxp == NULL ||
                    (uint32_t)(p->loop*sizeof(double)) > p->aux.size))
        csound->AuxAlloc(csound, (int32_t)(p->loop*sizeof(double)), &p->aux);
    p->yt1 = (double*)p->aux.auxp;
    if (LIKELY(!(*p->istor))) {
    memset(p->yt1, 0, p->loop*sizeof(double)); /* Punning zero and 0.0 */
    }
    return OK;
}

int32_t tonex(CSOUND *csound, TONEX *p)      /* From Gabriel Maldonado, modified */
{
    MYFLT       *ar = p->ar;
    double      c2 = p->c2, *yt1 = p->yt1,c1 = p->c1;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    int32_t     j, lp = p->loop;

    if (*p->khp != p->prvhp) {
      double b;
      p->prvhp = (double)*p->khp;
      b = 2.0 - cos(p->prvhp * (double)csound->tpidsr);
      p->c2 = b - sqrt(b * b - 1.0);
      p->c1 = 1.0 - p->c2;
    }

    memmove(ar,p->asig,sizeof(MYFLT)*nsmps);
    if (UNLIKELY(offset))  memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (j=0; j< lp; j++) {
      /* Should *yt1 be reset to something?? */
      for (n=0; n<nsmps; n++) {
        double x = c1 * ar[n] + c2 * yt1[j];
        yt1[j] = x;
        ar[n] = (MYFLT)x;
      }
    }
    return OK;
}

int32_t katone(CSOUND *csound, TONE *p)
{
    IGN(csound);
    double     sig, x;
    double      c2 = p->c2, yt1 = p->yt1;

    if (*p->khp != p->prvhp) {
      double b;
      p->prvhp = *p->khp;
      b = 2.0 - cos((double)(*p->khp * CS_ONEDKR *TWOPI));
      p->c2 = c2 = b - sqrt(b * b - 1.0);
/*      p->c1 = c1 = 1.0 - c2; */
    }
      sig = *p->asig;
      x = yt1 = c2 * (yt1 + sig);
      *p->ar = (MYFLT)x;
      yt1 -= sig;               /* yt1 contains yt1-xt1 */

    p->yt1 = yt1;
    return OK;
}


int32_t atone(CSOUND *csound, TONE *p)
{
    MYFLT       *ar, *asig;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    double      c2 = p->c2, yt1 = p->yt1;

    if (*p->khp != p->prvhp) {
      double b;
      p->prvhp = *p->khp;
      b = 2.0 - cos((double)(*p->khp * csound->tpidsr));
      p->c2 = c2 = b - sqrt(b * b - 1.0);
/*      p->c1 = c1 = 1.0 - c2; */
    }
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      double sig = (double)asig[n];
      double x = yt1 = c2 * (yt1 + sig);
      ar[n] = (MYFLT)x;
      yt1 -= sig;               /* yt1 contains yt1-xt1 */
    }
    p->yt1 = yt1;
    return OK;
}

int32_t atonex(CSOUND *csound, TONEX *p)      /* Gabriel Maldonado, modified */
{
    MYFLT       *ar = p->ar;
    double      c2 = p->c2, *yt1 = p->yt1;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    int32_t     j, lp = p->loop;

    if (*p->khp != p->prvhp) {
      double b;
      p->prvhp = *p->khp;
      b = 2.0 - cos((double)(*p->khp * csound->tpidsr));
      p->c2 = b - sqrt(b * b - 1.0);
      /*p->c1 = 1. - p->c2;*/
    }

    memmove(ar,p->asig,sizeof(MYFLT)*nsmps);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (j=1; j<lp; j++) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)ar[n];
        double x = c2 * (yt1[j] + sig);
        yt1[j] = x - sig;            /* yt1 contains yt1-xt1 */
        ar[n] = (MYFLT)x;
      }
    }
    return OK;
}

int32_t rsnset(CSOUND *csound, RESON *p)
{
    int32_t scale;
    p->scale = scale = (int32_t)*p->iscl;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound, Str("illegal reson iscl value, %f"),
                                       *p->iscl);
    }
    p->prvcf = p->prvbw = -100.0;
    if (!(*p->istor))
      p->yt1 = p->yt2 = 0.0;
    p->asigf = IS_ASIG_ARG(p->kcf);
    p->asigw = IS_ASIG_ARG(p->kbw);

    return OK;
}

int32_t krsnset(CSOUND *csound, RESON *p){ return rsnset(csound,p); }

int32_t kreson(CSOUND *csound, RESON *p)
{
    uint32_t flag = 0;
    double      c3p1, c3t4, omc3, c2sqr;
    double      yt0, yt1, yt2, c1 = p->c1, c2 = p->c2, c3 = p->c3;
    IGN(csound);

    if (*p->kcf != (MYFLT)p->prvcf) {
      p->prvcf = (double)*p->kcf;
      p->cosf = cos(p->prvcf * (double)(CS_ONEDKR *TWOPI));
      flag = 1;                 /* Mark as changed */
    }
    if (*p->kbw != (MYFLT)p->prvbw) {
      p->prvbw = (double)*p->kbw;
      c3 = p->c3 = exp(p->prvbw * (double)(-CS_ONEDKR *TWOPI));
      flag = 1;                /* Mark as changed */
    }
    if (flag) {
      c3p1 = c3 + 1.0;
      c3t4 = c3 * 4.0;
      omc3 = 1.0 - c3;
      c2 = p->c2 = c3t4 * p->cosf / c3p1;               /* -B, so + below */
      c2sqr = c2 * c2;
      if (p->scale == 1)
        c1 = p->c1 = omc3 * sqrt(1.0 - c2sqr / c3t4);
      else if (p->scale == 2)
        c1 = p->c1 = sqrt((c3p1*c3p1-c2sqr) * omc3/c3p1);
      else c1 = p->c1 = 1.0;
    }

    yt1 = p->yt1; yt2 = p->yt2;
    yt0 = c1 * ((double)*p->asig) + c2 * yt1 - c3 * yt2;
    *p->ar = (MYFLT)yt0;
    p->yt1 = yt0; p->yt2 = yt1; /* Write back for next cycle */
    return OK;
}


int32_t reson(CSOUND *csound, RESON *p)
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    flag = 0, n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig;
    double      c3p1, c3t4, omc3, c2sqr;
    double      yt1, yt2, c1 = p->c1, c2 = p->c2, c3 = p->c3;
    int32_t     asigf = p->asigf;
    int32_t     asigw = p->asigw;

    asig = p->asig;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    yt1 = p->yt1; yt2 = p->yt2;
    for (n=offset; n<nsmps; n++) {
      double yt0;
      MYFLT cf = asigf ? p->kcf[n] : *p->kcf;
      MYFLT bw = asigw ? p->kbw[n] : *p->kbw;
      if (cf != (MYFLT)p->prvcf) {
        p->prvcf = (double)cf;
        p->cosf = cos(cf * (double)(csound->tpidsr));
        flag = 1;                 /* Mark as changed */
      }
      if (bw != (MYFLT)p->prvbw) {
        p->prvbw = (double)bw;
        c3 = p->c3 = exp(bw * (double)(csound->mtpdsr));
        flag = 1;                /* Mark as changed */
      }
      if (flag) {
        c3p1 = c3 + 1.0;
        c3t4 = c3 * 4.0;
        omc3 = 1.0 - c3;
        c2 = p->c2 = c3t4 * p->cosf / c3p1;               /* -B, so + below */
        c2sqr = c2 * c2;
        if (p->scale == 1)
          c1 = p->c1 = omc3 * sqrt(1.0 - c2sqr / c3t4);
        else if (p->scale == 2)
          c1 = p->c1 = sqrt((c3p1*c3p1-c2sqr) * omc3/c3p1);
        else c1 = p->c1 = 1.0;
        flag = 0;
      }
      yt0 = c1 * ((double)asig[n]) + c2 * yt1 - c3 * yt2;
      ar[n] = (MYFLT)yt0;
      yt2 = yt1;
      yt1 = yt0;
    }
    p->yt1 = yt1; p->yt2 = yt2; /* Write back for next cycle */
    return OK;
}

int32_t rsnsetx(CSOUND *csound, RESONX *p)
{                               /* Gabriel Maldonado, modifies for arb order */
    int32_t scale;
    p->scale = scale = (int32_t) *p->iscl;
    if ((p->loop = (int32_t) (*p->ord + FL(0.5))) < 1)
      p->loop = 4; /* default value */
    if (!*p->istor && (p->aux.auxp == NULL ||
                       (uint32_t)(p->loop*2*sizeof(double)) > p->aux.size))
      csound->AuxAlloc(csound, (int32_t)(p->loop*2*sizeof(double)), &p->aux);
    p->yt1 = (double*)p->aux.auxp; p->yt2 = (double*)p->aux.auxp + p->loop;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound, Str("illegal reson iscl value, %f"),
                                       *p->iscl);
    }
    p->prvcf = p->prvbw = -100.0;

    if (!(*p->istor)) {
      memset(p->yt1, 0, p->loop*sizeof(double));
      memset(p->yt2, 0, p->loop*sizeof(double));
    }
    return OK;
}

int32_t resonx(CSOUND *csound, RESONX *p)   /* Gabriel Maldonado, modified  */
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    flag = 0, n, nsmps = CS_KSMPS;
    int32_t     j;
    MYFLT       *ar;
    double      c3p1, c3t4, omc3, c2sqr;
    double      *yt1, *yt2, c1,c2,c3;
    int32_t     asgf = IS_ASIG_ARG(p->kcf);
    int32_t     asgw = IS_ASIG_ARG(p->kbw);

    ar   = p->ar;
    c1   = p->c1;
    c2   = p->c2;
    c3   = p->c3;
    yt1  = p->yt1;
    yt2  = p->yt2;
    memmove(ar,p->asig,sizeof(MYFLT)*nsmps);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (j=0; j< p->loop; j++) {
      for (n=offset; n<nsmps; n++) {
        double x;
        MYFLT cf = asgf ? p->kcf[n] : *p->kcf;
        MYFLT bw = asgw ? p->kbw[n] : *p->kbw;
        if (cf != (MYFLT)p->prvcf) {
          p->prvcf = (double)cf;
          p->cosf = cos(cf * (double)(csound->tpidsr));
          flag = 1;
        }
        if (bw != (MYFLT)p->prvbw) {
          p->prvbw = (double)bw;
          c3 = exp(bw * (double)(csound->mtpdsr));
          flag = 1;
        }
        if (flag) {
          c3p1 = c3 + 1.0;
          c3t4 = c3 * 4.0;
          omc3 = 1.0 - c3;
          c2 = c3t4 * p->cosf / c3p1;            /* -B, so + below */
          c2sqr = c2 * c2;
          if (p->scale == 1)
            c1 = omc3 * sqrt(1.0 - (c2sqr / c3t4));
          else if (p->scale == 2)
            c1 = sqrt((c3p1*c3p1-c2sqr) * omc3/c3p1);
          else c1 = 1.0;
          flag =0;
        }
        x = c1 * ((double)ar[n]) + c2 * yt1[j] - c3 * yt2[j];
        yt2[j] = yt1[j];
        ar[n] = (MYFLT)x;
        yt1[j] = x;
      }
    }
    p->c1 = c1; p->c2 = c2; p->c3 = c3;
    return OK;
}

int32_t kareson(CSOUND *csound, RESON *p)
{
    uint32_t    flag = 0;
    double      c3p1, c3t4, omc3, c2sqr; //, D = 2.0; /* 1/RMS = root2 (rand) */
                                                   /*      or 1/.5  (sine) */
    double      yt1, yt2, c1, c2, c3;
    IGN(csound);

    if (*p->kcf != (MYFLT)p->prvcf) {
      p->prvcf = (double)*p->kcf;
      p->cosf = cos(p->prvcf * (double)(CS_ONEDKR *TWOPI));
      flag = 1;
    }
    if (*p->kbw != (MYFLT)p->prvbw) {
      p->prvbw = (double)*p->kbw;
      p->c3 = exp(p->prvbw * (double)(-CS_ONEDKR *TWOPI));
      flag = 1;
    }
    if (flag) {
      c3p1 = p->c3 + 1.0;
      c3t4 = p->c3 * 4.0;
      omc3 = 1.0 - p->c3;
      p->c2 = c3t4 * p->cosf / c3p1;
      c2sqr = p->c2 * p->c2;
      if (p->scale == 1)                        /* i.e. 1 - A(reson) */
        p->c1 = 1.0 - omc3 * sqrt(1.0 - c2sqr / c3t4);
      else if (p->scale == 2)                 /* i.e. D - A(reson) */
        p->c1 = 2.0 - sqrt((c3p1*c3p1-c2sqr)*omc3/c3p1);
      else p->c1 = 0.0;                        /* cannot tell        */
    }

    c1 = p->c1; c2 = p->c2; c3 = p->c3; yt1 = p->yt1; yt2 = p->yt2;
    if (p->scale == 1 || p->scale == 0) {
        double sig = (double) *p->asig;
        double ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - sig;  /* yt1 contains yt1-xt1 */
        *p->ar = (MYFLT)ans;
    }
    else if (p->scale == 2) {
        double sig = (double) *p->asig;
        double ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - 2.0 * sig;      /* yt1 contains yt1-D*xt1 */
        *p->ar = (MYFLT)ans;
    }
    p->yt1 = yt1; p->yt2 = yt2;
    return OK;
}

int32_t areson(CSOUND *csound, RESON *p)
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    flag = 0, n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig;
    double      c3p1, c3t4, omc3, c2sqr;//, D = 2.0; /* 1/RMS = root2 (rand) */
                                                   /*      or 1/.5  (sine) */
    double      yt1, yt2, c1, c2, c3;

    if (*p->kcf != (MYFLT)p->prvcf) {
      p->prvcf = (double)*p->kcf;
      p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
      flag = 1;
    }
    if (*p->kbw != (MYFLT)p->prvbw) {
      p->prvbw = (double)*p->kbw;
      p->c3 = exp(p->prvbw * (double)(csound->mtpdsr));
      flag = 1;
    }
    if (flag) {
      c3p1 = p->c3 + 1.0;
      c3t4 = p->c3 * 4.0;
      omc3 = 1.0 - p->c3;
      p->c2 = c3t4 * p->cosf / c3p1;
      c2sqr = p->c2 * p->c2;
      if (p->scale == 1)                        /* i.e. 1 - A(reson) */
        p->c1 = 1.0 - omc3 * sqrt(1.0 - c2sqr / c3t4);
      else if (p->scale == 2)                 /* i.e. D - A(reson) */
        p->c1 = 2.0 - sqrt((c3p1*c3p1-c2sqr)*omc3/c3p1);
      else p->c1 = 0.0;                        /* cannot tell        */
    }
    asig = p->asig;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    c1 = p->c1; c2 = p->c2; c3 = p->c3; yt1 = p->yt1; yt2 = p->yt2;
    if (p->scale == 1 || p->scale == 0) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)asig[n];
        double ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - sig;  /* yt1 contains yt1-xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    else if (p->scale == 2) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)asig[n];
        double ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - 2.0 * sig;      /* yt1 contains yt1-D*xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    p->yt1 = yt1; p->yt2 = yt2;
    return OK;
}

/*
 *
 * LPREAD opcode : initialisation phase
 *
 *
 */

int32_t lprdset_(CSOUND *csound, LPREAD *p, int32_t stringname)
{
    LPHEADER *lph;
    MEMFIL   *mfp;
    int32_t  magic;
    int32_t  totvals;
    char     lpfilname[MAXNAME];

    /* Store adress of opcode for other lpXXXX init to point to */
    if (csound->lprdaddr == NULL ||
        csound->currentLPCSlot >= csound->max_lpc_slot) {
      csound->max_lpc_slot = csound->currentLPCSlot + MAX_LPC_SLOT;
      csound->lprdaddr = csound->ReAlloc(csound,
                                  csound->lprdaddr,
                                  csound->max_lpc_slot * sizeof(LPREAD*));
    }
    ((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot] = p;
    //printf("*** slot %d has value %p\n", csound->currentLPCSlot, p);

    /* Build file name */
    if (stringname) strNcpy(lpfilname, ((STRINGDAT*)p->ifilcod)->data, MAXNAME-1);
    else if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(lpfilname, get_arg_string(csound, *p->ifilcod), MAXNAME-1);
    else csound->strarg2name(csound, lpfilname, p->ifilcod, "lp.", 0);

    /* Do not reload existing file ? */
    if (UNLIKELY((mfp = p->mfp) != NULL && strcmp(mfp->filename, lpfilname) == 0))
      goto lpend;                             /* rtn if file prv known */
    /* Load analysis in memory file */
    /* else read file  */
    if (UNLIKELY((mfp = ldmemfile2withCB(csound, lpfilname, CSFTYPE_LPC, NULL))
                 == NULL)) {
      return csound->InitError(csound, Str("LPREAD cannot load %s"), lpfilname);
    }
    /* Store memory file location in opcode */
    p->mfp = mfp;                                   /*  & record facts   */
    /* Take a peek to the header if exisiting. Else take input arguments */
    lph = (LPHEADER *) mfp->beginp;

    magic=lph->lpmagic;
    if (LIKELY((magic==LP_MAGIC)||(magic==LP_MAGIC2))) {
      p->storePoles = (magic==LP_MAGIC2);

      if(csound->oparms->odebug)
      csound->Message(csound, Str("Using %s type of file.\n"),
                      p->storePoles?Str("pole"):Str("filter coefficient"));
      /* Store header length */
      p->headlen = lph->headersize;
      /* Check if input values where available */
      if (*p->inpoles || *p->ifrmrate) {
        csound->Warning(csound, Str("lpheader overriding inputs"));
      }
      /* Check orc/analysis sample rate compatibility */
      if (lph->srate != csound->esr) {
        csound->Warning(csound, Str("lpfile srate != orch sr"));
      }
      p->npoles = lph->npoles;                /* note npoles, etc. */
      /* Store header info in opcode */
      p->nvals = lph->nvals;
      p->framrat16 = lph->framrate * FL(65536.0);/* scaled framno cvt */
    }
    else if (UNLIKELY(BYTREVL(lph->lpmagic) == LP_MAGIC)) { /* Header reversed: */
      return csound->InitError(csound, Str("file %s bytes are in wrong order"),
                                       lpfilname);
    }
    else {                                    /* No Header on file:*/
      p->headlen = 0;
      p->npoles = (int32_t)*p->inpoles;          /*  data from inargs */
      p->nvals = p->npoles + 4;
      p->framrat16 = *p->ifrmrate * FL(65536.0);
      if (UNLIKELY(!p->npoles || !p->framrat16)) {
        return csound->InitError(csound,
                                 Str("insufficient args and no file header"));
      }
    }
    /* Check pole number */
    csound->AuxAlloc(csound, (int32_t)(p->npoles*8*sizeof(MYFLT)), &p->aux);
    p->kcoefs = (MYFLT*)p->aux.auxp;
    /* if (UNLIKELY(p->npoles > MAXPOLES)) { */
    /*   return csound->InitError(csound, Str("npoles > MAXPOLES")); */
    /* } */
    /* Look for total frame data size (file size - header) */
    totvals = (mfp->length - p->headlen)/sizeof(MYFLT);
    /* Store the size of a frame in integer */
    p->lastfram16 = (((totvals - p->nvals) / p->nvals) << 16) - 1;
    if (UNLIKELY(csound->oparms->odebug))
      csound->Message(csound, Str(
                 "npoles %"PRIi32", nvals %"PRIi32", totvals %"PRIi32
                 ", lastfram16 = %"PRIi32"x\n"),
             p->npoles, p->nvals, totvals, p->lastfram16);
 lpend:
    p->lastmsg = 0;
    return OK;
}

int32_t lprdset(CSOUND *csound, LPREAD *p){
    return lprdset_(csound, p, 0);
}

int32_t lprdset_S(CSOUND *csound, LPREAD *p){
    return lprdset_(csound, p, 1);
}


#ifdef TRACE_POLES
static void
  DumpPolesF(int32_t poleCount, MYFLT *part1, MYFLT *part2,
             int32_t isMagn, char *where)
{
    int32_t i;

    csound->Message(csound, "%s\n", where);
    for (i=0; i<poleCount; i++) {
      if (isMagn)
        csound->Message(csound, Str("magnitude: %f   Phase: %f\n"),
                                part1[i], part2[i]);
      else
        csound->Message(csound, Str("Real: %f   Imag: %f\n"),
                                part1[i], part2[i]);
    }
}
#endif

static void SortPoles(int32_t poleCount, MYFLT *poleMagn, MYFLT *polePhas)
{
    int32_t i, j;
    MYFLT   diff, fTemp;
    int32_t shouldSwap;

    /*  DumpPolesF(poleCount, poleMagn, polePhas, 1, "Before sort"); */

    for (i=1; i<poleCount; i++) {
      for (j=0; j<i; j++) {

        shouldSwap = 0;

        diff = FABS(polePhas[j])-FABS(polePhas[i]);
        if (diff>FL(1.0e-10))
          shouldSwap = 1;
        else if (diff>-FL(1.0e-10)) {
          diff = poleMagn[j]-poleMagn[i];

          if (diff>FL(1.0e-10))
            shouldSwap = 1;
          else if (diff>-FL(1.0e-10))
            {
              if (polePhas[j]>polePhas[i])
                shouldSwap = 1;
            }
        }
        if (shouldSwap) {
          fTemp = poleMagn[i];
          poleMagn[i] = poleMagn[j];
          poleMagn[j] = fTemp;

          fTemp = polePhas[i];
          polePhas[i] = polePhas[j];
          polePhas[j] = fTemp;
        }
      }
    }
/*  DumpPolesF(poleCount, poleMagn, polePhas, 1, "After sort"); */
}

static int32_t DoPoleInterpolation(int poleCount,
                               MYFLT *pm1, MYFLT *pp1,
                               MYFLT *pm2, MYFLT *pp2,
                               MYFLT factor, MYFLT *outMagn, MYFLT *outPhas)
{
    int32_t i;

    if (UNLIKELY(poleCount%2!=0)) {
      printf (Str("Cannot handle uneven pole count yet\n"));
      return (0);
    }

    for (i=0; i<poleCount; i++) {
      if (FABS(FL(FABS(pp1[i])-PI))<FL(1.0e-5)) {
        pm1[i] = -pm1[i];
        pp1[i] = FL(0.0);
      }

      if (FABS(FL(FABS(pp2[i])-PI))<FL(1.0e-5)) {
        pm2[i] = -pm2[i];
        pp2[i] = FL(0.0);
      }
    }

    /* Sort poles according to abs(phase) */

    SortPoles(poleCount, pm1, pp1);
    SortPoles(poleCount, pm2, pp2);

        /*      DumpPolesF(poleCount, pm1, pp1, 1, "Sorted poles 1"); */
        /*      DumpPolesF(poleCount, pm2, pp2, 1, "Sorted poles 2"); */

        /*      printf ("factor=%f\n", factor); */

    for (i=0; i<poleCount; i++) {
      outMagn[i] = pm1[i]+(pm2[i]-pm1[i])*factor;
      outPhas[i] = pp1[i]+(pp2[i]-pp1[i])*factor;
    }

/*     DumpPolesF(poleCount, outMagn, outPhas, 1, "Interpolated poles"); */

    return(1);
}

static inline void InvertPoles(int32_t count, double *real, double *imag)
{
    int32_t    i;
    double     pr,pi,mag;

    for (i=0; i<count; i++) {
      pr = real[i];
      pi = imag[i];
      mag = pr*pr+pi*pi;
      real[i] = pr/mag;
      imag[i] = -pi/mag;
    }
}

/*
 *
 * Resynthetize filter coefficients from poles values
 *
 */

static inline void
    synthetize(int32_t    poleCount,
               double *poleReal, double *poleImag,
               double *polyReal, double *polyImag)
{
    int32_t    j, k;
    double     pr, pi, cr, ci;

    polyReal[0] = 1;
    polyImag[0] = 0;

    for (j=0; j<poleCount; j++) {
      polyReal[j+1] = 1;
      polyImag[j+1] = 0;

      pr = poleReal[j];
      pi = poleImag[j];

      for (k=j; k>=0; k--) {
        cr = polyReal[k];
        ci = polyImag[k];

        polyReal[k] = -(cr*pr-ci*pi);
        polyImag[k] = -(ci*pr+cr*pi);

        if (LIKELY(k>0)) {
            polyReal[k] += polyReal[k-1];
            polyImag[k] += polyImag[k-1];
        }
      }
    }

    /* Makes it 1+a1.x+...+anXn */

    pr = polyReal[0];
    for (j=0; j<=poleCount; j++)
      polyReal[j] /= pr;
}

/*
 *
 * LPREAD k/a time access. This will setup current pole values
 *
 */

int32_t lpread(CSOUND *csound, LPREAD *p)
{
    MYFLT   *bp, *np, *cp;
    int32_t  nn, framphase;
    MYFLT   fract;
    int32_t i, status;
    MYFLT   *poleMagn1 = p->kcoefs + 2*p->npoles;
    MYFLT   *polePhas1 = poleMagn1 + p->npoles;
    MYFLT   *poleMagn2 = polePhas1 + p->npoles;
    MYFLT   *polePhas2 = poleMagn2 + p->npoles;
    MYFLT   *interMagn = polePhas2 + p->npoles;
    MYFLT   *interPhas = interMagn + p->npoles;
    //printf("*** Line %d: q = %p  q->kcoefs=%p\n", __LINE__, p, p->kcoefs);


    if (UNLIKELY(p->mfp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("lpread: not initialised"));
    }
    /* Locate frame position range */
    if (UNLIKELY((framphase = (int32)(*p->ktimpt*p->framrat16)) < 0)) {
      /* for kfram reqd*/
      return csound->PerfError(csound, &(p->h),
                               Str("lpread timpnt < 0"));
    }
    if (framphase > p->lastfram16) {                /* not past last one */
      framphase = p->lastfram16;
      if (UNLIKELY(!p->lastmsg)) {
        p->lastmsg = 1;
        csound->Warning(csound, Str("lpread ktimpnt truncated to last frame"));
      }
    }
    /* Locate frames bounding current time */
    bp = (MYFLT *)(p->mfp->beginp + p->headlen); /* locate begin frame data */
    nn = (framphase >> 16) * p->nvals;
    bp = bp + nn;                                /* locate begin this frame */
    np = bp + p->nvals;                          /* & interp betw adj frams */
    fract = (framphase & 0x0FFFFL) / FL(65536.0);
    /* Interpolate freq/amplpitude and store in opcode */
    *p->krmr = *bp + (*np - *bp) * fract;   bp++;   np++; /* for 4 rslts */
    *p->krmo = *bp + (*np - *bp) * fract;   bp++;   np++;
    *p->kerr = *bp + (*np - *bp) * fract;   bp++;   np++;
    *p->kcps = *bp + (*np - *bp) * fract;   bp++;   np++;

   /* Interpolate filter or poles coef values and store in opcode */

    cp = p->kcoefs;      /* This is where the coefs get stored */
    if (p->storePoles) {
      for (i=0; i<p->npoles; i++) {
        poleMagn1[i] = *bp++;
        polePhas1[i] = *bp++;
        poleMagn2[i] = *np++;
        polePhas2[i] = *np++;
      }

      status =
        DoPoleInterpolation(p->npoles,poleMagn1,polePhas1,poleMagn2,
                            polePhas2,fract,interMagn,interPhas);
      if (UNLIKELY(!status)) {
        return csound->PerfError(csound, &(p->h),
                                 Str("Interpolation failed"));
      }
      for (i=0; i<p->npoles; i++) {
        *cp++ =  interMagn[i];
        *cp++ =  interPhas[i];
      }
    }
    else {
      for (nn = 0; nn< p->npoles; nn++) {
        cp[nn] = bp[nn] + (np[nn] - bp[nn]) * fract;
      }
    }
/*  if (csound->oparms->odebug) {
      csound->Message(csound,
          "phase:%lx fract:%6.2f rmsr:%6.2f rmso:%6.2f kerr:%6.2f kcps:%6.2f\n",
          framphase,fract,*p->krmr,*p->krmo,*p->kerr,*p->kcps);
      cp = p->kcoefs;
      nn = p->npoles;
      do {
        csound->Message(csound, " %6.2f",*cp++);
      } while (--nn);
      csound->Message(csound, "\n");
    }  */
    return OK;
}


int32_t lpformantset(CSOUND *csound, LPFORM *p)
{
    LPREAD *q;

   /* connect to previously loaded lpc analysis */
   /* get adr lpread struct */
    p->lpread = q = ((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot];
    csound->AuxAlloc(csound, p->lpread->npoles*sizeof(MYFLT), &p->aux);
    return OK;
}

int32_t lpformant(CSOUND *csound, LPFORM *p)
{
    LPREAD  *q = p->lpread;
    MYFLT   *coefp, sr = csound->esr;
    MYFLT   *cfs = (MYFLT*)p->aux.auxp;
    MYFLT   *bws = cfs+p->lpread->npoles/2;
    int32_t i, j, ndx = *p->kfor;
    double  pm,pp;

    if (q->storePoles) {
      coefp = q->kcoefs;
      for (i=2,j=0; i<q->npoles*2; i+=4, j++) {
        pm = coefp[i];
        pp = coefp[i+1];
        cfs[j] = pp*sr/TWOPI;
        /* if (pm > 1.0) csound->Message(csound,
                                        Str("warning unstable pole %f\n"), pm); */
        bws[j] = -LOG(pm)*sr/PI;
      }
    }
    else {
      csound->PerfError(csound, &(p->h),
                        Str("this opcode only works with LPC "
                            "pole analysis type (-a)\n"));
      return NOTOK;
    }

    j = (ndx < 1 ? 1 :
         (ndx >= p->lpread->npoles/2 ? p->lpread->npoles/2 : ndx)) - 1;
    if (bws[j] > sr/2 || isnan(bws[j])) bws[j] = sr/2;
    if (bws[j] < 1.0) bws[j] = 1.0;
    if (cfs[j] > sr/2 || isnan(cfs[j])) cfs[j] = sr/2;
    cfs[j] = FABS(cfs[j]);
    *p->kcf = cfs[j];
    *p->kbw = bws[j];

    return OK;
}


/*
 *
 * LPRESON: initialisation time
 *
 *
 */
int32_t lprsnset(CSOUND *csound, LPRESON *p)
{
    LPREAD *q;

   /* connect to previously loaded lpc analysis */
   /* get adr lpread struct */

    p->lpread = q = ((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot];
    csound->AuxAlloc(csound, (int32)((q->npoles<<1)*sizeof(MYFLT)), &p->aux);
   /* Initialize pointer to circular buffer (for filtering) */
    p->circjp = p->circbuf = (MYFLT*)p->aux.auxp;
    p->jp2lim = p->circbuf + (q->npoles << 1);  /* npoles det circbuflim */
    return OK;
}

/*
 *
 * LPRESON: k & a time access. Will actually filter the signal
 *                  Uses a circular buffer to store previous signal values.
 */

int32_t lpreson(CSOUND *csound, LPRESON *p)
{
    IGN(csound);
    LPREAD   *q = p->lpread;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT    *coefp, *pastp, *jp, *jp2, *rslt = p->ar, *asig = p->asig;
    MYFLT    x;
    double   poleReal[MAXPOLES], poleImag[MAXPOLES];
    double   polyReal[MAXPOLES+1], polyImag[MAXPOLES+1];
    int32_t  i, nn;
    double   pm,pp;

    jp = p->circjp;
    jp2 = jp + q->npoles;

    /* If we were using poles, we have to compute filter coefs now */
    if (q->storePoles) {
      coefp = q->kcoefs;
      for (i=0; i<q->npoles; i++) {
        pm = *coefp++;
        pp = *coefp++;
        /*       csound->Message(csound, "pole %d, fr=%.2f, BW=%.2f\n", i,
                        pp*(csound->esr)/6.28, -csound->esr*log(pm)/3.14);
        */
        if (fabs(pm)>0.999999)
          pm = 1/pm;
        poleReal[i] = pm*cos(pp);
        poleImag[i] = pm*sin(pp);
      }
      /*     DumpPoles(q->npoles,poleReal,poleImag,0,"About to filter"); */
      InvertPoles(q->npoles,poleReal,poleImag);
      synthetize(q->npoles,poleReal,poleImag,polyReal,polyImag);
      coefp = q->kcoefs;
      for (i=0; i<q->npoles; i++) {
        /* MR_WHY - somthing with the atan2 ? */
        coefp[i] = -(MYFLT)polyReal[q->npoles-i];
      }
    }

    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* For each sample */
    for (n=offset; n<nsmps; n++) {
      /* Compute Xn = Yn + CkXn-k */

#ifdef TRACE_FILTER
      csound->Message(csound, "Asig=%f\n", *asig);
#endif
      x = asig[n];
      coefp = q->kcoefs;              /* using lpread interp coefs */
      pastp = jp;
      nn = q->npoles;
      do {
#ifdef TRACE_FILTER
        csound->Message(csound, "\t%f,%f\n", *coefp, *pastp);
#endif
        x += *coefp++ * *pastp++;
      } while (--nn);
#ifdef TRACE_FILTER
      csound->Message(csound, "result=%f\n", x);
#endif
      /* Store result signal in circular and output buffers */

      *jp++ = *jp2++ = x;
      rslt[n] = x;

      /* Check if end of buffer reached */
      if (jp2 >= p->jp2lim) {
        jp2 = jp;
        jp = p->circbuf;
      }
    }
    p->circjp = jp;
    return OK;
}

/*
 *
 * LPFRESON : Initialisation time
 *
 */
int32_t lpfrsnset(CSOUND *csound, LPFRESON *p)
{

   /* Connect to previously loaded analysis file */

    if (((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot]->storePoles) {
      return csound->InitError(csound, Str("Pole file not supported "
                                           "for this opcode !"));
    }


    p->lpread = ((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot];
    if(p->lpread->npoles < 2) {
      return csound->InitError(csound, Str("Too few poles (> 2)"));
    }

    p->prvratio = FL(1.0);
    p->d = FL(0.0);
    p->prvout = FL(0.0);
    csound->AuxAlloc(csound, (int32)(p->lpread->npoles*sizeof(MYFLT)), &p->aux);
    p->past = (MYFLT*)p->aux.auxp;

    return OK;
}

/*
 *
 * LPFRESON : k & a time : actually filters the data
 *
 */
int32_t lpfreson(CSOUND *csound, LPFRESON *p)
{
    LPREAD   *q = p->lpread;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, n, nsmps = CS_KSMPS;
    MYFLT    *coefp, *pastp, *pastp1, *rslt = p->ar, *asig = p->asig;
    MYFLT    x, temp1, temp2, ampscale, cq;

    if (*p->kfrqratio != p->prvratio) {             /* for new freqratio */
      if (*p->kfrqratio <= FL(0.0)) {
        return csound->PerfError(csound, &(p->h),
                                 Str("illegal frqratio, %5.2f"),
                                         *p->kfrqratio);
      }                                             /*      calculate d  */
      p->d = (*p->kfrqratio - FL(1.0)) / (*p->kfrqratio + FL(1.0));
      p->prvratio = *p->kfrqratio;
    }
    if (p->d != FL(0.0)) {                          /* for non-zero d,   */
      coefp = q->kcoefs;
      nn = q->npoles - 1;
      do {
        temp1 = p->d * *coefp++;                    /*    shift formants */
        *coefp += temp1;
      }
      while (--nn);
      ampscale = FL(1.0) / (FL(1.0) - p->d * *coefp); /*    & reset scales */
      cq = (FL(1.0) - p->d * p->d) * ampscale;
    }
    else {
      cq = FL(1.0);
      ampscale = FL(1.0);
    }
    x = p->prvout;
    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      nn = q->npoles - 1;
      pastp  = pastp1 = p->past + nn;
      temp1 = *pastp;
      *pastp = cq * x - p->d * *pastp;
      pastp--;
      do {
        temp2 = *pastp;
        *pastp = (*pastp1 - *pastp) * p->d + temp1;
        pastp--;   pastp1--;
        temp1 = temp2;
      } while (--nn);
      x = asig[n];
      pastp = p->past;
      coefp = q->kcoefs;
      nn = q->npoles;
      do  {
        x += *coefp++ * *pastp++;
      } while (--nn);
      rslt[n] = x * ampscale;
    }
    p->prvout = x;
    return OK;
}

int32_t rmsset(CSOUND *csound, RMS *p)
{
    double   b;

    b = 2.0 - cos((double)(*p->ihp * csound->tpidsr));
    p->c2 = b - sqrt(b*b - 1.0);
    p->c1 = 1.0 - p->c2;
    if (!*p->istor)
      p->prvq = 0.0;
    return OK;
}

int32_t gainset(CSOUND *csound, GAIN *p)
{
    double   b;

    b = 2.0 - cos((double)(*p->ihp * csound->tpidsr));
    p->c2 = b - sqrt(b*b - 1.0);
    p->c1 = 1.0 - p->c2;
    if (!*p->istor)
      p->prvq = p->prva = 0.0;
    return OK;
}

int32_t balnset(CSOUND *csound, BALANCE *p)
{
    double   b;

    b = 2.0 - cos((double)(*p->ihp * csound->tpidsr));
    p->c2 = b - sqrt(b*b - 1.0);
    p->c1 = 1.0 - p->c2;
    if (!*p->istor)
      p->prvq = p->prvr = p->prva = 0.0;
    return OK;
}

int32_t rms(CSOUND *csound, RMS *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT    *asig;
    double   q;
    double   c1 = p->c1, c2 = p->c2;

    q = p->prvq;
    asig = p->asig;
    if (UNLIKELY(early)) nsmps -= early;
    for (n=offset; n<nsmps; n++) {
      double as = (double)asig[n];
      q = c1 * as * as + c2 * q;
    }
    p->prvq = q;
    *p->kr = (MYFLT) sqrt(q);
    return OK;
}

int32_t gain(CSOUND *csound, GAIN *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT    *ar, *asig;
    double   q, a, m, diff, inc;
    double   c1 = p->c1, c2 = p->c2;

    q = p->prvq;
    asig = p->asig;
    if (UNLIKELY(early)) nsmps -= early;
    for (n = offset; n < nsmps-early; n++) {
      double as = (double)asig[n];
      q = c1 * as * as + c2 * q;
    }
    p->prvq = q;
    if (q > 0.0)
      a = *p->krms / sqrt(q);
    else
      a = *p->krms;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if ((diff = a - p->prva) != 0.0) {
      m = p->prva;
      inc = diff / (double)(nsmps-offset);
      for (n = offset; n < nsmps; n++) {
        ar[n] = asig[n] * m;
        m += inc;
      }
      p->prva = a;
    }
    else {
      for (n = offset; n < nsmps; n++) {
        ar[n] = asig[n] * a;
      }
    }
    return OK;
}

int32_t balance(CSOUND *csound, BALANCE *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT    *ar, *asig, *csig;
    double   q, r, a, m, diff, inc;
    double   c1 = p->c1, c2 = p->c2;

    q = p->prvq;
    r = p->prvr;
    asig = p->asig;
    csig = p->csig;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      double as = (double)asig[n];
      double cs = (double)csig[n];
      q = c1 * as * as + c2 * q;
      r = c1 * cs * cs + c2 * r;
    }
    p->prvq = q;
    p->prvr = r;
    if (LIKELY(q != 0.0))
      a = sqrt(r/q);
    else
      a = sqrt(r);
    if ((diff = a - p->prva) != 0.0) {
      m = p->prva;
      inc = diff / (double)(nsmps-offset);
      for (n = offset; n < nsmps; n++) {
        ar[n] = asig[n] * m;
        m += inc;
      }
      p->prva = a;
    }
    else {
      for (n = offset; n < nsmps; n++) {
        ar[n] = asig[n] * a;
      }
    }
    return OK;
}


int32_t balance2(CSOUND *csound, BALANCE *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT    *ar, *asig, *csig;
    double   q, r, a;
    double   c1 = p->c1, c2 = p->c2;

    q = p->prvq;
    r = p->prvr;
    asig = p->asig;
    csig = p->csig;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      double as = (double)asig[n];
      double cs = (double)csig[n];
      q = c1 * as * as + c2 * q;
      r = c1 * cs * cs + c2 * r;
      if (LIKELY(q != 0.0))
        a = sqrt(r/q);
      else
        a = sqrt(r);
      ar[n] = asig[n] * a;
    }
    p->prvq = q;
    p->prvr = r;
    return OK;
}

/*
 *   Set current lpc slot
 */
int32_t lpslotset(CSOUND *csound, LPSLOT *p)
{
    int32_t n;

    n = (int32_t) *(p->islotnum);
    if (UNLIKELY(n < 0))
      return csound->InitError(csound, Str("lpslot number should be positive"));
    else {
      if (n >= csound->max_lpc_slot) {
        csound->max_lpc_slot = n + MAX_LPC_SLOT;
        csound->lprdaddr = csound->ReAlloc(csound,
                                    csound->lprdaddr,
                                    csound->max_lpc_slot * sizeof(LPREAD**));
      }
      csound->currentLPCSlot = n;
    }
    return OK;
}

int32_t lpitpset(CSOUND *csound, LPINTERPOL *p)
{

    if (UNLIKELY((uint32_t) ((int32_t) *(p->islot1))
                 >= (uint32_t) csound->max_lpc_slot ||
                 (uint32_t) ((int32_t) *(p->islot2))
                 >= (uint32_t) csound->max_lpc_slot))
      return csound->InitError(csound, Str("LPC slot is not allocated"));
  /* Get lpread pointers */
    p->lp1 = ((LPREAD**) csound->lprdaddr)[(int32_t) *(p->islot1)];
    p->lp2 = ((LPREAD**) csound->lprdaddr)[(int32_t) *(p->islot2)];

  /* Check if workable */

    if (UNLIKELY((!p->lp1->storePoles) || (!p->lp2->storePoles))) {
      return csound->InitError(csound, Str("lpinterpol works only "
                                           "with poles files.."));
    }
    if (UNLIKELY(p->lp1->npoles != p->lp2->npoles)) {
      return csound->InitError(csound, Str("The poles files "
                                           "have different pole count"));
    }

#if 0                   /* This is incorrect C */
    if (&p->kcoefs-p != &p->lp1->kcoefs-p->lp1)
      return csound->InitError(csound, Str("padding error"));
#endif

    p->npoles = p->lp1->npoles;
    csound->AuxAlloc(csound, (int32)(p->npoles*8*sizeof(MYFLT)), &p->aux);
    p->kcoefs = (MYFLT*)p->aux.auxp;
    p->storePoles = 1;
    {
      LPREAD *q;
      csound->AuxAlloc(csound, sizeof(LPREAD), &p->slotaux);
      q = (LPREAD*)p->slotaux.auxp;
      memcpy(q, p, sizeof(LPREAD));
      q->kcoefs = p->kcoefs;
      q->storePoles = 1;
      //csound->currentLPCSlot++; ?? Or othr way to create a slot
      // Following code shoukd not be necessary
      if (csound->lprdaddr == NULL ||
        csound->currentLPCSlot >= csound->max_lpc_slot) {
      csound->max_lpc_slot = csound->currentLPCSlot + MAX_LPC_SLOT;
      csound->lprdaddr = csound->ReAlloc(csound,
                                  csound->lprdaddr,
                                  csound->max_lpc_slot * sizeof(LPREAD*));
      }
      ((LPREAD**) csound->lprdaddr)[csound->currentLPCSlot] = q;
    }
    return OK;
}

int32_t lpinterpol(CSOUND *csound, LPINTERPOL *p)
{
    int32_t i,status;
    MYFLT   *cp,*cp1,*cp2;
    MYFLT   *poleMagn1 = p->kcoefs + 2*p->npoles;
    MYFLT   *polePhas1 = poleMagn1 + p->npoles;
    MYFLT   *poleMagn2 = polePhas1 + p->npoles;
    MYFLT   *polePhas2 = poleMagn2 + p->npoles;
    MYFLT   *interMagn = polePhas2 + p->npoles;
    MYFLT   *interPhas = interMagn + p->npoles;

    /* RWD: guessing this... */
    if (UNLIKELY(p->lp1==NULL || p->lp2==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("lpinterpol: not initialised"));
    }
    cp1 =  p->lp1->kcoefs;
    cp2 =  p->lp2->kcoefs;

    for (i=0; i<p->npoles; i++) {
      poleMagn1[i] = *cp1++;
      polePhas1[i] = *cp1++;
      poleMagn2[i] = *cp2++;
      polePhas2[i] = *cp2++;
    }

    status = DoPoleInterpolation(p->npoles,poleMagn1,polePhas1,poleMagn2,
                                     polePhas2,*p->kmix,interMagn,interPhas);
    if (UNLIKELY(!status)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Interpolation failed"));
    }

    cp = p->kcoefs;      /* This is where the coefs get stored */
    for (i=0; i<p->npoles; i++) {
      *cp++ = interMagn[i];
      *cp++ = interPhas[i];
    }
    return OK;
}


int32_t klimit(CSOUND *csound, LIMIT *p)
{
    IGN(csound);
    MYFLT       sig=*p->sig, min=*p->min, max=*p->max;
    if (LIKELY((sig <= max) && (sig >= min))) {
      *p->ans = sig;
    }
    else {
     if ( min >= max) {
        *p->ans = FL(0.5) * (min + max);
      }
      else {
        if (sig > max)
          *p->ans = max;
        else
          *p->ans = min;
      }
    }
    return OK;
}

int32_t limit(CSOUND *csound, LIMIT *p)
{
    IGN(csound);
    MYFLT       *ans, *asig;
    MYFLT       min=*p->min, max=*p->max, aver;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    ans = p->ans;
    asig  = p->sig;

    if (UNLIKELY(offset)) memset(ans, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (min >= max) {
      aver = (min + max) * FL(0.5);
      for (n=offset; n<nsmps; n++) {
        ans[n] = aver;
      }
    }
    else
      for (n=offset; n<nsmps; n++) {
        if ((asig[n] <= max) && (asig[n] >= min)) {
          ans[n] = asig[n];
        }
        else {
          if (asig[n] > max)
            ans[n] = max;
          else
            ans[n] = min;
        }
      }
    return OK;
}
