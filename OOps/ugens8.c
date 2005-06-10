/*
    ugens8.c:

    Copyright (C) 1991, 1998, 2000 Dan Ellis, Richard Karpen, Richard Dobson

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

#include "cs.h"   /*      UGENS8.C        */
#include <math.h>
#include "dsputil.h"
#include "pvocext.h"
#include "ugens8.h"
#include "soundio.h"
#include "oload.h"

/* RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int pvx_loadfile(ENVIRON *, const char *, PVOC *);

/********************************************/
/* Originated by Dan Ellis, MIT             */
/* Spectral Extraction and Amplitude Gating */
/* added by Richard Karpen, University      */
/* of Washington, Seattle 1998              */
/********************************************/

#define WLN   1                         /* time window is WLN*2*ksmps long  */
#define OPWLEN (2*WLN*csound->ksmps)    /* manifest used for final time wdw */

int pvset(ENVIRON *csound, PVOC *p)
{
    int      i;
    long     memsize;
    char     pvfilnam[MAXNAME];
    int      size;      /* THESE SHOULD BE SAVED IN PVOC STRUCT */
    FUNC     *AmpGateFunc = NULL;

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if (pvx_loadfile(csound, pvfilnam, p) != OK)
      return NOTOK;

    memsize = (long) (PVDATASIZE + PVFFTSIZE * 3 + PVWINLEN);
    if (*p->imode == 1 || *p->imode == 2) {
#ifndef USE_DOUBLE
      memsize += (long) ((p->frSiz + 2L) * (p->maxFr + 2L));
#else
      memsize += (((long) ((p->frSiz + 2L) * (p->maxFr + 2L)) + 1L) / 2L);
#endif
    }

    if (p->auxch.auxp == NULL || memsize != p->mems) {
      MYFLT *fltp;
      csound->AuxAlloc(csound, (memsize * sizeof(MYFLT)), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
      p->fftBuf = fltp;      fltp += PVFFTSIZE;
      p->dsBuf = fltp;       fltp += PVFFTSIZE;
      p->outBuf = fltp;      fltp += PVFFTSIZE;
      p->window = fltp;
      if (*p->imode == 1 || *p->imode == 2) {
        fltp += PVWINLEN;
        p->pvcopy = (float*) ((void*) fltp);
      }
    }
    p->mems = memsize;
    p->frPktim = ((MYFLT)csound->ksmps)/((MYFLT) p->frInc);
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr/((MYFLT) p->frInc);
    /* factor by which to mulitply 'real' time index to get frame index */
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
    /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
    /* 1/frSiz is the required scale down before (i)FFT */
    p->prFlg = 1;    /* true */
    p->opBpos = 0;
    p->lastPex = FL(1.0);     /* needs to know last pitchexp to update phase */
    /* Set up time window */
    for (i=0; i < pvdasiz(p); ++i) {  /* or maybe pvdasiz(p) */
      p->lastPhase[i] = FL(0.0);
    }
    if ((OPWLEN/2 + 1)>PVWINLEN ) {
      return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                           "max is %d for pv %s"),
                                       csound->ksmps, (OPWLEN/2 + 1), PVWINLEN,
                                       pvfilnam);
    }

    if (*p->igatefun > 0)
      if ((AmpGateFunc = csound->FTFind(csound, p->igatefun)) == NULL)
        return NOTOK;
    p->AmpGateFunc = AmpGateFunc;

    if (*p->igatefun > 0)
      p->PvMaxAmp = PvocMaxAmp(p->frPtr, size, p->maxFr);

    if (*p->imode == 1 || *p->imode == 2) {
      SpectralExtract(p->frPtr, p->pvcopy, size, p->maxFr,
                      (int) *p->imode, *p->ifreqlim);
      p->frPtr = p->pvcopy;
    }

    for (i=0; i < OPWLEN / 2 + 1; ++i)  /* time window is OPWLEN long */
      p->window[i] = (MYFLT) (0.5 - 0.5 * cos(TWOPI*(double)i/(double)OPWLEN));
    /* NB: HANNING */
    for (i=0; i< pvfrsiz(p); ++i)
      p->outBuf[i] = FL(0.0);
    MakeSinc();                         /* sinctab is same for all instances */

    return OK;
}

int pvoc(ENVIRON *csound, PVOC *p)
{
    MYFLT  *ar = p->rslt;
    MYFLT  frIndx;
    MYFLT  *buf = p->fftBuf;
    MYFLT  *buf2 = p->dsBuf;
    int    asize = pvdasiz(p);  /* new */
    int    size = pvfrsiz(p);
    int    i, buf2Size, outlen;
    int    circBufSize = PVFFTSIZE;
    int    specwp = (int)*p->ispecwp;   /* spectral warping flag */
    MYFLT  pex, scaleFac;

    if (p->auxch.auxp == NULL) {
      return csound->PerfError(csound, Str("pvoc: not initialised"));
    }
    pex = *p->kfmod;
    outlen = (int) (((MYFLT) size) / pex);
    /* use outlen to check window/krate/transpose combinations */
    if (outlen>PVFFTSIZE) {  /* Maximum transposition down is one octave */
                             /* ..so we won't run into buf2Size problems */
      return csound->PerfError(csound, Str("PVOC transpose too low"));
    }
    if (outlen<2*csound->ksmps) {    /* minimum post-squeeze windowlength */
      return csound->PerfError(csound, Str("PVOC transpose too high"));
    }
    buf2Size = OPWLEN;       /* always window to same length after DS */
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
    FetchIn(p->frPtr, buf, size, frIndx);

    if (*p->igatefun > 0)
      PvAmpGate(buf,size, p->AmpGateFunc, p->PvMaxAmp);

    FrqToPhase(buf, asize, pex * (MYFLT) csound->ksmps, p->asr,
               FL(0.5) * ((pex / p->lastPex) - FL(1.0)));
    /* accumulate phase and wrap to range -PI to PI */
    RewrapPhase(buf, asize, p->lastPhase);

    if (specwp > 0)
      /* RWD: THIS CAUSED MASSIVE MEMORY ERROR, BUT DOESN'T WORK ANYWAY */
      PreWarpSpec(buf, asize, pex);

    /* convert from magnitude/phase format to real/imaginary */
    for (i = 0; i < size; i += 4) {
      MYFLT re, im;
      re = buf[i] * (MYFLT) cos(buf[i + 1]);
      im = buf[i] * (MYFLT) sin(buf[i + 1]);
      buf[i    ] = re;
      buf[i + 1] = im;
      /* Offset the phase to align centres of stretched windows, not starts */
      re = -(buf[i + 2] * (MYFLT) cos(buf[i + 3]));
      im = -(buf[i + 2] * (MYFLT) sin(buf[i + 3]));
      buf[i + 2] = re;
      buf[i + 3] = im;
    }
    /* kill spurious imag at dc & fs/2 */
    buf[1] = buf[i]; buf[i] = buf[i + 1] = FL(0.0);

    csound->InverseRealFFT(csound, buf, (int) size);
    if (pex != FL(1.0))
      UDSample(buf, (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)),
               buf2, size, buf2Size, pex);
    else
      CopySamps(buf + (int) ((size - buf2Size) >> 1), buf2, buf2Size);
    ApplyHalfWin(buf2, p->window, buf2Size);
    addToCircBuf(buf2, p->outBuf, p->opBpos, csound->ksmps, circBufSize);
    writeClrFromCircBuf(p->outBuf, ar, p->opBpos, csound->ksmps, circBufSize);
    p->opBpos += csound->ksmps;
    if (p->opBpos > circBufSize)
      p->opBpos -= circBufSize;
    addToCircBuf(buf2 + csound->ksmps, p->outBuf,
                 p->opBpos, buf2Size - csound->ksmps, circBufSize);
    p->lastPex = pex;        /* needs to know last pitchexp to update phase */
    /* scale output */
    scaleFac = p->scale;
    if (pex > FL(1.0))
      scaleFac /= pex;
    for (i = 0; i < csound->ksmps; i++)
      p->rslt[i] *= scaleFac;

    return OK;
}

/* RWD 8:2001: custom version of ldmemfile();
   enables pvfileio funcs to apply byte-reversal if needed.

  this version applies scaling to match  existing  pvanal format
 */
static int pvx_loadfile(ENVIRON *csound, const char *fname, PVOC *p)
{
    PVOCEX_MEMFILE  pp;

    if (PVOCEX_LoadFile(csound, fname, &pp) != 0) {
      csound->InitError(csound, Str("PVOC cannot load %s"), fname);
      return NOTOK;
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    if (pp.fftsize > PVFRAMSIZE) {
      csound->InitError(csound, Str("pvoc-ex file %s: "
                                    "FFT size %d too large for Csound"),
                                fname, (int) pp.fftsize);
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
    p->baseFr   = 0;  /* point to first data frame */
    /* highest possible frame index */
    p->maxFr    = pp.nframes - 1;
    p->frInc    = pp.overlap;
    p->chans    = pp.chans;
    p->asr      = pp.srate;
    /* amplitude scale for PVOC */
    p->scale = (MYFLT) pp.fftsize * ((MYFLT) pp.fftsize / (MYFLT) pp.winsize);
    p->scale *= csound->GetInverseRealFFTScale(csound, pp.fftsize);

    return OK;
}

