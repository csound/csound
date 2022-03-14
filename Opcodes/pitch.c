
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

// #include "csdl.h"
#include "csoundCore.h"       /*                              PITCH.C         */
#include <math.h>
#include <limits.h>
#include "cwindow.h"
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"
#include <inttypes.h>

#define STARTING  1
#define PLAYING   2
#define LOGTWO    (0.69314718055994530942)

static inline int32 MYFLOOR(MYFLT x) {
  if (x >= 0.0) {
    return (int32) x;
  } else {
    return (int32) (x - FL(0.99999999));
  }
}

static const MYFLT bicoefs[] = {
    -FL(0.2674054), FL(0.7491305), FL(0.7160484), FL(0.0496285), FL(0.7160484),
     FL(0.0505247), FL(0.3514850), FL(0.5257536), FL(0.3505025), FL(0.5257536),
     FL(0.3661840), FL(0.0837990), FL(0.3867783), FL(0.6764264), FL(0.3867783)
};

#define rand_31(x) (x->Rand31(&(x->randSeed1)) - 1)

int32_t pitchset(CSOUND *csound, PITCH *p)  /* pitch - uses spectra technology */
{
    double  b;                          /* For RMS */
    int32_t     n, nocts, nfreqs, ncoefs;
    MYFLT   Q, *fltp;
    OCTDAT  *octp;
    DOWNDAT *dwnp = &p->downsig;
    SPECDAT *specp = &p->wsig;
    int32   npts, nptls, nn, lobin;
    int32_t     *dstp, ptlmax;
    MYFLT   fnfreqs, rolloff, *oct0p, *flop, *fhip, *fundp, *fendp, *fp;
    MYFLT   weight, weightsum, dbthresh, ampthresh;

                                /* RMS of input signal */
    b = 2.0 - cos(10.0*(double)csound->tpidsr);
    p->c2 = b - sqrt(b * b - 1.0);
    p->c1 = 1.0 - p->c2;
    if (!*p->istor) p->prvq = 0.0;
                                /* End of rms */
                                /* Initialise spectrum */
    /* for mac roundoff */
    p->timcount = (int32_t)(CS_EKR * *p->iprd + FL(0.001));
    nocts = (int32_t)*p->iocts; if (UNLIKELY(nocts<=0)) nocts = 6;
    nfreqs = (int32_t)*p->ifrqs; if (UNLIKELY(nfreqs<=0)) nfreqs = 12;
    ncoefs = nocts * nfreqs;
    Q = *p->iq; if (UNLIKELY(Q<=FL(0.0))) Q = FL(15.0);

    if (UNLIKELY(p->timcount <= 0))
      return csound->InitError(csound, Str("illegal iprd"));
    if (UNLIKELY(nocts > MAXOCTS))
      return csound->InitError(csound, Str("illegal iocts"));
    if (UNLIKELY(nfreqs > MAXFRQS))
      return csound->InitError(csound, Str("illegal ifrqs"));

    if (nocts != dwnp->nocts ||
        nfreqs != p->nfreqs  || /* if anything has changed */
        Q != p->curq ) {        /*     make new tables */
      double      basfrq, curfrq, frqmlt, Qfactor;
      double      theta, a, windamp, onedws, pidws;
      MYFLT       *sinp, *cosp;
      int32_t         k, sumk, windsiz, halfsiz, *wsizp, *woffp;
      int32       auxsiz, bufsiz;
      int32       majr, minr, totsamps;
      double      hicps,locps,oct;      /*   must alloc anew */

      p->nfreqs = nfreqs;
      p->curq = Q;
      p->ncoefs = ncoefs;
      dwnp->srate = CS_ESR;
      hicps = dwnp->srate * 0.375;            /* top freq is 3/4 pi/2 ...   */
      oct = log(hicps / ONEPT) / LOGTWO;      /* octcps()  (see aops.c)     */
      dwnp->looct = (MYFLT)(oct - nocts);     /* true oct val of lowest frq */
      locps = hicps / (1L << nocts);
      basfrq = hicps * 0.5;                   /* oct below retuned top */
      frqmlt = pow(2.0,1.0/(double)nfreqs);   /* nfreq interval mult */
      Qfactor = Q * dwnp->srate;
      curfrq = basfrq;
      for (sumk=0,wsizp=p->winlen,woffp=p->offset,n=nfreqs; n--; ) {
        *wsizp++ = k = (int32_t)(Qfactor/curfrq) | 01;  /* calc odd wind sizes */
        *woffp++ = (*(p->winlen) - k) / 2;          /* & symmetric offsets */
        sumk += k;                                  /*    and find total   */
        curfrq *= frqmlt;
      }
      windsiz = *(p->winlen);
      auxsiz = (windsiz + 2*sumk) * sizeof(MYFLT);   /* calc lcl space rqd */

      csound->AuxAlloc(csound, (size_t)auxsiz, &p->auxch1); /* & alloc auxspace  */

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
      DOWNset(csound, dwnp, totsamps);      /* auxalloc in DOWNDAT struct */
      fltp = (MYFLT *) dwnp->auxch.auxp;    /*  & distrib to octdata */
      for (n=nocts,octp=dwnp->octdata+(nocts-1); n--; octp--) {
        bufsiz = majr + minr;
        octp->begp = fltp;  fltp += bufsiz; /*        (lo oct first) */
        octp->endp = fltp;  minr *= 2;
      }
      SPECset(csound, specp, (int32)ncoefs);/* prep the spec dspace */
      specp->downsrcp = dwnp;               /*  & record its source */
    }
    for (octp=dwnp->octdata; nocts--; octp++) { /* reset all oct params, &  */
      octp->curp = octp->begp;
      memset(octp->feedback, '\0', 6*sizeof(MYFLT));
      octp->scount = 0;
    }
    specp->nfreqs = p->nfreqs;               /* save the spec descriptors */
    specp->dbout = 0;
    specp->ktimstamp = 0;                    /* init specdata to not new  */
    specp->ktimprd = p->timcount;
    p->scountdown = p->timcount;             /* prime the spect countdown */
                                             /* Start specptrk */
    if ((npts = specp->npts) != p->winpts) {        /* if size has changed */
      SPECset(csound, &p->wfund, (int32)npts);       /*   realloc for wfund */
      p->wfund.downsrcp = specp->downsrcp;
      p->fundp = (MYFLT *) p->wfund.auxch.auxp;
      p->winpts = npts;
    }
    if (UNLIKELY(*p->inptls<=FL(0.0))) nptls = 4;
    else nptls = (int32)*p->inptls;
    if (UNLIKELY(nptls > MAXPTL)) {
      return csound->InitError(csound, Str("illegal no of partials"));
    }
    if (UNLIKELY(*p->irolloff<=FL(0.0))) p->rolloff = FL(0.6);
    else p->rolloff = *p->irolloff;
    p->nptls = nptls;        /* number, whether all or odd */
    ptlmax = nptls;
    dstp = p->pdist;
    fnfreqs = (MYFLT)specp->nfreqs;
    for (nn = 1; nn <= ptlmax; nn++)
      *dstp++ = (int32_t) ((LOG((MYFLT) nn) / (MYFLT)LOGTWO) * fnfreqs + FL(0.5));
    if (UNLIKELY((rolloff = p->rolloff) == FL(0.0) ||
                 rolloff == FL(1.0) || nptls == 1)) {
      p->rolloff = FL(0.0);
      weightsum = (MYFLT)nptls;
    }
    else {
      MYFLT *fltp = p->pmult;
      MYFLT octdrop = (FL(1.0) - rolloff) / fnfreqs;
      weightsum = FL(0.0);
      for (dstp = p->pdist, nn = nptls; nn--; ) {
        weight     = FL(1.0) - octdrop * *dstp++; /* rolloff * octdistance */
        weightsum += weight;
        *fltp++    = weight;
      }
      if (UNLIKELY(*--fltp < FL(0.0))) {
        return csound->InitError(csound, Str("per octave rolloff too steep"));
      }
      p->rolloff = 1;
    }
    lobin = (int32)(specp->downsrcp->looct * fnfreqs);
    oct0p = p->fundp - lobin;           /* virtual loc of oct 0 */

    flop = oct0p + (int32_t)(*p->ilo * fnfreqs);
    fhip = oct0p + (int32_t)(*p->ihi * fnfreqs);
    fundp = p->fundp;
    fendp = fundp + specp->npts;
    if (flop < fundp) flop = fundp;
    if (UNLIKELY(fhip > fendp)) fhip = fendp;
    if (UNLIKELY(flop >= fhip)) {                 /* chk hi-lo range valid */
      return csound->InitError(csound, Str("illegal lo-hi values"));
    }
    for (fp = fundp; fp < flop; )
      *fp++ = FL(0.0);                  /* clear unused lo and hi range */
    for (fp = fhip; fp < fendp; )
      *fp++ = FL(0.0);

    dbthresh = *p->idbthresh;           /* thresholds: */
    ampthresh = (MYFLT)exp((double)dbthresh * LOG10D20);
    p->threshon = ampthresh;            /* mag */
    p->threshoff = ampthresh * FL(0.5);
    p->threshon *= weightsum;
    p->threshoff *= weightsum;
    p->oct0p = oct0p;                   /* virtual loc of oct 0 */
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

int32_t pitch(CSOUND *csound, PITCH *p)
{
    MYFLT       *asig;
    double      q;
    double      c1 = p->c1, c2 = p->c2;

    MYFLT   a, b, *dftp, SIG, yt1, yt2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     nocts, winlen;
    DOWNDAT *downp = &p->downsig;
    OCTDAT  *octp;
    SPECDAT *specp;
    MYFLT  c;
    MYFLT  kvar;
                                /* RMS */
    q = p->prvq;
    asig = p->asig;
    if (UNLIKELY(offset)) memset(asig, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&asig[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT as = asig[n]*DFLT_DBFS/csound->e0dbfs; /* Normalise.... */
      q = c1 * as * as + c2 * q;
      SIG = as;                              /* for each source sample: */
      octp = downp->octdata;                /*   align onto top octave */
      nocts = downp->nocts;
      do {                                  /*   then for each oct:    */
        const MYFLT *coefp;
        MYFLT *ytp, *curp;
        int32_t   nfilt;
        curp = octp->curp;
        *curp++ = SIG;                      /*  write samp to cur buf  */
        if (UNLIKELY(curp >= octp->endp))
          curp = octp->begp;                /*    & modulo the pointer */
        octp->curp = curp;
        if (UNLIKELY(!(--nocts)))  break;   /*  if lastoct, break      */
        coefp = bicoefs;  ytp = octp->feedback;
        for (nfilt = 3; nfilt--; ) {        /*  apply triple biquad:   */
          yt2    = *ytp++; yt1 = *ytp--;          /* get prev feedback */
          SIG   -= (*coefp++ * yt1);              /* apply recurs filt */
          SIG   -= (*coefp++ * yt2);
          *ytp++ = yt1; *ytp++ = SIG;             /* stor nxt feedback */
          SIG   *= *coefp++;
          SIG   += (*coefp++ * yt1);              /* apply forwrd filt */
          SIG   += (*coefp++ * yt2);
        }
      } while (!(++octp->scount & 01) && octp++); /* send alt samps to nxtoct */
    }
    p->prvq = q;
    kvar = SQRT((MYFLT)q);       /* End of spectrum part */

    specp = &p->wsig;
    if (LIKELY((--p->scountdown))) goto nxt;  /* if not yet time for new spec  */
    p->scountdown = p->timcount;          /* else reset counter & proceed: */
    downp = &p->downsig;
    nocts = downp->nocts;
    octp = downp->octdata + nocts;
    dftp = (MYFLT *) specp->auxch.auxp;
    winlen = *(p->winlen);
    while (nocts--) {
      MYFLT  *bufp, *sinp, *cosp;
      int32_t    len, *lenp, *offp, nfreqs;
      MYFLT  *begp, *curp, *endp, *linbufp;
      int32_t    len2;
      octp--;                                 /* for each oct (low to high) */
      begp = octp->begp;
      curp = octp->curp;
      endp = octp->endp;
      if (UNLIKELY((len = endp - curp) >= winlen)) /*   if no wrap          */
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
        c = HYPOT(a, b);                      /* get magnitude    */
        *dftp++ = c;                          /* store in out spectrum   */
      }
    }
    specp->ktimstamp = CS_KCNT;      /* time-stamp the output   */

 nxt:
                                /* specptrk */
    {
      MYFLT *inp = (MYFLT *) specp->auxch.auxp;
      MYFLT *endp = inp + specp->npts;
      MYFLT *inp2, sum, *fp;
      int32_t   nn, *pdist, confirms;
      MYFLT kval, fmax, *fmaxp, absdiff, realbin;
      MYFLT *flop, *fhip, *ilop, *ihip, a, b, c, denom, delta;
      int32 lobin, hibin;

      if (UNLIKELY(inp==NULL)) goto err1;            /* RWD fix */
      kval = p->playing == PLAYING ? p->kval : p->kvalsav;
      lobin = (int32)((kval - kvar) * specp->nfreqs);/* set lims of frq interest */
      hibin = (int32)((kval + kvar) * specp->nfreqs);
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
        if (UNLIKELY(*fp > fmax)) {
          fmax = *fp;
          fmaxp = fp;
        }
      if (!p->playing) {
        if (fmax > p->threshon)         /* not playing & threshon? */
          p->playing = STARTING;        /*   prepare to turn on    */
        else goto output;
      }
      else {
        if (fmax < p->threshoff) {      /* playing & threshoff ? */
          if (p->playing == PLAYING)
            p->kvalsav = p->kval;       /*   save val & turn off */
          p->kval = FL(0.0);
          p->kavl = FL(0.0);
          p->kinc = FL(0.0);
          p->kanc = FL(0.0);
          p->playing = 0;
          goto output;
        }
      }
      a = fmaxp>flop ? *(fmaxp-1) : FL(0.0);    /* calc a refined bin no */
      b = fmax;
      c = fmaxp<fhip-1 ? *(fmaxp+1) : FL(0.0);
      if (b < FL(2.0) * (a + c))
        denom = b + b - a - c;
      else denom = a + b + c;
      if (denom != FL(0.0))
        delta = FL(0.5) * (c - a) / denom;
      else delta = FL(0.0);
      realbin = (fmaxp - p->oct0p) + delta;     /* get modified bin number  */
      kval = realbin / specp->nfreqs;           /*     & cvt to true decoct */

      if (p->playing == STARTING) {             /* STARTING mode:           */
        absdiff = FABS(kval - p->kvalsav);// < FL(0.0)) absdiff = -absdiff;
        confirms = (int32_t)(absdiff * p->confact); /* get interval dependency  */
        if (UNLIKELY(p->jmpcount < confirms)) {
          p->jmpcount += 1;               /* if not enough confirms,  */
          goto output;                    /*    must wait some more   */
        } else {
          p->playing = PLAYING;           /* else switch on playing   */
          p->jmpcount = 0;
          p->kval = kval;                 /*    but suppress interp   */
          p->kinc = FL(0.0);
        }
      } else {                                  /* PLAYING mode:            */
        absdiff = FABS(kval - p->kval);
        confirms = (int32_t)(absdiff * p->confact); /* get interval dependency  */
        if (p->jmpcount < confirms) {
          p->jmpcount += 1;               /* if not enough confirms,  */
          p->kinc = FL(0.0);              /*    must wait some more   */
        } else {
          p->jmpcount = 0;                /* else OK to jump interval */
          p->kval = kval;
        }
      }
      fmax += delta * (c - a) * FL(0.25); /* get modified amp */
      p->kavl = fmax;
    }
 output:
    *p->koct = p->kval;                   /* output true decoct & amp */
    *p->kamp = p->kavl * FL(4.0);
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("pitch: not initialised"));
}

/* Multiply and accumulate opcodes */

int32_t macset(CSOUND *csound, SUM *p)
{
    if (UNLIKELY((((int32_t)p->INOCOUNT)&1)==1)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Must have even number of arguments in mac\n"));
    }
    return OK;
}

int32_t maca(CSOUND *csound, SUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    int32_t count=(int32_t) p->INOCOUNT, j;
    MYFLT *ar = p->ar, **args = p->argums;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (k=offset; k<nsmps; k++) {
      MYFLT ans = FL(0.0);
      for (j=0; j<count; j +=2)
        ans += args[j][k] * args[j+1][k];
      ar[k] = ans;
    }
    return OK;
}

int32_t mac(CSOUND *csound, SUM *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t k, nsmps = CS_KSMPS;
    int32_t count=(int32_t) p->INOCOUNT, j;
    MYFLT *ar = p->ar, **args = p->argums;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (k=offset; k<nsmps; k++) {
      MYFLT ans = FL(0.0);
      for (j=0; j<count; j +=2)
        ans += *args[j] * args[j+1][k];
      ar[k] = ans;
    }
    return OK;
}

typedef struct {
    RTCLOCK r;
    double  counters[33];
    int32_t     running[33];
} CPU_CLOCK;

static void initClockStruct(CSOUND *csound, void **p)
{
    *p = csound->QueryGlobalVariable(csound, "readClock::counters");
    if (UNLIKELY(*p == NULL)) {
      csound->CreateGlobalVariable(csound, "readClock::counters",
                                           sizeof(CPU_CLOCK));
      *p = csound->QueryGlobalVariable(csound, "readClock::counters");
      csound->InitTimerStruct(&(((CPU_CLOCK*) (*p))->r));
    }
}

static inline CPU_CLOCK *getClockStruct(CSOUND *csound, void **p)
{
    if (UNLIKELY(*p == NULL))
      initClockStruct(csound, p);
    return (CPU_CLOCK*) (*p);
}

int32_t clockset(CSOUND *csound, CLOCK *p)
{
   IGN(csound);
    p->c = (int32_t)*p->cnt;
    if (UNLIKELY(p->c < 0 || p->c > 31))
      p->c = 32;
    return OK;
}

int32_t clockon(CSOUND *csound, CLOCK *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    if (LIKELY(!clk->running[p->c])) {
      clk->running[p->c] = 1;
      clk->counters[p->c] -= csound->GetCPUTime(&(clk->r));
    }
    return OK;
}

int32_t clockoff(CSOUND *csound, CLOCK *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    if (LIKELY(clk->running[p->c])) {
      clk->running[p->c] = 0;
      clk->counters[p->c] += csound->GetCPUTime(&(clk->r));
    }
    return OK;
}

int32_t clockread(CSOUND *csound, CLKRD *p)
{
    CPU_CLOCK *clk = getClockStruct(csound, &(p->clk));
    int32_t cnt = (int32_t) *p->a;
    if (UNLIKELY(cnt < 0 || cnt > 32)) cnt = 32;
    if (UNLIKELY(clk->running[cnt]))
      return csound->InitError(csound, Str("clockread: clock still running, "
                                           "call clockoff first"));
    /* result in ms */
#ifdef JPFF
    printf("readclock%d: %g\n", cnt, clk->counters[cnt]);
#endif
    *p->r = (MYFLT) (clk->counters[cnt] * 1000.0);
    return OK;
}

int32_t scratchread(CSOUND *csound, SCRATCHPAD *p)
{
    int32_t index = MYFLT2LRND(*p->index);
    if (index<0 || index>3)
      return csound->PerfError(csound, &(p->h),
                               Str("scratchpad index out of range"));
    *p->val = p->h.insdshead->scratchpad[index];
    return OK;
}

int32_t scratchwrite(CSOUND *csound, SCRATCHPAD *p)
{
    int32_t index = MYFLT2LRND(*p->index);
    if (index<0 || index>3)
      return csound->PerfError(csound, &(p->h),
                               Str("scratchpad index out of range"));
    p->h.insdshead->scratchpad[index] = *p->val;
    return OK;
}


/* ************************************************************ */
/* Opcodes from Peter Neubäcker                                 */
/* ************************************************************ */

int32_t adsyntset(CSOUND *csound, ADSYNT *p)
{
    FUNC    *ftp;
    uint32_t     count;
    int32   *lphs;

    p->inerr = 0;

    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: wavetable not found!"));
    }

    count = (uint32_t)*p->icnt;
    if (UNLIKELY(count < 1))
      count = 1;
    p->count = count;

    if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifreqtbl)) != NULL)) {
      p->freqtp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: freqtable not found!"));
    }
    if (UNLIKELY(ftp->flen < count)) {
      p->inerr = 1;
      return csound->InitError(csound, Str(
                    "adsynt: partial count is greater than freqtable size!"));
    }

    if (LIKELY((ftp = csound->FTnp2Find(csound, p->iamptbl)) != NULL)) {
      p->amptp = ftp;
    }
    else {
      p->inerr = 1;
      return csound->InitError(csound, Str("adsynt: amptable not found!"));
    }
    if (UNLIKELY(ftp->flen < count)) {
      p->inerr = 1;
      return csound->InitError(csound, Str(
                    "adsynt: partial count is greater than amptable size!"));
    }

    if (p->lphs.auxp==NULL || p->lphs.size < (size_t)sizeof(int32)*count)
      csound->AuxAlloc(csound, sizeof(int32)*count, &p->lphs);

    lphs = (int32*)p->lphs.auxp;
    if (*p->iphs > 1) {
      do {
        *lphs++ = ((int32) ((MYFLT) ((double) rand_31(csound) / 2147483645.0)
                           * FMAXLEN)) & PHMASK;
      } while (--count);
    }
    else if (*p->iphs >= 0) {
      do {
        *lphs++ = ((int32) (*p->iphs * FMAXLEN)) & PHMASK;
      } while (--count);
    }

    return OK;
}

int32_t adsynt(CSOUND *csound, ADSYNT *p)
{
    FUNC    *ftp, *freqtp, *amptp;
    MYFLT   *ar, *ftbl, *freqtbl, *amptbl;
    MYFLT    amp0, amp, cps0, cps;
    int32    phs, inc, lobits;
    int32   *lphs;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      c, count;

    if (UNLIKELY(p->inerr)) {
      return csound->PerfError(csound, &(p->h),
                               Str("adsynt: not initialised"));
    }
    ftp = p->ftp;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    freqtp = p->freqtp;
    freqtbl = freqtp->ftable;
    amptp = p->amptp;
    amptbl = amptp->ftable;
    lphs = (int32*)p->lphs.auxp;

    cps0 = *p->kcps;
    amp0 = *p->kamp;
    count = p->count;

    ar = p->sr;
    memset(ar, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    for (c=0; c<count; c++) {
      amp = amptbl[c] * amp0;
      cps = freqtbl[c] * cps0;
      inc = (int32) (cps * csound->sicvt);
      phs = lphs[c];
      for (n=offset; n<nsmps; n++) {
        ar[n] += *(ftbl + (phs >> lobits)) * amp;
        phs += inc;
        phs &= PHMASK;
      }
      lphs[c] = phs;
    }
    return OK;
}

int32_t hsboscset(CSOUND *csound, HSBOSC *p)
{
    FUNC        *ftp;
    int32_t         octcnt, i;

    if (LIKELY((ftp = csound->FTnp2Finde(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (UNLIKELY(*p->ioctcnt < 2))
        octcnt = 3;
      else
        octcnt = (int32_t)*p->ioctcnt;
      if (UNLIKELY(octcnt > 10))
        octcnt = 10;
      p->octcnt = octcnt;
      if (*p->iphs >= 0) {
        for (i=0; i<octcnt; i++)
          p->lphs[i] = ((int32)(*p->iphs * FMAXLEN)) & PHMASK;
      }
    }
    else p->ftp = NULL;
    if (LIKELY((ftp = csound->FTnp2Finde(csound, p->imixtbl)) != NULL)) {
      p->mixtp = ftp;
    }
    else p->mixtp = NULL;
    return OK;
}

int32_t hsboscil(CSOUND *csound, HSBOSC   *p)
{
    FUNC        *ftp, *mixtp;
    MYFLT       fract, v1, amp0, amp, *ar, *ftab, *mtab;
    int32       phs, inc, lobits;
    int32       phases[10];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       tonal, bright, freq, ampscl;
    int32_t         octcnt = p->octcnt;
    MYFLT       octstart, octoffs, octbase;
    int32_t         octshift, i, mtablen;
    MYFLT       hesr = CS_ESR / FL(2.0);

    ftp = p->ftp;
    mixtp = p->mixtp;
    if (UNLIKELY(ftp==NULL || mixtp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("hsboscil: not initialised"));
    }

    tonal = *p->ktona;
    tonal -= MYFLOOR(tonal);
    bright = *p->kbrite - tonal;
    octstart = bright - (MYFLT)octcnt * FL(0.5);
    octbase = MYFLOOR(MYFLOOR(octstart) + FL(1.5));
    octoffs = octbase - octstart;

    mtab = mixtp->ftable;
    mtablen = mixtp->flen;
    freq = *p->ibasef * POWER(FL(2.0), tonal + octbase);

    ampscl = mtab[(int32_t)((1.0 / (MYFLT)octcnt) * mtablen)];
    amp = mtab[(int32_t)((octoffs / (MYFLT)octcnt) * mtablen)];
    if ((amp - p->prevamp) > (ampscl * FL(0.5)))
      octshift = 1;
    else if ((amp - p->prevamp) < (-(ampscl * FL(0.5))))
      octshift = -1;
    else
      octshift = 0;
    p->prevamp = amp;

    ampscl = FL(0.0);
    for (i=0; i<octcnt; i++) {
      phases[i] = p->lphs[(i+octshift+100*octcnt) % octcnt];
      ampscl += mtab[(int32_t)(((MYFLT)i / (MYFLT)octcnt) * mtablen)];
    }

    amp0 = *p->kamp / ampscl;
    lobits = ftp->lobits;
    ar = p->sr;
    memset(ar, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    for (i=0; i<octcnt; i++) {
      phs = phases[i];
      amp = mtab[(int32_t)((octoffs / (MYFLT)octcnt) * mtablen)] * amp0;
      if (UNLIKELY(freq > hesr))
        amp = FL(0.0);
      inc = (int32)(freq * csound->sicvt);
      for (n=offset; n<nsmps; n++) {
        fract = PFRAC(phs);
        ftab = ftp->ftable + (phs >> lobits);
        v1 = *ftab++;
        ar[n] += (v1 + (*ftab - v1) * fract) * amp;
        phs += inc;
        phs &= PHMASK;
      }
      p->lphs[i] = phs;

      octoffs += FL(1.0);
      freq *= FL(2.0);
    }
    return OK;
}

int32_t pitchamdfset(CSOUND *csound, PITCHAMDF *p)
{
    MYFLT srate, downs;
    int32  size, minperi, maxperi, downsamp, upsamp, msize, bufsize;
    uint32_t interval;
    uint32_t nsmps = CS_KSMPS;

    p->inerr = 0;

    downs = *p->idowns;
    if (downs < (-FL(1.9))) {
      upsamp = (int32_t)MYFLT2LONG((-downs));
      downsamp = 0;
      srate = CS_ESR * (MYFLT)upsamp;
    }
    else {
      downsamp = (int32_t)MYFLT2LONG(downs);
      if (UNLIKELY(downsamp < 1))
        downsamp = 1;
      srate = CS_ESR / (MYFLT)downsamp;
      upsamp = 0;
    }

    minperi = (int32)(srate / *p->imaxcps);
    maxperi = (int32)(FL(0.5)+srate / *p->imincps);
    if (UNLIKELY(maxperi <= minperi)) {
      p->inerr = 1;
      return csound->InitError(csound,
                               Str("pitchamdf: maxcps must be > mincps !"));
    }

    if (*p->iexcps < 1)
        interval = maxperi;
    else
        interval = (uint32_t)(srate / *p->iexcps);
    if (interval < nsmps) {
      if (downsamp)
        interval = nsmps / downsamp;
      else
        interval = nsmps * upsamp;
    }

    size = maxperi + interval;
    bufsize = sizeof(MYFLT)*(size + interval + 2);

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
        p->peri = (int32_t)(srate / *p->icps);

    if (*p->irmsmedi < 1)
        p->rmsmedisize = 0;
    else
      p->rmsmedisize = ((int32_t)MYFLT2LONG(*p->irmsmedi))*2+1;
    p->rmsmediptr = 0;

    if (p->rmsmedisize) {
      msize = p->rmsmedisize * 3 * sizeof(MYFLT);
      if (p->rmsmedian.auxp==NULL || p->rmsmedian.size < (size_t)msize)
        csound->AuxAlloc(csound, msize, &p->rmsmedian);
      else {
        memset(p->rmsmedian.auxp, 0, msize);
      }
    }

    if (*p->imedi < 1)
      p->medisize = 0;
    else
      p->medisize = (int32_t)MYFLT2LONG(*p->imedi)*2+1;
    p->mediptr = 0;

    if (p->medisize) {
      msize = p->medisize * 3 * sizeof(MYFLT);
      if (p->median.auxp==NULL || p->median.size < (size_t)msize)
        csound->AuxAlloc(csound, (size_t)msize, &p->median);
      else {
        memset(p->median.auxp, 0, msize);
      }
    }

    if (p->buffer.auxp==NULL || p->buffer.size < (size_t)bufsize) {
      csound->AuxAlloc(csound, bufsize, &p->buffer);
    }
    else
      memset(p->buffer.auxp, 0, bufsize);
    return OK;
}

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp

MYFLT medianvalue(uint32 n, MYFLT *vals)
{   /* vals must point to 1 below relevant data! */
    uint32 i, ir, j, l, mid;
    uint32 k = (n + 1) / 2;
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
          if (UNLIKELY(j < i)) break;
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

int32_t pitchamdf(CSOUND *csound, PITCHAMDF *p)
{
    MYFLT *buffer = (MYFLT*)p->buffer.auxp;
    MYFLT *rmsmedian = (MYFLT*)p->rmsmedian.auxp;
    int32  rmsmedisize = p->rmsmedisize;
    int32  rmsmediptr = p->rmsmediptr;
    MYFLT *median = (MYFLT*)p->median.auxp;
    int32  medisize = p->medisize;
    int32  mediptr = p->mediptr;
    int32  size = p->size;
    int32  index = p->index;
    int32  minperi = p->minperi;
    int32  maxperi = p->maxperi;
    MYFLT *asig = p->asig;
    MYFLT  srate = p->srate;
    int32  peri = p->peri;
    int32  upsamp = p->upsamp;
    MYFLT  upsmp = (MYFLT)upsamp;
    MYFLT  lastval = p->lastval;
    MYFLT  newval, delta;
    int32  readp = p->readp;
    int32  interval = size - maxperi;
    int32_t    nsmps = CS_KSMPS;
    int32_t    i;
    int32  i1, i2;
    MYFLT  val, rms;
    double sum;
    MYFLT  acc, accmin, diff;

    if (UNLIKELY(p->inerr)) {
      return csound->PerfError(csound, &(p->h),
                               Str("pitchamdf: not initialised"));
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
              if (diff > 0) accmin += diff;
              else          accmin -= diff;
            }
            for (i1 = minperi + 1; i1 <= maxperi; ++i1) {
              acc = FL(0.0);
              for (i2 = 0; i2 < size; ++i2) {
                diff = buffer[i1+i2] - buffer[i2];
                if (diff > 0) acc += diff;
                else          acc -= diff;
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
              peri = (int32)median[medisize*2 +
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
      int32  downsamp = p->downsamp;
      while (1) {
        buffer[index++] = asig[readp];
        readp += downsamp;

        if (index == size) {
          peri = minperi;
          accmin = FL(0.0);
          for (i2 = 0; i2 < size; ++i2) {
            diff = buffer[i2+minperi] - buffer[i2];
            if (diff > FL(0.0)) accmin += diff;
            else                accmin -= diff;
          }
          for (i1 = minperi + 1; i1 <= maxperi; ++i1) {
            acc = FL(0.0);
            for (i2 = 0; i2 < size; ++i2) {
              diff = buffer[i1+i2] - buffer[i2];
              if (diff > FL(0.0)) acc += diff;
              else                acc -= diff;
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
            peri = (int32)median[medisize*2 +
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
      val = buffer[i1];
      sum += (double)(val * val);
    }
    if (UNLIKELY(peri==0))      /* How xould thus happen??? */
      rms = FL(0.0);
    else
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

    if (UNLIKELY(peri==0)) {
      *p->kcps = FL(0.0);
    }
    else
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

int32_t phsbnkset(CSOUND *csound, PHSORBNK *p)
{
    double  phs;
    int32_t    n, count;
    double  *curphs;

    count = (int32_t)MYFLT2LONG(*p->icnt);
    if (UNLIKELY(count < 2))
      count = 2;

    if (p->curphs.auxp==NULL || p->curphs.size < (size_t)sizeof(double)*count)
      csound->AuxAlloc(csound, (size_t)sizeof(double)*count, &p->curphs);

    curphs = (double*)p->curphs.auxp;
    if (*p->iphs > 1) {
      for (n=0; n<count;n++)
        curphs[n] = (double) rand_31(csound) / 2147483645.0;
    }
    else if ((phs = *p->iphs) >= 0) {
      for (n=0; n<count;n++) curphs[n] = phs;
    }
    return OK;
}

int32_t kphsorbnk(CSOUND *csound, PHSORBNK *p)
{
    double  phs;
    double  *curphs = (double*)p->curphs.auxp;
    int32_t     size = p->curphs.size / sizeof(double);
    int32_t     index = (int32_t)(*p->kindx);

    if (UNLIKELY(curphs == NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("phasorbnk: not initialised"));
    }

    if (UNLIKELY(index<0 || index>=size)) {
      *p->sr = FL(0.0);
      return NOTOK;
    }

    *p->sr = (MYFLT)(phs = curphs[index]);
    if (UNLIKELY((phs += *p->xcps * csound->onedkr) >= 1.0))
      phs -= 1.0;
    else if (UNLIKELY(phs < 0.0)) /* patch from Matthew Scala */
      phs += 1.0;
    curphs[index] = phs;
    return OK;
}

int32_t phsorbnk(CSOUND *csound, PHSORBNK *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *rs;
    double  phase, incr;
    double  *curphs = (double*)p->curphs.auxp;
    int32_t     size = p->curphs.size / sizeof(double);
    int32_t     index = (int32_t)(*p->kindx);

    if (UNLIKELY(curphs == NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("phasorbnk: not initialised"));
    }

    if (UNLIKELY(index<0 || index>=size)) {
      *p->sr = FL(0.0);
      return NOTOK;
    }

    rs = p->sr;
    phase = curphs[index];
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (IS_ASIG_ARG(p->xcps)) {
      MYFLT *cps = p->xcps;
      for (n=offset; n<nsmps; n++) {
        incr = (double)(cps[n] * csound->onedsr);
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (UNLIKELY(phase >= 1.0))
          phase -= 1.0;
        else if (UNLIKELY(phase < 0.0))
          phase += 1.0;
      }
    }
    else {
      incr = (double)(*p->xcps * csound->onedsr);
      for (n=offset; n<nsmps; n++) {
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (UNLIKELY(phase >= 1.0))
          phase -= 1.0;
        else if (UNLIKELY(phase < 0.0))
          phase += 1.0;
      }
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

int32_t GardnerPink_init(CSOUND *csound, PINKISH *p);
int32_t GardnerPink_perf(CSOUND *csound, PINKISH *p);

int32_t pinkset(CSOUND *csound, PINKISH *p)
{
        /* Check valid method */
    if (UNLIKELY(*p->imethod != GARDNER_PINK && *p->imethod != KELLET_PINK
                 && *p->imethod != KELLET_CHEAP_PINK)) {
      return csound->InitError(csound, Str("pinkish: Invalid method code"));
    }
    /* User range scaling can be a- or k-rate for Gardner, a-rate only
       for filter */
    if (IS_ASIG_ARG(p->xin)) {
      p->ampinc = 1;
    }
    else {
      /* Cannot accept k-rate input with filter method */
      if (UNLIKELY(*p->imethod != FL(0.0))) {
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

int32_t pinkish(CSOUND *csound, PINKISH *p)
{
    MYFLT       *aout, *ain;
    double      c0, c1, c2, c3, c4, c5, c6, nxtin, nxtout;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    aout = p->aout;
    ain = p->xin;
    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (*p->imethod == GARDNER_PINK) {  /* Gardner method (default) */
      GardnerPink_perf(csound,p);
    }
    else if (*p->imethod == KELLET_PINK) {
      /* Paul Kellet's "refined" pink filter */
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;
      c3 = p->b3; c4 = p->b4; c5 = p->b5; c6 = p->b6;
      for (n=offset;n<nsmps;n++) {
        nxtin = (double)ain[n];
        c0 = c0 * 0.99886 + nxtin * 0.0555179;
        c1 = c1 * 0.99332 + nxtin * 0.0750759;
        c2 = c2 * 0.96900 + nxtin * 0.1538520;
        c3 = c3 * 0.86650 + nxtin * 0.3104856;
        c4 = c4 * 0.55000 + nxtin * 0.5329522;
        c5 = c5 * -0.7616 - nxtin * 0.0168980;
        nxtout = c0 + c1 + c2 + c3 + c4 + c5 + c6 + nxtin * 0.5362;
        aout[n] = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
        c6 = nxtin * 0.115926;
      }
      /* Store back filter coef states */
      p->b0 = c0; p->b1 = c1; p->b2 = c2;
      p->b3 = c3; p->b4 = c4; p->b5 = c5; p->b6 = c6;
    }
    else if (*p->imethod == KELLET_CHEAP_PINK) {
      /* Get filter states */
      c0 = p->b0; c1 = p->b1; c2 = p->b2;

      for (n=offset;n<nsmps;n++) {      /* Paul Kellet's "economy" pink filter */
        nxtin = (double)ain[n];
        c0 = c0 * 0.99765 + nxtin * 0.0990460;
        c1 = c1 * 0.96300 + nxtin * 0.2965164;
        c2 = c2 * 0.57000 + nxtin * 1.0526913;
        nxtout = c0 + c1 + c2 + nxtin * 0.1848;
        aout[n] = (MYFLT)(nxtout * 0.11);       /* (roughly) compensate for gain */
      }

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
#define PINK_RANDOM_SHIFT      (7)

/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static int32 GenerateRandomNumber(uint32 randSeed)
{
    randSeed = ((uint32_t) randSeed * 196314165U) + 907633515UL;
    return (int32) ((int32_t) ((uint32_t) randSeed));
}

/************************************************************/

/* Set up for user-selected number of bands of noise generators. */
int32_t GardnerPink_init(CSOUND *csound, PINKISH *p)
{
    int32_t i;
    MYFLT pmax;
    int32 numRows;

    /* Set number of rows to use (default to 20) */
    if (*p->iparam1 >= 4 && *p->iparam1 <= GRD_MAX_RANDOM_ROWS)
      p->grd_NumRows = (int32)*p->iparam1;
    else {
      p->grd_NumRows = 20;
      /* Warn if user tried but failed to give sensible number */
      if (UNLIKELY(*p->iparam1 != FL(0.0)))
        csound->Warning(csound, Str("pinkish: Gardner method requires 4-%d bands. "
                                    "Default %"PRIi32" substituted for %d.\n"),
                        GRD_MAX_RANDOM_ROWS, p->grd_NumRows,
                        (int32_t) *p->iparam1);
    }

    /* Seed random generator by user value or by time (default) */
    if (*p->iseed != FL(0.0)) {
      if (*p->iseed > -1.0 && *p->iseed < 1.0)
        p->randSeed = (uint32) (*p->iseed * (MYFLT)0x80000000);
      else p->randSeed = (uint32) *p->iseed;
    }
    else p->randSeed = (uint32) csound->GetRandomSeedFromTime();

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
      int32 randSeed, newRandom, runningSum = 0;
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
int32_t GardnerPink_perf(CSOUND *csound, PINKISH *p)
{
    IGN(csound);
    MYFLT *aout, *amp, scalar;
    int32 *rows, rowIndex, indexMask, randSeed, newRandom;
    int32 runningSum, sum, ampinc;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS - p->h.insdshead->ksmps_no_end;

    aout        = p->aout;
    amp         = p->xin;
    ampinc      = p->ampinc;    /* Used to increment user amp if a-rate */
    scalar      = p->grd_Scalar;
    rowIndex    = p->grd_Index;
    indexMask   = p->grd_IndexMask;
    runningSum  = p->grd_RunningSum;
    rows        = &(p->grd_Rows[0]);
    randSeed    = p->randSeed;

    for (n=offset; n<nsmps;n++) {
      /* Increment and mask index. */
      rowIndex = (rowIndex + 1) & indexMask;

      /* If index is zero, do not update any random values. */
      if ( rowIndex != 0 ) {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int32_t numZeros = 0;
        int32_t n = rowIndex;
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
      aout[n] = *amp * sum * scalar;
      amp += ampinc;            /* Increment if amp is a-rate */
    }

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
int32_t clip_set(CSOUND *csound, CLIP *p)
{
    IGN(csound);
    int32_t meth = (int32_t)MYFLT2LONG(*p->imethod);
    p->meth = meth;
    p->arg = FABS(*p->iarg);
    p->lim = *p->limit;
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
      p->k1 = FL(1.0)/TANH(FL(1.0));
      break;
    default:
      p->meth = 0;
    }
    return OK;
}

int32_t clip(CSOUND *csound, CLIP *p)
{
    IGN(csound);
    MYFLT *aout = p->aout, *ain = p->ain;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT a = p->arg, k1 = p->k1, k2 = p->k2;
    MYFLT limit = p->lim;
    MYFLT rlim = FL(1.0)/limit;

    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->meth) {
    case 0:                     /* Soft clip with division */
      for (n=offset;n<nsmps;n++) {
        MYFLT x = ain[n];
        if (x>=FL(0.0)) {
          if (UNLIKELY(x>limit)) x = k2;
          else if (x>a)
            x = a + (x-a)/(FL(1.0)+(x-a)*(x-a)*k1);
        }
        else {
          if (UNLIKELY(x<-limit))
            x = -k2;
          else if (-x>a)
            x = -a + (x+a)/(FL(1.0)+(x+a)*(x+a)*k1);
        }
        aout[n] = x;
      }
      return OK;
    case 1:
      for (n=offset;n<nsmps;n++) {
        MYFLT x = ain[n];
        if (UNLIKELY(x>=limit))
            x = limit;
        else if (UNLIKELY(x<= -limit))
          x = -limit;
        else
            x = limit*SIN(k1*x);
        aout[n] = x;
      }
      return OK;
    case 2:
      for (n=offset;n<nsmps;n++) {
        MYFLT x = ain[n];
        if (UNLIKELY(x>=limit))
          x = limit;
        else if (UNLIKELY(x<= -limit))
          x = -limit;
        else
          x = limit*k1*TANH(x*rlim);
        //printf("*** %g -> %g\n", ain[n], x);
        aout[n] = x;
      }
      return OK;
    }
    return OK;
}

/* ********************************************************************** */
/* *************** IMPULSE ********************************************** */
/* ********************************************************************** */

int32_t impulse_set(CSOUND *csound, IMPULSE *p)
{
    p->next = (uint32_t)MYFLT2LONG(*p->offset * CS_ESR);
    return OK;
}

int32_t impulse(CSOUND *csound, IMPULSE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t next = p->next;
    MYFLT *ar = p->ar;
    if (next<0) next = -next;
    if (UNLIKELY(next < (int32)nsmps)) { /* Impulse in this frame */
      MYFLT frq = *p->freq;     /* Freq at k-rate */
      int32_t sfreq;                /* Converted to samples */
      if (frq == FL(0.0)) sfreq = INT_MAX; /* Zero means infinite */
      else if (frq < FL(0.0)) sfreq = -(int32_t)frq; /* Negative cnts in sample */
      else sfreq = (int32_t)(frq*CS_ESR); /* Normal case */
      if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset;n<nsmps;n++) {
        if (UNLIKELY(next-- == 0)) {
          ar[n] = *p->amp;
          next = sfreq - 1;     /* Note can be less than k-rate */
        }
        else ar[n] = FL(0.0);
      }
    }
    else {                      /* Nothing this time so just fill */
      memset(ar, 0, nsmps*sizeof(MYFLT));
      next -= nsmps;
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
int32_t trnset(CSOUND *csound, TRANSEG *p)
{
    NSEG        *segp;
    int32_t         nsegs;
    MYFLT       **argp, val;

    if (UNLIKELY(p->INOCOUNT%3!=1))
      return csound->InitError(csound, Str("Incorrect argument count in transeg"));
    nsegs = p->INOCOUNT / 3;            /* count segs & alloc if nec */
    if ((segp = (NSEG *) p->auxch.auxp) == NULL ||
        (uint32_t)p->auxch.size < nsegs*sizeof(NSEG)) {
      csound->AuxAlloc(csound, (int32)nsegs*sizeof(NSEG), &p->auxch);
      p->cursegp = segp = (NSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS;       /* set endcount for safety */
    segp[nsegs-1].acnt = MAXPOS;       /* set endcount for safety */
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
      MYFLT d = dur * CS_ESR;
      if ((segp->acnt = segp->cnt = (int32)MYFLT2LONG(d)) < 0)
        segp->cnt = 0;
      else
        segp->cnt = (int32)(dur * CS_EKR);
      segp->nxtpt = nxtval;
      segp->val = val;
      if (alpha == FL(0.0)) {
        segp->c1 = (nxtval-val)/d;
      }
      else {
        segp->c1 = (nxtval - val)/(FL(1.0) - EXP(alpha));
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

int32_t trnset_bkpt(CSOUND *csound, TRANSEG *p)
{
    NSEG        *segp;
    int32_t         nsegs;
    MYFLT       **argp, val;
    MYFLT       totdur = FL(0.0);

    if (UNLIKELY(p->INOCOUNT%3!=1))
      return csound->InitError(csound, Str("Incorrect argument count in transegb"));
    nsegs = p->INOCOUNT / 3;            /* count segs & alloc if nec */
    if ((segp = (NSEG *) p->auxch.auxp) == NULL ||
        (uint32_t)p->auxch.size < nsegs*sizeof(NSEG)) {
      csound->AuxAlloc(csound, (int32)nsegs*sizeof(NSEG), &p->auxch);
      p->cursegp = segp = (NSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS;       /* set endcount for safety */
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
      MYFLT d;
      dur -= totdur;
      totdur += dur;
      d = dur * CS_ESR;
      if ((segp->cnt = (int32)MYFLT2LONG(d)) < 0)
        segp->cnt = 0;
      else
        segp->cnt = (int32)(dur * CS_EKR);
      segp->nxtpt = nxtval;
      segp->val = val;
      if (alpha == FL(0.0)) {
        segp->c1 = (nxtval-val)/d;
      }
      else {
        segp->c1 = (nxtval - val)/(FL(1.0) - EXP(alpha));
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

int32_t ktrnseg(CSOUND *csound, TRANSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (UNLIKELY(p->auxch.auxp==NULL)) { /* RWD fix */
      csound->PerfError(csound,&(p->h),
                        Str("Error: transeg not initialised (krate)\n"));
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
      p->curx += (MYFLT)CS_KSMPS*p->alpha;
      if (p->alpha == FL(0.0))
        p->curval += p->curinc*CS_KSMPS;   /* advance the cur val  */
      else
        p->curval = p->cursegp->val + p->curinc *
          (FL(1.0) - EXP(p->curx));
    }
    return OK;
}

int32_t trnseg(CSOUND *csound, TRANSEG *p)
{
    MYFLT  val, *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    NSEG        *segp = p->cursegp;
    if (UNLIKELY(p->auxch.auxp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("transeg: not initialised (arate)\n"));
    }
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
   val = p->curval;                      /* sav the cur value    */
   for (n=offset; n<nsmps; n++) {
    if (p->segsrem) {                     /* if no more segs putk */
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        segp = p->cursegp;
      chk1:
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          val = p->curval = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        p->cursegp = ++segp;              /*   else find the next */
        if (!(p->curcnt = segp->acnt)) {
          val = p->curval = segp->nxtpt;  /*   nonlen = discontin */
          goto chk1;
        }                                 /*   poslen = new slope */
        p->curinc = segp->c1;
        p->alpha = segp->alpha;
        p->curx = FL(0.0);
        p->curval = val;
      }
      if (p->alpha == FL(0.0)) {
          rs[n] = val;
          val += p->curinc;
        }
      else {
          rs[n] = val;
          p->curx += p->alpha;
          val = segp->val + p->curinc *
            (FL(1.0) - EXP(p->curx));
       }
    }
    else{
    putk:
        rs[n] = val;
    }
   }
    p->curval = val;
    return OK;
}

/* MIDI aware version of transeg */
int32_t trnsetr(CSOUND *csound, TRANSEG *p)
{
    int32_t         relestim;
    NSEG        *segp;
    int32_t         nsegs;
    MYFLT       **argp;
    double      val;

    if (UNLIKELY(p->INOCOUNT%3!=1))
      return csound->InitError(csound, Str("Incorrect argument count in transegr"));
    nsegs = p->INOCOUNT / 3;            /* count segs & alloc if nec */
    if ((segp = (NSEG *) p->auxch.auxp) == NULL ||
        (uint32_t)p->auxch.size < nsegs*sizeof(NSEG)) {
      csound->AuxAlloc(csound, (int32)nsegs*sizeof(NSEG), &p->auxch);
      p->cursegp = segp = (NSEG *) p->auxch.auxp;
    }
    segp[nsegs-1].cnt = MAXPOS;       /* set endcount for safety */
    segp[nsegs-1].acnt = MAXPOS;       /* set endcount for safety */
    argp = p->argums;
    val = (double)**argp++;
    if (UNLIKELY(**argp <= FL(0.0))) return OK; /* if idur1 <= 0, skip init  */
    p->curval = val;
    p->curcnt = 0;
    p->cursegp = segp - 1;            /* else setup null seg0 */
    p->segsrem = nsegs + 1;
    p->curx = FL(0.0);
    do {                              /* init each seg ..  */
      double dur = (double)**argp++;
      MYFLT alpha = **argp++;
      MYFLT nxtval = **argp++;
      MYFLT d = dur * CS_ESR;
      if ((segp->acnt = segp->cnt = (int32)(d + FL(0.5))) < 0)
        segp->cnt = 0;
      else
        segp->cnt = (int32)(dur * CS_EKR);
      segp->nxtpt = nxtval;
      segp->val = val;
      if (alpha == FL(0.0)) {
        segp->c1 = (nxtval-val)/d;
        //printf("alpha zero val=%f c1=%f\n", segp->val, segp->c1);
      }
      else {
        p->lastalpha = alpha;
        segp->c1 = (nxtval - val)/(FL(1.0) - EXP(alpha));
      }
      segp->alpha = alpha/d;
      val = nxtval;
      segp++;
      p->finalval = nxtval;
    } while (--nsegs);
    //p->xtra = -1;
    p->alpha = ((NSEG*)p->auxch.auxp)[0].alpha;
    p->curinc = ((NSEG*)p->auxch.auxp)[0].c1;
    relestim = (int32_t)(p->cursegp + p->segsrem - 1)->cnt;
    p->xtra = relestim;
    if (relestim > p->h.insdshead->xtratim)
      p->h.insdshead->xtratim = (int32_t)relestim;
    /* {  */
    /*   int32_t i; */
    /*   int32_t nseg = p->INOCOUNT / 3; */
    /*   NSEG *segp = p->cursegp; */
    /*   for (i=0; i<nseg; i++) */
    /*     printf("cnt=%d alpha=%f val=%f nxtpt=%f c1=%f\n", */
    /*      segp[i].cnt, segp[i].alpha, segp[i].val, segp[i].nxtpt, segp[i].c1); */
    /* } */
    return OK;
}

int32_t ktrnsegr(CSOUND *csound, TRANSEG *p)
{
    *p->rslt = p->curval;               /* put the cur value    */
    if (UNLIKELY(p->auxch.auxp==NULL)) { /* RWD fix */
      csound->PerfError(csound,&(p->h),
                        Str("Error: transeg not initialised (krate)\n"));
    }
    if (p->segsrem) {                   /* done if no more segs */
      NSEG        *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        //printf("releasing\n");
        while (p->segsrem > 1) {        /* reles flag new:      */
          segp = ++p->cursegp;          /*   go to last segment */
          p->segsrem--;
        }                               /*   get univ relestim  */
        segp->cnt = p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim;
        if (segp->alpha == FL(0.0)) {
          segp->c1 = (p->finalval-p->curval)/(segp->cnt*CS_KSMPS);
          //printf("finalval = %f curval = %f, cnt = %d c1 = %f\n",
          //       p->finalval, p->curval, segp->cnt, segp->c1);
        }
        else {
          segp->c1 = (p->finalval - p->curval)/(FL(1.0) - EXP(p->lastalpha));
          segp->alpha = p->lastalpha/(segp->cnt*CS_KSMPS);
          segp->val = p->curval;
        }
        goto newm;                      /*   and set new curmlt */
      }
      if (--p->curcnt <= 0) {           /* if done cur segment  */
      chk1:
          if (p->segsrem == 2) return OK;    /*   seg Y rpts lastval */
          if (!(--p->segsrem)) return OK;    /*   seg Z now done all */
        segp = ++p->cursegp;            /*   find the next      */
      newm:
        //printf("curcnt = %d seg/cnt = %d\n", p->curcnt, segp->cnt);
        if (!(p->curcnt = segp->cnt)) { /*   nonlen = discontin */
          p->curval = segp->nxtpt;      /*   poslen = new slope */
          //printf("curval = %f\n", p->curval);
          goto chk1;
        }
        p->curinc = segp->c1;
        p->alpha = segp->alpha;
        p->curx = FL(0.0);
      }
      if (p->alpha == FL(0.0)) {
        p->curval += p->curinc *CS_KSMPS;   /* advance the cur val  */
        //printf("curval = %f\n", p->curval);
      }
      else
        p->curval = p->cursegp->val + (p->curinc) *
          (FL(1.0) - EXP(p->curx));
      p->curx +=  (MYFLT)CS_KSMPS* p->alpha;
    }
    return OK;
}

int32_t trnsegr(CSOUND *csound, TRANSEG *p)
{
    MYFLT  val, *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    if (UNLIKELY(p->auxch.auxp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("transeg: not initialised (arate)\n"));
    }
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    val = p->curval;                      /* sav the cur value    */
   for (n=offset; n<nsmps; n++) {
    if (LIKELY(p->segsrem)) {             /* if no more segs putk */
      NSEG  *segp;
      if (p->h.insdshead->relesing && p->segsrem > 1) {
        while (p->segsrem > 1) {          /* if release flag new  */
          segp = ++p->cursegp;            /*   go to last segment */
          p->segsrem--;
        }                                 /*   get univ relestim  */
        segp->cnt = p->xtra>=0 ? p->xtra : p->h.insdshead->xtratim;
        if (segp->alpha == FL(0.0)) {
          segp->c1 = (p->finalval-val)/segp->acnt;
        }
        else {
          /* this is very wrong */
          segp->c1 = (p->finalval - val)/(FL(1.0) - EXP(p->lastalpha));
          segp->alpha = p->lastalpha/segp->acnt;
          segp->val = val;
        }
        goto newm;                        /*   and set new curmlt */
      }
      if (--p->curcnt <= 0) {             /*  if done cur segment */
        //segp = p->cursegp;              /* overwritten later -- coverity */
      chk1:
        if (p->segsrem == 2) goto putk;     /*   seg Y rpts lastval */
        if (UNLIKELY(!--p->segsrem)) {    /*   if none left       */
          //val = p->curval = segp->nxtpt;
          goto putk;                      /*      put endval      */
        }
        segp = ++p->cursegp;              /*   else find the next */
      newm:
       if (!(p->curcnt = segp->acnt)) {
          val = p->curval = segp->nxtpt;  /*   nonlen = discontin */
          goto chk1;
        }                                 /*   poslen = new slope */
        p->curinc = segp->c1;
        p->alpha = segp->alpha;
        p->curx = FL(0.0);
        p->curval = val;
      }
      if (p->alpha == FL(0.0)) {
          rs[n] = val;
          val += p->curinc;
      }
      else {
        segp = p->cursegp;
          rs[n] = val;
          p->curx += p->alpha;
          val = segp->val + p->curinc * (FL(1.0) - EXP(p->curx));
      }
    }
    else {
    putk:
        rs[n] = val;
    }
   }
   p->curval = val;

    return OK;
}

extern int32 randint31(int32);

int32_t varicolset(CSOUND *csound, VARI *p)
{
   IGN(csound);
    p->last = FL(0.0);
    p->lastbeta = *p->beta;
    p->sq1mb2 = SQRT(FL(1.0)-p->lastbeta * p->lastbeta);
    p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    p->ampinc = IS_ASIG_ARG(p->kamp) ? 1 : 0;
    return OK;
}

int32_t varicol(CSOUND *csound, VARI *p)
{
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       beta = *p->beta;
    MYFLT       sq1mb2 = p->sq1mb2;
    MYFLT       lastx = p->last;
    MYFLT       ampmod = p->ampmod;
    MYFLT       *kamp = p->kamp;
    int32_t         ampinc = p->ampinc;
    MYFLT       *rslt = p->rslt;

    if (beta != p->lastbeta) {
       beta = p->lastbeta = *p->beta;
       sq1mb2 = p->sq1mb2 =  SQRT(FL(1.0)-p->lastbeta * p->lastbeta);
       ampmod = p->ampmod = FL(0.785)/(FL(1.0)+p->lastbeta);
    }

    if (UNLIKELY(offset)) memset(rslt, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT rnd = FL(2.0) * (MYFLT) rand_31(csound) / FL(2147483645) - FL(1.0);
      lastx = lastx * beta + sq1mb2 * rnd;
      rslt[n] = lastx * *kamp * ampmod;
      kamp += ampinc;
    }
    p->last = lastx;
    return OK;
}

/* ************************************************************************ */
/* ***** Josep Comajuncosas' 18dB/oct resonant 3-pole LPF with tanh dist ** */
/* ***** Coded in C by John ffitch, 2000 Dec 17 *************************** */
/* ************************************************************************ */
#include <math.h>

/* This code is transcribed from a Csound macro, so no real comments */

int32_t lpf18set(CSOUND *csound, LPF18 *p)
{
    IGN(csound);
    /* Initialise delay lines */
    if (*p->istor==FL(0.0)) {
        p->ay1 = FL(0.0);
        p->ay2 = FL(0.0);
        p->aout = FL(0.0);
        p->lastin = FL(0.0);
    }
    return OK;
}

int32_t lpf18db(CSOUND *csound, LPF18 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT ay1 = p->ay1;
    MYFLT ay2 = p->ay2;
    MYFLT aout = p->aout;
    MYFLT *ain = p->ain;
    MYFLT *ar = p->ar;
    MYFLT lastin = p->lastin;
    double value = 0.0;
    int32_t   flag = 1;
    MYFLT lfc=0, lrs=0, kres=0, kfcn=0, kp=0, kp1=0,  kp1h=0;
    double lds = 0.0;
    MYFLT zerodb = csound->e0dbfs;
    int32_t   asgf = IS_ASIG_ARG(p->fco), asgr = IS_ASIG_ARG(p->res),
          asgd = IS_ASIG_ARG(p->dist);

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT fco, res, dist;
      MYFLT ax1  = lastin;
      MYFLT ay11 = ay1;
      MYFLT ay31 = ay2;
      fco        = (asgf ? p->fco[n] : *p->fco);
      res        = (asgr ? p->res[n] : *p->res);
      dist       = (double)(asgd ? p->dist[n] : *p->dist);
      if (fco != lfc || flag) {
        lfc = fco;
        kfcn = FL(2.0) * fco * csound->onedsr;
        kp   = ((-FL(2.7528)*kfcn + FL(3.0429))*kfcn +
                FL(1.718))*kfcn - FL(0.9984);
        kp1 = kp+FL(1.0);
        kp1h = FL(0.5)*kp1;
        flag = 1;
      }
      if (res != lrs || flag) {
        lrs = res;
        kres = res * (((-FL(2.7079)*kp1 + FL(10.963))*kp1
                           - FL(14.934))*kp1 + FL(8.4974));
        flag = 1;
      }
      if (dist != lds || flag) {
        lds = dist;
        value = 1.0+(dist*(1.5+2.0*(double)kres*(1.0-(double)kfcn)));
      }
      flag = 0;
      lastin     = ain[n]/zerodb - TANH(kres*aout);
      ay1        = kp1h * (lastin + ax1) - kp*ay1;
      ay2        = kp1h * (ay1 + ay11) - kp*ay2;
      aout       = kp1h * (ay2 + ay31) - kp*aout;

      ar[n] = TANH(aout*value)*zerodb;
    }
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

int32_t wavesetset(CSOUND *csound, BARRI *p)
{
    if (*p->len == FL(0.0))
      p->length = 1 + (int32_t)(p->h.insdshead->p3.value * CS_ESR * FL(0.5));
    else
      p->length = 1 + (int32_t)*p->len;
    if (UNLIKELY(p->length <= 1)) p->length = (int32_t)CS_ESR;
    csound->AuxAlloc(csound, (int32)p->length*sizeof(MYFLT), &p->auxch);
    p->cnt = 1;
    p->start = 0;
    p->current = 0;
    p->end = 0;
    p->direction = 1;
    p->lastsamp = FL(1.0);
    p->noinsert = 0;
    return OK;
}

int32_t waveset(CSOUND *csound, BARRI *p)
{
    IGN(csound);
    MYFLT *in = p->ain;
    MYFLT *out = p->ar;
    int32_t   index = p->end;
    MYFLT *insert = (MYFLT*)(p->auxch.auxp) + index;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->noinsert) goto output;
    for (n=offset;n<nsmps;n++) {                        /* Deal with inputs */
      *insert++ = in[n];
      if (++index ==  p->start) {
        p->noinsert = 1;
        break;
      }
      if (index==p->length) {   /* Input wrapping */
        index = 0;
        insert = (MYFLT*)(p->auxch.auxp);
      }
    }
 output:
    p->end = index;
    index = p->current;
    insert = (MYFLT*)(p->auxch.auxp) + index;
    for (n=offset;n<nsmps;n++) {
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
      out[n] = samp;
    }
    p->current = index;
    return OK;
}

int32_t medfiltset(CSOUND *csound, MEDFILT *p)
{
    int32_t maxwind = (int32_t)MYFLT2LONG(*p->imaxsize);
    int32_t auxsize = 2*sizeof(MYFLT)*maxwind;
    p->ind = 0;
    p->maxwind = maxwind;

    if (p->b.auxp==NULL || p->b.size < (size_t)auxsize)
      csound->AuxAlloc(csound, (size_t)auxsize, &p->b);
    else
      if (*p->iskip!=FL(0.0)) memset(p->b.auxp, 0, auxsize);
    p->buff = (MYFLT*)p->b.auxp;
    p->med = &(p->buff[maxwind]);
    return OK;
}

int32_t medfilt(CSOUND *csound, MEDFILT *p)
{
    MYFLT *aout = p->ans;
    MYFLT *asig = p->asig;
    MYFLT *buffer = p->buff;
    MYFLT *med = p->med;
    int32_t maxwind = p->maxwind;
    int32_t kwind = MYFLT2LONG(*p->kwind);
    int32_t index = p->ind;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    if (UNLIKELY(p->b.auxp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("median: not initialised (arate)\n"));
    }
    if (UNLIKELY(kwind > maxwind)) {
      csound->Warning(csound,
                      Str("median: window (%d)larger than maximum(%d); truncated"),
                      kwind, maxwind);
      kwind = maxwind;
    }
    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT x = asig[n];
      buffer[index++] = x;

      if (kwind<=index) {        /* all in centre */
        memcpy(&med[0], &buffer[index-kwind], kwind*sizeof(MYFLT));
        /* printf("memcpy: 0 <- %d (%d)\n", index-kwind, kwind); */
      }
      else {                    /* or in two parts */
        memcpy(&med[0], &buffer[0], index*sizeof(MYFLT));
        /* printf("memcpy: 0 <- 0 (%d)\n", index); */
        memcpy(&med[index], &buffer[maxwind+index-kwind],
               (kwind-index)*sizeof(MYFLT));
        /* printf("memcpy: %d <- %d (%d)\n",
           index, maxwind+index-kwind, kwind-index); */
      }
      /* { int32_t i; */
      /*   for (i=0; i<8; i++) printf(" %f", buffer[i]); */
      /*   printf("\n\t:"); */
      /*   for (i=0; i<5; i++) printf(" %f", med[i]); */
      /*   printf("\n"); */
      /* } */
      aout[n] = medianvalue(kwind, med-1); /* -1 as should point below data */
      /* printf("%d/$%d: %f -> %f\n", n, index-1, x, aout[n]); */
      if (index>=maxwind) index = 0;
    }
    p->ind = index;
    return OK;
}

int32_t kmedfilt(CSOUND *csound, MEDFILT *p)
{
    MYFLT *buffer = p->buff;
    MYFLT *med = p->med;
    MYFLT x = *p->asig;
    int32_t maxwind = p->maxwind;
    int32_t kwind = MYFLT2LONG(*p->kwind);
    int32_t index = p->ind;
    if (UNLIKELY(p->b.auxp==NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("median: not initialised (krate)\n"));
    }
    if (UNLIKELY(kwind > maxwind)) {
      csound->Warning(csound,
                      Str("median: window (%d)larger than maximum(%d); truncated"),
                      kwind, maxwind);
      kwind = maxwind;
    }
    buffer[index++] = x;
    if (kwind<=index) {        /* all in centre */
      memcpy(&med[0], &buffer[index-kwind], kwind*sizeof(MYFLT));
    }
    else {                    /* or in two parts */
      memcpy(&med[0], &buffer[0], index*sizeof(MYFLT));
      memcpy(&med[index], &buffer[maxwind+index-kwind],
             (kwind-index)*sizeof(MYFLT));
    }
    *p->ans = medianvalue(kwind, med-1); /* -1 as should point below data */
    if (index>=maxwind) index = 0;
    p->ind = index;
    return OK;
}
