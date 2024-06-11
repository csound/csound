/*
  pvinterp.c:

  Copyright (C) 1996 Richard Karpen

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

/*****************************************/
/*********** PVINTERP, PVCROSS ***********/
/******** By Richard Karpen 1996 *********/
/*****************************************/

#include "pvoc.h"
#include <math.h>

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*CS_KSMPS)    /* manifest used for final time wdw */

/************************************************************/
/*************PVBUFREAD**************************************/
/************************************************************/

int32_t pvbufreadset_(CSOUND *csound, PVBUFREAD *p, int32_t stringname)
{
  char     pvfilnam[MAXNAME];
  PVOCEX_MEMFILE  pp;
  int32_t      frInc, chans; /* THESE SHOULD BE SAVED IN PVOC STRUCT */

  {
    PVOC_GLOBALS  *p_ = PVOC_GetGlobals(csound);
    p_->pvbufreadaddr = p;
  }

  if (p->auxch.auxp == NULL) {              /* if no buffers yet, alloc now */
    /* Assumes PVDATASIZE, PVFFTSIZE, PVWINLEN constant */
    MYFLT *fltp;
    csound->AuxAlloc(csound,
                     (PVDATASIZE + PVFFTSIZE * 3 + PVWINLEN) * sizeof(MYFLT),
                     &p->auxch);
    fltp = (MYFLT *) p->auxch.auxp;
    p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
    p->fftBuf = fltp;   /* fltp += PVFFTSIZE; */  /* Not needed */
  }

  if (stringname==0){
      if (IsStringCode(*p->ifilno))
        strncpy(pvfilnam,csound->GetString(csound, *p->ifilno), MAXNAME-1);
      else csound->StringArg2Name(csound, pvfilnam, p->ifilno, "pvoc.",0);
                }
  else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);

  if (UNLIKELY(csound->PVOCEX_LoadFile(csound, pvfilnam, &pp) != 0))
    return csound->InitError(csound, Str("PVBUFREAD cannot load %s"),
                             pvfilnam);

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
  p->frPtr = (float*) pp.data;
  p->maxFr = pp.nframes - 1;
  p->frPktim = (MYFLT) CS_KSMPS / (MYFLT) frInc;
  p->frPrtim = CS_ESR / (MYFLT) frInc;
  p->prFlg = 1;       /* true */
  /* amplitude scale for PVOC */
  /* p->scale = (MYFLT) pp.fftsize * ((MYFLT) pp.fftsize / (MYFLT) pp.winsize);
   */
  p->scale = (MYFLT) pp.fftsize * FL(0.5);
  p->scale *= csound->GetInverseRealFFTScale(csound, pp.fftsize);

  if (UNLIKELY((OPWLEN / 2 + 1) > PVWINLEN )) {
    return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                         "max is %d for pv %s"),
                             CS_KSMPS, (int32_t) (OPWLEN / 2 + 1),
                             (int32_t) PVWINLEN, pvfilnam);
  }

  return OK;
}

int32_t pvbufreadset(CSOUND *csound, PVBUFREAD *p){
  return pvbufreadset_(csound,p,0);
}

int32_t pvbufreadset_S(CSOUND *csound, PVBUFREAD *p){
  return pvbufreadset_(csound,p,1);
}

int32_t pvbufread(CSOUND *csound, PVBUFREAD *p)
{
  MYFLT  frIndx;
  MYFLT  *buf = p->fftBuf;
  int32_t    size = pvfrsiz(p);

  if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;        /* RWD fix */
  if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err2;
  if (frIndx > (MYFLT) p->maxFr) {    /* not past last one */
    frIndx = (MYFLT) p->maxFr;
    if (UNLIKELY(p->prFlg)) {
      p->prFlg = 0;   /* false */
      csound->Warning(csound, "%s", Str("PVOC ktimpnt truncated to last frame"));
    }
  }
  FetchIn(p->frPtr, buf, size, frIndx);
  p->buf = buf;

  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("pvbufread: not initialised"));
 err2:
  return csound->PerfError(csound, &(p->h), "%s", Str("PVOC timpnt < 0"));
}

/************************************************************/
/*************PVINTERP**************************************/
/************************************************************/
int32_t pvinterpset_(CSOUND *csound, PVINTERP *p, int32_t stringname)
{
  uint32_t i;
  char     pvfilnam[MAXNAME];
  PVOCEX_MEMFILE  pp;
  int32_t      frInc, chans; /* THESE SHOULD BE SAVED IN PVOC STRUCT */

  p->pp = PVOC_GetGlobals(csound);
  p->pvbufread = p->pp->pvbufreadaddr;
  if (UNLIKELY(p->pvbufread == NULL))
    return csound->InitError(csound,
                             "%s", Str("pvinterp: associated pvbufread not found"));

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
  else strncpy(pvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);;
  if (UNLIKELY(csound->PVOCEX_LoadFile(csound, pvfilnam, &pp) != 0))
    return csound->InitError(csound, Str("PVINTERP cannot load %s"),
                             pvfilnam);

  p->frSiz = pp.fftsize;
  frInc    = pp.overlap;
  chans    = pp.chans;
  p->asr   = pp.srate;
  if (UNLIKELY(p->asr != CS_ESR)) {                /* & chk the data */
    csound->Warning(csound, Str("%s's srate = %8.0f, orch's srate = %8.0f"),
                    pvfilnam, p->asr, CS_ESR);
  }
  if (UNLIKELY(p->frSiz != p->pvbufread->frSiz)) {
    return csound->InitError(csound,
                             Str("pvinterp: %s: frame size %d does not "
                                 "match pvbufread frame size %d\n"), pvfilnam,
                             (int32_t) p->frSiz, (int32_t) p->pvbufread->frSiz);
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
  p->lastPex = FL(1.0);    /* needs to know last pitchexp to update phase */
  /* Set up time window */
  memset(p->lastPhase,'\0', pvdasiz(p)*sizeof(MYFLT));
  /* for (i = 0; i < pvdasiz(p); ++i) {      /\* or maybe pvdasiz(p) *\/ */
  /*   p->lastPhase[i] = FL(0.0); */
  /* } */
  if (UNLIKELY((OPWLEN / 2 + 1) > PVWINLEN)) {
    return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                         "max is %d for pv %s"),
                             CS_KSMPS, (OPWLEN / 2 + 1),
                             PVWINLEN, pvfilnam);
  }
  for (i = 0; i < OPWLEN / 2 + 1; ++i)    /* time window is OPWLEN long */
    p->window[i] = (MYFLT) (0.5 - 0.5 * cos(TWOPI*(double)i/(double)OPWLEN));
  /* NB: HANNING */
  memset(p->outBuf, 0, pvfrsiz(p)*sizeof(MYFLT));
  /* for (i = 0; i< pvfrsiz(p); ++i) */
  /*   p->outBuf[i] = FL(0.0); */
  MakeSinc(p->pp);                    /* sinctab is same for all instances */
  p->setup = csound->RealFFTSetup(csound, pvfrsiz(p), FFT_INV);

  return OK;
}

int32_t pvinterpset(CSOUND *csound, PVINTERP *p){
  return pvinterpset_(csound,p,0);
}

int32_t pvinterpset_S(CSOUND *csound, PVINTERP *p){
  return pvinterpset_(csound,p,1);
}


int32_t pvinterp(CSOUND *csound, PVINTERP *p)
{
  MYFLT   *ar = p->rslt;
  MYFLT   frIndx;
  MYFLT   *buf = p->fftBuf;
  MYFLT   *buf2 = p->dsBuf;
  int32_t     asize = pvdasiz(p); /* fix */
  int32_t     size = pvfrsiz(p);
  int32_t     buf2Size, outlen;
  int32_t     circBufSize = PVFFTSIZE;
  MYFLT   pex, scaleFac = p->scale;
  PVBUFREAD *q = p->pvbufread;
  int32    i, j;

  if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;    /* RWD Fix */
  pex = *p->kfmod;
  outlen = (int32_t) (((MYFLT) size) / pex);
  /* use outlen to check window/krate/transpose combinations */
  if (UNLIKELY(outlen>PVFFTSIZE))  /* Maximum transposition down is one octave */
    /* ..so we won't run into buf2Size problems */
    goto err2;
  if (UNLIKELY(outlen<(int32_t)(2*CS_KSMPS)))
    goto err3;   /* minimum post-squeeze windowlength */
  buf2Size = OPWLEN;     /* always window to same length after DS */
  if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err4;
  if (frIndx > (MYFLT)p->maxFr) { /* not past last one */
    frIndx = (MYFLT)p->maxFr;
    if (UNLIKELY(p->prFlg)) {
      p->prFlg = 0;   /* false */
      csound->Warning(csound, "%s", Str("PVOC ktimpnt truncated to last frame"));
    }
  }
  FetchIn(p->frPtr, buf, size, frIndx);

  /* Here's where the interpolation happens ***********************/
  if (pex > FL(1.0))
    scaleFac /= pex;
  for (i = 0, j = 1; i <= size; i += 2, j += 2) {
    buf[i] = buf[i] * *p->kampscale2;
    q->buf[i] = q->buf[i] * *p->kampscale1;
    buf[j] = buf[j] * *p->kfreqscale2;
    q->buf[j] = q->buf[j] * *p->kfreqscale1;
    buf[i] = (buf[i] + ((q->buf[i] - buf[i]) * *p->kampinterp)) * scaleFac;
    buf[j] = (buf[j] + ((q->buf[j] - buf[j]) * *p->kfreqinterp));
  }
  /*******************************************************************/
  FrqToPhase(buf, asize, pex * (MYFLT) CS_KSMPS, p->asr,
             (MYFLT) (0.5 * ((pex / p->lastPex) - 1)));
  /* accumulate phase and wrap to range -PI to PI */
  RewrapPhase(buf, asize, p->lastPhase);

  Polar2Real_PVOC(csound, buf, p->setup);

  if (pex != FL(1.0))
    UDSample(p->pp, buf,
             (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)), buf2,
             size, buf2Size, pex);
  else
    memcpy(buf2, buf + (int32_t) ((size - buf2Size) >> 1),
           sizeof(MYFLT) * buf2Size);
  ApplyHalfWin(buf2, p->window, buf2Size);

  addToCircBuf(buf2, p->outBuf, p->opBpos, CS_KSMPS, circBufSize);
  writeClrFromCircBuf(p->outBuf, ar, p->opBpos, CS_KSMPS, circBufSize);
  p->opBpos += CS_KSMPS;
  if (UNLIKELY(p->opBpos > circBufSize))
    p->opBpos -= circBufSize;
  addToCircBuf(buf2 + CS_KSMPS, p->outBuf, p->opBpos,
               buf2Size - CS_KSMPS, circBufSize);
  p->lastPex = pex;        /* needs to know last pitchexp to update phase */

  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("pvinterp: not initialised"));
 err2:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("PVOC transpose too low"));
 err3:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("PVOC transpose too high"));
 err4:
  return csound->PerfError(csound, &(p->h), "%s", Str("PVOC timpnt < 0"));
}

/************************************************************/
/************* PVCROSS **************************************/
/************************************************************/
int32_t pvcrossset_(CSOUND *csound, PVCROSS *p, int32_t stringname)
{
  uint32_t i;
  char     pvfilnam[MAXNAME];
  PVOCEX_MEMFILE  pp;
  int32_t      frInc, chans; /* THESE SHOULD BE SAVED IN PVOC STRUCT */

  p->pp = PVOC_GetGlobals(csound);
  p->pvbufread = p->pp->pvbufreadaddr;
  if (UNLIKELY(p->pvbufread == NULL))
    return csound->InitError(csound,
                             "%s", Str("pvcross: associated pvbufread not found"));

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
    return csound->InitError(csound,  Str("PVCROSS cannot load %s"), pvfilnam);

  p->frSiz = pp.fftsize;
  frInc    = pp.overlap;
  chans    = pp.chans;
  p->asr   = pp.srate;
  if (UNLIKELY(p->asr != CS_ESR)) {                /* & chk the data */
    csound->Warning(csound, Str("%s's srate = %8.0f, orch's srate = %8.0f"),
                    pvfilnam, p->asr, CS_ESR);
  }
  if (UNLIKELY(p->frSiz != p->pvbufread->frSiz)) {
    return csound->InitError(csound,
                             Str("pvcross: %s: frame size %d does not "
                                 "match pvbufread frame size %d\n"), pvfilnam,
                             (int32_t) p->frSiz, (int32_t) p->pvbufread->frSiz);
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
  p->prFlg = 1;    /* true */
  p->opBpos = 0;
  p->lastPex = FL(1.0);   /* needs to know last pitchexp to update phase */
  /* Set up time window */
  memset(p->lastPhase, '\0', pvdasiz(p)*sizeof(MYFLT));
  /* for (i = 0; i < pvdasiz(p); ++i) {      /\* or maybe pvdasiz(p) *\/ */
  /*     p->lastPhase[i] = FL(0.0); */
  /* } */
  if (UNLIKELY((OPWLEN / 2 + 1) > PVWINLEN )) {
    return csound->InitError(csound, Str("ksmps of %d needs wdw of %d, "
                                         "max is %d for pv %s"),
                             CS_KSMPS, (OPWLEN / 2 + 1),
                             PVWINLEN, pvfilnam);
  }
  for (i = 0; i < OPWLEN / 2 + 1; ++i)    /* time window is OPWLEN long */
    p->window[i] = (MYFLT) (0.5 - 0.5 * cos(TWOPI*(double)i/(double)OPWLEN));
  /* NB: HANNING */
  memset(p->outBuf, 0, pvfrsiz(p)*sizeof(MYFLT));
  /* for (i = 0; i < pvfrsiz(p); ++i) */
  /*   p->outBuf[i] = FL(0.0); */
  MakeSinc(p->pp);                    /* sinctab is same for all instances */
  if (p->memenv.auxp == NULL || p->memenv.size < pvdasiz(p)*sizeof(MYFLT))
    csound->AuxAlloc(csound, pvdasiz(p) * sizeof(MYFLT), &p->memenv);

  p->setup = csound->RealFFTSetup(csound, pvfrsiz(p), FFT_INV);
  return OK;
}

int32_t pvcrossset(CSOUND *csound, PVCROSS *p){
  return pvcrossset_(csound,p,0);
}

int32_t pvcrossset_S(CSOUND *csound, PVCROSS *p) {
  return pvcrossset_(csound,p,1);
}

int32_t pvcross(CSOUND *csound, PVCROSS *p)
{
  MYFLT   *ar = p->rslt;
  MYFLT   frIndx;
  MYFLT   *buf = p->fftBuf;
  MYFLT   *buf2 = p->dsBuf;
  int32_t     asize = pvdasiz(p);         /* fix */
  int32_t     size = pvfrsiz(p);
  int32_t     buf2Size, outlen;
  int32_t     circBufSize = PVFFTSIZE;
  int32_t     specwp = (int32_t) *p->ispecwp; /* spectral warping flag */
  MYFLT   pex, scaleFac = p->scale;
  PVBUFREAD *q = p->pvbufread;
  int32   i, j;
  MYFLT   ampscale1 = *p->kampscale1;
  MYFLT   ampscale2 = *p->kampscale2;

  if (UNLIKELY(p->auxch.auxp == NULL)) goto err1;        /* RWD Fix */
  pex = *p->kfmod;
  outlen = (int32_t) (((MYFLT) size) / pex);
  /* use outlen to check window/krate/transpose combinations */
  if (UNLIKELY(outlen>PVFFTSIZE)) /* Maximum transposition down is one octave */
    /* ..so we won't run into buf2Size problems */
    goto err2;
  if (UNLIKELY(outlen<(int32_t)(2*CS_KSMPS)))
    goto err3;               /* minimum post-squeeze windowlength */
  buf2Size = OPWLEN;     /* always window to same length after DS */
  if (UNLIKELY((frIndx = *p->ktimpnt * p->frPrtim) < 0)) goto err4;
  if (frIndx > (MYFLT) p->maxFr) {    /* not past last one */
    frIndx = (MYFLT) p->maxFr;
    if (p->prFlg) {
      p->prFlg = 0;   /* false */
      csound->Warning(csound, "%s", Str("PVOC ktimpnt truncated to last frame"));
    }
  }

  FetchIn(p->frPtr, buf, size, frIndx);

  /**** Apply amplitudes from pvbufread ********/
  if (pex > FL(1.0))
    scaleFac /= pex;
  for (i = 0, j = 0; i <= size; i += 2, j++)
    buf[i] = ((buf[i] * ampscale2) + (q->buf[i] * ampscale1)) * scaleFac;
  /***************************************************/

  FrqToPhase(buf, asize, pex * (MYFLT) CS_KSMPS, p->asr,
             (MYFLT) (0.5 * ((pex / p->lastPex) - 1)));
  /* accumulate phase and wrap to range -PI to PI */
  RewrapPhase(buf, asize, p->lastPhase);

  //if (specwp == 0 || (p->prFlg)++ == -(int32_t) specwp) {
  /* ?screws up when prFlg used */
  /* specwp=0 => normal; specwp = -n => just nth frame */

  // #ifdef BETA
  //  if (specwp < 0)
  //    csound->Message(csound, "%s", Str("PVOC debug: one frame gets through\n"));
  // #endif

  /* Since the code just above is plain wrong, I am assuming
     p->prFlg is only used to check if we are at the last frame
     and we are trying to warp the spectrum.
     Therefore I have adjusted the code accordingly
  */
  if(specwp == 0 || ((p->prFlg)++ != 0 && (int32_t) specwp)) {
    if (specwp > 0)
      PreWarpSpec(buf, asize, pex, (MYFLT *)p->memenv.auxp);

    Polar2Real_PVOC(csound, buf, p->setup);

    if (pex != FL(1.0))
      UDSample(p->pp, buf,
               (FL(0.5) * ((MYFLT) size - pex * (MYFLT) buf2Size)), buf2,
               size, buf2Size, pex);
    else
      memcpy(buf2, buf + (int32_t) ((size - buf2Size) >> 1),
             sizeof(MYFLT) * buf2Size);
    if (specwp >= 0)
      ApplyHalfWin(buf2, p->window, buf2Size);
  }
  else {
    memset(buf2, 0, buf2Size*sizeof(MYFLT));
  }

  addToCircBuf(buf2, p->outBuf, p->opBpos, CS_KSMPS, circBufSize);
  writeClrFromCircBuf(p->outBuf, ar, p->opBpos, CS_KSMPS, circBufSize);
  p->opBpos += CS_KSMPS;
  if (UNLIKELY(p->opBpos > circBufSize))
    p->opBpos -= circBufSize;
  addToCircBuf(buf2 + CS_KSMPS, p->outBuf, p->opBpos,
               buf2Size - CS_KSMPS, circBufSize);
  p->lastPex = pex;       /* needs to know last pitchexp to update phase */

  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("pvcross: not initialised"));
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
