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

#define RESOLUTION 100

static int spaceset(CSOUND *csound, SPACE *p)
{
    STDOPCOD_GLOBALS  *pp;
    FUNC              *ftp = NULL;

    if (*p->ifn > 0) {
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) == NULL))
        return NOTOK;
      p->ftp = ftp;
    }

    if (p->auxch.auxp == NULL ||
        p->auxch.size<sizeof(MYFLT)*(csound->ksmps * 4)) {
      MYFLT *fltp;
      csound->AuxAlloc(csound, (size_t) (csound->ksmps * 4)
                               * sizeof(MYFLT), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->rrev1 = fltp;   fltp += csound->ksmps;
      p->rrev2 = fltp;   fltp += csound->ksmps;
      p->rrev3 = fltp;   fltp += csound->ksmps;
      p->rrev4 = fltp;   fltp += csound->ksmps;
    }

    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    pp->spaceaddr = (void*) p;
    return OK;
}

static int space(CSOUND *csound, SPACE *p)
{
    MYFLT   *r1, *r2, *r3, *r4, *sigp, ch1, ch2, ch3, ch4;
    MYFLT   distance=FL(1.0), distr, distrsq, direct;
    MYFLT   *rrev1, *rrev2, *rrev3, *rrev4;
    MYFLT   torev, localrev, globalrev;
    MYFLT   xndx, yndx;
    MYFLT   half_pi = FL(0.5)*PI_F;
    MYFLT   sqrt2 = SQRT(FL(2.0));
    MYFLT   fabxndx, fabyndx;
    int     n;
    FUNC    *ftp;
    int32    indx, length, halflen;
    MYFLT   v1, v2, fract, ndx;
    int nsmps = csound->ksmps;

    if (*p->ifn > 0) { /* get xy vals from function table */
      if (UNLIKELY((ftp = p->ftp) == NULL)) goto err1;

      ndx = *p->time * RESOLUTION; /* when data is res. frames/second */
      length = ftp->flen;
      halflen = (int32)(length * FL(0.5));
      indx = (int32) floor(ndx);
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

    distance = HYPOT(xndx, yndx);
    fabxndx = FABS(xndx);
    fabyndx = FABS(yndx);
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
    distrsq = FL(1.0)/SQRT(distance);

    xndx = (xndx+FL(1.0))*FL(0.5);
    yndx = (yndx+FL(1.0))*FL(0.5);

    ch2 = SIN(half_pi * xndx) * SIN(half_pi * yndx) * sqrt2;
    ch4 = SIN(half_pi * xndx) * SIN(half_pi * (FL(1.0)-yndx)) * sqrt2;
    ch1 = SIN(half_pi * (FL(1.0)-xndx)) * SIN(half_pi * yndx) * sqrt2;
    ch3 = SIN(half_pi * (FL(1.0)-xndx)) * SIN(half_pi * (FL(1.0)-yndx)) * sqrt2;

    r1 = p->r1;
    r2 = p->r2;
    r3 = p->r3;
    r4 = p->r4;
    rrev1 = p->rrev1;
    rrev2 = p->rrev2;
    rrev3 = p->rrev3;
    rrev4 = p->rrev4;
    sigp = p->asig;
    for (n=0; n<nsmps; n++) {
      direct = sigp[n] * distr;
      torev = sigp[n] * distrsq * *p->reverbamount;
      globalrev = torev * distr;
      localrev = torev * (FL(1.0) - distr);
      r1[n] = direct * ch1;
      r2[n] = direct * ch2;
      r3[n] = direct * ch3;
      r4[n] = direct * ch4;
      rrev1[n] = (localrev * ch1) + globalrev;
      rrev2[n] = (localrev * ch2) + globalrev;
      rrev3[n] = (localrev * ch3) + globalrev;
      rrev4[n] = (localrev * ch4) + globalrev;
    }
    return OK;
 err1:
    return csound->PerfError(csound, Str("space: not initialised"));
}

static int spsendset(CSOUND *csound, SPSEND *p)
{
    STDOPCOD_GLOBALS  *pp;

    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    p->space = (SPACE*) pp->spaceaddr;
    return OK;
}

static int spsend(CSOUND *csound, SPSEND *p)
{
    SPACE *q = p->space;
    int nbytes = csound->ksmps*sizeof(MYFLT);

    memmove(p->r1, q->rrev1, nbytes);
    memmove(p->r2, q->rrev2, nbytes);
    memmove(p->r3, q->rrev3, nbytes);
    memmove(p->r4, q->rrev4, nbytes);
    return OK;
}

static int spdistset(CSOUND *csound, SPDIST *p)
{
   FUNC *ftp;

   if (*p->ifn > 0) {
     if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) == NULL))
       return NOTOK;
     p->ftp = ftp;
   }
   return OK;
}

static int spdist(CSOUND *csound, SPDIST *p)
{
    MYFLT      *r;
    MYFLT       distance, xndx, yndx;
    FUNC       *ftp;
    int32       indx, length, halflen;
    MYFLT       v1, v2, fract, ndx;

    r = p->r;

    if (*p->ifn > 0) {
      if (UNLIKELY((ftp = p->ftp)==NULL)) goto err1;

      ndx = *p->time * RESOLUTION; /* when data is 10 frames/second */
      length = ftp->flen;
      halflen = (int32)(length * FL(0.5));
      indx = (int32) floor(ndx);
      fract = ndx - indx;

      if (ndx > (halflen-1)) {
        indx  = halflen - 1;
        fract = FL(0.0);
      }
      else if (ndx < 0) {
        indx  = 0L;
        fract = FL(0.0);
      }

      v1 = *(ftp->ftable + (indx+indx));
      v2 = *(ftp->ftable + (indx+indx) + 2);
      xndx = v1 + (v2 - v1) * fract;

      v1 = *(ftp->ftable + (indx+indx) + 1);
      v2 = *(ftp->ftable + (indx+indx) + 3);
      yndx = v1 + (v2 - v1) * fract;
    }
    else { /* get xy vals from input arguments */
      xndx = *p->kx;
      yndx = *p->ky;
    }

    distance = HYPOT(xndx,yndx);
    if (distance < FL(1.0)) distance = FL(1.0);
    *r=distance;
    return OK;
 err1:
    return csound->PerfError(csound, Str("spdist: not initialised"));
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "space",  S(SPACE),  TR|5, "aaaa", "aikkkk",(SUBR)spaceset, NULL, (SUBR)space },
{ "spsend", S(SPSEND), 5, "aaaa", "",     (SUBR)spsendset, NULL, (SUBR)spsend },
{ "spdist", S(SPDIST), 3,    "k", "ikkk", (SUBR)spdistset, (SUBR)spdist, NULL }
};

int space_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

