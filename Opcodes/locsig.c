/*
    locsig.c:

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

/*LOCSIG*/

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/

#include "csdl.h"
#include "locsig.h"
#include <math.h>

static  LOCSIG  *locsigaddr;

int locsigset(ENVIRON *csound, LOCSIG *p)
{
    int outcount=p->OUTOCOUNT;

    if (outcount != 2)
      if (outcount != 4) {
        sprintf(errmsg,
                Str("Wrong number of outputs in locsig; must be 2 or 4"));
        goto locerr;
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

    p->prev_degree = -FL(918273645.192837465);
    p->prev_distance = -FL(918273645.192837465);

    locsigaddr=p;

    return OK;
locerr:
    return initerror(errmsg);
}

int locsig(ENVIRON *csound, LOCSIG *p)
{
    MYFLT *r1, *r2, *r3=NULL, *r4=NULL, degree, *asig;
    MYFLT direct, *rrev1, *rrev2, *rrev3=NULL, *rrev4=NULL;
    MYFLT torev, localrev, globalrev;
    int nsmps = ksmps;

    if (*p->distance != p->prev_distance) {
      p->distr=(FL(1.0) / *p->distance);
      p->distrsq = FL(1.0)/(MYFLT)sqrt(*p->distance);
      p->prev_distance = *p->distance;
    }

    if (*p->degree != p->prev_degree) {

      degree = *p->degree/FL(360.00);

      p->ch1 = (MYFLT)cos(TWOPI * (MYFLT)degree);
      if (p->ch1 < FL(0.0)) p->ch1 = FL(0.0);

      p->ch2 = (MYFLT)sin(TWOPI * (MYFLT)degree);
      if (p->ch2 < FL(0.0)) p->ch2 = FL(0.0);

      if (p->OUTOCOUNT == 4) {
        p->ch3 = (MYFLT)cos(TWOPI * ((MYFLT)degree + 0.5));
        if (p->ch3 < FL(0.0)) p->ch3 = FL(0.0);

        p->ch4 = (MYFLT)sin(TWOPI * ((MYFLT)degree + 0.5));
        if (p->ch4 < FL(0.0)) p->ch4 = FL(0.0);
      }

      p->prev_degree = *p->degree;
    }

    r1 = p->r1;
    r2 = p->r2;
    asig = p->asig;

    rrev1 = p->rrev1;
    rrev2 = p->rrev2;

    if (p->OUTOCOUNT == 4) {
      r3 = p->r3;
      r4 = p->r4;
      rrev3 = p->rrev3;
      rrev4 = p->rrev4;
    }

    do {
      direct = *asig * p->distr;
      torev = *asig * p->distrsq * *p->reverbamount;
      globalrev = torev * p->distr;
      localrev = torev * (1 - p->distr);

      *r1++ = direct * p->ch1;
      *r2++ = direct * p->ch2;
      *rrev1++ = (localrev * p->ch1) + globalrev;
      *rrev2++ = (localrev * p->ch2) + globalrev;

      if (p->OUTOCOUNT == 4) {
        *r3++ = direct *  p->ch3;
        *r4++ = direct *  p->ch4;
        *rrev3++ = (localrev* p->ch3) + globalrev;
        *rrev4++ = (localrev* p->ch4) + globalrev;
      }
      asig++;
    }
    while (--nsmps);
    return OK;
}

int locsendset(ENVIRON *csound, LOCSEND *p)
{
    LOCSIG *q;

    p->locsig=locsigaddr;
    q = p->locsig;

    if (p->OUTOCOUNT != q->OUTOCOUNT) {
      sprintf(errmsg,
              Str(
                  "Number of outputs must be the same as the previous locsig;"));
      goto locerr;
    }
    return OK;
  locerr:
    return initerror(errmsg);
}

int locsend(ENVIRON *csound, LOCSEND *p)
{
    MYFLT       *r1, *r2, *r3=NULL, *r4=NULL;
    MYFLT       *rrev1, *rrev2, *rrev3=NULL, *rrev4=NULL;
    LOCSIG *q = p->locsig;
    int nsmps = ksmps;

    r1 = p->r1;
    r2 = p->r2;
    rrev1 = q->rrev1;
    rrev2 = q->rrev2;

    if (p->OUTOCOUNT == 4) {
      r3 = p->r3;
      r4 = p->r4;
      rrev3 = q->rrev3;
      rrev4 = q->rrev4;
    }
    do {
      *r1++ = *rrev1++;
      *r2++ = *rrev2++;

      if (p->OUTOCOUNT == 4) {
        *r3++ = *rrev3++;
        *r4++ = *rrev4++;
      }
    }
    while (--nsmps);
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "locsig", S(LOCSIG),  5, "mmmm", "akkk", (SUBR)locsigset,NULL, (SUBR)locsig    },
{ "locsend", S(LOCSEND),5, "mmmm", "",     (SUBR)locsendset, NULL, (SUBR)locsend },
};

LINKAGE
