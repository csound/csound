/* Console Csound using the Csound API. */

#include "csound.h"
#include <stdio.h>
#include <stdarg.h>

int main(int argc, char **argv)
{
    /*	Create Csound. */
    void *csound = csoundCreate(0);
    /*	One complete performance cycle. */
    int result = csoundCompile(csound, argc, argv);
    if(!result)
      {
        while(csoundPerformKsmps(csound) == 0)
        {
          csoundYield(csound);
        }
        csoundCleanup(csound);
      }
    /*	Destroy Csound. */
    csoundDestroy(csound);
    return result;
}
