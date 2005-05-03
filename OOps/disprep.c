/*
    disprep.c:

    Copyright (C) 1991 Barry Vercoe

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

#include "cs.h"         /*                              DISPREP.C       */
#include <math.h>
#include "cwindow.h"
#include "disprep.h"
#include "dsputil.h"

static  MYFLT   *fftcoefs = NULL;     /* malloc for fourier coefs, mag or db */

void disprepRESET(ENVIRON *csound)
{
    if (fftcoefs) mfree(csound, fftcoefs);
    fftcoefs = NULL;
}

int printv(ENVIRON *csound, PRINTV *p)
{
    int    nargs = p->INOCOUNT;
    char   **txtp = p->h.optext->t.inlist->arg;
    MYFLT  **valp = p->iargs;

    csound->MessageS(csound, CSOUNDMSG_ORCH,
                     "instr %d:", (int) p->h.insdshead->p1);
    while (nargs--) {
      csound->MessageS(csound, CSOUNDMSG_ORCH,
                       "  %s = %5.3f", *txtp++, **valp++);
    }
    csound->MessageS(csound, CSOUNDMSG_ORCH, "\n");
    return OK;
}

int dspset(ENVIRON *csound, DSPLAY *p)
{
    long   npts, nprds, bufpts, totpts;
    char   *auxp;

    if (p->h.optext->t.intype == 'k')
      npts = (long)(*p->iprd * csound->ekr);
    else npts = (long)(*p->iprd * csound->esr);
    if (npts <= 0) {
      return csound->InitError(csound, Str("illegal iprd"));

    }
    if ((nprds = (long)*p->inprds) <= 1) {
      nprds = 0;
      bufpts = npts;
      totpts = npts;
    }
    else {
      bufpts = npts * nprds;
      totpts = bufpts * 2;
    }
    if ((auxp = p->auxch.auxp) == NULL || totpts != p->totpts) {
      csound->AuxAlloc(csound, totpts * sizeof(MYFLT), &p->auxch);
      auxp = p->auxch.auxp;
      p->begp = (MYFLT *) auxp;
      p->endp = p->begp + bufpts;
      p->npts = npts;
      p->nprds = nprds;
      p->bufpts = bufpts;
      p->totpts = totpts;
    }
    p->nxtp = (MYFLT *) auxp;
    p->pntcnt = npts;
    sprintf(csound->strmsg, Str("instr %d, signal %s:"),
                            p->h.insdshead->insno,
                            p->h.optext->t.inlist->arg[0]);
    dispset(&p->dwindow, (MYFLT *)auxp, bufpts, csound->strmsg,
            (int)*p->iwtflg,Str("display"));
    return OK;
}

int kdsplay(ENVIRON *csound, DSPLAY *p)
{
    MYFLT  *fp = p->nxtp;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return csound->PerfError(csound, Str("display: not initialised"));
    }
    if (!p->nprds) {
      *fp++ = *p->signal;
      if (fp >= p->endp) {
        fp = p->begp;
        display(&p->dwindow);
      }
    }
    else {
      MYFLT *fp2 = fp + p->bufpts;
      *fp++ = *p->signal;
      *fp2 = *p->signal;
      if (!(--p->pntcnt)) {
        p->pntcnt = p->npts;
        if (fp >= p->endp) {
          fp = p->begp;
          fp2 = fp + p->bufpts;
        }
        p->dwindow.fdata = fp;  /* display from fp */
        display(&p->dwindow);
      }
    }
    p->nxtp = fp;
    return OK;
}

int dsplay(ENVIRON *csound, DSPLAY *p)
{
    MYFLT  *fp = p->nxtp, *sp = p->signal, *endp = p->endp;
    int    nsmps = csound->ksmps;

    if (!p->nprds) {
      do {
        *fp++ = *sp++;
        if (fp >= endp) {
          fp = p->begp;
          display(&p->dwindow);
        }
      } while (--nsmps);
    }
    else {
      MYFLT *fp2 = fp + p->bufpts;
      do {
        *fp++ = *sp;
        *fp2++ = *sp++;
        if (!(--p->pntcnt)) {
          p->pntcnt = p->npts;
          if (fp >= endp) {
            fp = p->begp;
            fp2 = fp + p->bufpts;
          }
          p->dwindow.fdata = fp;  /* display from fp */
          display(&p->dwindow);
        }
      } while (--nsmps);
    }
    p->nxtp = fp;
    return OK;
}

int fftset(ENVIRON *csound, DSPFFT *p) /* fftset, dspfft -- calc Fast Fourier */
                                       /* Transform of collected samples and  */
                                       /* displays coefficients (mag or db)   */
{
    long  window_size, step_size;
    int   hanning;

    window_size = (long)*p->inpts;
    if (window_size > WINDMAX) {
      return csound->InitError(csound, Str("too many points requested"));
    }
    if (window_size < WINDMIN) {
      return csound->InitError(csound, Str("too few points requested"));
    }
    if (window_size < 1L || (window_size & (window_size - 1L)) != 0L) {
      return csound->InitError(csound, Str("window size must be power of two"));
    }
    if (p->h.optext->t.intype == 'k')
      step_size = (long)(*p->iprd * csound->ekr);
    else step_size = (long)(*p->iprd * csound->esr);
    if (step_size <= 0) {
      return csound->InitError(csound, Str("illegal iprd"));
    }
/*     if (inerrcnt) */
/*       return; */
    hanning = (int)*p->ihann;
    p->dbout   = (int)*p->idbout;
    p->overlap = window_size - step_size;
    if (window_size != p->windsize
        || hanning != p->hanning) {              /* if windowing has changed:  */
      long auxsiz;
      MYFLT *hWin;
      p->windsize = window_size;                   /* set new parameter values */
      p->hanning = hanning;
      p->bufp    = p->sampbuf;
      p->endp    = p->bufp + window_size;
      p->overN    = FL(1.0)/(*p->inpts);
      p->ncoefs  = window_size >>1;
      auxsiz = (window_size/2 + 1) * sizeof(MYFLT);  /* size for half window */
      csound->AuxAlloc(csound, (long)auxsiz, &p->auxch); /* alloc or realloc */
      hWin = (MYFLT *) p->auxch.auxp;
      FillHalfWin(hWin, window_size,
                  FL(1.0), hanning);  /* fill with proper values */
      if (fftcoefs == NULL)           /* room for WINDMAX*2 floats (fft size) */
        fftcoefs = (MYFLT *) mmalloc(csound, (long)WINDMAX * 2 * sizeof(MYFLT));
      sprintf(csound->strmsg, Str("instr %d, signal %s, fft (%s):"),
              p->h.insdshead->insno, p->h.optext->t.inlist->arg[0],
              p->dbout ? Str("db") : Str("mag"));
      dispset(&p->dwindow, fftcoefs, p->ncoefs, csound->strmsg,
              (int)*p->iwtflg,Str("fft"));
    }
    return OK;
}

static void d_fft(      /* perform an FFT as reqd below */
  MYFLT *sce,   /* input array - pure packed real */
  MYFLT *dst,   /* output array - packed magnitude, only half-length */
  long  size,   /* number of points in input */
  MYFLT *hWin,  /* hanning window lookup table */
  int   dbq)    /* flag: 1-> convert output into db */
{
    CopySamps(sce,dst,size);                    /* copy into scratch buffer */
    ApplyHalfWin(dst,hWin,size);
    csoundRealFFT(&cenviron, dst, (int) size);  /* perform the FFT */
    dst[size] = dst[1];
    dst[1] = dst[size + 1L] = FL(0.0);
    Rect2Polar(dst, (size >> 1) + 1);
    PackReals(dst, (size >> 1) + 1);
    if (dbq)
      Lin2DB(dst, (size >> 1) + 1);
}

int kdspfft(ENVIRON *csound, DSPFFT *p)
{
    MYFLT *bufp = p->bufp, *endp = p->endp;

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return csound->PerfError(csound, Str("dispfft: not initialised"));
    }
    if (bufp < p->sampbuf)          /* skip any spare samples */
      bufp++;
    else {                          /* then start collecting  */
      *bufp++ = *p->signal;
      if (bufp >= endp) {           /* when full, do fft:     */
        MYFLT *tp, *tplim;
        MYFLT *hWin = (MYFLT *) p->auxch.auxp;
        d_fft(p->sampbuf,fftcoefs,p->windsize,hWin,p->dbout);
        tp = fftcoefs;
        tplim = tp + p->ncoefs;
        do {
          *tp *= p->overN;          /* scale 1/N */
        } while (++tp < tplim);
        display(&p->dwindow);           /* & display */
        if (p->overlap > 0) {
          bufp = p->sampbuf;
          tp   = endp - p->overlap;
          do {
            *bufp++ = *tp++;
          } while (tp < endp);
        }
        else bufp = p->sampbuf + p->overlap;
      }
    }
    p->bufp = bufp;
    return OK;
}

int dspfft(ENVIRON *csound, DSPFFT *p)
{
    MYFLT *sigp = p->signal, *bufp = p->bufp, *endp = p->endp;
    int   nsmps = csound->ksmps;

    if (p->auxch.auxp==NULL) {
      csound->PerfError(csound, Str("dispfft: not initialised"));
      return NOTOK;
    }
    do {
      if (bufp < p->sampbuf) {            /* skip any spare samples */
        bufp++; sigp++;
      }
      else {                              /* then start collecting  */
        *bufp++ = *sigp++;
        if (bufp >= endp) {               /* when full, do fft:     */
          MYFLT *tp, *tplim;
          MYFLT *hWin = (MYFLT *) p->auxch.auxp;
          d_fft(p->sampbuf,fftcoefs,p->windsize,hWin,p->dbout);
          tp = fftcoefs;
          tplim = tp + p->ncoefs;
          do {
            *tp *= p->overN;                /* scale 1/N */
          } while (++tp < tplim);
          display(&p->dwindow);               /* & display */
          if (p->overlap > 0) {
            bufp = p->sampbuf;
            tp   = endp - p->overlap;
            do {
              *bufp++ = *tp++;
            } while(tp < endp);
          }
          else bufp = p->sampbuf + p->overlap;
        }
      }
    }
    while (--nsmps);
    p->bufp = bufp;
    return OK;
}

#define NTERMS  4
#define NCROSS  (NTERMS * (NTERMS-1))

int tempeset(ENVIRON *csound, TEMPEST *p)
{
    int  npts = 0, nptsm1, minlam = 0, maxlam, lamspan, auxsiz;
    MYFLT *fltp;
    FUNC *ftp;
    MYFLT b, iperiod = *p->iprd;

    if ((p->timcount = (int)(csound->ekr * iperiod)) <= 0)
      return csound->InitError(csound, Str("illegal iperiod"));
    if ((p->dtimcnt = (int)(csound->ekr * *p->idisprd)) < 0)
      return csound->InitError(csound, Str("illegal idisprd"));
    if ((p->tweek = *p->itweek) <= 0)
      return csound->InitError(csound, Str("illegal itweek"));
    if (iperiod != FL(0.0)) {
      if ((minlam = (int)(*p->imindur/iperiod)) <= 0)
        return csound->InitError(csound, Str("illegal imindur"));
      if ((npts = (int)(*p->imemdur / iperiod)) <= 0)
        return csound->InitError(csound, Str("illegal imemdur"));
    }
    if (*p->ihtim <= FL(0.0))
      return csound->InitError(csound, Str("illegal ihtim"));
    if (*p->istartempo <= FL(0.0))
      return csound->InitError(csound, Str("illegal startempo"));
    ftp = csound->FTFind(csound, p->ifn);
    if (ftp != NULL && *ftp->ftable == FL(0.0))
      return csound->InitError(csound, Str("ifn table begins with zero"));
    if (ftp==NULL) return NOTOK;

    nptsm1 = npts - 1;
    if (npts != p->npts || minlam != p->minlam) {
      p->npts = npts;
      p->minlam = minlam;
      p->maxlam = maxlam = nptsm1/(NTERMS-1);
      lamspan = maxlam - minlam + 1;          /* alloc 8 bufs: 2 circ, 6 lin */
      auxsiz = (npts * 5 + lamspan * 3) * sizeof(MYFLT);
      csound->AuxAlloc(csound, (long)auxsiz, &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->hbeg = fltp;     fltp += npts;
      p->hend = fltp;
      p->xbeg = fltp;     fltp += npts;
      p->xend = fltp;
      p->stmemp = fltp;   fltp += npts;
      p->linexp = fltp;   fltp += npts;
      p->ftable = fltp;   fltp += npts;
      p->xscale = fltp;   fltp += lamspan;
      p->lmults = fltp;   fltp += lamspan;
      p->lambdas = (short *) fltp;
      p->stmemnow = p->stmemp + nptsm1;
    }
    if (p->dtimcnt && !(p->dwindow.windid)) {  /* init to display stmem & exp */
      sprintf(csound->strmsg, "instr %d tempest:", p->h.insdshead->insno);
      dispset(&p->dwindow, p->stmemp, (long) npts * 2, csound->strmsg, 0,
              Str("tempest"));
      p->dwindow.danflag = 1;                    /* for mid-scale axis */
    }
    {
      MYFLT *funp = ftp->ftable;
      long phs = 0;
      long inc = (long)PHMASK / npts;
      long nn, lobits = ftp->lobits;
      for (fltp=p->hbeg, nn=npts*4; nn--; )   /* clr 2 circ & 1st 2 lin bufs */
        *fltp++ = FL(0.0);
      for (fltp=p->ftable+npts, nn=npts; nn--; ) {  /* now sample the ftable  */
        *--fltp = *(funp + (phs >> lobits));        /* backwards into tbl buf */
        phs += inc;
      }
    }
    {
      MYFLT *tblp, sumraw, sumsqr;      /* calc the CROSS prod scalers */
      long terms;
      long lambda, maxlam;
      MYFLT crossprods, RMS, *endtable = p->ftable + nptsm1;
      /*            MYFLT coef, log001 = -6.9078; */
      MYFLT *xscale = p->xscale;

      p->ncross = (MYFLT) NCROSS;
      for (lambda=p->minlam,maxlam=p->maxlam; lambda <= maxlam; lambda++) {
        tblp = endtable;
        sumraw = *tblp;
        sumsqr = *tblp * *tblp;
        terms = NTERMS - 1;
        do {
          tblp -= lambda;
          sumraw += *tblp;
          sumsqr += *tblp * *tblp;
        } while (--terms);
        crossprods = sumraw * sumraw - sumsqr;
        RMS = (MYFLT) sqrt(crossprods / p->ncross);
        /*              coef = exp(log001 * lambda / npts);
         *xscale++ = coef / RMS / (NTERMS - 1);  */
        *xscale++ = FL(0.05)/ RMS / lambda;
      }
    }
    /* calc input lo-pass filter coefs */
    b = FL(2.0) - (MYFLT)cos((*p->ihp * 6.28318 / csound->ekr));
    p->coef1 = b - (MYFLT)sqrt(b * b - 1.0);
    p->coef0 = FL(1.0) - p->coef1;
    p->yt1 = FL(0.0);
    p->fwdcoef = (MYFLT)pow(0.5, p->timcount/csound->ekr/(*p->ihtim));
    p->fwdmask = FL(0.0);
    csound->Message(csound,
                    Str("kin lopass coef1 %6.4f, fwd mask coef1 %6.4f\n"),
                    p->coef1, p->fwdcoef);
    p->thresh = *p->ithresh;            /* record incoming loudness threshold */
    p->xfdbak = *p->ixfdbak;            /*    & expectation feedback fraction */
    p->tempscal = FL(60.0) * csound->ekr / p->timcount;
    p->avglam = p->tempscal / *p->istartempo;       /* init the tempo factors */
    p->tempo = FL(0.0);
    p->hcur = p->hbeg;                              /* init the circular ptrs */
    p->xcur = p->xbeg;
    p->countdown = p->timcount;                     /* & prime the countdowns */
    p->dcntdown = p->dtimcnt;
    return OK;
}

#define NMULTS 5
static MYFLT lenmults[NMULTS] = { FL(3.0), FL(2.0), FL(1.0), FL(0.5), FL(0.333)};
static MYFLT lenfracs[NMULTS*2] = { FL(0.30), FL(0.3667), FL(0.45), FL(0.55),
                                    FL(0.92), FL(1.08), FL(1.88), FL(2.12),
                                    FL(2.85), FL(3.15) };

int tempest(ENVIRON *csound, TEMPEST *p)
{
    p->yt1 = p->coef0 * *p->kin + p->coef1 * p->yt1;  /* get lo-pass of kinput */

    if (p->auxch.auxp==NULL) { /* RWD fix */
      return csound->PerfError(csound, Str("tempest: not initialised"));
    }
    if (!(--p->countdown)) {                          /* then on countdown:    */
      MYFLT *memp;
      MYFLT kin, expect, *xcur = p->xcur;           /* xcur from prv pass    */
      MYFLT lamtot = FL(0.0), weightot = FL(0.0);

      p->countdown = p->timcount;           /* reset the countdown            */
      expect = *xcur;                       /* get expected val from prv calc */
      *xcur++ = FL(0.0);                    /*    & clear the loc it occupied */
      if (xcur >= p->xend) xcur = p->xbeg;  /* xcur now points to cur xarray  */
      p->xcur = xcur;
      if ((kin = *p->kin - p->yt1) < FL(0.0))  kin = FL(0.0);
      { /* ignore input below lopass */
        MYFLT *hcur = p->hcur;
        MYFLT *hend = p->hend;
        MYFLT *tblp = p->ftable;
        long  wrap;
        *hcur++ = kin + expect * p->xfdbak;   /* join insample & expect val */
        if (hcur < hend)  p->hcur = hcur;     /* stor pntr for next insamp  */
        else p->hcur = p->hbeg;
        wrap = hcur - p->hbeg;
        memp = p->stmemp;
        while (hcur < hend)                   /* now lineariz & envlp hbuf */
          *memp++ = *hcur++ * *tblp++;      /*  into st_mem buf          */
        for (hcur=p->hbeg; wrap--; )
          *memp++ = *hcur++ * *tblp++;
      }
      if (p->yt1 > p->thresh            /* if lo-pass of kinput now significant */
          && kin > p->fwdmask) {          /*    & kin > masking due to prev kin */
        MYFLT sumraw, sumsqr;
        long lambda, minlam, maxlam;
        int  terms, nn, npts = p->npts;
        MYFLT mult, crossprods, RMScross, RMStot, unilam, rd;
        MYFLT *xend = p->xend;
        /*                MYFLT *xscale = p->xscale; */
        MYFLT *mults, *fracs, *mulp;
        short minlen, maxlen, *lenp, *endlens;

        for (memp=p->stmemp,nn=npts,sumsqr=FL(0.0); nn--; memp++)
          sumsqr += *memp * *memp;
        RMStot = (MYFLT)sqrt(sumsqr/npts);
        /*        csound->Message(csound, "RMStot = %6.1f\n", RMStot);    */
        mults = lenmults;                       /* use the static lentables  */
        fracs = lenfracs;
        mulp = p->lmults;
        lenp = p->lambdas;
        minlam = p->minlam;
        maxlam = p->maxlam;
        nn = NMULTS;
        do {
          mult = *mults++;
          minlen = (short)(p->avglam * *fracs++); /* & the current avglam  */
          maxlen = (short)(p->avglam * *fracs++);
          if (minlen >= minlam && maxlen <= maxlam)
            do {
              *lenp++ = minlen++;         /*   creat lst of lambda lens */
              *mulp++ = mult;             /*   & their unit multipliers */
            } while (minlen <= maxlen);
        } while (--nn);
        endlens = lenp;                         /* now for these lambda lens: */
        for (lenp=p->lambdas,mulp=p->lmults; lenp < endlens; ) {
          lambda = *lenp++;
          mult = *mulp++;
          memp = p->stmemnow;
          sumraw = *memp;
          sumsqr = *memp * *memp;             /* autocorrelate the st_mem buf */
          terms = NTERMS - 1;
          do {
            memp -= lambda;
            sumraw += *memp;
            sumsqr += *memp * *memp;
          } while (--terms);
          crossprods = sumraw * sumraw - sumsqr;
          RMScross = (MYFLT)sqrt(crossprods / p->ncross);
          if (RMScross < FL(1.4) * RMStot)         /* if RMScross significant:   */
            continue;
          /*   csound->Message(csound, "RMScross = %6.1f, lambda = %ld\n",
                                       RMScross, lambda);  */
          /*                  RMS *= *xscale++;     */
          unilam = lambda * mult;               /*    get unit lambda implied */
          lamtot += unilam * RMScross;          /*    & add weighted to total */
          weightot += RMScross;
          RMScross /= FL(5.0);
          memp = xcur - 1;                /* multiply project into expect buf */
          for (terms=1; terms < NTERMS; ++terms) {
            if ((memp += (lambda-terms+1)) >= xend)
              memp -= npts;
            for (nn=terms,rd=RMScross/terms; nn--; ) {
              *memp++ += rd;
              if (memp >= xend)
                memp -= npts;
            }
          }
        }
      }
      if (weightot) {                                     /* if accumed weights, */
        p->avglam = (p->avglam + lamtot/weightot)/FL(2.0); /* update the avglam */
        p->avglam /= p->tweek;
        p->tempo = p->tempscal / p->avglam;         /*   & cvt to tempo    */
/*      csound->Message(csound, "lamtot %6.2f, weightot %6.2f, "
                                "newavglam %6.2f, tempo %6.2f\n",
                                lamtot, weightot, p->avglam, p->tempo);    */
/*      csound->Message(csound, "%6.1f\n", p->tempo);  */
        csound->Message(csound, ".");
      }
      else p->tempo = FL(0.0);              /* else tempo is 0     */
      p->fwdmask = p->fwdmask * p->fwdcoef + kin;
    }
    if (!(--p->dcntdown)) {                 /* on display countdown    */
      MYFLT *linp = p->linexp;
      MYFLT *xcur = p->xcur;
      MYFLT *xend = p->xend;
      long wrap = xcur - p->xbeg;
      while (xcur < xend)                   /* lineariz the circ xbuf */
        *linp++ = *xcur++;                  /*  into linexp buf       */
      for (xcur=p->xbeg; wrap--; )
        *linp++ = *xcur++;
      display(&p->dwindow);                 /* display double window  */
      p->dcntdown = p->dtimcnt;             /*   & reset the counter  */
    }
    *p->kout = p->tempo;                    /* put current tempo */
    return OK;
}

