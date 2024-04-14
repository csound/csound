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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "stdopcod.h"               /* Flanger by Maldonado, with coding
                                   enhancements by JPff -- July 1998 */
#include <math.h>
#include "flanger.h"

static int32_t flanger_set (CSOUND *csound, FLANGER *p)
{
    /*---------------- delay  -----------------------*/
    p->maxdelay = (uint32)(*p->maxd  * CS_ESR);
    if ((*p->iskip != 0) || (p->aux.auxp==NULL)) {
      csound->AuxAlloc(csound, p->maxdelay * sizeof(MYFLT), &p->aux);
      p->left = 0;
      p->yt1 = FL(0.0);
      p->fmaxd = (MYFLT) p->maxdelay;
    }
    return OK;
}

static int32_t flanger(CSOUND *csound, FLANGER *p)
{
        /*---------------- delay -----------------------*/
    uint32 indx = p->left;
    MYFLT *out = p->ar;  /* assign object data to local variables   */
    MYFLT *in = p->asig;
    MYFLT maxdelay = p->fmaxd, maxdelayM1 = maxdelay-1;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT *freq_del = p->xdel;
    MYFLT feedback =  *p->kfeedback;
    MYFLT fv1;
    int32 v2;
    int32 v1;
    MYFLT yt1= p->yt1;

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    freq_del += offset;
    for (n=offset; n<nsmps; n++) {
                /*---------------- delay -----------------------*/
      buf[indx] = in[n] + (yt1 * feedback);
      fv1 = indx - (*freq_del++ * CS_ESR); /* Make sure inside the buffer*/
      while (fv1 < 0)
        fv1 += maxdelay;
      while (fv1 >= maxdelay) fv1 -= maxdelay; /* Is this necessary? JPff */
      v1 = (int32)fv1;
      v2 = (fv1 < maxdelayM1)? v1+1 : 0; /*Find next sample for interpolation*/
      out[n] = yt1 = buf[v1] + (fv1 - v1) * ( buf[v2] - buf[v1]);
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
    csound->AuxAlloc(csound, p->maxd * sizeof(MYFLT), &p->aux);
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
    MYFLT *out      = p->ar;  /* assign object data to local variables   */
    MYFLT *in       = p->asig;
    MYFLT *buf      = (MYFLT *)p->aux.auxp;
    MYFLT *freq_del = p->xdel; /*(1 / *p->xdel)  * CS_ESR; */
    MYFLT feedback  = *p->kfeedback;
    MYFLT  fv1, fv2, out_delay,bufv1 ;
    uint32_t maxdM1 = p->maxd-1;
    int32   v1;
    /*---------------- filter -----------------------*/
    MYFLT c1, c2, yt1        = p->yt1;
    uint32_t offset          = p->h.insdshead->ksmps_offset;
    uint32_t early           = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps        = CS_KSMPS;

    /*---------------- delay -----------------------*/
    indx                     = p->left;
    /*---------------- filter -----------------------*/
    if (*p->filt_khp != p->prvhp) {
      double b;
      p->prvhp               = *p->filt_khp;
      b                      = 2.0 - cos((double)(p->prvhp * CS_TPIDSR));
      p->c2                  = (MYFLT)(b - sqrt(b * b - 1.0));
      p->c1                  = FL(1.0) - p->c2;
    }
    c1                       = p->c1;
    c2                       = p->c2;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps                 -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->xdelcod) { /* delay changes at audio-rate */
      for (n                 = offset; n<nsmps; n++) {
        /*---------------- delay -----------------------*/
        MYFLT fd             = *freq_del++;
        buf[indx]            = in[n] + (yt1 * feedback);
        if (UNLIKELY(fd<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd                 = FL(1.0)/MAXDELAY;
        fv1                  = indx - (CS_ESR/fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1                = fv1 + (MYFLT)p->maxd;
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
        MYFLT fd             = *freq_del;
        buf[indx]            = in[n] + (yt1 * feedback);
        if (UNLIKELY(fd<FL(1.0)/MAXDELAY)) /* Avoid silly values jpff */
          fd                 = FL(1.0)/MAXDELAY;
        fv1                  = indx - (CS_ESR/fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1                = fv1 + (MYFLT)p->maxd;
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
    csound->AuxAlloc(csound, p->maxd * sizeof(MYFLT), &p->aux1);
    p->left1                 = 0;
        /*---------------- delay2 -----------------------*/
    csound->AuxAlloc(csound, p->maxd * sizeof(MYFLT), &p->aux2);
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
    MYFLT *out               = p->ar;
    MYFLT *in                = p->asig;
    uint32_t offset          = p->h.insdshead->ksmps_offset;
    uint32_t early           = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps        = CS_KSMPS;
    MYFLT out1,out2, old_out = p->old_out;
    uint32_t maxdM1          = p->maxd-1;

    /*---------------- delay1 -----------------------*/
    uint32 indx1;
    MYFLT  *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT  *freq_del1 = p->xdel1; /*(1 / *p->xdel1)  * CS_ESR; */
    MYFLT  feedback1 =  *p->kfeedback1;
    MYFLT  fv1_1, fv2_1, out_delay1 ;
    int32  v1_1;
        /*---------------- filter1 -----------------------*/
    MYFLT c1_1, c2_1, yt1_1;
        /*---------------- delay2 -----------------------*/
    uint32 indx2;
    MYFLT  *buf2 = (MYFLT *)p->aux2.auxp;
    MYFLT  *freq_del2 = p->xdel2; /*(1 / *p->xdel2)  * CS_ESR;*/
    MYFLT  feedback2 =  *p->kfeedback2;
    MYFLT  fv1_2, fv2_2, out_delay2 ;
    int32  v1_2;
        /*---------------- filter2 -----------------------*/
    MYFLT c1_2, c2_2, yt1_2;
        /*-----------------------------------------------*/

    indx1 = p->left1;
    indx2 = p->left2;
    if (*p->filt_khp1 != p->prvhp1) {
      double b;
      p->prvhp1 = *p->filt_khp1;
      b = 2.0 - cos((double)(p->prvhp1 * CS_TPIDSR));
      p->c2_1 = (MYFLT)(b - sqrt((b * b) - 1.0));
      p->c1_1 = FL(1.0) - p->c2_1;
    }
    if (*p->filt_khp2 != p->prvhp2) {
      double b;
      p->prvhp2 = *p->filt_khp2;
      b = 2.0 - cos((double)(p->prvhp2 * CS_TPIDSR));
      p->c2_2 = (MYFLT)(b - sqrt((double)(b * b) - 1.0));
      p->c1_2 = FL(1.0) - p->c2_2;
    }
    c1_1= p->c1_1;
    c2_1= p->c2_1;
    c1_2= p->c1_2;
    c2_2= p->c2_2;
    yt1_1= p->yt1_1;
    yt1_2= p->yt1_2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->xdel1cod) { /* delays change at audio-rate */
      for (n=offset;n<nsmps;n++) {
        MYFLT fd1 = *freq_del1++;
        MYFLT fd2 = *freq_del2++;
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
        MYFLT fd1 = *freq_del1;
        MYFLT fd2 = *freq_del2;
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

