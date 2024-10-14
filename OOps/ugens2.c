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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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
#define MYFLOOR(x) (x >= FL(0.0) ? (int32_t)x : (int32_t)((double)x - 0.99999999))



int32_t phsset(CSOUND *csound, PHSOR *p)
{
  MYFLT       phs;
  int32_t  longphs;
  if ((phs = *p->iphs) >= FL(0.0)) {
    if (UNLIKELY((longphs = (int32_t)phs))) {
      csound->Warning(csound, Str("init phase truncation\n"));
    }
    p->curphs = phs - (MYFLT)longphs;
  }
  return OK;
}

int32_t ephsset(CSOUND *csound, EPHSOR *p)
{
  MYFLT       phs;
  int32_t  longphs;
  if ((phs = *p->iphs) >= FL(0.0)) {
    if (UNLIKELY((longphs = (int32_t)phs))) {
      csound->Warning(csound, Str("init phase truncation\n"));
    }
    p->curphs = phs - (MYFLT)longphs;
  }
  p->b = 1.0;
  return OK;
}

int32_t ephsor(CSOUND *csound, EPHSOR *p)
{
    double      phase;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *rs, *aphs, onedsr = CS_ONEDSR;
    double      b = p->b;
    double      incr, R = *p->kR;

  rs = p->sr;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
  }
  aphs = p->aphs;
  phase = p->curphs;
  if (IS_ASIG_ARG(p->xcps)) {
    MYFLT *cps = p->xcps;
    for (n=offset; n<nsmps; n++) {
      incr = (double)(cps[n] * onedsr);
      rs[n] = (MYFLT) b;
      aphs[n] = (MYFLT) phase;
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
    }
  }
  else {
    incr = (double)(*p->xcps * onedsr);
    for (n=offset; n<nsmps; n++) {
      rs[n] = (MYFLT) b;
      aphs[n] = (MYFLT) phase;
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
    }
  }
  p->curphs = phase;
  p->b = b;
  return OK;
}

int32_t kphsor(CSOUND *csound, PHSOR *p)
{
  IGN(csound);
  double      phs;
  *p->sr = (MYFLT)(phs = p->curphs);
  if (UNLIKELY((phs += (double)*p->xcps * CS_ONEDKR) >= 1.0))
    phs -= 1.0;
  else if (UNLIKELY(phs < 0.0))
    phs += 1.0;
  p->curphs = phs;
  return OK;
}

int32_t phsor(CSOUND *csound, PHSOR *p)
{

    double      phase;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *rs, onedsr = CS_ONEDSR;
    double      incr;


  rs = p->sr;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
  }
  phase = p->curphs;
  if (IS_ASIG_ARG(p->xcps)) {
    MYFLT *cps = p->xcps;
    for (n=offset; n<nsmps; n++) {
      incr = (double)(cps[n] * onedsr);
      rs[n] = (MYFLT)phase;
      phase += incr;
      if (UNLIKELY((MYFLT)phase >= FL(1.0))) /* VL convert to MYFLT
                                                to avoid rounded output
                                                exceeding 1.0 on float version */
        phase -= 1.0;
      else if (UNLIKELY((MYFLT)phase < FL(0.0)))
        phase += 1.0;
    }
  }
  else {
    incr = (double)(*p->xcps * onedsr);
    for (n=offset; n<nsmps; n++) {
      rs[n] = (MYFLT)phase;
      phase += incr;
      if (UNLIKELY((MYFLT)phase >= FL(1.0))) {
        phase -= 1.0;
      }
      else if (UNLIKELY((MYFLT)phase < FL(0.0)))
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
  if(IS_POW_TWO(ftp->flen)) { 
  if (UNLIKELY(*p->idur <= FL(0.0))) {
    p->phs = MAXLEN-1;
  }
  else p->phs = 0;
  p->kinc = (int32_t) (CS_KICVT / *p->idur);
  if (p->kinc==0) p->kinc = 1;
  } else {
    if (UNLIKELY(*p->idur <= FL(0.0)))
      p->fphs = 1. - 1./ftp->flen;
    else p->fphs = FL(0.0);
    p->kinc = 0;
    p->inc = 1./(*p->idur*CS_EKR);
  }
  p->ftp = ftp;
  p->dcnt = (int32_t)(*p->idel * CS_EKR);

  return OK;
}

int32_t kosc1(CSOUND *csound, OSCIL1 *p)
{
  FUNC *ftp;
  int32_t  phs = p->phs, dcnt;
  MYFLT fphs = p->fphs;
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
  MYFLT       fract, v1, *ftab, fphs = p->fphs;
  int32_t     phs = p->phs, dcnt;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  phs = p->phs;
  if(p->kinc != 0) {
  fract = PFRAC(phs);
  ftab = ftp->ftable + (phs >> ftp->lobits);
  v1 = *ftab++;
  *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
  } else {
    fphs = p->fphs;
    fract = fphs - (int64_t) fphs;
    ftab = ftp->ftable + (size_t) (fphs*ftp->flen);
    v1 = *ftab++;
    *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
  }
  if ((dcnt = p->dcnt) > 0) {
    dcnt--;
    p->dcnt = dcnt;
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
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("oscil1i(krate): not initialised"));
}

int32_t oscnset(CSOUND *csound, OSCILN *p)
{

    FUNC        *ftp;
    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      p->inc = ftp->flen * *p->ifrq * CS_ONEDSR;
      p->index = FL(0.0);
      p->maxndx = ftp->flen - FL(1.0);
      p->ntimes = (int32_t)*p->itimes;
      return OK;
    }
    else return NOTOK;
}

int32_t osciln(CSOUND *csound, OSCILN *p)
{
  MYFLT *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(p->ftp==NULL)) goto err1;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
  }
  if (p->ntimes) {
    MYFLT *ftbl = p->ftp->ftable;
    MYFLT amp = *p->kamp;
    MYFLT ndx = p->index;
    MYFLT inc = p->inc;
    MYFLT maxndx = p->maxndx;
    for (n=offset; n<nsmps; n++) {
      rs[n] = ftbl[(int32_t)ndx] * amp;
      if (UNLIKELY((ndx += inc) > maxndx)) {
        if (--p->ntimes)
          ndx -= maxndx;
        else if (UNLIKELY(n==nsmps))
          return OK;
        else
          goto putz;
      }
    }
    p->index = ndx;
  }
  else {
    n=0;              /* Can jump out of previous loop into this one */
  putz:
    memset(&rs[n], 0, (nsmps-n)*sizeof(MYFLT));
    /* for (; n<nsmps; n++) { */
    /*   rs[n] = FL(0.0); */
    /* } */
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("osciln: not initialised"));
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
  MYFLT       *out = p->sr, *ft;
  double      phs = p->phs;
  double      si = *p->xcps * p->tablenUPsr; 
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(early)) nsmps -= early;
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
  MYFLT       *out = p->sr, *ft;
  double      phs = p->phs;
  MYFLT       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *amp = p->xamp; 

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff = freq[n];
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
  MYFLT       *out = p->sr, *ft;
  double      phs = p->phs;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;
  MYFLT       *freq = p->xcps;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff  = freq[n];
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
  MYFLT       *out = p->sr, *ft;
  double      phs = p->phs;
  double      si = *p->xcps * p->tablenUPsr;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *amp = p->xamp; 

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
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
  double      phs = p->phs;
  double      si = *p->xcps * p->tablen * CS_ONEDKR;

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
  MYFLT       *out = p->sr, *ft;
  MYFLT       *curr_samp, fract;
  double      phs = p->phs;
  double      si = *p->xcps * p->tablenUPsr; /* gab c3 */
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(early)) nsmps -= early;
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract     = (MYFLT)(phs - (int32)phs);
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
  MYFLT       *out = p->sr, *ft;
  MYFLT       *curr_samp, fract;
  double      phs = p->phs;
  MYFLT       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *amp = p->xamp; /*gab c3*/

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff = freq[n];
    curr_samp = ft + (int32)phs;
    fract     = (MYFLT)(phs - (int32)phs);
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
  MYFLT       *out = p->sr, *ft;
  MYFLT       *curr_samp, fract;
  double      phs = p->phs;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;
  MYFLT       *freq = p->xcps;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff  = freq[n];
    curr_samp = ft + (int32)phs;
    fract     = (MYFLT)(phs - (int32)phs);
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
  MYFLT       *out = p->sr, *ft;
  MYFLT       *curr_samp, fract;
  double      phs = p->phs;
  double      si = *p->xcps * p->tablenUPsr;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *amp = p->xamp; /*gab c3*/

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil: not initialised"));
  ft = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract     = (MYFLT)(phs - (int32)phs);
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
  double      phs = p->phs;
  double      si = *p->xcps * p->tablen * CS_ONEDKR;
  MYFLT       *curr_samp = p->ftp->ftable + (int32)phs;
  MYFLT       fract = (MYFLT)(phs - (double)((int32)phs));

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
  MYFLT       *out = p->sr, *ftab;
  MYFLT       fract;
  double      phs  = p->phs;
  double      si   = *p->xcps * p->tablen * CS_ONEDSR;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;
  int32_t     x0;
  MYFLT       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (MYFLT)(phs - (double)x0);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1   = y2 + y0+y0+y0;
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
  MYFLT       *out = p->sr, *ftab;
  MYFLT       fract;
  double      phs  = p->phs;
  double      si   = *p->xcps * p->tablen * CS_ONEDSR;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *ampp = p->xamp;
  int32_t     x0;
  MYFLT       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (MYFLT)(phs - (double)x0);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1   = y2 + y0+y0+y0;
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
  MYFLT       *out = p->sr, *ftab;
  MYFLT       fract;
  double      phs  = p->phs;
  /*double      si   = *p->freq * p->tablen * CS_ONEDSR;*/
  MYFLT       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       amp = *p->xamp;
  int32_t     x0;
  MYFLT       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff = freq[n];
    x0    = (int32)phs;
    fract = (MYFLT)(phs - (double)x0);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1   = y2 + y0+y0+y0;
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
  MYFLT       *out = p->sr, *ftab;
  MYFLT       fract;
  double      phs  = p->phs;
  /*double      si   = *p->freq * p->tablen * CS_ONEDSR;*/
  MYFLT       *freq = p->xcps;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT       *ampp = p->xamp;
  int32_t     x0;
  MYFLT       y0, y1, ym1, y2;

  if (UNLIKELY(ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("poscil3: not initialised"));
  ftab = p->ftp->ftable;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT ff = freq[n];
    x0    = (int32)phs;
    fract = (MYFLT)(phs - (double)x0);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1   = y2 + y0+y0+y0;
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
  double      phs   = p->phs;
  double      si    = *p->xcps * p->tablen * CS_ONEDKR;
  MYFLT       *ftab = p->ftp->ftable;
  int32_t     x0    = (int32_t)phs;
  MYFLT       fract = (MYFLT)(phs - (double)x0);
  MYFLT       y0, y1, ym1, y2;
  MYFLT       amp = *p->xamp;

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
    MYFLT frsq = fract*fract;
    MYFLT frcu = frsq*ym1;
    MYFLT t1 = y2 + y0+y0+y0;
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
  if(!strcmp(name, "oscil")) {
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
  else if(!strcmp(name, "oscili")) {
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
      p->h.perf = (SUBR) poscaa;
    else if(IS_ASIG_ARG(p->xamp)) // ak
      p->h.perf = (SUBR) poscak;
    else if(IS_ASIG_ARG(p->xcps)) // ka
      p->h.perf = (SUBR) poscka; 
    else // kk
      p->h.perf = (SUBR) posckk;
  } else // kosc 
    p->h.perf = (SUBR) kposc;
  }
}

// this may select different perf if non-pow of two is used.
int32_t oscset(CSOUND *csound, OSC *p)
{
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))  
    return csound->InitError(csound, Str("table not found"));
  p->ftp = ftp;
  if(IS_POW_TWO(ftp->flen)) {
    if (*p->iphs >= 0)
      p->lphs = ((int32_t)(*p->iphs * FMAXLEN)) & PHMASK;
    return OK;
  }
  reassign_perf(csound, p);
  return posc_set(csound,p);
}


static int32_t fill_func_from_array(ARRAYDAT *a, FUNC *f)
{
  int32_t     lobits, ltest, flen, i;
  int32_t     nonpowof2_flag = 0;

  flen = f->flen = a->sizes[0];
  flen &= -2L;
  for (ltest = flen, lobits = 0;
       (ltest & MAXLEN) == 0L;
       lobits++, ltest <<= 1)
    ;
  if (UNLIKELY(ltest != MAXLEN)) {
    lobits = 0;
    nonpowof2_flag = 1;
  }
  f->ftable   = a->data;
  f->lenmask  = ((flen & (flen - 1L)) ?
                 0L : (flen - 1L));      /*  init hdr w powof2 data  */
  f->lobits   = lobits;
  i           = (1 << lobits);
  f->lomask   = (int32_t) (i - 1);
  f->lodiv    = FL(1.0) / (MYFLT) i;        /*    & other useful vals   */
  f->nchanls  = 1;                          /*    presume mono for now  */
  f->flenfrms = flen;
  if (nonpowof2_flag)
    f->lenmask = 0xFFFFFFFF;
  return OK;
}

int32_t oscsetA(CSOUND *csound, OSC *p)
{
  FUNC        *ftp = &p->FF;
  p->ftp = ftp;
  fill_func_from_array((ARRAYDAT*)p->ifn, ftp);
  if(IS_POW_TWO(ftp->flen)) {
  if (*p->iphs >= 0) 
    p->lphs = ((int32_t)(*p->iphs * FMAXLEN)) & PHMASK;
  return OK; // Indentation not logical always onbeyed NEEDS FIX JPff May 10 2024
  }
  p->tablen     = ftp->flen;
  p->tablenUPsr = p->tablen * (FL(1.0)/CS_ESR);
  if (*p->iphs>=FL(0.0))
    p->phs      = *p->iphs * p->tablen;
  while (UNLIKELY(p->phs >= p->tablen))
    p->phs     -= p->tablen;
  reassign_perf(csound, p);  
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
  MYFLT   amp, *ar, *ftbl;
  int32_t   phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * CS_SICVT);
    lobits = ftp->lobits;
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
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
    MYFLT   *ar, amp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftbl = ftp->ftable;
  lobits = ftp->lobits;
  amp = *p->xamp;
  cpsp = p->xcps;
  phs = p->lphs;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
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
  MYFLT   *ar, *ampp, *ftbl;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * CS_SICVT);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
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
    MYFLT   *ar, *ampp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftbl = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
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
  MYFLT  *ftab, fract, v1;

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

int32_t osckki(CSOUND *csound, OSC   *p)
{
  FUNC    *ftp;
  MYFLT   fract, v1, amp, *ar, *ft, *ftab;
  int32_t   phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY((ftp = p->ftp)==NULL)) goto err1;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * CS_SICVT);
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
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
    MYFLT   *ar, amp, *cpsp, fract, v1, *ftab, *ft;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  lobits = ftp->lobits;
  amp = *p->xamp;
  cpsp = p->xcps;
  phs = p->lphs;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  ft = ftp->ftable;
  for (n=offset;n<nsmps;n++) {
    int32_t inc;
    inc = MYFLT2LONG(cpsp[n] * sicvt);
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
  MYFLT    v1, fract, *ar, *ampp, *ftab, *ft;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * CS_SICVT);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    ft = ftp->ftable;
    for (n=offset;n<nsmps;n++) {
      fract = (MYFLT) PFRAC(phs);
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
    MYFLT   v1, fract, *ar, *ampp, *cpsp, *ftab, *ft;
    int32_t   phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ft = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc;
    inc = MYFLT2LONG(cpsp[n] * sicvt);
    fract = (MYFLT) PFRAC(phs);
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
  MYFLT  *ftab, fract;
  int32_t   x0;
  MYFLT   y0, y1, ym1, y2, amp = *p->xamp;

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
    MYFLT frsq = fract*fract;
    MYFLT frcu = frsq*ym1;
    MYFLT t1 = y2 + y0+y0+y0;
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
    MYFLT   fract, amp, *ar, *ftab;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * CS_SICVT);
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
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
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
/*      MYFLT old = (y0 + (y1 - y0) * fract) * amp; */
/*      double x = ((double)(x0-2)+fract)*twopi/32.0; */
/*      MYFLT tr = amp*sin(x); */
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
    MYFLT   *ar, amp, *cpsp, fract, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2;
    MYFLT   sicvt = CS_SICVT;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc;
      inc = MYFLT2LONG(cpsp[n] * sicvt);
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
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
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
  MYFLT   fract, *ar, *ampp, *ftab;
  int32_t    phs, inc, lobits;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32_t   x0;
  MYFLT   y0, y1, ym1, y2;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftab = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  inc = MYFLT2LONG(*p->xcps * CS_SICVT);
  ampp = p->xamp;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    fract = (MYFLT) PFRAC(phs);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1 = y2 + y0+y0+y0;
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
    MYFLT    fract, *ar, *ampp, *cpsp, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t    x0;
    MYFLT    y0, y1, ym1, y2;
    MYFLT    sicvt = CS_SICVT;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;
  ftab = ftp->ftable;
  lobits = ftp->lobits;
  phs = p->lphs;
  ampp = p->xamp;
  cpsp = p->xcps;
  ar = p->sr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset;n<nsmps;n++) {
    int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
    fract = (MYFLT) PFRAC(phs);
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
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1 = y2 + y0+y0+y0;
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


int32_t lposc_set(CSOUND *csound, LPOSC *p)
{
  FUNC   *ftp;
  MYFLT  loop, end, looplength;

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
    end = (MYFLT)p->tablen;
  looplength = end - loop;

  if (*p->iphs >= 0)
    p->phs = *p->iphs;
  while (UNLIKELY(p->phs >= end))
    p->phs -= looplength;
  return OK;
}

int32_t lposca(CSOUND *csound, LPOSC *p)
{
  double  *phs= &p->phs;
  double  si= *p->freq * (p->fsr/CS_ESR);
  MYFLT   *out = p->out,  *amp=p->amp;
  MYFLT   *ft =  p->ftp->ftable, *curr_samp;
  MYFLT   fract;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32   loop, end, looplength /* = p->looplength */ ;

  if ((loop = (int32_t) *p->kloop) < 0) loop=0;
  else if (loop > p->tablen-3) loop = p->tablen-3;
  if ((end = (int32_t) *p->kend) > p->tablen-1 ) end = p->tablen - 1;
  else if (end <= 2) end = 2;
  if (end < loop+2) end = loop + 2;
  looplength = end - loop;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp= ft + (int64_t)*phs;
    fract= (MYFLT)(*phs - (int64_t)*phs);
    out[n] = amp[n] * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    *phs += si;
    while (*phs  >= end) *phs -= looplength;
    while (*phs  < loop) *phs += looplength;
  }
  return OK;
}

int32_t lposc(CSOUND *csound, LPOSC *p)
{
  MYFLT       *out = p->out, *ft = p->ftp->ftable;
  MYFLT       *curr_samp, fract;
  double      phs= p->phs, si= *p->freq * (p->fsr*CS_ONEDSR);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  double      loop, end, looplength;// = p->looplength;
  MYFLT       amp = *p->amp;

  if ((loop = *p->kloop) < 0) loop=0;
  if ((end = *p->kend) > p->tablen || end <=0 )
    end = p->tablen;
  looplength = end - loop;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    curr_samp = ft + (int32)phs;
    fract = (MYFLT)(phs - (double)((int32)phs));
    out[n] = amp * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
    phs += si;
    if (phs >= end) phs -= looplength;
  }
  p->phs = phs;
  return OK;
}

int32_t lposc3(CSOUND *csound, LPOSC *p)
{
  MYFLT       *out = p->out, *ftab = p->ftp->ftable;
  MYFLT       fract;
  double      phs = p->phs, si= *p->freq * (p->fsr*CS_ONEDSR);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  double      loop, end, looplength;// = p->looplength;
  MYFLT       amp = *p->amp;
  int32_t     x0;
  MYFLT       y0, y1, ym1, y2;

  if (UNLIKELY((loop = *p->kloop) < 0)) loop=0;
  if ((end = *p->kend) > p->tablen || end <=0 ) end = p->tablen;
  looplength = end - loop;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    x0    = (int32)phs;
    fract = (MYFLT)(phs - (double)x0);
    x0--;
    if (x0<0) {
      ym1 = ftab[p->tablen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0    = ftab[x0++];
    y1    = ftab[x0++];
    if (x0>p->tablen) y2 = ftab[1]; else y2 = ftab[x0];
    {
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1   = y2 + y0+y0+y0;
      out[n]     = amp * (y0 + FL(0.5)*frcu +
                          fract*(y1 - frcu/FL(6.0) - t1/FL(6.0)
                                 - ym1/FL(3.0)) +
                          frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                          frsq*(FL(0.5)* y1 - y0));
    }
    phs += si;
    while (phs >= end) phs -= looplength;
  }
  p->phs = phs;
  return OK;
}
