/*
    compress.c:

    Copyright (C) 2006 by Barry Vercoe; adapted by John ffitch

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

//#include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"

typedef struct {
        OPDS    h;
        MYFLT   *ar, *aasig, *acsig, *kthresh, *kloknee, *khiknee;
        MYFLT   *kratio, *katt, *krls, *ilook;

        MYFLT   thresh, loknee, hiknee, ratio, curatt, currls;
        MYFLT   envthrsh, envlo, kneespan, kneemul, kneecoef, ratcoef;
        double  cenv, c1, c2, d1, d2, ampmul;
        MYFLT   *abuf, *cbuf, *aptr, *cptr, *clim, lmax, *lmaxp;
        int32   newenv;
        AUXCH   auxch;
        MYFLT   bias;
} CMPRS;

typedef struct {        /* this now added from 07/01 */
    OPDS    h;
    MYFLT   *ar, *asig, *kdist, *ifn, *ihp, *istor;
    double  c1, c2;
    MYFLT   prvq, prvd, min_rms;
    MYFLT   midphs, maxphs, begval, endval;
    FUNC    *ftp;
} DIST;

static int32_t compset(CSOUND *csound, CMPRS *p)
{
    int32    delsmps;

    p->thresh = (MYFLT) MAXPOS;
    p->loknee = (MYFLT) MAXPOS;                 /* force reinits        */
    p->hiknee = (MYFLT) MAXPOS;
    p->ratio  = (MYFLT) MAXPOS;
    p->curatt = (MYFLT) MAXPOS;
    p->currls = (MYFLT) MAXPOS;
    /* round to nearest integer */
    if (UNLIKELY((delsmps = MYFLT2LONG(*p->ilook * csound->GetSr(csound))) <= 0L))
      delsmps = 1L;                             /* alloc 2 delay bufs   */
    csound->AuxAlloc(csound, delsmps * 2 * sizeof(MYFLT), &p->auxch);
    p->abuf = (MYFLT *)p->auxch.auxp;
    p->cbuf = p->abuf + delsmps;                /*   for asig & csig    */
    p->clim = p->cbuf + delsmps;
    p->aptr = p->abuf;
    p->cptr = p->cbuf;
    p->lmaxp = p->clim - 1;
    p->lmax = FL(0.0);
    p->cenv = 0.0;
    p->newenv = 0;
    p->bias = FL(0.0);
    return OK;
}

/* compress2 is compress but with dB inputs in range [-90,0] rather
   than [0.90], by setting p->bias valuex -- JPff */
static int32_t comp2set(CSOUND *csound, CMPRS *p)
{
    int32_t ret = compset(csound, p);
    p->bias = FL(90.0);
    return ret;
}

static int32_t compress(CSOUND *csound, CMPRS *p)
{
    MYFLT       *ar, *ainp, *cinp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    /* VL: scale by 0dbfs, code is tuned to work in 16bit range */
    MYFLT scal = FL(32768.0)/csound->e0dbfs;

    if (*p->kthresh != p->thresh) {             /* check for changes:   */
      p->thresh = *p->kthresh;
      p->envthrsh = (MYFLT) exp((p->thresh+p->bias) * LOG10D20);
    }
    if (*p->kloknee != p->loknee ||
        *p->khiknee != p->hiknee ||
        *p->kratio != p->ratio) {
      MYFLT ratio, K;
      p->loknee = *p->kloknee;
      p->hiknee = *p->khiknee;
      p->ratio = *p->kratio;
      p->envlo = (MYFLT) exp((p->loknee+p->bias) * LOG10D20);
      if ((p->kneespan = p->hiknee - p->loknee) < FL(0.0))
        p->kneespan = FL(0.0);
      if ((ratio = p->ratio) < FL(0.01))         /* expand max is 100 */
        ratio = FL(0.01);
      K = (MYFLT) LOG10D20 * (FL(1.0) - ratio) / ratio;
      p->ratcoef = K;                            /* rat down per db */
      if (p->kneespan > FL(0.0)) {
        p->kneecoef = K*FL(0.5) / p->kneespan; /* y = x - (K/2span)x*x */
        p->kneemul = (MYFLT)exp(p->kneecoef * p->kneespan * p->kneespan);
      }
      else
        p->kneemul = FL(1.0);
    }
    if (*p->katt != p->curatt) {
      if ((p->curatt = *p->katt) < csound->onedsr)
        p->c2 = 0.0;
      else
        p->c2 = pow(0.5, csound->onedsr / p->curatt);
      p->c1 = 1.0 - p->c2;
    }
    if (*p->krls != p->currls) {
      if ((p->currls = *p->krls) < csound->onedsr)
        p->d2 = 0.0;
      else
        p->d2 = pow(0.5, csound->onedsr / p->currls);
      p->d1 = 1.0 - p->d2;
    }
    ar = p->ar;
    ainp = p->aasig;
    cinp = p->acsig;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {   /* now for each sample of both inputs:  */
      MYFLT asig, lsig;
      double csig;
      asig = *p->aptr;                  /* get signals from delay line  */
      csig = *p->cptr;
      *p->aptr = ainp[n]*scal;               /*   & replace with incoming    */
      lsig = FABS(cinp[n]*scal);
      //lsig = -lsig;                   /*   made abs for control       */
      *p->cptr = lsig;
      if (p->cptr == p->lmaxp) {        /* if prev ctrl was old lamax   */
        MYFLT *lap, newmax = FL(0.0);
        for (lap = p->cptr + 1; lap < p->clim; lap++)
          if (*lap >= newmax) {
            newmax = *lap;              /*   find next highest abs      */
            p->lmaxp = lap;
          }
        for (lap = p->cbuf; lap <= p->cptr; lap++)
          if (*lap >= newmax) {
            newmax = *lap;              /*   in lkahd circular cbuf     */
            p->lmaxp = lap;
          }
        p->lmax = newmax;
      }
      else if (lsig >= p->lmax) {       /* else keep lkahd max & adrs   */
        p->lmax = lsig;                 /*   on arrival                 */
        p->lmaxp = p->cptr;
      }
      if (csig > p->cenv)               /* follow a rising csig env     */
        p->cenv = p->c1 * csig + p->c2 * p->cenv;
      else if (p->cenv > p->lmax)       /* else if env above lookahead  */
        p->cenv = p->d1 * csig + p->d2 * p->cenv;    /*  apply release  */
      else goto lvlchk;
      p->newenv = 1;

    lvlchk:
      if (p->cenv > p->envlo) {         /* if env exceeds loknee amp    */
        if (p->newenv) {                /*   calc dbenv & ampmul        */
          double dbenv, excess;
          p->newenv = 0;
          dbenv = log(p->cenv + 0.001) / LOG10D20;      /* for softknee */
          if ((excess = dbenv - (p->loknee+p->bias)) < p->kneespan)
            p->ampmul = exp(p->kneecoef * excess * excess);
          else {
            excess -= p->kneespan;      /* or ratio line */
            p->ampmul = p->kneemul * exp(p->ratcoef * excess);
          }
        }
        asig *= (MYFLT)p->ampmul;       /* and compress the asig */
      }
      else if (p->cenv < p->envthrsh)
        asig = FL(0.0);                 /* else maybe noise gate */
      ar[n] = asig/scal;
      if (++p->aptr >= p->cbuf) {
        p->aptr = p->abuf;
        p->cptr = p->cbuf;
      }
      else p->cptr++;
    }
    return OK;
}

static int32_t distset(CSOUND *csound, DIST *p)
{
    double  b;
    FUNC    *ftp;

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->ifn)) == NULL)) return NOTOK;
    p->ftp = ftp;
    p->maxphs = (MYFLT)ftp->flen;       /* set ftable params    */
    p->midphs = p->maxphs * FL(0.5);
    p->begval = ftp->ftable[0];
    p->endval = ftp->ftable[ftp->flen];
    b = 2.0 - cos((double) (*p->ihp * csound->tpidsr)); /*  and rms coefs */
    p->c2 = b - sqrt(b * b - 1.0);
    p->c1 = 1.0 - p->c2;
    p->min_rms = csound->e0dbfs * DV32768;
    if (!*p->istor) {
      p->prvq = FL(0.0);
      p->prvd = FL(1000.0) * p->min_rms;
    }

    return OK;
}

static int32_t distort(CSOUND *csound, DIST *p)
{
    IGN(csound);
    MYFLT   *ar, *asig;
    MYFLT   q, rms, dist, dnew, dcur, dinc;
    FUNC    *ftp = p->ftp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    asig = p->asig;
    q = p->prvq;
    for (n=offset; n<nsmps-early; n++) {
      q = p->c1 * asig[n] * asig[n] + p->c2 * q;
    }
    p->prvq = q;
    rms = SQRT(q);    /* get running rms      */
    if (rms < p->min_rms)
      rms = p->min_rms;
    if ((dist = *p->kdist) < FL(0.001))
      dist = FL(0.001);
    dnew = rms / dist;                  /* & compress factor    */
    dcur = p->prvd;
    asig = p->asig;
    ar = p->ar;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    dinc = (dnew - dcur) / nsmps;
    for (n=offset; n<nsmps; n++) {
      MYFLT sig, phs, val;
      sig = asig[n] / dcur;             /* compress the sample  */
      phs = p->midphs * (FL(1.0) + sig); /* as index into table  */
      if (UNLIKELY(phs <= FL(0.0)))
        val = p->begval;
      else if (UNLIKELY(phs >= p->maxphs))        /* check sticky bits    */
        val = p->endval;
      else {
        int32  iphs = (int32)phs;
        MYFLT frac = phs - (MYFLT)iphs; /* waveshape the samp   */
        MYFLT *fp = ftp->ftable + iphs;
        val = *fp++;
        val += (*fp - val) * frac;
      }
      ar[n] = val * dcur;               /* and restor the amp   */
      dcur += dinc;
    }
    p->prvd = dcur;

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY compress_localops[] = {
  { "compress", S(CMPRS), 0, 3, "a", "aakkkkkki", (SUBR) compset, (SUBR) compress },
  { "compress2", S(CMPRS), 0, 3, "a", "aakkkkkki", (SUBR)comp2set,(SUBR) compress },
  { "distort", S(DIST), TR, 3, "a", "akiqo", (SUBR) distset, (SUBR) distort },
};

LINKAGE_BUILTIN(compress_localops)
