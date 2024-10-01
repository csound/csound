/*
  ugens7.c:

  Copyright (C) 1995 J. Michael Clarke, based on ideas from CHANT (IRCAM),
  Barry Vercoe, John ffitch
  float phase version: (C) 2024 Victor Lazzarini 

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

#include "stdopcod.h"               /*                      UGENS7.C        */
#include "ugens7.h"
#include <math.h>

/* loosely based on code of Michael Clarke, University of Huddersfield */

static   int32_t    newpulse(CSOUND *, FOFS *, OVRLAP *, MYFLT *, MYFLT *, MYFLT *);

static int32_t fofset0(CSOUND *csound, FOFS *p, int32_t flag)
{
  int32_t skip = (*p->iskip != FL(0.0) && p->auxch.auxp != 0);
  if (LIKELY((p->ftp1 = csound->FTFind(csound, p->ifna)) != NULL &&
             (p->ftp2 = csound->FTFind(csound, p->ifnb)) != NULL)) {  
    OVRLAP *ovp, *nxtovp;
    int32  olaps;
    // VL 22.03.24 check len to set float phase flag
    if(IS_POW_TWO(p->ftp1->flen) && IS_POW_TWO(p->ftp1->flen))
      p->floatph = 0;
    else p->floatph = 1;
 
    p->durtogo = (int32)(*p->itotdur * CS_ESR);
    if (!skip) { /* legato: skip all memory management */
      if(!p->floatph) {
        if (*p->iphs == FL(0.0))                /* if fundphs zero,  */
          p->fundphs = MAXLEN;                  /*   trigger new FOF */
        else p->fundphs = (int32)(*p->iphs * FMAXLEN) & PHMASK;
        p->fundphsf = 0.;
      } else {
        if (*p->iphs == FL(0.0))                /* if fundphs zero,  */
          p->fundphsf = 1.;                  /*   trigger new FOF */
        else p->fundphsf = PHMOD1(*p->iphs);
        p->fundphs = 0;
      }
      if (UNLIKELY((olaps = (int32)*p->iolaps) <= 0)) {
        return csound->InitError(csound, "%s", Str("illegal value for iolaps"));
      }
      if (*p->iphs >= FL(0.0))
        csound->AuxAlloc(csound, (size_t)olaps * sizeof(OVRLAP), &p->auxch);
      ovp = &p->basovrlap;
      nxtovp = (OVRLAP *) p->auxch.auxp;
      do {
        ovp->nxtact = NULL;
        ovp->nxtfree = nxtovp;                /* link the ovlap spaces */
        ovp = nxtovp++;
      } while (--olaps);
      ovp->nxtact = NULL;
      ovp->nxtfree = NULL;
      p->fofcount = -1;
      p->prvband = FL(0.0);
      p->expamp = FL(1.0);
      p->prvsmps = (int32)0;
      p->preamp = FL(1.0);
    } /* end of legato code */
    p->ampcod   = IS_ASIG_ARG(p->xamp) ? 1 : 0;
    p->fundcod  = IS_ASIG_ARG(p->xfund) ? 1 : 0;
    p->formcod  = IS_ASIG_ARG(p->xform) ? 1 : 0;
    p->xincod   = p->ampcod || p->fundcod || p->formcod;
    if (flag)
      p->fmtmod = (*p->ifmode == FL(0.0)) ? 0 : 1;
  }
  else return NOTOK;
  p->foftype = flag;
  return OK;
}

static int32_t fofset(CSOUND *csound, FOFS *p)
{
  return fofset0(csound, p, 1);
}

static int32_t fofset2(CSOUND *csound, FOFS *p)
{
  return fofset0(csound, p, 0);
}


static int32_t fof(CSOUND *csound, FOFS *p)
{
  OVRLAP  *ovp;
  FUNC    *ftp1,  *ftp2;
  MYFLT   *ar, *amp, *fund, *form;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32   fund_inc, form_inc, floatph = p->floatph;
  double  form_incf, fund_incf;
  MYFLT   v1, fract ,*ftab;

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
  ar = p->ar;
  amp = p->xamp;
  fund = p->xfund;
  form = p->xform;
  ftp1 = p->ftp1;
  ftp2 = p->ftp2;
  
  
  if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
  }
  if(floatph) {
    form_incf = *form * CS_ONEDSR;
    fund_incf = *fund * CS_ONEDSR;
  }
  else {
    form_inc = (int32)(*form * CS_SICVT);
    fund_inc = (int32)(*fund * CS_SICVT);
  }
  for (n=offset; n<nsmps; n++) {
    if (p->fundphs & MAXLEN ||
        p->fundphsf >= 1.) {
      /* if phs has wrapped */
      if (floatph) 
        p->fundphsf = PHMOD1(p->fundphsf);
      else 
        p->fundphs &= PHMASK;
      
      if ((ovp = p->basovrlap.nxtfree) == NULL) goto err2;
      if (newpulse(csound, p, ovp, amp, fund, form)) {   /* init new fof */
        ovp->nxtact = p->basovrlap.nxtact;     /* & link into  */
        p->basovrlap.nxtact = ovp;             /*   actlist    */
        p->basovrlap.nxtfree = ovp->nxtfree;
      }
    }
    ar[n] = FL(0.0);
    ovp = &p->basovrlap;
    while (ovp->nxtact != NULL) {         /* perform cur actlist:  */
      MYFLT  result;
      OVRLAP *prvact = ovp;
      ovp = ovp->nxtact;                   /*  formant waveform  */
      if(floatph) {
        double formphsf = ovp->formphsf;
        double frac = formphsf - (int32_t) formphsf; 
        ftab = ftp1->ftable + (size_t) (formphsf * ftp1->flen);
        v1 = *ftab++;  
        result = v1 + (*ftab - v1) * frac;
        if (p->foftype) {
          if (p->fmtmod)
            formphsf += form_incf;           /* inc phs on mode */
          else formphsf += ovp->formincf;
        } else 
          formphsf += (ovp->formincf + ovp->glissbas * ovp->sampct++);
        ovp->formphsf = PHMOD1(formphsf);
        if (ovp->risphsf < 1.) {             /*  formant ris envlp */
          result *= *(ftp2->ftable + (size_t) (ovp->risphsf * ftp2->flen));
          ovp->risphsf += ovp->risincf;
        }
        if (ovp->timrem <= ovp->dectim) {       /*  formant dec envlp */
          result *= *(ftp2->ftable + (size_t) (ovp->decphsf * ftp2->flen));
          if ((ovp->decphsf -= ovp->decincf) < 0.)
            ovp->decphsf = 0.;
        } 
      } else {
        fract = PFRAC1(ovp->formphs);        /* from JMC Fog*/
        ftab = ftp1->ftable + (ovp->formphs >> ftp1->lobits);/*JMC Fog*/
        v1 = *ftab++;                           /*JMC Fog*/
        result = v1 + (*ftab - v1) * fract;     /*JMC Fog*/
        /*              result = *(ftp1->ftable + (ovp->formphs >> ftp1->lobits) ); */
        if (p->foftype) {
          if (p->fmtmod)
            ovp->formphs += form_inc;           /* inc phs on mode */
          else ovp->formphs += ovp->forminc;
        }
        else {
#define kgliss ifmode
          /* MYFLT ovp->glissbas = kgliss / grain length. ovp->sampct is
             incremented each sample. We add glissbas * sampct to the
             pitch of grain at each a-rate pass (ovp->formphs is the
             index into ifna; ovp->forminc is the stepping factor that
             decides pitch) */
          ovp->formphs += (int32)(ovp->forminc + ovp->glissbas * ovp->sampct++);
        }
        ovp->formphs &= PHMASK;
        if (ovp->risphs < MAXLEN) {             /*  formant ris envlp */
          result *= *(ftp2->ftable + (ovp->risphs >> ftp2->lobits) );
          ovp->risphs += ovp->risinc;
        }
        if (ovp->timrem <= ovp->dectim) {       /*  formant dec envlp */
          result *= *(ftp2->ftable + (ovp->decphs >> ftp2->lobits) );
          if ((ovp->decphs -= ovp->decinc) < 0)
            ovp->decphs = 0;
        }
      }
      ar[n] += (result * ovp->curamp);        /*  add wavfrm to out */
      if (--ovp->timrem)                      /*  if fof not expird */
        ovp->curamp *= ovp->expamp;           /*   apply bw exp dec */
      else {
        prvact->nxtact = ovp->nxtact;         /*  else rm frm activ */
        ovp->nxtfree = p->basovrlap.nxtfree;  /*  & ret spc to free */
        p->basovrlap.nxtfree = ovp;
        ovp = prvact;
      }
    }
    if(floatph) {
      p->fundphsf  += fund_incf;
      if (p->xincod) {
        if (p->ampcod)    amp++;
        if (p->fundcod)   fund_incf = (*++fund * CS_ONEDSR);
        if (p->formcod)   form_incf = (*++form * CS_ONEDSR);
      }
    }
    else {
      p->fundphs += fund_inc;
      if (p->xincod) {
        if (p->ampcod)    amp++;
        if (p->fundcod)   fund_inc = (int32)(*++fund * CS_SICVT);
        if (p->formcod)   form_inc = (int32)(*++form * CS_SICVT);
      }
    }
    p->durtogo--;    
  }
  return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("fof: not initialised"));
 err2:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("FOF needs more overlaps"));
}

static int32_t newpulse(CSOUND *csound,
                        FOFS *p, OVRLAP *ovp, MYFLT *amp, MYFLT *fund, MYFLT *form)
{
  MYFLT   octamp = *amp, oct;
  int32   rismps, newexp = 0;

  if ((ovp->timrem = (int32)(*p->kdur * CS_ESR)) > p->durtogo &&
      (*p->iskip==FL(0.0))) /* ringtime */
    return(0);
  if ((oct = *p->koct) > FL(0.0)) {                   /* octaviation */
    int64_t csnt = -1;
    int64_t ioct = (int64_t) oct;
    int64_t bitpat = ~(csnt << ioct);
    if (bitpat & ++p->fofcount)
      return(0);
    if ((bitpat += 1) & p->fofcount)
      octamp *= (FL(1.0) + ioct - oct);
  }
  if(p->floatph) {
    if (*fund == FL(0.0))                               /* formant phs */
      ovp->formphsf = 0.;
    else ovp->formphsf =  PHMOD1(p->fundphsf * *form / *fund);
    ovp->formincf = (*form * CS_ONEDSR);
  }
  else{
    if (*fund == FL(0.0))                               /* formant phs */
      ovp->formphs = 0;
    else ovp->formphs = (int32)(p->fundphs * *form / *fund) & PHMASK;
    ovp->forminc = (int32)(*form * CS_SICVT);
  }
    
    
  if (*p->kband != p->prvband) {                    /* bw: exp dec */
    p->prvband = *p->kband;
    p->expamp =  EXP(*p->kband * -CS_PIDSR);
    newexp = 1;
  }
  /* Init grain rise ftable phase. Negative kform values make
     the kris (ifnb) initial index go negative and crash csound.
     So insert another if-test with compensating code. */
  if (*p->kris >= CS_ONEDSR && *form != FL(0.0)) {   /* init fnb ris */
    if(p->floatph) {
      if (*form < FL(0.0) && ovp->formphsf != 0.)
        ovp->risphsf = ((1. - ovp->formphsf) / -*form / *p->kris);
      else {
        ovp->risphsf = (ovp->formphsf / *form / *p->kris);
        
      }
      ovp->risincf = (CS_ONEDSR / *p->kris);
      rismps = (int32_t) (1. / ovp->risincf);  
    } else {
      if (*form < FL(0.0) && ovp->formphs != 0)
        ovp->risphs = (int32)((MAXLEN - ovp->formphs) / -*form / *p->kris);
      else
        ovp->risphs = (int32)(ovp->formphs / *form / *p->kris);
      ovp->risinc = (int32)(CS_SICVT / *p->kris);
      rismps = MAXLEN / ovp->risinc;
    }
  }
  else {
    if(p->floatph) ovp->risphsf = 1.;
    else ovp->risphs = MAXLEN;
    rismps = 0;
  }
    
  if (newexp || rismps != p->prvsmps) {            /* if new params */
    if ((p->prvsmps = rismps))                     /*   redo preamp */
      p->preamp = intpow(p->expamp, -rismps);
    else p->preamp = FL(1.0);
  }
  ovp->curamp = octamp * p->preamp;                /* set startamp  */
  ovp->expamp = p->expamp;

  if(p->floatph) {
    if ((ovp->dectim = (int32)(*p->kdec * CS_ESR)) > 0) 
      ovp->decincf = (CS_ONEDSR / *p->kdec);
    ovp->decphsf = PHMOD1(ovp->decphsf);
  } else {
    if ((ovp->dectim = (int32)(*p->kdec * CS_ESR)) > 0) 
      ovp->decinc = (int32)(CS_SICVT / *p->kdec);
    ovp->decphs = PHMASK;
  }
 
  if (!p->foftype) {
    /* Make fof take k-rate phase increment:
       Add current iphs to initial form phase */
    if(p->floatph) {
      ovp->formphsf += *p->iphs;
      ovp->formphsf = PHMOD1(ovp->formphsf);
      ovp->glissbas = ovp->formincf * (MYFLT)pow(2.0, (double)*p->kgliss);
      ovp->glissbas -= ovp->formincf;
      ovp->glissbas /= ovp->timrem;
    } else {
      ovp->formphs += (int32)(*p->iphs * FMAXLEN);           /*  krate phs */
      ovp->formphs &= PHMASK;
      /* Set up grain gliss increment: ovp->glissbas will be added to
         ovp->forminc at each pass in fof2. Thus glissbas must be
         equal to kgliss / grain playing time. Also make it harmonic,
         so integer kgliss can represent octaves (ie pow() call). */
      ovp->glissbas = ovp->forminc * (MYFLT)pow(2.0, (double)*p->kgliss);
      /* glissbas should be diff of start & end pitch*/
      ovp->glissbas -= ovp->forminc;
      ovp->glissbas /= ovp->timrem;
      ovp->sampct = 0;   /* Must be reset in case ovp was used before  */
    }
  }
  return(1);
}


static int32_t harmset(CSOUND *csound, HARMON *p)
{
  MYFLT minfrq = *p->ilowest;
  if (UNLIKELY(minfrq < FL(64.0))) {
    return csound->InitError(csound,  "%s", Str("Minimum frequency too low"));
  }
  if (p->auxch.auxp == NULL || minfrq < p->minfrq) {
    int32 nbufs = (int32)(CS_EKR * FL(3.0) / minfrq) + 1;
    int32 nbufsmps = nbufs * CS_KSMPS;
    int32 maxprd = (int32)(CS_ESR / minfrq);
    int32 totalsiz = nbufsmps * 5 + maxprd; /* Surely 5! not 4 */
    /* printf("init: nbufs = %d; nbufsmps = %d; maxprd = %d; totalsiz = %d\n", */
    /*        nbufs, nbufsmps, maxprd, totalsiz);       */
    csound->AuxAlloc(csound, (size_t)totalsiz * sizeof(MYFLT), &p->auxch);
    p->bufp = (MYFLT *) p->auxch.auxp;
    p->midp = p->bufp + nbufsmps;        /* each >= maxprd * 3 */
    p->bufq = p->midp + nbufsmps;
    p->midq = p->bufq + nbufsmps;
    p->autobuf = p->midq + nbufsmps;     /* size of maxprd */
    p->nbufsmps = nbufsmps;
    p->n2bufsmps = nbufsmps * 2;
    p->lomaxdist = maxprd;
    p->minfrq = minfrq;
  }
  if ((p->autoktim = (int32_t)/*MYFLT2LONG*/(*p->iptrkprd * CS_EKR)) < 1)
    p->autoktim = 1;
  p->autokcnt = 1;              /* init for immediate autocorr attempt */
  printf("ekr = %f iptrk = %f, autocnt = %d; autotim = %d\n",
         CS_EKR, *p->iptrkprd, p->autokcnt, p->autoktim);
  p->lsicvt = FL(65536.0) * CS_ONEDSR;
  p->cpsmode = ((*p->icpsmode != FL(0.0)));
  p->inp1 = p->bufp;
  p->inp2 = p->midp;
  p->inq1 = p->bufq;
  p->inq2 = p->midq;
  p->puls1 = NULL;
  p->puls2 = NULL;
  p->puls3 = NULL;
  p->prvest = FL(0.0);
  p->prvq = FL(0.0);
  p->phase1 = 0;
  p->phase2 = 0;
#if 0
  hrngflg = 0;
  p->period = -1;
#endif
  return OK;
}

#if 0
static int32_t hrngflg=0;
#endif
static int32_t harmon(CSOUND *csound, HARMON *p)
{
    MYFLT *src1, *src2, *src3, *inp1, *inp2, *outp;
    MYFLT c1, c2, qval, *inq1, *inq2;
    MYFLT sum, minval, *minqp = NULL, *minq1, *minq2, *endp;
    MYFLT *pulstrt, lin1, lin2, lin3;
    int32  cnt1, cnt2, cnt3;
    int32  nn, phase1, phase2, phsinc1, phsinc2, period;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    outp = p->ar;
#if 0
    if (early || offset) printf("early=%d, offet=%d\n", early, offset);
#endif
    if (UNLIKELY(offset)) memset(outp, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outp[nsmps], '\0', early*sizeof(MYFLT));
    }
    inp1 = p->inp1;
    inp2 = p->inp2;
    inq1 = p->inq1;
    inq2 = p->inq2;
    qval = p->prvq;
    if (*p->kest != p->prvest &&
        *p->kest != FL(0.0)) {    /* if new pitch estimate */
      MYFLT estperiod = CS_ESR / *p->kest;
      double b = 2.0 - cos((double)(*p->kest * CS_TPIDSR));
      p->c2 = (MYFLT)(b - sqrt(b*b - 1.0)); /*   recalc lopass coefs */
      p->c1 = FL(1.0) - p->c2;
      p->prvest = *p->kest;
      p->estprd = estperiod;
      p->prvar = FL(0.0);
    }
    if (*p->kvar != p->prvar) {
      MYFLT oneplusvar = FL(1.0) + *p->kvar;
      /* prd window is prd +/- var int32_t */
      p->mindist = (int32)(p->estprd/oneplusvar);
/*       if (p->mindist==0) p->mindist=1; */
      p->maxdist = (int32)(p->estprd*oneplusvar);
      if (p->maxdist > p->lomaxdist)
        p->maxdist = p->lomaxdist;
      p->max2dist = p->maxdist * 2;
      p->prvar = *p->kvar;
    }
    c1 = p->c1;
    c2 = p->c2;
    //printf("cycle %d\n", ++cycle);
    for (src1 = p->asig, n = offset; n<nsmps; n++) {
      *inp1++ = *inp2++ = src1[n];            /* dbl store the wavform */
      //printf("src[%d] = %f\n", n, src1[n]);
      if (src1[n] > FL(0.0))
        qval = c1 * src1[n] + c2 * qval;      /*  & its half-wave rect */
      else qval = c2 * qval;
      *inq1++ = *inq2++ = qval;
    }
    if (!(--p->autokcnt)) {                   /* if time for new autocorr  */
      MYFLT *mid1, *mid2, *src4;
      MYFLT *autop, *maxp;
      MYFLT dsum, dinv, win, windec, maxval;
      int32  dist;
      //printf("AUTOCORRELATE min/max = %d,%d\n",p->mindist, p->maxdist);
      p->autokcnt = p->autoktim;
      mid2 = inp2 - p->max2dist;
      mid1 = mid2 - 1;
      autop = p->autobuf;
      for (dist = p->mindist; dist <= p->maxdist; dist++) {
        dsum = FL(0.0);
        dinv = FL(1.0) / dist;
        src1 = mid1;  src3 = mid1 + dist;
        src2 = mid2;  src4 = mid2 + dist;
        for (win = FL(1.0), windec = dinv, nn = dist; nn--; ) {
          dsum += win * (*src1 * *src3 + *src2 * *src4);
          //printf("dsum = %f from %f %f %f %f\n", dsum, *src1, *src2, *src3, *src4);
          src1--; src2++; src3--; src4++;
          win -= windec;
        }
        *autop++ = dsum * dinv;
      }
      maxval = FL(0.0);
      maxp = autop = p->autobuf;
      endp = autop + p->maxdist - p->mindist;
      while (autop < endp) {
        //printf("newval, maxval = %f, %f\n", *autop, maxval);
        if (*autop > maxval) {          /* max autocorr gives new period */
          maxval = *autop;
          maxp = autop;
#if 0
          csound->Message(csound, "new maxval %f at %p\n", maxval, (int64_t)maxp);
#endif
        }
        autop++;
      }
      //printf("**** maxval = %f ****\n", maxval);
      period = (int32_t) (p->mindist + maxp - p->autobuf);
      if (period != p->period) {
#if 0
        csound->Message(csound, "New period %d %d\n", period, p->period);
#endif
        p->period = period;
        if (!p->cpsmode)
          p->lsicvt = FL(65536.0) / period;
        p->pnt1 = (int32)((MYFLT)period * FL(0.2));
        p->pnt2 = (int32)((MYFLT)period * FL(0.8));
        p->pnt3 = period;
        p->inc1 = FL(1.0) / p->pnt1;
        p->inc2 = FL(1.0) / (period - p->pnt2);
      }
    }
    else period = p->period;

    minval = (MYFLT)HUGE_VAL;               /* Suitably large ! */
    minq2 = inq2 - period;                  /* srch the qbuf for minima */
    minq1 = minq2 - period;                 /* which are 1 period apart */
    endp = inq2;                            /* move srch over 1 period  */
    while (minq2 < endp) {
      if ((sum = *minq1 + *minq2) < minval) {
        minval = sum;
        minqp = minq1;
      }
      minq1++; minq2++;
    }
    src1 = minqp - p->n2bufsmps;            /* get src equiv of 1st min  */
    if (period==0) {
      csound->Warning(csound, "%s", Str("Period zero\n"));
      outp = p->ar;
      memset(outp, 0, sizeof(MYFLT)*CS_KSMPS);
      return OK;
    }
    while (src1 + CS_KSMPS > inp2)     /* if not enough smps presnt */
      src1 -= period;                       /*      back up 1 prd        */
    pulstrt = src1;                         /* curr available pulse beg  */

    src1 = p->puls1;                        /* insert pulses into output */
    src2 = p->puls2;
    src3 = p->puls3;
    lin1 = p->lin1;
    lin2 = p->lin2;
    lin3 = p->lin3;
    cnt1 = p->cnt1;
    cnt2 = p->cnt2;
    cnt3 = p->cnt3;
    phase1 = p->phase1;
    phase2 = p->phase2;
    phsinc1 = (int32)(*p->kfrq1 * p->lsicvt);
    phsinc2 = (int32)(*p->kfrq2 * p->lsicvt);
    for (n=offset; n<nsmps; n++) {
      MYFLT sum;
      if (src1 != NULL) {
        if (++cnt1 < p->pnt11) {
          sum = *src1++ * lin1;
          lin1 += p->inc11;
        }
        else if (cnt1 <= p->pnt12)
          sum = *src1++;
        else if (cnt1 <= p->pnt13) {
          sum = *src1++ * lin1;
          lin1 -= p->inc12;
        }
        else {
          sum = FL(0.0);
          src1 = NULL;
        }
      }
      else sum = FL(0.0);
      if (src2 != NULL) {
        if (++cnt2 < p->pnt21) {
          sum += *src2++ * lin2;
          lin2 += p->inc21;
        }
        else if (cnt2 <= p->pnt22)
          sum += *src2++;
        else if (cnt2 <= p->pnt23) {
          sum += *src2++ * lin2;
          lin2 -= p->inc22;
        }
        else src2 = NULL;
      }
      if (src3 != NULL) {
        if (++cnt3 < p->pnt31) {
          sum += *src3++ * lin3;
          lin3 += p->inc31;
        }
        else if (cnt3 <= p->pnt32)
          sum += *src3++;
        else if (cnt3 <= p->pnt33) {
          sum += *src3++ * lin3;
          lin3 -= p->inc32;
        }
        else src3 = NULL;
      }
      if ((phase1 += phsinc1) & (~0xFFFFL)) { /* 64bit safe! */
        phase1 &= 0x0000FFFFL;
        if (src1 == NULL) {
          src1 = pulstrt;
          cnt1 = 0;
          lin1 = p->inc1;
          p->inc11 = p->inc1;
          p->inc12 = p->inc2;
          p->pnt11 = p->pnt1;
          p->pnt12 = p->pnt2;
          p->pnt13 = p->pnt3;
        }
        else if (src2 == NULL) {
          src2 = pulstrt;
          cnt2 = 0;
          lin2 = p->inc1;
          p->inc21 = p->inc1;
          p->inc22 = p->inc2;
          p->pnt21 = p->pnt1;
          p->pnt22 = p->pnt2;
          p->pnt23 = p->pnt3;
        }
        else if (src3 == NULL) {
          src3 = pulstrt;
          cnt3 = 0;
          lin3 = p->inc1;
          p->inc31 = p->inc1;
          p->inc32 = p->inc2;
          p->pnt31 = p->pnt1;
          p->pnt32 = p->pnt2;
          p->pnt33 = p->pnt3;
        }
#if 0
        else if (UNLIKELY(++hrngflg > 200)) {
          csound->Message(csound, "%s", Str("harmon out of range...\n"));
          hrngflg = 0;
        }
#endif
      }
      if ((phase2 += phsinc2) & (~0xFFFFL)) {
        phase2 &= 0x0000FFFFL;
        if (src1 == NULL) {
          src1 = pulstrt;
          cnt1 = 0;
          lin1 = p->inc1;
          p->inc11 = p->inc1;
          p->inc12 = p->inc2;
          p->pnt11 = p->pnt1;
          p->pnt12 = p->pnt2;
          p->pnt13 = p->pnt3;
        }
        else if (src2 == NULL) {
          src2 = pulstrt;
          cnt2 = 0;
          lin2 = p->inc1;
          p->inc21 = p->inc1;
          p->inc22 = p->inc2;
          p->pnt21 = p->pnt1;
          p->pnt22 = p->pnt2;
          p->pnt23 = p->pnt3;
        }
        else if (src3 == NULL) {
          src3 = pulstrt;
          cnt3 = 0;
          lin3 = p->inc1;
          p->inc31 = p->inc1;
          p->inc32 = p->inc2;
          p->pnt31 = p->pnt1;
          p->pnt32 = p->pnt2;
          p->pnt33 = p->pnt3;
        }
#if 0
        else if (UNLIKELY(++hrngflg > 200)) {
          csound->Message(csound, "%s", Str("harmon out of range\n"));
          hrngflg = 0;
        }
#endif
      }
      outp[n] = sum;
    }
    if (inp1 >= p->midp) {
      p->inp1 = p->bufp;
      p->inp2 = p->midp;
      p->inq1 = p->bufq;
      p->inq2 = p->midq;
      if (src1 != NULL)
        src1 -= p->nbufsmps;
      if (src2 != NULL)
        src2 -= p->nbufsmps;
      if (src3 != NULL)
        src3 -= p->nbufsmps;
    }
    else {
      p->inp1 = inp1;
      p->inp2 = inp2;
      p->inq1 = inq1;
      p->inq2 = inq2;
    }
    p->puls1 = src1;
    p->puls2 = src2;
    p->puls3 = src3;
    p->lin1 = lin1;
    p->lin2 = lin2;
    p->lin3 = lin3;
    p->cnt1 = cnt1;
    p->cnt2 = cnt2;
    p->cnt3 = cnt3;
    p->phase1 = phase1;
    p->phase2 = phase2;
    p->prvq = qval;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "fof",    S(FOFS),   TR,  "a","xxxkkkkkiiiiooo",(SUBR)fofset,(SUBR)fof   },
   { "fof2",   S(FOFS),   TR,  "a","xxxkkkkkiiiikko",(SUBR)fofset2,(SUBR)fof  },
   { "harmon", S(HARMON), 0, "a",  "akkkkiii",(SUBR)harmset,  (SUBR)harmon  }
};

int32_t ugens7_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

