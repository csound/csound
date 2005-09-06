
#include "csdl.h"
#include <math.h>

static int tanhtable(FGDATA *ff, FUNC *ftp)
{
 /* CSOUND  *csound = ff->csound; */
    MYFLT   *fp = ftp->ftable;
    MYFLT   range = ff->e.p[5];
    double  step = (double) range / (double) ftp->flen;
    int     i;
    double  x;

    for (i = 0, x = 0.0; i <= (int) ftp->flen; i++, x += step)
      fp[i] = (MYFLT) tanh(x);

    return OK;
}

static NGFENS localfgens[] = {
   { "tanh", tanhtable },
   { NULL, NULL }
};

static OENTRY *localops = NULL;

FLINKAGE

