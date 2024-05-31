/* VOSIM.C: VOice SIMulation implementation

   Copyright 2008 rasmus ekman

   rasmus ekman March 13, 2008, for Csound.

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


/* Programmer note:
 * Pre- and post-conditions are noted at function level.
 * These are essential for the code to work at all.
 * There are some complications because we accept weird user input:
 * (a) kfund < 0: This is forced to positive - no point in "backward" events.
 * (b) kform == 0: This leads to infinite length pulse, ie silence.
 * (c) kform < 0: Table is read backward.
 *     If table is symmetric, kform and -kform should give bit-identical outputs.
 *
 * Don't fiddle with the code unless you understand how the table read
 * params pulsephs and pulseinc are used both to read table and to indicate that
 * a new pulse should start, and that pulseinc can be negative.
 */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <math.h>
#include <limits.h>

typedef struct {
  OPDS h;
  MYFLT *ar, *amp, *kfund, *kform, *kdamp, *knofpulse, *kpulsemul,
    *iftab, *iskip;
  FUNC *ftable;
  int32 timrem;    /* samples left of event */
  int32 pulstogo;  /* count of pulses to produce in burst */
  int32 pulsephs;  /* index into table of this pulse (= MAXLEN / kform) */
  int32 pulseinc;  /* increment in table of pulse */
  MYFLT pulseamp;  /* amp of current pulse */
  MYFLT ampdecay;  /* subtract from amp on new pulse */
  MYFLT lenfact;   /* increase length of next pulse */
  int32 floatph;
  double pulsephsf;  /* float index */
  MYFLT  pulseincf;  /*  float incr */
} VOSIM;


/* Post: unless skipping init, timrem == 0 */
int32_t vosimset(CSOUND* csound, VOSIM *p)
{
  if (*p->iskip)
    return OK;


  p->ftable = csound->FTFind(csound, p->iftab);
  if (UNLIKELY(p->ftable == NULL)) {
    return csound->InitError(csound,  "%s", Str("vosim: pulse table not found"));
  }
  p->floatph = !(IS_POW_TWO(p->ftable->flen));
  p->pulsephsf = p->pulseincf = FL(0.0);
  p->timrem = p->pulstogo = p->pulsephs = p->pulseinc = 0;
  p->pulseamp = p->ampdecay = p->lenfact = FL(0.0);
  return OK;
}


/* Pre: timrem == 0.
 * Post:
 *    IF kform >= 0, pulsephs >= FMAXLEN.
 *    ELSE pulsephs < 0.
 *    timrem > 0.
 */
void vosim_event(CSOUND* csound, VOSIM *p)
{
  MYFLT fundabs = FABS(*p->kfund);
  /* count of pulses, (+1 since decr at start of pulse) */
  p->pulstogo = 1+(int32)*p->knofpulse;
  if (UNLIKELY(fundabs == FL(0.0))) {                /* infinitely long event */
    p->timrem = INT_MAX;
    csound->Warning(csound, "%s",
                    Str("vosim: zero kfund. 'Infinite' length event generated."));
  }
  else {
    p->timrem = (int32)(CS_ESR / fundabs);
    if (UNLIKELY(p->timrem == 0)) {
      p->timrem = CS_KSMPS;
      p->pulstogo = 0;
      csound->Warning(csound,
                      Str("vosim: kfund (%f) > sr. Generating ksmps silence."),
                      *p->kfund);
    }
  }
  if(p->floatph) {
    p->pulseincf = *p->kform * CS_ONEDSR;
    p->pulsephsf = (p->pulseinc >= 0) ? 1. : -1.; 
  } else {  
    p->pulseinc = (int32)(*p->kform * CS_SICVT);
    p->pulsephs = (p->pulseinc >= 0)? MAXLEN : -1;   /* starts a new pulse */
  }
  p->ampdecay = *p->kdamp;
  /* increase initial amp, since it's reduced at pulse start */
  p->pulseamp = *p->amp + p->ampdecay;
  /* if negative, table is read alternately back-/forward */
  p->lenfact  = *p->kpulsemul;
  /* reduce table rate, since it's increased at pulse start */
  if (p->lenfact != FL(0.0)) {
    if(p->floatph)  p->pulseincf /= p->lenfact;
    else p->pulseinc /= p->lenfact;
  }
}


/* Pre: pulsephs >= FMAXLEN OR pulsephs < 0.
 * Post:
 *    pulstogo is decremented or zero.
 *    0 <= pulsephs < FMAXLEN.
 */
void vosim_pulse(CSOUND* csound, VOSIM *p)
{
  IGN(csound);
  int32 pulselen;
  if(p->floatph) {
    p->pulsephsf = PHMOD1(p->pulsephsf);
    p->pulseincf *= p->lenfact;
    pulselen = (p->pulseincf != FL(0.0))?
      (int32) FABS(1. / p->pulseincf) : INT_MAX;
  } else {
    p->pulsephs &= PHMASK;
    p->pulseinc *= p->lenfact;
    /* If pulse can't fit in remaining event time, skip and generate silence */
    pulselen = (p->pulseinc != FL(0.0))?
      (int32)FABS(FMAXLEN / p->pulseinc) : INT_MAX;
  }
  if (p->pulstogo-- <= 0 || pulselen > p->timrem) {
    p->pulstogo = 0;
  }
  p->pulseamp -= p->ampdecay;
}


int32_t vosim(CSOUND* csound, VOSIM *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *ar = p->ar;
  MYFLT *ftdata, pulsephsf = p->pulsephsf, pulseincf = p->pulseincf;
  int32  lobits, floatph = p->floatph, flen;
  MYFLT pulseamp = p->pulseamp;

  FUNC *ftp = p->ftable;
  if (UNLIKELY(ftp == NULL)) goto err1;
  ftdata = ftp->ftable;
  lobits = ftp->lobits;
  flen = ftp->flen;

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    /* new event? */
    if (p->timrem == 0)
      vosim_event(csound, p);

    /* new pulse? */
    if (p->pulsephs >= MAXLEN || p->pulsephs < 0)
      vosim_pulse(csound, p);

    if (p->pulstogo > 0) {
      /* produce one sample */
      if(floatph) {
        ar[n] = *(ftdata + (size_t) (PHMOD1(pulsephsf)*flen)) * pulseamp;
        pulsephsf += pulseincf;
      } else {
        p->pulsephs &= PHMASK;
        ar[n] = *(ftdata + (p->pulsephs >> lobits)) * pulseamp;
        p->pulsephs += p->pulseinc;
      }
      --p->timrem;
    }
    else {
      /* silence after last pulse in burst: */
      /* bypass regular synthesis and fill output with zeros */
      while (p->timrem && n<nsmps) {
        ar[n] = FL(0.0);
        --p->timrem;
        n++;
      }
      n--;
    }
  }
  p->pulsephsf = pulsephsf;
  return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("vosim: not initialised"));
}


/* ar   vosim   kamp, kFund, kForm, kDamp, kPulseCount, kPulseFactor,
   ifn [, iskip] */

#define S(x)    sizeof(x)

static OENTRY vosim_localops[] = {
  { "vosim", S(VOSIM), TR,  "a", "kkkkkkio", (SUBR)vosimset, (SUBR)vosim }
};


LINKAGE_BUILTIN(vosim_localops)
