/*
    space.c:

    Copyright (C) 1998 Richard Karpen

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


/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/

#include "csdl.h"
#include "space.h"
#include <math.h>
static SPACE    *spaceaddr;

#define RESOLUTION 100

int spaceset(ENVIRON *csound, SPACE *p)
{

   FUNC *ftp=NULL;

   if (*p->ifn > 0) {
     if ((ftp = ftfind(p->h.insdshead->csound, p->ifn)) == NULL)
       return NOTOK;
     p->ftp = ftp;
   }

   if (p->auxch.auxp == NULL) {
     MYFLT *fltp;
     auxalloc((long)(ksmps * 4)  * sizeof(MYFLT), &p->auxch);
     fltp = (MYFLT *) p->auxch.auxp;
     p->rrev1 = fltp;   fltp += ksmps;
     p->rrev2 = fltp;   fltp += ksmps;
     p->rrev3 = fltp;   fltp += ksmps;
     p->rrev4 = fltp;   fltp += ksmps;
   }

   spaceaddr = p;
   return OK;
}

int space(ENVIRON *csound, SPACE *p)
{
    MYFLT       *r1, *r2, *r3, *r4, *sigp, ch1, ch2, ch3, ch4;
    MYFLT distance=FL(1.0), distr, distrsq, direct;
    MYFLT *rrev1, *rrev2, *rrev3, *rrev4;
    MYFLT torev, localrev, globalrev;
    MYFLT       xndx, yndx;
    MYFLT half_pi = FL(0.5)*PI_F;
    MYFLT sqrt2 = (MYFLT)sqrt(2.0);
    MYFLT fabxndx, fabyndx;
    int nsmps = ksmps;
    FUNC        *ftp;
    long        indx, length, halflen;
    MYFLT       v1, v2, fract, ndx;

    if (*p->ifn > 0) { /* get xy vals from function table */
     if ((ftp = p->ftp) == NULL) {
       return perferror(Str(X_1213,"space: not initialised"));
      }

      ndx = *p->time * RESOLUTION; /* when data is res. frames/second */
      length = ftp->flen;
      halflen = (long)(length * FL(0.5));
      indx = (long) floor(ndx);
      fract = ndx - indx;

      if (ndx > (halflen-1)) {
        indx  = halflen - 1;
        fract = FL(0.0);
      }
      else if (ndx < 0) {
        indx  = 0L;
        fract = FL(0.0);
      }

      v1 = *(ftp->ftable + (indx*2));
      v2 = *(ftp->ftable + (indx*2) + 2);
      xndx = v1 + (v2 - v1) * fract;

      v1 = *(ftp->ftable + (indx*2) + 1);
      v2 = *(ftp->ftable + (indx*2) + 3);
      yndx = v1 + (v2 - v1) * fract;
    }
    else { /* get xy vals from input arguments */
      xndx = *p->kx;
      yndx = *p->ky;
    }

    distance = (MYFLT)sqrt((xndx*xndx) + (yndx*yndx));
    fabxndx = (MYFLT)fabs(xndx);
    fabyndx = (MYFLT)fabs(yndx);
    if ((fabxndx > FL(1.0)) || (fabyndx > FL(1.0))) {
      if (fabxndx > fabyndx) {
        xndx = xndx/fabxndx;
        yndx = yndx/fabxndx;
        /* distance = fabxndx; */
      }
      else {
        xndx = xndx/fabyndx;
        yndx = yndx/fabyndx;
        /* distance = fabyndx; */
      }
    }

    if (distance < FL(1.0)) distance = FL(1.0);

    distr=(FL(1.0) / distance);
    distrsq = FL(1.0)/(MYFLT)sqrt(distance);

    xndx = (xndx+1)*FL(0.5);
    yndx = (yndx+1)*FL(0.5);

    ch2 = (MYFLT)(sin(half_pi * xndx) * sin(half_pi * yndx) * sqrt2);
    ch4 = (MYFLT)(sin(half_pi * xndx) * sin(half_pi * (1-yndx)) * sqrt2);
    ch1 = (MYFLT)(sin(half_pi * (1 - xndx)) * sin(half_pi * yndx) * sqrt2);
    ch3 = (MYFLT)(sin(half_pi * (1 - xndx)) * sin(half_pi * (1 - yndx)) * sqrt2);

    r1 = p->r1;
    r2 = p->r2;
    r3 = p->r3;
    r4 = p->r4;
    rrev1 = p->rrev1;
    rrev2 = p->rrev2;
    rrev3 = p->rrev3;
    rrev4 = p->rrev4;
    sigp = p->asig;
    do {
      direct = *sigp * distr;
      torev = *sigp * distrsq * *p->reverbamount;
      globalrev = torev * distr;
      localrev = torev * (1 - distr);
      *r1++ = direct * ch1;
      *r2++ = direct * ch2;
      *r3++ = direct * ch3;
      *r4++ = direct * ch4;
      *rrev1++ = (localrev * ch1) + globalrev;
      *rrev2++ = (localrev * ch2) + globalrev;
      *rrev3++ = (localrev * ch3) + globalrev;
      *rrev4++ = (localrev * ch4) + globalrev;
      sigp++;
    }
    while (--nsmps);
    return OK;
}


int spsendset(ENVIRON *csound, SPSEND *p)
{
    p->space=spaceaddr;
    return OK;
}

int spsend(ENVIRON *csound, SPSEND *p)
{
    MYFLT       *r1, *r2, *r3, *r4, *rrev1, *rrev2, *rrev3, *rrev4;
    SPACE *q = p->space;
    int nsmps = ksmps;

    r1 = p->r1;
    r2 = p->r2;
    r3 = p->r3;
    r4 = p->r4;
    rrev1 = q->rrev1;
    rrev2 = q->rrev2;
    rrev3 = q->rrev3;
    rrev4 = q->rrev4;

    do {
      *r1++ = *rrev1++;
      *r2++ = *rrev2++;
      *r3++ = *rrev3++;
      *r4++ = *rrev4++;

    }
    while (--nsmps);
    return OK;
}


int spdistset(ENVIRON *csound, SPDIST *p)
{
   FUNC *ftp;

   if (*p->ifn > 0) {
     if ((ftp = ftfind(p->h.insdshead->csound, p->ifn)) == NULL)
       return NOTOK;
     p->ftp = ftp;
   }
   return OK;
}

int spdist(ENVIRON *csound, SPDIST *p)
{
    MYFLT *r;
    MYFLT distance, xndx, yndx;
    FUNC *ftp;
    long        indx, length, halflen;
    MYFLT       v1, v2, fract, ndx;

    r = p->r;

    if (*p->ifn > 0) {
      if ((ftp = p->ftp)==NULL) {
        return perferror(Str(X_1214,"spdist: not initialised"));
      }

      ndx = *p->time * RESOLUTION; /* when data is 10 frames/second */
      length = ftp->flen;
      halflen = (long)(length * FL(0.5));
      indx = (long) floor(ndx);
      fract = ndx - indx;

      if (ndx > (halflen-1)) {
        indx  = halflen - 1;
        fract = FL(0.0);
      }
      else if (ndx < 0) {
        indx  = 0L;
        fract = FL(0.0);
      }

      v1 = *(ftp->ftable + (indx*2));
      v2 = *(ftp->ftable + (indx*2) + 2);
      xndx = v1 + (v2 - v1) * fract;

      v1 = *(ftp->ftable + (indx*2) + 1);
      v2 = *(ftp->ftable + (indx*2) + 3);
      yndx = v1 + (v2 - v1) * fract;
    }
    else { /* get xy vals from input arguments */
      xndx = *p->kx;
      yndx = *p->ky;
    }

    distance = (MYFLT)sqrt((xndx*xndx) + (yndx*yndx));
    if (distance < FL(1.0)) distance = FL(1.0);
    *r=distance;
    return OK;
}

#define S sizeof

static OENTRY localops[] = {
{ "space",  S(SPACE),  5, "aaaa", "aikkkk",(SUBR)spaceset, NULL, (SUBR)space },
{ "spsend", S(SPSEND), 5, "aaaa", "",     (SUBR)spsendset, NULL, (SUBR)spsend },
{ "spdist", S(SPDIST), 3,    "k", "ikkk", (SUBR)spdistset, (SUBR)spdist, NULL }
};

LINKAGE
