
#include "csdl.h"
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
    MYFLT   resc  = ff->e.p[7];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;

    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step)
      fp[i] = x*POWER(x/start, FL(33.0)/FL(78.0));

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    return OK;
}


static NGFENS localfgens[] = {
   { "tanh", tanhtable },
   { "exp", exptable },
   { "sone", sonetable },
   { NULL, NULL }
};

FLINKAGE

