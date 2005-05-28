#include "csdl.h"
#include <math.h>

void tanhtable(FUNC *ftp, FGDATA *ff)
{
    MYFLT *fp = ftp->ftable;
    MYFLT range = ff->e.p[5];
    double step = (double)range/(ff->e.p[3]);
    int i;
    double x;
    for (i=0, x=FL(0.0); i<ff->e.p[3]; i++, x+=step)
      *fp++ = (MYFLT)tanh(x);
}

static NGFENS localfgens[] = {
   { "tanh", (void(*)(void))tanhtable},
   { NULL, NULL}
};

#define S       sizeof

static OENTRY localops[] = {};

FLINKAGE

