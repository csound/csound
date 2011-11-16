/*
    ftest.c:

    Copyright (C) 2004,2008 John ffitch, Victor Lazzarini

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

//#include "csdl.h"
#include "csoundCore.h"
#include <math.h>

static int tanhtable(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;
    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step)
      fp[i] = TANH(x);

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    return OK;
}

static int exptable(FGDATA *ff, FUNC *ftp)
{
 /* CSOUND  *csound = ff->csound; */
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;

    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step)
      fp[i] = EXP(x);

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    return OK;
}

/* Translation table from perceived to actual amplitude */
static int sonetable(FGDATA *ff, FUNC *ftp)
{
 /* CSOUND  *csound = ff->csound; */
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   eqlp  = ff->e.p[7];
    MYFLT   resc  = ff->e.p[8];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;

    if (eqlp==FL(0.0)) eqlp = FL(0.001);
    /* printf("Sone: %f %f %f %f\n", ff->e.p[5], ff->e.p[6], ff->e.p[7], ff->e.p[8]); */
    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step) {
      fp[i] = x*POWER(x/eqlp, FL(33.0)/FL(78.0));
      /* printf("%f -> %f\n", x, fp[i]); */
    }

    if (resc!=FL(0.0)) ff->e.p[4] = -ff->e.p[4];
    return OK;
}


static NGFENS ftest_fgens[] = {
   { "tanh", tanhtable },
   { "exp", exptable },
   { "sone", sonetable },
   { NULL, NULL }
};

FLINKAGE1(ftest_fgens)

