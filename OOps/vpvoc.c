/*
    vpvoc.c:

    Copyright (C) 1992 Richard Karpen

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
/***********tableseg, tablexseg, voscili, vpvoc************/
/*** By Richard Karpen - July-October 1992************/
/************************************************************/

#include "csoundCore.h"
#include <math.h>
#include "dsputil.h"
#include "pvfileio.h"
#include "pstream.h"
#include "vpvoc.h"
#include "soundio.h"
#include "oload.h"

int tblesegset(CSOUND *csound, TABLESEG *p)
{
    TSEG    *segp;
    int     nsegs;
    MYFLT   **argp, dur;
    FUNC    *nxtfunc, *curfunc;
    long    flength;
    int     i;

    csound->tbladr = (void*) p;
    nsegs = (p->INCOUNT >> 1);  /* count segs & alloc if nec */

    if ((segp = (TSEG *) p->auxch.auxp) == NULL) {
        csound->AuxAlloc(csound, (long)(nsegs+1)*sizeof(TSEG), &p->auxch);
        p->cursegp = segp = (TSEG *) p->auxch.auxp;
        (segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if ((nxtfunc = csound->FTFind(csound, *argp++)) == NULL)
        return NOTOK;
    flength = nxtfunc->flen;
    p->outfunc = (FUNC *) mcalloc(csound,
                                  (long)sizeof(FUNC) + flength*sizeof(MYFLT));
    p->outfunc->flen = nxtfunc->flen;
    p->outfunc->lenmask = nxtfunc->lenmask;
    p->outfunc->lobits = nxtfunc->lobits;
    p->outfunc->lomask = nxtfunc->lomask;
    p->outfunc->lodiv = nxtfunc->lodiv;
    for (i=0; i<= flength; i++)
        *(p->outfunc->ftable + i) = FL(0.0);
    if (**argp <= 0.0)  return OK;         /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                      /* else proceed from 1st seg */
    segp--;
    do {
        segp++;                 /* init each seg ..  */
        curfunc = nxtfunc;
        dur = **argp++;
        if ((nxtfunc = csound->FTFind(csound, *argp++)) == NULL) return OK;
        if (dur > FL(0.0)) {
                segp->d = dur * csound->ekr;
                segp->function =  curfunc;
                segp->nxtfunction = nxtfunc;
                segp->cnt = (long) (segp->d + .5);
        }
        else break;             /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;
    segp->d = FL(0.0);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    return OK;
}

int ktableseg(CSOUND *csound, TABLESEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, durovercnt=FL(0.0);
    int         i;
    long        flength, upcnt;

    /* RWD fix */
    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("tableseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0)
      durovercnt = segp->d/upcnt;
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = segp->function->flen;
    for (i=0; i<flength; i++) {
      curval = *(curtab + i);
      nxtval = *(nxttab + i);
      if (durovercnt > 0.0)
        *(p->outfunc->ftable + i) = (curval + ((nxtval - curval) / durovercnt));
      else
        *(p->outfunc->ftable + i) = curval;
    }
    return OK;
}

int ktablexseg(CSOUND *csound, TABLESEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, cntoverdur=FL(0.0);
    int         i;
    long        flength, upcnt;

    /* RWD fix */
    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("tablexseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0) cntoverdur = upcnt/ segp->d;
    while(--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = segp->function->flen;
    for (i=0; i<flength; i++) {
      curval = *(curtab + i);
      nxtval = *(nxttab + i);
      *(p->outfunc->ftable + i) =
        (curval + ((nxtval - curval) * (cntoverdur*cntoverdur)));
    }
    return OK;
}

/************************************************************/
/*****************VPVOC**************************************/
/************************************************************/

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*csound->ksmps)    /* manifest used for final time wdw */

int vpvset(CSOUND *csound, VPVOC *p)
{
    int      i;
    char     pvfilnam[64];
    PVOCEX_MEMFILE  pp;
    int     frInc, chans, size; /* THESE SHOULD BE SAVED IN PVOC STRUCT */
                        /* If optional table given, fake it up -- JPff  */
    if (*p->isegtab == FL(0.0))
      p->tableseg = (TABLESEG*) csound->tbladr;
    else {
      csound->AuxAlloc(csound, sizeof(TABLESEG), &p->auxtab);
      p->tableseg = (TABLESEG*) p->auxtab.auxp;
      if ((p->tableseg->outfunc = csound->FTFind(csound, p->isegtab)) == NULL) {
        csound->InitError(csound,
                          Str("vpvoc: Could not find ifnmagctrl table %f"),
                          *p->isegtab);
        return NOTOK;
      }
    }
    if (p->tableseg == NULL)
      return csound->InitError(csound,
                               Str("vpvoc: associated tableseg not found"));

    if (p->auxch.auxp == NULL) {              /* if no buffers yet, alloc now */
      MYFLT *fltp;
      csound->AuxAlloc(csound,
                       (PVDATASIZE + PVFFTSIZE * 3 + PVWINLEN) * sizeof(MYFLT),
                       &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
      p->fftBuf = fltp;      fltp += PVFFTSIZE;
      p->dsBuf = fltp;       fltp += PVFFTSIZE;
      p->outBuf = fltp;      fltp += PVFFTSIZE;
      p->window = fltp;
    }
    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if (csound->PVOCEX_LoadFile(csound, pvfilnam, &pp) != 0)
      return csound->InitError(csound, Str("VPVOC cannot load %s"), pvfilnam);

    p->frSiz = pp.fftsize;
    frInc    = pp.overlap;
    chans    = pp.chans;
    p->asr   = pp.srate;
    if (p->asr != csound->esr) {                /* & chk the data */
      csound->Warning(csound, Str("%s's srate = %8.0f, orch's srate = %8.0f"),
                              pvfilnam, p->asr, csound->esr);
    }
    if (p->frSiz > PVFRAMSIZE) {
      return csound->InitError(csound,
                               Str("PVOC frame %ld bigger than %ld in %s"),
                               (long) p->frSiz, (long) PVFRAMSIZE, pvfilnam);
    }
    if (p->frSiz < 128) {
      return csound->InitError(csound,
                               Str("PVOC frame %ld seems too small in %s"),
                               (long) p->frSiz, pvfilnam);
    }
    if (chans != 1) {
      return csound->InitError(csound, Str("%d chans (not 1) in PVOC file %s"),
                                       (int) chans, pvfilnam);
    }
    /* Check that pv->frSiz is a power of two too ? */
    p->frPtr = (float*) pp.data;
    p->baseFr = 0;  /* point to first data frame */
    p->maxFr = pp.nframes - 1;
    /* highest possible frame index */
    p->frPktim = (MYFLT) csound->ksmps / (MYFLT) frInc;
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr / (MYFLT) frInc;
    /* factor by which to mulitply 'real' time index to get frame index */
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
    /* amplitude scale for PVOC */
 /* p->scale = (MYFLT) pp.fftsize * ((MYFLT) pp.fftsize / (MYFLT) pp.winsize);
  */
    p->scale = (MYFLT) pp.fftsize * FL(0.5);
    p->scale *= csound->GetInverseRealFFTScale(csound, pp.fftsize);
    /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
    /* 1/frSiz is the required scale down before (i)FFT */
    p->prFlg = 1;    /* true */
    p->opBpos = 0;
    p->lastPex = FL(1.0);   /* needs to know last pitchexp to update phase */
    /* Set up time window */
    for (i = 0; i < pvdasiz(p); ++i) {  /* or maybe pvdasiz(p) */
      p->lastPhase[i] = FL(0.0);
    }
    if ((OPWLEN / 2 + 1) > PVWINLEN) {
      return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                           "max is %d for pv %s"),
                                       csound->ksmps, (OPWLEN / 2 + 1),
                                       PVWINLEN, pvfilnam);
    }
    for (i = 0; i < OPWLEN / 2 + 1; ++i)    /* time window is OPWLEN long */
      p->window[i] = (MYFLT) (0.5 - 0.5 * cos(TWOPI*(double)i/(double)OPWLEN));
    /* NB: HANNING */
    for (i = 0; i < pvfrsiz(p); ++i)
      p->outBuf[i] = FL(0.0);
    MakeSinc();                         /* sinctab is same for all instances */

    return OK;
}

int vpvoc(CSOUND *csound, VPVOC *p)
{
    int       n;
    MYFLT     *ar = p->rslt;
    MYFLT     frIndx;
    MYFLT     *buf = p->fftBuf;
    MYFLT     *buf2 = p->dsBuf;
    int       asize = pvdasiz(p); /* fix */
    int       size = pvfrsiz(p);
    int       buf2Size, outlen;
    int       circBufSize = PVFFTSIZE;
    int       specwp = (int) *p->ispecwp;   /* spectral warping flag */
    MYFLT     pex, scaleFac = p->scale;
    TABLESEG  *q = p->tableseg;
    long      i, j;

    /* RWD fix */
    if (p->auxch.auxp == NULL) {
      return csound->PerfError(csound, Str("vpvoc: not initialised"));
    }

    pex = *p->kfmod;
    outlen = (int) (((MYFLT) size) / pex);
    /* use outlen to check window/krate/transpose combinations */
    if (outlen>PVFFTSIZE) { /* Maximum transposition down is one octave */
                            /* ..so we won't run into buf2Size problems */
      return csound->PerfError(csound, Str("PVOC transpose too low"));
    }
    if (outlen<2*csound->ksmps) {   /* minimum post-squeeze windowlength */
      return csound->PerfError(csound, Str("PVOC transpose too high"));
    }
    buf2Size = OPWLEN;     /* always window to same length after DS */
    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return csound->PerfError(csound, Str("PVOC timpnt < 0"));
    }
    if (frIndx > (MYFLT)p->maxFr) { /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        csound->Warning(csound, Str("PVOC ktimpnt truncated to last frame"));
      }
    }

    FetchIn(p->frPtr, buf, size, frIndx);

/**** Apply "spectral envelope" to magnitudes ********/
    if (pex > FL(1.0))
      scaleFac /= pex;
    for (i = 0, j = 0; i <= size; i += 2, j++)
      buf[i] *= *(q->outfunc->ftable + j) * scaleFac;
/***************************************************/

    FrqToPhase(buf, asize, pex * (MYFLT) csound->ksmps, p->asr,
               (MYFLT) (0.5 * ((pex / p->lastPex) - 1)));
    /* accumulate phase and wrap to range -PI to PI */
    RewrapPhase(buf, asize, p->lastPhase);

    if (specwp == 0 || (p->prFlg)++ == -(int)specwp) {
      /* ?screws up when prFlg used */
      /* specwp=0 => normal; specwp = -n => just nth frame */
      if (specwp < 0)
        csound->Message(csound, Str("PVOC debug: one frame gets through\n"));
      if (specwp > 0)
        PreWarpSpec(buf, asize, pex);

      Polar2Real_PVOC(csound, buf, size);

      if (pex != FL(1.0))
        UDSample(buf, (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)),
                 buf2, size, buf2Size, pex);
      else
        CopySamps(buf + (int) ((size - buf2Size) >> 1), buf2, buf2Size);
      if (specwp >= 0)
        ApplyHalfWin(buf2, p->window, buf2Size);
    }
    else {
      for (n = 0; n < buf2Size; ++n)
        buf2[n] = FL(0.0);
    }

    addToCircBuf(buf2, p->outBuf, p->opBpos, csound->ksmps, circBufSize);
    writeClrFromCircBuf(p->outBuf, ar, p->opBpos, csound->ksmps, circBufSize);
    p->opBpos += csound->ksmps;
    if (p->opBpos > circBufSize)
      p->opBpos -= circBufSize;
    addToCircBuf(buf2 + csound->ksmps, p->outBuf, p->opBpos,
                 buf2Size - csound->ksmps, circBufSize);
    p->lastPex = pex;        /* needs to know last pitchexp to update phase */

    return OK;
}

