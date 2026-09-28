/*
    flanger.c:

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "stdopcod.h"               /* Flanger by Maldonado, with coding
                                   enhancements by JPff -- July 1998 */
#include <math.h>
#include "flanger.h"

static int32_t flanger_set (CSOUND *csound, FLANGER *p)
{
    cs_float maxDelaySeconds = FABS(*p->maxd);
    cs_double maxDelaySamples = (cs_double)maxDelaySeconds * CS_ESR;
    uint32 maxDelay, bufferSize;

    if (UNLIKELY(!(maxDelaySamples >= 0.0) ||
                 maxDelaySamples > (cs_double)(UINT32_MAX - 2U)))
      return csound->InitError(csound, "%s",
                               Str("flanger: invalid maximum delay"));
    maxDelay = (uint32)maxDelaySamples;
    if ((cs_double)maxDelay < maxDelaySamples)
      maxDelay++;
    bufferSize = maxDelay + 1U;
    if (UNLIKELY((size_t)bufferSize > SIZE_MAX / sizeof(cs_float)))
      return csound->InitError(csound, "%s",
                               Str("flanger: delay buffer too large"));
    if (*p->iskip == 0 || p->aux.auxp == NULL ||
        p->maxdelay != bufferSize) {
      csound->AuxAlloc(csound, (size_t)bufferSize * sizeof(cs_float), &p->aux);
      p->left = 0;
      p->yt1 = FL(0.0);
    }
    p->maxdelay = bufferSize;
    p->maxDelaySeconds = maxDelaySeconds;
    return OK;
}

static int32_t flanger(CSOUND *csound, FLANGER *p)
{
        /*---------------- delay -----------------------*/
    uint32 indx = p->left;
    cs_float *out = p->ar;  /* assign object data to local variables   */
    cs_float *in = p->asig;
    uint32 maxdelay = p->maxdelay;
    cs_float maxDelaySeconds = p->maxDelaySeconds;
    cs_float *buf = (cs_float *)p->aux.auxp;
    cs_float *freq_del = p->xdel;
    cs_float feedback =  *p->kfeedback;
    cs_double fv1;
    uint32 v2;
    uint32 v1;
    cs_float yt1= p->yt1;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    freq_del += offset;
    for (n=offset; n<nsmps; n++) {
      cs_float delay = *freq_del++;

                /*---------------- delay -----------------------*/
      if (UNLIKELY(!(delay >= FL(0.0) && delay <= maxDelaySeconds)))
        return csound->PerfError(csound, &(p->h), "%s",
                                 Str("flanger: delay is outside imaxd"));
      buf[indx] = in[n] + (yt1 * feedback);
      fv1 = (cs_double)indx - (cs_double)delay * CS_ESR;
      if (fv1 < 0.0)
        fv1 += (cs_double)maxdelay;
      if (fv1 >= (cs_double)maxdelay)
        fv1 -= (cs_double)maxdelay;
      v1 = (uint32)fv1;
      v2 = v1 + 1U;
      if (v2 == maxdelay)
        v2 = 0;
      out[n] = yt1 =
        buf[v1] + (cs_float)(fv1 - v1) * (buf[v2] - buf[v1]);
      if (UNLIKELY(++indx == maxdelay))
        indx = 0;                      /* Advance current pointer */
    }
    p->left = indx;
    p->yt1 = yt1;
    return OK;
}

#define MAXDELAY        0.2 /* 5 Hz */

static int32_t wguide1set (CSOUND *csound, WGUIDE1 *p)
{
        /*---------------- delay -----------------------*/
    p->maxd = (uint32) (MAXDELAY * CS_ESR);
    csound->AuxAlloc(csound, p->maxd * sizeof(cs_float), &p->aux);
    p->left = 0;
        /*---------------- filter -----------------------*/
    p->c1 = p->prvhp = FL(0.0);
    p->c2 = FL(1.0);
    p->yt1 = FL(0.0);
    p->xdelcod = IS_ASIG_ARG(p->xdel) ? 1 : 0;
    return OK;
}

static int32_t wguide1(CSOUND *csound, WGUIDE1 *p)
{
        /*---------------- delay -----------------------*/
    uint32  indx;
    cs_float *out      = p->ar;  /* assign object data to local variables   */
    cs_float *in       = p->asig;
    cs_float *buf      = (cs_float *)p->aux.auxp;
    cs_float *freq_del = p->xdel; /*(1 / *p->xdel)  * CS_ESR; */
    cs_float feedback  = *p->kfeedback;
    cs_float  fv1, fv2, out_delay,bufv1 ;
    uint32_t maxdM1 = p->maxd-1;
    int32   v1;
    /*---------------- filter -----------------------*/
    cs_float c1, c2, yt1        = p->yt1;
    uint32_t offset          = p->h.insdshead->ksmps_offset;
    uint32_t early           = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps        = CS_KSMPS;

    /*---------------- delay -----------------------*/
    indx                     = p->left;
    /*---------------- filter -----------------------*/
    if (*p->filt_khp != p->prvhp) {
      cs_double b;
      p->prvhp               = *p->filt_khp;
      b                      = 2.0 - cos((cs_double)(p->prvhp * CS_TPIDSR));
      p->c2                  = (cs_float)(b - sqrt(b * b - 1.0));
      p->c1                  = FL(1.0) - p->c2;
    }
    c1                       = p->c1;
    c2                       = p->c2;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps                 -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    if (p->xdelcod) { /* delay changes at audio-rate */
      freq_del += offset;
      for (n                 = offset; n<nsmps; n++) {
        /*---------------- delay -----------------------*/
        cs_float fd             = *freq_del++;
        buf[indx]            = in[n] + (yt1 * feedback);
        if (UNLIKELY(fd<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd                 = FL(1.0)/MAXDELAY;
        fv1                  = indx - (CS_ESR/fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1                = fv1 + (cs_float)p->maxd;
        }
        fv2                  = (fv1 < maxdM1) ?
                  fv1 + 1 : 0;  /* Find next smpl for interpolation */
        bufv1                = buf[v1=(int32)fv1];
        out_delay            = bufv1 + (fv1 - v1) * ( buf[(int32)fv2] - bufv1);
        if (UNLIKELY(++indx == p->maxd)) indx = 0; /* Advance current pointer */
        /*---------------- filter -----------------------*/
        out[n]               = yt1 = c1 * out_delay + c2 * yt1;
      }
    }
    else {
      for (n                 = offset; n<nsmps; n++) {
        /*---------------- delay -----------------------*/
        cs_float fd             = *freq_del;
        buf[indx]            = in[n] + (yt1 * feedback);
        if (UNLIKELY(fd<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd                 = FL(1.0)/MAXDELAY;
        fv1                  = indx - (CS_ESR/fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1                = fv1 + (cs_float)p->maxd;
        }
        fv2                  = (fv1 < maxdM1) ?
              fv1 + 1 : 0;  /* Find next smpl for interpolation */
        bufv1                = buf[v1=(int32)fv1];
        out_delay            = bufv1 + (fv1 - v1) * ( buf[(int32)fv2] - bufv1);
        if (UNLIKELY(++indx == p->maxd)) indx = 0;     /* Advance current pointer */
        /*---------------- filter -----------------------*/
        out[n]               = yt1 = c1 * out_delay + c2 * yt1;
      }
    }
    p->left                  = indx;
    p->yt1                   = yt1;
    return OK;
}

static int32_t wguide2set (CSOUND *csound, WGUIDE2 *p)
{
        /*---------------- delay1 -----------------------*/
    p->maxd                  = (uint32) (MAXDELAY * CS_ESR);
    csound->AuxAlloc(csound, p->maxd * sizeof(cs_float), &p->aux1);
    p->left1                 = 0;
        /*---------------- delay2 -----------------------*/
    csound->AuxAlloc(csound, p->maxd * sizeof(cs_float), &p->aux2);
    p->left2                 = 0;
        /*---------------- filter1 -----------------------*/
    p->c1_1                  = p->prvhp1 = FL(0.0);
    p->c2_1                  = FL(1.0);
    p->yt1_1                 = FL(0.0);
        /*---------------- filter2 -----------------------*/
    p->c1_2                  = p->prvhp2 = FL(0.0);
    p->c2_2                  = FL(1.0);
    p->yt1_2                 = FL(0.0);

    p->old_out               = FL(0.0);
    p->xdel1cod              = IS_ASIG_ARG(p->xdel1) ? 1 : 0;
    p->xdel2cod              = IS_ASIG_ARG(p->xdel2) ? 1 : 0;

    if (UNLIKELY(p->xdel1cod != p->xdel2cod))
      return csound->InitError(csound, "%s", Str(
                    "wguide2 xfreq1 and xfreq2 arguments must"
                    " be both a-rate or k and i-rate"));
    return OK;
}

static int32_t wguide2(CSOUND *csound, WGUIDE2 *p)
{
    cs_float *out               = p->ar;
    cs_float *in                = p->asig;
    uint32_t offset          = p->h.insdshead->ksmps_offset;
    uint32_t early           = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps        = CS_KSMPS;
    cs_float out1,out2, old_out = p->old_out;
    uint32_t maxdM1          = p->maxd-1;

    /*---------------- delay1 -----------------------*/
    uint32 indx1;
    cs_float  *buf1 = (cs_float *)p->aux1.auxp;
    cs_float  *freq_del1 = p->xdel1; /*(1 / *p->xdel1)  * CS_ESR; */
    cs_float  feedback1 =  *p->kfeedback1;
    cs_float  fv1_1, fv2_1, out_delay1 ;
    int32  v1_1;
        /*---------------- filter1 -----------------------*/
    cs_float c1_1, c2_1, yt1_1;
        /*---------------- delay2 -----------------------*/
    uint32 indx2;
    cs_float  *buf2 = (cs_float *)p->aux2.auxp;
    cs_float  *freq_del2 = p->xdel2; /*(1 / *p->xdel2)  * CS_ESR;*/
    cs_float  feedback2 =  *p->kfeedback2;
    cs_float  fv1_2, fv2_2, out_delay2 ;
    int32  v1_2;
        /*---------------- filter2 -----------------------*/
    cs_float c1_2, c2_2, yt1_2;
        /*-----------------------------------------------*/

    indx1 = p->left1;
    indx2 = p->left2;
    if (*p->filt_khp1 != p->prvhp1) {
      cs_double b;
      p->prvhp1 = *p->filt_khp1;
      b = 2.0 - cos((cs_double)(p->prvhp1 * CS_TPIDSR));
      p->c2_1 = (cs_float)(b - sqrt((b * b) - 1.0));
      p->c1_1 = FL(1.0) - p->c2_1;
    }
    if (*p->filt_khp2 != p->prvhp2) {
      cs_double b;
      p->prvhp2 = *p->filt_khp2;
      b = 2.0 - cos((cs_double)(p->prvhp2 * CS_TPIDSR));
      p->c2_2 = (cs_float)(b - sqrt((cs_double)(b * b) - 1.0));
      p->c1_2 = FL(1.0) - p->c2_2;
    }
    c1_1= p->c1_1;
    c2_1= p->c2_1;
    c1_2= p->c1_2;
    c2_2= p->c2_2;
    yt1_1= p->yt1_1;
    yt1_2= p->yt1_2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    if (p->xdel1cod) { /* delays change at audio-rate */
      freq_del1 += offset;
      freq_del2 += offset;
      for (n=offset;n<nsmps;n++) {
        cs_float fd1 = *freq_del1++;
        cs_float fd2 = *freq_del2++;
        buf1[indx1] = buf2[indx2] =
          in[n] + old_out * (feedback1 + feedback2);
        if (UNLIKELY(fd1<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd1 = FL(1.0)/MAXDELAY;
        if (UNLIKELY(fd2<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd2 = FL(1.0)/MAXDELAY;
        fv1_1 = indx1 - (CS_ESR / fd1); /* Make sure inside the buffer */
        fv1_2 = indx2 - (CS_ESR / fd2); /* Make sure inside the buffer */
        while (fv1_1 < 0)    fv1_1 += p->maxd;
        while (fv1_2 < 0)    fv1_2 += p->maxd;
        fv2_1 = (fv1_1<maxdM1)?
          fv1_1+1:0; /*Find next sample for interpolation */
        fv2_2 = (fv1_2<maxdM1)?
          fv1_2+1:0; /*Find next sample for interpolation */
        v1_1 = (int32)fv1_1;
        v1_2 = (int32)fv1_2;
        out_delay1 = buf1[v1_1] + (fv1_1-v1_1)*(buf1[(int32)fv2_1]-buf1[v1_1]);
        out_delay2 = buf2[v1_2] + (fv1_2-v1_2)*(buf2[(int32)fv2_2]-buf2[v1_2]);
        if (UNLIKELY(++indx1 == p->maxd)) indx1 = 0; /* Advance current pointer */
        if (UNLIKELY(++indx2 == p->maxd)) indx2 = 0; /* Advance current pointer */
        out1 = yt1_1 = c1_1 * out_delay1 + c2_1 * yt1_1;
        out2 = yt1_2 = c1_2 * out_delay2 + c2_2 * yt1_2;
        out[n] = old_out = out1 + out2;
      }
    }
    else {
      for (n=offset; n<nsmps;n++) {
        cs_float fd1 = *freq_del1;
        cs_float fd2 = *freq_del2;
        buf1[indx1] = buf2[indx2] =
          in[n] + old_out * (feedback1 + feedback2);
        if (UNLIKELY(fd1<FL(1.0)/MAXDELAY))/* Avoid silly values jpff */
          fd1 = FL(1.0)/MAXDELAY;
        if (UNLIKELY(fd2<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd2 = FL(1.0)/MAXDELAY;
        fv1_1 = indx1 - (CS_ESR / fd1); /* Make sure inside the buffer */
        fv1_2 = indx2 - (CS_ESR / fd2); /* Make sure inside the buffer */
        while (fv1_1 < 0)    fv1_1 += p->maxd;
        while (fv1_2 < 0)    fv1_2 += p->maxd;
        fv2_1 = (fv1_1<maxdM1)?
          fv1_1+1:0; /*Find next sample for interpolation */
        fv2_2 = (fv1_2<maxdM1)?
          fv1_2+1:0; /*Find next sample for interpolation */
        v1_1 = (int32)fv1_1;
        v1_2 = (int32)fv1_2;
        out_delay1 = buf1[v1_1] + (fv1_1 - v1_1)*(buf1[(int32)fv2_1]-buf1[v1_1]);
        out_delay2 = buf2[v1_2] + (fv1_2 - v1_2)*(buf2[(int32)fv2_2]-buf2[v1_2]);
        if (UNLIKELY(++indx1 == p->maxd)) indx1 = 0; /* Advance current pointer */
        if (UNLIKELY(++indx2 == p->maxd)) indx2 = 0; /* Advance current pointer */
        out1 = yt1_1 = c1_1 * out_delay1 + c2_1 * yt1_1;
        out2 = yt1_2 = c1_2 * out_delay2 + c2_2 * yt1_2;
        out[n] = old_out = out1 + out2;
      }
    }
    p->left1 = indx1;
    p->left2 = indx2;
    p->old_out = old_out;
    p->yt1_1 = yt1_1;
    p->yt1_2 = yt1_2;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "flanger", S(FLANGER), 0,  "a", "aakvo", (SUBR)flanger_set, (SUBR)flanger },
{ "wguide1", S(WGUIDE1), 0, "a", "axkk",(SUBR) wguide1set, (SUBR)wguide1  },
{ "wguide2", S(WGUIDE2), 0,  "a", "axxkkkk",(SUBR)wguide2set, (SUBR)wguide2 }
};

int32_t flanger_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
