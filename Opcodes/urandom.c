/*
    urandom.c:

    Copyright (C) 2010 by John ffitch

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

#include "csdl.h"
//#include <ieee754.h>

typedef struct {
    OPDS    h;
    MYFLT   *ar;                /* Output */
    MYFLT   *imin;
    MYFLT   *imax;
    int     ur;
    MYFLT   mul;
    MYFLT   add;
} URANDOM;

static int urand_deinit(CSOUND *csound, URANDOM *p)
{
    close(p->ur);
    return OK;
}

static int urand_init(CSOUND *csound, URANDOM *p)
{
    int ur = open("/dev/urandom", O_RDONLY);
    if (UNLIKELY(ur<0)) return NOTOK;
    p->ur = ur;
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND *, void *)) urand_deinit);
    p->mul = FL(0.5)*(*p->imax - *p->imin);
    p->add = FL(0.5)*(*p->imax + *p->imin);
    return OK;
}

static int urand_run(CSOUND *csound, URANDOM *p)
{
    int ur = p->ur;
    /* union ieee754_double x; */
    int64_t x;
    if (UNLIKELY(read(ur, &x, sizeof(int64_t))!=sizeof(int64_t))) return NOTOK;

/* x.ieee.exponent = x.ieee.exponent& 0x377; */
/* printf("Debug: %s(%d): %g %d %03x %05x %08x\n", __FILE__, __LINE__, x.d, */
/*        x.ieee.negative, x.ieee.exponent, x.ieee.mantissa0, x.ieee.mantissa1); */
    *p->ar = p->mul *((MYFLT)x/(MYFLT)INT64_MAX) + p->add;
    return OK;
}

static int urand_irate(CSOUND *csound, URANDOM *p)
{
    if (LIKELY(urand_init(csound,p)==OK)) return urand_run(csound,p);
    else return NOTOK;
}

static int urand_arun(CSOUND *csound, URANDOM *p)
{
    int ur = p->ur;
    /* union ieee754_double x; */
    int64_t x;
    MYFLT *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      if (UNLIKELY(read(ur, &x, sizeof(int64_t))!= sizeof(int64_t))) return NOTOK;
      ar[n] = p->mul *((MYFLT)x/(MYFLT)INT64_MAX) + p->add;
    }
    return OK;
}


#define S(x)    sizeof(x)

static OENTRY urandom_localops[] = {
  { "urandom",      0xFFFF,  0,            0,      NULL,   NULL, NULL},
  { "urandom.i", S(URANDOM), 0, 1, "i", "jp", (SUBR) urand_irate },
  { "urandom.k", S(URANDOM), 0, 3, "k", "jp", (SUBR) urand_init, (SUBR) urand_run},
  { "urandom.a", S(URANDOM), 0, 5, "a", "jp",
                                    (SUBR) urand_init, NULL, (SUBR) urand_arun}
};

LINKAGE_BUILTIN(urandom_localops)

