/*
    pvread.c:

    Copyright (C) 1992, 2000 Richard Karpen, Richard Dobson

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/**************************************************************/
/***********pvread ********************************************/
/*** By Richard Karpen - July-October 1992*********************/
/**************************************************************/
#include "cs.h"
#include "dsputil.h"
#include "pvread.h"
#include "soundio.h"
#include <math.h>
#include "oload.h"

/*RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int pvocex_loadfile(ENVIRON *, const char *fname, PVREAD *p);

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*ksmps)    /* manifest used for final time wdw */

void FetchInOne(
    float   *inp,       /* pointer to input data */
    MYFLT   *buf,       /* where to put our nice mag/pha pairs */
    long    fsize,      /* frame size we're working with */
    MYFLT   pos,        /* fractional frame we want */
    long    mybin)
{
    float   *frame0;
    float   *frame1;
    long    base;
    MYFLT   frac;
    long    twmybin = mybin+mybin; /* Always used thus */

    /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
    base = (long)pos;               /* index of basis frame of interpolation */
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frame0 = inp + ((long) fsize + 2L) * base + twmybin;
    frame1 = frame0 + ((long) fsize + 2L);      /* addresses of both frames */
    if (frac != 0.0) {          /* must have 2 cases to avoid poss seg vlns */
                                /* and failed computes, else may interp   */
                                /* bd valid data */
      buf[0] = frame0[0] + frac * (frame1[0] - frame0[0]);
      buf[1] = frame0[1] + frac * (frame1[1] - frame0[1]);
    }
    else {
        /* frac is 0.0 i.e. just copy the source frame */
      buf[0] = frame0[0];
      buf[1] = frame0[1];
    }
}

int pvreadset(ENVIRON *csound, PVREAD *p)
{
    char      pvfilnam[256];

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if (pvocex_loadfile(csound, pvfilnam, p) == OK) {
      p->prFlg = 1;
      p->mybin = MYFLT2LRND(*p->ibin);
      return OK;
    }
    return NOTOK;
}

int pvread(ENVIRON *csound, PVREAD *p)
{
    MYFLT  frIndx;
    MYFLT  *buf = (MYFLT*) p->fftBuf.auxp;
    int    size = pvfrsiz(p);

    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return csound->PerfError(csound, Str("PVOC timpnt < 0"));
    }
    if (frIndx > p->maxFr) {  /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        csound->Warning(csound, Str("PVOC ktimpnt truncated to last frame"));
      }
    }
    FetchInOne(p->frPtr, buf, size, frIndx, p->mybin);
    *p->kfreq = buf[1];
    *p->kamp = buf[0];
    return OK;
}

static int pvocex_loadfile(ENVIRON *csound, const char *fname, PVREAD *p)
{
    PVOCEX_MEMFILE  pp;

    if (PVOCEX_LoadFile(csound, fname, &pp) != 0) {
      csound->InitError(csound, Str("PVREAD cannot load %s"), fname);
      return NOTOK;
    }
    /* ignore the window spec until we can use it! */
    p->frSiz    = pp.fftsize;
    p->frPtr    = (float*) ((void*) pp.mfp->beginp);
    p->baseFr   = 0;  /* point to first data frame */
    p->maxFr    = pp.nframes - 1;
    p->asr      = pp.srate;
    /* highest possible frame index */
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr / ((MYFLT) pp.overlap);
    p->mfp = pp.mfp;
    return OK;
}

