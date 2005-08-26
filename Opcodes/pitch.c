/*
    pitch.c:

    Copyright (C) 1999 John ffitch, Istvan Varga, Peter Neubäcker,
                       rasmus ekman, Phil Burk

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

#include "csdl.h"       /*                              PITCH.C         */
#include <math.h>
#include <limits.h>
#include "cwindow.h"
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"

extern void DOWNset(DOWNDAT *, long);
extern void SPECset(SPECDAT *, long);

#define STARTING 1
#define PLAYING  2
#define LOGTWO (0.693147)

static MYFLT bicoefs[] =
    { -FL(0.2674054), FL(0.7491305), FL(0.7160484), FL(0.0496285), FL(0.7160484),
       FL(0.0505247), FL(0.3514850), FL(0.5257536), FL(0.3505025), FL(0.5257536),
       FL(0.3661840), FL(0.0837990), FL(0.3867783), FL(0.6764264), FL(0.3867783)
    };

int pitchset(CSOUND *csound, PITCH *p)     /* pitch - uses specta technology */
{
    MYFLT       b;              /* For RMS */
    int     n, nocts, nfreqs, ncoefs;
    MYFLT   Q, *fltp;
    OCTDAT  *octp;
    DOWNDAT *dwnp = &p->downsig;
    SPECDAT *specp = &p->wsig;
    long    npts, nptls, nn, lobin;
    int     *dstp, ptlmax;
    MYFLT   fnfreqs, rolloff, *oct0p, *flop, *fhip, *fundp, *fendp, *fp;
    MYFLT   weight, weightsum, dbthresh, ampthresh;

                                /* RMS of input signal */
    b = FL(2.0) - (MYFLT)cos((double)(10.0 * csound->tpidsr));
    p->c2 = b - (MYFLT)sqrt((double)(b * b - 1.0));
    p->c1 = FL(1.0) - p->c2;
    if (!*p->istor) p->prvq = FL(0.0);
                                /* End of rms */
                                /* Initialise spectrum */
    /* for mac roundoff */
    p->timcount = (int)(csound->ekr * *p->iprd + FL(0.001));
    nocts = (int)*p->iocts; if (nocts<=0) nocts = 6;
    nfreqs = (int)*p->ifrqs; if (nfreqs<=0) nfreqs = 12;
    ncoefs = nocts * nfreqs;
    Q = *p->iq; if (Q<=FL(0.0)) Q = FL(15.0);

    if (p->timcount <= 0) return csound->InitError(csound, Str("illegal iprd"));
    if (nocts > MAXOCTS)  return csound->InitError(csound, Str("illegal iocts"));
    if (nfreqs > MAXFRQS) return csound->InitError(csound, Str("illegal ifrqs"));

    if (nocts != dwnp->nocts
        || nfreqs != p->nfreqs  /* if anything has changed */
        || Q != p->curq ) {                /*     make new tables */
      double      basfrq, curfrq, frqmlt, Qfactor;
      double      theta, a, windamp, onedws, pidws;
      MYFLT       *sinp, *cosp;
      int         k, sumk, windsiz, halfsiz, *wsizp, *woffp;
      long        auxsiz, bufsiz;
      long        majr, minr, totsamps;
      double      hicps,locps,oct;  /*   must alloc anew */

      p->nfreqs = nfreqs;
      p->curq = Q;
      p->ncoefs = ncoefs;
      dwnp->srate = csound->esr;
      hicps = dwnp->srate * 0.375;            /* top freq is 3/4 pi/2 ...   */
      oct = log(hicps / ONEPT) / LOGTWO;      /* octcps()  (see aops.c)     */
      dwnp->looct = (MYFLT)(oct - nocts);     /* true oct val of lowest frq */
      locps = hicps / (1L << nocts);
      basfrq = hicps/2.0;                          /* oct below retuned top */
      frqmlt = pow((double)2.0,(double)1.0/nfreqs);  /* nfreq interval mult */
      Qfactor = Q * dwnp->srate;
      curfrq = basfrq;
      for (sumk=0,wsizp=p->winlen,woffp=p->offset,n=nfreqs; n--; ) {
        *wsizp++ = k = (int)(Qfactor/curfrq) | 01;  /* calc odd wind sizes */
        *woffp++ = (*(p->winlen) - k) / 2;          /* & symmetric offsets */
        sumk += k;                                  /*    and find total   */
        curfrq *= frqmlt;
      }
      windsiz = *(p->winlen);
      auxsiz = (windsiz + 2*sumk) * sizeof(MYFLT);   /* calc lcl space rqd */

      csound->AuxAlloc(csound, (long)auxsiz, &p->auxch1); /* & alloc auxspace  */

      fltp = (MYFLT *) p->auxch1.auxp;
      p->linbufp = fltp;      fltp += windsiz; /* linbuf must take nsamps */
      p->sinp = sinp = fltp;  fltp += sumk;
      p->cosp = cosp = fltp;                         /* cos gets rem sumk  */
      wsizp = p->winlen;
      curfrq = basfrq * TWOPI / dwnp->srate;
      for (n = nfreqs; n--; ) {                      /* now fill tables */
        windsiz = *wsizp++;                          /*  (odd win size) */
        halfsiz = windsiz >> 1;
        onedws = 1.0 / (windsiz-1);
        pidws = PI / (windsiz-1);
        for (k = -halfsiz; k<=halfsiz; k++) {        /*   with sines    */
          a = cos(k * pidws);
          windamp = 0.08 + 0.92 * a * a;             /*   times hamming */
          windamp *= onedws;                         /*   scaled        */
          theta = k * curfrq;
          *sinp++ = (MYFLT)(windamp * sin(theta));
          *cosp++ = (MYFLT)(windamp * cos(theta));
        }
        curfrq *= frqmlt;                        /*   step by log freq  */
      }
      dwnp->hifrq = (MYFLT)hicps;
      dwnp->lofrq = (MYFLT)locps;
      dwnp->nsamps = windsiz = *(p->winlen);
      dwnp->nocts = nocts;
      minr = windsiz >> 1;                  /* sep odd windsiz into maj, min */
      majr = windsiz - minr;                /*      & calc totsamps reqd     */
      totsamps = (majr*nocts) + (minr<<nocts) - minr;
      DOWNset(dwnp, totsamps);              /* auxalloc in DOWNDAT struct */
      fltp = (MYFLT *) dwnp->auxch.auxp;    /*  & distrib to octdata */
      for (n=nocts,octp=dwnp->octdata+(nocts-1); n--; octp--) {
        bufsiz = majr + minr;
        octp->begp = fltp;  fltp += bufsiz; /*        (lo oct first) */
        octp->endp = fltp;  minr *= 2;
      }
      SPECset(specp, (long)ncoefs);         /* prep the spec dspace */
      specp->downsrcp = dwnp;               /*  & record its source */
    }
    for (octp=dwnp->octdata; nocts--; octp++) { /* reset all oct params, &  */
      octp->curp = octp->begp;
      for (fltp=octp->feedback,n=6; n--; )
        *fltp++ = FL(0.0);
      octp->scount = 0;
    }
    specp->nfreqs = p->nfreqs;               /* save the spec descriptors */
    specp->dbout = 0;
    specp->ktimstamp = 0;                    /* init specdata to not new  */
    specp->ktimprd = p->timcount;
    p->scountdown = p->timcount;             /* prime the spect countdown */
                                             /* Start specptrk */
    if ((npts = specp->npts) != p->winpts) {        /* if size has changed */
      SPECset(&p->wfund, (long)npts);               /*   realloc for wfund */
      p->wfund.downsrcp = specp->downsrcp;
      p->fundp = (MYFLT *) p->wfund.auxch.auxp;
      p->winpts = npts;
    }
    if (*p->inptls<=FL(0.0)) nptls = 4;
    else nptls = (long)*p->inptls;
    if (nptls > MAXPTL) {
      return csound->InitError(csound, Str("illegal no of partials"));
    }
    if (*p->irolloff<=FL(0.0)) *p->irolloff = FL(0.6);
    p->nptls = nptls;        /* number, whether all or odd */
      ptlmax = nptls;
    dstp = p->pdist;
    fnfreqs = (MYFLT)specp->nfreqs;
    for (nn = 1; nn <= ptlmax; nn++)
      *dstp++ = (int) ((log((double) nn) / LOGTWO) * fnfreqs + 0.5);
    if ((rolloff = *p->irolloff) == 0. || rolloff == 1. || nptls == 1) {
      p->rolloff = 0;
      weightsum = (MYFLT)nptls;
    } else {
      MYFLT *fltp = p->pmult;
      MYFLT octdrop = (FL(1.0) - rolloff) / fnfreqs;
      weightsum = FL(0.0);
      for (dstp = p->pdist, nn = nptls; nn--; ) {
        weight = FL(1.0) - octdrop * *dstp++;       /* rolloff * octdistance */
        weightsum += weight;
        *fltp++ = weight;
      }
      if (*--fltp < FL(0.0)) {
        return csound->InitError(csound, Str("per oct rolloff too steep"));
      }
      p->rolloff = 1;
    }
    lobin = (long)(specp->downsrcp->looct * fnfreqs);
    oct0p = p->fundp - lobin;                   /* virtual loc of oct 0 */

    flop = oct0p + (int)(*p->ilo * fnfreqs);
    fhip = oct0p + (int)(*p->ihi * fnfreqs);
    fundp = p->fundp;
    fendp = fundp + specp->npts;
    if (flop < fundp) flop = fundp;
    if (fhip > fendp) fhip = fendp;
    if (flop >= fhip) {         /* chk hi-lo range valid */
      return csound->InitError(csound, Str("illegal lo-hi values"));
    }
    for (fp = fundp; fp < flop; )
      *fp++ = FL(0.0);   /* clear unused lo and hi range */
    for (fp = fhip; fp < fendp; )
      *fp++ = FL(0.0);

    dbthresh = *p->idbthresh;                     /* thresholds: */
    ampthresh = (MYFLT)exp((double)dbthresh * LOG10D20);
    p->threshon = ampthresh;              /* mag */
    p->threshoff = ampthresh * FL(0.5);
    p->threshon *= weightsum;
    p->threshoff *= weightsum;
    p->oct0p = oct0p;                 /* virtual loc of oct 0 */
    p->confact = *p->iconf;
    p->flop = flop;
    p->fhip = fhip;
    p->playing = 0;
    p->kvalsav = (*p->istrt>=FL(0.0) ? *p->istrt : (*p->ilo+*p->ihi)*FL(0.5));
    p->kval = p->kinc = FL(0.0);
    p->kavl = p->kanc = FL(0.0);
    p->jmpcount =  0;
    return OK;
}

int pitch(CSOUND *csound, PITCH *p)
{
    MYFLT       *asig;
    MYFLT       q;
    MYFLT       c1 = p->c1, c2 = p->c2;

    MYFLT   a, b, *dftp, *sigp = p->asig, SIG, yt1, yt2;
    int     nocts, nsmps = csound->ksmps, winlen;
    DOWNDAT *downp = &p->downsig;
    OCTDAT  *octp;
    SPECDAT *specp;
    double  c;
    MYFLT kvar;
                                /* RMS */
    q = p->prvq;
    asig = p->asig;
    do {
      MYFLT as = *asig++;
      q = c1 * as * as + c2 * q;
      SIG = *sigp++;                        /* for each source sample: */
      octp = downp->octdata;                /*   align onto top octave */
      nocts = downp->nocts;
      do {                                  /*   then for each oct:    */
        MYFLT *coefp,*ytp,*curp;
        int   nfilt;
        curp = octp->curp;
        *curp++ = SIG;                      /*  write samp to cur buf  */
        if (curp >= octp->endp)
          curp = octp->begp;                /*    & modulo the pointer */
        octp->curp = curp;
        if (!(--nocts))  break;             /*  if lastoct, break      */
        coefp = bicoefs;  ytp = octp->feedback;
        for (nfilt = 3; nfilt--; ) {        /*  apply triple biquad:   */
          yt2 = *ytp++; yt1 = *ytp--;             /* get prev feedback */
          SIG -= (*coefp++ * yt1);                /* apply recurs filt */
          SIG -= (*coefp++ * yt2);
          *ytp++ = yt1; *ytp++ = SIG;             /* stor nxt feedback */
          SIG *= *coefp++;
          SIG += (*coefp++ * yt1);                /* apply forwrd filt */
          SIG += (*coefp++ * yt2);
        }
      } while (!(++octp->scount & 01) && octp++); /* send alt samps to nxtoct */
    } while (--nsmps);
    p->prvq = q;
    kvar = (MYFLT) sqrt((double)q); /* End of spectrun part */

    specp = &p->wsig;
    if ((--p->scountdown)) goto nxt;      /* if not yet time for new spec  */
    p->scountdown = p->timcount;          /* else reset counter & proceed: */
    downp = &p->downsig;
    nocts = downp->nocts;
    octp = downp->octdata + nocts;
    dftp = (MYFLT *) specp->auxch.auxp;
    winlen = *(p->winlen);
    while (nocts--) {
      MYFLT  *bufp, *sinp, *cosp;
      int    len, *lenp, *offp, nfreqs;
      MYFLT    *begp, *curp, *endp, *linbufp;
      int      len2;
      octp--;                              /* for each oct (low to high)   */
      begp = octp->begp;
      curp = octp->curp;
      endp = octp->endp;
      if ((len = endp - curp) >= winlen)      /*   if no wrap               */
        linbufp = curp;                       /*     use samples in circbuf */
      else {
        len2 = winlen - len;
        linbufp = bufp = p->linbufp;          /*   else cp crcbuf to linbuf */
        while (len--)
          *bufp++ = *curp++;
        curp = begp;
        while (len2--)
          *bufp++ = *curp++;
      }
      cosp = p->cosp;                         /*   get start windowed sines */
      sinp = p->sinp;
      lenp = p->winlen;
      offp = p->offset;
      for (nfreqs=p->nfreqs; nfreqs--; ) {    /*   now for ea. frq this oct */
        a = FL(0.0);
        b = FL(0.0);
        bufp = linbufp + *offp++;
        for (len = *lenp++; len--; bufp++) {  /* apply windowed sine seg */
          a += *bufp * *cosp++;
          b += *bufp * *sinp++;
        }
        c = a*a + b*b;                        /* get magnitude    */
        c = sqrt(c);
        *dftp++ = (MYFLT)c;                   /* store in out spectrum   */
      }
    }
    specp->ktimstamp = csound->kcounter;      /* time-stamp the output   */

 nxt:
                                /* specptrk */
    {
      MYFLT *inp = (MYFLT *) specp->auxch.auxp;
      MYFLT *endp = inp + specp->npts;
      MYFLT *inp2, sum, *fp;
      int   nn, *pdist, confirms;
      MYFLT kval, fmax, *fmaxp, absdiff, realbin;
      MYFLT *flop, *fhip, *ilop, *ihip, a, b, c, denom, delta;
      long  lobin, hibin;

      if (inp==NULL) {             /* RWD fix */
        return csound->PerfError(csound, Str("pitch: not initialised"));
      }
      kval = p->playing == PLAYING ? p->kval : p->kvalsav;
      lobin = (long)((kval - kvar) * specp->nfreqs);/* set lims of frq interest */
      hibin = (long)((kval + kvar) * specp->nfreqs);
      if ((flop = p->oct0p + lobin) < p->flop)  /*       as fundp bin pntrs */
        flop = p->flop;
      if ((fhip = p->oct0p + hibin) > p->fhip)  /*       within hard limits */
        fhip = p->fhip;
      ilop = inp + (flop - p->fundp);           /* similar for input bins   */
      ihip = inp + (fhip - p->fundp);
      inp = ilop;
      fp = flop;
      if (p->rolloff) {
        MYFLT *pmult;
        do {
          sum = *inp;
          pdist = p->pdist + 1;
          pmult = p->pmult + 1;
          for (nn = p->nptls; --nn; ) {
            if ((inp2 = inp + *pdist++) >= endp)
              break;
            sum += *inp2 * *pmult++;
          }
          *fp++ = sum;
        } while (++inp < ihip);
      }
      else {
        do {
          sum = *inp;
          pdist = p->pdist + 1;
          for (nn = p->nptls; --nn; ) {
            if ((inp2 = inp + *pdist++) >= endp)
              break;
            sum += *inp2;
          }
          *fp++ = sum;
        } while (++inp < ihip);
      }
      fp = flop;                               /* now srch fbins for peak */
      for (fmaxp = fp, fmax = *fp; ++fp<fhip; )
        if (*fp > fmax) {
          fmax = *fp;
          fmaxp = fp;
        }
      if (!p->playing) {
        if (fmax > p->threshon)         /* not playing & threshon? */
          p->playing = STARTING;      /*   prepare to turn on    */
        else goto output;
      }
      else {
        if (fmax < p->threshoff) {      /* playing & threshoff ? */
          if (p->playing == PLAYING)
            p->kvalsav = p->kval;   /*   save val & turn off */
          p->kval = FL(0.0);
          p->kavl = FL(0.0);
          p->kinc = FL(0.0);
          p->kanc = FL(0.0);
          p->playing = 0;
          goto output;
        }
      }
      a = fmaxp>flop ? *(fmaxp-1) : FL(0.0);     /* calc a refined bin no */
      b = fmax;
      c = fmaxp<fhip-1 ? *(fmaxp+1) : FL(0.0);
      if (b < FL(2.0) * (a + c))
        denom = b + b - a - c;
      else denom = a + b + c;
      if (denom != FL(0.0))
        delta = FL(0.5) * (c - a) / denom;
      else delta = FL(0.0);
      realbin = (fmaxp - p->oct0p) + delta;    /* get modified bin number  */
      kval = realbin / specp->nfreqs;        /*     & cvt to true decoct */

      if (p->playing == STARTING) {            /* STARTING mode:           */
        if ((absdiff = kval - p->kvalsav) < FL(0.0))
          absdiff = -absdiff;
        confirms = (int)(absdiff * p->confact); /* get interval dependency  */
        if (p->jmpcount < confirms) {
          p->jmpcount += 1;                /* if not enough confirms,  */
          goto output;                     /*    must wait some more   */
        } else {
          p->playing = PLAYING;            /* else switch on playing   */
          p->jmpcount = 0;
          p->kval = kval;                  /*    but suppress interp   */
          p->kinc = FL(0.0);
        }
      } else {                                 /* PLAYING mode:            */
        if ((absdiff = kval - p->kval) < FL(0.0))
          absdiff = -absdiff;
        confirms = (int)(absdiff * p->confact); /* get interval dependency  */
        if (p->jmpcount < confirms) {
          p->jmpcount += 1;                /* if not enough confirms,  */
          p->kinc = FL(0.0);                 /*    must wait some more   */
        } else {
          p->jmpcount = 0;                 /* else OK to jump interval */
          p->kval = kval;
        }
      }
      fmax += delta * (c - a) / FL(4.0);  /* get modified amp */
      p->kavl = fmax;
    }
output:
    *p->koct = p->kval;                   /* output true decoct & amp */
    *p->kamp = p->kavl;
    return OK;
}

/* Multiply and accumulate opcodes */

int macset(CSOUND *csound, SUM *p)
{
    if ((((int)p->INOCOUNT)&1)==1) {
      return csound->PerfError(csound,
                               Str("Must have even number of arguments in mac\n"));
    }
    return OK;
}

int maca(CSOUND *csound, SUM *p)
{
    int nsmps=csound->ksmps, count=(int) p->INOCOUNT, j, k=0;
    MYFLT *ar = p->ar, **args = p->argums;
    do {
      MYFLT ans = FL(0.0);
      for (j=0; j<count; j +=2)
        ans += args[j][k] * args[j+1][k];
      k++;
      *ar++ = ans;
    } while (--nsmps);
    return OK;
}

int mac(CSOUND *csound, SUM *p)
{
    int nsmps=csound->ksmps, count=(int) p->INOCOUNT, j, k=0;
    MYFLT *ar = p->ar, **args = p->argums;
    do {
      MYFLT ans = FL(0.0);
      for (j=0; j<count; j +=2)
        ans += *args[j]* args[j+1][k];
      k++;
      *ar++ = ans;
    } while (--nsmps);
    return OK;
}

typedef struct {
    RTCLOCK r;
    double  counters[33];
    int     running[33];
} CPU_CLOCK;

static void initClockStruct(CSOUND *csound, void **p)
{
    *p = csound->QueryGlobalVariable(csound, "readClock::counters");
    if (*p == NULL) {
      csound->CreateGlobalVariable(csound, "readClock::counters",
                                           sizeof(CPU_CLOCK));
      *p = csound->QueryGlobalVariable(csound, "readClock::counters");
      csound->InitTimerStruct(&(((CPU_CLOCK*) (*p))->r));
    }
}

static inline CPU_CLOCK *getClockStruct(CSOUND *csound, void **p)
{
    if (*p == NULL)
      initClockStruct(csound, p);
    return (CPU_CLOCK*) (*p);
}

int clockset(CSOUND *csound, CLOCK *p)
{
    p->c = (int)*p->cnt;
    if (p->c < 0 || p->c > 31)
      p->c = 32;
    return OK;
}

int clockon(CSOUND *csound, CLOCK *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    if (!clk->running[p->c]) {
      clk->running[p->c] = 1;
      clk->counters[p->c] = csound->GetCPUTime(&(clk->r));
    }
    return OK;
}

int clockoff(CSOUND *csound, CLOCK *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    if (clk->running[p->c]) {
      clk->running[p->c] = 0;
      clk->counters[p->c] = csound->GetCPUTime(&(clk->r)) - clk->counters[p->c];
    }
    return OK;
}

int clockread(CSOUND *csound, CLKRD *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    int cnt = (int) *p->a;
    if (cnt < 0 || cnt > 32) cnt = 32;
    if (clk->running[cnt])
      return csound->InitError(csound, Str("clockread: clock still running, "
                                           "call clockoff first"));
    /* result in ms */
    *p->r = (MYFLT) (clk->counters[cnt] * 1000.0);
    return OK;
}

/* ************************************************************ */
/* Opcodes from Peter Neubäcker                                 */
/* ************************************************************ */

int adsyntset(CSOUND *csound, ADSYNT *p)
{
    FUNC    *ftp;
    int     count;
    long    *lphs;

    p->inerr = 0;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: wavetable not found!"));
    }

    count = (int)*p->icnt;
    if (count < 1)
      count = 1;
    p->count = count;

    if ((ftp = csound->FTFind(csound, p->ifreqtbl)) != NULL) {
      p->freqtp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: freqtable not found!"));
    }
    if (ftp->flen < count) {
      p->inerr = 1;
      return csound->InitError(csound, Str(
                    "adsynt: partial count is greater than freqtable size!"));
    }

    if ((ftp = csound->FTFind(csound, p->iamptbl)) != NULL) {
      p->amptp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: amptable not found!"));
    }
    if (ftp->flen < count) {
      p->inerr = 1;
      return csound->InitError(csound, Str(
                    "adsynt: partial count is greater than amptable size!"));
    }

    if (p->lphs.auxp==NULL || p->lphs.size < (long)sizeof(long)*count)
      csound->AuxAlloc(csound, sizeof(long)*count, &p->lphs);

    lphs = (long*)p->lphs.auxp;
    if (*p->iphs > 1) {
      do
        *lphs++ = ((long)((MYFLT)((double)rand()/(double)RAND_MAX)
                          * FMAXLEN)) & PHMASK;
      while (--count);
    }
    else if (*p->iphs >= 0) {
      do
        *lphs++ = ((long)(*p->iphs * FMAXLEN)) & PHMASK;
      while (--count);
    }
    return OK;
}

int adsynt(CSOUND *csound, ADSYNT *p)
{
    FUNC    *ftp, *freqtp, *amptp;
    MYFLT   *ar, *ar0, *ftbl, *freqtbl, *amptbl;
    MYFLT   amp0, amp, cps0, cps;
    long    phs, inc, lobits;
    long    *lphs;
    int     nsmps, count;

    if (p->inerr) {
      return csound->PerfError(csound, Str("adsynt: not initialised"));
    }
    ftp = p->ftp;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    freqtp = p->freqtp;
    freqtbl = freqtp->ftable;
    amptp = p->amptp;
    amptbl = amptp->ftable;
    lphs = (long*)p->lphs.auxp;

    cps0 = *p->kcps;
    amp0 = *p->kamp;
    count = p->count;

    ar0 = p->sr;
    ar = ar0;
    nsmps = csound->ksmps;
    do
      *ar++ = FL(0.0);
    while (--nsmps);

    do {
      ar = ar0;
      nsmps = csound->ksmps;
      amp = *amptbl++ * amp0;
      cps = *freqtbl++ * cps0;
      inc = (long) (cps * csound->sicvt);
      phs = *lphs;
      do {
        *ar++ += *(ftbl + (phs >> lobits)) * amp;
        phs += inc;
        phs &= PHMASK;
      } while (--nsmps);
      *lphs++ = phs;
    } while (--count);
    return OK;
}

int hsboscset(CSOUND *csound, HSBOSC *p)
{
    FUNC        *ftp;
    int         octcnt, i;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      if (*p->ioctcnt < 2)
        octcnt = 3;
      else
        octcnt = (int)*p->ioctcnt;
      if (octcnt > 10)
        octcnt = 10;
      p->octcnt = octcnt;
      if (*p->iphs >= 0)
        {  for (i=0; i<octcnt; i++)
          p->lphs[i] = ((long)(*p->iphs * FMAXLEN)) & PHMASK;
        }
    }
    if ((ftp = csound->FTFind(csound, p->imixtbl)) != NULL) {
      p->mixtp = ftp;
    }
    return OK;
}

int hsboscil(CSOUND *csound, HSBOSC   *p)
{
    FUNC        *ftp, *mixtp;
    MYFLT       fract, v1, amp0, amp, *ar0, *ar, *ftab, *mtab;
    long        phs, inc, lobits;
    long    phases[10];
    int         nsmps;
    MYFLT       tonal, bright, freq, ampscl;
    int     octcnt = p->octcnt;
    MYFLT   octstart, octoffs, octbase;
    int     octshift, i, mtablen;
    MYFLT       hesr = csound->esr / FL(2.0);

    ftp = p->ftp;
    mixtp = p->mixtp;
    if (ftp==NULL || mixtp==NULL) {
      return csound->PerfError(csound, Str("hsboscil: not initialised"));
    }

    tonal = *p->ktona;
    tonal -= (MYFLT)floor(tonal);
    bright = *p->kbrite - tonal;
    octstart = bright - (MYFLT)octcnt / FL(2.0);
    octbase = (MYFLT)floor(floor(octstart) + 1.5);
    octoffs = octbase - octstart;

    mtab = mixtp->ftable;
    mtablen = mixtp->flen;
    freq = *p->ibasef * (MYFLT)pow(2.0, tonal) * (MYFLT)pow(2.0, octbase);

    ampscl = mtab[(int)((1.0 / (MYFLT)octcnt) * mtablen)];
    amp = mtab[(int)((octoffs / (MYFLT)octcnt) * mtablen)];
    if ((amp - p->prevamp) > (ampscl * 0.5))
      octshift = 1;
    else if ((amp - p->prevamp) < (-(ampscl * 0.5)))
      octshift = -1;
    else
      octshift = 0;
    p->prevamp = amp;

    ampscl = FL(0.0);
    for (i=0; i<octcnt; i++) {
      phases[i] = p->lphs[(i+octshift+100*octcnt) % octcnt];
      ampscl += mtab[(int)(((MYFLT)i / (MYFLT)octcnt) * mtablen)];
    }

    amp0 = *p->kamp / ampscl;
    lobits = ftp->lobits;
    ar0 = p->sr;
    ar = ar0;
    nsmps = csound->ksmps;
    do {
      *ar++ = FL(0.0);
    } while (--nsmps);

    for (i=0; i<octcnt; i++) {
      ar = ar0;
      nsmps = csound->ksmps;
      phs = phases[i];
      amp = mtab[(int)((octoffs / (MYFLT)octcnt) * mtablen)] * amp0;
      if (freq > hesr)
        amp = FL(0.0);
      inc = (long)(freq * csound->sicvt);
      do {
        fract = PFRAC(phs);
        ftab = ftp->ftable + (phs >> lobits);
        v1 = *ftab++;
        *ar++ += (v1 + (*ftab - v1) * fract) * amp;
        phs += inc;
        phs &= PHMASK;
      } while (--nsmps);
      p->lphs[i] = phs;

      octoffs += FL(1.0);
      freq *= FL(2.0);
    }
    return OK;
}

int pitchamdfset(CSOUND *csound, PITCHAMDF *p)
{
    MYFLT srate, downs;
    long  size, minperi, maxperi, downsamp, upsamp, msize, bufsize, interval;
    MYFLT *medi, *buf;

    p->inerr = 0;

    downs = *p->idowns;
    if (downs < (-1.9)) {
      upsamp = (int)((downs * (-1.0)) + 0.5);
      downsamp = 0;
      srate = csound->esr * (float)upsamp;
    }
    else {
      downsamp = (int)(downs+0.5);
      if (downsamp < 1)
        downsamp = 1;
      srate = csound->esr / (float)downsamp;
      upsamp = 0;
    }

    minperi = (long)(srate / *p->imaxcps);
    maxperi = (long)(srate / *p->imincps);
    if (maxperi <= minperi) {
      p->inerr = 1;
      return csound->InitError(csound,
                               Str("pitchamdf: maxcps must be > mincps !"));
    }

    if (*p->iexcps < 1)
        interval = maxperi;
    else
        interval = (long)(srate / *p->iexcps);
    if (interval < csound->ksmps) {
      if (downsamp)
        interval = csound->ksmps / downsamp;
      else
        interval = csound->ksmps * upsamp;
    }

    size = maxperi + interval;
    bufsize = size + maxperi + 2;

    p->srate = srate;
    p->downsamp = downsamp;
    p->upsamp = upsamp;
    p->minperi = minperi;
    p->maxperi = maxperi;
    p->size = size;
    p->readp = 0;
    p->index = 0;
    p->lastval = FL(0.0);
    if (*p->icps < 1)
        p->peri = (minperi + maxperi) / 2;
    else
        p->peri = (int)(srate / *p->icps);

    if (*p->irmsmedi < 1)
        p->rmsmedisize = 0;
    else
        p->rmsmedisize = (int)(*p->irmsmedi+0.5)*2+1;
    p->rmsmediptr = 0;

    if (p->medisize) {
      msize = p->medisize * 3;
      if (p->median.auxp==NULL || p->median.size < (long)sizeof(MYFLT)*msize)
        csound->AuxAlloc(csound, sizeof(MYFLT)*(msize), &p->median);
      medi = (MYFLT*)p->median.auxp;
      do
        *medi++ = FL(0.0);
      while (--msize);
    }

    if (*p->imedi < 1)
      p->medisize = 0;
    else
      p->medisize = (int)(*p->imedi+0.5)*2+1;
    p->mediptr = 0;

    if (p->medisize) {
      msize = p->medisize * 3;
      if (p->median.auxp==NULL || p->median.size < (long)sizeof(MYFLT)*msize)
        csound->AuxAlloc(csound, sizeof(MYFLT)*(msize), &p->median);
      medi = (MYFLT*)p->median.auxp;
      do {
        *medi++ = (MYFLT)p->peri;
      } while (--msize);
    }

    if (p->buffer.auxp==NULL ||
        p->buffer.size < (long)sizeof(MYFLT)*(bufsize)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*(bufsize), &p->buffer);
      buf = (MYFLT*)p->buffer.auxp;
      do {
        *buf++ = FL(0.0);
      } while (--bufsize);
    }
    return OK;
}

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp

MYFLT medianvalue(unsigned long n, MYFLT *vals)
{   /* vals must point to 1 below relevant data! */
    unsigned long i, ir, j, l, mid;
    unsigned long k = (n + 1) / 2;
    MYFLT a, temp;

    l = 1;
    ir = n;
    while (1) {
      if (ir <= l+1) {
        if (ir == l+1 && vals[ir] < vals[l]) {
          SWAP(vals[l], vals[ir]);
        }
        return vals[k];
      }
      else {
        mid = (l+ir) >> 1;
        SWAP(vals[mid], vals[l+1]);
        if (vals[l+1] > vals[ir]) {
          SWAP(vals[l+1], vals[ir]);
        }
        if (vals[l] > vals[ir]) {
          SWAP(vals[l], vals[ir]);
        }
        if (vals[l+1] > vals[l]) {
          SWAP(vals[l+1], vals[l]);
        }
        i = l + 1;
        j = ir;
        a = vals[l];
        while (1) {
          do i++; while (vals[i] < a);
          do j--; while (vals[j] > a);
          if (j < i) break;
          SWAP(vals[i], vals[j]);
        }
        vals[l] = vals[j];
        vals[j] = a;
        if (j >= k) ir = j-1;
        if (j <= k) l = i;
      }
    }
}
#undef SWAP

int pitchamdf(CSOUND *csound, PITCHAMDF *p)
{
    MYFLT *buffer = (MYFLT*)p->buffer.auxp;
    MYFLT *rmsmedian = (MYFLT*)p->rmsmedian.auxp;
    long  rmsmedisize = p->rmsmedisize;
    long  rmsmediptr = p->rmsmediptr;
    MYFLT *median = (MYFLT*)p->median.auxp;
    long  medisize = p->medisize;
    long  mediptr = p->mediptr;
    long  size = p->size;
    long  index = p->index;
    long  minperi = p->minperi;
    long  maxperi = p->maxperi;
    MYFLT *asig = p->asig;
    MYFLT srate = p->srate;
    long  peri = p->peri;
    long  downsamp = p->downsamp;
    long  upsamp = p->upsamp;
    MYFLT upsmp = (MYFLT)upsamp;
    MYFLT lastval = p->lastval;
    MYFLT newval, delta;
    long  readp = p->readp;
    long  interval = size - maxperi;
    int   nsmps = csound->ksmps;
    int   i;
    long  i1, i2;
    MYFLT val, rms;
    double sum;
    MYFLT acc, accmin, diff;

    if (p->inerr) {
      return csound->PerfError(csound, Str("pitchamdf: not initialised"));
    }

    if (upsamp) {
      while (1) {
        newval = asig[readp++];
        delta = (newval-lastval) / upsmp;
        lastval = newval;

        for (i=0; i<upsamp; i++) {
          newval += delta;
          buffer[index++] = newval;

          if (index == size) {
            peri = minperi;
            accmin = FL(0.0);
            for (i2 = 0; i2 < size; ++i2) {
              diff = buffer[i2+minperi] - buffer[i2];
              if (diff > 0)  accmin += diff;
              else           accmin -= diff;
            }
            for (i1 = minperi + 1; i1 <= maxperi; ++i1) {
              acc = FL(0.0);
              for (i2 = 0; i2 < size; ++i2) {
                diff = buffer[i1+i2] - buffer[i2];
                if (diff > 0)   acc += diff;
                else            acc -= diff;
              }
              if (acc < accmin) {
                accmin = acc;
                peri = i1;
              }
            }

            for (i1 = 0; i1 < interval; i1++)
              buffer[i1] = buffer[i1+interval];
            index = maxperi;

            if (medisize) {
              median[mediptr] = (MYFLT)peri;
              for (i1 = 0; i1 < medisize; i1++)
                median[medisize+i1] = median[i1];

              median[medisize*2+mediptr] =
                medianvalue(medisize, &median[medisize-1]);
              peri = (long)median[medisize*2 +
                                 ((mediptr+medisize/2+1) % medisize)];

              mediptr = (mediptr + 1) % medisize;
              p->mediptr = mediptr;
            }
          }
        }
        if (readp >= nsmps) break;
      }
      readp = readp % nsmps;
      p->lastval = lastval;
    }
    else {
      while (1) {
        buffer[index++] = asig[readp];
        readp += downsamp;

        if (index == size) {
          peri = minperi;
          accmin = FL(0.0);
          for (i2 = 0; i2 < size; ++i2) {
            diff = buffer[i2+minperi] - buffer[i2];
            if (diff > FL(0.0))  accmin += diff;
            else              accmin -= diff;
          }
          for (i1 = minperi + 1; i1 <= maxperi; ++i1) {
            acc = FL(0.0);
            for (i2 = 0; i2 < size; ++i2) {
              diff = buffer[i1+i2] - buffer[i2];
              if (diff > FL(0.0))   acc += diff;
              else               acc -= diff;
            }
            if (acc < accmin) {
              accmin = acc;
              peri = i1;
            }
          }

          for (i1 = 0; i1 < interval; i1++)
            buffer[i1] = buffer[i1+interval];
          index = maxperi;

          if (medisize) {
            median[mediptr] = (MYFLT)peri;
            for (i1 = 0; i1 < medisize; i1++)
              median[medisize+i1] = median[i1];

            median[medisize*2+mediptr] =
              medianvalue(medisize, &median[medisize-1]);
            peri = (long)median[medisize*2 +
                               ((mediptr+medisize/2+1) % medisize)];

            mediptr = (mediptr + 1) % medisize;
            p->mediptr = mediptr;
          }
        }

        if (readp >= nsmps) break;
      }
      readp = readp % nsmps;
    }
    buffer = &buffer[(index + size - peri) % size];
    sum = 0.0;
    for (i1=0; i1<peri; i1++) {
      val = *buffer++;
      sum += (double)(val * val);
    }
    rms = (MYFLT)sqrt(sum / (double)peri);
    if (rmsmedisize) {
      rmsmedian[rmsmediptr] = rms;
      for (i1 = 0; i1 < rmsmedisize; i1++)
        rmsmedian[rmsmedisize+i1] = rmsmedian[i1];

      rmsmedian[rmsmedisize*2+rmsmediptr] =
        medianvalue(rmsmedisize, &rmsmedian[rmsmedisize-1]);
      rms = rmsmedian[rmsmedisize*2 +
                     ((rmsmediptr+rmsmedisize/2+1) % rmsmedisize)];

      rmsmediptr = (rmsmediptr + 1) % rmsmedisize;
      p->rmsmediptr = rmsmediptr;
    }

    *p->kcps = srate / (MYFLT)peri;
    *p->krms = rms;
    p->index = index;
    p->peri = peri;
    p->readp = readp;

    return OK;
}

/*==================================================================*/
/* phasorbnk                                                        */
/*==================================================================*/

int phsbnkset(CSOUND *csound, PHSORBNK *p)
{
    double  phs;
    int    count;
    double  *curphs;

    count = (int)(*p->icnt + 0.5);
    if (count < 2)
      count = 2;

    if (p->curphs.auxp==NULL || p->curphs.size < (long)sizeof(double)*count)
      csound->AuxAlloc(csound, sizeof(double)*count, &p->curphs);

    curphs = (double*)p->curphs.auxp;
    if (*p->iphs > 1) {
      do
        *curphs++ = (double)rand()/(double)RAND_MAX;
      while (--count);
    }
    else if ((phs = *p->iphs) >= 0) {
      do
        *curphs++ = phs;
      while (--count);
    }
    return OK;
}

int kphsorbnk(CSOUND *csound, PHSORBNK *p)
{
    double  phs;
    double  *curphs = (double*)p->curphs.auxp;
    int     size = p->curphs.size / sizeof(double);
    int     index = (int)(*p->kindx);

    if (curphs == NULL) {
      return csound->PerfError(csound, Str("phasorbnk: not initialised"));
    }

    if (index<0 || index>=size) {
      *p->sr = FL(0.0);
      return NOTOK;
    }

    *p->sr = (MYFLT)(phs = curphs[index]);
    if ((phs += *p->xcps * csound->onedkr) >= 1.0)
      phs -= 1.0;
    else if (phs < 1.0)
      phs += 1.0;
    curphs[index] = phs;
    return OK;
}

int phsorbnk(CSOUND *csound, PHSORBNK *p)
{
    int     nsmps = csound->ksmps;
    MYFLT   *rs;
    double  phase, incr;
    double  *curphs = (double*)p->curphs.auxp;
    int     size = p->curphs.size / sizeof(double);
    int     index = (int)(*p->kindx);

    if (curphs == NULL) {
      return csound->PerfError(csound, Str("phasorbnk: not initialised"));
    }

    if (index<0 || index>=size) {
      *p->sr = FL(0.0);
      return NOTOK;
    }

    rs = p->sr;
    phase = curphs[index];
    if (p->XINCODE) {
      MYFLT *cps = p->xcps;
      do {
        incr = (double)(*cps++ * csound->onedsr);
        *rs++ = (MYFLT)phase;
        phase += incr;
        if (phase >= 1.0)
          phase -= 1.0;
        else if (phase < 0.0)
          phase += 1.0;
      } while (--nsmps);
    }
    else {
      incr = (double)(*p->xcps * csound->onedsr);
      do {
        *rs++ = (MYFLT)phase;
        phase += incr;
        if (phase >= 1.0)
          phase -= 1.0;
        else if (phase < 0.0)
          phase += 1.0;
      } while (--nsmps);
    }
    curphs[index] = phase;
    return OK;
}

/* Opcodes from rasmus ekman */

/* pinkish: Two methods for pink-type noise generation
   The Moore/Gardner method, coded by Phil Burke, optimised by James McCartney;
   Paul Kellet's  -3dB/octave white->pink filter bank, "refined" version;
   Paul Kellet's  -3dB/octave white->pink filter bank, "economy" version

   The Moore/Gardner method output seems to have bumps in the low-mid and
   mid-high ranges.
   The Kellet method (refined) has smooth spectrum, but goes up slightly
   at the far high end.
 */

#define GARDNER_PINK        FL(0.0)
#define KELLET_PINK         FL(1.0)
#define KELLET_CHEAP_PINK   FL(2.0)

int GardnerPink_init(CSOUND *csound, PINKISH *p);
int GardnerPink_perf(CSOUND *csound, PINKISH *p);

int pinkset(CSOUND *csound, PINKISH *p)
{
        /* Check valid method */
    if (*p->imethod != GARDNER_PINK && *p->imethod != KELLET_PINK
        && *p->imethod != KELLET_CHEAP_PINK) {
      return csound->InitError(csound, Str("pinkish: Invalid method code"));
    }
    /* User range scaling can be a- or k-rate for Gardner, a-rate only
       for filter */
    if (XINARG1) {
      p->ampinc = 1;
    }
    else {
      /* Cannot accept k-rate input with filter method */
      if (*p->imethod != FL(0.0)) {
        return csound->InitError(csound, Str(
                      "pinkish: Filter method requires a-rate (noise) input"));
      }
      p->ampinc = 0;
    }
    /* Unless we're reinitializing a tied note, zero coefs */
    if (*p->iskip != FL(1.0)) {
      if (*p->imethod == GARDNER_PINK)
        GardnerPink_init(csound,p);
      else                                      /* Filter method */
        p->b0 = p->b1 = p->b2 = p->b3 = p->b4 = p->b5 = p->b6 = FL(0.0);
    }
    return OK;
}

int pinkish(CSOUND *csound, PINKISH *p)
{
    MYFLT       *aout, *ain;
    double      c0, c1, c2, c3, c4, c5, c6, nxtin, nxtout;
    int    nsmps = csound->ksmps;
    aout = p->aout;
    ain = p->xin;

    if (*p->imethod == GARDNER_PINK) {  /* Gardner method (default) */
      GardnerPink_perf(csound,p);
    }
    else if (*p->imethod == KELLET_PINK) {
      /* Paul Kellet's "refined" pink filter */
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;
      c3 = p->b3; c4 = p->b4; c5 = p->b5; c6 = p->b6;
      do {
        nxtin = *ain++;
        c0 = c0 * 0.99886 + nxtin * 0.0555179;
        c1 = c1 * 0.99332 + nxtin * 0.0750759;
        c2 = c2 * 0.96900 + nxtin * 0.1538520;
        c3 = c3 * 0.86650 + nxtin * 0.3104856;
        c4 = c4 * 0.55000 + nxtin * 0.5329522;
        c5 = c5 * -0.7616 - nxtin * 0.0168980;
        nxtout = c0 + c1 + c2 + c3 + c4 + c5 + c6 + nxtin * 0.5362;
        *aout++ = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
        c6 = nxtin * 0.115926;
      } while(--nsmps);
      /* Store back filter coef states */
      p->b0 = c0; p->b1 = c1; p->b2 = c2;
      p->b3 = c3; p->b4 = c4; p->b5 = c5; p->b6 = c6;
    }
    else if (*p->imethod == KELLET_CHEAP_PINK) {
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;

      do {      /* Paul Kellet's "economy" pink filter */
        nxtin = *ain++;
        c0 = c0 * 0.99765 + nxtin * 0.0990460;
        c1 = c1 * 0.96300 + nxtin * 0.2965164;
        c2 = c2 * 0.57000 + nxtin * 1.0526913;
        nxtout = c0 + c1 + c2 + nxtin * 0.1848;
        *aout++ = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
      } while(--nsmps);

      /* Store back filter coef states */
      p->b0 = c0; p->b1 = c1; p->b2 = c2;
    }
    return OK;
}

/************************************************************/
/*
        GardnerPink_init() and GardnerPink_perf()

        Generate Pink Noise using Gardner method.
        Optimization suggested by James McCartney uses a tree
        to select which random value to replace.

    x x x x x x x x x x x x x x x x
     x   x   x   x   x   x   x   x
       x       x       x       x
           x               x
                   x

    Tree is generated by counting trailing zeros in an increasing index.
        When the index is zero, no random number is selected.

    Author: Phil Burk, http://www.softsynth.com

        Revision History:
                Csound version by rasmus ekman May 2000
                Several changes, some marked "(re)"

    Copyright 1999 Phil Burk - No rights reserved.
*/

/************************************************************/

/* Yet another pseudo-random generator. Could probably be changed
   for any of the other available ones in Csound */

#define PINK_RANDOM_BITS       (24)
/* Left-shift one bit less 24 to allow negative values (re) */
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS-1)

/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static long GenerateRandomNumber(unsigned long randSeed)
{
    randSeed = (randSeed * 196314165) + 907633515;
    return randSeed;
}

/************************************************************/

/* Set up for user-selected number of bands of noise generators. */
int GardnerPink_init(CSOUND *csound, PINKISH *p)
{
    int i;
    MYFLT pmax;
    long numRows;

    /* Set number of rows to use (default to 20) */
    if (*p->iparam1 >= 4 && *p->iparam1 <= GRD_MAX_RANDOM_ROWS)
      p->grd_NumRows = (long)*p->iparam1;
    else {
      p->grd_NumRows = 20;
      /* Warn if user tried but failed to give sensible number */
      if (*p->iparam1 != FL(0.0))
        csound->Message(csound, "pinkish: Gardner method requires 4-%d bands. "
                                "Default %ld substituted for %d.\n",
                                GRD_MAX_RANDOM_ROWS, p->grd_NumRows,
                                (int) *p->iparam1);
    }

    /* Seed random generator by user value or by time (default) */
    if (*p->iseed != FL(0.0)) {
      if (*p->iseed > -1.0 && *p->iseed < 1.0)
        p->randSeed = (unsigned long) (*p->iseed * (MYFLT)0x80000000);
      else p->randSeed = (unsigned long) *p->iseed;
    }
    else p->randSeed = (unsigned long) csound->GetRandomSeedFromTime();

    numRows = p->grd_NumRows;
    p->grd_Index = 0;
    if (numRows == 32) p->grd_IndexMask = 0xFFFFFFFF;
    else p->grd_IndexMask = (1<<numRows) - 1;

    /* Calculate reasonable maximum signed random value. */
    /* Tweaked to get sameish peak value over all numRows values (re) */
    pmax = (MYFLT)((numRows + 30) * (1<<(PINK_RANDOM_BITS-2)));
    p->grd_Scalar = FL(1.0) / pmax;

/* Warm up by filling all rows (re) (original zeroed all rows, and runningSum) */
    {
      long randSeed, newRandom, runningSum = 0;
      randSeed = p->randSeed;
      for (i = 0; i < numRows; i++) {
        randSeed = GenerateRandomNumber(randSeed);
        newRandom = randSeed >> PINK_RANDOM_SHIFT;
        runningSum += newRandom;
        p->grd_Rows[i] = newRandom;
      }
      p->grd_RunningSum = runningSum;
      p->randSeed = randSeed;
    }
    return OK;
}

/* Generate numRows octave-spaced white bands and sum to pink noise. */
int GardnerPink_perf(CSOUND *csound, PINKISH *p)
{
    MYFLT *aout, *amp, scalar;
    long *rows, rowIndex, indexMask, randSeed, newRandom;
    long runningSum, sum, ampinc;
    int nsmps = csound->ksmps;

    aout        = p->aout;
    amp         = p->xin;
    ampinc      = p->ampinc;    /* Used to increment user amp if a-rate */
    scalar      = p->grd_Scalar;
    rowIndex    = p->grd_Index;
    indexMask   = p->grd_IndexMask;
    runningSum  = p->grd_RunningSum;
    rows        = &(p->grd_Rows[0]);
    randSeed    = p->randSeed;

    do {
      /* Increment and mask index. */
      rowIndex = (rowIndex + 1) & indexMask;

      /* If index is zero, don't update any random values. */
      if ( rowIndex != 0 ) {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int numZeros = 0;
        int n = rowIndex;
        while( (n & 1) == 0 ) {
          n = n >> 1;
          numZeros++;
        }

        /* Replace the indexed ROWS random value.
         * Subtract and add back to RunningSum instead of adding all
         * the random values together. Only one changes each time.
         */
        runningSum -= rows[numZeros];
        randSeed = GenerateRandomNumber(randSeed);
        newRandom = randSeed >> PINK_RANDOM_SHIFT;
        runningSum += newRandom;
        rows[numZeros] = newRandom;
      }

      /* Add extra white noise value. */
      randSeed = GenerateRandomNumber(randSeed);
      newRandom = randSeed >> PINK_RANDOM_SHIFT;
      sum = runningSum + newRandom;

      /* Scale to range of +/-p->xin (user-selected amp) */
      *aout++ = *amp * sum * scalar;
      amp += ampinc;            /* Increment if amp is a-rate */
    } while(--nsmps);

    p->grd_RunningSum = runningSum;
    p->grd_Index = rowIndex;
    p->randSeed = randSeed;
    return OK;
}

/* ************************************************************ */
/* A collection of clipping techniques    -- JPff               */
/* Method 0: Bram de Jong <Bram.DeJong@rug.ac.be>               */
/* x > a:  f(x) = a + (x-a)/(1+((x-a)/(1-a))^2)                 */
/* x > 1:  f(x) = (a+1)/2                                       */
/* JPff scaled this to a limit and a fraction                   */
/* Method 1:                                                    */
/* |x|<limit f(x) = limit * sin(pi x/(2*limit)                  */
/*           f(x) = limit * sign(x)                             */
/* Method 2:                                                    */
/* |x|<limit f(x) = limit * tanh(x/limit)/tanh(1)               */
/*           f(x) = limit * sign(x)                             */
/* ************************************************************ */

/* Methods 0 and 2 OK, method1 broken */

double tanh(double);
int clip_set(CSOUND *csound, CLIP *p)
{
    int meth = (int)(*p->imethod + FL(0.5));
    p->meth = meth;
    p->arg = *p->iarg;
    p->lim = *p->limit;
    if (p->arg < FL(0.0)) p->arg = - p->arg;
    switch (meth) {
    case 0:                     /* Bram de Jong method */
      if (p->arg > FL(1.0) || p->arg < FL(0.0)) p->arg = FL(0.999);
      p->arg = p->lim * p->arg;
      p->k1 = FL(1.0)/(p->lim - p->arg);
      p->k1 = p->k1 * p->k1;
      p->k2 = (p->lim + p->arg)*FL(0.5);
      break;
    case 1:
      p->k1 = PI_F/(FL(2.0) * p->lim);
      break;
    case 2:
      p->k1 = FL(1.0)/(MYFLT)tanh(1.0);
      break;
    default:
      p->meth = 0;
    }
    return OK;
}

int clip(CSOUND *csound, CLIP *p)
{
    MYFLT *aout = p->aout, *ain = p->ain;
    int nsmps = csound->ksmps;
    MYFLT a = p->arg, k1 = p->k1, k2 = p->k2;
    MYFLT limit = p->lim;
    MYFLT rlim = FL(1.0)/limit;

    switch (p->meth) {
    case 0:                     /* Soft clip with division */
      do {
        MYFLT x = *ain++;
        if (x>=FL(0.0)) {
          if (x>limit) x = k2;
          else if (x>a)
            x = a + (x-a)/(FL(1.0)+(x-a)*(x-a)*k1);
        }
        else {
          if (x<-limit)
            x = -k2;
          else if (-x>a)
            x = -a + (x+a)/(FL(1.0)+(x+a)*(x+a)*k1);
        }
        *aout++ = x;
      } while(--nsmps);
      return OK;
    case 1:
      do {
        MYFLT x = *ain++;
        if (x>=limit)
            x = limit;
        else if (x<= -limit)
          x = -limit;
        else
            x = limit*(MYFLT)sin((double)(k1*x));
        *aout++ = x;
      } while(--nsmps);
      return OK;
    case 2:
      do {
        MYFLT x = *ain++;
        if (x>=limit)
            x = limit;
        else if (x<= -limit)
          x = -limit;
        else
          x = limit*k1*(MYFLT)tanh((double)(x*rlim));
        *aout++ = x;
      } while(--nsmps);
      return OK;
    }
    return OK;
}

#ifdef BETA
/* ********************************************************************** */
/* *************** EXPERIMENT ******************************************* */
/* ********************************************************************** */

#include "ugens2.h"

int Foscset(CSOUND *csound, XOSC *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL) { /*Allow any length*/
      p->ftp = ftp;
      if (*p->iphs >= 0) {
        p->lphs = *p->iphs * ftp->flen;
        while (p->lphs>ftp->flen) p->lphs -= ftp->flen;
      }
      else
        p->lphs = FL(0.0);
    }
    return OK;
}

int Fosckk(CSOUND *csound, XOSC *p)
{
    FUNC        *ftp;
    MYFLT       amp, *ar, *ftbl;
    MYFLT       inc, phs;
    int nsmps = csound->ksmps;
    int flen;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    flen = ftp->flen;
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = (*p->xcps * flen) * csound->onedsr;
    amp = *p->xamp;
    ar = p->sr;
    do {
      *ar++ = *(ftbl + (int)(phs)) * amp;
      phs += inc;
      if (phs>flen) phs -= flen;
    } while (--nsmps);
    p->lphs = phs;
    return OK;
}

int Foscak(CSOUND *csound, XOSC *p)
{
    FUNC        *ftp;
    MYFLT       *ampp, *ar, *ftbl;
    MYFLT       inc, phs;
    int nsmps = csound->ksmps;
    int flen;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    flen = ftp->flen;
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = (*p->xcps * flen) * csound->onedsr;
    ampp = p->xamp;
    ar = p->sr;
    do {
      *ar++ = *(ftbl + (int)(phs)) * *ampp++;
      phs += inc;
      if (phs>flen) phs -= flen;
    } while (--nsmps);
    p->lphs = phs;
    return OK;
}

int Foscka(CSOUND *csound, XOSC *p)
{
    FUNC        *ftp;
    MYFLT       amp, *ar, *cpsp, *ftbl;
    MYFLT       inc, phs;
    int nsmps = csound->ksmps;
    int flen;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    flen = ftp->flen;
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = (*p->xcps * flen) * csound->onedsr;
    amp = *p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    do {
      MYFLT inc = (*cpsp++ *flen * csound->onedsr);
      *ar++ = *(ftbl + (int)(phs)) * amp;
      phs += inc;
      if (phs>flen) phs -= flen;
    } while (--nsmps);
    p->lphs = phs;
    return OK;
}

int Foscaa(CSOUND *csound, XOSC *p)
{
    FUNC        *ftp;
    MYFLT       *ampp, *ar, *ftbl;
    MYFLT       phs;
    int nsmps = csound->ksmps;
    int flen;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    flen = ftp->flen;
    ftbl = ftp->ftable;
    phs = p->lphs;
    ampp = p->xamp;
    ar = p->sr;
    do {
      MYFLT inc = (*p->xcps++ * flen) * csound->onedsr;
      *ar++ = *(ftbl + (int)(phs)) * *ampp++;
      phs += inc;
      if (phs>flen) phs -= flen;
    } while (--nsmps);
    p->lphs = phs;
    return OK;
}

#endif

/* ********************************************************************** */
/* *************** SENSING ********************************************** */
/* ********************************************************************** */
#if defined(__unix)
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef HAVE_TERMIOS_H
struct termios tty;
#endif

int isense(CSOUND *csound, KSENSE *p)
{
#if defined(WIN32)
    setvbuf(stdin, NULL, _IONBF, 0); /* Does not seem to work */
#else
    tcgetattr(0, &tty);
    tty.c_lflag &= (~ICANON);
    tcsetattr(0, TCSANOW, &tty);
#endif
    return OK;
}

int ksense(CSOUND *csound, KSENSE *p)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    /* No waiting */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    retval = select(1, &rfds, NULL, NULL, &tv);

    if (retval) {
      char ch;
      read(0, &ch,1);
      *p->ans = (MYFLT)ch /*getchar()*/;
    /* FD_ISSET(0, &rfds) will be true. */
    }
    else
      *p->ans = -FL(1.0);

    return OK;
}
#else
int isense(CSOUND *csound, KSENSE *p)
{
    return OK;
}
# ifdef WIN32
#include <conio.h>

int ksense(CSOUND *csound, KSENSE *p)
{
    if (_kbhit())
      *p->ans = (MYFLT)_getch();
    else
      *p->ans = -FL(1.0);
    return OK;
}
# else
int ksense(CSOUND *csound, KSENSE *p)
{
        *p->ans = getchar();
        return OK;
}
# endif
#endif

/* ********************************************************************** */
/* *************** IMPULSE ********************************************** */
/* ********************************************************************** */

int impulse_set(CSOUND *csound, IMPULSE *p)
{
    p->next = (int)(FL(0.5) + *p->offset * csound->esr);
    return OK;
}

int impulse(CSOUND *csound, IMPULSE *p)
{
    int nsmps = csound->ksmps;
    int next = p->next;
    MYFLT *ar = p->ar;
    if (next < csound->ksmps) {         /* Impulse in this frame */
      MYFLT frq = *p->freq;     /* Freq at k-rate */
      int sfreq;                /* Converted to samples */
      if (frq == FL(0.0)) sfreq = INT_MAX; /* Zero means infinite */
      else if (frq < FL(0.0)) sfreq = -(int)frq; /* Negative cnts in sample */
      else sfreq = (int)(frq*csound->esr); /* Normal case */
      do {
        if (next-- == 0) {
          *ar = *p->amp;
          next = sfreq - 1;     /* Note can be less than k-rate */
        }
        else *ar = FL(0.0);
        ar++;
      } while (--nsmps);
    }
    else {                      /* Nothing this time so just fill */
      do {
        *ar++ = FL(0.0);
      } while (--nsmps);
      next -= csound->ksmps;
    }
    p->next = next;
    return OK;
}

/* ********************************************************************** */
/* Version of CMUSIC trans opcode                                         */
/* creates y0 + (y1 - y0) * (1 - exp( t*alpha )) / (1 - exp(alpha))       */
/* or                                                                     */
/*         y0 + (y1 - y0) * t if alpha is zero                            */
/* ********************************************************************** */
int trnset(CSOUND *csound, TRANSEG *p)
{
    NSEG        *segp;
    int         nsegs;
    MYFLT       **argp, val;

    nsegs = p->INOCOUNT / 3;            /* count segs & alloc if nec */
    if ((segp = (NSEG *) p->auxch.auxp) == NULL ||
        nsegs*sizeof(NSEG) < (unsigned int)p->auxch.size) {
      csound->AuxAlloc(csound, (long)nsegs*sizeof(NSEG), &p->auxch);
      p->cursegp = segp = (NSEG *) p->auxch.auxp;
      segp[nsegs-1].cnt = MAXPOS;     /* set endcount for safety */
    }
    argp = p->argums;
    val = **argp++;
    if (**argp <= FL(0.0)) return OK; /* if idur1 <= 0, skip init  */
    p->curval = val;
    p->curcnt = 0;
    p->cursegp = segp - 1;            /* else setup null seg0 */
    p->segsrem = nsegs + 1;
    p->curx = FL(0.0);
    do {                              /* init each seg ..  */
      MYFLT dur = **argp++;
      MYFLT alpha = **argp++;
      MYFLT nxtval = **argp++;
      MYFLT d = dur * csound->esr;
      if ((segp->cnt = (long)(d + FL(0.5))) < 0)
        segp->cnt = 0;
      else
        segp->cnt = (long)(dur * csound->ekr);
        segp->nxtpt = nxtval;
      segp->val = val;
      if (alpha == FL(0.0)) {
        segp->c1 = (nxtval-val)/d;
      }
      else {
        segp->c1 = (nxtval - val)/(FL(1.0) - (MYFLT)exp((double)alpha));
      }
      segp->alpha = alpha/d;
      val = nxtval;
      segp++;
    } while (--nsegs);
    p->xtra = -1;
    p->alpha = ((NSEG*)p->auxch.auxp)[0].alpha;
    p->curinc = ((NSEG*)p->auxch.auxp)[0].c1;
    return OK;
}

int ktrnseg(CSOUND *csound, TRANSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (p->auxch.auxp==NULL) { /* RWD fix */
      csound->Die(csound, Str("\nError: transeg not initialised (krate)"));
    }
    if (p->segsrem) {                   /* done if no more segs */
      if (--p->curcnt <= 0) {           /* if done cur segment  */
        NSEG *segp = p->cursegp;
      chk1:
        if (!(--p->segsrem))  {
          p->curval = segp->nxtpt;      /* advance the cur val  */
          return OK;
        }
        p->cursegp = ++segp;            /*   find the next      */
        if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
          p->curval = segp->nxtpt;      /*   poslen = new slope */
          goto chk1;
        }
        p->curinc = segp->c1;
        p->alpha = segp->alpha;
        p->curx = FL(0.0);
      }
      if (p->alpha == FL(0.0))
        p->curval += p->curinc*csound->ksmps;   /* advance the cur val  */
      else
        p->curval = p->cursegp->val + p->curinc *
          (FL(1.0) - (MYFLT)exp((double)(p->curx)));
      p->curx += (MYFLT)csound->ksmps*p->alpha;
    }
    return OK;
}

int trnseg(CSOUND *csound, TRANSEG *p)
{
    MYFLT  val, *rs = p->rslt;
    int         nsmps = csound->ksmps;
    NSEG        *segp = p->cursegp;
    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("transeg: not initialised (arate)\n"));
    }
    val = p->curval;                      /* sav the cur value    */
    if (p->segsrem) {                     /* if no more segs putk */
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        segp = p->cursegp;
      chk1:
        if (!--p->segsrem) {              /*   if none left       */
          val = p->curval = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        p->cursegp = ++segp;              /*   else find the next */
        if (!(p->curcnt = segp->cnt)) {
          val = p->curval = segp->nxtpt;  /*   nonlen = discontin */
          goto chk1;
        }                                 /*   poslen = new slope */
        p->curinc = segp->c1;
        p->alpha = segp->alpha;
        p->curx = FL(0.0);
        p->curval = val;
      }
      if (p->alpha == FL(0.0)) {
        do {
          *rs++ = val;
          val += p->curinc;
        } while (--nsmps);
      }
      else {
        do {
          *rs++ = val;
          p->curx += p->alpha;
          val = segp->val + p->curinc *
            (FL(1.0) - (MYFLT)exp((double)(p->curx)));
        } while (--nsmps);
      }
      p->curval = val;
      return OK;
putk:
      do {
        *rs++ = val;
      } while (--nsmps);
    }
    return OK;
}

extern long randint31(long);

int varicolset(CSOUND *csound, VARI *p)
{
    p->last = FL(0.0);
    p->lastbeta = *p->beta;
    p->sq1mb2 = (MYFLT)sqrt(FL(1.0)-p->lastbeta * p->lastbeta);
    p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    p->ampinc = XINARG1 ? 1 : 0;
    return OK;
}

int varicol(CSOUND *csound, VARI *p)
{
    int         nsmps = csound->ksmps;
    MYFLT       beta = *p->beta;
    MYFLT       sq1mb2 = p->sq1mb2;
    MYFLT       lastx = p->last;
    MYFLT       ampmod = p->ampmod;
    MYFLT       *kamp = p->kamp;
    int         ampinc = p->ampinc;
    MYFLT       *rslt = p->rslt;

    if (beta != p->lastbeta) {
       beta = p->lastbeta = *p->beta;
       sq1mb2 = p->sq1mb2 = (MYFLT)sqrt(FL(1.0)-p->lastbeta * p->lastbeta);
       ampmod = p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    }

    do {
      MYFLT rnd =  FL(2.0)*(MYFLT)rand()/(MYFLT)RAND_MAX - FL(1.0);
      lastx = lastx * beta + sq1mb2 * rnd;
      *rslt++ = lastx * *kamp * ampmod;
      kamp += ampinc;
    } while (--nsmps);
    p->last = lastx;
    return OK;
}

/* ************************************************************************ */
/* ***** Josep Comajuncosas' 18dB/oct resonant 3-pole LPF with tanh dist ** */
/* ***** Coded in C by John ffitch, 2000 Dec 17 *************************** */
/* ************************************************************************ */
#include <math.h>

/* This code is transcribed from a Csound macro, so no real comments */

int lpf18set(CSOUND *csound, LPF18 *p)
{
    /* Initialise delay lines */
    p->ay1 = FL(0.0);
    p->ay2 = FL(0.0);
    p->aout = FL(0.0);
    p->lastin = FL(0.0);
    return OK;
}

int lpf18db(CSOUND *csound, LPF18 *p)
{
    int         nsmps = csound->ksmps;
    MYFLT kfcn = FL(2.0) * *p->fco * csound->onedsr;
    MYFLT kp   = ((-FL(2.7528)*kfcn + FL(3.0429))*kfcn +
                  FL(1.718))*kfcn - FL(0.9984);
    MYFLT kp1 = kp+FL(1.0);
    MYFLT kp1h = FL(0.5)*kp1;
    /* Version using log */
    /* MYFLT kres = *p->res * (FL(2.2173) - FL(1.6519)*log(kp+FL(1.0))); */
    MYFLT kres = *p->res * (((-FL(2.7079)*kp1 + FL(10.963))*kp1
                             - FL(14.934))*kp1 + FL(8.4974));
    MYFLT ay1 = p->ay1;
    MYFLT ay2 = p->ay2;
    MYFLT aout = p->aout;
    MYFLT *ain = p->ain;
    MYFLT *ar = p->ar;
    double dist = (double)*p->dist;
    MYFLT lastin = p->lastin;
    double value = 1.0+(dist*(1.5+2.0*(MYFLT)kres*(1.0-(MYFLT)kfcn)));

    do {
      MYFLT ax1   = lastin;
      MYFLT ay11  = ay1;
      MYFLT ay31  = ay2;
      lastin  =  *ain++ - (MYFLT)tanh((double)(kres*aout));
      ay1      = kp1h * (lastin + ax1) - kp*ay1;
      ay2      = kp1h * (ay1 + ay11) - kp*ay2;
      aout     = kp1h * (ay2 + ay31) - kp*aout;

      *ar++ = (MYFLT) tanh(aout*value);
    } while (--nsmps);
    p->ay1 = ay1;
    p->ay2 = ay2;
    p->aout = aout;
    p->lastin = lastin;
    return OK;
}

/* ************************************************** */
/* **** Wishart wavesets     ************************ */
/* **** from Trevor and CDP  ************************ */
/* **** John ffitch Jan 2001 ************************ */
/* ************************************************** */

int wavesetset(CSOUND *csound, BARRI *p)
{
    if (*p->len == FL(0.0))
      p->length = 1 + (int)(p->h.insdshead->p3 * csound->esr * FL(0.5));
    else
      p->length = 1 + (int)*p->len;
    if (p->length <= 1) p->length = (int)csound->esr;
    csound->AuxAlloc(csound, (long)p->length*sizeof(MYFLT), &p->auxch);
    p->cnt = 1;
    p->start = 0;
    p->current = 0;
    p->end = 0;
    p->direction = 1;
    p->lastsamp = FL(1.0);
    p->noinsert = 0;
    return OK;
}

int waveset(CSOUND *csound, BARRI *p)
{
    MYFLT *in = p->ain;
    MYFLT *out = p->ar;
    int   index = p->end;
    MYFLT *insert = (MYFLT*)(p->auxch.auxp) + index;
    int   nsmps = csound->ksmps;
    if (p->noinsert) goto output;
    do {                        /* Deal with inputs */
      *insert++ = *in++;
      if (++index ==  p->start) {
        p->noinsert = 1;
        break;
      }
      if (index==p->length) {   /* Input wrapping */
        index = 0;
        insert = (MYFLT*)(p->auxch.auxp);
      }
    } while (--nsmps);
 output:
    p->end = index;
    nsmps = csound->ksmps;
    index = p->current;
    insert = (MYFLT*)(p->auxch.auxp) + index;
    do {
      MYFLT samp = *insert++;
      index ++;
      if (index==p->length) {
        index = 0;
        insert = (MYFLT*)(p->auxch.auxp);
      }
      if (samp != FL(0.0) && p->lastsamp*samp < FL(0.0)) {
        if (p->direction == 1)
          p->direction = -1;    /* First cross */
        else {                  /* Second cross */
          p->direction = 1;
          if (++p->cnt > *p->rep) {
            p->cnt = 1;
            p->start = index;
            p->noinsert = 0;
          }
          else {
            index = p->start;
            insert = (MYFLT*)(p->auxch.auxp) + index;
          }
        }
      }
      if (samp != FL(0.0)) p->lastsamp = samp;
      *out++ = samp;
    } while (--nsmps);
    p->current = index;
    return OK;
}

