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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif      /*                      AFILTERS.C        */
#include "ugens5.h"
#include <math.h>


static int32_t atonset(CSOUND *csound, TONE *p)
{
  double b;
  p->prvhp = (double)*p->khp;
  b = 2.0 - cos((double)(p->prvhp * CS_TPIDSR));
  p->c2 = b - sqrt(b * b - 1.0);
  p->c1 = 1.0 - p->c2;

  if (LIKELY(!(*p->istor)))
    p->yt1 = 0.0;
  return OK;
}


static int32_t atonsetx(CSOUND *csound, TONEX *p)
{                   /* From Gabriel Maldonado, modified for arbitrary order */
  {
    double b;
    p->prvhp = *p->khp;
    b = 2.0 - cos((double)(*p->khp * CS_TPIDSR));
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


static int32_t arsnset(CSOUND *csound, RESON *p)
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

static int32_t aresonaa(CSOUND *csound, RESON *p)
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
        p->cosf = cos(p->prvcf * (double)(CS_TPIDSR));
        flag = 1;
      }
      if (p->kbw[n] != (MYFLT)p->prvbw) {
        p->prvbw = (double)p->kbw[n];
        p->c3 = exp(p->prvbw * (double)(CS_MTPIDSR));
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
        p->cosf = cos(p->prvcf * (double)(CS_TPIDSR));
        flag = 1;
      }
      if (p->kbw[n] != (MYFLT)p->prvbw) {
        p->prvbw = (double)p->kbw[n];
        p->c3 = exp(p->prvbw * (double)(CS_MTPIDSR));
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

static int32_t aresonak(CSOUND *csound, RESON *p)
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
    p->c3 = exp(p->prvbw * (double)(CS_MTPIDSR));
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
        p->cosf = cos(p->prvcf * (double)(CS_TPIDSR));
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
        p->cosf = cos(p->prvcf * (double)(CS_TPIDSR));
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

static int32_t aresonka(CSOUND *csound, RESON *p)
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
    p->cosf = cos(p->prvcf * (double)(CS_TPIDSR));
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
        p->c3 = exp(p->prvbw * (double)(CS_MTPIDSR));
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
        p->c3 = exp(p->prvbw * (double)(CS_MTPIDSR));
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

extern int32_t tonset(CSOUND*, TONE*);
extern int32_t tonsetx(CSOUND *csound, TONEX *p);

static int32_t atonea(CSOUND *csound, TONE *p)
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
      b = 2.0 - cos((double)(p->khp[n] * CS_TPIDSR));
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

static int32_t tonea(CSOUND *csound, TONE *p)
{
  MYFLT       *ar, *asig;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  double      c1 = p->c1, c2 = p->c2;
  double      yt1 = p->yt1;
  double      prvhp = p->prvhp;

  ar = p->ar;
  asig = p->asig;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    if (p->khp[n] != prvhp) {
      double b;
      prvhp = (double)p->khp[n];
      b = 2.0 - cos((double)(prvhp * CS_TPIDSR));
      c2 = b - sqrt(b * b - 1.0);
      c1 = 1.0 - c2;
    }
    yt1 = c1 * (double)(asig[n]) + c2 * yt1;
    ar[n] = (MYFLT)yt1;
  }
  p->yt1 = yt1;
  p->prvhp = prvhp;
  p->c1 = c1;
  p->c2 = c2;
  return OK;
}

static int32_t tonexa(CSOUND *csound, TONEX *p) /* From G Maldonado, modified */
{
  MYFLT       *ar = p->ar;
  double      c2 = p->c2, *yt1 = p->yt1,c1 = p->c1;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  int32_t     j, lp = p->loop;

  memmove(ar,p->asig,sizeof(MYFLT)*nsmps);
  if (UNLIKELY(offset))  memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (j=0; j< lp; j++) {
    /* Should *yt1 be reset to something?? */
    for (n=offset; n<nsmps; n++) {
      double x;
      if (p->khp[n] != p->prvhp) {
        double b;
        p->prvhp = (double)p->khp[n];
        b = 2.0 - cos(p->prvhp * (double)CS_TPIDSR);
        p->c2 = b - sqrt(b * b - 1.0);
        p->c1 = 1.0 - p->c2;
      }
      x = c1 * ar[n] + c2 * yt1[j];
      yt1[j] = x;
      ar[n] = (MYFLT)x;
    }
  }
  return OK;
}

static int32_t atonexa(CSOUND *csound, TONEX *p) /* Gabriel Maldonado, modified */
{
  MYFLT       *ar = p->ar;
  double      c2 = p->c2, *yt1 = p->yt1;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32_t  j, lp = p->loop;
  MYFLT    prvhp = p->prvhp;

  memmove(ar,p->asig,sizeof(MYFLT)*nsmps);
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (j=1; j<lp; j++) {
    for (n=offset; n<nsmps; n++) {
      double sig = (double)ar[n];
      double x;
      if (p->khp[n] != prvhp) {
        double b;
        prvhp = p->khp[n];
        b = 2.0 - cos((double)(p->prvhp * CS_TPIDSR));
        c2 = b - sqrt(b * b - 1.0);
      }
      x = c2 * (yt1[j] + sig);
      yt1[j] = x - sig;            /* yt1 contains yt1-xt1 */
      ar[n] = (MYFLT)x;
    }
  }
  p->c2 = c2;
  p->prvhp = prvhp;
  return OK;
}


typedef struct  {
  OPDS    h;
  MYFLT   *sr, *ain, *afc, *istor;
  MYFLT   lkf;
  double  a[8];
} BFIL;

typedef struct  {
  OPDS    h;
  MYFLT   *sr, *ain, *kfo, *kbw, *istor;
  MYFLT   lkf, lkb;
  double  a[8];
} BBFIL;

//#define ROOT2 (1.4142135623730950488)
static int32_t butseta(CSOUND *csound, BFIL *p)      /*      Hi/Lo pass set-up   */
{
  IGN(csound);
  if (*p->istor==FL(0.0)) {
    p->a[6] = p->a[7] = 0.0;
    p->lkf = FL(0.0);
  }
  return OK;
}


static int32_t bbutset(CSOUND *csound, BBFIL *p)    /*      Band set-up         */
{
  IGN(csound);
  if (*p->istor==FL(0.0)) {
    p->a[6] = p->a[7] = 0.0;
    p->lkb = FL(0.0);
    p->lkf = FL(0.0);
  }
  return OK;
}


static int32_t hibuta(CSOUND *csound, BFIL *p) /*      Hipass filter       */
{
  MYFLT    *out, *in;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;
  double   *a;
  double   t, y;
  uint32_t nn;
  a = p->a;

  in = p->ain;
  out = p->sr;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  if (UNLIKELY(p->afc[0] <= FL(0.0)))     {
    memcpy(&out[offset], &in[offset], (nsmps-offset)*sizeof(MYFLT));
    return OK;
  }

  /* if (p->afc[0] != p->lkf)      { */
  /*   p->lkf = p->afc[0]; */
  /*   c = tan((double)(CS_PIDSR * p->lkf)); */

  /*   a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c); */
  /*   a[2] = -(a[1] + a[1]); */
  /*   a[3] = a[1]; */
  /*   a[4] = 2.0 * ( c*c - 1.0) * a[1]; */
  /*   a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1]; */
  /* } */
  for (nn=offset; nn<nsmps; nn++) {
    if (p->afc[nn] != p->lkf)      {
      double c;
      p->lkf = p->afc[nn];
      c = tan((double)(CS_PIDSR * p->lkf));

      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = -(a[1] + a[1]);
      a[3] = a[1];
      a[4] = 2.0 * ( c*c - 1.0) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }
    t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
    t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
    y = t * a[1] + a[2] * a[6] + a[3] * a[7];
    a[7] = a[6];
    a[6] = t;
    out[nn] = (MYFLT)y;
  }
  return OK;
}

static int32_t lobuta(CSOUND *csound, BFIL *p)       /*      Lopass filter       */
{
  MYFLT    *out, *in;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;
  double   *a = p->a;
  double   t, y;
  uint32_t nn;

  in = p->ain;
  out = p->sr;

  if (UNLIKELY(*p->afc <= FL(0.0)))     {
    memset(out, 0, CS_KSMPS*sizeof(MYFLT));
    return OK;
  }

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (nn=offset; nn<nsmps; nn++) {
    if (p->afc[nn] != p->lkf) {
      double c;
      p->lkf = p->afc[nn];
      c = 1.0 / tan((double)(CS_PIDSR * p->lkf));
      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = a[1] + a[1];
      a[3] = a[1];
      a[4] = 2.0 * ( 1.0 - c*c) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }
    t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
    t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
    y = t * a[1] + a[2] * a[6] + a[3] * a[7];
    a[7] = a[6];
    a[6] = t;
    out[nn] = (MYFLT)y;
  }
  return OK;
}

static int32_t bppasxx(CSOUND *csound, BBFIL *p)      /*      Bandpass filter     */
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;
  MYFLT       *out, *in;
  double *a = p->a;
  double t, y;
  uint32_t nn;
  int32_t asgbw = IS_ASIG_ARG(p->kbw), asgfr = IS_ASIG_ARG(p->kfo);

  in = p->ain;
  out = p->sr;
  if (UNLIKELY(p->kbw[0] <= FL(0.0)))     {
    memset(out, 0, CS_KSMPS*sizeof(MYFLT));
    return OK;
  }
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (nn=offset; nn<nsmps; nn++) {
    MYFLT bw, fr;
    bw = (asgbw ? p->kbw[nn] : *p->kbw);
    fr = (asgfr ? p->kfo[nn] : *p->kfo);
    if (bw != p->lkb || fr != p->lkf) {
      double c, d;
      p->lkf = fr;
      p->lkb = bw;
      c = 1.0 / tan((double)(CS_PIDSR * bw));
      d = 2.0 * cos((double)(CS_TPIDSR * fr));
      a[1] = 1.0 / (1.0 + c);
      a[2] = 0.0;
      a[3] = -a[1];
      a[4] = - c * d * a[1];
      a[5] = (c - 1.0) * a[1];
    }
    t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
    t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
    y = t * a[1] + a[2] * a[6] + a[3] * a[7];
    a[7] = a[6];
    a[6] = t;
    out[nn] = (MYFLT)y;
  }
  return OK;
}

static int32_t bpcutxx(CSOUND *csound, BBFIL *p)      /*      Band reject filter  */
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS;
  MYFLT       *out, *in;
  double *a = p->a;
  double t, y;
  uint32_t nn;
  int32_t
    asgbw = IS_ASIG_ARG(p->kbw), asgfr = IS_ASIG_ARG(p->kfo);

  in = p->ain;
  out = p->sr;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  if (UNLIKELY(p->kbw[0] <= FL(0.0)))     {
    memcpy(&out[offset], &in[offset], (nsmps-offset)*sizeof(MYFLT));
    return OK;
  }

  for (nn=offset; nn<nsmps; nn++) {
    MYFLT bw, fr;
    bw = (asgbw ? p->kbw[nn] : *p->kbw);
    fr = (asgfr ? p->kfo[nn] : *p->kfo);
    if (bw != p->lkb || fr != p->lkf) {
      double c, d;
      p->lkf = fr;
      p->lkb = bw;
      c = tan((double)(CS_PIDSR * bw));
      d = 2.0 * cos((double)(CS_TPIDSR * fr));
      a[1] = 1.0 / (1.0 + c);
      a[2] = - d * a[1];
      a[3] = a[1];
      a[4] = a[2];
      a[5] = (1.0 - c) * a[1];
    }
    t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
    t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
    y = t * a[1] + a[2] * a[6] + a[3] * a[7];
    a[7] = a[6];
    a[6] = t;
    out[nn] = (MYFLT)y;
  }
  return OK;
}



static OENTRY afilts_localops[] =
  {
    { "areson.aa", sizeof(RESON), 0,"a","aaaoo",(SUBR)arsnset,(SUBR)aresonaa},
    { "areson.ak", sizeof(RESON), 0,"a","aakoo",(SUBR)arsnset,(SUBR)aresonak},
    { "areson.ka", sizeof(RESON), 0,"a","akaoo",(SUBR)arsnset,(SUBR)aresonka},
    { "atone.a",  sizeof(TONE),   0,"a","ako",  (SUBR)atonset,(SUBR)atonea  },
    { "atonex.a", sizeof(TONEX),  0, "a","aaoo",(SUBR)atonsetx,(SUBR)atonexa},
    { "tone.a",  sizeof(TONE),    0,"a","aao",  (SUBR)atonset,(SUBR)tonea   },
    { "tonex.a", sizeof(TONEX),   0,"a","aaoo", (SUBR)atonsetx,(SUBR)tonexa },
    { "butterhp.a", sizeof(BFIL), 0,"a","aao",  (SUBR)butseta,(SUBR)hibuta  },
    { "butterlp.a", sizeof(BFIL), 0,"a","aao",  (SUBR)butseta,(SUBR)lobuta  },
    { "buthp.a",    sizeof(BFIL), 0,"a","aao",  (SUBR)butseta,(SUBR)hibuta  },
    { "butlp.a",    sizeof(BFIL), 0,"a","aao",  (SUBR)butseta,(SUBR)lobuta  },
    { "butterbp",   sizeof(BBFIL),0,"a","axxo", (SUBR)bbutset,(SUBR)bppasxx},
    { "butbp",      sizeof(BBFIL),0,"a","axxo", (SUBR)bbutset,(SUBR)bppasxx},
    { "butterbr",   sizeof(BBFIL),0,"a","axxo", (SUBR)bbutset,(SUBR)bpcutxx},
    { "butbr",      sizeof(BBFIL),0,"a","axxo", (SUBR)bbutset,(SUBR)bpcutxx},
  };

LINKAGE_BUILTIN(afilts_localops)


