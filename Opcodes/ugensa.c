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

#include "csdl.h"                 /*                              UGENSA.C  */
#include "ugensa.h"
#include "ugens7.h"
#include <math.h>

/* FOG generator */

static int newpulse(FOGS *, OVERLAP *, MYFLT *, MYFLT *, MYFLT *);

int fogset(FOGS *p)
{
    /* legato test, not sure if the last bit (auxch) is correct? */
    int skip = (*p->iskip != FL(0.0) && p->auxch.auxp != 0);
    if ((p->ftp1 = ftfind(p->h.insdshead->csound, p->ifna)) != NULL &&
        (p->ftp2 = ftfind(p->h.insdshead->csound, p->ifnb)) != NULL) {
      OVERLAP *ovp, *nxtovp;
      long   olaps;
      p->fogcvt = FMAXLEN/(p->ftp1)->flen; /*JMC for FOG*/
      p->durtogo = (long)(*p->itotdur * esr);
      if (!skip) { /* legato: skip all memory management */
        p->spdphs = 0L; /*JMC for FOG*/
        if (*p->iphs == FL(0.0))                  /* if fundphs zero,  */
          p->fundphs = MAXLEN;                    /*   trigger new FOF */
        else p->fundphs = (long)(*p->iphs * FMAXLEN) & PHMASK;
        if ((olaps = (long)*p->iolaps) <= 0) {
          return initerror(Str(X_891,"illegal value for iolaps"));
        }
        if (*p->iphs>=FL(0.0))
          auxalloc((long)olaps * sizeof(OVERLAP), &p->auxch);
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
      p->xincod    = (p->XINCODE & 0xF) ? 1 : 0;
      p->ampcod    = (XINARG1) ? 1 : 0;
      p->fundcod   = (XINARG2) ? 1 : 0;
      p->formcod   = (XINARG3) ? 1 : 0;
/* p->speedcod  = (p->XINCODE & 0x8) ? 1 : 0; */ /*out for phs version of fog*/
      p->fmtmod    = (*p->itmode == 0.0) ? 0 : 1;
    }
    else return NOTOK;
    return OK;
}

int fog(FOGS *p)
{
    OVERLAP *ovp;
    FUNC        *ftp1,  *ftp2;
    MYFLT       *ar, *amp, *fund, *ptch, *speed;
    MYFLT  v1, fract ,*ftab, fogcvt = p->fogcvt; /*JMC added for FOG*/
    long   nsmps = ksmps, fund_inc, form_inc;
    /* long speed_inc; */ /*JMC added last--out for phs version*/

    ar = p->ar;
    amp = p->xamp;
    fund = p->xdens;
    ptch = p->xtrans;
    speed = p->xspd;
    ftp1 = p->ftp1;
    ftp2 = p->ftp2;
    fund_inc = (long)(*fund * sicvt);
    form_inc = (long)(*ptch * fogcvt);  /*form_inc = *form * sicvt;*/
/*      speed_inc = *speed * fogcvt; */   /*JMC for FOG--out for phs version*/
    do {
      if (p->fundphs & MAXLEN) {                     /* if phs has wrapped */
        p->fundphs &= PHMASK;
        if ((ovp = p->basovrlap.nxtfree) == NULL) {
          return perferror(Str(X_267,"FOF needs more overlaps"));
        }
        if (newpulse(p, ovp, amp, fund, ptch)) {         /* init new fof */
          ovp->nxtact = p->basovrlap.nxtact;           /* & link into  */
          p->basovrlap.nxtact = ovp;                   /*   actlist    */
          p->basovrlap.nxtfree = ovp->nxtfree;
        }
      }
      *ar = FL(0.0);
      ovp = &p->basovrlap;
      while (ovp->nxtact != NULL) {         /* perform cur actlist:  */
        MYFLT result;
        OVERLAP *prvact = ovp;
        ovp = ovp->nxtact;                     /*  formant waveform  */
        fract = PFRAC1(ovp->formphs);                   /*JMC Fog*/
        ftab = ftp1->ftable + (ovp->formphs >> ftp1->lobits);/*JMC Fog*/
/*              printf("\n ovp->formphs = %ld, ", ovp->formphs); */ /* TEMP JMC*/
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
        *ar += (result * ovp->curamp);        /*  add wavfrm to out */
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
      p->spdphs = (long)(*speed++ * FMAXLEN); /*for phs version of FOG*/
      p->spdphs &= PHMASK; /*JMC for FOG*/
      if (p->xincod) {
        if (p->ampcod)    amp++;
        if (p->fundcod)   fund_inc = (long)(*++fund * sicvt);
        if (p->formcod)   form_inc = (long)(*++ptch * fogcvt);
/*form_inc = *++form * sicvt;*/
/*      if (p->speedcod)  speed_inc = *++speed * fogcvt; */  /*JMC for FOG*/
      }
      p->durtogo--;
      ar++;
    } while (--nsmps);
    return OK;
}

static int newpulse(FOGS *p, OVERLAP *ovp, MYFLT  *amp,
                    MYFLT *fund, MYFLT *ptch)
{
    MYFLT       octamp = *amp, oct;
    MYFLT       form = *ptch /sicvt, fogcvt = p->fogcvt;  /*added JMC for Fog*/
    long   rismps, newexp = 0;
    if ((ovp->timrem = (long)(*p->kdur * esr)) > p->durtogo &&
        (*p->iskip==FL(0.0)))  /* ringtime    */
      return(0);
    if ((oct = *p->koct) > 0.0) {                   /* octaviation */
      long ioct = (long)oct, bitpat = ~(-1L << ioct);
      if (bitpat & ++p->fofcount)
        return(0);
      if ((bitpat += 1) & p->fofcount)
        octamp *= (FL(1.0) + ioct - oct);
    }
    if (*fund == 0.0)                               /* formant phs */
      ovp->formphs = 0;
/* else ovp->formphs = (long)((p->fundphs * form / *fund) + p->spdphs) & PHMASK; */
    else ovp->formphs = (long)(p->fundphs * form / *fund) & PHMASK;
    ovp->forminc = (long)(*ptch * fogcvt);/*JMC for FOG*/
    /*ovp->forminc = *form * sicvt;*/
    if (*p->kband != p->prvband) {                    /* bw: exp dec */
      p->prvband = *p->kband;
      p->expamp = (MYFLT)exp(*p->kband * mpidsr);
      newexp = 1;
    }
    if (*p->kris >= onedsr && form != 0.0) {                /* init fnb ris */
      ovp->risphs = (unsigned long)(ovp->formphs / (fabs(form))
                                    / *p->kris); /* JPff fix */
      ovp->risinc = (long)(sicvt / *p->kris);
      rismps = MAXLEN / ovp->risinc;
    }
    else {
      ovp->risphs = MAXLEN;
      rismps = 0;
    }

    /* p->spdphs (soundfile ftable index) must be added to
       ovp->formphs (sound ftable reading rate) AFTER ovp-risphs is calculated */
    ovp->formphs = (ovp->formphs + p->spdphs) & PHMASK;

    if (newexp || rismps != p->prvsmps) {            /* if new params */
      if ((p->prvsmps = rismps))                     /*   redo preamp */
        p->preamp = intpow(p->expamp, -rismps);
      else p->preamp = FL(1.0);
    }
    ovp->curamp = octamp * p->preamp;                /* set startamp  */
    ovp->expamp = p->expamp;
    if ((ovp->dectim = (long)(*p->kdec * esr)) > 0)          /*      fnb dec  */
      ovp->decinc = (long)(sicvt / *p->kdec);
    ovp->decphs = PHMASK;
    return(1);
}

/* JMC test additional UG */
#define S       sizeof

static OENTRY localops[] = {
{ "fog",  S(FOGS), 5, "a","xxxakkkkkiiiiooo",(SUBR)fogset,NULL,(SUBR)fog},
};

LINKAGE
