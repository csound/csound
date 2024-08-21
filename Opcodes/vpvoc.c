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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

/**************************************************************/
/***********tableseg, tablexseg, voscili, vpvoc************/
/*** By Richard Karpen - July-October 1992************/
/************************************************************/

#include "pvoc.h"
#include <math.h>

int32_t tblesegset(CSOUND *csound, TABLESEG *p)
{
  TSEG    *segp;
  int32_t     nsegs;
  MYFLT   **argp, dur;
  FUNC    *nxtfunc, *curfunc;
  int32    flength;

  if (UNLIKELY(!(p->INCOUNT & 1))) {
    return csound->InitError(csound, "%s",
                             Str("incomplete number of input arguments"));
  }

  {
    PVOC_GLOBALS  *p_ = PVOC_GetGlobals(csound);
    p_->tbladr = p;
  }

  nsegs = (p->INCOUNT >> 1);  /* count segs & alloc if nec */

  if ((segp = (TSEG *) p->auxch.auxp) == NULL ||
      p->auxch.size<(nsegs+1)*sizeof(TSEG)) {
    csound->AuxAlloc(csound, (size_t)(nsegs+1)*sizeof(TSEG), &p->auxch);
    p->cursegp = segp = (TSEG *) p->auxch.auxp;
    (segp+nsegs)->cnt = MAXPOS;
  }
  argp = p->argums;
  if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
    return NOTOK;
  flength = nxtfunc->flen;
  p->outfunc =
    (FUNC*) csound->Calloc(csound, sizeof(FUNC));
  p->outfunc->ftable =
    (MYFLT*)csound->Calloc(csound, (1 + flength) * sizeof(MYFLT));
  p->outfunc->flen = nxtfunc->flen;
  p->outfunc->lenmask = nxtfunc->lenmask;
  p->outfunc->lobits = nxtfunc->lobits;
  p->outfunc->lomask = nxtfunc->lomask;
  p->outfunc->lodiv = nxtfunc->lodiv;
  //memset(p->outfunc->ftable, 0, sizeof(MYFLT)*(flength+1)); not needed -- calloc
  if (**argp <= 0.0)  return OK;         /* if idur1 <= 0, skip init  */
  p->cursegp = segp;                      /* else proceed from 1st seg */
  segp--;
  do {
    segp++;                 /* init each seg ..  */
    curfunc = nxtfunc;
    dur = **argp++;
    if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
      return OK;
    if (LIKELY(dur > FL(0.0))) {
      segp->d = dur * CS_EKR;
      segp->function =  curfunc;
      segp->nxtfunction = nxtfunc;
      segp->cnt = (int32) (segp->d + FL(0.5));
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

int32_t ktableseg(CSOUND *csound, TABLESEG *p)
{
  TSEG        *segp;
  MYFLT       *curtab, *nxttab,curval, nxtval, durovercnt=FL(0.0);
  int32_t         i;
  int32        flength, upcnt;

  /* RWD fix */
  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
  segp = p->cursegp;
  curtab = segp->function->ftable;
  nxttab = segp->nxtfunction->ftable;
  upcnt = (int32)segp->d-segp->cnt;
  if (upcnt > 0)
    durovercnt = segp->d/upcnt;
  while (--segp->cnt < 0)
    p->cursegp = ++segp;
  flength = segp->function->flen;
  for (i=0; i<flength; i++) {
    curval = curtab[i];
    nxtval = nxttab[i];
    if (durovercnt > FL(0.0))
      p->outfunc->ftable[i] = (curval + ((nxtval - curval) / durovercnt));
    else
      p->outfunc->ftable[i] = curval;
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h), "%s",
                           Str("tableseg: not initialised"));
}

int32_t ktablexseg(CSOUND *csound, TABLESEG *p)
{
  TSEG        *segp;
  MYFLT       *curtab, *nxttab,curval, nxtval, cntoverdur=FL(0.0);
  int32_t         i;
  int32        flength, upcnt;

  /* RWD fix */
  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
  segp = p->cursegp;
  curtab = segp->function->ftable;
  nxttab = segp->nxtfunction->ftable;
  upcnt = (int32)segp->d-segp->cnt;
  if (upcnt > 0) cntoverdur = upcnt/ segp->d;
  while(--segp->cnt < 0)
    p->cursegp = ++segp;
  flength = segp->function->flen;
  for (i=0; i<flength; i++) {
    curval = curtab[i];
    nxtval = nxttab[i];
    p->outfunc->ftable[i] =
      (curval + ((nxtval - curval) * (cntoverdur*cntoverdur)));
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h), "%s",
                           Str("tablexseg: not initialised"));
}

/************************************************************/
/*****************VPVOC**************************************/
/************************************************************/

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*CS_KSMPS)    /* manifest used for final time wdw */

int32_t vpvset_(CSOUND *csound, VPVOC *p, int32_t stringname)
{
  uint32_t i;
  char     pvfilnam[MAXNAME];
  PVOCEX_MEMFILE  pp;
  int32_t  frInc, chans; /* THESE SHOULD BE SAVED IN PVOC STRUCT */

  p->pp = PVOC_GetGlobals(csound);
  /* If optional table given, fake it up -- JPff  */
  if (*p->isegtab == FL(0.0))
    p->tableseg = p->pp->tbladr;
  else {
    csound->AuxAlloc(csound, sizeof(TABLESEG), &p->auxtab);
    p->tableseg = (TABLESEG*) p->auxtab.auxp;
    if (UNLIKELY((p->tableseg->outfunc =
                  csound->FTFind(csound, p->isegtab)) == NULL)) {
      return csound->InitError(csound, "%s%f",
                               Str("vpvoc: Could not find ifnmagctrl table "),
                               *p->isegtab);
    }
  }
  if (UNLIKELY(p->tableseg == NULL))
    return csound->InitError(csound, "%s",
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
  if (stringname==0){
    if (IsStringCode(*p->ifilno))
      strncpy(pvfilnam,csound->GetString(csound, *p->ifilno), MAXNAME-1);
    else csound->StringArg2Name(csound, pvfilnam, p->ifilno, "pvoc.",0);
  }
  else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);

  if (UNLIKELY(csound->PVOCEX_LoadFile(csound, pvfilnam, &pp) != 0))
    return csound->InitError(csound, Str("VPVOC cannot load %s"), pvfilnam);

  p->frSiz = pp.fftsize;
  frInc    = pp.overlap;
  chans    = pp.chans;
  p->asr   = pp.srate;
  if (UNLIKELY(p->asr != CS_ESR)) {                /* & chk the data */
    csound->Warning(csound, Str("%s's srate = %8.0f, orch's srate = %8.0f"),
                    pvfilnam, p->asr, CS_ESR);
  }
  if (UNLIKELY(p->frSiz > PVFRAMSIZE)) {
    return csound->InitError(csound,
                             Str("PVOC frame %ld bigger than %ld in %s"),
                             (long) p->frSiz, (long) PVFRAMSIZE, pvfilnam);
  }
  if (UNLIKELY(p->frSiz < 128)) {
    return csound->InitError(csound,
                             Str("PVOC frame %ld seems too small in %s"),
                             (long) p->frSiz, pvfilnam);
  }
  if (UNLIKELY(chans != 1)) {
    return csound->InitError(csound, Str("%d chans (not 1) in PVOC file %s"),
                             (int32_t) chans, pvfilnam);
  }
  /* Check that pv->frSiz is a power of two too ? */
  p->frPtr = (float*) pp.data;
  p->baseFr = 0;  /* point to first data frame */
  p->maxFr = pp.nframes - 1;
  /* highest possible frame index */
  p->frPktim = (MYFLT) CS_KSMPS / (MYFLT) frInc;
  /* factor by which to mult expand phase diffs (ratio of samp spacings) */
  p->frPrtim = CS_ESR / (MYFLT) frInc;
  /* factor by which to mulitply 'real' time index to get frame index */
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
  memset(p->lastPhase, 0, sizeof(MYFLT)*pvdasiz(p));
  /* for (i = 0; i < pvdasiz(p); ++i) {  /\* or maybe pvdasiz(p) *\/ */
  /*   p->lastPhase[i] = FL(0.0); */
  /* } */
  if (UNLIKELY((OPWLEN / 2 + 1) > PVWINLEN)) {
    return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                         "max is %d for pv %s"),
                             CS_KSMPS, (OPWLEN / 2 + 1),
                             PVWINLEN, pvfilnam);
  }
  for (i = 0; i < OPWLEN / 2 + 1; ++i)    /* time window is OPWLEN long */
    p->window[i] = (FL(0.5) - FL(0.5) * COS(TWOPI_F*(MYFLT)i/(MYFLT)OPWLEN));
  /* NB: HANNING */
  memset(p->outBuf, 0, sizeof(MYFLT)*pvfrsiz(p));
  /* for (i = 0; i < pvfrsiz(p); ++i) */
  /*   p->outBuf[i] = FL(0.0); */
  MakeSinc(p->pp);                    /* sinctab is same for all instances */
  if (p->memenv.auxp == NULL || p->memenv.size < pvdasiz(p)*sizeof(MYFLT))
    csound->AuxAlloc(csound, pvdasiz(p) * sizeof(MYFLT), &p->memenv);

  p->setup = csound->RealFFTSetup(csound, pvfrsiz(p), FFT_INV);
  return OK;
}

int32_t vpvset(CSOUND *csound, VPVOC *p){
  return vpvset_(csound,p,0);
}

int32_t vpvset_S(CSOUND *csound, VPVOC *p){
  return vpvset_(csound,p,1);
}

int32_t vpvoc(CSOUND *csound, VPVOC *p)
{
  MYFLT     *ar = p->rslt;
  MYFLT     frIndx;
  MYFLT     *buf = p->fftBuf;
  MYFLT     *buf2 = p->dsBuf;
  int32_t       asize = pvdasiz(p); /* fix */
  int32_t       size = pvfrsiz(p);
  int32_t       buf2Size, outlen;
  int32_t       circBufSize = PVFFTSIZE;
  int32_t       specwp = (int32_t) *p->ispecwp;   /* spectral warping flag */
  MYFLT     pex, scaleFac = p->scale;
  TABLESEG  *q = p->tableseg;
  int32     i, j;

  /* RWD fix */
  if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;

  pex = *p->kfmod;
  outlen = (int32_t) (((MYFLT) size) / pex);
  /* use outlen to check window/krate/transpose combinations */
  if (UNLIKELY(outlen>PVFFTSIZE)) { /* Maximum transposition down is one octave */
    /* ..so we won't run into buf2Size problems */
    goto err2;
  }
  if (UNLIKELY(outlen<(int32_t)(2*CS_KSMPS))) {
    /* minimum post-squeeze windowlength */
    goto err3;
  }
  buf2Size = OPWLEN;     /* always window to same length after DS */
  if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) {
    goto err4;
  }
  if (frIndx > (MYFLT)p->maxFr) { /* not past last one */
    frIndx = (MYFLT)p->maxFr;
    if (UNLIKELY(p->prFlg)) {
      p->prFlg = 0;   /* false */
      csound->Warning(csound, "%s", Str("PVOC ktimpnt truncated to last frame"));
    }
  }

  FetchIn(p->frPtr, buf, size, frIndx);

  /**** Apply "spectral envelope" to magnitudes ********/
  if (pex > FL(1.0))
    scaleFac /= pex;
  {
    MYFLT *ftable = q->outfunc->ftable;
    for (i = 0, j = 0; i <= size; i += 2, j++)
      buf[i] *= ftable[j] * scaleFac;
  }
  /***************************************************/

  FrqToPhase(buf, asize, pex * (MYFLT) CS_KSMPS, p->asr,
             (MYFLT) (0.5 * ((pex / p->lastPex) - 1)));
  /* accumulate phase and wrap to range -PI to PI */
  RewrapPhase(buf, asize, p->lastPhase);

  if (specwp == 0 || (p->prFlg)++ == -(int32_t)specwp) {
    /* ?screws up when prFlg used */
    /* specwp=0 => normal; specwp = -n => just nth frame */
    if (UNLIKELY(specwp < 0))
      csound->Warning(csound, "%s", Str("PVOC debug: one frame gets through\n"));
    if (specwp > 0)
      PreWarpSpec(buf, asize, pex, (MYFLT *)p->memenv.auxp);

    Polar2Real_PVOC(csound, buf, p->setup);

    if (pex != FL(1.0))
      UDSample(p->pp, buf,
               (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)),
               buf2, size, buf2Size, pex);
    else
      memcpy(buf2, buf + (int32_t) ((size - buf2Size) >> 1),
             sizeof(MYFLT) * buf2Size);
    if (specwp >= 0)
      ApplyHalfWin(buf2, p->window, buf2Size);
  }
  else {
    memset(buf2, 0, sizeof(MYFLT)*buf2Size);
    /* for (n = 0; n < buf2Size; ++n) */
    /*   buf2[n] = FL(0.0); */
  }

  addToCircBuf(buf2, p->outBuf, p->opBpos, CS_KSMPS, circBufSize);
  writeClrFromCircBuf(p->outBuf, ar, p->opBpos, CS_KSMPS, circBufSize);
  p->opBpos += CS_KSMPS;
  if (p->opBpos > circBufSize)
    p->opBpos -= circBufSize;
  addToCircBuf(buf2 + CS_KSMPS, p->outBuf, p->opBpos,
               buf2Size - CS_KSMPS, circBufSize);
  p->lastPex = pex;        /* needs to know last pitchexp to update phase */

  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("vpvoc: not initialised"));
 err2:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("PVOC transpose too low"));
 err3:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("PVOC transpose too high"));
 err4:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("PVOC timpnt < 0"));
}

