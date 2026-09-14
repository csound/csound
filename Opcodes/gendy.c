/*
  gendy.c:

  Implementation of the dynamic stochastic synthesis generator
  conceived by 63 Xenakis.

  Based on Nick Collins's Gendy1 ugen (SuperCollider).

  Copyright (c) Tito Latini, 2012

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

typedef struct {
  OPDS    h;
  MYFLT   *out, *kamp, *ampdist, *durdist, *adpar, *ddpar;
  MYFLT   *minfreq, *maxfreq, *ampscl, *durscl, *initcps, *knum;
  MYFLT   phase, amp, nextamp, dur, speed;
  int32   index, rand, points;
  AUXCH   memamp, memdur;
} GENDY;

typedef struct {
  OPDS    h;
  MYFLT   *out, *kamp, *ampdist, *durdist, *adpar, *ddpar;
  MYFLT   *minfreq, *maxfreq, *ampscl, *durscl;
  MYFLT   *kcurveup, *kcurvedown, *initcps, *knum;
  MYFLT   phase, amp, nextamp, dur, speed;
  int32   index, rand, points;
  AUXCH   memamp, memdur;
} GENDYX;

typedef struct {
  OPDS    h;
  MYFLT   *out, *kamp, *ampdist, *durdist, *adpar, *ddpar;
  MYFLT   *minfreq, *maxfreq, *ampscl, *durscl, *initcps, *knum;
  MYFLT   amp, nextamp, dur, slope, midpnt, curve;
  int32   phase, index, rand, points;
  AUXCH   memamp, memdur;
} GENDYC;

#define BIPOLAR      0x7FFFFFFF    /* Constant to make bipolar */
#define dv2_31       (FL(4.656612873077392578125e-10))
#define GENDYMAXCPS  8192          /* Max number of control points */

#define GENDY_BIPOLAR(rnd)                                                \
  ((MYFLT)((int64_t)(uint32_t)(rnd) * 2 - BIPOLAR) * dv2_31)

#define GENDY_POINTS(value)                                               \
  ((value) >= FL(1.0)                                                     \
   ? ((value) > GENDYMAXCPS ? GENDYMAXCPS : (int32_t)(value))            \
   : 12)

#define GENDY_KNUM(value, points)                                         \
  ((value) >= FL(1.0) && (value) <= (MYFLT)(points)                       \
   ? (int32_t)(value) : (points))

#define GENDY_SET_CUBIC_PHASE(result, frequency, sample_rate)             \
  do {                                                                    \
    double frequency_ = (double)(frequency);                              \
    double phase_;                                                        \
    if (UNLIKELY(!(frequency_ > 0.001)))                                  \
      frequency_ = 0.001;                                                 \
    phase_ = (double)(sample_rate) / frequency_;                           \
    if (UNLIKELY(!(phase_ < (double)INT32_MAX)))                           \
      (result) = INT32_MAX;                                               \
    else if (UNLIKELY(phase_ < 2.0))                                      \
      (result) = 2;                                                       \
    else                                                                  \
      (result) = (int32_t)phase_;                                         \
  } while (0)

#define GENDY_SET_SPEED(result, minfreq, maxfreq, dur, onedsr, points)    \
  do {                                                                    \
    MYFLT speed_ = ((minfreq) + ((maxfreq) - (minfreq)) * (dur))          \
      * (onedsr) * (points);                                              \
    if (UNLIKELY(!(speed_ > FL(0.0))))                                    \
      speed_ = FL(0.0);                                                   \
    else if (UNLIKELY(speed_ > (MYFLT)(points)))                          \
      speed_ = (MYFLT)(points);                                           \
    (result) = speed_;                                                    \
  } while (0)

static MYFLT gendy_distribution(CSOUND *csound, MYFLT which, MYFLT a, int32 rnd)
{
  IGN(csound);
  int32_t selection;
  MYFLT   c, r;
  if (which >= FL(0.0) && which < FL(7.0))
    selection = (int32_t)which;
  else
    selection = 0;
  if (a > FL(1.0))
    a = FL(1.0);
  else if (a < FL(0.0001))
    a = FL(0.0001);
  switch (selection) {
  case 0: // linear
    break;
  case 1: // cauchy
    c = ATAN(FL(10.0)*a);
    r = GENDY_BIPOLAR(rnd);
    r = (FL(1.0)/a) * TAN(c*r) * FL(0.1);
    return r;
  case 2: // logist
    c = FL(0.5)+(FL(0.499)*a);
    c = LOG((FL(1.0)-c)/c);
    r = (((MYFLT)rnd*dv2_31 - FL(0.5)) * FL(0.998)*a) + FL(0.5);
    r = LOG((FL(1.0)-r)/r)/c;
    return r;
  case 3: // hyperbcos
    c = TAN(FL(1.5692255)*a);
    r = TAN(FL(1.5692255)*a*(MYFLT)rnd*dv2_31)/c;
    r = (LOG(r*FL(0.999)+FL(0.001)) * FL(-0.1447648))*FL(2.0) - FL(1.0);
    return r;
  case 4: // arcsine
    c = SIN(FL(1.5707963)*a);
    r = SIN(PI_F * ((MYFLT)rnd*dv2_31 - FL(0.5))*a)/c;
    return r;
  case 5: // expon
    c = LOG(FL(1.0)-(FL(0.999)*a));
    r = (MYFLT)rnd*dv2_31*FL(0.999)*a;
    r = (LOG(FL(1.0)-r)/c)*FL(2.0) - FL(1.0);
    return r;
  case 6: // external sig
    return a*FL(2.0) - FL(1.0);
  default:
    break;
  }
  r = GENDY_BIPOLAR(rnd);
  return r;
}

static int32_t gendyset(CSOUND *csound, GENDY *p)
{
  int32_t     i;
  MYFLT   *memamp, *memdur;
  p->amp     = FL(0.0);
  p->nextamp = FL(0.0);
  p->phase   = FL(1.0);
  p->speed   = FL(100.0);
  p->index   = 0;
  p->points = GENDY_POINTS(*p->initcps);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memamp);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memdur);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  p->rand = csound->Rand31(csound->RandSeed31(csound));
  for (i=0; i < p->points; i++) {
    p->rand = csound->Rand31(&p->rand);
    memamp[i] = GENDY_BIPOLAR(p->rand);
    p->rand = csound->Rand31(&p->rand);
    memdur[i] = (MYFLT)p->rand * dv2_31;
  }
  return OK;
}

static int32_t kgendy(CSOUND *csound, GENDY *p)
{
  int32_t     knum;
  MYFLT   *memamp, *memdur, minfreq, maxfreq, dist;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  while (p->phase >= FL(1.0)) {
    int32_t index = p->index;
    p->phase -= FL(1.0);
    p->index = index = (index+1) % knum;
    p->amp = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
    p->nextamp = memamp[index] + *p->ampscl * dist;
    /* amplitude variations within the boundaries of a mirror */
    if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
      if (p->nextamp < FL(0.0))
        p->nextamp += FL(4.0);
      p->nextamp = FMOD(p->nextamp, FL(4.0));
      if (p->nextamp > FL(1.0)) {
        p->nextamp =
          (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
      }
    }
    memamp[index] = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
    p->dur = memdur[index] + *p->durscl * dist;
    /* time variations within the boundaries of a mirror */
    if (p->dur > FL(1.0))
      p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
    else if (p->dur < FL(0.0))
      p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
    memdur[index] = p->dur;
    GENDY_SET_SPEED(p->speed, minfreq, maxfreq, p->dur, CS_ONEDSR, knum);
  }
  *p->out = *p->kamp * ((FL(1.0) - p->phase) * p->amp + p->phase * p->nextamp);
  p->phase += p->speed;
  return OK;
}

static int32_t agendy(CSOUND *csound, GENDY *p)
{
  int32_t     knum;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT   *out, *memamp, *memdur, minfreq, maxfreq, dist;
  out  = p->out;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    while (p->phase >= FL(1.0)) {
      int32_t index = p->index;
      p->phase -= FL(1.0);
      p->index = index = (index+1) % knum;
      p->amp = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
      p->nextamp = memamp[index] + *p->ampscl * dist;
      if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
        if (p->nextamp < FL(0.0))
          p->nextamp += FL(4.0);
        p->nextamp = FMOD(p->nextamp, FL(4.0));
        if (p->nextamp > FL(1.0)) {
          p->nextamp =
            (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
        }
      }
      memamp[index] = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
      p->dur = memdur[index] + *p->durscl * dist;
      if (p->dur > FL(1.0))
        p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
      else if (p->dur < FL(0.0))
        p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
      memdur[index] = p->dur;
      GENDY_SET_SPEED(p->speed, minfreq, maxfreq, p->dur, CS_ONEDSR, knum);
    }
    out[n] = *p->kamp * ((FL(1.0) - p->phase) * p->amp + p->phase * p->nextamp);
    p->phase += p->speed;
  }
  return OK;
}

static int32_t gendyxset(CSOUND *csound, GENDYX *p)
{
  int32_t     i;
  MYFLT   *memamp, *memdur;
  p->amp     = FL(0.0);
  p->nextamp = FL(0.0);
  p->phase   = FL(1.0);
  p->speed   = FL(100.0);
  p->index   = 0;
  p->points = GENDY_POINTS(*p->initcps);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memamp);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memdur);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  p->rand = csound->Rand31(csound->RandSeed31(csound));

  for (i=0; i < p->points; i++) {
    p->rand   = (int32)csound->Rand31(&p->rand);
    memamp[i] = GENDY_BIPOLAR(p->rand);
    p->rand   = csound->Rand31(&p->rand);
    memdur[i] = (MYFLT)p->rand * dv2_31;
  }
  return OK;
}

static int32_t kgendyx(CSOUND *csound, GENDYX *p)
{
  int32_t     knum;
  MYFLT   *memamp, *memdur, minfreq, maxfreq, dist;
  MYFLT   curve, curveup, curvedown;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  curveup = (*p->kcurveup > FL(0.0) ? *p->kcurveup : FL(0.0));
  curvedown = (*p->kcurvedown > FL(0.0) ? *p->kcurvedown : FL(0.0));
  while (p->phase >= FL(1.0)) {
    int32_t index = p->index;
    p->phase -= FL(1.0);
    p->index = index = (index+1) % knum;
    p->amp = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
    p->nextamp = memamp[index] + *p->ampscl * dist;
    if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
      if (p->nextamp < FL(0.0))
        p->nextamp += FL(4.0);
      p->nextamp = FMOD(p->nextamp, FL(4.0));
      if (p->nextamp > FL(1.0)) {
        p->nextamp =
          (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
      }
    }
    memamp[index] = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
    p->dur = memdur[index] + *p->durscl * dist;
    if (p->dur > FL(1.0))
      p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
    else if (p->dur < FL(0.0))
      p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
    memdur[index] = p->dur;
    GENDY_SET_SPEED(p->speed, minfreq, maxfreq, p->dur, CS_ONEDSR, knum);
  }
  curve = ((p->nextamp - p->amp) > FL(0.0) ? curveup : curvedown);
  *p->out = *p->kamp * (p->amp + POWER(p->phase, curve) * (p->nextamp - p->amp));
  p->phase += p->speed;
  return OK;
}

static int32_t agendyx(CSOUND *csound, GENDYX *p)
{
  int32_t     knum;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT   *out, *memamp, *memdur, minfreq, maxfreq, dist;
  MYFLT   curve, curveup, curvedown;
  out  = p->out;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  curveup = (*p->kcurveup > FL(0.0) ? *p->kcurveup : FL(0.0));
  curvedown = (*p->kcurvedown > FL(0.0) ? *p->kcurvedown : FL(0.0));
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    while (p->phase >= FL(1.0)) {
      int32_t index = p->index;
      p->phase -= FL(1.0);
      p->index = index = (index+1) % knum;
      p->amp = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
      p->nextamp = memamp[index] + *p->ampscl * dist;
      if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
        if (p->nextamp < FL(0.0))
          p->nextamp += FL(4.0);
        p->nextamp = FMOD(p->nextamp, FL(4.0));
        if (p->nextamp > FL(1.0)) {
          p->nextamp =
            (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
        }
      }
      memamp[index] = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
      p->dur = memdur[index] + *p->durscl * dist;
      if (p->dur > FL(1.0))
        p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
      else if (p->dur < FL(0.0))
        p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
      memdur[index] = p->dur;
      GENDY_SET_SPEED(p->speed, minfreq, maxfreq, p->dur, CS_ONEDSR, knum);
    }
    curve = ((p->nextamp - p->amp) > FL(0.0) ? curveup : curvedown);
    out[n] = *p->kamp * (p->amp + POWER(p->phase, curve) * (p->nextamp - p->amp));
    p->phase += p->speed;
  }
  return OK;
}

/* version with cubic interpolation based from Bhob Rainey's Gendy4 */
static int32_t gendycset(CSOUND *csound, GENDYC *p)
{
  int32_t     i;
  MYFLT   *memamp, *memdur;
  p->amp     = FL(0.0);
  p->nextamp = FL(0.0);
  p->slope   = FL(0.0);
  p->midpnt  = FL(0.0);
  p->curve   = FL(0.0);
  p->phase   = 0;
  p->index   = 0;
  p->points = GENDY_POINTS(*p->initcps);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memamp);
  csound->AuxAlloc(csound, p->points*sizeof(MYFLT), &p->memdur);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  p->rand = csound->Rand31(csound->RandSeed31(csound));
  for (i=0; i < p->points; i++) {
    p->rand = csound->Rand31(&p->rand);
    memamp[i] = GENDY_BIPOLAR(p->rand);
    p->rand = csound->Rand31(&p->rand);
    memdur[i] = (MYFLT)p->rand * dv2_31;
  }
  return OK;
}

static int32_t kgendyc(CSOUND *csound, GENDYC *p)
{
  int32_t     knum;
  MYFLT   *memamp, *memdur, minfreq, maxfreq, dist;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  if (p->phase <= 0) {
    int32_t     index = p->index;
    MYFLT   fphase, next_midpnt;
    p->index = index = (index+1) % knum;
    p->amp = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
    p->nextamp = memamp[index] + *p->ampscl * dist;
    if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
      if (p->nextamp < FL(0.0))
        p->nextamp += FL(4.0);
      p->nextamp = FMOD(p->nextamp, FL(4.0));
      if (p->nextamp > FL(1.0)) {
        p->nextamp =
          (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
      }
    }
    next_midpnt = (p->amp + p->nextamp) * 0.5;
    memamp[index] = p->nextamp;
    p->rand = csound->Rand31(&p->rand);
    dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
    p->dur = memdur[index] + *p->durscl * dist;
    if (p->dur > FL(1.0))
      p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
    else if (p->dur < FL(0.0))
      p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
    memdur[index] = p->dur;
    fphase = (minfreq + (maxfreq - minfreq) * p->dur) * knum;
    GENDY_SET_CUBIC_PHASE(p->phase, fphase, CS_ESR);
    p->curve = FL(2.0) * (next_midpnt - p->midpnt - p->phase * p->slope);
    fphase = (MYFLT)p->phase;
    p->curve = p->curve / (fphase * fphase + fphase);
  }
  p->phase--;
  *p->out = *p->kamp * p->midpnt;
  p->slope  += p->curve;
  p->midpnt += p->slope;

  return OK;
}

static int32_t agendyc(CSOUND *csound, GENDYC *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  int32_t     knum;
  uint32_t    n = offset;
  uint32_t    remain = CS_KSMPS-offset-early;
  MYFLT   *out, *memamp, *memdur, minfreq, maxfreq, dist;
  out  = p->out;
  knum = GENDY_KNUM(*p->knum, p->points);
  memamp  = p->memamp.auxp;
  memdur  = p->memdur.auxp;
  minfreq = *p->minfreq;
  maxfreq = *p->maxfreq;
  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    memset(&out[remain+offset], '\0', early*sizeof(MYFLT));
  }
  while (remain > 0) {
    uint32_t end, nsmps;
    if (p->phase <= 0) {
      int32_t     index = p->index;
      MYFLT   fphase, next_midpnt;
      p->index = index = (index+1) % knum;
      p->amp = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->ampdist, *p->adpar, p->rand);
      p->nextamp = memamp[index] + *p->ampscl * dist;
      if (p->nextamp < FL(-1.0) || p->nextamp > FL(1.0)) {
        if (p->nextamp < FL(0.0))
          p->nextamp += FL(4.0);
        p->nextamp = FMOD(p->nextamp, FL(4.0));
        if (p->nextamp > FL(1.0)) {
          p->nextamp =
            (p->nextamp < FL(3.0) ? FL(2.0) - p->nextamp : p->nextamp - FL(4.0));
        }
      }
      next_midpnt = (p->amp + p->nextamp) * 0.5;
      memamp[index] = p->nextamp;
      p->rand = csound->Rand31(&p->rand);
      dist = gendy_distribution(csound, *p->durdist, *p->ddpar, p->rand);
      p->dur = memdur[index] + *p->durscl * dist;
      if (p->dur > FL(1.0))
        p->dur = FL(2.0) - FMOD(p->dur, FL(2.0));
      else if (p->dur < FL(0.0))
        p->dur = FL(2.0) - FMOD(p->dur + FL(2.0), FL(2.0));
      memdur[index] = p->dur;
      fphase = (minfreq + (maxfreq - minfreq) * p->dur) * knum;
      GENDY_SET_CUBIC_PHASE(p->phase, fphase, CS_ESR);
      p->curve = FL(2.0) * (next_midpnt - p->midpnt - p->phase * p->slope);
      fphase = (MYFLT)p->phase;
      p->curve = p->curve / (fphase * fphase + fphase);
    }
    nsmps = (remain < (uint32_t)p->phase ? remain : (uint32_t)p->phase);
    remain   -= nsmps;
    p->phase -= nsmps;
    end = n + nsmps;
    for (; n<end; n++) {
      out[n] = *p->kamp * p->midpnt; /* JPff-- was *out++ but that ignores
                                        sample accurate */
      p->slope  += p->curve;
      p->midpnt += p->slope;
    }
  }
  return OK;
}

static OENTRY gendy_localops[] = {
  { "gendy.k",  sizeof(GENDY),  0, "k", "kkkkkkkkkoO",
    (SUBR)gendyset,  (SUBR)kgendy,  (SUBR)NULL           },
  { "gendy.a",  sizeof(GENDY),  0, "a", "kkkkkkkkkoO",
    (SUBR)gendyset,    (SUBR)agendy         },
  { "gendyx.k", sizeof(GENDYX), 0, "k", "kkkkkkkkkkkoO",
    (SUBR)gendyxset, (SUBR)kgendyx, (SUBR)NULL           },
  { "gendyx.a", sizeof(GENDYX), 0, "a", "kkkkkkkkkkkoO",
    (SUBR)gendyxset,    (SUBR)agendyx        },
  { "gendyc.k", sizeof(GENDYC), 0, "k", "kkkkkkkkkoO",
    (SUBR)gendycset, (SUBR)kgendyc, (SUBR)NULL           },
  { "gendyc.a", sizeof(GENDYC), 0, "a", "kkkkkkkkkoO",
    (SUBR)gendycset,    (SUBR)agendyc        }
};

LINKAGE_BUILTIN(gendy_localops)
