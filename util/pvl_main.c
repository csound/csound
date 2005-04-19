
#include "csoundCore.h"

static const char *utilName = "pvlook";

int main(int argc, char **argv)
{
    ENVIRON *csound;
    int     retval;

    if ( argc == 1 ) {
      fprintf(stderr,"pvlook is a program which reads a Csound pvanal's pvoc.n "
               "file and outputs frequency and magnitude trajectories for each "
               "of the analysis bins. \n");
      fprintf(stderr, "usage: pvlook [-bb X] [-eb X] [-bf X] [-ef X] [-i X]  "
              "file > output\n");
      fprintf(stderr,
              " -bb X  begin at anaysis bin X. Numbered from 1 "
              "[defaults to 1]\n");
      fprintf(stderr,
              " -eb X  end at anaysis bin X [defaults to highest]\n");
      fprintf(stderr,
              " -bf X  begin at anaysis frame X. Numbered from 1 "
              "[defaults to 1]\n");
      fprintf(stderr,
              " -ef X  end at anaysis frame X [defaults to last]\n");
      fprintf(stderr,
              " -i X  prints values as integers [defaults to "
              "floating point]\n");
      exit(-1);
    }
    init_getstring(argc, argv);
    csound = (ENVIRON*) csoundCreate(NULL);
    csoundPreCompile(csound);
    csound->orchname_ = (char*) utilName;
    csound->scorename_ = (char*) utilName;
    retval = csound->Utility(csound, utilName, argc, argv);
    csoundDestroy(csound);
    return retval;
}

