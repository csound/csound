#include "csdl.h"
#include <math.h>

void tanhtable(FUNC *ftp, EVTBLK *e)
{
    MYFLT *fp = ftp->ftable;
    MYFLT range = e->p[5];
    MYFLT step = range/(e->p[3]);
    int i;
    MYFLT x;
    for (i=0, x=0.0; i<e->p[3]; i++, x+=step)
      *fp++ = tanh(x);
}

static NGFENS localfgens[] = {
   { "tanh", (void(*)(void))tanhtable},
   { NULL, NULL}
};

#define S       sizeof

static OENTRY localops[] = {};

FLINKAGE

