/*
    nlfilt.c:

    Copyright (C) 1996 John ffitch, Richard Dobson

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

/* Y{n} =a Y{n-1} + b Y{n-2} + d Y^2{n-L} + X{n} - C */

/***************************************************************\
*       nlfilt.c                                                *
*       Non-linear filter (Excitable region)                    *
*       5 June 1996 John ffitch                                 *
*       See paper by Dobson and ffitch, ICMC'96                 *
\***************************************************************/

#include "csdl.h"
#include "nlfilt.h"

#define MAX_DELAY       (1024)
int nlfiltset(ENVIRON *csound, NLFILT *p)
{
    int   i;
    MYFLT *fp;

    if (p->delay.auxp == NULL) { /* get newspace    */
      auxalloc(MAX_DELAY*sizeof(MYFLT),&p->delay);
    }
    fp = (MYFLT*)p->delay.auxp;
    for (i=0; i<MAX_DELAY; i++)
      fp[i] = FL(0.0);  /* Clear delays */
    p->point = 0;
    return OK;
} /* end nlfset(p) */

#define MAXAMP (FL(64000.0))
int nlfilt(ENVIRON *csound, NLFILT *p)
{
    MYFLT       *ar;
    int         nsmps;
    int         point = p->point;
    int         nm1 = point;
    int         nm2 = point -1;
    int         nmL;
    MYFLT       ynm1, ynm2, ynmL;
    MYFLT       a = *p->a, b = *p->b, d = *p->d, C = *p->C;
    MYFLT       *in = p->in;
    MYFLT       *fp = (MYFLT*)p->delay.auxp;
    MYFLT       L = *p->L;

    if (fp==NULL) {             /* RWD fix */
      return perferror(Str(X_1015,"nlfilt: not initialised"));
    }
    ar   = p->ar;
    nsmps = ksmps;             /* Number of points to calculate */
                                /* L is k-rate so need to check */
    if (L < FL(1.0)) L = FL(1.0);
    else if (L >= MAX_DELAY) {
      /*      fprintf(stderr, "Delay parameter too large at %f\n", L); */
      L = (MYFLT)MAX_DELAY;
    }
    nmL = point - (int)(L)-1;
    if (nm1<0) nm1 += MAX_DELAY; /* Deal with the wrapping */
    if (nm2<0) nm2 += MAX_DELAY;
    if (nmL<0) nmL += MAX_DELAY;
    ynm1 = fp[nm1];     /* Pick up running values */
    ynm2 = fp[nm2];
    ynmL = fp[nmL];
/* printf("Y-1=%f Y-2=%f Y-L=%f\t", ynm1, ynm2, ynmL); */
/* printf("n-1=%d n-2=%d n-L=%d\n", nm1, nm2, nmL); */
    do {
      MYFLT yn;
      MYFLT out;
      yn = a*ynm1 + b*ynm2 + d*ynmL*ynmL - C;
      if (in != NULL) {
        yn += (*in++)/e0dbfs;   /* Must work in (-1,1) amplitudes  */
      }
/* printf("=> yn=%f\n", yn); */
      out = yn*e0dbfs;          /* Write output */
      if (out >= e0dbfs)       out =  e0dbfs*FL(0.999);
      else if (out <= -e0dbfs) out = -e0dbfs*FL(0.999);
      *ar++ = out;
      if (++point == MAX_DELAY) {
        point = 0;
      }
      fp[point] = yn; /* and delay line */
      if (++nmL==MAX_DELAY) {
        nmL = 0;
      }
      ynm2 = ynm1;              /* Shuffle along */
      ynm1 = yn;
      ynmL = fp[nmL];
/* printf("Y-1=%f Y-2=%f Y-L(%d)=%f\n", ynm1, ynm2, nmL, ynmL); */
    } while (--nsmps);
    p->point = point;
    return OK;
} /* end nlfilt(p) */

/* Y{n} =a Y{n-1} + b Y{n-2} + d Y^2{n-L} + X{n} - C */

#define S       sizeof

static OENTRY localops[] = {
{ "nlfilt",  S(NLFILT), 5, "a","akkkkk",(SUBR)nlfiltset, NULL, (SUBR)nlfilt}
};

LINKAGE

