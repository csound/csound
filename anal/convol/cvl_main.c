#include "cs.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    scorename = "Cvanal";
    orchname = "Cvanal";
    init_getstring(argc,argv);
    csoundPreCompile(csoundCreate(NULL));
    return cvanal(argc,argv);
}

