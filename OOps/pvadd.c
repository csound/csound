/*
    pvadd.c:

    Copyright (C) 1998 Richard Karpen

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

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/
/*    PVADD.C        */

#include "cs.h"
#include <math.h>
#include "dsputil.h"
#include "pvfileio.h"
#include "pstream.h"
#include "pvocext.h"
#include "pvadd.h"
#include "oload.h"

static int pvx_loadfile(ENVIRON *csound, const char *fname, PVADD *p);

/* This is used in pvadd instead of the Fetch() from dsputil.c */
void FetchInForAdd(float *inp, MYFLT *buf, long fsize,
                   MYFLT pos, int binoffset, int maxbin, int binincr)
{
    long    j;
    float   *frm0, *frm1;
    long    base;
    MYFLT   frac;

    base = (long)pos;
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frm0 = inp + ((long)fsize+2L)*base;
    frm1 = frm0 + ((long)fsize+2L);
    if (frac != FL(0.0)) {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frm0[2L*j] + frac*(frm1[2L*j]-frm0[2L*j]);
        buf[2L*j+1L] = frm0[2L*j+1L]
          + frac*(frm1[2L*j+1L]-frm0[2L*j+1L]);
      }
    }
    else {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frm0[2L*j];
        buf[2L*j+1] = frm0[2L*j+1L];
      }
    }
}

int pvaddset(ENVIRON *csound, PVADD *p)
{
    int      i, ibins;
    char     pvfilnam[MAXNAME];
    int      size;
    FUNC     *ftp = NULL, *AmpGateFunc = NULL;
    MYFLT    *oscphase;
    long     memsize;

   if (*p->ifn > FL(0.0))
     if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
       return NOTOK;
   p->ftp = ftp;

   if (*p->igatefun > FL(0.0))
     if ((AmpGateFunc = csound->FTFind(csound, p->igatefun)) == NULL)
       return NOTOK;
    p->AmpGateFunc = AmpGateFunc;

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if (pvx_loadfile(csound, pvfilnam, p) != OK)
      return NOTOK;

    memsize = (long) (MAXBINS + PVFFTSIZE + PVFFTSIZE);
    if (*p->imode == 1 || *p->imode == 2) {
      long  n = (long) ((p->frSiz + 2L) * (p->maxFr + 2L));
#ifdef USE_DOUBLE
      n = (n + 1L) * (long) sizeof(float) / (long) sizeof(double);
#endif
      memsize += n;
    }

    if (p->auxch.auxp == NULL || memsize != p->mems) {
      MYFLT *fltp;
      csound->AuxAlloc(csound, (memsize * sizeof(MYFLT)), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->oscphase = fltp;
      fltp += MAXBINS;
      p->buf = fltp;
      if (*p->imode == 1 || *p->imode == 2) {
        fltp += PVFFTSIZE * 2;
        p->pvcopy = (float*) ((void*) fltp);
      }
    }
    p->mems = memsize;

    size = pvfrsiz(p);
    p->prFlg = 1;    /* true */

   if (*p->igatefun > 0)
     p->PvMaxAmp = PvocMaxAmp(p->frPtr, size, p->maxFr);

   if (*p->imode == 1 || *p->imode == 2) {
     SpectralExtract(p->frPtr, p->pvcopy, size, p->maxFr,
                     (int) *p->imode, *p->ifreqlim);
     p->frPtr = (float*) p->pvcopy;
   }

    oscphase = p->oscphase;

    for (i=0; i < MAXBINS; i++)
      *oscphase++ = FL(0.0);

    ibins = (*p->ibins <= FL(0.0) ? (size / 2) : (int) *p->ibins);
    p->maxbin = ibins + (int) *p->ibinoffset;
    p->maxbin = (p->maxbin > (size / 2) ? (size / 2) : p->maxbin);

    return OK;
}

int pvadd(ENVIRON *csound, PVADD *p)
{
    MYFLT   *ar, *ftab;
    MYFLT   frIndx;
    int     size = pvfrsiz(p);
    int     i, binincr = (int) *p->ibinincr, nsmps = csound->ksmps;
    MYFLT   amp, frq, v1, fract, *oscphase;
    long    phase, incr;
    FUNC    *ftp;
    long    lobits;

    if (p->auxch.auxp == NULL)
      return csound->PerfError(csound, Str("pvadd: not initialised"));
    ftp = p->ftp;
    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0)
      return csound->PerfError(csound, Str("PVADD timpnt < 0"));

    if (frIndx > p->maxFr) { /* not past last one */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        csound->Warning(csound, Str("PVADD ktimpnt truncated to last frame"));
      }
    }
    FetchInForAdd(p->frPtr, p->buf, size, frIndx,
                  (int) *p->ibinoffset, p->maxbin, binincr);

    if (*p->igatefun > 0)
      PvAmpGate(p->buf, p->maxbin*2, p->AmpGateFunc, p->PvMaxAmp);

    ar = p->rslt;
    for (i = 0; i < nsmps; i++)
      *ar++ = FL(0.0);
    oscphase = p->oscphase;
    for (i = (int) *p->ibinoffset; i < p->maxbin; i += binincr) {
      lobits = ftp->lobits;
      nsmps = csound->ksmps;
      ar = p->rslt;
      phase = (long) *oscphase;
      frq = p->buf[i * 2 + 1] * *p->kfmod;
      if (p->buf[i * 2 + 1] == FL(0.0) || frq >= csound->esr * FL(0.5)) {
        incr = 0;               /* Hope then does not matter */
        amp = FL(0.0);
      }
      else {
        MYFLT tmp = frq * csound->sicvt;
        incr = (long) MYFLT2LONG(tmp);
        amp = p->buf[i * 2];
      }
      do {
        fract = PFRAC(phase);
        ftab = ftp->ftable + (phase >> lobits);
        v1 = *ftab++;
        *ar += (v1 + (*ftab - v1) * fract) * amp;
        ar++;
        phase += incr;
        phase &= PHMASK;
      } while (--nsmps);
      *oscphase = (MYFLT) phase;
      oscphase++;
    }
    return OK;
}

static int pvx_loadfile(ENVIRON *csound, const char *fname, PVADD *p)
{
    PVOCEX_MEMFILE  pp;

    if (PVOCEX_LoadFile(csound, fname, &pp) != 0) {
      csound->InitError(csound, Str("PVADD cannot load %s"), fname);
      return NOTOK;
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    if (pp.fftsize > PVFRAMSIZE) {
      csound->InitError(csound, Str("pvoc-ex file %s: "
                                    "FFT size %d too large for Csound"),
                                fname, (int) pp.fftsize);
      return NOTOK;
    }
    if (pp.fftsize < 128) {
      csound->InitError(csound, Str("PV frame %ld seems too small in %s"),
                                (long) pp.fftsize, fname);
      return NOTOK;
    }
    /* have to reject m/c files for now, until opcodes upgraded */
    if (pp.chans > 1) {
      csound->InitError(csound, Str("pvoc-ex file %s is not mono"), fname);
      return NOTOK;
    }
    /* ignore the window spec until we can use it! */
    p->frSiz    = pp.fftsize;
    p->frPtr    = (float*) pp.data;
    p->maxFr    = pp.nframes - 1;
    p->asr      = pp.srate;
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr / (MYFLT) pp.overlap;
    return OK;
}

