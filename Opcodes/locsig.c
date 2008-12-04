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

static int locsigset(CSOUND *csound, LOCSIG *p)
{
    STDOPCOD_GLOBALS  *pp;
    int     outcount = p->OUTOCOUNT;

    if (UNLIKELY(outcount != 2 && outcount != 4))
      return csound->InitError(csound, Str("Wrong number of outputs in locsig; "
                                           "must be 2 or 4"));

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

    p->prev_degree = -FL(918273645.192837465);
    p->prev_distance = -FL(918273645.192837465);

    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    pp->locsigaddr = (void*) p;

    return OK;
}

static int locsig(CSOUND *csound, LOCSIG *p)
{
    MYFLT *r1, *r2, *r3=NULL, *r4=NULL, degree, *asig;
    MYFLT direct, *rrev1, *rrev2, *rrev3=NULL, *rrev4=NULL;
    MYFLT torev, localrev, globalrev;
    int n, nsmps = csound->ksmps;

    if (*p->distance != p->prev_distance) {
      p->distr=(FL(1.0) / *p->distance);
      p->distrsq = FL(1.0)/SQRT(*p->distance);
      p->prev_distance = *p->distance;
    }

    if (*p->degree != p->prev_degree) {

      degree = *p->degree/FL(360.00);

      p->ch1 = COS(TWOPI_F * degree);
      if (p->ch1 < FL(0.0)) p->ch1 = FL(0.0);

      p->ch2 = SIN(TWOPI_F * degree);
      if (p->ch2 < FL(0.0)) p->ch2 = FL(0.0);

      if (p->OUTOCOUNT == 4) {
        p->ch3 = COS(TWOPI_F * (degree + FL(0.5)));
        if (p->ch3 < FL(0.0)) p->ch3 = FL(0.0);

        p->ch4 = SIN(TWOPI_F * (degree + FL(0.5)));
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

    for (n=0; n<nsmps; n++) {
      direct = asig[n] * p->distr;
      torev = asig[n] * p->distrsq * *p->reverbamount;
      globalrev = torev * p->distr;
      localrev = torev * (FL(1.0) - p->distr);

      r1[n] = direct * p->ch1;
      r2[n] = direct * p->ch2;
      rrev1[n] = (localrev * p->ch1) + globalrev;
      rrev2[n] = (localrev * p->ch2) + globalrev;

      if (p->OUTOCOUNT == 4) {
        r3[n] = direct *  p->ch3;
        r4[n] = direct *  p->ch4;
        rrev3[n] = (localrev* p->ch3) + globalrev;
        rrev4[n] = (localrev* p->ch4) + globalrev;
      }
    }
    return OK;
}

static int locsendset(CSOUND *csound, LOCSEND *p)
{
    STDOPCOD_GLOBALS  *pp;
    LOCSIG  *q;

    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    q = (LOCSIG*) pp->locsigaddr;
    p->locsig = q;

    if (UNLIKELY(p->OUTOCOUNT != q->OUTOCOUNT)) {
      return csound->InitError(csound, Str("Number of outputs must be the "
                                           "same as the previous locsig"));
    }
    return OK;
}

static int locsend(CSOUND *csound, LOCSEND *p)
{
/*     MYFLT       *r1, *r2, *r3=NULL, *r4=NULL; */
/*     MYFLT       *rrev1, *rrev2, *rrev3=NULL, *rrev4=NULL; */
    LOCSIG *q = p->locsig;
    int n, nsmps = csound->ksmps;

/*     r1 = p->r1; */
/*     r2 = p->r2; */
/*     rrev1 = q->rrev1; */
/*     rrev2 = q->rrev2; */

/*     if (p->OUTOCOUNT == 4) { */
/*       r3 = p->r3; */
/*       r4 = p->r4; */
/*       rrev3 = q->rrev3; */
/*       rrev4 = q->rrev4; */
/*     } */
/*     for (n=0;n<nsmps; n++) { */
/*       r1[n] = rrev1[n]; */
/*       r2[n] = rrev2[n]; */

/*       if (p->OUTOCOUNT == 4) { */
/*         r3[n] = rrev3[n]; */
/*         r4[n] = rrev4[n]; */
/*       } */
/*     } */
    /*
      Quicker form is: */
    n = nsmps*sizeof(MYFLT);
    memcpy(p->r1, q->rrev1, n);
    memcpy(p->r2, q->rrev2, n);
    if (p->OUTOCOUNT == 4) {
      memcpy(p->r3, q->rrev3, n);
      memcpy(p->r4, q->rrev4, n);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "locsig", S(LOCSIG),  5, "mmmm", "akkk", (SUBR)locsigset,NULL, (SUBR)locsig    },
{ "locsend", S(LOCSEND),5, "mmmm", "",     (SUBR)locsendset, NULL, (SUBR)locsend }
};

int locsig_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

