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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"               /* Flanger by Maldonado, with coding
                                   enhancements by JPff -- July 1998 */
#include <math.h>
#include "flanger.h"

int flanger_set (ENVIRON *csound, FLANGER *p)
{
        /*---------------- delay  -----------------------*/
    p->maxdelay = (unsigned long)(*p->maxd  * esr);
    auxalloc(p->maxdelay * sizeof(MYFLT), &p->aux);
    p->left = 0;
    p->yt1 = FL(0.0);
    p->fmaxd = (MYFLT) p->maxdelay;
    return OK;
}

int flanger(ENVIRON *csound, FLANGER *p)
{
        /*---------------- delay -----------------------*/
    unsigned long  indx = p->left;
    MYFLT *out = p->ar;  /* assign object data to local variables   */
    MYFLT *in = p->asig;
    MYFLT maxdelay = p->fmaxd, maxdelayM1 = maxdelay-1;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT *freq_del = p->xdel;
    MYFLT feedback =  *p->kfeedback;
    MYFLT fv1;
    long  v2;
    long  v1;
    MYFLT *yt1= &(p->yt1);

    int nsmps = ksmps;

    do {
                /*---------------- delay -----------------------*/
      buf[indx] = *in++ + (*yt1 * feedback);
      fv1 = indx - (*freq_del++ * esr); /* Make sure inside the buffer     */
      while (fv1 < 0)
        fv1 += maxdelay;
      v1 = (long)fv1;
      v2 = (fv1 < maxdelayM1)? v1+1 : 0; /*Find next sample for interpolation*/
      *out++ = *yt1 = buf[v1] + (fv1 - v1) * ( buf[v2] - buf[v1]);
      if (++indx == maxdelay)
        indx = 0;                      /* Advance current pointer */
    } while (--nsmps);
    p->left = indx;
    return OK;
}

#define MAXDELAY        .2 /* 5 Hz */
static unsigned long maxd; /* max delay samples */
static unsigned long maxdM1; /* max delay samples - 1 */

int wguide1set (ENVIRON *csound, WGUIDE1 *p)
{
        /*---------------- delay -----------------------*/
    auxalloc((maxd=(unsigned long)(MAXDELAY * esr)) * sizeof(MYFLT), &p->aux);
    maxdM1 = maxd-1;
    p->left = 0;
        /*---------------- filter -----------------------*/
    p->c1 = p->prvhp = FL(0.0);
    p->c2 = FL(1.0);
    p->yt1 = FL(0.0);
    p->xdelcod = (XINARG2) ? 1 : 0;
    return OK;
}

int wguide1(ENVIRON *csound, WGUIDE1 *p)
{
        /*---------------- delay -----------------------*/
    unsigned long  indx;
    MYFLT *out = p->ar;  /* assign object data to local variables   */
    MYFLT *in = p->asig;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT *freq_del =  p->xdel; /*(1 / *p->xdel)  * esr; */
    MYFLT feedback =  *p->kfeedback;
    MYFLT  fv1, fv2, out_delay,bufv1 ;
    long   v1;
    /*---------------- filter -----------------------*/
    MYFLT c1, c2, *yt1;
    int nsmps = ksmps;

    /*---------------- delay -----------------------*/
    indx = p->left;
    /*---------------- filter -----------------------*/
    if (*p->filt_khp != p->prvhp) {
      MYFLT b;
      p->prvhp = *p->filt_khp;
      b = FL(2.0) - (MYFLT)cos((double)(*p->filt_khp * tpidsr));
      p->c2 = b - (MYFLT)sqrt((double)(b * b - 1.0));
      p->c1 = FL(1.0) - p->c2;
    }
    c1= p->c1;
    c2= p->c2;
    yt1= &(p->yt1);
    if (p->xdelcod) { /* delay changes at audio-rate */
      do {
        /*---------------- delay -----------------------*/
        MYFLT fd = *freq_del++;
        buf[indx] = *in++ + (*yt1 * feedback);
        if (fd<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd = FL(1.0)/MAXDELAY;
        fv1 = indx - (esr / fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1 = fv1 + (MYFLT)maxd;
        }
        fv2 = (fv1 < maxdM1) ? fv1 + 1 : 0;  /* Find next smpl for interpolation */
        bufv1 = buf[v1=(long)fv1];
        out_delay = bufv1 + (fv1 - v1) * ( buf[(long)fv2] - bufv1);
        if (++indx == maxd) indx = 0;        /* Advance current pointer */
        /*---------------- filter -----------------------*/
        *out++ = *yt1 = c1 * out_delay + c2 * *yt1;
      } while (--nsmps);
    } else {
      do {
        /*---------------- delay -----------------------*/
        MYFLT fd = *freq_del;
        buf[indx] = *in++ + (*yt1 * feedback);
        if (fd<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd = FL(1.0)/MAXDELAY;
        fv1 = indx - (esr / fd); /* Make sure inside the buffer */
        while (fv1 < 0) {
          fv1 = fv1 + (MYFLT)maxd;
        }
        fv2 = (fv1 < maxdM1) ? fv1 + 1 : 0;  /* Find next smpl for interpolation */
        bufv1 = buf[v1=(long)fv1];
        out_delay = bufv1 + (fv1 - v1) * ( buf[(long)fv2] - bufv1);
        if (++indx == maxd) indx = 0;        /* Advance current pointer */
        /*---------------- filter -----------------------*/
        *out++ = *yt1 = c1 * out_delay + c2 * *yt1;
      } while (--nsmps);
    }
    p->left = indx;
    return OK;
}

int wguide2set (ENVIRON *csound, WGUIDE2 *p)
{
        /*---------------- delay1 -----------------------*/
    auxalloc((maxd = (unsigned long)(MAXDELAY * esr)) * sizeof(MYFLT), &p->aux1);
    maxdM1 = maxd-1;
    p->left1 = 0;
        /*---------------- delay2 -----------------------*/
    auxalloc(maxd * sizeof(MYFLT), &p->aux2);
    p->left2 = 0;
        /*---------------- filter1 -----------------------*/
    p->c1_1 = p->prvhp1 = FL(0.0);
    p->c2_1 = FL(1.0);
    p->yt1_1 = FL(0.0);
        /*---------------- filter2 -----------------------*/
    p->c1_2 = p->prvhp2 = FL(0.0);
    p->c2_2 = FL(1.0);
    p->yt1_2 = FL(0.0);

    p->old_out=FL(0.0);
    p->xdel1cod = (XINARG1) ? 1 : 0;
    p->xdel2cod = (XINARG2) ? 1 : 0;
    if (p->xdel2cod != p->xdel2cod)
      return initerror(Str(
                    "wguide2 xfreq1 and xfreq2 arguments must"
                    " be both a-rate or k and i-rate"));
    return OK;
}

int wguide2(ENVIRON *csound, WGUIDE2 *p)
{
    MYFLT *out = p->ar;
    MYFLT *in = p->asig;
    int nsmps = ksmps;
    MYFLT out1,out2, *old_out = &(p->old_out);

    /*---------------- delay1 -----------------------*/
    unsigned long  indx1;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT *freq_del1 = p->xdel1; /*(1 / *p->xdel1)  * esr; */
    MYFLT feedback1 =  *p->kfeedback1;
    MYFLT  fv1_1, fv2_1, out_delay1 ;
    long   v1_1;
        /*---------------- filter1 -----------------------*/
    MYFLT c1_1, c2_1, *yt1_1;
        /*---------------- delay2 -----------------------*/
    unsigned long  indx2;
    MYFLT *buf2 = (MYFLT *)p->aux2.auxp;
    MYFLT *freq_del2 = p->xdel2; /*(1 / *p->xdel2)  * esr;*/
    MYFLT feedback2 =  *p->kfeedback2;
    MYFLT  fv1_2, fv2_2, out_delay2 ;
    long   v1_2;
        /*---------------- filter2 -----------------------*/
    MYFLT c1_2, c2_2, *yt1_2;
        /*-----------------------------------------------*/

    indx1 = p->left1;
    indx2 = p->left2;
    if (*p->filt_khp1 != p->prvhp1) {
      MYFLT b;
      p->prvhp1 = *p->filt_khp1;
      b = FL(2.0) - (MYFLT)cos((double)(*p->filt_khp1 * tpidsr));
      p->c2_1 = b - (MYFLT)sqrt((double)(b * b) - 1.0);
      p->c1_1 = FL(1.0) - p->c2_1;
    }
    if (*p->filt_khp2 != p->prvhp2) {
      MYFLT b;
      p->prvhp2 = *p->filt_khp2;
      b = FL(2.0) - (MYFLT)cos((double)(*p->filt_khp2 * tpidsr));
      p->c2_2 = b - (MYFLT)sqrt((double)(b * b) - 1.0);
      p->c1_2 = FL(1.0) - p->c2_2;
    }
    c1_1= p->c1_1;
    c2_1= p->c2_1;
    c1_2= p->c1_2;
    c2_2= p->c2_2;
    yt1_1= &(p->yt1_1);
    yt1_2= &(p->yt1_2);

    if (p->xdel1cod) { /* delays change at audio-rate */
      do {
        MYFLT fd1 = *freq_del1++;
        MYFLT fd2 = *freq_del2++;
        buf1[indx1] = buf2[indx2] =
          *in++ + (*old_out * feedback1) + (*old_out * feedback2);
        if (fd1<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd1 = FL(1.0)/MAXDELAY;
        if (fd2<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd2 = FL(1.0)/MAXDELAY;
        fv1_1 = indx1 - (esr / fd1); /*Make sure inside the buffer */
        fv1_2 = indx2 - (esr / fd2); /* Make sure inside the buffer  */
        while (fv1_1 < 0)    fv1_1 += maxd;
        while (fv1_2 < 0)    fv1_2 += maxd;
        fv2_1 = (fv1_1<maxdM1)?fv1_1+1:0; /*Find next sample for interpolation */
        fv2_2 = (fv1_2<maxdM1)?fv1_2+1:0; /*Find next sample for interpolation */
        v1_1 = (long)fv1_1;
        v1_2 = (long)fv1_2;
        out_delay1 = buf1[v1_1] + (fv1_1-v1_1)*(buf1[(long)fv2_1]-buf1[v1_1]);
        out_delay2 = buf2[v1_2] + (fv1_2-v1_2)*(buf2[(long)fv2_2]-buf2[v1_2]);
        if (++indx1 == maxd) indx1 = 0;        /* Advance current pointer */
        if (++indx2 == maxd) indx2 = 0;        /* Advance current pointer */
        out1 = *yt1_1 = c1_1 * out_delay1 + c2_1 * *yt1_1;
        out2 = *yt1_2 = c1_2 * out_delay2 + c2_2 * *yt1_2;
        *out++ = *old_out = out1 + out2;
      } while (--nsmps);
    }
    else {
      do {
        MYFLT fd1 = *freq_del1;
        MYFLT fd2 = *freq_del2;
        buf1[indx1] = buf2[indx2] =
          *in++ + (*old_out * feedback1) + (*old_out * feedback2);
        if (fd1<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd1 = FL(1.0)/MAXDELAY;
        if (fd2<FL(1.0)/MAXDELAY) /* Avoid silly values jpff */
          fd2 = FL(1.0)/MAXDELAY;
        fv1_1 = indx1 - (esr / fd1); /*Make sure inside the buffer     */
        fv1_2 = indx2 - (esr / fd2); /*Make sure inside the buffer     */
        while (fv1_1 < 0)    fv1_1 += maxd;
        while (fv1_2 < 0)    fv1_2 += maxd;
        fv2_1 = (fv1_1<maxdM1)?fv1_1+1:0; /*Find next sample for interpolation */
        fv2_2 = (fv1_2<maxdM1)?fv1_2+1:0; /*Find next sample for interpolation */
        v1_1 = (long)fv1_1;
        v1_2 = (long)fv1_2;
        out_delay1 = buf1[v1_1] + (fv1_1 - v1_1)*(buf1[(long)fv2_1]-buf1[v1_1]);
        out_delay2 = buf2[v1_2] + (fv1_2 - v1_2)*(buf2[(long)fv2_2]-buf2[v1_2]);
        if (++indx1 == maxd) indx1 = 0;        /* Advance current pointer */
        if (++indx2 == maxd) indx2 = 0;        /* Advance current pointer */
        out1 = *yt1_1 = c1_1 * out_delay1 + c2_1 * *yt1_1;
        out2 = *yt1_2 = c1_2 * out_delay2 + c2_2 * *yt1_2;
        *out++ = *old_out = out1 + out2;
      } while (--nsmps);
    }
    p->left1 = indx1;
    p->left2 = indx2;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "flanger", S(FLANGER), 5, "a", "aakv", (SUBR)flanger_set, NULL, (SUBR)flanger },
{ "wguide1", S(WGUIDE1), 5, "a", "axkk",(SUBR) wguide1set, NULL, (SUBR)wguide1  },
{ "wguide2", S(WGUIDE2), 5, "a", "axxkkkk",(SUBR)wguide2set, NULL, (SUBR)wguide2 },
};

LINKAGE


