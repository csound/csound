#include "csdl.h"

typedef struct _inmess {
  OPDS h;
  MYFLT *SMess, *ktrig;
} inmess;

int messi(CSOUND *csound, inmess *p)
{
    csound->InputMessage(csound, (char *)p->SMess);
    return OK;
}

int messk(CSOUND *csound, inmess *p){
   if(*p->ktrig) csound->InputMessage(csound, (char *)p->SMess);
    return OK;
}

static OENTRY localops[] = {
  {"scoreline_i", sizeof(inmess), 1, "", "S", (SUBR)messi, NULL, NULL},
  {"scoreline", sizeof(inmess), 2, "", "Sk", NULL, (SUBR)messk, NULL}
};

LINKAGE
