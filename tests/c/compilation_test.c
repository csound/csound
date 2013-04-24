#include "csound.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    CSOUND  *csound;
    int     result, compile_again=0;
    char  *instrument = 
      "instr 1 \n"
      "k1 expon p4, p3, p4*0.001 \n"
      "a1 randi  k1, p5   \n"
      "out  a1   \n"
       "endin \n";

    char  *instrument2 = 
      "instr 2 \n"
      "k1 expon p4, p3, p4*0.001 \n"
      "a1 vco2  k1, p5   \n"
      "out  a1   \n"
       "endin \n"
      "event_i \"i\",2, 0.5, 2, 10000, 800 \n";


    csoundInitialize(&argc, &argv, 0);
    csound = csoundCreate(NULL);
    csoundSetOption(csound,"-odac");
    result = csoundCompileOrc(csound, instrument);
   if(!result) {
       result = csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
      if(!result) {
           result = csoundStart(csound);
           while(!result){
            result = csoundPerformKsmps(csound);
            if(!compile_again){ 
              /* new compilation */
        	csoundCompileOrc(csound, instrument2);
	       /* schedule an event on instr2 */
	        csoundReadScore(csound, "i2 1 1 10000 110 \n i2 + 1 1000 660");
                compile_again = 1;
              }
            }
        }
    }
    /* delete Csound instance */
     csoundDestroy(csound);
    return 0;
}

