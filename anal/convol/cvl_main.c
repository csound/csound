#include "ustub.h"
#include <stdio.h>

#ifdef USE_FLTK
int fltk_abort = 0;
#endif

int main(int argc, char **argv)
{
    scorename = "Cvanal";
    orchname = "Cvanal";
    init_getstring(argc,argv);
    return cvanal(argc,argv);
}
