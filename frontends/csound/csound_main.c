/* Console Csound using the Csound API. */

#include "csound.h"
#include <stdio.h>
#include <stdarg.h>

int main(int argc, char **argv)
{
    void *csound;
    int result;
    /* set stdout to non buffering if not outputing to console window */
    if(!isatty(fileno(stdout)))
    {
        setvbuf(stdout, (char *)NULL, _IONBF, 0);
    }

    /*	Create Csound. */
    csound = csoundCreate(0);
    /*	One complete performance cycle. */
    result = csoundCompile(csound, argc, argv);
    if (!result)
      {
        while (csoundPerformKsmps(csound) == 0)
        {
          csoundYield(csound);
        }
      }
    {       /* IV - Jan 28 2005 */
      extern void print_benchmark_info(void*, const char*);   /* in main.c */
      print_benchmark_info(csound, Str("end of performance"));
    }
    csoundDestroy(csound);
    return result;
}
