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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/**************************************************************/
/***********tableseg, tablexseg, voscili, vpvoc************/
/*** By Richard Karpen - July-October 1992************/
/************************************************************/

#include "cs.h"
#include <math.h>
#include "dsputil.h"
#include "fft.h"
#include "pvoc.h"
#include "vpvoc.h"
#include "soundio.h"
#include "oload.h"

static  TABLESEG        *tbladr;

int tblesegset(TABLESEG *p)
{
    TSEG        *segp;
    int nsegs;
    MYFLT       **argp, dur;
    FUNC *nxtfunc, *curfunc;
    long        flength;
    int i;

    tbladr=p;
    nsegs = (p->INCOUNT >> 1);  /* count segs & alloc if nec */

    if ((segp = (TSEG *) p->auxch.auxp) == NULL) {
        auxalloc((long)(nsegs+1)*sizeof(TSEG), &p->auxch);
        p->cursegp = segp = (TSEG *) p->auxch.auxp;
        (segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if ((nxtfunc = ftfind(*argp++)) == NULL)
        return NOTOK;
    flength = nxtfunc->flen;
    p->outfunc = (FUNC *) mcalloc((long)sizeof(FUNC) + flength*sizeof(MYFLT));
    p->outfunc->flen = nxtfunc->flen;
    p->outfunc->lenmask = nxtfunc->lenmask;
    p->outfunc->lobits = nxtfunc->lobits;
    p->outfunc->lomask = nxtfunc->lomask;
    p->outfunc->lodiv = nxtfunc->lodiv;
    for (i=0; i<= flength; i++)
        *(p->outfunc->ftable + i) = FL(0.0);
    if (**argp <= 0.0)  return OK;         /* if idur1 <= 0, skip init  */
    p->cursegp = segp;                      /* else proceed from 1st seg */
    segp--;
    do {
        segp++;                 /* init each seg ..  */
        curfunc = nxtfunc;
        dur = **argp++;
        if ((nxtfunc = ftfind(*argp++)) == NULL) return OK;
        if (dur > FL(0.0)) {
                segp->d = dur * ekr;
                segp->function =  curfunc;
                segp->nxtfunction = nxtfunc;
                segp->cnt = (long) (segp->d + .5);
        }
        else break;             /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;
    segp->d = FL(0.0);
    segp->cnt = MAXPOS;         /* set last cntr to infin */
    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    return OK;
}

int ktableseg(TABLESEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, durovercnt=FL(0.0);
    int         i;
    long        flength, upcnt;

    /* RWD fix */
    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_1270,"tableseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0)
      durovercnt = segp->d/upcnt;
    while (--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = segp->function->flen;
    for (i=0; i<flength; i++) {
      curval = *(curtab + i);
      nxtval = *(nxttab + i);
      if (durovercnt > 0.0)
        *(p->outfunc->ftable + i) = (curval + ((nxtval - curval) / durovercnt));
      else
        *(p->outfunc->ftable + i) = curval;
    }
    return OK;
}

int ktablexseg(TABLESEG *p)
{
    TSEG        *segp;
    MYFLT       *curtab, *nxttab,curval, nxtval, cntoverdur=FL(0.0);
    int         i;
    long        flength, upcnt;

    /* RWD fix */
    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_1271,"tablexseg: not initialised"));
    }
    segp = p->cursegp;
    curtab = segp->function->ftable;
    nxttab = segp->nxtfunction->ftable;
    upcnt = (long)segp->d-segp->cnt;
    if (upcnt > 0) cntoverdur = upcnt/ segp->d;
    while(--segp->cnt < 0)
      p->cursegp = ++segp;
    flength = segp->function->flen;
    for (i=0; i<flength; i++) {
      curval = *(curtab + i);
      nxtval = *(nxttab + i);
      *(p->outfunc->ftable + i) =
        (curval + ((nxtval - curval) * (cntoverdur*cntoverdur)));
    }
    return OK;
}

#ifdef never
int voscset(VOSC *p)
{
    p->tableseg = tbladr;
    if (*p->iphs >= 0)
      p->lphs = ((long)(*p->iphs * FMAXLEN)) & PHMASK;
    return OK;
}

int voscili(VOSC *p)
{
    FUNC        *ftp;
    MYFLT       v1, fract, *ar, *ampp, *cpsp, *ftab;
    long        phs, lobits;
    int         nsmps = ksmps;
    TABLESEG    *q = p->tableseg;

    /* RWD fix */
    if (q->auxch.auxp==NULL) {
      return perferror(Str(X_1373,"voscili: not initialised"));
    }
    ftp = q->outfunc;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;

    do {
      long inc;
      inc = (long)(*cpsp++ * sicvt);
      fract = PFRAC(phs);
      ftab =ftp->ftable + (phs >> lobits);
      v1 = *ftab++;
      *ar++ = (v1 + (*ftab - v1) * fract) * *ampp;
      phs += inc;
      phs &= PHMASK;
    }
    while (--nsmps);
    p->lphs = phs;
    return OK;
}
#endif

/************************************************************/
/*****************VPVOC**************************************/
/************************************************************/

#define WLN   1         /* time window is WLN*2*ksmps long */

/*  #define OPWLEN      size    */
#define OPWLEN (2*WLN*ksmps)    /* manifest used for final time wdw */

/* static       int     pdebug = 0; */
/* static       int     dchan = 6; */   /* which channel to examine on debug */

int vpvset(VPVOC *p)
{
    int      i;
    char     pvfilnam[64];
    MEMFIL   *mfp, *ldmemfile(char*);
    PVSTRUCT *pvh;
    int     frInc, chans, size; /* THESE SHOULD BE SAVED IN PVOC STRUCT */
                                /* If optional table given, fake it up -- JPff */
    if (*p->isegtab==FL(0.0)) p->tableseg = tbladr;
    else {
      auxalloc(sizeof(TABLESEG), &p->auxtab);
      p->tableseg = (TABLESEG*) p->auxtab.auxp;
      if ((p->tableseg->outfunc = ftfind(p->isegtab)) == NULL) {
        sprintf(errmsg,
                Str(X_1517,"vpvoc: Could not find ifnmagctrl table %f\n"),
                *p->isegtab);
        return initerror(errmsg);
      }
    }

    if (p->auxch.auxp == NULL) {              /* if no buffers yet, alloc now */
        MYFLT *fltp;
        auxalloc((long)(PVDATASIZE + PVFFTSIZE*3 + PVWINLEN) * sizeof(MYFLT),
                 &p->auxch);
        fltp = (MYFLT *) p->auxch.auxp;
        p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
        p->fftBuf = fltp;      fltp += PVFFTSIZE;
        p->dsBuf = fltp;       fltp += PVFFTSIZE;
        p->outBuf = fltp;      fltp += PVFFTSIZE;
        p->window = fltp;
    }
    if (*p->ifilno == SSTRCOD) {                       /* if strg name given */
        strcpy(pvfilnam, unquote(p->STRARG));          /*   use that         */
    }
    else if ((long)*p->ifilno <= strsmax && strsets != NULL &&
             strsets[(long)*p->ifilno]) {
      strcpy(pvfilnam, strsets[(long)*p->ifilno]);
    }
    else sprintf(pvfilnam,"pvoc.%d", (int)*p->ifilno); /* else pvoc.filnum   */
    if ((mfp = p->mfp) == NULL
      || strcmp(mfp->filename, pvfilnam) != 0) /* if file not already readin */
        if ( (mfp = ldmemfile(pvfilnam)) == NULL) {
            sprintf(errmsg,Str(X_411,"PVOC cannot load %s"), pvfilnam);
            goto pverr;
        }
    pvh = (PVSTRUCT *)mfp->beginp;
    if (pvh->magic != PVMAGIC) {
        sprintf(errmsg,Str(X_60,"%s not a PVOC file (magic %ld)"),
                pvfilnam, pvh->magic );
        goto pverr;
    }
    p->frSiz = pvh->frameSize;
    frInc    = pvh->frameIncr;
    chans    = pvh->channels;
    if ((p->asr = pvh->samplingRate) != esr &&
        (O.msglevel & WARNMSG)) { /* & chk the data */
      printf(Str(X_63,"WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
             pvfilnam, p->asr, esr);
    }
    if (pvh->dataFormat != PVMYFLT) {
      sprintf(errmsg,Str(X_1359,"unsupported PVOC data format %ld in %s"),
              pvh->dataFormat, pvfilnam);
      goto pverr;
    }
    if (p->frSiz > PVFRAMSIZE) {
      sprintf(errmsg,Str(X_413,"PVOC frame %d bigger than %ld in %s"),
              p->frSiz, PVFRAMSIZE, pvfilnam);
      goto pverr;
    }
    if (p->frSiz < PVFRAMSIZE/8) {
      sprintf(errmsg,Str(X_414,"PVOC frame %ld seems too small in %s"),
              p->frSiz, pvfilnam);
      goto pverr;
    }
    if (chans != 1) {
      sprintf(errmsg,Str(X_32,"%d chans (not 1) in PVOC file %s"),
              chans, pvfilnam);
      goto pverr;
    }
    /* Check that pv->frSiz is a power of two too ? */
    p->frPtr = (MYFLT *) ((char *)pvh+pvh->headBsize);
    p->baseFr = 0;  /* point to first data frame */
    p->maxFr = -1 + ( pvh->dataBsize / (chans * (p->frSiz+2) * sizeof(MYFLT)));
    /* highest possible frame index */
    p->frPktim = ((MYFLT)ksmps)/((MYFLT)frInc);
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = esr/((MYFLT)frInc);
    /* factor by which to mulitply 'real' time index to get frame index */
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
/*  p->scale = 4.*((MYFLT)ksmps)/((MYFLT)pvfrsiz(p)*(MYFLT)pvfrsiz(p)); */
/*    p->scale = 2.*((MYFLT)ksmps)/((MYFLT)OPWLEN*(MYFLT)pvfrsiz(p));   */
    p->scale = e0dbfs * FL(2.0)*((MYFLT)ksmps)/((MYFLT)OPWLEN*(MYFLT)pvfrsiz(p));
    /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
    /* 1/frSiz is the required scale down before (i)FFT */
    p->prFlg = 1;    /* true */
    p->opBpos = 0;
    p->lastPex = FL(1.0);           /* needs to know last pitchexp to update phase */
    /* Set up time window */
    for (i=0; i < pvdasiz(p); ++i) {  /* or maybe pvdasiz(p) */
     /* p->window[i] = (0.54-0.46*cos(2.0*pi*(MYFLT)i/(MYFLT)(pvfrsiz(p)))); */
        p->lastPhase[i] = FL(0.0);
    }
    if ( (OPWLEN/2 + 1)>PVWINLEN ) {
        sprintf(errmsg, Str(X_960,"ksmps of %d needs wdw of %d, max is %d for pv %s\n"),
                ksmps, (OPWLEN/2 + 1), PVWINLEN, pvfilnam);
        goto pverr;
    }
    for (i=0; i < OPWLEN/2+1; ++i)    /* time window is OPWLEN long */
        p->window[i] = (FL(0.54)-FL(0.46)*(MYFLT)cos(TWOPI*(MYFLT)i/(MYFLT)OPWLEN));
    /* NB : HAMMING */
    for (i=0; i< pvfrsiz(p); ++i)
        p->outBuf[i] = FL(0.0);
    MakeSinc( /* p->sncTab */ );        /* sinctab is same for all instances */
    p->plut = (MYFLT *)AssignBasis(NULL, pvfrsiz(p));    /* SET UP NONET FFT */

    return OK;

 pverr:
    return initerror(errmsg);
}


int vpvoc(VPVOC *p)
{
    int    n;
/*     MYFLT  *samp; */
    MYFLT  *ar = p->rslt;
    MYFLT  frIndx;
    MYFLT  *buf = p->fftBuf;
    MYFLT  *buf2 = p->dsBuf;
    int  asize = pvdasiz(p); /* fix */
    MYFLT  *plut = p->plut;
    int    size = pvfrsiz(p);
    int    buf2Size, outlen;
    int    circBufSize = PVFFTSIZE;
    int    specwp = (int)*p->ispecwp;   /* spectral warping flag */
    MYFLT  pex;
    TABLESEG *q = p->tableseg;
    long         i,j ;

    /* RWD fix */
    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_1374,"vpvoc: not initialised"));
    }

/*     if (pdebug) { printf("<%7.4f>",*p->ktimpnt); fflush(stdout); } */
    pex = *p->kfmod;
    outlen = (int)(((MYFLT)size)/pex);
    /* use outlen to check window/krate/transpose combinations */
    if (outlen>PVFFTSIZE) { /* Maximum transposition down is one octave */
                            /* ..so we won't run into buf2Size problems */
      return perferror(Str(X_418,"PVOC transpose too low"));
    }
    if (outlen<2*ksmps) {   /* minimum post-squeeze windowlength */
      return perferror(Str(X_417,"PVOC transpose too high"));
    }
    buf2Size = OPWLEN;     /* always window to same length after DS */
    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return perferror(Str(X_416,"PVOC timpnt < 0"));
    }
    if (frIndx > (MYFLT)p->maxFr) { /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        if (O.msglevel & WARNMSG)
          printf(Str(X_415,"WARNING: PVOC ktimpnt truncated to last frame\n"));
      }
    }
    FetchIn(p->frPtr,buf,size,frIndx);
/*    if (frIndx >= p->maxFr)
        printf("Fetched %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f\n",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);  */

/**** Apply "spectral envelope" to magnitudes ********/
    for (i=0, j=0; i<=size; i+=2, j++)
      buf[i] *= *(q->outfunc->ftable + j);
/***************************************************/

    FrqToPhase(buf, asize, pex*(MYFLT)ksmps, p->asr,
           /*a0.0*/(MYFLT)(.5 * ( (pex / p->lastPex) - 1) ));
    /* Offset the phase to align centres of stretched windows, not starts */
    RewrapPhase(buf,asize,p->lastPhase);
    /**/
    if ( specwp == 0 || (p->prFlg)++ == -(int)specwp) { /* ?screws up when prFlg used */
      /* specwp=0 => normal; specwp = -n => just nth frame */
      if (specwp<0) printf(Str(X_412,"PVOC debug : one frame gets through \n"));
      if (specwp>0)
        PreWarpSpec(buf, asize, pex);
      Polar2Rect(buf,size);
      /*    if (frIndx >= p->maxFr)
            printf("Rected %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]); */
      buf[1] = FL(0.0); buf[size+1] = FL(0.0);    /* kill spurious imag at dc & fs/2 */
      FFT2torl((complex *)buf,size,1,/*a pex*/ p->scale, (complex *)plut);
      /* CALL TO NONET FFT */
      PackReals(buf, size);
      /*    if (frIndx >= p->maxFr)
            printf("IFFTed %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f %8.1f %6.1f\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]); */
      /*a    ApplyHalfWin(buf, p->window, size);      */
      if (pex != 1.0)
        UDSample(buf,(FL(0.5)*((MYFLT)size - pex*(MYFLT)buf2Size)),
                 buf2, size, buf2Size, pex);
      else
        CopySamps(buf+(int)(FL(0.5)*((MYFLT)size - pex*(MYFLT)buf2Size)),
                  buf2,buf2Size);
      if (specwp>=0) ApplyHalfWin(buf2, p->window, buf2Size);
    }
    else
      for (n = 0; n<buf2Size; ++n)
        buf2[n] = FL(0.0);            /*      */
    addToCircBuf(buf2, p->outBuf, p->opBpos, ksmps, circBufSize);
    writeClrFromCircBuf(p->outBuf, ar, p->opBpos, ksmps, circBufSize);
    p->opBpos += ksmps;
    if (p->opBpos > circBufSize)     p->opBpos -= circBufSize;
    addToCircBuf(buf2+ksmps,p->outBuf,p->opBpos,buf2Size-ksmps,circBufSize);
    p->lastPex = pex;        /* needs to know last pitchexp to update phase */
    return OK;
}

