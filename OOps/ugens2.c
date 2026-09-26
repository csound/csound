/*
  ugens2.c:

  Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado

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

#include "csoundCore.h" /*                              UGENS2.C        */
#include "ugens2.h"
#include <math.h>

/* Macro form of Istvan's speedup ; constant should be 3fefffffffffffff */
/* #define FLOOR(x) (x >= FL(0.0) ? (int64_t)x                          */
/*                                  : (int64_t)((double)x - 0.999999999999999))
 */
/* 1.0-1e-8 is safe for a maximum table length of 16777216 */
/* 1.0-1e-15 could incorrectly round down large negative integers, */
/* because doubles do not have sufficient resolution for numbers like */
/* -1000.999999999999999 (FLOOR(-1000) might possibly be -1001 which is wrong)*/
/* it should be noted, though, that the above incorrect result would not be */
/* a problem in the case of interpolating table opcodes, as the fractional */
/* part would then be exactly 1.0, still giving a correct output value */
#define MYFLOOR(x) (x >= FL(0.0) ? (int32_t)x : (int32_t)((cs_double)x - 0.99999999))



int32_t phsset(CSOUND *csound, PHSOR *p)
{
  cs_float       phs;
  int32_t  longphs;
  if ((phs = *p->iphs) >= FL(0.0)) {
    if (UNLIKELY((longphs = (int32_t)phs))) {
      csound->Warning(csound, Str("init phase truncation\n"));
    }
    p->curphs = phs - (cs_float)longphs;
  }
  return OK;
}

int32_t ephsset(CSOUND *csound, EPHSOR *p)
{
  cs_double phs = (cs_double)*p->iphs;
  if (UNLIKELY(!isfinite(phs)))
    return csound->InitError(csound, "%s", Str("ephasor: initial phase must be finite"));
  if (phs >= 0.0) {
    if (UNLIKELY(phs >= 1.0)) {
      csound->Warning(csound, Str("init phase truncation\n"));
      phs -= floor(phs);
    }
    p->curphs = phs;
  }
  p->b = 1.0;
  return OK;
}

/* Remove whole cycles before addition so large increments retain the phase.
   Keep the wrap event: it also resets the exponential output. */
#define EPHASOR_INCREMENT(incr, wrapped) do {                         \
    (wrapped) = (incr) >= 1.0 || (incr) <= -1.0;                       \
    if (UNLIKELY(wrapped)) (incr) -= trunc(incr);                       \
  } while (0)

/* A double phase just below one may round to one in a float build. */
#define EPHASOR_OUTPUT(phase)                                        \
  ((cs_float)(phase) < FL(1.0) ? (cs_float)(phase) : FL(0.0))

int32_t ephsor(CSOUND *csound, EPHSOR *p)
{
    cs_double      phase;
    uint32_t    offset = GetKsmpsOffset(&p->h);
    uint32_t    early  = GetEarlySmps(&p->h);
    uint32_t    n, nsmps = CS_KSMPS;
    cs_float       *rs, *aphs, onedsr = CS_ONEDSR;
    cs_double      b = p->b;
    cs_double      incr, R = *p->kR;
    int32_t     whole_cycle;

  rs = p->sr;
  aphs = p->aphs;
  if (UNLIKELY(offset)) {
    memset(rs, '\0', offset*sizeof(cs_float));
    memset(aphs, '\0', offset*sizeof(cs_float));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
    memset(&aphs[nsmps], '\0', early*sizeof(cs_float));
  }
  phase = p->curphs;
  if (IS_ASIG_ARG(p->xcps)) {
    cs_float *cps = p->xcps;
    for (n=offset; n<nsmps; n++) {
      incr = (cs_double)(cps[n] * onedsr);
      EPHASOR_INCREMENT(incr, whole_cycle);
      rs[n] = (cs_float) b;
      aphs[n] = EPHASOR_OUTPUT(phase);
      phase += incr;
      b *= R;
      if (UNLIKELY(phase >= 1.0)) {
        phase -= 1.0;
        b = pow(R, 1.0+phase);
      }
      else if (UNLIKELY(phase < 0.0)) {
        phase += 1.0;
        b = pow(R, 1.0+phase);
      }
      else if (UNLIKELY(whole_cycle))
        b = pow(R, 1.0+phase);
    }
  }
  else {
    incr = (cs_double)(*p->xcps * onedsr);
    EPHASOR_INCREMENT(incr, whole_cycle);
    for (n=offset; n<nsmps; n++) {
      rs[n] = (cs_float) b;
      aphs[n] = EPHASOR_OUTPUT(phase);
      phase += incr;
      b *= R;
      if (UNLIKELY(phase >= 1.0)) {
        phase -= 1.0;
        b =  pow(R, 1.0+phase);
      }
      else if (UNLIKELY(phase < 0.0)) {
        phase += 1.0;
        b = pow(R, 1.0+phase);
      }
      else if (UNLIKELY(whole_cycle))
        b = pow(R, 1.0+phase);
    }
  }
  p->curphs = phase;
  p->b = b;
  return OK;
}

/* A phase just below one can round to one in cs_float. Wrap the output,
   retaining the more precise internal phase for the next sample. */
#define PHASOR_OUTPUT(phase)                                      \
  ((cs_float)(phase) == FL(1.0) ? FL(0.0) : (cs_float)(phase))

int32_t kphsor(CSOUND *csound, PHSOR *p)
{
  IGN(csound);
  cs_double      phs;
  phs = p->curphs;
  *p->sr = PHASOR_OUTPUT(phs);
  if (UNLIKELY((phs += (cs_double)*p->xcps * CS_ONEDKR) >= 1.0))
    phs -= 1.0;
  else if (UNLIKELY(phs < 0.0))
    phs += 1.0;
  p->curphs = phs;
  return OK;
}

int32_t phsor(CSOUND *csound, PHSOR *p)
{

    cs_double      phase;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    cs_float       *rs, onedsr = CS_ONEDSR;
    cs_double      incr;


  rs = p->sr;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  phase = p->curphs;
  if (IS_ASIG_ARG(p->xcps)) {
    cs_float *cps = p->xcps;
    for (n=offset; n<nsmps; n++) {
      incr = (cs_double)(cps[n] * onedsr);
      rs[n] = PHASOR_OUTPUT(phase);
      phase += incr;
      if (UNLIKELY(phase >= 1.0))
        phase -= 1.0;
      else if (UNLIKELY(phase < 0.0))
        phase += 1.0;
    }
  }
  else {
    incr = (cs_double)(*p->xcps * onedsr);
    for (n=offset; n<nsmps; n++) {
      rs[n] = PHASOR_OUTPUT(phase);
      phase += incr;
      if (UNLIKELY(phase >= 1.0)) {
        phase -= 1.0;
      }
      else if (UNLIKELY(phase < 0.0))
        phase += 1.0;
    }
  }
  p->curphs = phase;
  return OK;
}


int32_t ko1set(CSOUND *csound, OSCIL1 *p)
{
  FUNC        *ftp;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
    return NOTOK;
  if (UNLIKELY(!isfinite(*p->idur)))
    return csound->InitError(csound, "%s",
                            Str("oscil1: duration must be finite"));

  p->ftp = ftp;
  p->dcnt = (int32_t)(*p->idel * CS_EKR);
  if (IS_POW_TWO(ftp->flen)) {
    if (*p->idur == FL(0.0)) {
      p->phs = MAXLEN;
      p->kinc = 1; /* Select the fixed-point table path. */
      p->dcnt = -1;
    }
    else {
      cs_double increment = CS_KICVT / *p->idur;
      p->phs = *p->idur < FL(0.0) ? MAXLEN - 1 : 0;
      /* A scan shorter than one control period reaches the end in one step. */
      if (increment >= MAXLEN) p->kinc = MAXLEN;
      else if (increment <= -MAXLEN) p->kinc = -MAXLEN;
      else p->kinc = (int32_t) increment;
      if (p->kinc == 0) p->kinc = *p->idur < FL(0.0) ? -1 : 1;
    }
  }
  else {
    p->kinc = 0;
    if (*p->idur == FL(0.0)) {
      p->fphs = 1.;
      p->inc = 0.;
      p->dcnt = -1;
    }
    else {
      p->fphs = *p->idur < FL(0.0) ? 1. - 1./ftp->flen : 0.;
      p->inc = 1./(*p->idur*CS_EKR);
    }
  }

  return OK;
}

int32_t kosc1(CSOUND *csound, OSCIL1 *p)
{
  FUNC *ftp;
  int32_t  phs = p->phs, dcnt;
  cs_float fphs = p->fphs;
  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  if(p->kinc != 0)
    *p->rslt = *(ftp->ftable + (phs >> ftp->lobits)) * *p->kamp;
  else
    *p->rslt = *(ftp->ftable + (size_t) (fphs*ftp->flen)) * *p->kamp;

  if ((dcnt = p->dcnt) > 0)
    dcnt--;
  else if (dcnt == 0) {
    if(p->kinc != 0) {
      phs += p->kinc;
      if (UNLIKELY(phs >= MAXLEN)){
        phs = MAXLEN;
        dcnt--;
      }
      else if (UNLIKELY(phs < 0)){
        phs = 0;
        dcnt--;
      }
      p->phs = phs;
    } else {
      fphs += p->inc;
      if (UNLIKELY(fphs >= 1.)){
        fphs = 1.;
        dcnt--;
      }
    else if (UNLIKELY(fphs < 0)){
      fphs = 0.;
      dcnt--;
    }
    p->fphs = fphs;
  }
}
  p->dcnt = dcnt;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil1(krate): not initialised"));
}

int32_t kosc1i(CSOUND *csound, OSCIL1   *p)
{
  FUNC        *ftp;
  cs_float       fract, v1, *ftab;
  cs_double      fphs = p->fphs;
  int32_t     phs = p->phs, dcnt;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  phs = p->phs;
  if (p->kinc != 0) {
    if (phs >= MAXLEN)
      *p->rslt = ftp->ftable[ftp->flen] * *p->kamp;
    else {
      fract = PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
    }
  }
  else {
    cs_double position = fphs * ftp->flen;
    if (position >= ftp->flen)
      *p->rslt = ftp->ftable[ftp->flen] * *p->kamp;
    else {
      uint32_t index = (uint32_t) position;
      fract = (cs_float)(position - index);
      ftab = ftp->ftable + index;
      v1 = *ftab++;
      *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
    }
  }
  if ((dcnt = p->dcnt) > 0) {
    dcnt--;
  }
  else if (dcnt == 0) {
    if(p->kinc != 0) {
    phs += p->kinc;
    if (UNLIKELY(phs >= MAXLEN)){
      phs = MAXLEN;
      dcnt--;
    }
    else if (UNLIKELY(phs < 0)){
      phs = 0;
      dcnt--;
    }
    p->phs = phs;
    } else {
    fphs += p->inc;
    if (UNLIKELY(fphs >= 1.)){
      fphs = 1.;
      dcnt--;
    }
    else if (UNLIKELY(fphs < 0)){
      fphs = 0.;
      dcnt--;
    }
    p->fphs = fphs;
    }
  }
  p->dcnt = dcnt;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil1i(krate): not initialised"));
}

int32_t oscnset(CSOUND *csound, OSCILN *p)
{
    FUNC *ftp = csound->FTFind(csound, p->ifn);
    cs_double repeats = *p->itimes;
    cs_double frequency = *p->ifrq;
    cs_double advance;

    if (UNLIKELY(ftp == NULL)) return NOTOK;
    if (UNLIKELY(!(repeats >= 0.0 && repeats < 2147483648.0)))
      return csound->InitError(csound, "%s", Str("osciln: invalid repeat count"));
    if (UNLIKELY(frequency < 0.0 || !isfinite(frequency)))
      return csound->InitError(csound, "%s", Str("osciln: invalid frequency"));

    p->ftp = ftp;
    p->ntimes = (int32_t)repeats;
    p->phase = 0.0;
    advance = frequency / CS_ESR;
    /* Split at init time: even several cycles per sample need no wrapping
       function in the audio loop. Larger advances finish on the first sample. */
    if (advance >= p->ntimes) {
      p->cycles = p->ntimes;
      p->inc = 0.0;
    }
    else {
      p->cycles = (int32_t)advance;
      p->inc = advance - p->cycles;
    }
    return OK;
}

int32_t osciln(CSOUND *csound, OSCILN *p)
{
  cs_float *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t n = offset, nsmps = CS_KSMPS;

  if (UNLIKELY(p->ftp == NULL))
    return csound->PerfError(csound, &(p->h), Str("osciln: not initialised"));
  if (UNLIKELY(offset)) memset(rs, 0, offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], 0, early*sizeof(cs_float));
  }
  if (p->ntimes > 0) {
    cs_float *ftbl = p->ftp->ftable;
    cs_float amp = *p->kamp;
    cs_double phase = p->phase, inc = p->inc;
    cs_double length = p->ftp->flen;
    int32_t remaining = p->ntimes, cycles = p->cycles;

    for (; n < nsmps; n++) {
      rs[n] = ftbl[(int32_t)(phase * length)] * amp;
      phase += inc;
      if (phase >= 1.0) {
        phase -= 1.0;
        remaining--;
      }
      remaining -= cycles;
      if (remaining <= 0) {
        remaining = 0;
        phase = 0.0;
        n++; /* Keep the sample just emitted. */
        break;
      }
    }
    p->phase = phase;
    p->ntimes = remaining;
  }
  if (n < nsmps)
    memset(&rs[n], 0, (nsmps-n)*sizeof(cs_float));
  return OK;
}

/* Oscillators */
int32_t posc_set(CSOUND *csound, OSC *p)
{
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
    return csound->InitError(csound, Str("table not found in poscil"));
  p->ftp        = ftp;
  p->tablen     = ftp->flen;
  p->tablenUPsr = p->tablen * (FL(1.0)/CS_ESR);
  if (*p->iphs>=FL(0.0))
    p->phs      = *p->iphs * p->tablen;
  while (UNLIKELY(p->phs >= p->tablen))
    p->phs     -= p->tablen;
  return OK;
}
int32_t posckkt(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablenUPsr;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    out[n]    = *(ft + (int32)phs)*amp;
    phs      += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0 ))
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t poscaat(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_double      phs = p->phs;
  cs_float       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *amp = p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff = freq[n];
    out[n]   = *(ft + (int32)phs)*amp[n];
    phs      += ff * p->tablenUPsr;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t posckat(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_double      phs = p->phs;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;
  cs_float       *freq = p->xcps;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff  = freq[n];
    out[n]    = *(ft + (int32)phs)*amp;
    phs      += ff * p->tablenUPsr;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0 ))
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t poscakt(CSOUND *csound, OSC *p)
{

  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablenUPsr;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *amp = p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    out[n]    = *(ft + (int32)phs)*amp[n];
    phs      += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t kposct(CSOUND *csound, OSC *p)
{
  IGN(csound);
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablen * CS_ONEDKR;

  *p->sr = *(p->ftp->ftable + (int32)phs) * *p->xamp;
  phs    += si;
  while (UNLIKELY(phs >= p->tablen))
    phs -= p->tablen;
  while (UNLIKELY(phs < 0.0))
    phs += p->tablen;
  p->phs = phs;
  return OK;
}

int32_t posckk(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_float       *curr_samp, fract;
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablenUPsr; /* gab c3 */
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract     = (cs_float)(phs - (int32)phs);
    out[n]    = amp * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    phs      += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0 ))
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t poscaa(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_float       *curr_samp, fract;
  cs_double      phs = p->phs;
  cs_float       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *amp = p->xamp; /*gab c3*/

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff = freq[n];
    curr_samp = ft + (int32)phs;
    fract     = (cs_float)(phs - (int32)phs);
    out[n]    = amp[n] *
      (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);/* gab c3 */
    phs      += ff * p->tablenUPsr;/* gab c3 */
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t poscka(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_float       *curr_samp, fract;
  cs_double      phs = p->phs;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;
  cs_float       *freq = p->xcps;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff  = freq[n];
    curr_samp = ft + (int32)phs;
    fract     = (cs_float)(phs - (int32)phs);
    out[n]    = amp * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    phs      += ff * p->tablenUPsr;/* gab c3 */
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0 ))
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t poscak(CSOUND *csound, OSC *p)
{

  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ft;
  cs_float       *curr_samp, fract;
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablenUPsr;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *amp = p->xamp; /*gab c3*/

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract     = (cs_float)(phs - (int32)phs);
    out[n]    = amp[n] *
      (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);/* gab c3 */
    phs      += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t kposc(CSOUND *csound, OSC *p)
{
  IGN(csound);
  cs_double      phs = p->phs;
  cs_double      si = *p->xcps * p->tablen * CS_ONEDKR;
  cs_float       *curr_samp = p->ftp->ftable + (int32)phs;
  cs_float       fract = (cs_float)(phs - (cs_double)((int32)phs));

  *p->sr = *p->xamp * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
  phs    += si;
  while (UNLIKELY(phs >= p->tablen))
    phs -= p->tablen;
  while (UNLIKELY(phs < 0.0))
    phs += p->tablen;
  p->phs = phs;
  return OK;
}

int32_t posc3kk(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ftab;
  cs_float       fract;
  cs_double      phs  = p->phs;
  cs_double      si   = *p->xcps * p->tablen * CS_ONEDSR;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;
  int32_t     x0;
  cs_float       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (cs_float)(phs - (cs_double)x0);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (UNLIKELY(x0>p->tablen)) y2 = ftab[1];
    else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1   = y2 + y0+y0+y0;
      out[n]     = amp * (y0 + FL(0.5)*frcu +
                          fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                 - ym1/FL(3.0)) +
                          frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                          frsq*(FL(0.5)* y1 - y0));
    }
    phs += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t posc3ak(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ftab;
  cs_float       fract;
  cs_double      phs  = p->phs;
  cs_double      si   = *p->xcps * p->tablen * CS_ONEDSR;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *ampp = p->xamp;
  int32_t     x0;
  cs_float       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (cs_float)(phs - (cs_double)x0);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (UNLIKELY(x0>p->tablen)) y2 = ftab[1];
    else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1   = y2 + y0+y0+y0;
      out[n]     = ampp[n] * (y0 + FL(0.5)*frcu +
                              fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                     - ym1/FL(3.0)) +
                              frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                              frsq*(FL(0.5)* y1 - y0));
    }
    phs += si;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t posc3ka(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ftab;
  cs_float       fract;
  cs_double      phs  = p->phs;
  /*double      si   = *p->freq * p->tablen * CS_ONEDSR;*/
  cs_float       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       amp = *p->xamp;
  int32_t     x0;
  cs_float       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff = freq[n];
    x0    = (int32)phs;
    fract = (cs_float)(phs - (cs_double)x0);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (UNLIKELY(x0>p->tablen)) y2 = ftab[1];
    else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1   = y2 + y0+y0+y0;
      out[n]     = amp * (y0 + FL(0.5)*frcu +
                          fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                 - ym1/FL(3.0)) +
                          frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                          frsq*(FL(0.5)* y1 - y0));
    }
    phs      += ff * p->tablenUPsr;
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t posc3aa(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = p->ftp;
  cs_float       *out = p->sr, *ftab;
  cs_float       fract;
  cs_double      phs  = p->phs;
  /*double      si   = *p->freq * p->tablen * CS_ONEDSR;*/
  cs_float       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *ampp = p->xamp;
  int32_t     x0;
  cs_float       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    cs_float ff = freq[n];
    x0    = (int32)phs;
    fract = (cs_float)(phs - (cs_double)x0);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (UNLIKELY(x0>p->tablen)) y2 = ftab[1];
    else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1   = y2 + y0+y0+y0;
      out[n]     = ampp[n] * (y0 + FL(0.5)*frcu +
                              fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                     - ym1/FL(3.0)) +
                              frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                              frsq*(FL(0.5)* y1 - y0));
      phs       += ff * p->tablenUPsr;
    }
    while (UNLIKELY(phs >= p->tablen))
      phs -= p->tablen;
    while (UNLIKELY(phs < 0.0) )
      phs += p->tablen;
  }
  p->phs = phs;
  return OK;
}

int32_t kposc3(CSOUND *csound, OSC *p)
{
  IGN(csound);
  cs_double      phs   = p->phs;
  cs_double      si    = *p->xcps * p->tablen * CS_ONEDKR;
  cs_float       *ftab = p->ftp->ftable;
  int32_t     x0    = (int32_t)phs;
  cs_float       fract = (cs_float)(phs - (cs_double)x0);
  cs_float       y0, y1, ym1, y2;
  cs_float       amp = *p->xamp;

  x0--;
  if (UNLIKELY(x0<0)) {
    ym1 = ftab[p->tablen-1]; x0 = 0;
  }
  else ym1 = ftab[x0++];
  y0 = ftab[x0++];
  y1 = ftab[x0++];
  if (UNLIKELY(x0>p->tablen)) y2 = ftab[1];
  else y2 = ftab[x0];
  {
    cs_float frsq = fract*fract;
    cs_float frcu = frsq*ym1;
    cs_float t1 = y2 + y0+y0+y0;
    *p->sr  = amp * (y0 + FL(0.5)*frcu +
                     fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                            - ym1/FL(3.0)) +
                     frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                     frsq*(FL(0.5)* y1 - y0));
  }
  phs += si;
  while (UNLIKELY(phs >= p->tablen))
    phs -= p->tablen;
  while (UNLIKELY(phs < 0.0))
    phs += p->tablen;
  p->phs = phs;
  return OK;
}

static void reassign_perf(CSOUND *csound, OSC *p) {
  const char* name = p->h.optext->t.opcod;
  // check for arg types and change PDS
  if (!strcmp(name, "oscil") || !strncmp(name, "oscil.", 6)) {
    if(IS_ASIG_ARG(p->sr)) {
    if(IS_ASIG_ARG(p->xamp) && IS_ASIG_ARG(p->xcps)) // aa
      p->h.perf = (SUBR) poscaat;
    else if(IS_ASIG_ARG(p->xamp)) // ak
      p->h.perf = (SUBR) poscakt;
    else if(IS_ASIG_ARG(p->xcps)) // ka
      p->h.perf = (SUBR) posckat;
    else // kk
      p->h.perf = (SUBR) posckkt;
    } else // kosc
    p->h.perf = (SUBR) kposct;
  }
  else if (!strcmp(name, "oscili") || !strncmp(name, "oscili.", 7)) {
  if(IS_ASIG_ARG(p->sr)) {
    if(IS_ASIG_ARG(p->xamp) && IS_ASIG_ARG(p->xcps)) // aa
      p->h.perf = (SUBR) poscaa;
    else if(IS_ASIG_ARG(p->xamp)) // ak
      p->h.perf = (SUBR) poscak;
    else if(IS_ASIG_ARG(p->xcps)) // ka
      p->h.perf = (SUBR) poscka;
    else // kk
      p->h.perf = (SUBR) posckk;
  } else // kosc
    p->h.perf = (SUBR) kposc;
  } else {  // oscil3
   if(IS_ASIG_ARG(p->sr)) {
    if(IS_ASIG_ARG(p->xamp) && IS_ASIG_ARG(p->xcps)) // aa
      p->h.perf = (SUBR) posc3aa;
    else if(IS_ASIG_ARG(p->xamp)) // ak
      p->h.perf = (SUBR) posc3ak;
    else if(IS_ASIG_ARG(p->xcps)) // ka
      p->h.perf = (SUBR) posc3ka; 
    else // kk
      p->h.perf = (SUBR) posc3kk;
  } else // kosc 
    p->h.perf = (SUBR) kposc3;
  }
}

/* Select the phase representation on every init, including reinit. */
static void osc_init_phase(CSOUND *csound, OSC *p)
{
  int32_t len = p->ftp->flen;
  if (IS_POW_TWO(len)) {
    if (*p->iphs >= FL(0.0))
      p->lphs = ((int32_t)(*p->iphs * FMAXLEN)) & PHMASK;
    else if (p->tablen > 0 && !IS_POW_TWO(p->tablen))
      p->lphs = (int32_t)(p->phs / p->tablen * FMAXLEN) & PHMASK;
    p->h.perf = p->h.optext->t.oentry->perf;
  }
  else {
    if (*p->iphs >= FL(0.0))
      p->phs = *p->iphs * len;
    else if (p->tablen > 0) {
      /* A negative iphs preserves the phase as a fraction of a cycle. */
      if (IS_POW_TWO(p->tablen))
        p->phs = (cs_double)p->lphs / FMAXLEN * len;
      else if (p->tablen != len)
        p->phs = p->phs / p->tablen * len;
    }
    while (UNLIKELY(p->phs >= len))
      p->phs -= len;
    reassign_perf(csound, p);
  }
  p->tablen = len;
  p->tablenUPsr = len * (FL(1.0) / CS_ESR);
}

int32_t oscset(CSOUND *csound, OSC *p)
{
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
    return csound->InitError(csound, Str("table not found"));
  p->ftp = ftp;
  osc_init_phase(csound, p);
  return OK;
}


int32_t oscsetA(CSOUND *csound, OSC *p)
{
  ARRAYDAT *a = (ARRAYDAT*)p->ifn;
  FUNC *f = &p->FF;
  int32_t     lobits, ltest, flen, i;
  int32_t     nonpowof2_flag = 0;
  size_t bytes;

  if (UNLIKELY(a->dimensions != 1 || a->sizes == NULL || a->data == NULL ||
               a->sizes[0] < 1 || a->sizes[0] > MAXLEN))
    return csound->InitError(csound,
                            Str("oscil: invalid waveform array size or dimensions"));
  flen = f->flen = a->sizes[0];
  /* Array inputs have no guard point and may move when resized. Keep an
     init-time copy with the wraparound sample expected by interpolation. */
  bytes = ((size_t)flen + 1) * sizeof(cs_float);
  if (p->arraydata.auxp == NULL || p->arraydata.size < bytes)
    csound->AuxAlloc(csound, bytes, &p->arraydata);
  f->ftable = (cs_float*)p->arraydata.auxp;
  memcpy(f->ftable, a->data, (size_t)flen * sizeof(cs_float));
  f->ftable[flen] = f->ftable[0];
  for (ltest = flen, lobits = 0;
       (ltest & MAXLEN) == 0L;
       lobits++, ltest <<= 1)
    ;
  if (UNLIKELY(ltest != MAXLEN)) {
    lobits = 0;
    nonpowof2_flag = 1;
  }
  f->lenmask  = ((flen & (flen - 1L)) ?
                 0L : (flen - 1L));      /*  init hdr w powof2 data  */
  f->lobits   = lobits;
  i           = (1 << lobits);
  f->lomask   = (int32_t) (i - 1);
  f->lodiv    = FL(1.0) / (cs_float) i;        /*    & other useful vals   */
  f->nchanls  = 1;                          /*    presume mono for now  */
  f->flenfrms = flen;
  if (nonpowof2_flag)
    f->lenmask = 0xFFFFFFFF;
  p->ftp = f;
  osc_init_phase(csound, p);
  return OK;
}



int32_t koscil(CSOUND *csound, OSC *p)
{
  FUNC    *ftp;
  int32_t    phs, inc;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  phs = p->lphs;
  inc = (int32_t) (*p->xcps * CS_KICVT);
  *p->sr = ftp->ftable[phs >> ftp->lobits] * *p->xamp;
  phs += inc;
  phs &= PHMASK;
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil(krate): not initialised"));
}

int32_t osckk(CSOUND *csound, OSC *p)
{
  FUNC    *ftp;
  cs_float   amp, *ar, *ftbl;
  int32_t   phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) {
      if (UNLIKELY(oscset(csound, p) != OK)) goto err1;
      ftp = p->ftp;
    }
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
    lobits = ftp->lobits;
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }

  for (n=offset;n<nsmps;n++) {
    ar[n] = ftbl[phs >> lobits] * amp;
    /* phs += inc; */
    /* phs &= PHMASK; */
    phs = (phs+inc)&PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil: not initialised"));
}

int32_t oscka(CSOUND *csound, OSC *p)
{

    FUNC    *ftp;
    cs_float   *ar, amp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftbl = ftp->ftable;
  lobits = ftp->lobits;
  amp = *p->xamp;
  cpsp = p->xcps;
  phs = p->lphs;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
    ar[n] = ftbl[phs >> lobits] * amp;
    phs += inc;
    phs &= PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil: not initialised"));
}

int32_t oscak(CSOUND *csound, OSC *p)
{
  FUNC    *ftp;
  cs_float   *ar, *ampp, *ftbl;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;


    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) {
      if (UNLIKELY(oscset(csound, p) != OK)) goto err1;
      ftp = p->ftp;
    }
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset;n<nsmps;n++) {
      ar[n] = ftbl[phs >> lobits] * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;

 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil: not initialised"));
}

int32_t oscaa(CSOUND *csound, OSC *p)
{

    FUNC    *ftp;
    cs_float   *ar, *ampp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) {
    if (UNLIKELY(oscset(csound, p) != OK)) goto err1;
    ftp = p->ftp;
  }
  ftbl = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
    ar[n] = ftbl[phs >> lobits] * ampp[n];
    phs = (phs+inc) & PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil: not initialised"));
}

int32_t koscli(CSOUND *csound, OSC   *p)
{
  FUNC    *ftp;
  int32_t    phs, inc;
  cs_float  *ftab, fract, v1;

  phs = p->lphs;
  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  fract = PFRAC(phs);
  ftab = ftp->ftable + (phs >> ftp->lobits);
  v1 = ftab[0];
  *p->sr = (v1 + (ftab[1] - v1) * fract) * *p->xamp;
  inc = (int32_t)(*p->xcps * CS_KICVT);
  phs += inc;
  phs &= PHMASK;
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscili(krate): not initialised"));
}

int32_t osckki(CSOUND *csound, OSC   *p){
  FUNC    *ftp;
  cs_float   fract, v1, amp, *ar, *ft, *ftab;
  int32_t   phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if(!(IS_ASIG_ARG(p->sr)))
     csound->PerfError(csound, &p->h, "output is not a-type\n");

  if (UNLIKELY((ftp = p->ftp)==NULL)) goto err1;
  lobits = ftp->lobits;
  phs = p->lphs;
  inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
  amp = *p->xamp;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  ft = ftp->ftable;
  for (n=offset; n<nsmps; n++) {
    fract = PFRAC(phs);
    ftab = ft + (phs >> lobits);
    v1 = ftab[0];
    ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
    phs = (phs+inc) & PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscili: not initialised"));
}

int32_t osckai(CSOUND *csound, OSC   *p)
{

    FUNC    *ftp;
    cs_float   *ar, amp, *cpsp, fract, v1, *ftab, *ft;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  lobits = ftp->lobits;
  amp = *p->xamp;
  cpsp = p->xcps;
  phs = p->lphs;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  ft = ftp->ftable;
  for (n=offset;n<nsmps;n++) {
    int32_t inc;
    inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
    fract = PFRAC(phs);
    ftab = ft + (phs >> lobits);
    v1 = ftab[0];
    ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
    phs += inc;
    phs &= PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscili: not initialised"));
}

int32_t oscaki(CSOUND *csound, OSC   *p)
{
  FUNC    *ftp;
  cs_float    v1, fract, *ar, *ampp, *ftab, *ft;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    ft = ftp->ftable;
    for (n=offset;n<nsmps;n++) {
      fract = (cs_float) PFRAC(phs);
      ftab = ft + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;

 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscili: not initialised"));
}

int32_t oscaai(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    cs_float   v1, fract, *ar, *ampp, *cpsp, *ftab, *ft;
    int32_t   phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    cs_float   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ft = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc;
    inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
    fract = (cs_float) PFRAC(phs);
    ftab = ft + (phs >> lobits);
    v1 = ftab[0];
    ar[n] = (v1 + (ftab[1] - v1) * fract) * ampp[n];
    phs = (phs+inc) & PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscili: not initialised"));
}

int32_t koscl3(CSOUND *csound, OSC   *p)
{
  FUNC    *ftp;
  int32_t    phs, inc;
  cs_float  *ftab, fract;
  int32_t   x0;
  cs_float   y0, y1, ym1, y2, amp = *p->xamp;

  phs = p->lphs;
  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftab = ftp->ftable;
  fract = PFRAC(phs);
  x0 = (phs >> ftp->lobits);
  x0--;
  if (UNLIKELY(x0<0)) {
    ym1 = ftab[ftp->flen-1]; x0 = 0;
  }
  else ym1 = ftab[x0++];
  y0 = ftab[x0++];
  y1 = ftab[x0++];
  if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
  {
    cs_float frsq = fract*fract;
    cs_float frcu = frsq*ym1;
    cs_float t1 = y2 + y0+y0+y0;
    *p->sr = amp * (y0 + FL(0.5)*frcu +
                    fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                    frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                    frsq*(FL(0.5)* y1 - y0));
  }
  inc = (int32_t)(*p->xcps * CS_KICVT);
  phs += inc;
  phs &= PHMASK;
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil3(krate): not initialised"));
}


int32_t osckk3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    cs_float   fract, amp, *ar, *ftab;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    cs_float   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset;n<nsmps;n++) {
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
/*    printf("fract = %f; y = %f, %f, %f, %f\n", fract,ym1,y0,y1,y2); */
      {
        cs_float frsq = fract*fract;
        cs_float frcu = frsq*ym1;
        cs_float t1 = y2 + y0+y0+y0;
/*      cs_float old = (y0 + (y1 - y0) * fract) * amp; */
/*      double x = ((double)(x0-2)+fract)*twopi/32.0; */
/*      cs_float tr = amp*sin(x); */
        ar[n] = amp * (y0 + FL(0.5)*frcu +
                       fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                       frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                       frsq*(FL(0.5)* y1 - y0));
/*      printf("oscilkk3: old=%.4f new=%.4f true=%.4f (%f; %f)\n", */
/*                       old, *(ar-1), tr, fabs(*(ar-1)-tr), fabs(old-tr)); */
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}

int32_t oscka3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    cs_float   *ar, amp, *cpsp, fract, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    cs_float   y0, y1, ym1, y2;
    cs_float   sicvt = CS_SICVT;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(cs_float));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc;
      inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
      {
        cs_float frsq = fract*fract;
        cs_float frcu = frsq*ym1;
        cs_float t1 = y2 + y0+y0+y0;
        ar[n] = amp * (y0 + FL(0.5)*frcu +
                       fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                       frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)*
                                                                    y1 - y0));
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}

int32_t oscak3(CSOUND *csound, OSC   *p)
{
  FUNC    *ftp;
  cs_float   fract, *ar, *ampp, *ftab;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32_t   x0;
  cs_float   y0, y1, ym1, y2;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftab = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  inc = CS_FLOAT2LONG(*p->xcps * CS_SICVT);
  ampp = p->xamp;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset;n<nsmps;n++) {
    fract = (cs_float) PFRAC(phs);
    x0 = (phs >> lobits);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[ftp->flen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0 = ftab[x0++];
    y1 = ftab[x0++];
    if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1 = y2 + y0+y0+y0;
      ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                        + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                        + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                        + frsq*(FL(0.5)* y1 - y0));
    }
    phs = (phs+inc) & PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil3: not initialised"));
}

int32_t oscaa3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    cs_float    fract, *ar, *ampp, *cpsp, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t    x0;
    cs_float    y0, y1, ym1, y2;
    cs_float    sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftab = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = CS_FLOAT2LONG(cpsp[n] * sicvt);
    fract = (cs_float) PFRAC(phs);
    x0 = (phs >> lobits);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[ftp->flen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0 = ftab[x0++];
    y1 = ftab[x0++];
    if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1 = y2 + y0+y0+y0;
      ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                        + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                        + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                        + frsq*(FL(0.5)* y1 - y0));
    }
    phs = (phs+inc) & PHMASK;
  }
  p->lphs = phs;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil3: not initialised"));
}


/* Keep ordinary wraps cheap; reduce larger jumps without repeated subtraction. */
#define LPOSC_WRAP(phase, start, end, length) do {                       \
  if ((phase) >= (end)) (phase) -= (length);                             \
  else if ((phase) < (start)) (phase) += (length);                       \
  if ((phase) >= (end) || (phase) < (start)) {                           \
    (phase) = (start) + fmod((phase) - (start), (length));                \
    if ((phase) < (start)) (phase) += (length);                          \
  }                                                                    \
  if ((phase) >= (end)) (phase) = (start); /* rounding at the endpoint */ \
} while (0)

int32_t lposc_set(CSOUND *csound, LPOSC *p)
{
  FUNC   *ftp;
  cs_double loop, end, looplength;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->ift)) == NULL))
    return NOTOK;
  if (UNLIKELY(!(p->fsr=ftp->gen01args.sample_rate))) {
    csound->Warning(csound, Str("lposc: no sample rate stored in function "
                                "assuming=sr\n"));
    p->fsr=CS_ESR;
  }
  p->ftp    = ftp;
  p->tablen = ftp->flen;
  /* changed from
     p->phs    = *p->iphs * p->tablen;   */

  if (UNLIKELY((loop = *p->kloop) < 0)) loop=FL(0.0);
  if ((end = *p->kend) > p->tablen || end <=0 )
    end = (cs_float)p->tablen;
  if (UNLIKELY(!(loop < end)))
    return csound->InitError(csound, Str("lposcil: loop start must precede end"));
  looplength = end - loop;

  if (*p->iphs >= 0)
    p->phs = *p->iphs;
  if (p->phs >= end || p->phs < 0)
    LPOSC_WRAP(p->phs, loop, end, looplength);
  return OK;
}

int32_t lposca(CSOUND *csound, LPOSC *p)
{
  cs_double  phs = p->phs;
  cs_double  si= *p->freq * (p->fsr/CS_ESR);
  cs_float   *out = p->out,  *amp=p->amp;
  cs_float   *ft =  p->ftp->ftable, *curr_samp;
  cs_float   fract;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double loop, end, looplength;

  if ((loop = *p->kloop) < 0) loop = 0;
  if ((end = *p->kend) > p->tablen || end <= 0) end = p->tablen;
  if (UNLIKELY(!(loop < end)))
    return csound->PerfError(csound, &(p->h),
                             Str("lposcil: loop start must precede end"));
  looplength = end - loop;
  if (phs >= end || phs < 0)
    LPOSC_WRAP(phs, loop, end, looplength);
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp= ft + (int64_t)phs;
    fract= (cs_float)(phs - (int64_t)phs);
    out[n] = amp[n] * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    phs += si;
    if (phs >= end || (si < 0 && phs < loop))
      LPOSC_WRAP(phs, loop, end, looplength);
  }
  p->phs = phs;
  return OK;
}

int32_t lposc(CSOUND *csound, LPOSC *p)
{
  cs_float       *out = p->out, *ft = p->ftp->ftable;
  cs_float       *curr_samp, fract;
  cs_double      phs= p->phs, si= *p->freq * (p->fsr*CS_ONEDSR);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double      loop, end, looplength;// = p->looplength;
  cs_float       amp = *p->amp;

  if ((loop = *p->kloop) < 0) loop=0;
  if ((end = *p->kend) > p->tablen || end <=0 )
    end = p->tablen;
  if (UNLIKELY(!(loop < end)))
    return csound->PerfError(csound, &(p->h),
                             Str("lposcil: loop start must precede end"));
  looplength = end - loop;
  if (phs >= end || phs < 0)
    LPOSC_WRAP(phs, loop, end, looplength);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract = (cs_float)(phs - (cs_double)((int32)phs));
    out[n] = amp * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    phs += si;
    if (phs >= end || (si < 0 && phs < loop))
      LPOSC_WRAP(phs, loop, end, looplength);
  }
  p->phs = phs;
  return OK;
}

int32_t lposc3(CSOUND *csound, LPOSC *p)
{
  cs_float       *out = p->out, *ftab = p->ftp->ftable;
  cs_float       fract;
  cs_double      phs = p->phs, si= *p->freq * (p->fsr*CS_ONEDSR);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double      loop, end, looplength;// = p->looplength;
  cs_float       amp = *p->amp;
  int32_t     x0;
  cs_float       y0, y1, ym1, y2;

  if (UNLIKELY((loop = *p->kloop) < 0)) loop=0;
  if ((end = *p->kend) > p->tablen || end <=0 ) end = p->tablen;
  if (UNLIKELY(!(loop < end)))
    return csound->PerfError(csound, &(p->h),
                             Str("lposcil: loop start must precede end"));
  looplength = end - loop;
  if (phs >= end || phs < 0)
    LPOSC_WRAP(phs, loop, end, looplength);

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (cs_float)(phs - (cs_double)x0);
    x0--;
    if (x0<0) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (x0>p->tablen) y2 = ftab[1]; else y2 = ftab[x0];
    {
      cs_float frsq = fract*fract;
      cs_float frcu = frsq*ym1;
      cs_float t1   = y2 + y0+y0+y0;
      out[n]     = amp * (y0 + FL(0.5)*frcu +
                          fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                 - ym1/FL(3.0)) +
                          frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                          frsq*(FL(0.5)* y1 - y0));
    }
    phs += si;
    if (phs >= end || (si < 0 && phs < loop))
      LPOSC_WRAP(phs, loop, end, looplength);
  }
  p->phs = phs;
  return OK;
}
