#include "csound.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    CSOUND  *csound;
    int     result, compile_again=0;
    char  *instrument = 
      "instr 2 \n"
      "k1 expon p4, p3, p4*0.001 \n"
      "a1 vco2  k1, p5   \n"
      "out  a1   \n"
       "endin \n";
    csoundInitialize(&argc, &argv, 0);
    csound = csoundCreate(NULL);
    result = csoundCompile(csound, argc, argv);
    while(!result){
    result = csoundPerformKsmps(csound);
    if(!compile_again){ 
         /* new compilation */
    	csoundCompileOrc(csound, instrument);
	/* schedule an event on instr 2 */
	csoundReadScore(csound, "i2 0 1 10000 110 \n i2 + 1 1000 660");
        compile_again = 1;
      }
    }
    /* delete Csound instance */
     csoundDestroy(csound);
    return 0;
}

