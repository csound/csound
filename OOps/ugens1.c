/*
  gens1.c:

  Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"         /*                      UGENS1.C        */
#include "ugens1.h"
#include <math.h>

#define FHUND (FL(100.0))


int32_t linset(CSOUND *csound, LINE *p)
{
  cs_double       dur;
  if (LIKELY((dur = *p->idur) > FL(0.0))) {
    p->incr = (*p->ib - *p->ia) / dur * CS_ONEDSR;
    p->kincr = p->incr*CS_KSMPS;
    p->val = *p->ia;
  }
  return OK;
}

int32_t kline(CSOUND *csound, LINE *p)
{
  IGN(csound);
  *p->xr = p->val;            /* rslt = val   */
  p->val += p->kincr;          /* val += incr  */
  return OK;
}

int32_t aline(CSOUND *csound, LINE *p)
{
  IGN(csound);
  cs_double val, inc;
  cs_float *ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  ar = p->xr;
  val = p->val;
  inc = p->incr;

  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }

  //p->val += inc;/* nxtval = val + inc */
  //inc /= (nsmps - offset);
  for (n=offset; n<nsmps; n++) {
    ar[n] = (cs_float)val;
    val += inc;       /* interp val for ksmps */
  }
  p->val = val;
  return OK;
}

int32_t expset(CSOUND *csound, EXPON *p)
{
  cs_double       dur, a, b;
  //printf("kr = %f , 1/kr = %f \n",CS_EKR, CS_ONEDKR);
  if (LIKELY((dur = *p->idur) > FL(0.0) )) {
    a = *p->ia;
    b = *p->ib;
    if (LIKELY((a * b) > FL(0.0))) {
      p->mlt = POWER(b/a, CS_ONEDSR/dur);
      p->kmlt = POWER(b/a, CS_ONEDKR/dur);
      p->val = a;
    }
    else if (a == FL(0.0))
      return csound->InitError(csound, Str("arg1 is zero"));
    else if (b == FL(0.0))
      return csound->InitError(csound, Str("arg2 is zero"));
    else return csound->InitError(csound, Str("unlike signs"));
  }
  return OK;
}

int32_t kexpon(CSOUND *csound, EXPON *p)
{
  IGN(csound);
  *p->xr = p->val;            /* rslt = val   */
  p->val *= p->kmlt;           /* val *= mlt  */
  return OK;
}

int32_t expon(CSOUND *csound, EXPON *p)
{
  IGN(csound);
  cs_double val, mlt;//, inc;//, nxtval;
  cs_float *ar;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  val = p->val;
  mlt = p->mlt;
  // nxtval = val * mlt;
  ar = p->xr;
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(cs_float));
  }
  //inc = nxtval - val;
  //inc /= (nsmps - offset);   /* increment per sample */
  for (n=offset; n<nsmps; n++) {
    ar[n] = (cs_float)val;
    val *= mlt;               /* interp val for ksmps */
  }
  // p->val = nxtval;            /* store next value */
  p->val = val;
  return OK;
}

int32_t lsgset(CSOUND *csound, LINSEG *p)
{
  SEG *segp;
  int32_t nsegs;
  cs_float       **argp;
  cs_double val;

  if (UNLIKELY(!(p->INOCOUNT & 1))) {
    return csound->InitError(csound, Str("incomplete number of input arguments"));
  }

  /* count segs & alloc if nec */
  nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
  /* VL: 29.05.17 allocating one extra empty segment
     so that the breakpoint version of this opcode
     can work properly without a fencepost bug */
  if (UNLIKELY((p->cursegp = (SEG *) p->auxch.auxp) == NULL ||
               (nsegs+1)*sizeof(SEG) < (uint32_t)p->auxch.size)) {
    csound->AuxAlloc(csound, (int32_t)(nsegs+1)*sizeof(SEG), &p->auxch);
    p->cursegp = (SEG *) p->auxch.auxp;
    segp = p->cursegp + 1; /* point to first seg */
    p->cursegp->cnt = 0;   /* zero duration of segment 0  */
    segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
  } else segp = p->cursegp + 1; /* point to first seg */
  argp = p->argums;
  val = (cs_double)**argp++;
  if (UNLIKELY(**argp <= FL(0.0)))  return OK;    /* if idur1 <= 0, skip init  */
  p->curval = val;
  p->curcnt = 0;
  /* VL: 29.05.17 this was causing a fencepost error in
     the breakpoint version (linsegb) */
  //p->cursegp = segp - 1;          /* else setup null seg0 */
  p->segsrem = nsegs + 1;
  do {                                /* init each seg ..  */
    cs_double dur = (cs_double)**argp++;
    segp->nxtpt = (cs_double)**argp++;
    if (UNLIKELY((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) < 0))
      segp->cnt = 0;
    if (UNLIKELY((segp->acnt = (int32_t)(dur * CS_ESR + FL(0.5))) < 0))
      segp->acnt = 0;
    segp++;
  } while (--nsegs);
  p->xtra = -1;

  return OK;

}

int32_t lsgset_bkpt(CSOUND *csound, LINSEG *p)
{
  int32_t cnt = 0, acnt = 0, bkpt = 0;
  int32_t nsegs;
  int32_t n;
  SEG *segp;
  n = lsgset(csound, p);
  if (UNLIKELY(n!=0)) return n;
  nsegs = p->segsrem;
  segp = p->cursegp;
  do {
    if (UNLIKELY(cnt > segp->cnt || acnt > segp->acnt))
      return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
    segp->cnt -= cnt;
    cnt += segp->cnt;
    segp->acnt -= acnt;
    acnt += segp->acnt;
    segp++;
    bkpt++;
  } while (--nsegs);
  return OK;
}


int32_t klnseg(CSOUND *csound, LINSEG *p)
{
  IGN(csound);
  *p->rslt = p->curval;               /* put the cur value    */
  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */
  if (UNLIKELY(p->segsrem)) {                   /* done if no more segs */
    if (--p->curcnt <= 0) {           /* if done cur segment  */
      SEG *segp = p->cursegp;
      if (UNLIKELY(!(--p->segsrem)))  {
        p->curval = segp->nxtpt;      /* advance the cur val  */
        return OK;
      }
      p->cursegp = ++segp;            /*   find the next      */
      if (UNLIKELY(!(p->curcnt = segp->cnt))) { /*   nonlen = discontin */
        p->curval = segp->nxtpt;      /*   poslen = new slope */
        /*          p->curval += p->curinc;  ??????? */
        return OK;
      }
      else {
        p->curinc = (segp->nxtpt - p->curval) / segp->cnt;
        p->curval += p->curinc;
        return OK;
      }
    }
    if (p->curcnt<10)         /* This is a fiddle to get rounding right!  */
      p->curinc = (p->cursegp->nxtpt - p->curval) / p->curcnt; /* recalc */
    p->curval += p->curinc;           /* advance the cur val  */
  }
  return OK;
 err1:
  return csound->InitError(csound, Str("linseg not initialised (krate)\n"));
}

int32_t linseg(CSOUND *csound, LINSEG *p)
{
  cs_double val, ainc;
  cs_float *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;  /* RWD fix */

  val = p->curval;                      /* sav the cur value    */
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }

  for (n=offset; n<nsmps; n++) {
    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        SEG *segp = p->cursegp;
      chk1:
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          val = p->curval = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        p->cursegp = ++segp;              /*   else find the next */
        //printf("newseg: nxtpt=%f acnt=%d\n", segp->nxtpt, segp->acnt);
        if (UNLIKELY(!(p->curcnt = segp->acnt))) {
          val = p->curval = segp->nxtpt;  /* nonlen = discontin */
          goto chk1;
        }                                 /*   poslen = new slope */
        p->curainc = (segp->nxtpt - val) / segp->acnt;
        // p->curainc = p->curinc * CS_ONEDKSMPS;
      }
      // p->curval = val + p->curinc;        /* advance the cur val  */
      if (UNLIKELY((ainc = p->curainc) == FL(0.0)))
        goto putk;
      rs[n] = (cs_float)val;
      val += ainc;
    }
    else {                      /* no more segments */
    putk:
      rs[n] = (cs_float)val;
    }
  }
  p->curval = val;
  return OK;
 err1:

  return csound->PerfError(csound, &(p->h),
                           Str("linseg: not initialised (arate)\n"));
}

/* The ADSR family shares timing and release handling at both rates. */

enum { ADSR_DELAY, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE,
       ADSR_DONE };

static int32_t adsr_count(CSOUND *csound, cs_double duration, cs_double rate,
                          int32_t *count)
{
  cs_double n = floor(duration * rate + 0.5);
  if (UNLIKELY(!isfinite(duration) || duration < 0.0 ||
               !isfinite(n) || n > INT_MAX))
    return csound->InitError(csound,
                            Str("ADSR: duration is negative or out of range"));
  *count = (int32_t)n;
  return OK;
}

static int32_t adsr_init(CSOUND *csound, ADSR *p, int32_t midi,
                         int32_t exponential)
{
  cs_float **args = p->argums;
  cs_double rate = IS_ASIG_ARG(p->rslt) ? CS_ESR : CS_EKR;
  cs_double length = p->h.insdshead->p3.value;
  int32_t counts[5], release, override = -1, length_count, i;
  const int32_t inputs[4] = {4, 0, 1, 3};
  const int32_t stages[4] = {ADSR_DELAY, ADSR_ATTACK, ADSR_DECAY, ADSR_RELEASE};

  /* Preserve the historical negative-attack skip, but initialize zero. */
  if (isfinite(*args[0]) && *args[0] < FL(0.0))
    return OK;
  if (UNLIKELY(!isfinite(*args[2]) || (exponential && *args[2] < FL(0.0))))
    return csound->InitError(csound, Str("ADSR: invalid sustain level"));
  for (i = 0; i < 4; i++) {
    if (adsr_count(csound, *args[inputs[i]], rate, &counts[stages[i]]) != OK)
      return NOTOK;
  }
  counts[ADSR_SUSTAIN] = 0;
  if (!midi && length > 0.0) {
    int64_t sustain;
    if (adsr_count(csound, length, rate, &length_count) != OK)
      return NOTOK;
    sustain = (int64_t)length_count - counts[ADSR_DELAY] - counts[ADSR_ATTACK]
              - counts[ADSR_DECAY] - counts[ADSR_RELEASE];
    if (sustain < 0)
      csound->Warning(csound, Str("length of ADSR note too short"));
    else
      counts[ADSR_SUSTAIN] = (int32_t)sustain;
  }
  if (midi) {
    if (!isfinite(*args[5]))
      return csound->InitError(csound, Str("ADSR: invalid release override"));
    if (adsr_count(csound, *args[3], CS_EKR, &release) != OK)
      return NOTOK;
    if (*args[5] >= FL(0.0)) {
      if (adsr_count(csound, *args[5], CS_EKR, &override) != OK)
        return NOTOK;
      if (override > release)
        release = override;
    }
    if (release > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = release;
  }
  memcpy(p->counts, counts, sizeof(counts));
  p->scale = IS_ASIG_ARG(p->rslt) ? CS_KSMPS : 1;
  p->xtra = override;
  p->midi = midi;
  p->exponential = exponential;
  p->hold = midi || length <= 0.0;
  p->sustain = *args[2];
  p->stage = -1;
  p->remaining = 0;
  p->value = p->target = p->increment = 0.0;
  p->multiplier = 1.0;
  p->initialized = 1;
  return OK;
}

static void adsr_stage(ADSR *p, int32_t stage, int64_t count)
{
  p->stage = stage;
  p->remaining = count;
  p->target = stage == ADSR_ATTACK ? 1.0 :
    (stage == ADSR_DECAY || stage == ADSR_SUSTAIN ? p->sustain : 0.0);
  p->increment = 0.0;
  p->multiplier = 1.0;
  if (count == 0) {
    p->value = p->target;
    return;
  }
  if (p->exponential && stage != ADSR_DELAY && stage != ADSR_SUSTAIN) {
    cs_double logtarget;
    /* Retain the exponential attack's positive starting floor. */
    if (stage == ADSR_ATTACK)
      p->value = 0.001;
    if (p->value == 0.0 || count == 1)
      return;
    if (p->target == 0.0) {
      /* Do not turn a silent or very quiet release into a rising ramp. */
      logtarget = p->value > 0.001 ? log(0.001) : log(p->value) + log(0.001);
    }
    else
      logtarget = log(p->target);
    p->multiplier = exp((logtarget - log(p->value)) / (cs_double)count);
  }
  else
    p->increment = (p->target - p->value) / (cs_double)count;
}

static cs_double adsr_next(ADSR *p)
{
  cs_double value;
  if (!p->initialized)
    return 0.0;
  if (p->midi && p->h.insdshead->relesing && p->stage < ADSR_RELEASE) {
    int64_t count = p->xtra >= 0 ? p->xtra : p->h.insdshead->xtratim;
    adsr_stage(p, ADSR_RELEASE, count * p->scale);
  }
  while (p->remaining == 0) {
    if (p->stage == ADSR_SUSTAIN && p->hold)
      return p->value;
    if (p->stage >= ADSR_RELEASE) {
      p->stage = ADSR_DONE;
      return p->value = 0.0;
    }
    adsr_stage(p, p->stage + 1, p->counts[p->stage + 1]);
  }
  value = p->value;
  if (--p->remaining == 0)
    p->value = p->target;
  else if (p->exponential)
    p->value *= p->multiplier;
  else
    p->value += p->increment;
  return value;
}

int32_t adsrset(CSOUND *csound, ADSR *p)
{
  return adsr_init(csound, p, 0, 0);
}

int32_t madsrset(CSOUND *csound, ADSR *p)
{
  return adsr_init(csound, p, 1, 0);
}

int32_t xdsrset(CSOUND *csound, ADSR *p)
{
  return adsr_init(csound, p, 0, 1);
}

int32_t mxdsrset(CSOUND *csound, ADSR *p)
{
  return adsr_init(csound, p, 1, 1);
}

int32_t kadsr(CSOUND *csound, ADSR *p)
{
  IGN(csound);
  *p->rslt = (cs_float)adsr_next(p);
  return OK;
}

int32_t aadsr(CSOUND *csound, ADSR *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  if (UNLIKELY(offset))
    memset(p->rslt, 0, offset * sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(p->rslt + nsmps, 0, early * sizeof(cs_float));
  }
  for (n = offset; n < nsmps; n++)
    p->rslt[n] = (cs_float)adsr_next(p);
  return OK;
}

/* End of ADSR */


int32_t lsgrset(CSOUND *csound, LINSEG *p)
{
  int32_t relestim;
  if (lsgset(csound,p) == OK){
    relestim = (p->cursegp + p->segsrem - 1)->cnt;
    /* VL 4-1-2011 was -1, making all linsegr
       releases in an instr => xtratim
       set to relestim seems to fix this */
    p->xtra = relestim;
    if (relestim > p->h.insdshead->xtratim)
      /* VL: 12.12.22 add extra kcycle to allow envelope 
         to reach target */
      p->h.insdshead->xtratim = (int32_t) relestim + 1;
    return OK;
  }
  else return NOTOK;
}

int32_t klnsegr(CSOUND *csound, LINSEG *p)
{
  IGN(csound);
  *p->rslt = p->curval;                   /* put the cur value    */
  if (p->segsrem) {                       /* done if no more segs */
    SEG *segp;
    if (p->h.insdshead->relesing && p->segsrem > 1) {
      while (p->segsrem > 1) {           /* reles flag new:      */
        segp = ++p->cursegp;             /*   go to last segment */
        p->segsrem--;
      }                                  /*   get univ relestim  */
      segp->cnt = p->xtra>= 0 ? p->xtra : p->h.insdshead->xtratim;
      goto newi;                         /*   and set new curinc */
    }
    if (--p->curcnt <= 0) {              /* if done cur seg      */
    chk2:
      if (p->segsrem == 2) return OK;    /*   seg Y rpts lastval */
      if (!(--p->segsrem)) {
        *p->rslt = p->cursegp->nxtpt;  /* VL: 12.12.22 set out to target */
        return OK;    /*   seg Z now done all */
      }
      segp = ++p->cursegp;               /*   else find nextseg  */
    newi:
      if (!(p->curcnt = segp->cnt)) {    /*   nonlen = discontin */
        p->curval = segp->nxtpt;         /*     reload & rechk   */
        goto chk2;
      }                                  /*   else get new slope */
      p->curinc = (segp->nxtpt - p->curval) / segp->cnt;
    }
    p->curval += p->curinc;              /* advance the cur val  */
  }
  return OK;
}

int32_t linsegr(CSOUND *csound, LINSEG *p)
{
  IGN(csound);
  cs_float  val, ainc, *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  val = p->curval;                          /* sav the cur value    */
  for (n=offset; n<nsmps; n++) {
    if (LIKELY(p->segsrem)) {               /* if no more segs putk */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {            /* release flag new:    */
          segp = ++p->cursegp;              /*   go to last segment */
          p->segsrem--;
        }                                   /*   get univ relestim  */
        segp->acnt = (p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
        goto newi;                          /*   and set new curinc */
      }
      if (--p->curcnt <= 0) {               /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) goto putk;     /*   seg Y rpts lastval */
        if (!(--p->segsrem)) {
          val  = p->cursegp->nxtpt;  /* VL: 12.12.22 set out to target */
          goto putk;     /*   seg Z now done all */
        }
        segp = ++p->cursegp;                /*   else find nextseg  */
      newi:
        if (!(p->curcnt = segp->acnt)) {    /*   nonlen = discontin */
          val = p->curval = segp->nxtpt;    /*   reload & rechk  */
          goto chk2;
        }                                   /*   else get new slope */
        p->curainc = (segp->nxtpt - val) / segp->acnt;
        // p->curainc = p->curinc * CS_ONEDKSMPS;
      }
      //p->curval = val + p->curainc*CS_KSMPS;    /* advance the cur val  */
      if ((ainc = p->curainc) == FL(0.0))
        goto putk;

      rs[n] = val;
      val += ainc;
    }
    else {
    putk:
      rs[n] = val;
    }
  }
  p->curval = val;
  return OK;
}

/* Shared initialization for duration and absolute-time exponential segments.
   rate is the rate used by cnt/mlt; acnt/amlt always use the sample rate. */
static int32_t expseg_init(CSOUND *csound, cs_float **args, int32_t nargs,
                           AUXCH *aux, XSEG **current, cs_float rate,
                           cs_float sample_rate, int32_t absolute)
{
  int32_t n, nsegs;
  XSEG *segments, *segp = NULL;
  cs_float val, next, previous_time = FL(0.0);

  if (UNLIKELY(nargs < 3 || !(nargs & 1)))
    return csound->InitError(csound,
                             Str("incomplete number of input arguments"));
  /* A skipped reinit must leave the current segment and its state intact. */
  if (*args[1] <= FL(0.0)) return OK;

  next = *args[0];
  nsegs = (nargs - 1) / 2;
  if (aux->auxp == NULL || (size_t)nsegs * sizeof(XSEG) > aux->size)
    csound->AuxAlloc(csound, (size_t)nsegs * sizeof(XSEG), aux);
  segments = (XSEG *)aux->auxp;

  for (n=0; n<nsegs; n++) {
    cs_float time = *args[2*n + 1];
    cs_float dur = time;
    cs_double count, audio_count;
    if (absolute) {
      if (UNLIKELY(time < previous_time))
        return csound->InitError(csound,
                                 Str("Breakpoint time %f not valid"), time);
      dur = time - previous_time;
      previous_time = time;
    }
    else if (dur <= FL(0.0)) {
      /* Ignore this endpoint and everything after the terminating duration. */
      break;
    }
    val = next;
    next = *args[2*n + 2];
    if (UNLIKELY(val == FL(0.0)))
      return csound->InitError(csound, Str("ival%lld is zero"), (int64_t)n+1);
    if (UNLIKELY(next == FL(0.0)))
      return csound->InitError(csound, Str("ival%lld is zero"), (int64_t)n+2);
    if (UNLIKELY(!((val > FL(0.0) && next > FL(0.0)) ||
                   (val < FL(0.0) && next < FL(0.0)))))
      return csound->InitError(csound,
                               Str("ival%lld sign conflict"), (int64_t)n+2);

    count = (cs_double)dur * rate;
    audio_count = (cs_double)dur * sample_rate;
    /* Reserve MAXPOS for continuation and check before converting to int. */
    if (UNLIKELY(!(count >= 0.0 && count < (cs_double)MAXPOS - 0.5 &&
                   audio_count >= 0.0 && audio_count < (cs_double)MAXPOS - 0.5)))
      return csound->InitError(csound, "%s",
                               Str("exponential segment duration out of range"));
    segp = &segments[n];
    segp->cnt = (int32_t)(count + 0.5);
    segp->acnt = (int32_t)(audio_count + 0.5);
    if (dur == FL(0.0)) {
      /* Repeated absolute times jump immediately, even at the final point. */
      segp->val = next;
      segp->mlt = segp->amlt = FL(1.0);
    }
    else {
      segp->val = val;
      segp->mlt = (cs_float)pow((cs_double)next / val, 1.0 / count);
      segp->amlt = (cs_float)pow((cs_double)next / val, 1.0 / audio_count);
    }
  }
  segp->cnt = segp->acnt = MAXPOS;
  *current = segments;
  return OK;
}

int32_t xsgset(CSOUND *csound, EXXPSEG *p)
{
  return expseg_init(csound, p->argums, p->INOCOUNT, &p->auxch,
                     &p->cursegp, CS_EKR, CS_ESR, 0);
}

int32_t xsgset_bkpt(CSOUND *csound, EXXPSEG *p)
{
  return expseg_init(csound, p->argums, p->INOCOUNT, &p->auxch,
                     &p->cursegp, CS_EKR, CS_ESR, 1);
}

int32_t xsgset2b(CSOUND *csound, EXPSEG2 *p)
{
  return expseg_init(csound, p->argums, p->INOCOUNT, &p->auxch,
                     &p->cursegp, CS_ESR, CS_ESR, 1);
}

int32_t xsgset2(CSOUND *csound, EXPSEG2 *p)
{
  return expseg_init(csound, p->argums, p->INOCOUNT, &p->auxch,
                     &p->cursegp, CS_ESR, CS_ESR, 0);
}

/***************************************/

int32_t expseg2(CSOUND *csound, EXPSEG2 *p)             /* gab-A1 (G.Maldonado) */
{
  IGN(csound);
  XSEG        *segp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       val, *rs;
  segp = p->cursegp;
  if (UNLIKELY(segp == NULL))
    return csound->PerfError(csound, &p->h,
                             Str("expsega: not initialised"));
  val  = segp->val;
  rs   = p->rslt;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    while (segp->cnt != MAXPOS && --segp->cnt < 0)   {
      p->cursegp = ++segp;
      val = segp->val;
    }
    rs[n] = val;
    val *=  segp->mlt;
  }
  segp->val = val;
  return OK;
}



int32_t kxpseg(CSOUND *csound, EXXPSEG *p)
{
  XSEG        *segp;


  segp = p->cursegp;
  if (UNLIKELY(segp == NULL)) goto err1;
  while (segp->cnt != MAXPOS && --segp->cnt < 0)
    p->cursegp = ++segp;
  *p->rslt = segp->val;
  segp->val *= segp->mlt;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("expseg (krate): not initialised"));
}


int32_t expseg(CSOUND *csound, EXXPSEG *p)
{
  XSEG        *segp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float       *rs = p->rslt;



  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    segp = p->cursegp;
    if (UNLIKELY(segp == NULL)) goto err1;
    while (segp->acnt != MAXPOS && --segp->acnt < 0) {
      //printf("seg: val=%f amlt=%f\n", segp->val,segp->amlt );
      p->cursegp = ++segp;
      //printf("nxtseg: val=%f amlt=%f acnt=%d\n",
      //       segp->val,segp->amlt,segp->acnt);
    }
    rs[n] = segp->val;
    segp->val *= segp->amlt;
  }
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("expseg (arate): not initialised"));
}

int32_t xsgrset(CSOUND *csound, EXPSEG *p)
{
  int32_t     relestim;
  SEG     *segp;
  int32_t     nsegs;
  int64_t         n=0;n = 0;
  cs_float   **argp, prvpt;


  if (!(p->INOCOUNT & 1)){
    return csound->InitError(csound, Str("incomplete number of input arguments"));
  }

  //p->xtra = -1;
  /* count segs & alloc if nec */
  nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
  if ((segp = (SEG *) p->auxch.auxp) == NULL ||
      (uint32_t)nsegs*sizeof(SEG) > (uint32_t)p->auxch.size) {
    csound->AuxAlloc(csound, (int32_t)nsegs*sizeof(SEG), &p->auxch);
    p->cursegp = segp = (SEG *) p->auxch.auxp;
  }
  argp = p->argums;
  prvpt = **argp++;
  if (**argp < FL(0.0))  return OK; /* if idur1 < 0, skip init      */
  p->curval  = prvpt;
  p->curcnt  = 0;                   /* else setup null seg0         */
  p->cursegp = segp - 1;
  p->segsrem = nsegs + 1;
  do {                              /* init & chk each real seg ..  */
    cs_float dur = **argp++;
    segp->nxtpt = **argp++;
    if ((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) <= 0)
      segp->cnt = 0;
    else if (segp->nxtpt * prvpt <= FL(0.0))
      goto experr;
    if ((segp->acnt = (int32_t)(dur * CS_ESR )) <= 0)
      segp->acnt = 0;
    prvpt = segp->nxtpt;
    segp++;
  } while (--nsegs);
  relestim = (int32_t)(p->cursegp + p->segsrem - 1)->cnt;
  p->xtra = relestim;
  if (relestim > p->h.insdshead->xtratim)
    p->h.insdshead->xtratim = relestim;
  return OK;

 experr:
  n = segp - p->cursegp;// + 2;
  if (prvpt == FL(0.0))
    return csound->InitError(csound, Str("ival%lld is zero"), n);
  else if (segp->nxtpt == FL(0.0))
    return csound->InitError(csound, Str("ival%lld is zero"), n+1);
  return csound->InitError(csound, Str("ival%lld sign conflict"), n+1);
}



int32_t kxpsegr(CSOUND *csound, EXPSEG *p)
{
  IGN(csound);
  *p->rslt = p->curval;               /* put the cur value    */
  if (p->segsrem) {                   /* done if no more segs */
    SEG *segp;
    if (p->h.insdshead->relesing && p->segsrem > 1) {
      while (p->segsrem > 1) {        /* reles flag new:      */
        segp = ++p->cursegp;          /*   go to last segment */
        p->segsrem--;
      }                               /*   get univ relestim  */
      segp->cnt = p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim;
      goto newm;                      /*   and set new curmlt */
    }
    if (--p->curcnt <= 0) {           /* if done cur seg      */
    chk2:
      if (p->segsrem == 2) return OK; /*   seg Y rpts lastval */
      if (!(--p->segsrem)) return OK; /*   seg Z now done all */
      segp = ++p->cursegp;            /*   else find nextseg  */
    newm:
      if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
        p->curval = segp->nxtpt;      /*     reload & rechk   */
        goto chk2;
      }
      if (segp->nxtpt == p->curval)   /*   else get new mlt   */
        p->curmlt = FL(1.0);
      else p->curmlt = (cs_float) pow(segp->nxtpt/p->curval, 1.0/segp->cnt);
    }
    p->curval *= p->curmlt;           /* advance the cur val  */
  }
  return OK;
}

int32_t expsegr(CSOUND *csound, EXPSEG *p)
{
  IGN(csound);
  cs_float  val, amlt, *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;


  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  /* sav the cur value    */
  val = p->curval;
  for (n=offset; n<nsmps; n++) {

    if (p->segsrem) {                   /* if no more segs putk */
      SEG *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {        /* if reles flag new    */
          segp = ++p->cursegp;          /*   go to last segment */
          p->segsrem--;
        }                               /*   get univ relestim  */
        segp->acnt = (p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
        goto newm;                      /*   and set new curmlt */
      }
      if (--p->curcnt <= 0) {           /* if done cur seg      */
      chk2:
        if (p->segsrem == 2) goto putk; /*   seg Y rpts lastval */
        if (!(--p->segsrem)) goto putk; /*   seg Z now done all */
        segp = ++p->cursegp;            /*   else find nextseg  */
      newm:
        if (!(p->curcnt = segp->acnt)) { /*   nonlen = discontin */
          val = p->curval = segp->nxtpt; /*   reload & rechk  */
          goto chk2;
        }                               /*   else get new mlts  */
        if (segp->nxtpt == val) {
          p->curmlt = p->curamlt = FL(1.0);
          p->curval = val;
          goto putk;
        }
        else {
          p->curmlt = POWER((segp->nxtpt/val), FL(1.0)/segp->cnt);
          // VL: this line introduces a bug
          //p->curamlt = POWER(p->curmlt, FL(1.0)/(cs_float)(nsmps-offset));
          // VL: this line fixes it but does not take account of offset
          p->curamlt = POWER((segp->nxtpt/val), FL(1.0)/segp->acnt);
        }
      }
      if ((amlt = p->curamlt) == FL(1.0)) goto putk;
      rs[n] = val;
      val *= amlt;
    }
    else {
    putk:
      rs[n] =  val;
    }
  }
  p->curval = val;
  return OK;
}

/* Compatibility: the unrounded +0.5 in linen's decay divisor can leave
 * a positive value at idur. Fractional times can also shift the decay start.
 * Preserve this historical arithmetic in lnnset and alnnset for existing
 * scores. Use linseg when the envelope must reach and hold zero. See #3247. */
int32_t lnnset(CSOUND *csound, LINEN *p)
{
  cs_float a,b,dur;
  cs_float len = csound->curip->p3.value;

  if ((dur = *p->idur) > FL(0.0)) {
    cs_float iris = *p->iris, idec = *p->idec;
    if (len<(iris<idec?idec:iris))
      csound->Warning(csound, Str("p3 too short in linen"));

    p->cnt1 = (int32_t)(iris * CS_EKR + FL(0.5));
    if (p->cnt1 > (int32_t)0) {
      p->inc1 = FL(1.0) / (cs_float) p->cnt1;
    }
    else p->inc1 = FL(1.0);
    a = dur * CS_EKR + FL(0.5);
    b = idec * CS_EKR + FL(0.5);
    if ((int32_t) b > 0) {
      p->cnt2 = (int32_t) (a - b);
      p->inc2 = FL(1.0) /  b;
    }
    else {
      /* Compatibility: zero decay still falls by one per tick after idur. */
      p->inc2 = FL(1.0);
      p->cnt2 = (int32_t) a;
    }
    p->lin1 = FL(0.0);
    p->lin2 = FL(1.0);
  }
  return OK;
}

int32_t alnnset(CSOUND *csound, LINEN *p)
{
  cs_float a,b,dur;
  cs_float len = csound->curip->p3.value;

  if ((dur = *p->idur) > FL(0.0)) {
    cs_float iris = *p->iris, idec = *p->idec;
    if (len<(iris<idec?idec:iris))
      csound->Warning(csound, Str("p3 too short in linen"));
    p->cnt1 = (int64_t)(*p->iris * CS_ESR + FL(0.5));
    if (p->cnt1 > 0) {
      p->inc1 = FL(1.0) / (cs_float) p->cnt1;
    }
    else p->inc1 = FL(1.0);
    /* Keep the same unrounded divisor as lnnset for compatibility. */
    a = dur * CS_ESR + FL(0.5);
    b = *p->idec * CS_ESR + FL(0.5);
    if ((int64_t) b > 0) {
      p->cnt2 = (int64_t) (a - b);
      p->inc2 = FL(1.0) /  b;
    }
    else {
      /* Compatibility: zero decay still falls by one per tick after idur. */
      p->inc2 = FL(1.0);
      p->cnt2 = (int64_t) a;
    }
    p->lin1 = FL(0.0);
    p->lin2 = FL(1.0);
  }
  return OK;
}

int32_t klinen(CSOUND *csound, LINEN *p)
{
  IGN(csound);
  cs_float fact = FL(1.0);

  if (p->cnt1 > 0) {
    fact = p->lin1;
    p->lin1 += p->inc1;
    p->cnt1--;
  }
  if (p->cnt2 > 0)
    p->cnt2--;
  else {
    fact *= p->lin2;
    p->lin2 -= p->inc2;
  }
  *p->rslt = *p->sig * fact;
  return OK;
}

int32_t linen(CSOUND *csound, LINEN *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t flag=0, n, nsmps = CS_KSMPS;
  cs_float *rs,*sg,val;
  int32_t    asgsg = IS_ASIG_ARG(p->sig);
  int64_t cnt2 = p->cnt2;
  int64_t cnt1 = p->cnt1;
  cs_float lin1 = p->lin1;
  cs_float lin2 = p->lin2;

  rs = p->rslt;
  sg = p->sig;

  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }

  for (n=offset; n<nsmps; n++) {
    val = FL(1.0);
    if (cnt1 > 0) {
      flag = 1;
      val = lin1;
      lin1 += p->inc1;
      cnt1--;
    }

    if (cnt2 > 0){
      cnt2--;
    }
    else {
      val *= lin2;
      lin2 -= p->inc2;
      flag = 1;
    }

    if (flag) {
      if (asgsg)
        rs[n] = sg[n] * val;
      else
        rs[n] = *sg * val;
    }
    else {
      if (asgsg)
        rs[n] = sg[n];
      else rs[n] = *sg;
    }
    flag = 0;
  }
  p->cnt2 = cnt2;
  p->cnt1 = cnt1;
  p->lin1 = lin1;
  p->lin2 = lin2;
  return OK;
}



int32_t lnrset(CSOUND *csound, LINENR *p)
{
  p->cnt1 = (int32_t)(*p->iris * CS_EKR + FL(0.5));
  if (p->cnt1 > 0L) {
    p->inc1 = FL(1.0) / (cs_float)p->cnt1;
    p->val = FL(0.0);
  }
  else p->inc1 = p->val = FL(1.0);
  if (*p->idec > FL(0.0)) {
    int32_t relestim = (int32_t)(*p->idec * CS_EKR + FL(0.5));

    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    if (UNLIKELY(*p->iatdec <= FL(0.0))) {
      return csound->InitError(csound, Str("non-positive iatdec"));
    }
    else p->mlt2 = POWER(*p->iatdec, CS_ONEDKR / *p->idec);
  }
  else p->mlt2 = FL(1.0);
  p->lin1 = FL(0.0);
  p->val2 = FL(1.0);
  return OK;
}

int32_t alnrset(CSOUND *csound, LINENR *p)
{
  p->cnt1 = (int32_t)(*p->iris * CS_ESR);
  if (p->cnt1 > 0L) {
    p->inc1 = FL(1.0) / (cs_float)p->cnt1;
    p->val = FL(0.0);
  }
  else p->inc1 = p->val = FL(1.0);
  if (*p->idec > FL(0.0)) {
    int32_t relestim = (int32_t)(*p->idec * CS_EKR + FL(0.5));
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = relestim;
    if (UNLIKELY(*p->iatdec <= FL(0.0))) {
      return csound->InitError(csound, Str("non-positive iatdec"));
    }
    else p->mlt2 = POWER(*p->iatdec, CS_ONEDSR / *p->idec);
  }
  else p->mlt2 = FL(1.0);
  p->lin1 = FL(0.0);
  p->val2 = FL(1.0);
  return OK;
}


int32_t klinenr(CSOUND *csound, LINENR *p)
{
  IGN(csound);
  cs_float fact = FL(1.0);

  if (p->cnt1 > 0L) {
    fact = p->lin1;
    p->lin1 += p->inc1;
    p->cnt1--;
  }
  if (p->h.insdshead->relesing) {
    fact *= p->val2;
    p->val2 *= p->mlt2;
  }
  *p->rslt = *p->sig * fact;
  return OK;
}

int32_t linenr(CSOUND *csound, LINENR *p)
{
  IGN(csound);
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t flag=0, n, nsmps = CS_KSMPS;
  cs_float *rs,*sg,val,val2 = p->val2;
  int32_t    asgsg = IS_ASIG_ARG(p->sig);
  val = p->val;
  rs = p->rslt;
  sg = p->sig;
  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps; n++) {
    flag = 0;
    val = FL(1.0);
    if (p->cnt1 > 0L) {
      flag = 1;
      val = p->lin1;
      p->lin1 += p->inc1;
      p->cnt1--;
    }
    if (p->h.insdshead->relesing) {
      flag = 1;
      val *= val2;
      val2 *= p->mlt2;
    }
    if (flag) {
      if (asgsg)
        rs[n] = sg[n] * val;
      else
        rs[n] = *sg * val;
    }
    else {
      if (asgsg) rs[n] = sg[n];
      else rs[n] = *sg;
    }
  }
  p->val = val;
  p->val2 = val2;
  return OK;
}

int32_t evxset(CSOUND *csound, ENVLPX *p)
{
  FUNC        *ftp;
  cs_float       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
  int32_t       cnt1;
  cs_float       len = csound->curip->p3.value;

  if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
    return NOTOK;
  p->floatph = !IS_POW_TWO(ftp->flen);
  p->ftp = ftp;
  if ((idur = *p->idur) > FL(0.0)) {
    if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
      return csound->InitError(csound, "iatss = 0");
    }
    if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
      if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
        return csound->InitError(csound, Str("ixmod out of range."));
      }
      ixmod = -SIN(SIN(ixmod));
      prod = ixmod * iatss;
      diff = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FL(0.0))
        asym = FHUND;
      else {
        asym = FL(2.0) * prod / denom;
        if (FABS(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FL(0.0);
    if ((irise = *p->irise) > FL(0.0)) {
      if (irise + *p->idec > len)
        csound->Warning(csound, Str("p3 too short in envlpx"));
      p->phs = 0;
      p->phsf = FL(0.0);
      p->ki = (int32_t) (CS_KICVT / irise);
      p->kif = CS_ONEDKR/irise;
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1;
      p->phsf = FL(-1.0);
      p->val = *(ftp->ftable + ftp->flen)-asym;
      irise = FL(0.0);  /* in case irise < 0 */
    }
    if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
      return csound->InitError(csound, Str("rise func ends with zero"));
    }
    cnt1 = (int32_t) ((idur - irise - *p->idec) * CS_EKR + FL(0.5));
    if (cnt1 < 0L) {

      cnt1 = 0;
      nk = CS_EKR;
    }
    else {
      if (*p->iatss < FL(0.0) || cnt1 <= 4L)
        nk = CS_EKR;
      else nk = (cs_float) cnt1;
    }
    p->mlt1 = POWER(iatss, (FL(1.0)/nk));
    if (*p->idec > FL(0.0)) {
      if (UNLIKELY(*p->iatdec <= FL(0.0))) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
      p->mlt2 = POWER(*p->iatdec, (CS_ONEDKR / *p->idec));
    }
    else p->mlt2 = 1.0;
    p->cnt1 = cnt1;
    if (cnt1 == 0) {
      /* With no steady stage there is no curve offset to remove later. */
      if (p->phs < 0) p->val = ftp->ftable[ftp->flen];
      asym = FL(0.0);
    }
    p->asym = asym;
  }
  return OK;
}

int32_t knvlpx(CSOUND *csound, ENVLPX *p)
{
  FUNC        *ftp;
  int32_t       phs, check;
  cs_float       fact, v1, fract, *ftab;
  cs_double phsf;

  ftp = p->ftp;
  if (UNLIKELY(ftp==NULL)) goto err1;        /* RWD fix */
  if(p->floatph) check = ((phsf = p->phsf) >= FL(0.0));
  else check = ((phs = p->phs) >= 0);
  if (check) {
    if(!p->floatph) {
      fract = (cs_float) PFRAC(phs);
      ftab = ftp->ftable + (phs >> ftp->lobits);
      v1 = *ftab++;
      fact = (v1 + (*ftab - v1) * fract);
      phs += p->ki;
      if (phs >= MAXLEN) {  /* check that 2**N+1th pnt is good */
        p->val = *(ftp->ftable + ftp->flen );
        if (UNLIKELY(!p->val)) {
          return csound->PerfError(csound, &(p->h),
                                   Str("envlpx rise func ends with zero"));
        }
        p->val -= p->asym;
        phs = -1L;
      }
      p->phs = phs;
    } else {
      cs_double pos = phsf * ftp->flen;
      fract = pos - (int32_t) pos;
      ftab = ftp->ftable + (int32_t) pos;
      v1 = *ftab++;
      fact = (v1 + (*ftab - v1) * fract);
      phsf += p->kif;
      if (phsf >= FL(1.0)) {
        p->val = *(ftp->ftable + ftp->flen);
        if (UNLIKELY(!p->val)) {
          return csound->PerfError(csound, &(p->h),
                                   Str("envlpx rise func ends with zero"));
        }
        p->val -= p->asym;
        phsf = FL(-1.0);
      }
      p->phsf = phsf;
    }
  }
  else {
    fact = p->val;
    if (p->cnt1 > 0L) {
      p->val *= p->mlt1;
      fact += p->asym;
      p->cnt1--;
      if (p->cnt1 == 0L)
        p->val += p->asym;
    }
    else p->val *= p->mlt2;
  }
  *p->rslt = *p->xamp * fact;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("envlpx(krate): not initialised"));
}

int32_t aevxset(CSOUND *csound, ENVLPX *p)
{
  FUNC        *ftp;
  cs_float       ixmod, iatss, idur, prod, diff, asym, nk, denom, irise;
  int32_t       cnt1;
  cs_float       len = csound->curip->p3.value;

  if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
    return NOTOK;
  p->floatph = !IS_POW_TWO(ftp->flen);
  p->ftp = ftp;
  if ((idur = *p->idur) > FL(0.0)) {
    if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
      return csound->InitError(csound, "iatss = 0");
    }
    if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
      if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
        return csound->InitError(csound, Str("ixmod out of range."));
      }
      ixmod = -SIN(SIN(ixmod));
      prod = ixmod * iatss;
      diff = ixmod - iatss;
      denom = diff + prod + FL(1.0);
      if (denom == FL(0.0))
        asym = FHUND;
      else {
        asym = FL(2.0) * prod / denom;
        if (FABS(asym) > FHUND)
          asym = FHUND;
      }
      iatss = (iatss - asym) / (FL(1.0) - asym);
      asym = asym* *(ftp->ftable + ftp->flen); /* +1 */
    }
    else asym = FL(0.0);

    if ((irise = *p->irise) > FL(0.0)) {
      if (irise + *p->idec > len)
        csound->Warning(csound, Str("p3 too short in envlpx"));
      p->phs = 0;
      p->phsf = FL(0.0);
      p->ki = (int32_t) ((FMAXLEN / CS_ESR )/ irise);
      p->kif = ((1./ CS_ESR )/ irise);
      p->val = *ftp->ftable;
    }
    else {
      p->phs = -1;
      p->phsf = FL(-1.0); 
      p->val = *(ftp->ftable + ftp->flen)-asym;
      irise = FL(0.0);  /* in case irise < 0 */
    }
    if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
      return csound->InitError(csound, Str("rise func ends with zero"));
    }
    cnt1 = (int32_t) ((idur - irise - *p->idec) * CS_ESR);
    if (cnt1 < 0L) {
      cnt1 = 0;
      nk = CS_ESR;
    }
    else {
      if (*p->iatss < FL(0.0) || cnt1 <= 4L)
        nk = CS_ESR;
      else nk = (cs_float) cnt1;
    }
    p->mlt1 = POWER(iatss, (FL(1.0)/nk));
    if (*p->idec > FL(0.0)) {
      if (UNLIKELY(*p->iatdec <= FL(0.0))) {
        return csound->InitError(csound, Str("non-positive iatdec"));
      }
      p->mlt2 = POWER(*p->iatdec, (CS_ONEDSR / *p->idec));
    }
    else p->mlt2 = 1.0;
    p->cnt1 = cnt1;
    if (cnt1 == 0) {
      /* With no steady stage there is no curve offset to remove later. */
      if (p->phs < 0) p->val = ftp->ftable[ftp->flen];
      asym = FL(0.0);
    }
    p->asym = asym;
  }
  return OK;
}


int32_t envlpx(CSOUND *csound, ENVLPX *p)
{
  int32_t       phs;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int64_t pos, lobits, lomask;
  cs_float      fact, *xamp, *rslt, val, asym, mlt, mlt2, v1, fract, *ftab, lodiv;
  int32_t    asgsg = IS_ASIG_ARG(p->xamp), floatph = p->floatph, check,
    flen;
  cs_double phsf;
  xamp = p->xamp;
  rslt = p->rslt;
  val  = p->val;
  mlt = p->mlt1;
  mlt2 = p->mlt2;
  asym = p->asym;

  if (UNLIKELY(p->ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("envlpx(krate): not initialised"));
  flen = p->ftp->flen;
  ftab = p->ftp->ftable;
  lobits = p->ftp->lobits;
  lomask = p->ftp->lomask;
  lodiv  = p->ftp->lodiv;
  if (UNLIKELY(ftab[p->ftp->flen] == 0.0))
    return csound->PerfError(csound, &(p->h),
                             Str("envlpx rise func ends with zero"));

  if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rslt[nsmps], '\0', early*sizeof(cs_float));
  }
  for (n=offset; n<nsmps;n++) {
    if(floatph) check = ((phsf = p->phsf) >= FL(0.0));
    else check = ((phs = p->phs) >= 0); 
    if (check) {
      if(!floatph) {
        fract = ((phs) & lomask) * lodiv;
        pos = (int64_t) (phs >> lobits);
        v1 = ftab[pos];
        fact = (v1 + (ftab[pos+1] - v1) * fract);
        phs += p->ki;
        if (phs >= MAXLEN) {
          val = ftab[flen];
          val -= p->asym;
          phs = -1;
        }
        p->phs = phs;
      } else {
        cs_double posf = phsf * flen;
        fract = posf - (int32_t) posf;
        v1 = ftab[(int32_t) posf];
        fact = (v1 + (ftab[(int32_t)posf+1] - v1) * fract);
        phsf += p->kif;
        if (phsf >= FL(1.0)) {
          val = ftab[flen] - asym;
          phsf = FL(-1.0);
        }
        p->phsf = phsf;
      }
    }
    else {
      fact = val;
      if (p->cnt1 > 0L) {
        val *= mlt;
        fact += asym;
        p->cnt1--;
        if (p->cnt1 == 0L)
          val += asym;
      }
      else val *= mlt2;
    }
    if (asgsg)
      rslt[n] = xamp[n] * fact;
    else
      rslt[n] = *xamp * fact;
  }
  p->val = val;
  return OK;
}

int32_t evrset(CSOUND *csound, ENVLPR *p)
{
  FUNC        *ftp;
  cs_float       ixmod, iatss, prod, diff, asym, denom, irise;

  if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
    return NOTOK;
  p->ftp = ftp;
  p->floatph = !IS_POW_TWO(ftp->flen);
  if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
    return csound->InitError(csound, "iatss = 0");
  }
  if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
    if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
      return csound->InitError(csound, Str("ixmod out of range."));
    }
    ixmod = -SIN(SIN(ixmod));
    prod  = ixmod * iatss;
    diff  = ixmod - iatss;
    denom = diff + prod + FL(1.0);
    if (denom == FL(0.0))
      asym = FHUND;
    else {
      asym = FL(2.0) * prod / denom;
      if (FABS(asym) > FHUND)
        asym = FHUND;
    }
    iatss = (iatss - asym) / (FL(1.0) - asym);
    asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
  }
  else asym = FL(0.0);
  if ((irise = *p->irise) > FL(0.0)) {
    p->phs = 0; p->phsf = FL(0.0);
    p->ki = (int32_t) (CS_KICVT / irise);
    p->kif = CS_ONEDKR / irise;
    p->val = *ftp->ftable;
  }
  else {
    p->phsf = FL((p->phs = -1));
    p->val = *(ftp->ftable + ftp->flen)-asym;
    /* irise = FL(0.0); */          /* in case irise < 0 */
  }
  if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
    return csound->InitError(csound, Str("rise func ends with zero"));
  }
  p->mlt1 = POWER(iatss, CS_ONEDKR);
  p->rlscnt = 0;
  p->rindep = (*p->irind != FL(0.0));
  p->atdec = 1.0;
  if (*p->idec > FL(0.0)) {
    int32_t rlscnt = (int32_t)(*p->idec * CS_EKR + FL(0.5));
    if (p->rindep)
      p->rlscnt = rlscnt;
    else if (rlscnt > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int32_t)rlscnt;
    if (UNLIKELY((p->atdec = *p->iatdec) <= FL(0.0) )) {
      return csound->InitError(csound, Str("non-positive iatdec"));
    }
  }
  p->asym = asym;
  p->rlsing = 0;
  return OK;
}

int32_t aevrset(CSOUND *csound, ENVLPR *p)
{
  FUNC        *ftp;
  cs_float       ixmod, iatss, prod, diff, asym, denom, irise;

  if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
    return NOTOK;
  p->ftp = ftp;
  p->floatph = !IS_POW_TWO(ftp->flen);
  if (UNLIKELY((iatss = FABS(*p->iatss)) == FL(0.0))) {
    return csound->InitError(csound, "iatss = 0");
  }
  if (iatss != FL(1.0) && (ixmod = *p->ixmod) != FL(0.0)) {
    if (UNLIKELY(FABS(ixmod) > FL(0.95))) {
      return csound->InitError(csound, Str("ixmod out of range."));
    }
    ixmod = -SIN(SIN(ixmod));
    prod  = ixmod * iatss;
    diff  = ixmod - iatss;
    denom = diff + prod + FL(1.0);
    if (denom == FL(0.0))
      asym = FHUND;
    else {
      asym = FL(2.0) * prod / denom;
      if (FABS(asym) > FHUND)
        asym = FHUND;
    }
    iatss = (iatss - asym) / (FL(1.0) - asym);
    asym = asym * *(ftp->ftable + ftp->flen); /* +1 */
  }
  else asym = FL(0.0);
  if ((irise = *p->irise) > FL(0.0)) {
    p->phsf = FL((p->phs = 0));
    p->ki = (int32_t) ((FMAXLEN / CS_ESR)/ irise);
    p->kif = (1./ CS_ESR)/ irise;
    p->val = *ftp->ftable;
  }
  else {
    p->phsf = FL((p->phs = -1));
    p->val = *(ftp->ftable + ftp->flen)-asym;
    /* irise = FL(0.0); */          /* in case irise < 0 */
  }
  if (UNLIKELY(!(*(ftp->ftable + ftp->flen)))) {
    return csound->InitError(csound, Str("rise func ends with zero"));
  }
  p->mlt1 = POWER(iatss, CS_ONEDSR);
  p->rlscnt = 0;
  p->rindep = (*p->irind != FL(0.0));
  p->atdec = 1.0;
  if (*p->idec > FL(0.0)) {
    int32_t rlscnt = (int32_t)(*p->idec * CS_EKR + FL(0.5));
    if (p->rindep)
      p->rlscnt = rlscnt;
    else if (rlscnt > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int32_t)rlscnt;
    if (UNLIKELY((p->atdec = *p->iatdec) <= FL(0.0) )) {
      return csound->InitError(csound, Str("non-positive iatdec"));
    }
  }
  p->asym = asym;
  p->rlsing = 0;
  return OK;
}


int32_t knvlpxr(CSOUND *csound, ENVLPR *p)
{
  IGN(csound);
  cs_float  fact;
  int32_t  rlscnt, check, phs;
  cs_double phsf;

  if(p->floatph) check = ((phsf = p->phsf) >= FL(0.0));
  else check = ((phs = p->phs) >= 0);

  if (!p->rlsing) {                   /* if not in reles seg  */
    if (p->h.insdshead->relesing) {
      p->rlsing = 1;                  /*   if new flag, set mlt2 */
      rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
      if (rlscnt)
        p->mlt2 = POWER(p->atdec, FL(1.0)/rlscnt);
      else p->mlt2 = FL(1.0);
    }
    if (check) {                /* do fn rise for seg 1 */
      FUNC *ftp = p->ftp;
      if(!p->floatph) {
        cs_float fract = PFRAC(phs);
        cs_float *ftab = ftp->ftable + (phs >> ftp->lobits);
        cs_float v1 = *ftab++;
        fact = (v1 + (*ftab - v1) * fract);
        phs += p->ki;
        if (phs < MAXLEN || p->rlsing)  /* if more fn or beg rls */
          p->val = fact;                /*      save cur val     */
        else {                          /* else prep for seg 2  */
          p->val = *(ftp->ftable + ftp->flen) - p->asym;
          phs = -1L;
        }
        p->phs = phs;
      } else {
        cs_double pos = phsf * ftp->flen;
        cs_float fract = pos - (int32_t) pos;
        cs_float *ftab = ftp->ftable + (int32_t) pos;
        cs_float v1 = *ftab++;
        fact = (v1 + (*ftab - v1) * fract);
        phsf += p->kif;
        if (phsf < FL(1.0) || p->rlsing)
          p->val = fact;
        else {
          p->val = *(ftp->ftable + ftp->flen);
          p->val -= p->asym;
          phsf = FL(-1.0);
        }
        p->phsf = phsf;
      }
    }
    else {
      fact = p->val + p->asym;        /* do seg 2 with asym */
      p->val *= p->mlt1;
      if (p->rlsing)                  /* if ending, rm asym */
        p->val += p->asym;
    }
  }
  else fact = p->val *= p->mlt2;      /* else do seg 3 decay */
  *p->rslt = *p->xamp * fact;
  return OK;
}

int32_t envlpxr(CSOUND *csound, ENVLPR *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double rlscnt;
  int64_t lobits, lomask, pos;
  int32_t phs = p->phs;
  cs_float fact, *xamp, *rslt, val, asym, mlt, v1, fract, *ftab, lodiv;
  int32_t    asgsg = IS_ASIG_ARG(p->xamp), check, floatph = p->floatph,
    flen;
  cs_double phsf;

  xamp = p->xamp;
  rslt = p->rslt;
  val  = p->val;
  mlt = p->mlt1;
  //mlt2 = p->mlt2;
  asym = p->asym;

  if (UNLIKELY(p->ftp==NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("envlpx(krate): not initialised"));
  flen = p->ftp->flen;
  ftab = p->ftp->ftable;
  lobits = p->ftp->lobits;
  lomask = p->ftp->lomask;
  lodiv  = p->ftp->lodiv;
  if (UNLIKELY(ftab[p->ftp->flen] == 0.0))
    return csound->PerfError(csound, &(p->h),
                             Str("envlpx rise func ends with zero"));

  if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rslt[nsmps], '\0', early*sizeof(cs_float));
  }

  for (n=offset; n<nsmps; n++) {
    if(p->floatph) check = ((phsf = p->phsf) >= FL(0.0));
    else check = ((phs = p->phs) >= 0);

    if (!p->rlsing) {                   /* if not in reles seg  */
      if (p->h.insdshead->relesing) {
        p->rlsing = 1;                  /*   if new flag, set mlt2 */
        rlscnt = (p->rindep) ? p->rlscnt : p->h.insdshead->xtratim;
        rlscnt *= CS_KSMPS;
        if (rlscnt)
          p->mlt2 = POWER(p->atdec, FL(1.0)/rlscnt);
        else p->mlt2 = FL(1.0);
      }
      if (check) {                /* do fn rise for seg 1 */
        if(!floatph) {
          fract = ((phs) & lomask) * lodiv;
          pos = (int64_t) (phs >> lobits);
          v1 = ftab[pos];
          fact = (v1 + (ftab[pos+1] - v1) * fract);
          phs += p->ki;
          if (phs < MAXLEN || p->rlsing)
            val = fact;
          else {
            val = ftab[p->ftp->flen];
            val -= p->asym;
            phs = -1;
          }
          p->phs = phs;
        } else {
          cs_double fpos = phsf * flen;
          fract = fpos - (int32_t) fpos;
          v1 = ftab[(int32_t) fpos];
          fact = (v1 + (ftab[(int32_t) fpos+1] - v1) * fract);
          phsf += p->kif;
          if (phsf < 1.0 || p->rlsing)
            val = fact;
          else {
            val = ftab[p->ftp->flen];
            val -= p->asym;
            phsf = -1;
          }
          p->phsf = phsf;
        }
      }
      else {
        fact = val + asym;
        val *= mlt;
        if (p->rlsing)
          val += asym;
      }
    }
    else
      fact = val *= p->mlt2;     /* else do seg 3 decay  */

    if (asgsg)
      rslt[n] = xamp[n] * fact;
    else
      rslt[n] = *xamp * fact;
  }
  p->val = val;
  return OK;
}

int32_t csgset(CSOUND *csound, COSSEG *p)
{
  SEG *segp, *sp;
  int32_t nsegs;
  cs_float       **argp;
  cs_double val, y1, y2;


  if (p->INOCOUNT < 3 || !(p->INOCOUNT & 1)) {
    return csound->InitError(csound, Str("incomplete number of input arguments"));
  }

  /* count segs & alloc if nec */
  nsegs = (p->INOCOUNT - (!(p->INOCOUNT & 1))) >> 1;
  //printf("****nsegs = %d\n", nsegs);
  if ((segp = (SEG *) p->auxch.auxp) == NULL ||
      (1+nsegs)*sizeof(SEG) > p->auxch.size) {
    csound->AuxAlloc(csound, (int32_t)(1+nsegs)*sizeof(SEG), &p->auxch);
    p->cursegp = 1+(segp = (SEG *) p->auxch.auxp);
    segp[nsegs-1].cnt = MAXPOS; /* set endcount for safety */
    segp[nsegs-1].acnt = MAXPOS;
  }
  sp = segp;
  argp = p->argums;
  y1 = val = (cs_double)**argp++;
  if (UNLIKELY(**argp <= FL(0.0)))  return OK;    /* if idur1 <= 0, skip init  */
  p->curcnt = 0;
  p->cursegp = segp+1;          /* else setup first seg */
  p->segsrem = nsegs;
  //printf("****current seg = %p segp = %p\n", p->cursegp, segp);
  do {                                /* init each seg ..  */
    cs_double dur = (cs_double)**argp++;
    segp->nxtpt = (cs_double)**argp++;
    if (UNLIKELY((segp->cnt = (int32_t)(dur * CS_EKR + FL(0.5))) < 0))
      segp->cnt = 0;
    if (UNLIKELY((segp->acnt = (int32_t)(dur * CS_ESR)) < 0))
      segp->acnt = 0;

    //printf("****i: %d(%p): cnt=%d nxtpt=%f\n",
    //       p->segsrem-nsegs, segp, segp->cnt, segp->nxtpt);
    segp++;
  } while (--nsegs);
  p->y1 = y1;
  p->y2 = y2 = sp->nxtpt;
  p->x = 0.0;
  if (IS_ASIG_ARG(p->rslt)) {
    p->inc = (y2!=y1 && sp->acnt ? 1.0/(sp->acnt) : 0.0);
    p->curcnt = sp->acnt;
  }
  else {
    p->inc = (y2!=y1 && sp->cnt ? 1.0/(sp->cnt) : 0.0);
    p->curcnt = sp->cnt;
  }
  //printf("****incx, y1,y2 = %g, %f, %f\n", p->inc, p->y1, p->y2);
  p->val = p->y1;
  return OK;
}

int32_t csgset_bkpt(CSOUND *csound, COSSEG *p)
{
  int32_t cnt, bkpt = 0;
  int32_t nsegs;
  int32_t n;
  SEG *segp;
  n = csgset(csound, p);
  if (UNLIKELY(n!=0)) return n;
  if (*p->argums[1] <= FL(0.0)) return OK;
  cnt = p->curcnt;
  nsegs = p->segsrem-1;
  segp = p->cursegp;
  if (IS_ASIG_ARG(p->rslt))
    while (nsegs-- > 0) {
      if (UNLIKELY(cnt > segp->acnt))
        return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
      segp->acnt -= cnt;
      cnt += segp->acnt;
      segp++;
      bkpt++;
    }
  else
    while (nsegs-- > 0) {
      //csound->Message(csound, "%d/ %d: %d, %d ", nsegs, bkpt, cnt, segp->cnt);
      if (UNLIKELY(cnt > segp->cnt))
        return csound->InitError(csound, Str("Breakpoint %d not valid"), bkpt);
      segp->cnt -= cnt;
      cnt += segp->cnt;
      //csound->Message(csound, "-> %d, %d %f\n", cnt, segp->cnt, segp->nxtpt);
      segp++;
      bkpt++;
    }

  return OK;
}

int32_t csgrset(CSOUND *csound, COSSEG *p)
{
  int32_t relestim;
  if (csgset(csound,p) != 0) return NOTOK;
  relestim = (p->cursegp + p->segsrem-2)->cnt;
  p->xtra = relestim;
  if (relestim > p->h.insdshead->xtratim)
    p->h.insdshead->xtratim = (int32_t)relestim;
  return OK;
}

int32_t kosseg(CSOUND *csound, COSSEG *p)
{
  cs_double val1 = p->y1, val2 = p->y2, x = p->x;
  cs_double inc = p->inc;

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */

  if (LIKELY(p->segsrem)) {             /* if no more segs putk */
    if (p->curcnt <= 0) {             /*  if done cur segment */
      SEG *segp = p->cursegp;
    chk1:
      p->y1 = val1 = val2;
      if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
        goto putk;                      /*      put endval      */
      }
      //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
      val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
      inc = p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
      x = 0.0;
      p->cursegp = segp+1;              /*   else find the next */
      if (UNLIKELY(!(p->curcnt = segp->cnt))) {
        val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
        /* inc = */ p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        goto chk1;
      }                                 /*   poslen = new slope */
    }
    {
      cs_double mu2 = (1.0-cos(x*PI))*0.5;
      *p->rslt = (cs_float)(val1*(1.0-mu2)+val2*mu2);
      x += inc;
      --p->curcnt;
    }
  }
  else {
  putk:
    *p->rslt = (cs_float)val1;
  }
  p->x = x;
  return OK;
 err1:
  return csound->InitError(csound, Str("cosseg not initialised (krate)\n"));
}

int32_t cosseg(CSOUND *csound, COSSEG *p)
{
  cs_double val1 = p->y1, val2 = p->y2, x = p->x;
  cs_float *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double inc = p->inc;///(nsmps-offset);

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

  if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&rs[nsmps], '\0', early*sizeof(cs_float));
  }

  for (n=offset; n<nsmps; n++) {
    cs_double mu2;
    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      if (p->curcnt <= 0) {             /*  if done cur segment */
        SEG *segp = p->cursegp;
      chk1:
        p->y1 = val1 = val2;
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          goto putk;                      /*      put endval      */
        }
        val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
        inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
        x = 0.0;
        //printf("****new seg val1.val2=%f,%f inc=%f\n", val1,val2, inc);
        p->cursegp = segp+1;              /*   else find the next */
        if (UNLIKELY(!(p->curcnt = segp->acnt))) {
          val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
          inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
          //printf("****val1,val2=%f,%f inc=%f\n", val1, val2, inc);
          goto chk1;
        }                                 /*   poslen = new slope */
      }
      mu2 = (1.0-cos(x*PI))*0.5;
      //printf("****x=%f inc=%f mu2=%f\n", x, inc, mu2);
      rs[n] = (cs_float)(val1*(1.0-mu2)+val2*mu2);
      x += inc;
      --p->curcnt;
    }
    else {
    putk:
      rs[n] = (cs_float)val1;

    }
  }
  p->inc = inc;
  p->x = x;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("cosseg: not initialised (arate)\n"));
}

int32_t cossegr(CSOUND *csound, COSSEG *p)
{
  cs_double val1 = p->y1, val2 = p->y2, x = p->x, val = p->val;
  cs_float *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double inc = p->inc;

  memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

  for (n=offset; n<nsmps; n++) {
    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      SEG *segp = p->cursegp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {            /* release flag new:    */
          segp = ++p->cursegp;              /*   go to last segment */
          p->segsrem--;
        }
        segp--;                                 /*   get univ relestim  */
        segp->acnt = (p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim)*CS_KSMPS;
        //p->y1 = val1 = val2;
        p->y1 = val1 = val;
        //printf("%d(%p): cnt=%d strt= %f nxtpt=%f\n",
        //      p->segsrem, segp, segp->cnt,val1, segp->nxtpt);
        goto newi;                          /*   and set new curinc */
      }
      if (p->segsrem == 1 && !p->h.insdshead->relesing) {
        goto putk;
      }
      if (p->curcnt <= 0) {             /*  if done cur segment */
      chk1:
        p->y1 = val1 = val2;
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          goto putk;                      /*      put endval      */
        }
      newi:
        //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
        val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
        inc =p->inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
        x = 0.0;
        p->cursegp = segp+1;              /*   else find the next */
        p->curcnt = segp->acnt;
        /* The release segment starts only after note-off. */
        if (p->segsrem == 1 && !p->h.insdshead->relesing)
          goto putk;
        if (UNLIKELY(!p->curcnt)) {
          val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
          inc = p->inc = (segp->acnt ? 1.0/(segp->acnt) : 0.0);
          goto chk1;
        }                                 /*   poslen = new slope */
        //printf("New segment incx, y1,y2 = %g, %f, %f\n", inc, val1, val2);
      }
      {
        cs_double mu2 = (1.0-cos(x*PI))*0.5;
        val = rs[n] = (cs_float)(val1*(1.0-mu2)+val2*mu2);
        x += inc;
        --p->curcnt;
        //if (x>1 || x<0) printf("x=%f out of range\n", x);
      }
    }
    else {
    putk:
      val = rs[n] = (cs_float)val1;
    }
  }
  p->inc = inc;
  p->x = x;
  p->val = val;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("cossegr: not initialised (arate)\n"));
}


#if 0
int32_t cossegr(CSOUND *csound, COSSEG *p)
{
  cs_double val1 = p->y1, val2 = p->y2, x = p->x;
  cs_float *rs = p->rslt;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t n, nsmps = CS_KSMPS;
  cs_double inc = p->inc/(nsmps-offset);

  memset(rs, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;

  if (LIKELY(p->segsrem)) {             /* if no more segs putk */
    SEG *segp = p->cursegp;
    if (p->h.insdshead->relesing && p->segsrem > 1) {
      while (p->segsrem > 1) {            /* release flag new:    */
        segp = ++p->cursegp;              /*   go to last segment */
        p->segsrem--;
      }                                   /*   get univ relestim  */
      segp->cnt = p->xtra >=0 ? p->xtra : p->h.insdshead->xtratim;
      goto newi;                          /*   and set new curinc */
    }
    if (--p->curcnt <= 0) {             /*  if done cur segment */
    chk1:
      p->y1 = val1 = val2;
      if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
        p->y2 = val2 = segp->nxtpt;
        goto putk;                      /*      put endval      */
      }
    newi:
      //printf("new seg: %d %f\n", segp->cnt, segp->nxtpt);
      val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
      p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
      inc /= nsmps;
      x = 0.0;
      p->cursegp = segp+1;              /*   else find the next */
      if (UNLIKELY(!(p->curcnt = segp->cnt))) {
        val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
        p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        inc /= nsmps;
        //printf("zero length: incx, y1,y2 = %f, %f, %f\n", inc, val1, val2);
        goto chk1;
      }                                 /*   poslen = new slope */
      //printf("New segment incx, y1,y2 = %g, %f, %f\n", inc, val1, val2);
    }
    for (n=offset; n<nsmps; n++) {
      cs_double mu2 = (1.0-cos(x*PI))*0.5;
      rs[n] = (cs_float)(val1*(1.0-mu2)+val2*mu2);
      x += inc;
      //if (x>1 || x<0) printf("x=%f out of range\n", x);
    }
  }
  else {
  putk:
    //printf("ending at %f\n", val1);
    for (n=offset; n<nsmps; n++) {
      rs[n] = (cs_float)val1;
    }
  }
  p->x = x;
  return OK;
 err1:
  return csound->PerfError(csound, &(p->h),
                           Str("cossegr: not initialised (arate)\n"));
}
#endif

int32_t kcssegr(CSOUND *csound, COSSEG *p)
{
  cs_double val1 = p->y1, val2 = p->y2, x = p->x, val = p->val;
  cs_double inc = p->inc;

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;          /* RWD fix */

  if (LIKELY(p->segsrem)) {             /* if no more segs putk */
    SEG *segp = p->cursegp;
    if (p->h.insdshead->relesing && p->segsrem > 1) {
      while (p->segsrem > 1) {           /* reles flag new:      */
        segp = ++p->cursegp;             /*   go to last segment */
        p->segsrem--;
      }
      segp--;                             /*   get univ relestim  */
      segp->cnt = p->xtra>= 0 ? p->xtra : p->h.insdshead->xtratim;
      p->y1 = val1 = val;
      goto newi;                         /*   and set new curinc */
    }
    if (p->segsrem == 1 && !p->h.insdshead->relesing) {
      goto putk;
    }
    if (p->curcnt <= 0) {             /*  if done cur segment */
    chk1:
      p->y1 = val1 = val2;
      if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
        goto putk;                      /*      put endval      */
      }
    newi:
      val2 = p->y2 = segp->nxtpt;          /* Base of next segment */
      inc = p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
      x = 0.0;
      p->cursegp = segp+1;              /*   else find the next */
      p->curcnt = segp->cnt;
      /* The release segment starts only after note-off. */
      if (p->segsrem == 1 && !p->h.insdshead->relesing)
        goto putk;
      if (UNLIKELY(!p->curcnt)) {
        val2 = p->y2 = segp->nxtpt;  /* nonlen = discontin */
        /* inc = */ p->inc = (segp->cnt ? 1.0/(segp->cnt) : 0.0);
        goto chk1;
      }                                 /*   poslen = new slope */
    }
    {
      cs_double mu2 = (1.0-cos(x*PI))*0.5;
      val = *p->rslt = (cs_float)(val1*(1.0-mu2)+val2*mu2);
      x += inc;
      --p->curcnt;
    }
  }
  else {
  putk:
    val = *p->rslt = (cs_float)val1;
  }
  p->x = x;
  p->val = val;
  return OK;
 err1:
  return csound->InitError(csound, Str("cosseg not initialised (krate)\n"));
}
