
#include "csoundCore.h"

static const char *utilName = "lpanal";

int main(int argc, char **argv)
{
    ENVIRON *csound;
    int     retval;
    init_getstring(argc, argv);
    csound = (ENVIRON*) csoundCreate(NULL);
    csoundPreCompile(csound);
    csound->orchname_ = (char*) utilName;
    csound->scorename_ = (char*) utilName;
    retval = csound->Utility(csound, utilName, argc, argv);
    csoundDestroy(csound);
    return retval;
}

