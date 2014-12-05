/*
    ugensa.c:

    Copyright (C) 1997 J. Michael Clarke, based on ideas from CHANT (IRCAM)

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

#include "stdopcod.h"                 /*                              UGENSA.C  */
#include "ugensa.h"
#include "ugens7.h"
#include <math.h>

/* FOG generator */

static int newpulse(CSOUND *, FOGS *, OVERLAP *, MYFLT *, MYFLT *, MYFLT *);

static int fogset(CSOUND *csound, FOGS *p)
{
    /* legato test, not sure if the last bit (auxch) is correct? */
    int skip = (*p->iskip != FL(0.0) && p->auxch.auxp != 0);
    if ((p->ftp1 = csound->FTFind(csound, p->ifna)) != NULL &&
        (p->ftp2 = csound->FTFind(csound, p->ifnb)) != NULL) {
      OVERLAP *ovp, *nxtovp;
      int32   olaps;
      p->fogcvt = FMAXLEN/(p->ftp1)->flen; /*JMC for FOG*/
      p->durtogo = (int32)(*p->itotdur * CS_ESR);
      if (!skip) { /* legato: skip all memory management */
        p->spdphs = 0L; /*JMC for FOG*/
        if (*p->iphs == FL(0.0))                  /* if fundphs zero,  */
          p->fundphs = MAXLEN;                    /*   trigger new FOF */
        else p->fundphs = (int32)(*p->iphs * FMAXLEN) & PHMASK;
        if (UNLIKELY((olaps = (int32)*p->iolaps) <= 0)) {
          return csound->InitError(csound, Str("illegal value for iolaps"));
        }
        if (*p->iphs>=FL(0.0))
          csound->AuxAlloc(csound, (size_t)olaps * sizeof(OVERLAP), &p->auxch);
        ovp = &p->basovrlap;
        nxtovp = (OVERLAP *) p->auxch.auxp;
        do {
          ovp->nxtact = NULL;
          ovp->nxtfree = nxtovp;              /* link the ovlap spaces */
          ovp = nxtovp++;
        } while (--olaps);
        ovp->nxtact  = NULL;
        ovp->nxtfree = NULL;
        p->fofcount  = -1;
        p->prvband   = FL(0.0);
        p->expamp    = FL(1.0);
        p->prvsmps   = 0;
        p->preamp    = FL(1.0);
      }
      p->ampcod   = IS_ASIG_ARG(p->xamp) ? 1 : 0;
      p->fundcod  = IS_ASIG_ARG(p->xdens) ? 1 : 0;
      p->formcod  = IS_ASIG_ARG(p->xtrans) ? 1 : 0;
      p->xincod   = p->ampcod || p->fundcod || p->formcod;
/* p->speedcod  = (p->XINCODE & 0x8) ? 1 : 0; */ /*out for phs version of fog*/
      p->fmtmod    = (*p->itmode == 0.0) ? 0 : 1;
    }
    else return NOTOK;
    return OK;
}

static int fog(CSOUND *csound, FOGS *p)
{
    OVERLAP *ovp;
    FUNC        *ftp1,  *ftp2;
    MYFLT       *ar, *amp, *fund, *ptch, *speed;
    MYFLT  v1, fract ,*ftab, fogcvt = p->fogcvt; /*JMC added for FOG*/
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32   fund_inc, form_inc;
    /* long speed_inc; */ /*JMC added last--out for phs version*/

    ar = p->ar;
    amp = p->xamp;
    fund = p->xdens;
    ptch = p->xtrans;
    speed = p->xspd;
    ftp1 = p->ftp1;
    ftp2 = p->ftp2;
    fund_inc = (int32)(*fund * csound->sicvt);
    form_inc = (int32)(*ptch * fogcvt);  /*form_inc = *form * csound->sicvt;*/
/*      speed_inc = *speed * fogcvt; */   /*JMC for FOG--out for phs version*/
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      if (p->fundphs & MAXLEN) {                       /* if phs has wrapped */
        p->fundphs &= PHMASK;
        if (UNLIKELY((ovp = p->basovrlap.nxtfree) == NULL)) goto err1;
        if (newpulse(csound, p, ovp, amp, fund, ptch)) { /* init new fof */
          ovp->nxtact = p->basovrlap.nxtact;           /* & link into  */
          p->basovrlap.nxtact = ovp;                   /*   actlist    */
          p->basovrlap.nxtfree = ovp->nxtfree;
        }
      }
      ar[n] = FL(0.0);
      ovp = &p->basovrlap;
      while (ovp->nxtact != NULL) {         /* perform cur actlist:  */
        MYFLT result;
        OVERLAP *prvact = ovp;
        ovp = ovp->nxtact;                     /*  formant waveform  */
        fract = PFRAC1(ovp->formphs);                   /*JMC Fog*/
        ftab = ftp1->ftable + (ovp->formphs >> ftp1->lobits);/*JMC Fog*/
        v1 = *ftab++;                                   /*JMC Fog*/
        result = v1 + (*ftab - v1) * fract;             /*JMC Fog*/
/*  result = *(ftp1->ftable + (ovp->formphs >> ftp1->lobits) ); FOF version*/
        if (p->fmtmod)
          ovp->formphs += form_inc;         /*   inc phs on mode  */
        else ovp->formphs += ovp->forminc;
        ovp->formphs &= PHMASK;
        if (ovp->risphs < MAXLEN) {           /*  formant ris envlp */
          result *= *(ftp2->ftable + (ovp->risphs >> ftp2->lobits) );
          ovp->risphs += ovp->risinc;
        }
        if (ovp->timrem <= ovp->dectim) {     /*  formant dec envlp */
          result *= *(ftp2->ftable + (ovp->decphs >> ftp2->lobits) );
          if ((ovp->decphs -= ovp->decinc) < 0)
            ovp->decphs = 0;
        }
        ar[n] += (result * ovp->curamp);        /*  add wavfrm to out */
        if (--ovp->timrem)                    /*  if fof not expird */
          ovp->curamp *= ovp->expamp;       /*   apply bw exp dec */
        else {
          prvact->nxtact = ovp->nxtact;     /*  else rm frm activ */
          ovp->nxtfree = p->basovrlap.nxtfree;/*  & ret spc to free */
          p->basovrlap.nxtfree = ovp;
          ovp = prvact;
        }
      }
      p->fundphs += fund_inc;
/*          p->spdphs += speed_inc; */ /*JMC for FOG*/
      p->spdphs = (int32)(speed[n] * FMAXLEN); /*for phs version of FOG*/
      p->spdphs &= PHMASK; /*JMC for FOG*/
      if (p->xincod) {
        if (p->ampcod)    amp++;
        if (p->fundcod)   fund_inc = (int32)(*++fund * csound->sicvt);
        if (p->formcod)   form_inc = (int32)(*++ptch * fogcvt);
/*form_inc = *++form * csound->sicvt;*/
/*      if (p->speedcod)  speed_inc = *++speed * fogcvt; */  /*JMC for FOG*/
      }
      p->durtogo--;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("FOF needs more overlaps"));
}

static int newpulse(CSOUND *csound, FOGS *p, OVERLAP *ovp, MYFLT   *amp,
                    MYFLT *fund, MYFLT *ptch)
{
    MYFLT       octamp = *amp, oct;
    MYFLT       form = *ptch / csound->sicvt, fogcvt = p->fogcvt;
    int32   rismps, newexp = 0;
    if ((ovp->timrem = (int32)(*p->kdur * CS_ESR)) > p->durtogo &&
        (*p->iskip==FL(0.0)))  /* ringtime    */
      return(0);
    if ((oct = *p->koct) > 0.0) {                   /* octaviation */
      int32 ioct = (int32)oct, bitpat = ~(-1L << ioct);
      if (bitpat & ++p->fofcount)
        return(0);
      if ((bitpat += 1) & p->fofcount)
        octamp *= (FL(1.0) + ioct - oct);
    }
    if (*fund == 0.0)                               /* formant phs */
      ovp->formphs = 0;
/*  else
     ovp->formphs = (int32)((p->fundphs * form / *fund) + p->spdphs) & PHMASK; */
    else ovp->formphs = (int32)(p->fundphs * form / *fund) & PHMASK;
    ovp->forminc = (int32)(*ptch * fogcvt);/*JMC for FOG*/
    /*ovp->forminc = *form * csound->sicvt;*/
    if (*p->kband != p->prvband) {                    /* bw: exp dec */
      p->prvband = *p->kband;
      p->expamp = EXP(*p->kband * csound->mpidsr);
      newexp = 1;
    }
    if (*p->kris >= csound->onedsr && form != 0.0) {  /* init fnb ris */
      ovp->risphs = (uint32)(ovp->formphs / (fabs(form))
                                    / *p->kris); /* JPff fix */
      ovp->risinc = (int32)(csound->sicvt / *p->kris);
      rismps = MAXLEN / ovp->risinc;
    }
    else {
      ovp->risphs = MAXLEN;
      rismps = 0;
    }

    /* p->spdphs (soundfile ftable index) must be added to
       ovp->formphs (sound ftable reading rate)
       AFTER ovp-risphs is calculated */
    ovp->formphs = (ovp->formphs + p->spdphs) & PHMASK;

    if (newexp || rismps != p->prvsmps) {            /* if new params */
      if ((p->prvsmps = rismps))                     /*   redo preamp */
        p->preamp = csound->intpow(p->expamp, -rismps);
      else p->preamp = FL(1.0);
    }
    ovp->curamp = octamp * p->preamp;                /* set startamp  */
    ovp->expamp = p->expamp;
    if ((ovp->dectim = (int32)(*p->kdec * CS_ESR)) > 0) /*      fnb dec  */
      ovp->decinc = (int32)(csound->sicvt / *p->kdec);
    ovp->decphs = PHMASK;
    return(1);
}

/* JMC test additional UG */
#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "fog",  S(FOGS), TR, 5, "a","xxxakkkkkiiiiooo",(SUBR)fogset,NULL,(SUBR)fog}
};

int ugensa_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

