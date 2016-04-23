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

#include "pvoc.h"         /*      UGENS8.C        */
#include <math.h>

/* RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int pvx_loadfile(CSOUND *, const char *, PVOC *);

/********************************************/
/* Originated by Dan Ellis, MIT             */
/* Spectral Extraction and Amplitude Gating */
/* added by Richard Karpen, University      */
/* of Washington, Seattle 1998              */
/********************************************/

#define WLN   1                         /* time window is WLN*2*ksmps long  */
#define OPWLEN (2*WLN*CS_KSMPS)    /* manifest used for final time wdw */

int pvset_(CSOUND *csound, PVOC *p, int stringname)
{
    unsigned int      i;
    int32    memsize;
    char     pvfilnam[MAXNAME];
    int      size;      /* THESE SHOULD BE SAVED IN PVOC STRUCT */
    FUNC     *AmpGateFunc = NULL;

    p->pp = PVOC_GetGlobals(csound);

     if (stringname==0){
      if (csound->ISSTRCOD(*p->ifilno))
        strncpy(pvfilnam,get_arg_string(csound, *p->ifilno), MAXNAME-1);
      else csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.",0);
    }
    else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);

    if (UNLIKELY(pvx_loadfile(csound, pvfilnam, p) != OK))
      return NOTOK;

    memsize = (int32) (PVDATASIZE + PVFFTSIZE * 3 + PVWINLEN);
    if (*p->imode == 1 || *p->imode == 2) {
      int32  n = (int32) ((p->frSiz + 2L) * (p->maxFr + 2L));
#ifdef USE_DOUBLE
      n = (n + 1L) * (int32) sizeof(float) / (int32) sizeof(double);
#endif
      memsize += n;
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
    p->frPktim = ((MYFLT)CS_KSMPS)/((MYFLT) p->frInc);
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = CS_ESR/((MYFLT) p->frInc);
    /* factor by which to mulitply 'real' time index to get frame index */
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
    /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
    /* 1/frSiz is the required scale down before (i)FFT */
    p->prFlg = 1;    /* true */
    p->opBpos = 0;
    p->lastPex = FL(1.0);     /* needs to know last pitchexp to update phase */
    /* Set up time window */
    memset(p->lastPhase, 0, sizeof(MYFLT)*pvdasiz(p));
    /* for (i=0; i < pvdasiz(p); ++i) {  /\* or maybe pvdasiz(p) *\/ */
    /*   p->lastPhase[i] = FL(0.0); */
    /* } */
    if (UNLIKELY((OPWLEN/2 + 1)>PVWINLEN )) {
      return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                           "max is %d for pv %s"),
                                       CS_KSMPS, (OPWLEN/2 + 1), PVWINLEN,
                                       pvfilnam);
    }

    if (*p->igatefun > 0)
      if (UNLIKELY((AmpGateFunc = csound->FTnp2Find(csound, p->igatefun)) == NULL))
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
      p->window[i] = (FL(0.5) - FL(0.5) * COS(TWOPI_F*(MYFLT)i/(MYFLT)OPWLEN));
    /* NB: HANNING */
    memset(p->outBuf, 0, sizeof(MYFLT)*pvfrsiz(p));
    /* for (i=0; i< pvfrsiz(p); ++i) */
    /*   p->outBuf[i] = FL(0.0); */
    MakeSinc(p->pp);                    /* sinctab is same for all instances */

    if (p->memenv.auxp == NULL || p->memenv.size < pvdasiz(p)*sizeof(MYFLT))
        csound->AuxAlloc(csound, pvdasiz(p) * sizeof(MYFLT), &p->memenv);

    return OK;
}

int pvset(CSOUND *csound, PVOC *p){
  return pvset_(csound,p,0);
}

int pvset_S(CSOUND *csound, PVOC *p){
  return pvset_(csound,p,1);
}


int pvoc(CSOUND *csound, PVOC *p)
{
    MYFLT  *ar = p->rslt;
    MYFLT  frIndx;
    MYFLT  *buf = p->fftBuf;
    MYFLT  *buf2 = p->dsBuf;
    int    asize = pvdasiz(p);  /* new */
    int    size = pvfrsiz(p);
    int    buf2Size, outlen;
    int    circBufSize = PVFFTSIZE;
    int    specwp = (int)*p->ispecwp;   /* spectral warping flag */
    MYFLT  pex, scaleFac;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;
    pex = *p->kfmod;
    outlen = (int) (((MYFLT) size) / pex);
    /* use outlen to check window/krate/transpose combinations */
    if (UNLIKELY(outlen>PVFFTSIZE))  /* Maximum transposition down is one octave */
      goto err2;           /* ..so we won't run into buf2Size problems */
    if (UNLIKELY(outlen<(int)(2*nsmps)))     /* minimum post-squeeze windowlength */
      goto err3;
    buf2Size = OPWLEN;       /* always window to same length after DS */
    if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err4;
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

    FrqToPhase(buf, asize, pex * (MYFLT) nsmps, p->asr,
               FL(0.5) * ((pex / p->lastPex) - FL(1.0)));
    /* accumulate phase and wrap to range -PI to PI */
    RewrapPhase(buf, asize, p->lastPhase);

    if (specwp > 0){
      /* RWD: THIS CAUSED MASSIVE MEMORY ERROR, BUT DOESN'T WORK ANYWAY */
        PreWarpSpec(buf, asize, pex, (MYFLT *)p->memenv.auxp);
     }

    Polar2Real_PVOC(csound, buf, size);

    if (pex != FL(1.0))
      UDSample(p->pp, buf, (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)),
               buf2, size, buf2Size, pex);
    else
      memcpy(buf2, buf + (int) ((size - buf2Size) >> 1),
             sizeof(MYFLT) * buf2Size);
    ApplyHalfWin(buf2, p->window, buf2Size);
    addToCircBuf(buf2, p->outBuf, p->opBpos, nsmps, circBufSize);
    writeClrFromCircBuf(p->outBuf, ar, p->opBpos, nsmps, circBufSize);
    p->opBpos += nsmps;
    if (p->opBpos > circBufSize)
      p->opBpos -= circBufSize;
    addToCircBuf(buf2 + nsmps, p->outBuf,
                 p->opBpos, buf2Size - nsmps, circBufSize);
    p->lastPex = pex;        /* needs to know last pitchexp to update phase */
    /* scale output */
    scaleFac = p->scale;
    if (pex > FL(1.0))
      scaleFac /= pex;
    if (UNLIKELY(offset)) memset(p->rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i < nsmps; i++)
      p->rslt[i] *= scaleFac;

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead, Str("pvoc: not initialised"));
 err2:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("PVOC transpose too low"));
 err3:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("PVOC transpose too high"));
 err4:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("PVOC timpnt < 0"));
}

/* RWD 8:2001: custom version of ldmemfile();
   enables pvfileio funcs to apply byte-reversal if needed.

  this version applies scaling to match  existing  pvanal format
 */
static int pvx_loadfile(CSOUND *csound, const char *fname, PVOC *p)
{
    PVOCEX_MEMFILE  pp;

    if (UNLIKELY(csound->PVOCEX_LoadFile(csound, fname, &pp) != 0)) {
      return csound->InitError(csound, Str("PVOC cannot load %s"), fname);
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    if (UNLIKELY(pp.fftsize > PVFRAMSIZE)) {
      return csound->InitError(csound, Str("pvoc-ex file %s: "
                                           "FFT size %d too large for Csound"),
                               fname, (int) pp.fftsize);
    }
    /* have to reject m/c files for now, until opcodes upgraded */
    if (UNLIKELY(pp.chans > 1)) {
      return csound->InitError(csound, Str("pvoc-ex file %s is not mono"), fname);
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
 /* p->scale = (MYFLT) pp.fftsize * ((MYFLT) pp.fftsize / (MYFLT) pp.winsize);
  */
    p->scale = (MYFLT) pp.fftsize * FL(0.5);
    p->scale *= csound->GetInverseRealFFTScale(csound, pp.fftsize);

    return OK;
}

