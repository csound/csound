/*
    afilters.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"         /*                      AFILTERS.C        */
#include "ugens5.h"
#include <math.h>

extern int rsnset(CSOUND *csound, RESON *p);

static int aresonaa(CSOUND *csound, RESON *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t flag = 0, n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig;
    double      c3p1, c3t4, omc3, c2sqr; /* 1/RMS = root2 (rand) */
                                         /*      or 1/.5  (sine) */
    double      yt1, yt2, c1, c2, c3;

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
        double ans;
        if (p->kcf[n] != (MYFLT)p->prvcf) {
          p->prvcf = (double)p->kcf[n];
          p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
          flag = 1;
        }
        if (p->kbw[n] != (MYFLT)p->prvbw) {
          p->prvbw = (double)p->kbw[n];
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - sig;  /* yt1 contains yt1-xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    else if (p->scale == 2) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)asig[n];
        double ans;
        if (p->kcf[n] != (MYFLT)p->prvcf) {
          p->prvcf = (double)p->kcf[n];
          p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
          flag = 1;
        }
        if (p->kbw[n] != (MYFLT)p->prvbw) {
          p->prvbw = (double)p->kbw[n];
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - 2.0 * sig;      /* yt1 contains yt1-D*xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    p->yt1 = yt1; p->yt2 = yt2;
    return OK;
}

static int aresonak(CSOUND *csound, RESON *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig;
    double      c3p1, c3t4, omc3, c2sqr; /* 1/RMS = root2 (rand) */
                                         /*      or 1/.5  (sine) */
    double      yt1, yt2, c1, c2, c3;

    if (*p->kbw != (MYFLT)p->prvbw) {
      p->prvbw = (double)*p->kbw;
      p->c3 = exp(p->prvbw * (double)(csound->mtpdsr));
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
        double ans;
        if (p->kcf[n] != (MYFLT)p->prvcf) {
          p->prvcf = (double)p->kcf[n];
          p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - sig;  /* yt1 contains yt1-xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    else if (p->scale == 2) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)asig[n];
        double ans;
        if (p->kcf[n] != (MYFLT)p->prvcf) {
          p->prvcf = (double)p->kcf[n];
          p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - 2.0 * sig;      /* yt1 contains yt1-D*xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    p->yt1 = yt1; p->yt2 = yt2;
    return OK;
}

static int aresonka(CSOUND *csound, RESON *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *asig;
    double      c3p1, c3t4, omc3, c2sqr; /* 1/RMS = root2 (rand) */
                                         /*      or 1/.5  (sine) */
    double      yt1, yt2, c1, c2, c3;

    if (*p->kcf != (MYFLT)p->prvcf) {
      p->prvcf = (double)*p->kcf;
      p->cosf = cos(p->prvcf * (double)(csound->tpidsr));
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
        double ans;
        if (p->kbw[n] != (MYFLT)p->prvbw) {
          p->prvbw = (double)p->kbw[n];
          p->c3 = exp(p->prvbw * (double)(csound->mtpdsr));
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - sig;  /* yt1 contains yt1-xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    else if (p->scale == 2) {
      for (n=offset; n<nsmps; n++) {
        double sig = (double)asig[n];
        double ans;
        if (p->kbw[n] != (MYFLT)p->prvbw) {
          p->prvbw = (double)p->kbw[n];
          p->c3 = exp(p->prvbw * (double)(csound->mtpdsr));
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
        ans = c1 * sig + c2 * yt1 - c3 * yt2;
        yt2 = yt1;
        yt1 = ans - 2.0 * sig;      /* yt1 contains yt1-D*xt1 */
        ar[n] = (MYFLT)ans;
      }
    }
    p->yt1 = yt1; p->yt2 = yt2;
    return OK;
}

extern int tonset(CSOUND*, TONE*);

static int atonea(CSOUND *csound, TONE *p)
{
    MYFLT       *ar, *asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double      c2 = p->c2, yt1 = p->yt1;

    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    asig = p->asig;
    for (n=offset; n<nsmps; n++) {
      double sig = (double)asig[n];
      double x;
      if (p->khp[n] != p->prvhp) {
        double b;
        p->prvhp = p->khp[n];
        b = 2.0 - cos((double)(p->khp[n] * csound->tpidsr));
        p->c2 = c2 = b - sqrt(b * b - 1.0);
        /*      p->c1 = c1 = 1.0 - c2; */
      }
      x = yt1 = c2 * (yt1 + sig);
      ar[n] = (MYFLT)x;
      yt1 -= sig;               /* yt1 contains yt1-xt1 */
    }
    p->yt1 = yt1;
    return OK;
}

static int tonea(CSOUND *csound, TONE *p)
{
    MYFLT       *ar, *asig;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double      c1 = p->c1, c2 = p->c2;
    double      yt1 = p->yt1;

    ar = p->ar;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (p->khp[n] != (MYFLT)p->prvhp) {
        double b;
        p->prvhp = (double)p->khp[n];
        b = 2.0 - cos((double)(p->prvhp * csound->tpidsr));
        p->c2 = c2 = b - sqrt(b * b - 1.0);
        p->c1 = c1 = 1.0 - c2;
      }
      yt1 = c1 * (double)(asig[n]) + c2 * yt1;
      ar[n] = (MYFLT)yt1;
    }
    p->yt1 = yt1;
    return OK;
}


static OENTRY afilts_localops[] =
{
  { "areson.aa", sizeof(RESON), 0, 5, "a", "aaaoo", (SUBR)rsnset, NULL, (SUBR)aresonaa},
  { "areson.ak", sizeof(RESON), 0, 5, "a", "aakoo", (SUBR)rsnset, NULL, (SUBR)aresonak},
  { "areson.ka", sizeof(RESON), 0, 5, "a", "akaoo", (SUBR)rsnset, NULL, (SUBR)aresonka},
  { "atone.a",  sizeof(TONE),0,  5,   "a", "ako",  (SUBR)tonset, NULL, (SUBR)atonea   },
  { "tone.a",  sizeof(TONE),0,  5,    "a",  "aao",  (SUBR)tonset, NULL, (SUBR)tonea   }

};

LINKAGE_BUILTIN(afilts_localops)


