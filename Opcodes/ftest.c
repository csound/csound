#include "csdl.h"
#include <math.h>

void tanhtable(FUNC *ftp, FGDATA *ff)
{
    MYFLT *fp = ftp->ftable;
    MYFLT range = ff->e.p[5];
    double step = (double)range/(ff->e.p[3]);
    int i;
    double x;
    for (i=0, x=FL(0.0); i<ff->e.p[3]; i++, x+=step)
      *fp++ = (MYFLT)tanh(x);
}

static void gentune(void)             /* Gab 1/3/2005 */
{
    int j;
    int notenum;
    int grade;
    int numgrades;
    int basekeymidi;
    MYFLT basefreq, factor,interval;

    MYFLT	*fp = ftp->ftable, *pp = &e->p[5];
    int	nvals = nargs;

    numgrades = (int) *pp++;
    interval  = *pp++;
    basefreq  = *pp++;
    basekeymidi=  (int) *pp++;

    nvals = flenp1 - 1;

    for (j =0; j < nvals; j++) {
      notenum = j;
      if (notenum < basekeymidi) {
        notenum = basekeymidi - notenum;
        grade  = (numgrades-(notenum % numgrades)) % numgrades;
        factor = - (MYFLT)(int) ((notenum+numgrades-1) / numgrades) ;
      }
      else {
        notenum = notenum - basekeymidi;
        grade  = notenum % numgrades;
        factor = (MYFLT)(int) (notenum / numgrades);
      }
      factor = (MYFLT)pow((double)interval, (double)factor);
      fp[j] = pp[grade] * factor * basefreq;
      
    }
}


static NGFENS localfgens[] = {
   { "tanh", (void(*)(void))tanhtable},
   { "cpstune", (void(*)(void))gentune},
   { NULL, NULL}
};

#define S       sizeof

static OENTRY localops[] = {};

FLINKAGE

