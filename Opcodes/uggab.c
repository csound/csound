/*
    uggab.c:

    Copyright (C) 1998 Gabriel Maldonado, John ffitch

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

/********************************************/
/* wrap and mirror UGs by Gabriel Maldonado */
/* and others by same author                */
/* Code adapted by JPff 1998 Sep 19         */
/********************************************/

#include "stdopcod.h"
#include "uggab.h"
#include <math.h>

static int32_t wrap(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    MYFLT       *adest= p->xdest;
    MYFLT       *asig = p->xsig;
    MYFLT       xlow, xhigh, xsig;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(adest, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&adest[nsmps], '\0', early*sizeof(MYFLT));
    }
    if ((xlow=*p->xlow) >= (xhigh=*p->xhigh)) {
      MYFLT     xaverage;
      xaverage = (xlow + xhigh) * FL(0.5);
      for (n=offset; n<nsmps; n++) {
        adest[n] = xaverage;
      }
    }
    else
      for (n=offset; n<nsmps; n++) {
        if ((xsig=asig[n]) >= xlow )
          adest[n] = xlow + FMOD(xsig - xlow, FABS(xlow-xhigh));
        else
          adest[n] = xhigh- FMOD(xhigh- xsig, FABS(xlow-xhigh));
      }
    return OK;
}

static int32_t kwrap(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    MYFLT xsig, xlow, xhigh;

    if ((xlow=*p->xlow) >= (xhigh=*p->xhigh))
      *p->xdest = (xlow + xhigh)*FL(0.5);
    else {
      if ((xsig=*p->xsig) >= xlow )
        *p->xdest = xlow + FMOD(xsig - xlow, FABS(xlow-xhigh));
      else
        *p->xdest = xhigh- FMOD(xhigh- xsig, FABS(xlow-xhigh));
    }
    return OK;
}

/*---------------------------------------------------------------------*/

static int32_t kmirror(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    MYFLT  xsig, xlow, xhigh;
    xsig = *p->xsig;
    xhigh= *p->xhigh;
    xlow = *p->xlow;

    if (xlow >= xhigh) *p->xdest = (xlow + xhigh)*FL(0.5);
    else {
      while ((xsig > xhigh) || (xsig < xlow)) {
        if (xsig > xhigh)
          xsig = xhigh + xhigh - xsig;
        else
          xsig = xlow + xlow - xsig;
      }
      *p->xdest = xsig;
    }
    return OK;
}

static int32_t mirror(CSOUND *csound, WRAP *p)
{
    IGN(csound);
    MYFLT       *adest, *asig;
    MYFLT       xlow, xhigh, xaverage, xsig;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;

    adest = p->xdest;
    asig  = p->xsig;
    xlow = *p->xlow;
    xhigh = *p->xhigh;

    if (UNLIKELY(offset)) memset(adest, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&adest[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (xlow >= xhigh)  {
      xaverage = (xlow + xhigh)*FL(0.5);
      for (n=offset;n<nsmps;n++) {
        adest[n] = xaverage;
      }
      return OK;                   /* Suggested by Istvan Varga */
    }

    for (n=offset;n<nsmps;n++) {
      xsig = asig[n];
      while ((xsig > xhigh) || ( xsig < xlow )) {
        if (xsig > xhigh)
          xsig = xhigh + xhigh - xsig;
        else
          xsig = xlow + xlow - xsig;
      }
      adest[n] = xsig;
    }
    return OK;
}

static int32_t trig_set(CSOUND *csound, TRIG *p)
{
    IGN(csound);
    p->old_sig = FL(0.0);
    return OK;
}

static int32_t trig(CSOUND *csound, TRIG *p)
{
    MYFLT sig = *p->ksig;
    MYFLT threshold = *p->kthreshold;

    switch ((int32_t) MYFLT2LONG(*p->kmode)) {
    case 0:       /* down-up */
      if (p->old_sig <= threshold &&
          sig > threshold)
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    case 1:      /* up-down */
      if (p->old_sig >= threshold &&
          sig < threshold)
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    case 2:      /* both */
      if ((p->old_sig <= threshold && sig > threshold) ||
          (p->old_sig >= threshold && sig < threshold ) )
        *p->kout = FL(1.0);
      else
        *p->kout = FL(0.0);
      break;
    default:
      return
        csound->PerfError(csound, &(p->h), "%s",
                          Str(" bad imode value"));
    }
    p->old_sig = sig;
    return OK;
}

/*-------------------------------*/

static int32_t interpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    MYFLT point_value = (*p->point - *p->imin) / (*p->imax - *p->imin);
    *p->r = point_value * (*p->val2 - *p->val1) + *p->val1;
    return OK;
}

static int32_t nterpol_init(CSOUND *csound, INTERPOL *p)
{
    if (LIKELY(*p->imax != *p->imin))
      p->point_factor = FL(1.0)/(*p->imax - *p->imin);
    else
      return csound->InitError(csound, "%s", Str("Min and max the same"));
    return OK;
 }

static int32_t knterpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    MYFLT point_value = (*p->point - *p->imin ) * p->point_factor;
    *p->r = point_value * (*p->val2 - *p->val1) + *p->val1;
    return OK;
}

static int32_t anterpol(CSOUND *csound, INTERPOL *p)
{
    IGN(csound);
    MYFLT point_value = (*p->point - *p->imin ) * p->point_factor;
    MYFLT *out        = p->r, *val1 = p->val1, *val2 = p->val2;
    uint32_t offset   = p->h.insdshead->ksmps_offset;
    uint32_t early    = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT fv1 = val1[n];
      out[n] = point_value * (val2[n] - fv1) + fv1;
    }
    return OK;
}

      
static int32_t sum_init(CSOUND *csound, SUM *p) {
    uint32_t nsmps = CS_KSMPS;
    if (p->aux.auxp == NULL || p->aux.size < nsmps * sizeof(MYFLT))
      csound->AuxAlloc(csound, (size_t)(nsmps*sizeof(MYFLT)), &p->aux);
    return OK;
}


static int32_t sum_(CSOUND *csound, SUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    int32_t   count = (int32_t) p->INOCOUNT;
    MYFLT *accum = p->aux.auxp, **args = p->argums;
    MYFLT *in0, *in1, *in2, *in3;
    if (UNLIKELY(offset)) memset(accum, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&accum[nsmps], '\0', early*sizeof(MYFLT));
    }
    memset(accum, '\0', nsmps*sizeof(MYFLT));
    int32_t count4 = count - (count % 4);
    for(int32_t i=0; i<count4; i+= 4) {
      in0 = *(args+i);
      in1 = *(args+i+1);
      in2 = *(args+i+2);
      in3 = *(args+i+3);
      for(k=offset; k<nsmps; k++) {
        // ar[k] += in0[k] + in1[k] +in2[k] + in3[k];
        accum[k] += in0[k] + in1[k] +in2[k] + in3[k];
      }
    }
    for(int32_t i=count4; i<count; i++) {
      in0 = *(args + i);
      for(k=offset; k<nsmps; k++) {
        // ar[k] += in0[k];
        accum[k] += in0[k];
      }
    }
    memcpy(p->ar+offset, accum+offset, (nsmps - offset)*sizeof(MYFLT));

    return OK;
}

/* Actually by JPff but after Gabriel */
static int32_t product(CSOUND *csound, SUM *p)
{
    IGN(csound);
    int32_t    count = (int32_t) p->INOCOUNT;
    uint32_t  offset = p->h.insdshead->ksmps_offset;
    uint32_t  early  = p->h.insdshead->ksmps_no_end;
    uint32_t   k, nsmps = CS_KSMPS;
    MYFLT *ar = p->ar, **args = p->argums;
    MYFLT *ag = *args;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    memcpy(&ar[offset], &ag[offset], sizeof(MYFLT)*(nsmps-offset));
    while (--count) {
      ag = *(++args);                   /* over all arguments */
      for (k=offset; k<nsmps; k++) {
        ar[k] *= ag[k];                 /* Over audio vector */
      }
    }
    return OK;
}

static int32_t rsnsety(CSOUND *csound, RESONY *p)
{
    int32_t scale;
    uint32_t nsmps = CS_KSMPS;
    p->scale = scale = (int32_t) *p->iscl;
    if ((p->loop = (int32_t) MYFLT2LONG(*p->ord)) < 1)
      p->loop = 4;  /* default value */
    if (!*p->istor && (p->aux.auxp == NULL ||
                      (uint32_t) (p->loop * 2 * sizeof(MYFLT)) > p->aux.size))
      csound->AuxAlloc(csound, (size_t) (p->loop * 2 * sizeof(MYFLT)), &p->aux);
    p->yt1 = (MYFLT*)p->aux.auxp; p->yt2 = (MYFLT*)p->aux.auxp + p->loop;
    if (UNLIKELY(scale && scale != 1 && scale != 2)) {
      return csound->InitError(csound, Str("illegal reson iscl value: %f"),
                                       *p->iscl);
    }
    if (!(*p->istor)) {
      memset(p->yt1, 0, p->loop*sizeof(MYFLT));
      memset(p->yt2, 0, p->loop*sizeof(MYFLT));
      /* for (j = 0; j < p->loop; j++) */
      /*   p->yt1[j] = p->yt2[j] = FL(0.0); */
    }
    if (p->buffer.auxp == NULL || p->buffer.size<nsmps*sizeof(MYFLT))
      csound->AuxAlloc(csound, (size_t)(nsmps*sizeof(MYFLT)), &p->buffer);
    return OK;
}

static int32_t resony(CSOUND *csound, RESONY *p)
{
    int32_t j;
    MYFLT   *ar = p->ar, *asig;
    MYFLT   c3p1, c3t4, omc3, c2sqr;
    MYFLT   *yt1, *yt2, c1, c2, c3, cosf;
    double  cf;
    int32_t loop = p->loop;
    if (UNLIKELY(loop==0))
      return csound->InitError(csound, "%s", Str("loop cannot be zero"));
    {
      MYFLT   sep = (*p->sep / (MYFLT) loop);
      int32_t     flag = (int32_t) *p->iflag;
      MYFLT   *buffer = (MYFLT*) (p->buffer.auxp);
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;

      asig = p->asig;

      memset(buffer, 0, nsmps*sizeof(MYFLT));
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      }

      yt1 = p->yt1;
      yt2 = p->yt2;

      for (j = 0; j < loop; j++) {
        if (flag)                     /* linear separation in hertz */
          cosf = (MYFLT) cos((cf = (double) (*p->kcf * sep * j))
                             * (double) CS_TPIDSR);
        else                          /* logarithmic separation in octaves */
          cosf = (MYFLT) cos((cf = (double) (*p->kcf * pow(2.0, sep * j)))
                             * (double) CS_TPIDSR);
        c3 = EXP(*p->kbw * (cf / *p->kcf) * CS_MTPIDSR);
        c3p1 = c3 + FL(1.0);
        c3t4 = c3 * FL(4.0);
        c2 = c3t4 * cosf / c3p1;
        c2sqr = c2 * c2;
        omc3 = FL(1.0) - c3;
        if (p->scale == 1)
          c1 = omc3 * SQRT(FL(1.0) - c2sqr / c3t4);
        else if (p->scale == 2)
          c1 = SQRT((c3p1*c3p1-c2sqr) * omc3/c3p1);
        else
          c1 = FL(1.0);
        for (n = offset; n < nsmps; n++) {
          MYFLT temp = c1 * asig[n] + c2 * *yt1 - c3 * *yt2;
          buffer[n] += temp;
          *yt2 = *yt1;
          *yt1 = temp;
        }
        yt1++;
        yt2++;
      }
      memcpy(&ar[offset], &buffer[offset], sizeof(MYFLT)*(nsmps-offset));
      return OK;
    }
}

static int32_t fold_set(CSOUND *csound, FOLD *p)
{
    IGN(csound);
    p->sample_index = 0;
    p->index = 0.0;
    p->value = FL(0.0);         /* This was not initialised -- JPff */
    return OK;
}

static int32_t fold(CSOUND *csound, FOLD *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *ar = p->ar;
    MYFLT *asig = p->asig;
    MYFLT kincr = *p->kincr;
    double index = p->index;
    int32 sample_index = p->sample_index;
    MYFLT value = p->value;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (index < (double)sample_index) {
        index += (double)kincr;
        ar[n]  = value = asig[n];
      }
      else ar[n]= value;
      sample_index++;
    }
    p->index = index;
    p->sample_index = sample_index;
    p->value = value;
    return OK;
}

/* by Gab Maldonado. Under GNU license with a special exception for
   Canonical Csound addition */

static int32_t loopseg_set(CSOUND *csound, LOOPSEG *p)
{
    p->nsegs   = p->INOCOUNT-3;
    // Should check this is even
    if (UNLIKELY((p->nsegs&1)!=0))
      csound->Warning(csound, "%s", Str("loop opcode: wrong argument count"));
    p->args[0] = FL(0.0);
    p->phs     = *p->iphase;
    return OK;
}

static int32_t loopseg(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    MYFLT *argp=p->args;
    MYFLT beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;
    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        MYFLT diff = end_seg - beg_seg;
        MYFLT fract = ((MYFLT)phs-beg_seg)/diff;
        MYFLT v1 = argp[j+1];
        MYFLT v2 = argp[j+3];
        *p->out = v1 + (v2-v1) * fract;
        break;
      }
    }
    phs    += si;
    while (phs >= 1.0)
      phs -= 1.0;
    while (phs < 0.0 )
      phs += 1.0;
    p->phs = phs;
    return OK;
}

static int32_t loopxseg(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    MYFLT exp1 = FL(1.0)/(FL(1.0)-EXP(FL(1.0)));
    MYFLT *argp=p->args;
    MYFLT beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;
    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        MYFLT diff = end_seg - beg_seg;
        MYFLT fract = ((MYFLT)phs-beg_seg)/diff;
        MYFLT v1 = argp[j+1];
        MYFLT v2 = argp[j+3];
        *p->out = v1 + (v2 - v1) * (1 - EXP(fract)) * exp1;
        break;
      }
    }
    phs    += si;
    while (phs >= 1.0)
      phs -= 1.0;
    while (phs < 0.0 )
      phs += 1.0;
    p->phs = phs;
    return OK;
}

static int32_t looptseg_set(CSOUND *csound, LOOPTSEG *p)
{
    IGN(csound);
    p->nsegs   = (p->INOCOUNT-2)/3;
    p->phs     = *p->iphase;
    return OK;
}

static int32_t looptseg(CSOUND *csound, LOOPTSEG *p)
{
    IGN(csound);
    MYFLT beg_seg=FL(0.0), end_seg=FL(0.0), durtot=FL(0.0);
    double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs;
    int32_t j;

    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;

    for ( j=0; j<nsegs; j++)
      durtot += *(p->argums[j].time);
    for ( j=0; j < nsegs; j++) {
      beg_seg = end_seg;
      end_seg = beg_seg + *(p->argums[j].time) / durtot;
      if (beg_seg <= phs && end_seg > phs) {
        MYFLT alpha = *(p->argums[j].type);
        MYFLT diff = end_seg - beg_seg;
        MYFLT fract = ((MYFLT)phs-beg_seg)/diff;
        MYFLT v1 = *(p->argums[j].start);
        MYFLT v2 = (j!=nsegs-1)?*(p->argums[j+1].start):*(p->argums[0].start);
        if (alpha==FL(0.0))
          *p->out = v1 + (v2 - v1) * fract;
        else
          *p->out = v1 +
            (v2 - v1) * (FL(1.0)-EXP(alpha*fract))/(FL(1.0)-EXP(alpha));
        break;
      }
    }
    phs    += si;
    while (UNLIKELY(phs >= 1.0))
      phs -= 1.0;
    while (UNLIKELY(phs < 0.0 ))
      phs += 1.0;
    p->phs = phs;
    return OK;
}

static int32_t lpshold(CSOUND *csound, LOOPSEG *p)
{
    IGN(csound);
    MYFLT *argp=p->args;
    MYFLT beg_seg=0, end_seg, durtot=FL(0.0);
    double   phs, si=*p->freq*CS_ONEDKR;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    if (*p->retrig)
      phs=p->phs=*p->iphase;
    else
      phs=p->phs;

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];
    argp[nsegs] = *p->argums[0];
    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];

    for ( j=0; j < nsegs; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;
      if (beg_seg <= phs && end_seg > phs) {
        if (beg_seg <= phs && end_seg > phs) {
          *p->out = argp[j+1];
          break;
        }
      }
    }
    phs    += si;
    while (phs >= 1.0)
      phs -= 1.0;
    while (phs < 0.0 )
      phs += 1.0;
    p->phs = phs;
    return OK;
}

static int32_t loopsegp_set(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    p->nsegs   = p->INOCOUNT-1;
    p->args[0] = FL(0.0);
    return OK;
}

static int32_t loopsegp(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    MYFLT *argp = p->args;
    MYFLT beg_seg=0, end_seg, durtot=FL(0.0);
    MYFLT phs;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    phs = *p->kphase;

    while (phs >= FL(1.0))
      phs -= FL(1.0);
    while (phs < FL(0.0))
      phs += FL(1.0);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        MYFLT diff = end_seg - beg_seg;
        MYFLT fract = ((MYFLT)phs-beg_seg)/diff;
        MYFLT v1 = argp[j+1];
        MYFLT v2 = argp[j+3];
        *p->out = v1 + (v2-v1) * fract;
        break;
      }
    }
    return OK;
}

static int32_t lpsholdp(CSOUND *csound, LOOPSEGP *p)
{
    IGN(csound);
    MYFLT *argp=p->args;
    MYFLT beg_seg=FL(0.0), end_seg, durtot=FL(0.0);
    MYFLT phs;
    int32_t nsegs=p->nsegs+1;
    int32_t j;

    phs = *p->kphase;

    while (phs >= FL(1.0))
      phs -= FL(1.0);
    while (phs < FL(0.0))
      phs += FL(1.0);

    for (j=1; j<nsegs; j++)
      argp[j] = *p->argums[j-1];

    argp[nsegs] = *p->argums[0];

    for ( j=0; j <nsegs; j+=2)
      durtot += argp[j];
    for ( j=0; j < nsegs; j+=2) {
      beg_seg += argp[j] / durtot;
      end_seg = beg_seg + argp[j+2] / durtot;

      if (beg_seg <= phs && end_seg > phs) {
        if (beg_seg <= phs && end_seg > phs) {
          *p->out = argp[j+1];
          break;
        }
      }
    }
    return OK;
}

/* by Gab Maldonado. Under GNU license with a special exception
   for Canonical Csound addition */

static int32_t lineto_set(CSOUND *csound, LINETO *p)
{
    IGN(csound);
    p->current_time = FL(0.0);
    p->incr=FL(0.0);
    p->old_time=FL(0.0);
    p->flag = 1;
    return OK;
}

static int32_t lineto(CSOUND *csound, LINETO *p)
{
    IGN(csound);
    if (UNLIKELY(p->flag)) {
      p->val_incremented = p->current_val = *p->ksig;
      p->flag=0;
    }
    /* printf("lineto: ktime=%lf ksig=%lf\n " */
    /*        "old_time=%lf val_inc=%lf incr=%lf val=%lf\n", */
    /*        *p->ktime, *p->ksig, p->old_time, p->val_incremented, p->incr, */
    /*        p->current_val); */
    if (*p->ksig != p->current_val && p->current_time > p->old_time) {
      p->old_time = *p->ktime;
      p->val_incremented = p->current_val;
      p->current_time = FL(0.0);
      p->incr = (*p->ksig - p->current_val)
                / ((int32) (CS_EKR * p->old_time) -1); /* by experiment */
      p->current_val = *p->ksig;
    }
    else if (p->current_time < p->old_time) {
      p->val_incremented += p->incr;
    }
    p->current_time += 1/CS_EKR;
    *p->kr = p->val_incremented;
    return OK;
}

static int32_t tlineto_set(CSOUND *csound, LINETO2 *p)
{
    IGN(csound);
    p->current_time = FL(0.0);
    p->incr=FL(0.0);
    p->old_time=FL(1.0);
    p->flag = 1;
    return OK;
}

static int32_t tlineto(CSOUND *csound, LINETO2 *p)
{
    IGN(csound);
    if (UNLIKELY(p->flag)) {
      p->val_incremented = p->current_val = *p->ksig;
      p->flag=0;
    }
    if (*p->ktrig) {
      p->old_time = *p->ktime;
      /* p->val_incremented = p->current_val; */
      p->current_time = FL(0.0);
      p->incr = (*p->ksig - p->current_val)
                / ((int32) (CS_EKR * p->old_time) + 1);
      p->current_val = *p->ksig;
    }
    else if (p->current_time < p->old_time) {
      p->current_time += CS_ONEDKR;
      p->val_incremented += p->incr;
    }
    *p->kr = p->val_incremented;
    return OK;
}

/* by Gabriel Maldonado. Under GNU license with a special exception
   for Canonical Csound addition */

static int32_t vibrato_set(CSOUND *csound, VIBRATO *p)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0 && *p->iphs<1.0)
        p->lphs = (((int64_t)(*p->iphs * FMAXLEN)) & PHMASK) >> ftp->lobits;
      else if (UNLIKELY(*p->iphs>=1.0))
        return csound->InitError(csound, "%s", Str("vibrato@ Phase out of range"));
    }
    else return NOTOK;
    p->xcpsAmpRate = randGab(csound) *(*p->cpsMaxRate - *p->cpsMinRate) +
      *p->cpsMinRate;
    p->xcpsFreqRate = randGab(csound) *(*p->ampMaxRate - *p->ampMinRate) +
      *p->ampMinRate;
    p->tablen = ftp->flen;
    p->tablenUPkr = p->tablen * CS_ONEDKR;
    return OK;
}

static int32_t vibrato(CSOUND *csound, VIBRATO *p)
{
    FUNC        *ftp;
    double      phs, inc;
    MYFLT       *ftab, fract, v1;
    MYFLT       RandAmountAmp,RandAmountFreq;

    RandAmountAmp = (p->num1amp + (MYFLT)p->phsAmpRate * p->dfdmaxAmp) *
      *p->randAmountAmp ;
    RandAmountFreq = (p->num1freq + (MYFLT)p->phsFreqRate * p->dfdmaxFreq) *
      *p->randAmountFreq ;

    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    fract = (MYFLT) (phs - (int32)phs);
    ftab = ftp->ftable + (int32)phs;
    v1 = *ftab++;
    *p->out = (v1 + (*ftab - v1) * fract) *
      (*p->AverageAmp * POWER(FL(2.0),RandAmountAmp));
    inc = ( *p->AverageFreq * POWER(FL(2.0),RandAmountFreq)) *  p->tablenUPkr;
    phs += inc;
    while (phs >= p->tablen)
      phs -= p->tablen;
    while (phs < 0.0 )
      phs += p->tablen;
    p->lphs = phs;
    p->phsAmpRate += (int32)(p->xcpsAmpRate * CS_KICVT);
    if (p->phsAmpRate >= MAXLEN) {
      p->xcpsAmpRate =  randGab(csound)  * (*p->ampMaxRate - *p->ampMinRate) +
        *p->ampMinRate;
      p->phsAmpRate &= PHMASK;
      p->num1amp = p->num2amp;
      p->num2amp = BiRandGab(csound) ;
      p->dfdmaxAmp = (p->num2amp - p->num1amp) / FMAXLEN;
    }
    p->phsFreqRate += (int32)(p->xcpsFreqRate * CS_KICVT);
    if (p->phsFreqRate >= MAXLEN) {
      p->xcpsFreqRate =  randGab(csound)  * (*p->cpsMaxRate - *p->cpsMinRate) +
        *p->cpsMinRate;
      p->phsFreqRate &= PHMASK;
      p->num1freq = p->num2freq;
      p->num2freq = BiRandGab(csound) ;
      p->dfdmaxFreq = (p->num2freq - p->num1freq) / FMAXLEN;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("vibrato(krate): not initialised"));
}

static int32_t vibr_set(CSOUND *csound, VIBR *p)
  /* faster and easier to use than vibrato, but less flexible */
{
    FUNC        *ftp;
#define randAmountAmp   FL(1.59055)   /* these default values are far from */
#define randAmountFreq  FL(0.629921)  /* being the best.  If you think you */
#define ampMinRate      FL(1.0)       /* found better ones, please tell me */
#define ampMaxRate      FL(3.0)       /* by posting a message to
                                         g.maldonado@agora.stm.it */
#define cpsMinRate      FL(1.19377)
#define cpsMaxRate      FL(2.28100)
#define iphs            FL(0.0)

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      p->lphs = (((int32)(iphs * FMAXLEN)) & PHMASK) >> ftp->lobits;
    }
    else return NOTOK;
    p->xcpsAmpRate = randGab(csound)  * (cpsMaxRate - cpsMinRate) + cpsMinRate;
    p->xcpsFreqRate = randGab(csound)  * (ampMaxRate - ampMinRate) + ampMinRate;
    p->tablen = ftp->flen;
    p->tablenUPkr = p->tablen * CS_ONEDKR;
    return OK;
}

static int32_t vibr(CSOUND *csound, VIBR *p)
{
    FUNC        *ftp;
    double      phs, inc;
    MYFLT       *ftab, fract, v1;
    MYFLT       rAmountAmp,rAmountFreq;

    rAmountAmp =
      (p->num1amp+(MYFLT)p->phsAmpRate * p->dfdmaxAmp)*randAmountAmp;
    rAmountFreq =
      (p->num1freq+(MYFLT)p->phsFreqRate*p->dfdmaxFreq)*randAmountFreq;
    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vibrato(krate): not initialised"));
    }
    fract = (MYFLT) (phs - (int32)phs); /*PFRAC(phs);*/
    ftab = ftp->ftable + (int32)phs; /*(phs >> ftp->lobits);*/
    v1 = *ftab++;
    *p->out = (v1 + (*ftab - v1) * fract) *
      (*p->AverageAmp * POWER(FL(2.0),rAmountAmp));
    inc = ( *p->AverageFreq * POWER(FL(2.0),rAmountFreq) ) *  p->tablenUPkr;
    phs += inc;
    while (phs >= p->tablen)
      phs -= p->tablen;
    while (phs < 0.0 )
      phs += p->tablen;
    p->lphs = phs;

    p->phsAmpRate += (int32)(p->xcpsAmpRate * CS_KICVT);
    if (p->phsAmpRate >= MAXLEN) {
      p->xcpsAmpRate =  randGab(csound)  * (ampMaxRate - ampMinRate) + ampMinRate;
      p->phsAmpRate &= PHMASK;
      p->num1amp = p->num2amp;
      p->num2amp = BiRandGab(csound);
      p->dfdmaxAmp = (p->num2amp - p->num1amp) / FMAXLEN;
    }

    p->phsFreqRate += (int32)(p->xcpsFreqRate * CS_KICVT);
    if (p->phsFreqRate >= MAXLEN) {
      p->xcpsFreqRate =  randGab(csound)  * (cpsMaxRate - cpsMinRate) + cpsMinRate;
      p->phsFreqRate &= PHMASK;
      p->num1freq = p->num2freq;
      p->num2freq = BiRandGab(csound);
      p->dfdmaxFreq = (p->num2freq - p->num1freq) / FMAXLEN;
    }
#undef  randAmountAmp
#undef  randAmountFreq
#undef  ampMinRate
#undef  ampMaxRate
#undef  cpsMinRate
#undef  cpsMaxRate
#undef  iphs
    return OK;
}

static int32_t jitter2_set(CSOUND *csound, JITTER2 *p)
{
    if (*p->cps1==FL(0.0) && *p->cps2==FL(0.0) && /* accept default values */
        *p->cps2==FL(0.0) && *p->amp1==FL(0.0) &&
        *p->amp2==FL(0.0) && *p->amp3==FL(0.0))
      p->flag = 1;
    else
      p->flag = 0;
    p->dfdmax1 = p->dfdmax2 = p->dfdmax3 = FL(0.0);
    p->phs1 = p->phs2 = p->phs3 = 0;
    p->num1a = p->num1b = p->num1c = FL(0.0); /* JPff Jul 2016 */
    if (*p->option != FL(0.0)) {
      p->num1a   = p->num2a;
      p->num2a   = BiRandGab(csound);
      p->dfdmax1 = (p->num2a - p->num1a) / FMAXLEN;
      p->num1b   = p->num2b;
      p->num2b   = BiRandGab(csound);
      p->dfdmax2 = (p->num2b- p->num1b) / FMAXLEN;
      p->num1c   = p->num2c;
      p->num2c   = BiRandGab(csound);
      p->dfdmax3 = (p->num2c- p->num1c) / FMAXLEN;
    }
    return OK;
}

static int32_t jitter2(CSOUND *csound, JITTER2 *p)
{
    MYFLT out1,out2,out3;
    out1 = (p->num1a + (MYFLT)p->phs1 * p->dfdmax1);
    out2 = (p->num1b + (MYFLT)p->phs2 * p->dfdmax2);
    out3 = (p->num1c + (MYFLT)p->phs3 * p->dfdmax3);

    if (p->flag) { /* accept default values */
      *p->out  = (out1* FL(0.5) + out2 * FL(0.3) + out3* FL(0.2)) * *p->gamp;
      p->phs1 += (int32) (FL(0.82071231913) * CS_KICVT);
      p->phs2 += (int32) (FL(7.009019029039107) * CS_KICVT);
      p->phs3 += (int32) (FL(10.0) * CS_KICVT);
    }
    else {
      *p->out  = (out1* *p->amp1 + out2* *p->amp2 +out3* *p->amp3) * *p->gamp;
      p->phs1 += (int32)( *p->cps1 * CS_KICVT);
      p->phs2 += (int32)( *p->cps2 * CS_KICVT);
      p->phs3 += (int32)( *p->cps3 * CS_KICVT);
    }
    if (p->phs1 >= MAXLEN) {
      p->phs1   &= PHMASK;
      p->num1a   = p->num2a;
      p->num2a   = BiRandGab(csound);
      p->dfdmax1 = (p->num2a - p->num1a) / FMAXLEN;
    }
    if (p->phs2 >= MAXLEN) {
      p->phs2   &= PHMASK;
      p->num1b   = p->num2b;
      p->num2b   = BiRandGab(csound);
      p->dfdmax2 = (p->num2b - p->num1b) / FMAXLEN;
    }
    if (p->phs3 >= MAXLEN) {
      p->phs3   &= PHMASK;
      p->num1c   = p->num2c;
      p->num2c   = BiRandGab(csound);
      p->dfdmax3 = (p->num2c - p->num1c) / FMAXLEN;
    }
    return OK;
}

static int32_t jitter_set(CSOUND *csound, JITTER *p)
{
    p->num2     = BiRandGab(csound);
    p->initflag = 1;
    p->phs=0;
    return OK;
}

static int32_t jitter(CSOUND *csound, JITTER *p)
{
    if (p->initflag) {
      p->initflag = 0;
      *p->ar = p->num2 * *p->amp;
      goto next;
    }
    *p->ar = (p->num1 + (MYFLT)p->phs * p->dfdmax) * *p->amp;
    p->phs += (int32)(p->xcps * CS_KICVT);

    if (p->phs >= MAXLEN) {
    next:
      p->xcps   = randGab(csound)  * (*p->cpsMax - *p->cpsMin) + *p->cpsMin;
      p->phs   &= PHMASK;
      p->num1   = p->num2;
      p->num2   = BiRandGab(csound);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

static int32_t jitters_set(CSOUND *csound, JITTERS *p)
{
    p->num1     = BiRandGab(csound);
    p->num2     = BiRandGab(csound);
    p->df1      = FL(0.0);
    p->initflag = 1;
    p->cod      = IS_ASIG_ARG(p->amp) ? 1 : 0;
    p->phs      = 0;
    return OK;
}

static int32_t jitters(CSOUND *csound, JITTERS *p)
{
    MYFLT       x, c3= p->c3, c2= p->c2;
    MYFLT       f0 = p->num0, df0= p->df0;

    if (p->initflag == 1) {
      p->initflag = 0;
      goto next;
    }
    p->phs += p->si;
    if (p->phs >= 1.0) {
      MYFLT     slope, resd1, resd0, f2, f1;
    next:
      p->si = (randGab(csound) * (*p->cpsMax-*p->cpsMin) + *p->cpsMin)*CS_ONEDKR;
      if (p->si == 0) p->si = 1; /* Is this necessary? */
      while (p->phs > 1.0)
        p->phs -= 1.0;
      f0 = p->num0 = p->num1;
      f1 = p->num1 = p->num2;
      f2 = p->num2 = BiRandGab(csound);
      df0 = p->df0 = p->df1;
      p->df1 = ( f2  - f0 ) * FL(0.5);
      slope = f1 - f0;
      resd0 = df0 - slope;
      resd1 = p->df1 - slope;
      c3 = p->c3 = resd0 + resd1;
      c2 = p->c2 = - (resd1 + FL(2.0)* resd0);
    }
    x= (MYFLT) p->phs;
    *p->ar = (((c3 * x + c2) * x + df0) * x + f0) * *p->amp;
    return OK;
}

static int32_t jittersa(CSOUND *csound, JITTERS *p)
{
    MYFLT   x, c3=p->c3, c2=p->c2;
    MYFLT   f0= p->num0, df0 = p->df0;
    MYFLT   *ar = p->ar, *amp = p->amp;
    MYFLT   cpsMax = *p->cpsMax, cpsMin = *p->cpsMin;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  cod = p->cod;
    double phs = p->phs, si = p->si;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->initflag) {
      p->initflag = 0;
      n = offset;
      goto next;
    }
    for (n=offset; n<nsmps; n++) {
      phs += si;
      if (phs >= 1.0) {
        MYFLT   slope, resd1, resd0, f2, f1;
      next:
        si =  (randGab(csound)  * (cpsMax - cpsMin) + cpsMin)*CS_ONEDSR;
        if (si == 0) si = 1; /* Is this necessary? */
        while (phs > 1.0)
          phs -= 1.0;
        f0 = p->num0 = p->num1;
        f1 = p->num1 = p->num2;
        f2 = p->num2 = BiRandGab(csound);
        df0 = p->df0 = p->df1;
        p->df1 = ( f2 - f0 ) * FL(0.5);
        slope = f1 - f0;
        resd0 = df0 - slope;
        resd1 = p->df1 - slope;
        c3 = p->c3 = resd0 + resd1;
        c2 = p->c2 = - (resd1 + FL(2.0)* resd0);
      }
      x = (MYFLT) phs;
      ar[n] = (((c3 * x + c2) * x + df0) * x + f0) * *amp;
      if (cod) amp++;
    }
    p->phs = phs;
    p->si =si;
    return OK;
}

static int32_t kDiscreteUserRand(CSOUND *csound, DURAND *p)
{ /* gab d5*/
    if (p->pfn != (int32)*p->tableNum) {
      if (UNLIKELY( (p->ftp = csound->FTFind(csound, p->tableNum) ) == NULL))
        goto err1;
      p->pfn = (int32)*p->tableNum;
    }
    *p->out = p->ftp->ftable[(int32)(randGab(csound) * p->ftp->flen)];
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("Invalid ftable no. %f"),
                             *p->tableNum);
}

static int32_t iDiscreteUserRand(CSOUND *csound, DURAND *p)
{
    p->pfn = 0L;
    kDiscreteUserRand(csound,p);
    return OK;
}

static int32_t aDiscreteUserRand(CSOUND *csound, DURAND *p)
{ /* gab d5*/
    MYFLT *out = p->out, *table;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, flen;

    if (p->pfn != (int32)*p->tableNum) {
      if (UNLIKELY( (p->ftp = csound->FTFind(csound, p->tableNum) ) == NULL))
        goto err1;
      p->pfn = (int32)*p->tableNum;
    }
    table = p->ftp->ftable;
    flen = p->ftp->flen;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      out[n] = table[(int32)(randGab(csound)) * flen];
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("Invalid ftable no. %f"),
                             *p->tableNum);
}

static int32_t kContinuousUserRand(CSOUND *csound, CURAND *p)
{ /* gab d5*/
    int32 indx;
    MYFLT findx, fract, v1, v2;
    if (p->pfn != (int32)*p->tableNum) {
      if (UNLIKELY( (p->ftp = csound->FTFind(csound, p->tableNum) ) == NULL))
        goto err1;
      p->pfn = (int32)*p->tableNum;
    }
    findx = (MYFLT) (randGab(csound) * p->ftp->flen);
    indx = (int32) findx;
    fract = findx - indx;
    v1 = *(p->ftp->ftable + indx);
    v2 = *(p->ftp->ftable + indx + 1);
    *p->out = (v1 + (v2 - v1) * fract) * (*p->max - *p->min) + *p->min;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("Invalid ftable no. %f"),
                             *p->tableNum);
}

static int32_t iContinuousUserRand(CSOUND *csound, CURAND *p)
{
    p->pfn = 0;
    kContinuousUserRand(csound,p);
    return OK;
}

static int32_t Cuserrnd_set(CSOUND *csound, CURAND *p)
{
    IGN(csound);
    p->pfn = 0;
    return OK;
}

static int32_t aContinuousUserRand(CSOUND *csound, CURAND *p)
{ /* gab d5*/
    MYFLT min = *p->min, rge = *p->max;
    MYFLT *out = p->out, *table;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS, flen;
    int32_t indx;
    MYFLT findx, fract,v1,v2;

    if (p->pfn != (int32)*p->tableNum) {
      if (UNLIKELY( (p->ftp = csound->FTFind(csound, p->tableNum) ) == NULL))
        goto err1;
      p->pfn = (int32)*p->tableNum;
    }

    table = p->ftp->ftable;
    flen = p->ftp->flen;

    rge -= min;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      findx = (MYFLT) (randGab(csound) * flen);
      indx = (int32) findx;
      fract = findx - indx;
      v1 = table[indx];
      v2 = table[indx+1];
      out[n] = (v1 + (v2 - v1) * fract) * rge + min;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("Invalid ftable no. %f"),
                             *p->tableNum);
}

static int32_t ikRangeRand(CSOUND *csound, RANGERAND *p)
{ /* gab d5*/
    *p->out = randGab(csound) * (*p->max - *p->min) + *p->min;
    return OK;
}

static int32_t aRangeRand(CSOUND *csound, RANGERAND *p)
{ /* gab d5*/
    MYFLT min = *p->min, max = *p->max, *out = p->out;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT rge = max - min;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      out[n] = randGab(csound) * rge + min;
    }
    return OK;
}

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
static int32_t randomi_set(CSOUND *csound, RANDOMI *p)
{
    int32_t mode = (int32_t)(*p->mode);
    p->phs = 0;
    switch (mode) {
    case 1: /* immediate interpolation between kmin and 1st random number */
        p->num1 = FL(0.0);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    case 2: /* immediate interpolation between ifirstval and 1st random number */
        p->num1 = (*p->max - *p->min) ?
          (*p->fstval - *p->min) / (*p->max - *p->min) : FL(0.0);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    case 3: /* immediate interpolation between 1st and 2nd random number */
        p->num1 = randGab(csound);
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
        break;
    default: /* old behaviour as developped by Gabriel */
        p->num1 = p->num2 = FL(0.0);
        p->dfdmax = FL(0.0);
    }
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

static int32_t krandomi(CSOUND *csound, RANDOMI *p)
{
    *p->ar = (p->num1 + (MYFLT)p->phs * p->dfdmax) * (*p->max - *p->min) + *p->min;
    p->phs += (int32)(*p->xcps * CS_KICVT);
    if (p->phs >= MAXLEN) {
      p->phs   &= PHMASK;
      p->num1   = p->num2;
      p->num2   = randGab(csound);
      p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
    }
    return OK;
}

static int32_t randomi(CSOUND *csound, RANDOMI *p)
{
    int32       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *cpsp;
    MYFLT       amp, min;

    cpsp = p->xcps;
    min = *p->min;
    amp =  (*p->max - min);
    ar = p->ar;
    inc = (int32)(*cpsp++ * CS_SICVT);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      ar[n] = (p->num1 + (MYFLT)phs * p->dfdmax) * amp + min;
      phs += inc;
      if (p->cpscod)
        inc = (int32)(*cpsp++ * CS_SICVT);
      if (phs >= MAXLEN) {
        phs &= PHMASK;
        p->num1 = p->num2;
        p->num2 = randGab(csound);
        p->dfdmax = (p->num2 - p->num1) / FMAXLEN;
      }
    }
    p->phs = phs;
    return OK;
}

/* mode and fstval arguments added */
/* by Francois Pinot, jan. 2011    */
static int32_t randomh_set(CSOUND *csound, RANDOMH *p)
{
    int32_t mode = (int32_t)(*p->mode);
    p->phs = 0;
    switch (mode) {
    case 2: /* the first output value is ifirstval */
        p->num1 = (*p->max - *p->min) ?
          (*p->fstval - *p->min) / (*p->max - *p->min) : FL(0.0);
        break;
    case 3: /* the first output value is a random number within the defined range */
        p->num1 = randGab(csound);
        break;
    default: /* old behaviour as developped by Gabriel */
        p->num1 = FL(0.0);
    }
    p->cpscod = IS_ASIG_ARG(p->xcps) ? 1 : 0;
    return OK;
}

static int32_t krandomh(CSOUND *csound, RANDOMH *p)
{
    *p->ar = p->num1 * (*p->max - *p->min) + *p->min;
    p->phs += (int32)(*p->xcps * CS_KICVT);
    if (p->phs >= MAXLEN) {
      p->phs &= PHMASK;
      p->num1 = randGab(csound);
    }
    return OK;
}

static int32_t randomh(CSOUND *csound, RANDOMH *p)
{
    int32       phs = p->phs, inc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *ar, *cpsp;
    MYFLT       amp, min;

    cpsp = p->xcps;
    min  = *p->min;
    amp  = (*p->max - min);
    ar   = p->ar;
    inc  = (int32)(*cpsp++ * CS_SICVT);
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      ar[n]     = p->num1 * amp + min;
      phs      += inc;
      if (p->cpscod)
        inc     = (int32)(*cpsp++ * CS_SICVT);
      if (phs >= MAXLEN) {
        phs    &= PHMASK;
        p->num1 = randGab(csound);
      }
    }
    p->phs = phs;
    return OK;
}

static int32_t random3_set(CSOUND *csound, RANDOM3 *p)
{
    p->num1     = randGab(csound);
    p->num2     = randGab(csound);
    p->df1      = FL(0.0);
    p->initflag = 1;
    p->rangeMin_cod = IS_ASIG_ARG(p->rangeMin);
    p->rangeMax_cod = IS_ASIG_ARG(p->rangeMin);
    p->phs      = 0.0;
    return OK;
}

static int32_t random3(CSOUND *csound, RANDOM3 *p)
{
    MYFLT       x, c3= p->c3, c2= p->c2;
    MYFLT       f0 = p->num0, df0= p->df0;

    if (p->initflag) {
      p->initflag = 0;
      goto next;
    }
    p->phs += p->si;
    if (p->phs >= 1.0) {
      MYFLT     slope, resd1, resd0, f2, f1;
    next:
      p->si = (randGab(csound) * (*p->cpsMax-*p->cpsMin) + *p->cpsMin)*CS_ONEDKR;
      while (p->phs > 1.0)
        p->phs -= 1.0;
      f0     = p->num0 = p->num1;
      f1     = p->num1 = p->num2;
      f2     = p->num2 = randGab(csound);
      df0    = p->df0 = p->df1;
      p->df1 = ( f2  - f0 ) * FL(0.5);
      slope  = f1 - f0;
      resd0  = df0 - slope;
      resd1  = p->df1 - slope;
      c3     = p->c3 = resd0 + resd1;
      c2     = p->c2 = - (resd1 + FL(2.0)* resd0);
    }
    x = (MYFLT) p->phs;
    *p->ar = (((c3 * x + c2) * x + df0) * x + f0) *
      (*p->rangeMax - *p->rangeMin) + *p->rangeMin;
    return OK;
}

static int32_t random3a(CSOUND *csound, RANDOM3 *p)
{
    int32_t         rangeMin_cod = p->rangeMin_cod, rangeMax_cod = p->rangeMax_cod;
    MYFLT       x, c3=p->c3, c2=p->c2;
    MYFLT       f0 = p->num0, df0 = p->df0;
    MYFLT       *ar = p->ar, *rangeMin = p->rangeMin;
    MYFLT       *rangeMax = p->rangeMax;
    MYFLT       cpsMin = *p->cpsMin, cpsMax = *p->cpsMax;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    double      phs = p->phs, si = p->si;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->initflag) {
      p->initflag = 0;
      n = offset;
      goto next;
    }
    for (n=offset; n<nsmps; n++) {
      phs += si;
      if (phs >= 1.0) {
        MYFLT   slope, resd1, resd0, f2, f1;
      next:
        si =  (randGab(csound)  * (cpsMax - cpsMin) + cpsMin)*CS_ONEDSR;
        while (phs > 1.0) phs -= 1.0;
        f0     = p->num0 = p->num1;
        f1     = p->num1 = p->num2;
        f2     = p->num2 = BiRandGab(csound);
        df0    = p->df0 = p->df1;
        p->df1 = ( f2 - f0 ) * FL(0.5);
        slope  = f1 - f0;
        resd0  = df0 - slope;
        resd1  = p->df1 - slope;
        c3     = p->c3 = resd0 + resd1;
        c2     = p->c2 = - (resd1 + FL(2.0)* resd0);
      }
      x = (MYFLT) phs;
      ar[n] = (((c3 * x + c2) * x + df0) * x + f0) *
        (*rangeMax - *rangeMin) + *rangeMin;
      if (rangeMin_cod) rangeMin++;
      if (rangeMax_cod) rangeMax++;
    }
    p->phs = phs;
    p->si  = si;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "wrap.i", S(WRAP),     0,  "i", "iii",  (SUBR)kwrap, NULL,    NULL        },
{ "wrap.k", S(WRAP),     0,  "k", "kkk",  NULL,  (SUBR)kwrap,   NULL        },
{ "wrap.a", S(WRAP),     0,  "a", "akk",  NULL,          (SUBR)wrap  },
{ "mirror", 0xffff                                                          },
{ "mirror.i", S(WRAP),   0,  "i", "iii",  (SUBR)kmirror, NULL,  NULL        },
{ "mirror.k", S(WRAP),   0,  "k", "kkk",  NULL,  (SUBR)kmirror, NULL        },
{ "mirror.a", S(WRAP),   0,  "a", "akk",  NULL,         (SUBR)mirror },
{ "ntrpol.i",S(INTERPOL), 0, "i", "iiiop",(SUBR)interpol                     },
{ "ntrpol.k",S(INTERPOL), 0, "k", "kkkop",(SUBR)nterpol_init, (SUBR)knterpol },
{ "ntrpol.a",S(INTERPOL), 0, "a", "aakop",(SUBR)nterpol_init,(SUBR)anterpol},
{ "fold",    S(FOLD),     0, "a", "ak",   (SUBR)fold_set, (SUBR)fold      },
{ "lineto",   S(LINETO),  0, "k", "kk",   (SUBR)lineto_set,  (SUBR)lineto, NULL },
{ "tlineto",  S(LINETO2), 0, "k", "kkk",  (SUBR)tlineto_set, (SUBR)tlineto, NULL},
{ "vibrato",  S(VIBRATO), TR, "k", "kkkkkkkkio",
                                        (SUBR)vibrato_set, (SUBR)vibrato, NULL   },
{ "vibr",     S(VIBRATO), TR, "k", "kki",  (SUBR)vibr_set, (SUBR)vibr, NULL   },
{ "jitter2",  S(JITTER2), 0, "k", "kkkkkkko", (SUBR)jitter2_set, (SUBR)jitter2 },
{ "jitter",   S(JITTER),  0, "k", "kkk",  (SUBR)jitter_set, (SUBR)jitter, NULL },
{ "jspline",  S(JITTERS), 0, "k", "xkk",
                                (SUBR)jitters_set, (SUBR)jitters, NULL },
{ "jspline.a",  S(JITTERS), 0, "a", "xkk",
    (SUBR)jitters_set, (SUBR)jittersa },
{ "loopseg",  S(LOOPSEG), 0, "k", "kkiz", (SUBR)loopseg_set, (SUBR)loopseg, NULL},
{ "loopxseg", S(LOOPSEG), 0, "k", "kkiz", (SUBR)loopseg_set,(SUBR)loopxseg, NULL},
{ "looptseg", S(LOOPSEG), 0, "k", "kkiz",(SUBR)looptseg_set,(SUBR)looptseg, NULL},
{ "lpshold",  S(LOOPSEG), 0, "k", "kkiz",(SUBR)loopseg_set, (SUBR)lpshold, NULL },
{ "loopsegp", S(LOOPSEGP), 0,"k", "kz",  (SUBR)loopsegp_set,(SUBR)loopsegp, NULL},
{ "lpsholdp", S(LOOPSEGP), 0,"k", "kz",  (SUBR)loopsegp_set,(SUBR)lpsholdp, NULL},
{ "cuserrnd.i", S(CURAND),0,"i",  "iii",  (SUBR)iContinuousUserRand, NULL, NULL },
{ "cuserrnd.k", S(CURAND),0,"k",  "kkk",
                            (SUBR)Cuserrnd_set, (SUBR)kContinuousUserRand, NULL },
{ "cuserrnd.a",S(CURAND),0, "a", "kkk",
                            (SUBR)Cuserrnd_set, (SUBR)aContinuousUserRand },
{ "random.i", S(RANGERAND), 0, "i", "ii",    (SUBR)ikRangeRand, NULL, NULL      },
{ "random.k", S(RANGERAND), 0, "k", "kk",    NULL, (SUBR)ikRangeRand, NULL      },
{ "random.a", S(RANGERAND), 0, "a", "kk",    NULL,  (SUBR)aRangeRand      },
{ "rspline",  S(RANDOM3), 0, "k", "xxkk",
                               (SUBR)random3_set, (SUBR)random3, NULL },
{ "rspline.a",  S(RANDOM3), 0, "a", "xxkk",
                               (SUBR)random3_set, (SUBR)random3a },
{ "randomi",  S(RANDOMI), 0, "a", "kkxoo",
                               (SUBR)randomi_set, (SUBR)randomi },
{ "randomi.k",  S(RANDOMI), 0, "k", "kkkoo",
                               (SUBR)randomi_set, (SUBR)krandomi,NULL },
{ "randomh",  S(RANDOMH), 0, "a", "kkxoo",
                                 (SUBR)randomh_set,(SUBR)randomh },
{ "randomh.k",  S(RANDOMH), 0, "k", "kkkoo",
                                 (SUBR)randomh_set,(SUBR)krandomh,NULL},
{ "urd.i",  S(DURAND),  0, "i", "i", (SUBR)iDiscreteUserRand, NULL, NULL    },
{ "urd.k",  S(DURAND),  0, "k", "k", (SUBR)Cuserrnd_set,(SUBR)kDiscreteUserRand },
{ "urd.a",  S(DURAND),  0, "a", "k",
                              (SUBR)Cuserrnd_set, (SUBR)aDiscreteUserRand },
{ "duserrnd.i", S(DURAND),0, "i", "i",  (SUBR)iDiscreteUserRand, NULL, NULL },
{ "duserrnd.k", S(DURAND),0, "k", "k",
                                (SUBR)Cuserrnd_set,(SUBR)kDiscreteUserRand,NULL },
{ "duserrnd.a", S(DURAND),0, "a", "k",
                                (SUBR)Cuserrnd_set,(SUBR)aDiscreteUserRand },
{ "trigger",  S(TRIG),  0, "k", "kkk",  (SUBR)trig_set, (SUBR)trig,   NULL  },
{ "sum",      S(SUM),   0, "a", "y",    (SUBR)sum_init, (SUBR)sum_               },
{ "product",  S(SUM),   0, "a", "y",    NULL, (SUBR)product           },
{ "resony",  S(RESONY), 0, "a", "akkikooo", (SUBR)rsnsety, (SUBR)resony }
};

int32_t uggab_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}
