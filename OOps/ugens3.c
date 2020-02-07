/*
    ugens3.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"         /*                              UGENS3.C    */
#include "ugens3.h"
#include <math.h>

int32_t foscset(CSOUND *csound, FOSC *p)
{
    FUNC    *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->cphs = p->mphs = (int32_t)(*p->iphs * FMAXLEN);
      p->ampcod = IS_ASIG_ARG(p->xamp) ? 1 : 0;
      p->carcod = IS_ASIG_ARG(p->xcar) ? 1 : 0;
      p->modcod = IS_ASIG_ARG(p->xmod) ? 1 : 0;
      return OK;
    }
    return NOTOK;
}

int32_t foscil(CSOUND *csound, FOSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *modp, cps, amp;
    MYFLT   xcar, xmod, *carp, car, fmod, cfreq, mod, ndx, *ftab;
    int32_t    mphs, cphs, minc, cinc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = csound->sicvt;

    ar = p->rslt;
    ftp = p->ftp;
    if (UNLIKELY(ftp == NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    mphs = p->mphs;
    cphs = p->cphs;
    ampp = p->xamp;
    cps  = *p->kcps;
    carp = p->xcar;
    modp = p->xmod;
    amp  = *ampp;
    xcar = *carp;
    xmod = *modp;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (p->ampcod || p->carcod || p->modcod) {
      for (n=offset;n<nsmps;n++) {
        if (p->ampcod) amp = ampp[n];
        if (p->carcod) xcar = carp[n];
        if (p->modcod) xmod = modp[n];
        car = cps * xcar;
        mod = cps * xmod;
        ndx = *p->kndx * mod;
        minc = (int32_t)(mod * sicvt);
        mphs &= PHMASK;
        fmod = *(ftab + (mphs >>lobits)) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (int32_t)(cfreq * sicvt);
        cphs &= PHMASK;
        ar[n] = *(ftab + (cphs >>lobits)) * amp;
        cphs += cinc;
      }
    }
    else {
      MYFLT amp = *ampp;
      cps = *p->kcps;
      car = cps * *carp;
      mod = cps * *modp;
      ndx = *p->kndx * mod;
      minc = (int32_t)(mod * sicvt);
      for (n=offset;n<nsmps;n++) {
        mphs &= PHMASK;
        fmod = *(ftab + (mphs >>lobits)) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (int32_t)(cfreq * sicvt);
        cphs &= PHMASK;
        ar[n] = *(ftab + (cphs >>lobits)) * amp;
        cphs += cinc;
      }
    }
    p->mphs = mphs;
    p->cphs = cphs;

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("foscil: not initialised"));
}

int32_t foscili(CSOUND *csound, FOSC *p)
{
    FUNC   *ftp;
    MYFLT  *ar, *ampp, amp, cps, fract, v1, car, fmod, cfreq, mod;
    MYFLT  *carp, *modp, xcar, xmod, ndx, *ftab;
    int32_t  mphs, cphs, minc, cinc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT  sicvt = csound->sicvt;
    MYFLT  *ft;

    ar = p->rslt;
    ftp = p->ftp;
    if (UNLIKELY(ftp == NULL)) goto err1;        /* RWD fix */
    ft = ftp->ftable;
    lobits = ftp->lobits;
    mphs = p->mphs;
    cphs = p->cphs;
    ampp = p->xamp;
    cps  = *p->kcps;
    carp = p->xcar;
    modp = p->xmod;
    amp  = *ampp;
    xcar = *carp;
    xmod = *modp;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->ampcod || p->carcod || p->modcod) {
      for (n=offset;n<nsmps;n++) {
        if (p->ampcod)  amp = ampp[n];
        if (p->carcod)  xcar = carp[n];
        if (p->modcod)  xmod = modp[n];
        car = cps * xcar;
        mod = cps * xmod;
        ndx = *p->kndx * mod;
        minc = (int32_t)(mod * sicvt);
        mphs &= PHMASK;
        fract = PFRAC(mphs);
        ftab = ft + (mphs >>lobits);
        v1 = *ftab++;
        fmod = (v1 + (*ftab - v1) * fract) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (int32_t)(cfreq * sicvt);
        cphs &= PHMASK;
        fract = PFRAC(cphs);
        ftab = ft + (cphs >>lobits);
        v1 = *ftab++;
        ar[n] = (v1 + (*ftab - v1) * fract) * amp;
        cphs += cinc;
      }
    }
    else {
      cps = *p->kcps;
      car = cps * *carp;
      mod = cps * *modp;
      ndx = *p->kndx * mod;
      minc = (int32_t)(mod * sicvt);
      for (n=offset;n<nsmps;n++) {
        mphs &= PHMASK;
        fract = PFRAC(mphs);
        ftab = ft + (mphs >>lobits);
        v1 = *ftab++;
        fmod = (v1 + (*ftab - v1) * fract) * ndx;
        mphs += minc;
        cfreq = car + fmod;
        cinc = (int32_t)(cfreq * sicvt);
        cphs &= PHMASK;
        fract = PFRAC(cphs);
        ftab = ft + (cphs >>lobits);
        v1 = *ftab++;
        ar[n] = (v1 + (*ftab - v1) * fract) * amp;
        cphs += cinc;
      }
    }
    p->mphs = mphs;
    p->cphs = cphs;

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("foscili: not initialised"));
}


int32_t losset(CSOUND *csound, LOSC *p)
{
    FUNC    *ftp;
    if ((ftp = csound->FTnp2Finde(csound,p->ifn)) != NULL) {
      uint32 maxphs = ftp->flenfrms;
      //printf("****maxphs = %d (%x)\n", maxphs, maxphs);
      //printf("****ftp cvtbas = %g ibas = %g\n", ftp->cvtbas, *p->ibas);
      p->ftp = ftp;
      if (*p->ibas != FL(0.0))
        p->cpscvt = (ftp->cvtbas / *p->ibas)/LOFACT;
      else if (UNLIKELY((p->cpscvt = ftp->cpscvt) == FL(0.0))) {
        p->cpscvt = FL(1.0) /FL(261.62556530059862592); /* Middle C */
        csound->Warning(csound, Str("no legal base frequency"));
      }
      else p->cpscvt /= LOFACT;
      //printf("****cpscvt = %g\n", p->cpscvt);
      if ((p->mod1 = (int16) *p->imod1) < 0) {
        if (UNLIKELY((p->mod1 = ftp->loopmode1) == 0)) {
          csound->Warning(csound, Str("loscil: sustain defers to "
                                      "non-looping source"));
        }
        p->beg1 = ftp->begin1;
        p->end1 = ftp->end1;
      }
      else if (UNLIKELY(p->mod1 < 0 || p->mod1 > 3))
        goto lerr2;
      else {
        p->beg1 = *p->ibeg1;
        p->end1 = *p->iend1;
        if (!p->beg1 && !p->end1)
          /* default to looping the whole sample */
          p->end1 =            /* These are the same!! */
            (p->mod1 ? (MYFLT)maxphs : (MYFLT)ftp->flenfrms);
        else if (UNLIKELY(p->beg1 < 0 ||
                          p->end1 > maxphs ||
                          p->beg1 >= p->end1)) {
          csound->Message(csound, "beg: %g, end = %g, maxphs = %d\n",
                          p->beg1, p->end1, maxphs);
          goto lerr2;
        }
      }
      if ((p->mod2 = (int16) *p->imod2) < 0) {
        p->mod2 = ftp->loopmode2;
        p->beg2 = ftp->begin2;
        p->end2 = ftp->end2;
      }
      else {
        p->beg2 = *p->ibeg2;
        p->end2 = *p->iend2;
        if (UNLIKELY(p->mod2 < 0 || p->mod2 > 3 ||
                     p->beg2 < 0 || p->end2 > (int32_t)maxphs ||
                     p->beg2 >= p->end2)) {
          goto lerr3;
        }
      }
      p->beg1 = (p->beg1 >= 0L ? p->beg1 : 0L);
      p->end1 = (p->end1 < (int32_t)maxphs ? p->end1 : (int32_t)maxphs);
      if (UNLIKELY(p->beg1 >= p->end1)) {
        p->mod1 = 0;
        p->beg1 = 0L;
        p->end1 = maxphs;
      }
      p->beg2 = (p->beg2 >= 0L ? p->beg2 : 0L);
      p->end2 = (p->end2 < (int32_t)maxphs ? p->end2 : (int32_t)maxphs);
      if (UNLIKELY(p->beg2 >= p->end2)) {
        p->mod2 = 0;
        p->beg2 = 0L;
      }
      if (!p->mod2 && !p->end2)       /* if no release looping */
        p->end2 = maxphs;             /*   set a reading limit */
      p->lphs = 0;
      p->seg1 = 1;
      if ((p->curmod = p->mod1))
        p->looping = 1;
      else p->looping = 0;
      if (p->OUTOCOUNT == 1) {
        p->stereo = 0;
        if (UNLIKELY(ftp->nchanls != 1))
          return csound->InitError(csound, Str(
                               "mono loscil cannot read from stereo ftable"));
      }
      else {
        p->stereo = 1;
        if (UNLIKELY(ftp->nchanls != 2))
          return csound->InitError(csound, Str(
                               "stereo loscil cannot read from mono ftable"));
      }
      return OK;
    }
    return NOTOK;

 lerr2:
    return csound->InitError(csound, Str("illegal sustain loop data"));
 lerr3:
    return csound->InitError(csound, Str("illegal release loop data"));
}

int32_t losset_phs(CSOUND *csound, LOSCPHS *p)
{
    FUNC    *ftp;
    if ((ftp = csound->FTnp2Finde(csound,p->ifn)) != NULL) {
      uint32 maxphs = ftp->flenfrms;
      //printf("****maxphs = %d (%x)\n", maxphs, maxphs);
      p->ftp = ftp;
      if (*p->ibas != FL(0.0))
        p->cpscvt = (ftp->cvtbas / *p->ibas)/LOFACT;
      else if (UNLIKELY((p->cpscvt = ftp->cpscvt) == FL(0.0))) {
        p->cpscvt = FL(1.0) /FL(261.62556530059862592); /* Middle C */
        csound->Warning(csound, Str("no legal base frequency"));
      }
      else p->cpscvt /= LOFACT;
      //printf("****cpscvt = %g\n", p->cpscvt);
      if ((p->mod1 = (int16) *p->imod1) < 0) {
        if (UNLIKELY((p->mod1 = ftp->loopmode1) == 0)) {
          csound->Warning(csound, Str("loscil: sustain defers to "
                                      "non-looping source"));
        }
        p->beg1 = ftp->begin1;
        p->end1 = ftp->end1;
      }
      else if (UNLIKELY(p->mod1 < 0 || p->mod1 > 3))
        goto lerr2;
      else {
        p->beg1 = *p->ibeg1;
        p->end1 = *p->iend1;
        if (!p->beg1 && !p->end1)
          /* default to looping the whole sample */
          p->end1 =            /* These are the same!! */
            (p->mod1 ? (MYFLT)maxphs : (MYFLT)ftp->flenfrms);
        else if (UNLIKELY(p->beg1 < 0 ||
                          p->end1 > maxphs ||
                          p->beg1 >= p->end1)) {
          csound->Message(csound, "beg: %g, end = %g, maxphs = %d\n",
                          p->beg1, p->end1, maxphs);
          goto lerr2;
        }
      }
      if ((p->mod2 = (int16) *p->imod2) < 0) {
        p->mod2 = ftp->loopmode2;
        p->beg2 = ftp->begin2;
        p->end2 = ftp->end2;
      }
      else {
        p->beg2 = *p->ibeg2;
        p->end2 = *p->iend2;
        if (UNLIKELY(p->mod2 < 0 || p->mod2 > 3 ||
                     p->beg2 < 0 || p->end2 > (int32_t)maxphs ||
                     p->beg2 >= p->end2)) {
          goto lerr3;
        }
      }
      p->beg1 = (p->beg1 >= 0L ? p->beg1 : 0L);
      p->end1 = (p->end1 < (int32_t)maxphs ? p->end1 : (int32_t)maxphs);
      if (UNLIKELY(p->beg1 >= p->end1)) {
        p->mod1 = 0;
        p->beg1 = 0L;
        p->end1 = maxphs;
      }
      p->beg2 = (p->beg2 >= 0L ? p->beg2 : 0L);
      p->end2 = (p->end2 < (int32_t)maxphs ? p->end2 : (int32_t)maxphs);
      if (UNLIKELY(p->beg2 >= p->end2)) {
        p->mod2 = 0;
        p->beg2 = 0L;
      }
      if (!p->mod2 && !p->end2)       /* if no release looping */
        p->end2 = maxphs;             /*   set a reading limit */
      p->lphs = 0;
      p->seg1 = 1;
      if ((p->curmod = p->mod1))
        p->looping = 1;
      else p->looping = 0;
      if (p->OUTOCOUNT == 2) {
        p->stereo = 0;
        if (UNLIKELY(ftp->nchanls != 1))
          return csound->InitError(csound, Str(
                               "mono loscilphs cannot read from stereo ftable"));
      }
      else if (p->OUTOCOUNT == 3)
        {
          p->stereo = 1;
          if (UNLIKELY(ftp->nchanls != 2))
            return csound->InitError(csound, Str(
                               "stereo loscilphs cannot read from mono ftable"));
        }
      else {
        return csound->InitError(csound, Str(
                               "loscilphs: insufficient outputs"));
      }
      return OK;
    }
    return NOTOK;

 lerr2:
    return csound->InitError(csound, Str("illegal sustain loop data"));
 lerr3:
    return csound->InitError(csound, Str("illegal release loop data"));
}

static inline void loscil_linear_interp_mono(MYFLT *ar,
                                             MYFLT *ftbl, MYFLT phs, int32_t flen)
{
    MYFLT   fract, tmp;
    int32_t   x;

    fract = MODF(phs, &tmp);
    x = (int32_t) tmp;
    //printf("phs=%d+%f\n",x, fract);
    tmp = ftbl[x];
    x = (x < flen ? (x + 1) : flen);
    *ar = tmp + ((ftbl[x] - tmp) * fract);
}

static inline void loscil_linear_interp_stereo(MYFLT *arL, MYFLT *arR,
                                               MYFLT *ftbl, MYFLT phs, int32_t flen)
{
    MYFLT   fract, tmpL, tmpR;
    int     x;

    fract = MODF(phs, &tmpL);
    x = (int32_t) 2*tmpL;
    //printf("phs=%d+%f\n",x, fract);
    tmpL = ftbl[x];
    tmpR = ftbl[x + 1];
    x = (x < ((int32_t) flen - 1) ? (x + 2) : ((int32_t) flen - 1));
    *arL = tmpL + ((ftbl[x] - tmpL) * fract);
    *arR = tmpR + ((ftbl[x + 1] - tmpR) * fract);
}

static inline void loscil_cubic_interp_mono(MYFLT *ar,
                                            MYFLT *ftbl, MYFLT phs, int32_t flen)
{
    MYFLT   fract, tmp, a0, a1, a2, a3;
    int32_t     x;

    fract = MODF(phs, &tmp);
    x = (int32_t) tmp;
    //printf("phs=%d+%f\n",x, fract);
    a3 = fract * fract; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
    a2 = fract; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
    a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= fract;
    a0 *= fract; a1 *= fract; a2 *= fract; a3 *= fract; a1 += FL(1.0);
    tmp = ftbl[(x >= 0 ? x : 0)] * a0;
    tmp += ftbl[++x] * a1;
    x++;
    tmp += ftbl[(x < (int32_t) flen ? x : (int32_t) flen)] * a2;
    x++;
    tmp += ftbl[(x < (int32_t) flen ? x : (int32_t) flen)] * a3;
    *ar = tmp;
}

static CS_NOINLINE void
    loscil_cubic_interp_stereo(MYFLT *arL, MYFLT *arR,
                               MYFLT *ftbl, MYFLT phs, int32_t flen)
{
    MYFLT   fract, tmpL, tmpR, a0, a1, a2, a3;
    int32_t     x;

    fract = MODF(phs, &tmpL);
    x = (int32_t) 2*tmpL;
    //printf("phs=%d+%f\n",x, fract);
    a3 = fract * fract; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
    a2 = fract; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
    a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= fract;
    a0 *= fract; a1 *= fract; a2 *= fract; a3 *= fract; a1 += FL(1.0);
    tmpL = ftbl[(x >= 0 ? x : 0)] * a0;
    tmpR = ftbl[(x >= 0 ? (x + 1) : 1)] * a0;
    x += 2;
    tmpL += ftbl[x] * a1;
    tmpR += ftbl[x + 1] * a1;
    x = (x < ((int32_t) flen - 1) ? (x + 2) : ((int32_t) flen - 1));
    tmpL += ftbl[x] * a2;
    tmpR += ftbl[x + 1] * a2;
    x = (x < ((int32_t) flen - 1) ? (x + 2) : ((int32_t) flen - 1));
    tmpL += ftbl[x] * a3;
    tmpR += ftbl[x + 1] * a3;
    *arL = tmpL;
    *arR = tmpR;
}

/* *********************** needs total rewrite **************** */
int32_t loscil(CSOUND *csound, LOSC *p)
{
    IGN(csound);
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp;
    MYFLT    phs;
    MYFLT    inc, beg, end;
    uint32_t n = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;
    int32_t      aamp;
    MYFLT    xx;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    //printf("inc: %lf * %lf = %lf\n", *p->kcps, p->cpscvt, inc);
    xamp = p->xamp;
    xx = *xamp;
    aamp = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (UNLIKELY(p->h.insdshead->relesing))     /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->stereo) {
      ar2 = p->ar2;
      if (UNLIKELY(n)) memset(ar2, '\0', n*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar2[nsmps], '\0', early*sizeof(MYFLT));
      goto phsck2;
    }
 phschk:
    if (phs >= end && p->curmod != 3) {
      //printf("****phs = %f end = %f\n", phs,end);
      goto put0;
    }
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if ((phs += inc) >= end) {
          //printf("****phs, end = %f, %f\n", phs, end);
          goto nxtseg;
        }
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3;
          else break;
        }
      }
      break;
    case 3:
    case3:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2;
          else break;
        }
      }
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n>nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
put0:
    //printf("****put0\n");
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;

 phsck2:
    /*VL increment for stereo */
    if (phs >= end && p->curmod != 3)
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg2;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2s:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3s;
          else break;
        }
      }
      break;
    case 3:
    case3s:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
       loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2s;
          else break;
        }
      }
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n<nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    memset(&ar2[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;
}


int32_t loscil_phs(CSOUND *csound, LOSCPHS *p)
{
    IGN(csound);
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp, *sphs;
    MYFLT    phs;
    MYFLT    inc, beg, end;
    uint32_t n = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;
    int32_t      aamp;
    MYFLT    xx;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    xamp = p->xamp;
    xx = *xamp;
    aamp = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (UNLIKELY(p->h.insdshead->relesing))     /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    sphs = p->sphs;
    if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&sphs[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->stereo) {
      ar2 = p->ar2;
      if (UNLIKELY(n)) memset(ar2, '\0', n*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar2[nsmps], '\0', early*sizeof(MYFLT));
      goto phsck2;
    }
 phschk:
    if (phs >= end && p->curmod != 3) {
      //printf("****phs = %f end = %d\n", phs,end);
      goto put0;
    }
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if ((phs += inc) >= end) {
          //printf("****phs, end = %f, %d\n", phs, end);
          goto nxtseg;
        }
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if ((phs += inc) >= end) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3;
          else break;
        }
      }
      break;
    case 3:
    case3:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_linear_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2;
          else break;
        }
      }
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n>nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
put0:
    //printf("****put0\n");
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;

 phsck2:
    if (phs >= end && p->curmod != 3)
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg2;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2s:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3s;
          else break;
        }
      }
      break;
    case 3:
    case3s:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
       loscil_linear_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2s;
          else break;
        }
      }
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n<nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    memset(&ar2[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;
}



int32_t loscil3_phs(CSOUND *csound, LOSCPHS *p)
{
    IGN(csound);
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp, *sphs;
    MYFLT    phs;
    MYFLT    inc, beg, end;
    uint32_t n = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;
    int32_t     aamp;
    MYFLT   xx;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    xamp = p->xamp;
    xx = *xamp;
    aamp = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (p->h.insdshead->relesing)   /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    sphs = p->sphs;
    if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&sphs[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->stereo) {
      ar2 = p->ar2;
      if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar2[nsmps], '\0', early*sizeof(MYFLT));
      goto phsck2;
    }
 phschk:
    if (UNLIKELY(phs >= end && p->curmod != 3))
      goto put0;
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3;
          else break;
        }
      }
      break;
    case 3:
    case3:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);;
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2;
          else break;
        }
      }
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
 put0:
    memset(&ar1[n], 0, sizeof(MYFLT)*(nsmps-n));
    /* do { */
    /*   *ar1++ = FL(0.0); */
    /* } while (--nsmps); */
    return OK;

 phsck2:
    if (UNLIKELY(phs >= end && p->curmod != 3))
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg2;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2s:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3s;
          else break;
        }
      }
      break;
    case 3:
    case3s:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        sphs[n] = phs/ftp->flen;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2s;
          else break;
        }
      }
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n<nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    memset(&ar2[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;
}


int32_t loscil3(CSOUND *csound, LOSC *p)
{
    IGN(csound);
    FUNC    *ftp;
    MYFLT   *ar1, *ar2, *ftbl, *xamp;
    MYFLT    phs;
    MYFLT    inc, beg, end;
    uint32_t n = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nsmps = CS_KSMPS;
    int32_t     aamp;
    MYFLT   xx;

    ftp = p->ftp;
    ftbl = ftp->ftable;
    if ((inc = (*p->kcps * p->cpscvt)) < 0)
      inc = -inc;
    xamp = p->xamp;
    xx = *xamp;
    aamp = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    if (p->seg1) {                      /* if still segment 1  */
      beg = p->beg1;
      end = p->end1;
      if (p->h.insdshead->relesing)   /*    sense note_off   */
        p->looping = 0;
    }
    else {
      beg = p->beg2;
      end = p->end2;
    }
    phs = p->lphs;
    ar1 = p->ar1;
    if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar1[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->stereo) {
      ar2 = p->ar2;
      if (UNLIKELY(n)) memset(ar1, '\0', n*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ar2[nsmps], '\0', early*sizeof(MYFLT));
      goto phsck2;
    }
 phschk:
    if (UNLIKELY(phs >= end && p->curmod != 3))
      goto put0;
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3;
          else break;
        }
      }
      break;
    case 3:
    case3:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_cubic_interp_mono(&ar1[n], ftbl, phs, ftp->flen);;
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2;
          else break;
        }
      }
      break;

    nxtseg:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (--nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phschk;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout:
    p->lphs = phs;
 put0:
    memset(&ar1[n], 0, sizeof(MYFLT)*(nsmps-n));
    /* do { */
    /*   *ar1++ = FL(0.0); */
    /* } while (--nsmps); */
    return OK;

 phsck2:
    if (UNLIKELY(phs >= end && p->curmod != 3))
      goto put0s;                               /* for STEREO:  */
    switch (p->curmod) {
    case 0:
      for (; n<nsmps; n++) {                    /* NO LOOPING  */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end))
          goto nxtseg2;
      }
      break;
    case 1:
      for (; n<nsmps; n++) {                    /* NORMAL LOOPING */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= end - beg;
        }
      }
      break;
    case 2:
    case2s:
      for (; n<nsmps; n++) {                    /* BIDIR FORW, EVEN */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs += inc) >= end)) {
          if (!(p->looping)) goto nxtseg2;
          phs -= (phs - end) * 2;
          p->curmod = 3;
          if (++n<nsmps) goto case3s;
          else break;
        }
      }
      break;
    case 3:
    case3s:
      for (; n<nsmps; n++) {                    /* BIDIR BACK, EVEN */
        loscil_cubic_interp_stereo(&ar1[n], &ar2[n], ftbl, phs, ftp->flen);
        if (aamp) xx = xamp[n];
        ar1[n] *= xx;
        ar2[n] *= xx;
        if (UNLIKELY((phs -= inc) < beg)) {
          phs += (beg - phs) * 2;
          p->curmod = 2;
          if (++n<nsmps) goto case2s;
          else break;
        }
      }
      break;

    nxtseg2:
      if (p->seg1) {
        p->seg1 = 0;
        if ((p->curmod = p->mod2) != 0)
          p->looping = 1;
        if (++n<nsmps) {
          beg = p->beg2;
          end = p->end2;
          p->lphs = phs;
          goto phsck2;
        }
        break;
      }
      if (LIKELY(++n<nsmps)) goto phsout2;
      break;
    }
    p->lphs = phs;
    return OK;

 phsout2:
    p->lphs = phs;
 put0s:
    memset(&ar1[n], '\0', sizeof(MYFLT)*(nsmps-n));
    memset(&ar2[n], '\0', sizeof(MYFLT)*(nsmps-n));
    return OK;
}



#define ISINSIZ 32768L
#define ADMASK  32767L

static int32_t adset_(CSOUND *csound, ADSYN *p, int32_t stringname)
{
    int32_t    n;
    char    filnam[MAXNAME];
    MEMFIL  *mfp;
    int16   *adp, *endata, val;
    PTLPTR  *ptlap, *ptlfp, *ptlim;
    int32_t     size;

    if (csound->isintab == NULL) {  /* if no sin table yet, make one */
      int16 *ip;
      csound->isintab = ip =
        (int16*) csound->Malloc(csound, ISINSIZ * sizeof(int16));
      for (n = 0; n < ISINSIZ; n++)
        *ip++ = (int16) (sin(TWOPI * n / ISINSIZ) * 32767.0);
    }
    if (stringname) strNcpy(filnam, ((STRINGDAT*)p->ifilcod)->data, MAXNAME-1);
    else if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(filnam, get_arg_string(csound, *p->ifilcod), MAXNAME-1);
    else csound->strarg2name(csound, filnam, p->ifilcod, "adsyn.", 0);


    if ((mfp = p->mfp) == NULL || strcmp(mfp->filename,filnam) != 0) {
      /* readfile if reqd */
      if (UNLIKELY((mfp = ldmemfile2withCB(csound, filnam,
                                           CSFTYPE_HETRO, NULL)) == NULL)) {
        return csound->InitError(csound, Str("ADSYN cannot load %s"), filnam);
      }
      p->mfp = mfp;                         /*   & record         */
    }

    adp = (int16 *) mfp->beginp;            /* align on file data */
    endata = (int16 *) mfp->endp;
    size = 1+(*adp == -1 ? MAXPTLS : *adp++); /* Old no header -> MAXPIL */
    if (p->aux.auxp==NULL || p->aux.size < (uint32_t)sizeof(PTLPTR)*size)
      csound->AuxAlloc(csound, sizeof(PTLPTR)*size, &p->aux);

    ptlap = ptlfp = (PTLPTR*)p->aux.auxp;   /* find base ptl blk */
    ptlim = ptlap + size;
    do {
      if ((val = *adp++) < 0) {             /* then for each brkpt set,   */
        switch (val) {
        case -1:
          ptlap->nxtp = ptlap + 1;       /* chain the ptl blks */
          if (UNLIKELY((ptlap = ptlap->nxtp) >= ptlim)) goto adsful;
          ptlap->ap = (DUPLE *) adp;     /*  record start amp  */
          ptlap->amp = ptlap->ap->val;
          break;
        case -2:
          if (UNLIKELY((ptlfp += 1) >= ptlim)) goto adsful;
          ptlfp->fp = (DUPLE *) adp;     /*  record start frq  */
          ptlfp->frq = ptlfp->fp->val;
          ptlfp->phs = 0;                /*  and clr the phase */
          break;
        default:
          return csound->InitError(csound, Str("illegal code %d encountered"), val);
        }
      }
    } while (adp < endata);
    if (UNLIKELY(ptlap != ptlfp)) {
      return csound->InitError(csound, Str("%d amp tracks, %d freq tracks"),
                               (int32_t) (ptlap - (PTLPTR*)p->aux.auxp) - 1,
                               (int32_t) (ptlfp - (PTLPTR*)p->aux.auxp) - 1);
    }
    ptlap->nxtp = NULL;   /* terminate the chain */
    p->mksecs = 0;

    return OK;

 adsful:
    return csound->InitError(csound, Str("partial count exceeds MAXPTLS"));
}

int32_t adset(CSOUND *csound, ADSYN *p){
  return adset_(csound,p,0);
}

int32_t adset_S(CSOUND *csound, ADSYN *p){
  return adset_(csound,p,1);
}

#define ADSYN_MAXLONG FL(2147483647.0)

int32_t adsyn(CSOUND *csound, ADSYN *p)
{
    PTLPTR  *curp, *prvp;
    DUPLE   *ap, *fp;
    int16   curtim, diff, ktogo;
    int32_t   phs, sinc, amp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *ar = p->rslt;
    MYFLT   ampscale, frqscale;
    int32_t   timkincr, nxtim;

    if (UNLIKELY(csound->isintab == NULL)) {      /* RWD fix */
      return csound->PerfError(csound, &(p->h),
                               Str("adsyn: not initialised"));
    }
    /* IV - Jul 11 2002 */
    ampscale = *p->kamod * csound->e0dbfs;      /* since 15-bit sine table */
    frqscale = *p->kfmod * ISINSIZ * csound->onedsr;
    /* 1024 * msecs of analysis */
    memset(p->rslt,0,sizeof(MYFLT)*nsmps);
    if (UNLIKELY(early)) nsmps -= early;
    timkincr = (int32)(*p->ksmod*FL(1024000.0)*CS_ONEDKR);
    curtim = (int16)(p->mksecs >> 10);          /* cvt mksecs to msecs */
    curp = (PTLPTR*)p->aux.auxp;                /* now for each partial:    */
    while ((prvp = curp) && (curp = curp->nxtp) != NULL ) {
      ap = curp->ap;
      fp = curp->fp;
      while (curtim >= (ap+1)->tim)       /* timealign ap, fp */
        curp->ap = ap += 1;
      while (curtim >= (fp+1)->tim)
        curp->fp = fp += 1;
      if ((amp = curp->amp)) {            /* for non-zero amp   */
        sinc = (int32)(curp->frq * frqscale);
        phs = curp->phs;
        /*   addin a sinusoid */
        for (n=offset; n<nsmps; n++) {
          ar[n] += (ampscale*(MYFLT)csound->isintab[phs]*(MYFLT)amp)/ADSYN_MAXLONG;
          phs += sinc;
          phs &= ADMASK;
        }
        curp->phs = phs;
      }
      if ((nxtim = (ap+1)->tim) == 32767) {   /* if last amp this partial */
        prvp->nxtp = curp->nxtp;            /*   remov from activ chain */
        curp = prvp;
      }
      else {                                 /* else interp towds nxt amp */
        if ((diff = (int16)((ap+1)->val - amp))) {
          ktogo = (int16)(((nxtim<<10) - p->mksecs + timkincr - 1) / timkincr);
          if (ktogo == 0) curp->amp += diff;
          else            curp->amp += diff / ktogo;
        }
        if ((nxtim = (fp+1)->tim) != 32767            /*      & nxt frq */
            && (diff = (fp+1)->val - curp->frq)) {
          ktogo = (int16)(((nxtim<<10) - p->mksecs + timkincr - 1) / timkincr);
          if (ktogo == 0) curp->frq += diff;
          else            curp->frq += diff / ktogo;
        }
      }
    }
    p->mksecs += timkincr;                  /* advance the time */
    return OK;
}
