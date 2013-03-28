#ifdef JPFF

#include "std_util.h"                                   /*  HETRO.C   */

static int scope(CSOUND *csound, int argc, char **argv)
{
    FILE *fin = stdin;
    if (argc!=1) fin = fopen(argv[1], "r");

}

/* module interface */

int scope_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "scope", scope);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "scope",
                                             Str("Test utility for score parser"));
    }
    return retval;
}

#endif

