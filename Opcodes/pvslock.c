#include "csdl.h"
#include "pvsbasic.h"
#include "pvfileio.h"
#include <math.h>

typedef struct _pvslock {
    OPDS    h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *delta;
    float   mag;
    uint32  lastframe;
} PVSLOCK;

static int pvslockset(CSOUND *csound, PVSLOCK *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->mag = 0.000001;
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);


    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                 (p->fout->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvslock: signal format "
                                           "must be amp-phase or amp-freq."));

    return OK;
}

static int pvslockprocess(CSOUND *csound, PVSLOCK *p)
{
    int     i;
    int32    framesize, N = p->fin->N;
    float   *fout, *fin, cmag = p->mag, mag=0.0f, diff;
    fout = (float *) p->fout->frame.auxp;
    fin = (float *) p->fin->frame.auxp;
    framesize = N + 2;
    if (p->lastframe < p->fin->framecount) {

      for (i = 0; i < framesize; i += 2) {

        mag += fin[i];
        fout[i] = fin[i]; fout[i+1] = fin[i+1];

      }
      mag /= N;
      diff = 20.0f*log10f(mag/cmag);
      if (diff > *p->delta)
        csound->Warning(csound, "att= %f %f\n", diff, mag);
      p->mag = mag;
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}



static OENTRY localops[] = {
    {"pvslock", sizeof(PVSLOCK), 3, "f", "fk", (SUBR) pvslockset,
         (SUBR) pvslockprocess, NULL}
};

int pvslock_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

