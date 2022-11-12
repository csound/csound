/*
    spectra.c:

    Copyright (C) 1995 Barry Vercoe

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
#include "csoundCore.h"
#include "interlocks.h"
#include <math.h>
#include "cwindow.h"
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"
#include <inttypes.h>

#define LOGTWO  (0.69314718056)

void DOWNset(CSOUND *p, DOWNDAT *downdp, int32_t npts)
{
    int32_t nbytes = npts * sizeof(MYFLT);

    if (downdp->auxch.auxp == NULL || downdp->auxch.size != (uint32_t)nbytes)
      p->AuxAlloc(p, nbytes, &downdp->auxch);
    downdp->npts = npts;
}

void SPECset(CSOUND *p, SPECDAT *specdp, int32_t npts)
{
    int32_t nbytes = npts * sizeof(MYFLT);

    if (specdp->auxch.auxp == NULL || (uint32_t)nbytes != specdp->auxch.size)
      p->AuxAlloc(p, nbytes, &specdp->auxch);
    specdp->npts = npts;
}

static const char *outstring[] = {"mag", "db", "mag sqrd", "root mag"};

int32_t spectset(CSOUND *csound, SPECTRUM *p)
                           /* spectrum - calcs disc Fourier transform of */
                           /* oct-downsampled data outputs coefs (mag, */
                           /* db or mag2) of log freq within each octave */
{
    int32_t     n, nocts, nfreqs, ncoefs, hanning;
    MYFLT   Q, *fltp;
    OCTDAT  *octp;
    DOWNDAT *dwnp = &p->downsig;
    SPECDAT *specp = p->wsig;

    /* for mac roundoff */
    p->timcount = (int32_t)(CS_EKR * *p->iprd + FL(0.001));
    nocts = (int32_t)*p->iocts;
    nfreqs = (int32_t)*p->ifrqs;
    ncoefs = nocts * nfreqs;
    Q = *p->iq;
    hanning = (*p->ihann) ? 1 : 0;
    p->dbout = (int32_t)*p->idbout;
    if (UNLIKELY((p->disprd = (int32_t)(CS_EKR * *p->idisprd)) < 0))
      p->disprd = 0;

    if (UNLIKELY(p->timcount <= 0))
      return csound->InitError(csound, Str("illegal iprd"));
    if (UNLIKELY(nocts <= 0 || nocts > MAXOCTS))
      return csound->InitError(csound, Str("illegal iocts"));
    if (UNLIKELY(nfreqs <= 0 || nfreqs > MAXFRQS))
      return csound->InitError(csound, Str("illegal ifrqs"));
    if (UNLIKELY(Q <= FL(0.0)))
      return csound->InitError(csound, Str("illegal Q value"));
    if (UNLIKELY(p->dbout < 0 || p->dbout > 3))
      return csound->InitError(csound, Str("unknown dbout code"));

    if (nocts != dwnp->nocts ||
        nfreqs != p->nfreqs  || /* if anything has changed */
        Q != p->curq         ||
        (p->disprd && !p->octwindow.windid) ||
        hanning != p->hanning) {                /*     make new tables */
      double      basfrq, curfrq, frqmlt, Qfactor;
      double      theta, a, windamp, onedws, pidws;
      MYFLT       *sinp, *cosp;
      int32_t         k, sumk, windsiz, halfsiz, *wsizp, *woffp;
      int32_t       auxsiz, bufsiz = 0;
      int32_t       majr, minr, totsamps, totsize;
      double      hicps,locps,oct;  /*   must alloc anew */

      p->nfreqs = nfreqs;
      p->curq = Q;
      p->hanning = hanning;
      p->ncoefs = ncoefs;
      csound->Warning(csound,
                      Str("spectrum: %s window, %s out, making tables ...\n"),
                      (hanning) ? "hanning":"hamming", outstring[p->dbout]);

      if (csoundGetTypeForArg(p->signal) == &CS_VAR_TYPE_K) {
        dwnp->srate = CS_EKR;            /* define the srate */
        p->nsmps = 1;
      }
      else {
        dwnp->srate = CS_ESR;
        p->nsmps = CS_KSMPS;
      }
      hicps = dwnp->srate * 0.375;            /* top freq is 3/4 pi/2 ...   */
      oct = log(hicps / ONEPT) / LOGTWO;      /* octcps()  (see aops.c)     */
      if (csoundGetTypeForArg(p->signal) != &CS_VAR_TYPE_K) {     /* for sr sampling:           */
        oct = ((int32_t)(oct*12.0 + 0.5)) / 12.0; /*     semitone round to A440 */
        hicps = pow(2.0, oct) * ONEPT;        /*     cpsoct()               */
      }
      dwnp->looct = (MYFLT)(oct - nocts);     /* true oct val of lowest frq */
      locps = hicps / (1L << nocts);
      csound->Warning(csound, Str("\thigh cps %7.1f\n\t low cps %7.1f\n"),
                              hicps, locps);

      basfrq = hicps/2.0;                     /* oct below retuned top */
      frqmlt = pow(2.0,(double)1.0/nfreqs);   /* nfreq interval mult */
      Qfactor = Q * dwnp->srate;
      curfrq = basfrq;
      for (sumk=0,wsizp=p->winlen,woffp=p->offset,n=nfreqs; n--; ) {
        *wsizp++ = k = (int32_t)(Qfactor/curfrq) | 01;  /* calc odd wind sizes */
        *woffp++ = (*(p->winlen) - k) / 2;          /* & symmetric offsets */
        sumk += k;                                  /*    and find total   */
        curfrq *= frqmlt;
      }
      windsiz = *(p->winlen);
      csound->Warning(csound,
                      Str("\tQ %4.1f uses a %d sample window each octdown\n"),
                      Q, windsiz);
      auxsiz = (windsiz + 2*sumk) * sizeof(MYFLT);   /* calc lcl space rqd */

      csound->AuxAlloc(csound, (size_t)auxsiz, &p->auxch1); /* & alloc auxspace */

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
          windamp = a * a;                           /*   times hanning */
          if (!hanning)
            windamp = 0.08 + 0.92 * windamp;         /*   or hamming    */
          windamp *= onedws;                         /*   scaled        */
          theta = k * curfrq;
          *sinp++ = (MYFLT)(windamp * sin(theta));
          *cosp++ = (MYFLT)(windamp * cos(theta));
        }
        curfrq *= frqmlt;                        /*   step by log freq  */
      }
      if (*p->idsines != FL(0.0)) {      /* if reqd, dsply windowed sines now! */
        csound->dispset(csound, &p->sinwindow, p->sinp, (int32_t) sumk,
                                Str("spectrum windowed sines:"), 0, "spectrum");
        csound->display(csound, &p->sinwindow);
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
      csound->Warning(csound, Str("\t%d oct analysis window "
                                  "delay = %"PRIi32" samples (%d msecs)\n"),
                              nocts, bufsiz, (int32_t)(bufsiz*1000/dwnp->srate));
      if (p->disprd) {                      /* if display requested, */
        totsize = totsamps * sizeof(MYFLT); /*  alloc an equiv local */
        csound->AuxAlloc(csound,
                         (size_t)totsize, &p->auxch2);/*  linear output window */
        csound->dispset(csound, &p->octwindow, (MYFLT *)p->auxch2.auxp,
                        (int32_t)totsamps, Str("octdown buffers:"), 0, "spectrum");
      }
      SPECset(csound, specp, (int32_t)ncoefs);  /* prep the spec dspace */
      specp->downsrcp = dwnp;                /*  & record its source */
    }
    for (octp=dwnp->octdata; nocts--; octp++) { /* reset all oct params, &    */
      octp->curp = octp->begp;
      for (fltp=octp->feedback,n=6; n--; )
        *fltp++ = FL(0.0);
      octp->scount = 0;
    }
    specp->nfreqs = p->nfreqs;               /* save the spec descriptors */
    specp->dbout = p->dbout;
    specp->ktimstamp = 0;                    /* init specdata to not new  */
    specp->ktimprd = p->timcount;
    p->scountdown = p->timcount;             /* prime the spect countdown */
    p->dcountdown = p->disprd;               /*   & the display countdown */
    return OK;
}

static void linocts(DOWNDAT *dwnp, MYFLT *bufp)
     /* linearize octdown dat to 1 buf */
{    /* presumes correct buffer alloc'd in set */
    MYFLT  *curp, *endp;
    int32_t    wrap;
    OCTDAT *octp;
    int32_t    nocts;
    MYFLT  *begp;

    nocts = dwnp->nocts;
    octp = dwnp->octdata + nocts;
    while (nocts--) {
      octp--;                            /* for each octave (low to high) */
      begp = octp->begp;
      curp = octp->curp;
      endp = octp->endp;
      wrap = curp - begp;
      while (curp < endp)                     /*   copy circbuf to linbuf */
        *bufp++ = *curp++;
      for (curp=begp; wrap--; )
        *bufp++ = *curp++;
    }
}

static const MYFLT bicoefs[] = {
   -FL(0.2674054), FL(0.7491305), FL(0.7160484), FL(0.0496285), FL(0.7160484),
    FL(0.0505247), FL(0.3514850), FL(0.5257536), FL(0.3505025), FL(0.5257536),
    FL(0.3661840), FL(0.0837990), FL(0.3867783), FL(0.6764264), FL(0.3867783)
};

int32_t spectrum(CSOUND *csound, SPECTRUM *p)
{
    MYFLT   a, b, *dftp, *sigp = p->signal, SIG, yt1, yt2;
    int32_t     nocts, nsmps = p->nsmps, winlen;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    DOWNDAT *downp = &p->downsig;
    OCTDAT  *octp;
    SPECDAT *specp;
    double  c;

    if (UNLIKELY(early)) nsmps -= early;
    do {
      SIG = *sigp++;                        /* for each source sample:     */
      if (offset--) SIG = FL(0.0);          /* for sample accuracy         */
      octp = downp->octdata;                /*   align onto top octave     */
      nocts = downp->nocts;
      do {                                  /*   then for each oct:        */
        const MYFLT *coefp;
        MYFLT       *ytp, *curp;
        int32_t         nfilt;
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

    if (p->disprd)                               /* if displays requested,   */
      if (!(--p->dcountdown)) {                  /*   on countdown           */
        linocts(downp, (MYFLT *)p->auxch2.auxp); /*   linearize the oct bufs */
        csound->display(csound, &p->octwindow);  /*      & display           */
        p->dcountdown = p->disprd;
      }

    if ((--p->scountdown)) return OK;/* if not yet time for new spec, return */
    p->scountdown = p->timcount;     /* else reset counter & proceed:        */
    downp = &p->downsig;
    specp = p->wsig;
    nocts = downp->nocts;
    octp = downp->octdata + nocts;
    dftp = (MYFLT *) specp->auxch.auxp;
    winlen = *(p->winlen);
    while (nocts--) {
      MYFLT  *bufp, *sinp, *cosp;
      int32_t    len, *lenp, *offp, nfreqs;
      MYFLT    *begp, *curp, *endp, *linbufp;
      int32_t      len2;
      octp--;                                /* for each oct (low to high) */
      begp = octp->begp;
      curp = octp->curp;
      endp = octp->endp;
      if ((len = endp - curp) >= winlen)     /*   if no wrap               */
        linbufp = curp;                      /*     use samples in circbuf */
      else {
        len2 = winlen - len;
        linbufp = bufp = p->linbufp;         /*   else cp crcbuf to linbuf */
        while (len--)
          *bufp++ = *curp++;
        curp = begp;
        while (len2--)
          *bufp++ = *curp++;
      }
      cosp = p->cosp;                        /*   get start windowed sines */
      sinp = p->sinp;
      lenp = p->winlen;
      offp = p->offset;
      for (nfreqs=p->nfreqs; nfreqs--; ) {   /*   now for ea. frq this oct */
        a = FL(0.0);
        b = FL(0.0);
        bufp = linbufp + *offp++;
        for (len = *lenp++; len--; bufp++) { /* apply windowed sine seg */
          a += *bufp * *cosp++;
          b += *bufp * *sinp++;
        }
        c = a*a + b*b;                       /* get magnitude squared   */
        switch (p->dbout) {
        case 1:
          if (c < .001) c = .001;            /* and convert to db       */
          c = 10.0 * log10(c);
          break;
        case 3:
          c = sqrt(c);                       /*    or root mag          */
          /* FALLTHRU */
        case 0:
          c = sqrt(c);                       /*    or mag               */
          /* FALLTHRU */
        case 2:
          break;                             /*    or leave mag sqrd    */
        }
        *dftp++ = (MYFLT)c;                  /* store in out spectrum   */
      }
    }
    specp->ktimstamp = CS_KCNT;     /* time-stamp the output   */
    return OK;
}

/* int32_t nocdfset(CSOUND *csound, NOCTDFT *p) */
/*     /\* noctdft - calcs disc Fourier transform of oct-downsampled data *\/ */
/*     /\* outputs coefs (mag, db or mag2) of log freq within each octave *\/ */
/* { */
/*     int32_t     nfreqs, hanning, nocts, ncoefs; */
/*     MYFLT   Q, *fltp; */
/*     DOWNDAT *downp = p->dsig; */
/*     SPECDAT *specp = p->wsig; */

/*     p->timcount = CS_EKR * *p->iprd; */
/*     nfreqs = *p->ifrqs; */
/*     Q = *p->iq; */
/*     hanning = (*p->ihann) ? 1 : 0; */
/*     if ((p->dbout = *p->idbout) && p->dbout != 1 && p->dbout != 2) { */
/*       return csound->InitError(csound,
                                  Str("noctdft: unknown dbout code of %d"), */
/*                                        p->dbout); */
/*     } */
/*     nocts = downp->nocts; */
/*     ncoefs = nocts * nfreqs; */
/*     if (nfreqs != p->nfreqs || Q != p->curq  /\* if anything changed *\/ */
/*         || p->timcount <= 0 || Q <= 0. */
/*         || hanning != p->hanning */
/*         || ncoefs != p->ncoefs) {       /\*     make new tables *\/ */
/*       double      basfrq, curfrq, frqmlt, Qfactor; */
/*       double      theta, a, windamp, onedws, pidws; */
/*       MYFLT       *sinp, *cosp; */
/*       int32_t         n, k, sumk, windsiz, *wsizp, nsamps; */
/*       int64_t        auxsiz; */

/*       csound->Message(csound, */
/*                       Str("noctdft: %s window, %s out, making tables ...\n"), */
/*                       (hanning) ? "hanning":"hamming", outstring[p->dbout]); */
/*       if (p->timcount <= 0) */
/*         return csound->InitError(csound, Str("illegal iprd")); */
/*       if (nfreqs <= 0 || nfreqs > MAXFRQS) */
/*         return csound->InitError(csound, Str("illegal ifrqs")); */
/*       if (Q <= FL(0.0)) */
/*         return csound->InitError(csound, Str("illegal Q value")); */
/*       nsamps = downp->nsamps; */
/*       p->nfreqs = nfreqs; */
/*       p->curq = Q; */
/*       p->hanning = hanning; */
/*       p->ncoefs = ncoefs; */
/*       basfrq = downp->hifrq/2.0 * TWOPI/downp->srate;
                                                /\* oct below retuned top *\/ */
/*       frqmlt = pow(2.0,1.0/(double)nfreqs);    /\* nfreq interval mult *\/ */
/*       Qfactor = TWOPI * Q;      /\* Was incorrect value for 2pi?? *\/ */
/*       curfrq = basfrq; */
/*       for (sumk=0,wsizp=p->winlen,n=nfreqs; n--; ) { */
/*         *wsizp++ = k = Qfactor/curfrq + 0.5; /\* calc window sizes  *\/ */
/*         sumk += k;                           /\*   and find total   *\/ */
/*         curfrq *= frqmlt; */
/*       } */
/*       if ((windsiz = *(p->winlen)) > nsamps) {/\* chk longest windsiz *\/ */
/*         return csound->InitError(csound, Str("Q %4.1f needs %d samples, " */
/*                                              "octdown has just %d"), */
/*                                          Q, windsiz, nsamps); */
/*       } */
/*       else csound->Message(csound, Str("noctdft: Q %4.1f uses %d of " */
/*                                        "%d samps per octdown\n"), */
/*                                    Q, windsiz, nsamps); */
/*       auxsiz = (nsamps + 2*sumk) * sizeof(MYFLT);/\* calc local space reqd *\/ */
/*       csound->AuxAlloc(csound, (size_t)auxsiz, &p->auxch);
         /\* & alloc auxspace  *\/ */
/*       fltp = (MYFLT *) p->auxch.auxp; */
/*       p->linbufp = fltp;          fltp += nsamps;
         /\* linbuf must handle nsamps *\/ */
/*       p->sinp = sinp = fltp;      fltp += sumk; */
/*       p->cosp = cosp = fltp;                 /\* cos gets rem sumk  *\/ */
/*       wsizp = p->winlen; */
/*       for (curfrq=basfrq,n=nfreqs; n--; ) {     /\* now fill tables *\/ */
/*         windsiz = *wsizp++; */
/*         onedws = 1.0 / windsiz; */
/*         pidws = PI / windsiz; */
/*         for (k=0; k<windsiz; k++) {                 /\*   with sines    *\/ */
/*           a = sin(k * pidws); */
/*           windamp = a * a;                        /\*   times hanning *\/ */
/*           if (!hanning) */
/*             windamp = 0.08 + 0.92 * windamp;    /\*   or hamming    *\/ */
/*           windamp *= onedws;                      /\*   scaled        *\/ */
/*           theta = k * curfrq; */
/*           *sinp++ = windamp * sin(theta); */
/*           *cosp++ = windamp * cos(theta); */
/*         } */
/*         curfrq *= frqmlt;                     /\*   step by log freq  *\/ */
/*       } */
/*       if (*p->idsines != FL(0.0)) { */
/*         /\* if reqd, display windowed sines immediately *\/ */
/*         csound->dispset(csound, &p->dwindow, p->sinp, (int32_t) sumk, */
/*                                 Str("octdft windowed sines:"), 0, "octdft"); */
/*         csound->display(csound, &p->dwindow); */
/*       } */
/*       SPECset(csound, */
/*               specp, (int64_t)ncoefs);          /\* prep the spec dspace *\/ */
/*       specp->downsrcp = downp;               /\*  & record its source *\/ */
/*     } */
/*     specp->nfreqs = p->nfreqs;            \* save the spec descriptors *\/ */
/*     specp->dbout = p->dbout; */
/*     specp->ktimstamp = 0;                 \* init specdata to not new  *\/ */
/*     specp->ktimprd = p->timcount; */
/*     p->countdown = p->timcount;           \*     & prime the countdown *\/ */
/*     return OK; */
/* } */

/* int32_t noctdft(CSOUND *csound, NOCTDFT *p) */
/* { */
/*     DOWNDAT *downp; */
/*     SPECDAT *specp; */
/*     OCTDAT  *octp; */
/*     MYFLT   *dftp; */
/*     int32_t     nocts, wrap; */
/*     MYFLT   a, b; */
/*     double  c; */

/*     if ((--p->countdown))  return;
       /\* if not yet time for new spec, return *\/ */
/*     if (p->auxch.auxp==NULL) { /\* RWD fix *\/ */
/*       return csound->PerfError(csound, &(p->h), */
/*                                Str("noctdft: not initialised")); */
/*     } */
/*     p->countdown = p->timcount;      /\* else reset counter & proceed:   *\/ */
/*     downp = p->dsig; */
/*     specp = p->wsig; */
/*     nocts = downp->nocts; */
/*     octp = downp->octdata + nocts; */
/*     dftp = (MYFLT *) specp->auxch.auxp; */
/*     while (nocts--) { */
/*       MYFLT  *bufp, *sinp, *cosp; */
/*       int32_t    len, *lenp, nfreqs; */
/*       MYFLT   *begp, *curp, *endp; */
/*       octp--;                        /\* for each octave (low to high)   *\/ */
/*       begp = octp->begp; */
/*       curp = octp->curp; */
/*       endp = octp->endp; */
/*       wrap = curp - begp; */
/*       bufp = p->linbufp; */
/*       while (curp < endp)              /\*   copy circbuf to linbuf   *\/ */
/*         *bufp++ = *curp++; */
/*       for (curp=begp,len=wrap; len--; ) */
/*         *bufp++ = *curp++; */
/*       cosp = p->cosp;                  /\*   get start windowed sines *\/ */
/*       sinp = p->sinp; */
/*       lenp = p->winlen; */
/*       for (nfreqs=p->nfreqs; nfreqs--; ) { /\* now for each freq this oct: *\/ */
/*         a = 0.0; */
/*         b = 0.0; */
/*         bufp = p->linbufp; */
/*         for (len = *lenp++; len--; bufp++) {/\*  apply windowed sine seg *\/ */
/*           a += *bufp * *cosp++; */
/*           b += *bufp * *sinp++; */
/*         } */
/*         c = a*a + b*b;                    /\*  get magnitude squared   *\/ */
/*         if (!(p->dbout))                  /\*    & optionally convert  *\/ */
/*           c = sqrt(c);                    /\*    to  mag or db         *\/ */
/*         else if (p->dbout == 1) { */
/*           if (c < .001) c = .001; */
/*           c = 10. * log10(c); */
/*         } */
/*         *dftp++ = c;                       /\* store in out spectrum   *\/ */
/*       } */
/*     } */
/*     specp->ktimstamp = CS_KCNT;   /\* time-stamp the output   *\/ */
/*     return OK; */
/* } */

int32_t spdspset(CSOUND *csound, SPECDISP *p)
{
    char  strmsg[256];
    /* RWD is this enough? */
    if (UNLIKELY(p->wsig->auxch.auxp==NULL)) {
      return csound->InitError(csound, Str("specdisp: not initialised"));
    }
    if (UNLIKELY((p->timcount = (int32_t)(CS_EKR * *p->iprd)) <= 0)) {
      return csound->InitError(csound, Str("illegal iperiod"));
    }
    if (!(p->dwindow.windid)) {
      SPECDAT *specp = p->wsig;
      DOWNDAT *downp = specp->downsrcp;
      if (downp->lofrq > FL(5.0)) {
        snprintf(strmsg, 256,
                Str("instr %d %s, dft (%s), %d octaves (%d - %d Hz):"),
                (int32_t) p->h.insdshead->p1.value, p->h.optext->t.inlist->arg[0],
                outstring[specp->dbout],
                downp->nocts, (int32_t)downp->lofrq, (int32_t)downp->hifrq);
      }
      else {                            /* more detail if low frequency  */
        snprintf(strmsg, 256,
                Str("instr %d %s, dft (%s), %d octaves (%3.1f - %3.1f Hz):"),
                (int32_t) p->h.insdshead->p1.value, p->h.optext->t.inlist->arg[0],
                outstring[specp->dbout],
                downp->nocts, downp->lofrq, downp->hifrq);
      }
      csound->dispset(csound, &p->dwindow, (MYFLT*) specp->auxch.auxp,
                      (int32_t)specp->npts, strmsg, (int32_t)*p->iwtflg,
                      "specdisp");
    }
    p->countdown = p->timcount;         /* prime the countdown */
    return OK;
}

int32_t specdisp(CSOUND *csound, SPECDISP *p)
{
    /* RWD is this enough? */
    if (UNLIKELY(p->wsig->auxch.auxp==NULL)) goto err1;
    if (!(--p->countdown)) {            /* on countdown     */
      csound->display(csound, &p->dwindow);     /*    display spect */
      p->countdown = p->timcount;       /*    & reset count */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specdisp: not initialised"));
}

int32_t sptrkset(CSOUND *csound, SPECPTRK *p)
{
    SPECDAT *inspecp = p->wsig;
    int32_t   npts, nptls, nn, lobin;
    int32_t     *dstp, ptlmax, inc;
    MYFLT   nfreqs, rolloff, *oct0p, *flop, *fhip, *fundp, *fendp, *fp;
    MYFLT   weight, weightsum, dbthresh, ampthresh;

    if ((npts = inspecp->npts) != p->winpts) {  /* if size has changed */
      SPECset(csound,
              &p->wfund, (int32_t)npts);           /*   realloc for wfund */
      p->wfund.downsrcp = inspecp->downsrcp;
      p->fundp = (MYFLT *) p->wfund.auxch.auxp;
      p->winpts = npts;
        }
    if ((p->ftimcnt = (int32_t)(CS_EKR**p->ifprd)) > 0) {
      /* if displaying wfund */
      SPECDISP *fdp = &p->fdisplay;
      fdp->h = p->h;
      fdp->wsig = &p->wfund;                    /*  pass the param pntrs */
      fdp->iprd = p->ifprd;
      fdp->iwtflg = p->iwtflg;
/*       fdp->altname = "specptrk"; */
/*       fdp->altarg = "X-corr"; */
      p->wfund.dbout = inspecp->dbout;
      spdspset(csound,fdp);                     /*  & call specdisp init */
    }
    else p->ftimcnt = 0;
    if (UNLIKELY((nptls = (int32_t)*p->inptls) <= 0 || nptls > MAXPTL)) {
      return csound->InitError(csound, Str("illegal no of partials"));
    }
    p->nptls = nptls;        /* number, whether all or odd */
    if (*p->iodd == FL(0.0)) {
      ptlmax = nptls;
      inc = 1;
    } else {
      ptlmax = nptls * 2 - 1;
      inc = 2;
    }
    dstp = p->pdist;
    nfreqs = (MYFLT)inspecp->nfreqs;
    for (nn = 1; nn <= ptlmax; nn += inc)
      *dstp++ = (int32_t) ((log((double) nn) / LOGTWO) * nfreqs + 0.5);
    if ((rolloff = *p->irolloff) == 0.0 || rolloff == 1.0 || nptls == 1) {
      p->rolloff = 0;
      weightsum = (MYFLT)nptls;
    } else {
      MYFLT *fltp = p->pmult;
      MYFLT octdrop = (FL(1.0) - rolloff) / nfreqs;
      weightsum = FL(0.0);
      for (dstp = p->pdist, nn = nptls; nn--; ) {
        weight = FL(1.0) - octdrop * *dstp++;       /* rolloff * octdistance */
        weightsum += weight;
        *fltp++ = weight;
      }
      if (UNLIKELY(*--fltp < FL(0.0))) {
        return csound->InitError(csound, Str("per oct rolloff too steep"));
      }
      p->rolloff = 1;
    }
    lobin = (int32_t)(inspecp->downsrcp->looct * nfreqs);
    oct0p = p->fundp - lobin;                   /* virtual loc of oct 0 */

    flop = oct0p + (int32_t)(*p->ilo * nfreqs);
    fhip = oct0p + (int32_t)(*p->ihi * nfreqs);
    fundp = p->fundp;
    fendp = fundp + inspecp->npts;
    if (flop < fundp) flop = fundp;
    if (fhip > fendp) fhip = fendp;
    if (UNLIKELY(flop >= fhip)) {         /* chk hi-lo range valid */
      return csound->InitError(csound, Str("illegal lo-hi values"));
    }
    for (fp = fundp; fp < flop; )
      *fp++ = FL(0.0);   /* clear unused lo and hi range */
    for (fp = fhip; fp < fendp; )
      *fp++ = FL(0.0);

    csound->Warning(csound, Str("specptrk: %d freqs, %d%s ptls at "),
                    (int32_t)nfreqs, (int32_t)nptls, inc==2 ? Str(" odd") : "");
    for (nn = 0; nn < nptls; nn++)
      csound->Warning(csound, "\t%d", p->pdist[nn]);
    if (p->rolloff) {
      csound->Warning(csound, Str("\n\t\trolloff vals:"));
      for (nn = 0; nn < nptls; nn++)
        csound->Warning(csound, "\t%4.2f", p->pmult[nn]);
    }

    dbthresh = *p->idbthresh;                     /* thresholds: */
    ampthresh = (MYFLT)exp((double)dbthresh * LOG10D20);
    switch(inspecp->dbout) {
    case 0: p->threshon = ampthresh;              /* mag */
      p->threshoff = ampthresh / FL(2.0);
                break;
    case 1: p->threshon = dbthresh;               /* db  */
      p->threshoff = dbthresh - FL(6.0);
      break;
    case 2: p->threshon = ampthresh * ampthresh;  /* mag sqrd */
      p->threshoff = p->threshon / FL(4.0);
      break;
    case 3: p->threshon = (MYFLT)sqrt(ampthresh);        /* root mag */
      p->threshoff = p->threshon / FL(1.414);
      break;
    }
    p->threshon *= weightsum;
    p->threshoff *= weightsum;
    csound->Warning(csound, Str("\n\tdbthresh %4.1f: X-corr %s "
                                "threshon %4.1f, threshoff %4.1f\n"),
                            dbthresh, outstring[inspecp->dbout],
                            p->threshon, p->threshoff);
    p->oct0p = oct0p;                 /* virtual loc of oct 0 */
    p->confact = *p->iconf;
    p->flop = flop;
    p->fhip = fhip;
    p->kinterp = (*p->interp == FL(0.0)) ? 0 : 1;
    p->playing = 0;
    p->kvalsav = *p->istrt;
    p->kval = p->kinc = FL(0.0);
    p->kavl = p->kanc = FL(0.0);
    p->jmpcount =  0;
    return OK;
}

#define STARTING 1
#define PLAYING  2

int32_t specptrk(CSOUND *csound, SPECPTRK *p)
{
    SPECDAT *inspecp = p->wsig;

    if (inspecp->ktimstamp == CS_KCNT) {   /* if inspectrum is new: */
      MYFLT *inp = (MYFLT *) inspecp->auxch.auxp;
      MYFLT *endp = inp + inspecp->npts;
      MYFLT *inp2, sum, *fp;
      int32_t   nn, *pdist, confirms;
      MYFLT kval, kvar, fmax, *fmaxp, absdiff, realbin;
      MYFLT *flop, *fhip, *ilop, *ihip, a, b, c, denom, delta;
      int32_t lobin, hibin;

      if (UNLIKELY(inp==NULL)) goto err1;             /* RWD fix */
      kvar = FABS(*p->kvar);
      kval = p->playing == PLAYING ? p->kval : p->kvalsav;
      lobin =
        (int32_t)((kval-kvar) * inspecp->nfreqs); /* set lim of frq interest */
      hibin = (int32_t)((kval+kvar) * inspecp->nfreqs);
      if ((flop = p->oct0p + lobin) < p->flop)  /*       as fundp bin pntrs */
        flop = p->flop;
      if ((fhip = p->oct0p + hibin) > p->fhip)  /*       within hard limits */
        fhip = p->fhip;
      ilop = inp + (flop - p->fundp);           /* similar for input bins   */
      ihip = inp + (fhip - p->fundp);
      if (p->ftimcnt) {                         /* if displaying,  */
        for (fp = p->flop; fp < flop; )         /*   clr to limits */
          *fp++ = FL(0.0);
        for (fp = p->fhip; fp > fhip; )
          *--fp = FL(0.0);
      }
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
        denom = b * FL(2.0) - a - c;
      else denom = a + b + c;
      if (denom != FL(0.0))
        delta = FL(0.5) * (c - a) / denom;
      else delta = FL(0.0);
      realbin = (fmaxp - p->oct0p) + delta;    /* get modified bin number  */
      kval = realbin / inspecp->nfreqs;        /*     & cvt to true decoct */

      if (p->playing == STARTING) {            /* STARTING mode:           */
        absdiff = FABS(kval - p->kvalsav);
        confirms = (int32_t)(absdiff * p->confact); /* get interval dependency  */
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
        absdiff = FABS(kval - p->kval);
        confirms = (int32_t)(absdiff * p->confact); /* get interval dependency  */
        if (p->jmpcount < confirms) {
          p->jmpcount += 1;                /* if not enough confirms,  */
          p->kinc = FL(0.0);                 /*    must wait some more   */
        } else {
          p->jmpcount = 0;                 /* else OK to jump interval */
          if (p->kinterp)                  /*    with optional interp  */
            p->kinc = (kval - p->kval) / inspecp->ktimprd;
          else p->kval = kval;
        }
      }
      fmax += delta * (c - a) / FL(4.0);           /* get modified amp */
      if (p->kinterp)                         /*   & new kanc if interp */
        p->kanc = (fmax - p->kavl) / inspecp->ktimprd;
      else p->kavl = fmax;
    }
 output:
    *p->koct = p->kval;                   /* output true decoct & amp */
    *p->kamp = p->kavl;
    if (p->kinterp) {                     /* interp if reqd  */
      p->kval += p->kinc;
      p->kavl += p->kanc;
    }
    if (p->ftimcnt)
      specdisp(csound,&p->fdisplay);
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specptrk: not initialised"));
}

int32_t spsumset(CSOUND *csound, SPECSUM *p)
{
   IGN(csound);
    p->kinterp = (*p->interp == FL(0.0)) ? 0 : 1;
    p->kinc = p->kval = FL(0.0);
    return OK;
}

int32_t specsum(CSOUND *csound, SPECSUM *p)
                               /* sum all vals of a spectrum and put as ksig */
                               /*         optionally interpolate the output  */
{
    SPECDAT *specp = p->wsig;
    if (UNLIKELY(specp->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (specp->ktimstamp == CS_KCNT) { /* if spectrum is new   */
      MYFLT *valp = (MYFLT *) specp->auxch.auxp;
      MYFLT sum = FL(0.0);
      int32_t n,npts = specp->npts;                /*   sum all the values */
      for (n=0;n<npts;n++) {
        sum += valp[n];
      }
      if (p->kinterp)                           /*   new kinc if interp */
        p->kinc = (sum - p->kval) / specp->ktimprd;
      else p->kval = sum;
    }
    *p->ksum = p->kval;       /* output current kval */
    if (p->kinterp)           /*   & interp if reqd  */
      p->kval += p->kinc;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specsum: not initialised"));
}

int32_t spadmset(CSOUND *csound, SPECADDM *p)
{
    SPECDAT *inspec1p = p->wsig1;
    SPECDAT *inspec2p = p->wsig2;
    int32_t   npts;

    if (UNLIKELY((npts = inspec1p->npts) != inspec2p->npts))
      /* inspecs must agree in size */
      return csound->InitError(csound, Str("inputs have different sizes"));
    if (UNLIKELY(inspec1p->ktimprd != inspec2p->ktimprd))
      /*                time period */
      return csound->InitError(csound, Str("inputs have diff. time periods"));
    if (UNLIKELY(inspec1p->nfreqs != inspec2p->nfreqs))
      /*                frq resoltn */
      return csound->InitError(csound,
                               Str("inputs have different freq resolution"));
    if (UNLIKELY(inspec1p->dbout != inspec2p->dbout))
      /*                and db type */
      return csound->InitError(csound, Str("inputs have different amptypes"));
    if (npts != p->waddm->npts) {                 /* if out does not match ins */
      SPECset(csound,
              p->waddm, (int32_t)npts);              /*       reinit the out spec */
      p->waddm->downsrcp = inspec1p->downsrcp;
    }
    p->waddm->ktimprd = inspec1p->ktimprd;        /* pass the other specinfo */
    p->waddm->nfreqs = inspec1p->nfreqs;
    p->waddm->dbout = inspec1p->dbout;
    p->waddm->ktimstamp = 0;                      /* mark the outspec not new */
    return OK;
}

int32_t specaddm(CSOUND *csound, SPECADDM *p)
{
    if (UNLIKELY((p->wsig1->auxch.auxp==NULL) || /* RWD fix */
                 (p->wsig2->auxch.auxp==NULL) ||
                 (p->waddm->auxch.auxp==NULL))) goto err1;
    if (p->wsig1->ktimstamp == CS_KCNT) {  /* if inspec1 is new:     */
      MYFLT *in1p = (MYFLT *) p->wsig1->auxch.auxp;
      MYFLT *in2p = (MYFLT *) p->wsig2->auxch.auxp;
      MYFLT *outp = (MYFLT *) p->waddm->auxch.auxp;
      MYFLT mul2 = p->mul2;
      int32_t   n,npts = p->wsig1->npts;

      for (n=0;n<npts;n++) {
        outp[n] = in1p[n] + in2p[n] * mul2;         /* out = in1 + in2 * mul2 */
      }
      p->waddm->ktimstamp = CS_KCNT;  /* mark the output spec as new */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specaddm: not initialised"));
}

int32_t spdifset(CSOUND *csound, SPECDIFF *p)
{
    SPECDAT *inspecp = p->wsig;
    MYFLT *lclp;
    MYFLT *outp;
    int32_t   npts;

    if ((npts = inspecp->npts) != p->specsave.npts) { /* if inspec not matched  */
      SPECset(csound,
              &p->specsave, (int32_t)npts);             /*   reinit the save spec */
      SPECset(csound,
              p->wdiff, (int32_t)npts);                 /*   & the out diff spec  */
      p->wdiff->downsrcp = inspecp->downsrcp;
    }
    p->wdiff->ktimprd = inspecp->ktimprd;            /* pass the other specinfo */
    p->wdiff->nfreqs = inspecp->nfreqs;
    p->wdiff->dbout = inspecp->dbout;
    lclp = (MYFLT *) p->specsave.auxch.auxp;
    outp = (MYFLT *) p->wdiff->auxch.auxp;
    if (UNLIKELY(lclp==NULL || outp==NULL)) { /* RWD  */
      return csound->InitError(csound,
                               Str("specdiff: local buffers not initialised"));
    }
    memset(lclp, 0, npts*sizeof(MYFLT));          /* clr local & out spec bufs */
    memset(outp, 0, npts*sizeof(MYFLT));
    p->wdiff->ktimstamp = 0;                      /* mark the out spec not new */
    return OK;
}

int32_t specdiff(CSOUND *csound, SPECDIFF *p)
{
    SPECDAT *inspecp = p->wsig;

    if (UNLIKELY((inspecp->auxch.auxp==NULL) /* RWD fix */
        ||
        (p->specsave.auxch.auxp==NULL)
        ||
                 (p->wdiff->auxch.auxp==NULL))) goto err1;
    if (inspecp->ktimstamp == CS_KCNT) {   /* if inspectrum is new: */
      MYFLT *newp = (MYFLT *) inspecp->auxch.auxp;
      MYFLT *prvp = (MYFLT *) p->specsave.auxch.auxp;
      MYFLT *difp = (MYFLT *) p->wdiff->auxch.auxp;
      MYFLT newval, prvval, diff;//, possum = FL(0.0); /* possum not used! */
      int32_t   n,npts = inspecp->npts;

      for (n=0; n<npts;n++) {
        newval = newp[n];                     /* compare new & old coefs */
        prvval = prvp[n];
        if ((diff = newval-prvval) > FL(0.0)) {  /* if new coef > prv coef  */
          difp[n] = diff;
          //possum += diff;                     /*   enter & accum diff    */
        }
        else difp[n] = FL(0.0);               /* else enter zero         */
        prvp[n] = newval;                     /* sav newval for nxt time */
      }
      p->wdiff->ktimstamp = CS_KCNT; /* mark the output spec as new */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specdiff: not initialised"));
}

int32_t spsclset(CSOUND *csound, SPECSCAL *p)
{
    SPECDAT *inspecp = p->wsig;
    SPECDAT *outspecp = p->wscaled;
    FUNC    *ftp;
    int32_t   npts;

    if ((npts = inspecp->npts) != outspecp->npts) {  /* if size has changed,   */
      SPECset(csound,
              outspecp, (int32_t)npts);                 /*    realloc             */
      outspecp->downsrcp = inspecp->downsrcp;
      csound->AuxAlloc(csound, (int32_t)npts * 2 * sizeof(MYFLT), &p->auxch);
    }
    outspecp->ktimprd = inspecp->ktimprd;      /* pass the source spec info     */
    outspecp->nfreqs = inspecp->nfreqs;
    outspecp->dbout = inspecp->dbout;
    p->fscale = (MYFLT *) p->auxch.auxp;       /* setup scale & thresh fn areas */
    if (UNLIKELY(p->fscale==NULL)) {  /* RWD fix */
      return csound->InitError(csound,
                               Str("specscal: local buffer not initialised"));
    }
    p->fthresh = p->fscale + npts;
    if (UNLIKELY((ftp=csound->FTFind(csound, p->ifscale)) == NULL)) {
      /* if fscale given,        */
      return csound->InitError(csound, Str("missing fscale table"));
    }
    else {
      int32_t nn; // = npts;
      int32_t phs = 0;
      int32_t inc = (int32_t)PHMASK / npts;
      int32_t lobits = ftp->lobits;
      MYFLT *ftable = ftp->ftable;
      MYFLT *flp = p->fscale;
      for (nn=0;nn<npts;nn++) {
        flp[nn] = *(ftable + (phs >> lobits));   /*  sample into scale area */
        phs += inc;
      }
    }
    if ((p->thresh = (int32_t)*p->ifthresh) &&
        (ftp=csound->FTFind(csound, p->ifthresh)) != NULL) {
      /* if fthresh given,       */
      int32_t nn; // = npts;
      int32_t phs = 0;
      int32_t inc = (int32_t)PHMASK / npts;
      int32_t lobits = ftp->lobits;
      MYFLT *ftable = ftp->ftable;
      MYFLT *flp = p->fthresh;
      for (nn=0;nn<npts;nn++) {
        flp[nn] = *(ftable + (phs >> lobits));   /*  sample into thresh area */
        phs += inc;
      }
    }
    else p->thresh = 0;
    outspecp->ktimstamp = 0;                     /* mark the out spec not new */
    return OK;
}

int32_t specscal(CSOUND *csound, SPECSCAL *p)
{
    SPECDAT *inspecp = p->wsig;
    if ((inspecp->auxch.auxp==NULL) /* RWD fix */ ||
        (p->wscaled->auxch.auxp==NULL)            ||
        (p->fscale==NULL)) goto err1;
    if (inspecp->ktimstamp == CS_KCNT) {   /* if inspectrum is new: */
      SPECDAT *outspecp = p->wscaled;
      MYFLT *inp = (MYFLT *) inspecp->auxch.auxp;
      MYFLT *outp = (MYFLT *) outspecp->auxch.auxp;
      MYFLT *sclp = p->fscale;
      int32_t n,npts = inspecp->npts;

      if (p->thresh) {                              /* if thresh requested,  */
        MYFLT *threshp = p->fthresh;
        MYFLT val;
        for (n=0; n<npts;n++) {
          if ((val = inp[n] - threshp[n]) > FL(0.0)) /* for vals above thresh */
            outp[n] = val * sclp[n];                 /*     scale & write out */
          else outp[n] = FL(0.0);                    /*   else output is 0.   */
        }
      }
      else {
        for (n=0; n<npts;n++) {
          outp[n] = inp[n] * sclp[n];             /* no thresh: rescale only */
        }
      }
      outspecp->ktimstamp = CS_KCNT;     /* mark the outspec as new */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specscal: not initialised"));
}

int32_t sphstset(CSOUND *csound, SPECHIST *p)
{
    SPECDAT *inspecp = p->wsig;
    MYFLT *lclp;
    MYFLT *outp;
    int32_t   npts;

    if ((npts = inspecp->npts) != p->accumer.npts) { /* if inspec not matched   */
      SPECset(csound,
              &p->accumer, (int32_t)npts);             /*   reinit the accum spec */
      SPECset(csound,
              p->wacout, (int32_t)npts);               /*    & the output spec    */
      p->wacout->downsrcp = inspecp->downsrcp;
    }
    p->wacout->ktimprd = inspecp->ktimprd;           /* pass the other specinfo */
    p->wacout->nfreqs = inspecp->nfreqs;
    p->wacout->dbout = inspecp->dbout;
    lclp = (MYFLT *) p->accumer.auxch.auxp;
    outp = (MYFLT *) p->wacout->auxch.auxp;
    if (UNLIKELY(lclp==NULL || outp==NULL)) { /* RWD fix */
      return csound->InitError(csound,
                               Str("spechist: local buffers not initialised"));
    }
    memset(lclp,0,npts*sizeof(MYFLT));      /* clr local & out spec bufs */
    memset(outp,0,npts*sizeof(MYFLT));
    p->wacout->ktimstamp = 0;             /* mark the out spec not new */
    return OK;
}

int32_t spechist(CSOUND *csound, SPECHIST *p)
{
    SPECDAT *inspecp = p->wsig;
    if (UNLIKELY((inspecp->auxch.auxp==NULL) /* RWD fix */
        ||
        (p->accumer.auxch.auxp==NULL)
        ||
                 (p->wacout->auxch.auxp==NULL))) goto err1;
    if (inspecp->ktimstamp == CS_KCNT) {   /* if inspectrum is new: */
      MYFLT *newp = (MYFLT *) inspecp->auxch.auxp;
      MYFLT *acup = (MYFLT *) p->accumer.auxch.auxp;
      MYFLT *outp = (MYFLT *) p->wacout->auxch.auxp;
      MYFLT newval;
      int32_t   n,npts = inspecp->npts;

      for (n=0;n<npts;n++) {
        newval = acup[n] + newp[n];         /* add new to old coefs */
        acup[n] = newval;                   /* sav in accumulator   */
        outp[n] = newval;                   /* & copy to output     */
      }
      p->wacout->ktimstamp = CS_KCNT; /* mark the output spec as new */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("spechist: not initialised"));
}

int32_t spfilset(CSOUND *csound, SPECFILT *p)
{
    SPECDAT *inspecp = p->wsig;
    SPECDAT *outspecp = p->wfil;
    FUNC    *ftp;
    int32_t   npts;

    if ((npts = inspecp->npts) != outspecp->npts) {  /* if inspec not matched */
      SPECset(csound,
              outspecp, (int32_t)npts);                /*   reinit the out spec */
      csound->AuxAlloc(csound,
                       (size_t)npts*2* sizeof(MYFLT),
                       &p->auxch);                   /*   & local auxspace  */
      p->coefs = (MYFLT *) p->auxch.auxp;            /*   reassign filt tbls  */
      p->states = p->coefs + npts;
    }
    if (UNLIKELY(p->coefs==NULL || p->states==NULL)) { /* RWD fix */
      return csound->InitError(csound,
                               Str("specfilt: local buffers not initialised"));
    }
    outspecp->ktimprd = inspecp->ktimprd;          /* pass other spect info */
    outspecp->nfreqs = inspecp->nfreqs;
    outspecp->dbout = inspecp->dbout;
    outspecp->downsrcp = inspecp->downsrcp;
    if (UNLIKELY((ftp=csound->FTFind(csound, p->ifhtim)) == NULL)) {
      /* if fhtim table given,    */
      return csound->InitError(csound, Str("missing htim ftable"));
    }
    {
      int32_t nn;
      int32_t phs = 0;
      int32_t inc = (int32_t)PHMASK / npts;
      int32_t lobits = ftp->lobits;
      MYFLT *ftable = ftp->ftable;
      MYFLT *flp = p->coefs;
      for (nn=0;nn<npts;nn++) {
        flp[nn] = *(ftable + (phs >> lobits));    /*  sample into coefs area */
        phs += inc;
      }
    }
    {
      int32_t nn;
      MYFLT *flp = p->coefs;
      double halftim, reittim = inspecp->ktimprd * CS_ONEDKR;
      for (nn=0;nn<npts;nn++) {
        if ((halftim = flp[nn]) > 0.)
          flp[nn] = (MYFLT)pow(0.5, reittim/halftim);
        else {
          return csound->InitError(csound,
                                   Str("htim ftable must be all-positive"));
        }
      }
    }
    csound->Warning(csound, Str("coef range: %6.3f - %6.3f\n"),
                            *p->coefs, *(p->coefs+npts-1));
    {
      MYFLT *flp = (MYFLT *) p->states;
      memset(flp,0,npts*sizeof(MYFLT)); /* clr the persist buf state mem */
    }
    outspecp->ktimstamp = 0;            /* mark the output spec as not new */
    return OK;
}

int32_t specfilt(CSOUND *csound, SPECFILT *p)
{
    if (p->wsig->ktimstamp == CS_KCNT) {   /* if input spec is new,  */
      SPECDAT *inspecp = p->wsig;
      SPECDAT *outspecp = p->wfil;
      MYFLT *newp = (MYFLT *) inspecp->auxch.auxp;
      MYFLT *outp = (MYFLT *) outspecp->auxch.auxp;
      MYFLT curval, *coefp = p->coefs;
      MYFLT *persp = p->states;
      int32_t   n,npts = inspecp->npts;

      if (UNLIKELY(newp==NULL || outp==NULL ||
                   coefp==NULL || persp==NULL))  /* RWD */
        goto err1;
      for (n=0; n<npts;n++) {                      /* for npts of inspec:     */
        outp[n] = curval = persp[n];               /*   output current point  */
        persp[n] = coefp[n] * curval + newp[n];    /*   decay & addin newval  */
      }
      outspecp->ktimstamp = CS_KCNT;      /* mark output spec as new */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("specfilt: not initialised"));
}

#define S       sizeof

static OENTRY spectra_localops[] = {
{ "spectrum", S(SPECTRUM),_QQ, 3, "w", "kiiiqoooo",
                                   (SUBR)spectset,(SUBR)spectrum },
{ "spectrum", S(SPECTRUM),_QQ, 3, "w", "aiiiqoooo",
                                   (SUBR)spectset,(SUBR)spectrum },
{ "specaddm", S(SPECADDM),_QQ, 3, "w", "wwp", (SUBR)spadmset,  (SUBR)specaddm},
{ "specdiff", S(SPECDIFF),_QQ, 3, "w", "w",   (SUBR)spdifset,  (SUBR)specdiff},
{ "specscal", S(SPECSCAL),_QQ, 3, "w", "wii", (SUBR)spsclset,  (SUBR)specscal},
{ "spechist", S(SPECHIST),_QQ, 3, "w", "w",   (SUBR)sphstset,  (SUBR)spechist},
{ "specfilt", S(SPECFILT),_QQ, 3, "w", "wi",  (SUBR)spfilset,  (SUBR)specfilt},
{ "specptrk", S(SPECPTRK),_QQ, 3, "kk", "wkiiiiiioqooo",
                                             (SUBR)sptrkset,(SUBR)specptrk},
{ "specsum",  S(SPECSUM), _QQ, 3, "k", "wo",  (SUBR)spsumset,  (SUBR)specsum },
{ "specdisp", S(SPECDISP),_QQ, 3, "",  "wio", (SUBR)spdspset,  (SUBR)specdisp},
{ "pitch", S(PITCH),   0,  3,    "kk", "aiiiiqooooojo",
                                             (SUBR)pitchset, (SUBR)pitch },
{ "maca", S(SUM),      0,  3,  "a", "y",    (SUBR)macset, (SUBR)maca  },
{ "mac", S(SUM),       0,  3,  "a", "Z",    (SUBR)macset, (SUBR)mac   },
{ "clockon", S(CLOCK), 0,  3,  "",  "i",    (SUBR)clockset, (SUBR)clockon, NULL  },
{ "clockoff", S(CLOCK),0,  3,  "",  "i",    (SUBR)clockset, (SUBR)clockoff, NULL },
{ "readclock", S(CLKRD),0, 1,  "i", "i",    (SUBR)clockread, NULL, NULL          },
{ "readscratch", S(SCRATCHPAD),0, 1, "i", "o", (SUBR)scratchread, NULL, NULL     },
{ "writescratch", S(SCRATCHPAD),0, 1, "", "io", (SUBR)scratchwrite, NULL, NULL   },
{ "pitchamdf",S(PITCHAMDF),0,3,"kk","aiioppoo",
                                     (SUBR)pitchamdfset, (SUBR)pitchamdf },
{ "hsboscil",S(HSBOSC), TR, 3, "a", "kkkiiioo",

                                    (SUBR)hsboscset,(SUBR)hsboscil },
{ "phasorbnk", S(PHSORBNK),0,3,"a", "xkio",
                                (SUBR)phsbnkset, (SUBR)phsorbnk },
{ "phasorbnk.k", S(PHSORBNK),0,3,"k", "xkio",
                                (SUBR)phsbnkset, (SUBR)kphsorbnk, NULL},
{ "adsynt",S(HSBOSC), TR, 3,  "a", "kkiiiio", (SUBR)adsyntset, (SUBR)adsynt },
{ "mpulse", S(IMPULSE), 0, 3,  "a", "kko",
                                    (SUBR)impulse_set, (SUBR)impulse },
{ "lpf18", S(LPF18), 0, 3,  "a", "axxxo",  (SUBR)lpf18set, (SUBR)lpf18db },
{ "waveset", S(BARRI), 0, 3,  "a", "ako",  (SUBR)wavesetset, (SUBR)waveset},
{ "pinkish", S(PINKISH), 0, 3, "a", "xoooo", (SUBR)pinkset, (SUBR)pinkish },
{ "noise",  S(VARI), 0, 3,  "a", "xk",   (SUBR)varicolset, (SUBR)varicol },
{ "transeg", S(TRANSEG),0, 3,  "k", "iiim",
                                           (SUBR)trnset,(SUBR)ktrnseg, NULL},
{ "transeg.a", S(TRANSEG),0, 3,  "a", "iiim",
                                           (SUBR)trnset,(SUBR)trnseg},
{ "transegb", S(TRANSEG),0, 3,  "k", "iiim",
                              (SUBR)trnset_bkpt,(SUBR)ktrnseg,(SUBR)NULL},
{ "transegb.a", S(TRANSEG),0, 3,  "a", "iiim",
                              (SUBR)trnset_bkpt,(SUBR)trnseg    },
{ "transegr", S(TRANSEG),0, 3, "k", "iiim",
                              (SUBR)trnsetr,(SUBR)ktrnsegr,(SUBR)NULL },
{ "transegr.a", S(TRANSEG),0, 3, "a", "iiim",
                              (SUBR)trnsetr,(SUBR)trnsegr      },
{ "clip", S(CLIP),      0, 3,  "a", "aiiv", (SUBR)clip_set, (SUBR)clip  },
{ "cpuprc", S(CPU_PERC),0, 1,     "",     "Si",   (SUBR)cpuperc_S, NULL, NULL   },
{ "maxalloc", S(CPU_PERC),0, 1,   "",     "Si",   (SUBR)maxalloc_S, NULL, NULL  },
{ "cpuprc", S(CPU_PERC),0, 1,     "",     "ii",   (SUBR)cpuperc, NULL, NULL   },
{ "maxalloc", S(CPU_PERC),0, 1,   "",     "ii",   (SUBR)maxalloc, NULL, NULL  },
{ "active", 0xffff                                                          },
{ "active.iS", S(INSTCNT),0,1,    "i",    "Soo",   (SUBR)instcount_S, NULL, NULL },
{ "active.kS", S(INSTCNT),0,2,    "k",    "Soo",   NULL, (SUBR)instcount_S, NULL },
{ "active.i", S(INSTCNT),0,1,     "i",    "ioo",   (SUBR)instcount, NULL, NULL },
{ "active.k", S(INSTCNT),0,2,     "k",    "koo",   NULL, (SUBR)instcount, NULL },
{ "p.i", S(PFUN),        0,1,     "i",    "i",     (SUBR)pfun, NULL, NULL     },
{ "p.k", S(PFUNK),       0,3,     "k",    "k",
                                          (SUBR)pfunk_init, (SUBR)pfunk, NULL },
{ "mute", S(MUTE), 0,1,           "",      "So",   (SUBR)mute_inst_S             },
{ "mute.i", S(MUTE), 0,1,         "",      "io",   (SUBR)mute_inst             },
{ "median", S(MEDFILT), 0, 3,     "a", "akio", (SUBR)medfiltset, (SUBR)medfilt },
{ "mediank", S(MEDFILT), 0,3,     "k", "kkio", (SUBR)medfiltset, (SUBR)kmedfilt},
};

LINKAGE_BUILTIN(spectra_localops)
