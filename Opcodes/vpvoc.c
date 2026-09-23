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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/**************************************************************/
/***********tableseg, tablexseg, voscili, vpvoc************/
/*** By Richard Karpen - July-October 1992************/
/************************************************************/

#include "pvoc.h"
#include <math.h>

int32_t tblesegset(CSOUND *csound, TABLESEG *p)
{
  TSEG *segp;
  cs_float **argp = p->argums;
  FUNC *first, *current, *next;
  int32_t i, nsegs = p->INOCOUNT >> 1;
  PVOC_GLOBALS *globals;

  p->cursegp = NULL;
  if (UNLIKELY(p->INOCOUNT < 3 || !(p->INOCOUNT & 1)))
    return csound->InitError(csound, "%s",
                             Str("incomplete number of input arguments"));

  first = current = csound->FTFind(csound, *argp++);
  if (UNLIKELY(first == NULL))
    return NOTOK;
  csound->AuxAlloc(csound, (size_t)(nsegs + 1) * sizeof(TSEG), &p->auxch);
  segp = (TSEG *)p->auxch.auxp;
  for (i = 0; i < nsegs; i++) {
    cs_double cycles = (cs_double)**argp++ * (cs_double)CS_EKR;
    if (UNLIKELY(!(cycles >= 0.0 && cycles < (INT32_MAX + 0.0))))
      return csound->InitError(csound, "%s",
                               Str("tableseg: invalid segment duration"));
    next = csound->FTFind(csound, *argp++);
    if (UNLIKELY(next == NULL))
      return NOTOK;
    if (UNLIKELY(next->flen != first->flen))
      return csound->InitError(csound, "%s",
                               Str("tableseg: tables must have the same size"));
    segp[i].function = current;
    segp[i].nxtfunction = next;
    segp[i].duration = segp[i].cnt = (int32_t)(cycles + 0.5);
    current = next;
  }
  /* The final table is held without a countdown. */
  segp[nsegs].function = segp[nsegs].nxtfunction = current;

  csound->AuxAlloc(csound, ((size_t)first->flen + 1) * sizeof(cs_float),
                   &p->outaux);
  p->outfunc = &p->outtable;
  p->outfunc->ftable = (cs_float *)p->outaux.auxp;
  p->outfunc->flen = first->flen;
  p->outfunc->lenmask = first->lenmask;
  p->outfunc->lobits = first->lobits;
  p->outfunc->lomask = first->lomask;
  p->outfunc->lodiv = first->lodiv;
  memcpy(p->outfunc->ftable, first->ftable,
         ((size_t)first->flen + 1) * sizeof(cs_float));
  p->cursegp = segp;
  p->nsegs = nsegs;
  globals = PVOC_GetGlobals(csound);
  if (UNLIKELY(globals == NULL))
    return NOTOK;
  globals->tbladr = p;
  return OK;
}

static int32_t tableseg_perf(CSOUND *csound, TABLESEG *p, int32_t quadratic)
{
  TSEG *segp = p->cursegp;
  cs_float fraction = FL(0.0), *curtab, *nxttab, *out;
  uint32_t i;

  if (UNLIKELY(segp == NULL))
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("tableseg: not initialised"));
  /* Resolve completed and zero-length stages before reading their tables. */
  while (p->nsegs > 0 && segp->cnt == 0) {
    segp++;
    p->nsegs--;
  }
  p->cursegp = segp;
  if (p->nsegs > 0) {
    fraction = (cs_float)(segp->duration - segp->cnt) / segp->duration;
    segp->cnt--;
  }
  if (quadratic)
    fraction *= fraction;
  curtab = segp->function->ftable;
  nxttab = segp->nxtfunction->ftable;
  out = p->outfunc->ftable;
  /* vpvoc also reads the guard point for the Nyquist bin. */
  for (i = 0; i <= p->outfunc->flen; i++)
    out[i] = curtab[i] + (nxttab[i] - curtab[i]) * fraction;
  return OK;
}

int32_t ktableseg(CSOUND *csound, TABLESEG *p)
{
  return tableseg_perf(csound, p, 0);
}

/* tablexseg has historically used a quadratic curve, although the manual
   calls it exponential. Keep that curve for compatibility with old scores. */
int32_t ktablexseg(CSOUND *csound, TABLESEG *p)
{
  return tableseg_perf(csound, p, 1);
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
    cs_float *fltp;
    csound->AuxAlloc(csound,
                     (PVDATASIZE + PVFFTSIZE * 3 + PVWINLEN) * sizeof(cs_float),
                     &p->auxch);
    fltp = (cs_float *) p->auxch.auxp;
    p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
    p->fftBuf = fltp;      fltp += PVFFTSIZE;
    p->dsBuf = fltp;       fltp += PVFFTSIZE;
    p->outBuf = fltp;      fltp += PVFFTSIZE;
    p->window = fltp;
  }
  if (stringname==0){
    if (IsStringCode(*p->ifilno))
      strncpy(pvfilnam,csound->GetArgString(csound, *p->ifilno), MAXNAME-1);
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
  if (UNLIKELY(p->tableseg->outfunc->flen < (uint32_t)p->frSiz / 2))
    return csound->InitError(csound, "%s",
                             Str("vpvoc: spectral envelope table is too short"));
  if (UNLIKELY(chans != 1)) {
    return csound->InitError(csound, Str("%d chans (not 1) in PVOC file %s"),
                             (int32_t) chans, pvfilnam);
  }
  /* Check that pv->frSiz is a power of two too ? */
  p->frPtr = (float*) pp.data;
  p->baseFr = 0;  /* point to first data frame */
  p->maxFr = pp.nframes - 1;
  /* highest possible frame index */
  p->frPktim = (cs_float) CS_KSMPS / (cs_float) frInc;
  /* factor by which to mult expand phase diffs (ratio of samp spacings) */
  p->frPrtim = CS_ESR / (cs_float) frInc;
  /* factor by which to mulitply 'real' time index to get frame index */
  /* amplitude scale for PVOC */
  /* p->scale = (cs_float) pp.fftsize * ((cs_float) pp.fftsize / (cs_float) pp.winsize);
   */
  p->scale = (cs_float) pp.fftsize * FL(0.5);
  p->scale *= csound->GetInverseRealFFTScale(csound, pp.fftsize);
  /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
  /* 1/frSiz is the required scale down before (i)FFT */
  p->prFlg = 1;    /* true */
  p->opBpos = 0;
  p->lastPex = FL(1.0);   /* needs to know last pitchexp to update phase */
  /* Set up time window */
  memset(p->lastPhase, 0, sizeof(cs_float)*pvdasiz(p));
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
    p->window[i] = (FL(0.5) - FL(0.5) * COS(TWOPI_F*(cs_float)i/(cs_float)OPWLEN));
  /* NB: HANNING */
  memset(p->outBuf, 0, sizeof(cs_float)*pvfrsiz(p));
  /* for (i = 0; i < pvfrsiz(p); ++i) */
  /*   p->outBuf[i] = FL(0.0); */
  MakeSinc(p->pp);                    /* sinctab is same for all instances */
  if (p->memenv.auxp == NULL || p->memenv.size < pvdasiz(p)*sizeof(cs_float))
    csound->AuxAlloc(csound, pvdasiz(p) * sizeof(cs_float), &p->memenv);

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
  cs_float     *ar = p->rslt;
  cs_float     frIndx;
  cs_float     *buf = p->fftBuf;
  cs_float     *buf2 = p->dsBuf;
  int32_t       asize = pvdasiz(p); /* fix */
  int32_t       size = pvfrsiz(p);
  int32_t       buf2Size, outlen;
  int32_t       circBufSize = PVFFTSIZE;
  int32_t       specwp = (int32_t) *p->ispecwp;   /* spectral warping flag */
  cs_float     pex, scaleFac = p->scale;
  TABLESEG  *q = p->tableseg;
  int32     i, j;

  /* RWD fix */
  if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;

  pex = *p->kfmod;
  outlen = (int32_t) (((cs_float) size) / pex);
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
  if (frIndx > (cs_float)p->maxFr) { /* not past last one */
    frIndx = (cs_float)p->maxFr;
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
    cs_float *ftable = q->outfunc->ftable;
    for (i = 0, j = 0; i <= size; i += 2, j++)
      buf[i] *= ftable[j] * scaleFac;
  }
  /***************************************************/

  FrqToPhase(buf, asize, pex * (cs_float) CS_KSMPS, p->asr,
             (cs_float) (0.5 * ((pex / p->lastPex) - 1)));
  /* accumulate phase and wrap to range -PI to PI */
  RewrapPhase(buf, asize, p->lastPhase);

  if (specwp == 0 || (p->prFlg)++ == -(int32_t)specwp) {
    /* ?screws up when prFlg used */
    /* specwp=0 => normal; specwp = -n => just nth frame */
    if (UNLIKELY(specwp < 0))
      csound->Warning(csound, "%s", Str("PVOC debug: one frame gets through\n"));
    if (specwp > 0)
      PreWarpSpec(buf, asize, pex, (cs_float *)p->memenv.auxp);

    Polar2Real_PVOC(csound, buf, p->setup);

    if (pex != FL(1.0))
      UDSample(p->pp, buf,
               (FL(0.5) * ((cs_float) size - pex * (cs_float) buf2Size)),
               buf2, size, buf2Size, pex);
    else
      memcpy(buf2, buf + (int32_t) ((size - buf2Size) >> 1),
             sizeof(cs_float) * buf2Size);
    if (specwp >= 0)
      ApplyHalfWin(buf2, p->window, buf2Size);
  }
  else {
    memset(buf2, 0, sizeof(cs_float)*buf2Size);
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
