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
#include <ieee754.h>

/* %% bar sound synthesis translated from Mathlab and much changed */

/*  Note: position of strike along bar (0 - 1), normalized strike
    velocity, and spatial width of strike.  */

typedef struct {
    OPDS    h;
    MYFLT   *ar;                /* Output */
    int     ur;
} URANDOM;

static int urand_deinit(CSOUND *csound, URANDOM *p)
{
    close(p->ur);
    return OK;
}

static int urand_init(CSOUND *csound, URANDOM *p)
{
    int ur = open("/dev/urandom", O_RDONLY);
    if (ur<0) return NOTOK;
    p->ur = ur;
    csound->RegisterDeinitCallback(csound, p,
                                   (int (*)(CSOUND *, void *)) urand_deinit);
    return OK;
}

static int urand_run(CSOUND *csound, URANDOM *p)
{
    int ur = p->ur;
    /* union ieee754_double x; */
    int64_t x;
    read(p->ur, &x, sizeof(int64_t));
    
    /* x.ieee.exponent = x.ieee.exponent& 0x377; */
    /* printf("Debug: %s(%d): %g %d %03x %05x %08x\n", __FILE__, __LINE__, x.d, */
    /*        x.ieee.negative, x.ieee.exponent, x.ieee.mantissa0, x.ieee.mantissa1); */
    *p->ar = (MYFLT)x/(MYFLT)0x7fffffffffffffff;
    return OK;
}


#define S(x)    sizeof(x)

static OENTRY localops[] = {
    {"urandom", S(URANDOM), 3, "k", "", (SUBR) urand_init, (SUBR) urand_run}
};

LINKAGE

