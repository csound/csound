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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"         /*                      DISPREP.C       */
#include <math.h>
#include "cwindow.h"
#include "disprep.h"


#ifdef MSVC                   /* Thanks to Richard Dobson */
# define hypot _hypot
#endif

int32_t printv(CSOUND *csound, PRINTV *p)
{
    int32_t    nargs = p->INOCOUNT;
    char   **txtp = p->h.optext->t.inlist->arg;
    MYFLT  **valp = p->iargs;

    csound->MessageS(csound, CSOUNDMSG_ORCH,
                     "instr %d:", (int32_t) p->h.insdshead->p1.value);
    while (nargs--) {
      csound->MessageS(csound, CSOUNDMSG_ORCH,
                       "  %s = %5.3f", *txtp++, **valp++);
    }
    csound->MessageS(csound, CSOUNDMSG_ORCH, "\n");
    return OK;
}

int32_t fdspset(CSOUND *csound, FSIGDISP *p){
    char strmsg[256];
    p->size = p->fin->N/2 + 1;
    if ((*p->points != (MYFLT) 0) && (p->size > (int32_t) *p->points)) {
      p->size = *p->points;
    }
    if ((p->fdata.auxp == NULL) ||
        (p->fdata.size < (uint32_t) (p->size*sizeof(MYFLT)))) {
      csound->AuxAlloc(csound, p->size*sizeof(MYFLT), &p->fdata);
    }
    snprintf(strmsg, 256, Str("instr %d, pvs-signal %s:"),
            (int32_t) p->h.insdshead->p1.value, p->h.optext->t.inlist->arg[0]);
    dispset(csound, &p->dwindow, (MYFLT*) p->fdata.auxp, p->size, strmsg,
                    (int32_t) *p->flag, Str("display"));
    p->lastframe = 0;
    return OK;

}

int32_t fdsplay(CSOUND *csound, FSIGDISP *p)
{
    float *fin   = p->fin->frame.auxp;
    MYFLT *pdata = p->fdata.auxp;
    int32_t i,k, end = p->size;

    if (p->lastframe < p->fin->framecount) {
      for (i=0, k=0; k < end; i+=2,k++) pdata[k] = fin[i];
      display(csound, &p->dwindow);
      p->lastframe = p->fin->framecount;
    }
    return OK;
 }

int32_t dspset(CSOUND *csound, DSPLAY *p)
{
    int32_t  npts, nprds, bufpts, totpts;
    char   *auxp;
    char   strmsg[256];

    if (p->h.optext->t.intype == 'k')
      npts = (int32_t)(*p->iprd * CS_EKR);
    else npts = (int32_t)(*p->iprd * csound->esr);
    if (UNLIKELY(npts <= 0)) {
      return csound->InitError(csound, Str("illegal iprd in display"));

    }
    if ((nprds = (int32_t)*p->inprds) <= 1) {
      nprds  = 0;
      bufpts = npts;
      totpts = npts;
    }
    else {
      bufpts = npts * nprds;
      totpts = bufpts * 2;
    }
    if ((auxp = p->auxch.auxp) == NULL || totpts != p->totpts) {
      csound->AuxAlloc(csound, totpts * sizeof(MYFLT), &p->auxch);
      auxp      = p->auxch.auxp;
      p->begp   = (MYFLT *) auxp;
      p->endp   = p->begp + bufpts;
      p->npts   = npts;
      p->nprds  = nprds;
      p->bufpts = bufpts;
      p->totpts = totpts;
    }
    p->nxtp = (MYFLT *) auxp;
    p->pntcnt = npts;
    snprintf(strmsg, 256, Str("instr %d, signal %s:"),
             (int32_t) p->h.insdshead->p1.value, p->h.optext->t.inlist->arg[0]);
    dispset(csound, &p->dwindow, (MYFLT*) auxp, bufpts, strmsg,
            (int32_t) *p->iwtflg, Str("display"));
    return OK;
}

int32_t kdsplay(CSOUND *csound, DSPLAY *p)
{
    MYFLT  *fp = p->nxtp;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (!p->nprds) {
      *fp++ = *p->signal;
      if (fp >= p->endp) {
        fp = p->begp;
        display(csound, &p->dwindow);
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
          fp2 = fp + p->bufpts; /* FIXME: Unused */
        }
        p->dwindow.fdata = fp;  /* display from fp */
        display(csound, &p->dwindow);
      }
    }
    p->nxtp = fp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("display: not initialised"));
}

int32_t dsplay(CSOUND *csound, DSPLAY *p)
{
    MYFLT  *fp = p->nxtp, *sp = p->signal, *endp = p->endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    nsmps -= early;
    if (!p->nprds) {
      for (n=offset; n<nsmps; n++) {
        *fp++ = sp[n];
        if (fp >= endp) {
          fp = p->begp;
          display(csound, &p->dwindow);
        }
      }
    }
    else {
      MYFLT *fp2 = fp + p->bufpts;
      for (n=offset; n<nsmps; n++) {
        *fp++ = sp[n];
        *fp2++ = sp[n];
        if (!(--p->pntcnt)) {
          p->pntcnt = p->npts;
          if (fp >= endp) {
            fp = p->begp;
            fp2 = fp + p->bufpts;
          }
          p->dwindow.fdata = fp;  /* display from fp */
          display(csound, &p->dwindow);
        }
      }
    }
    p->nxtp = fp;
    return OK;
}

/* Write window coefs into buffer, don't malloc */
static void FillHalfWin(MYFLT *wBuf, int32_t size, MYFLT max, int32_t hannq)
    /* 1 => hanning window else hamming */
{
    MYFLT       a,b;
    int32_t        i;

    if (hannq) {
      a = FL(0.50);
      b = FL(0.50);
    }
    else {
      a = FL(0.54);
      b = FL(0.46);
    }

    if (wBuf!= NULL) {        /* NB: size/2 + 1 long - just indep terms */
      MYFLT tmp;
      size /= 2;              /* to fix scaling */
      tmp = PI_F/(MYFLT)size; /* optimisatuon?? */
      for (i=0; i<=size;++i)
        wBuf[i] = max * (a-b*COS(tmp*(MYFLT)i) );
    }
    return;
}

static void ApplyHalfWin(MYFLT *buf, MYFLT *win, int32_t len)
{   /* Window only store 1st half, is symmetric */
    int32_t    j;
    int32_t    lenOn2 = (len/2);

    for (j=lenOn2+1; j--; )
      *buf++ *= *win++;
    for (j =len-lenOn2-1, win--; j--; )
      *buf++ *= *--win;
}

int32_t fftset(CSOUND *csound, DSPFFT *p) /* fftset, dspfft -- calc Fast Fourier */
                                      /* Transform of collected samples and  */
                                      /* displays coefficients (mag or db)   */
{
    int32_t window_size, step_size;
    int32_t   hanning;
    char  strmsg[256];
    int32_t  minbin, maxbin;
    minbin = *p->imin;
    maxbin = *p->imax;

    if (p->smpbuf.auxp == NULL)
      csound->AuxAlloc(csound, sizeof(MYFLT)*WINDMAX, &(p->smpbuf));

    p->sampbuf = (MYFLT *) p->smpbuf.auxp;

    window_size = (int32_t)*p->inpts;
    if (UNLIKELY(window_size > WINDMAX)) {
      return csound->InitError(csound, Str("too many points requested (%d)"), window_size);
    }
    if (UNLIKELY(window_size < WINDMIN)) {
      return csound->InitError(csound, Str("too few points requested (%d), minimum is %d"),
                               window_size, WINDMIN);
    }
    if (UNLIKELY(window_size < 1L || (window_size & (window_size - 1L)) != 0L)) {
      return csound->InitError(csound, Str("window size must be power of two"));
    }
    if (p->h.optext->t.intype == 'k')
      step_size = (int32_t)(*p->iprd * CS_EKR);
    else step_size = (int32_t)(*p->iprd * csound->esr);
    if (UNLIKELY(step_size <= 0)) {
      return csound->InitError(csound, Str("illegal iprd in ffy display"));
    }
    hanning = (int32_t)*p->ihann;
    p->dbout   = (int32_t)*p->idbout;
    p->overlap = window_size - step_size;



    if ( (maxbin - minbin) != p->npts ||
         minbin != p->start         ||
         window_size != p->windsize ||
         hanning != p->hanning) {             /* if windowing has changed:  */
      int32_t auxsiz;
      MYFLT *hWin;
      p->windsize = window_size;                /* set new parameter values */
      p->hanning = hanning;
      p->bufp    = p->sampbuf;
      p->endp    = p->bufp + window_size;
      p->overN   = FL(1.0)/(*p->inpts);
      p->ncoefs  = window_size >>1;
      auxsiz = (window_size/2 + 1) * sizeof(MYFLT);  /* size for half window */
      csound->AuxAlloc(csound, (int32_t)auxsiz, &p->auxch); /* alloc or realloc */
      hWin = (MYFLT *) p->auxch.auxp;
      FillHalfWin(hWin, window_size,
                  FL(1.0), hanning);            /* fill with proper values */
      if (csound->disprep_fftcoefs == NULL) {
        /* room for WINDMAX*2 floats (fft size) */
        csound->disprep_fftcoefs = (MYFLT*) csound->Malloc(csound, WINDMAX * 2
                                                            * sizeof(MYFLT));
      }
      snprintf(strmsg, 256, Str("instr %d, signal %s, fft (%s):"),
               (int32_t) p->h.insdshead->p1.value, p->h.optext->t.inlist->arg[0],
               p->dbout ? Str("db") : Str("mag"));
      if (maxbin == 0) maxbin = p->ncoefs;
      if (minbin > maxbin) minbin = 0;
      p->npts = maxbin - minbin;
      p->start = minbin;
      dispset(csound, &p->dwindow,
              csound->disprep_fftcoefs+p->start, p->npts, strmsg,
              (int32_t) *p->iwtflg, Str("fft"));
       }

    return OK;
}

/* pack re,im,re,im into re,re */
static void PackReals(MYFLT *buffer, int32_t size)
{
    MYFLT   *b2 = buffer;

    ++size;
    while (--size) {
      *b2++ = *buffer++;
      ++buffer;
    }
}

/* Convert Real & Imaginary spectra into Amplitude & Phase */
static void Rect2Polar(MYFLT *buffer, int32_t size, MYFLT scal)
{
    int32_t   i;
    MYFLT   *real,*imag;
    MYFLT   re,im;
    MYFLT   mag;

    real = buffer;
    imag = buffer+1;
    for (i = 0; i < size; i++) {
      re = real[i+i]*scal;
      im = imag[i+i]*scal;
      real[2L*i] = mag = HYPOT(re,im);
      if (mag == FL(0.0))
        imag[i+i] = FL(0.0);
      else
        imag[i+i] = ATAN2(im,re);
    }
}

/* packed buffer ie. reals, not complex */
static void Lin2DB(MYFLT *buffer, int32_t size)
{
    while (size--) {
      if (*buffer > 0.0)
      *buffer = /* FL(20.0)*log10 */ FL(8.68589)*LOG(*buffer);
      buffer++;
    }
}

static void d_fft(      /* perform an FFT as reqd below */
  CSOUND *csound,
  MYFLT  *sce,   /* input array - pure packed real */
  MYFLT  *dst,   /* output array - packed magnitude, only half-length */
  int32_t  size,   /* number of points in input */
  MYFLT  *hWin,  /* hanning window lookup table */
  int32_t    dbq, MYFLT scal)    /* flag: 1-> convert output into db */
{
    memcpy(dst, sce, sizeof(MYFLT) * size);     /* copy into scratch buffer */
    ApplyHalfWin(dst, hWin, size);
    csound->RealFFT(csound, dst, (int32_t) size);   /* perform the FFT */
    dst[size] = dst[1];
    dst[1] = dst[size + 1L] = FL(0.0);
    Rect2Polar(dst, (size >> 1) + 1, scal);
    PackReals(dst, (size >> 1) + 1);
    if (dbq)
      Lin2DB(dst, (size >> 1) + 1);
}

int32_t kdspfft(CSOUND *csound, DSPFFT *p)
{
    MYFLT *bufp = p->bufp, *endp = p->endp;

    if (p->dbout) p->dwindow.polarity = NEGPOL;
          else p->dwindow.polarity = POSPOL;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (bufp < p->sampbuf)          /* skip any spare samples */
      bufp++;
    else {                          /* then start collecting  */
      *bufp++ = *p->signal;
      if (bufp >= endp) {           /* when full, do fft:     */
        MYFLT *tp;
        //MYFLT *tplim;
        MYFLT *hWin = (MYFLT *) p->auxch.auxp;
        d_fft(csound, p->sampbuf, csound->disprep_fftcoefs,
              p->windsize, hWin, p->dbout, p->overN);
        //tp = csound->disprep_fftcoefs; UNUSED
        //tplim = tp + p->ncoefs;
        //do {
        // *tp *= p->overN;            /* scale 1/N */
        //} while (++tp < tplim);
        display(csound, &p->dwindow); /* & display */
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
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("dispfft: not initialised"));
}

int32_t dspfft(CSOUND *csound, DSPFFT *p)
{
    MYFLT *sigp = p->signal, *bufp = p->bufp, *endp = p->endp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (p->dbout) {
       p->dwindow.polarity = NEGPOL;
       p->dwindow.absflag = 1;
    }
    else p->dwindow.polarity = POSPOL;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    nsmps -= early;
    for (n=offset; n<nsmps; n++) {
      if (bufp < p->sampbuf) {            /* skip any spare samples */
        bufp++; sigp++;
      }
      else {                              /* then start collecting  */
        *bufp++ = *sigp++;
        if (bufp >= endp) {               /* when full, do fft:     */
          MYFLT *tp;
          //MYFLT *tplim;
          MYFLT *hWin = (MYFLT *) p->auxch.auxp;
          d_fft(csound, p->sampbuf, csound->disprep_fftcoefs,
                p->windsize, hWin, p->dbout, p->overN);
          //tp = csound->disprep_fftcoefs; UNUSED
          //tplim = tp + p->ncoefs;
          //do {
          //  *tp *= p->overN;              /* scale 1/N */
          //} while (++tp < tplim);

          display(csound, &p->dwindow);   /* & display */
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
    }
    p->bufp = bufp;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("dispfft: not initialised"));
}

#define NTERMS  4
#define NCROSS  (NTERMS * (NTERMS-1))

int32_t tempeset(CSOUND *csound, TEMPEST *p)
{
    int32_t   npts = 0, nptsm1, minlam = 0, maxlam, lamspan, auxsiz;
    MYFLT *fltp;
    FUNC  *ftp;
    MYFLT b, iperiod = *p->iprd;
    char  strmsg[256];

    if (UNLIKELY((p->timcount = (int32_t)(CS_EKR * iperiod)) <= 0))
      return csound->InitError(csound, Str("illegal iperiod"));
    if (UNLIKELY((p->dtimcnt = (int32_t)(CS_EKR * *p->idisprd)) < 0))
      return csound->InitError(csound, Str("illegal idisprd"));
    if (UNLIKELY((p->tweek = *p->itweek) <= 0))
      return csound->InitError(csound, Str("illegal itweek"));
    if (iperiod != FL(0.0)) {
      if (UNLIKELY((minlam = (int32_t)(*p->imindur/iperiod)) <= 0))
        return csound->InitError(csound, Str("illegal imindur"));
      if (UNLIKELY((npts = (int32_t)(*p->imemdur / iperiod)) <= 0))
        return csound->InitError(csound, Str("illegal imemdur"));
    }
    if (UNLIKELY(*p->ihtim <= FL(0.0)))
      return csound->InitError(csound, Str("illegal ihtim"));
    if (UNLIKELY(*p->istartempo <= FL(0.0)))
      return csound->InitError(csound, Str("illegal startempo"));
    ftp = csound->FTFind(csound, p->ifn);
    if (UNLIKELY(ftp != NULL && *ftp->ftable == FL(0.0)))
      return csound->InitError(csound, Str("ifn table begins with zero"));
    if (UNLIKELY(ftp==NULL)) return NOTOK;

    if (npts==0) return NOTOK;

    nptsm1 = npts - 1;
    if (npts != p->npts || minlam != p->minlam) {
      p->npts = npts;
      p->minlam = minlam;
      p->maxlam = maxlam = nptsm1/(NTERMS-1);
      lamspan = maxlam - minlam + 1;          /* alloc 8 bufs: 2 circ, 6 lin */
      auxsiz = (npts * 5 + lamspan * 3) * sizeof(MYFLT);
      csound->AuxAlloc(csound, (int32_t)auxsiz, &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->hbeg   = fltp;   fltp += npts;
      p->hend   = fltp;
      p->xbeg   = fltp;   fltp += npts;
      p->xend   = fltp;
      p->stmemp = fltp;   fltp += npts;
      p->linexp = fltp;   fltp += npts;
      p->ftable = fltp;   fltp += npts;
      p->xscale = fltp;   fltp += lamspan;
      p->lmults = fltp;   fltp += lamspan;
      p->lambdas = (int16 *) fltp;
      p->stmemnow = p->stmemp + nptsm1;
    }
    if (p->dtimcnt && !(p->dwindow.windid)) {  /* init to display stmem & exp */
      snprintf(strmsg, 256, "instr %d tempest:", (int32_t) p->h.insdshead->p1.value);
      dispset(csound, &p->dwindow, p->stmemp, (int32_t)npts * 2, strmsg, 0,
                      Str("tempest"));
      p->dwindow.danflag = 1;                    /* for mid-scale axis */
    }
    {
      MYFLT *funp = ftp->ftable;
      int32_t phs = 0;
      int32_t inc = (int32_t)PHMASK / npts;
      int32_t nn, lobits = ftp->lobits;
      for (fltp=p->hbeg, nn=npts*4; nn--; )   /* clr 2 circ & 1st 2 lin bufs */
        *fltp++ = FL(0.0);
      for (fltp=p->ftable+npts, nn=npts; nn--; ) {  /* now sample the ftable  */
        *--fltp = *(funp + (phs >> lobits));        /* backwards into tbl buf */
        phs += inc;
      }
    }
    {
      MYFLT *tblp, sumraw, sumsqr;      /* calc the CROSS prod scalers */
      int32_t terms;
      int32_t lambda, maxlam;
      MYFLT crossprods, RMS, *endtable = p->ftable + nptsm1;
   /* MYFLT coef, log001 = -6.9078; */
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
        RMS = SQRT(crossprods / p->ncross);
    /*  coef = exp(log001 * lambda / npts);
        *xscale++ = coef / RMS / (NTERMS - 1);  */
        *xscale++ = FL(0.05)/ RMS / lambda;
      }
    }
    /* calc input lo-pass filter coefs */
    b = FL(2.0) - COS((*p->ihp * 6.28318 * CS_ONEDKR));
    p->coef1 = b - SQRT(b * b - FL(1.0));
    p->coef0 = FL(1.0) - p->coef1;
    p->yt1 = FL(0.0);
    p->fwdcoef = POWER(FL(0.5), p->timcount*CS_ONEDKR/(*p->ihtim));
    p->fwdmask = FL(0.0);
#ifdef DEBUG
    csound->Message(csound,
                    Str("kin lopass coef1 %6.4f, fwd mask coef1 %6.4f\n"),
                    p->coef1, p->fwdcoef);
#endif
    p->thresh = *p->ithresh;            /* record incoming loudness threshold */
    p->xfdbak = *p->ixfdbak;            /*    & expectation feedback fraction */
    p->tempscal = FL(60.0) * CS_EKR / p->timcount;
    p->avglam = p->tempscal / *p->istartempo;       /* init the tempo factors */
    p->tempo = FL(0.0);
    p->hcur = p->hbeg;                              /* init the circular ptrs */
    p->xcur = p->xbeg;
    p->countdown = p->timcount;                     /* & prime the countdowns */
    p->dcntdown = p->dtimcnt;
    return OK;
}

#define NMULTS 5

static const MYFLT lenmults[NMULTS] = {
    FL(3.0), FL(2.0), FL(1.0), FL(0.5), FL(0.333)
};

static const MYFLT lenfracs[NMULTS*2] = {
    FL(0.30), FL(0.3667),   FL(0.45), FL(0.55),     FL(0.92), FL(1.08),
    FL(1.88), FL(2.12),     FL(2.85), FL(3.15)
};

int32_t tempest(CSOUND *csound, TEMPEST *p)
{
    p->yt1 = p->coef0 * *p->kin + p->coef1 * p->yt1; /* get lo-pass of kinput */

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1; /* RWD fix */
    if (!(--p->countdown)) {                         /* then on countdown:    */
      MYFLT *memp;
      MYFLT kin, expect, *xcur = p->xcur;            /* xcur from prv pass    */
      MYFLT lamtot = FL(0.0), weightot = FL(0.0);

      p->countdown = p->timcount;           /* reset the countdown            */
      expect = *xcur;                       /* get expected val from prv calc */
      *xcur++ = FL(0.0);                    /*    & clear the loc it occupied */
      if (xcur >= p->xend) xcur = p->xbeg;  /* xcur now points to cur xarray  */
      p->xcur = xcur;
#ifdef DEBUG
      csound->Message(csound, "**kin -> %f (%f,%f)\n",
                      *p->kin - p->yt1, *p->kin, p->yt1);
#endif
      if ((kin = *p->kin - p->yt1) < FL(0.0))
        kin = FL(0.0);
      { /* ignore input below lopass */
        MYFLT *hcur = p->hcur;
        MYFLT *hend = p->hend;
        MYFLT *tblp = p->ftable;
        int32_t  wrap;
        *hcur++ = kin + expect * p->xfdbak;   /* join insample & expect val */
        if (hcur < hend)  p->hcur = hcur;     /* stor pntr for next insamp  */
        else p->hcur = p->hbeg;
        wrap = hcur - p->hbeg;
        memp = p->stmemp;
        while (hcur < hend)                   /* now lineariz & envlp hbuf */
          *memp++ = *hcur++ * *tblp++;        /*  into st_mem buf          */
        for (hcur=p->hbeg; wrap--; )
          *memp++ = *hcur++ * *tblp++;
      }
      if (p->yt1 > p->thresh        /* if lo-pass of kinput now significant */
          && kin > p->fwdmask) {    /*    & kin > masking due to prev kin */
        MYFLT sumraw, sumsqr;
        int32_t lambda, minlam, maxlam;
        int32_t  terms, nn, npts = p->npts;
        MYFLT mult, crossprods, RMScross, RMStot, unilam, rd;
        MYFLT *xend = p->xend;
   /*   MYFLT *xscale = p->xscale;   */
        const MYFLT *mults, *fracs;
        MYFLT *mulp;
        int16 minlen, maxlen, *lenp, *endlens;

        for (memp=p->stmemp,nn=npts,sumsqr=FL(0.0); nn--; memp++)
          sumsqr += *memp * *memp;
        RMStot = (MYFLT)sqrt(sumsqr/npts);
#ifdef DEBUG
        csound->Message(csound, "RMStot = %6.1f\n", RMStot);
#endif
        mults = lenmults;                     /* use the static lentables  */
        fracs = lenfracs;
        mulp = p->lmults;
        lenp = p->lambdas;
        minlam = p->minlam;
        maxlam = p->maxlam;
        nn = NMULTS;
        do {
          mult = *mults++;
          minlen = (int16)(p->avglam * *fracs++); /* & the current avglam  */
          maxlen = (int16)(p->avglam * *fracs++);
          if (minlen >= minlam && maxlen <= maxlam)
            do {
              *lenp++ = minlen++;         /*   creat lst of lambda lens */
              *mulp++ = mult;             /*   & their unit multipliers */
            } while (minlen <= maxlen);
        } while (--nn);
        endlens = lenp;                       /* now for these lambda lens: */
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
          if (crossprods >= 0)
            RMScross = SQRT(crossprods / p->ncross);
          else
            RMScross = FL(0.0);
          if (RMScross < FL(1.4) * RMStot)    /* if RMScross significant:   */
            continue;
#ifdef DEBUG
          csound->Message(csound, "RMScross = %6.1f, lambda = %ld\n",
                                  RMScross, lambda);
#endif
       /* RMS *= *xscale++; */
          unilam = lambda * mult;             /*    get unit lambda implied */
          lamtot += unilam * RMScross;        /*    & add weighted to total */
          weightot += RMScross;
          RMScross /= FL(5.0);
          memp = xcur - 1;              /* multiply project into expect buf */
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
      if (weightot) {                         /* if accumed weights, */
        p->avglam = (p->avglam + lamtot/weightot)/FL(2.0); /* update the avglam */
        p->avglam /= p->tweek;
        p->tempo = p->tempscal / p->avglam;   /*   & cvt to tempo    */
#ifdef DEBUG
        csound->Message(csound, "lamtot %6.2f, weightot %6.2f, "
                                "newavglam %6.2f, tempo %6.2f\n",
                        lamtot, weightot, p->avglam, p->tempo);
#endif
      }
      else {
        if (kin < -(p->fwdmask)) {
          p->tempo = FL(0.0);                 /* else tempo is 0     */
        }
      }
      p->fwdmask = p->fwdmask * p->fwdcoef + kin;
    }
    if (!(--p->dcntdown)) {                 /* on display countdown    */
      MYFLT *linp = p->linexp;
      MYFLT *xcur = p->xcur;
      MYFLT *xend = p->xend;
      int32_t wrap = xcur - p->xbeg;
      while (xcur < xend)                   /* lineariz the circ xbuf */
        *linp++ = *xcur++;                  /*  into linexp buf       */
      for (xcur=p->xbeg; wrap--; )
        *linp++ = *xcur++;
      display(csound, &p->dwindow);         /* display double window  */
      p->dcntdown = p->dtimcnt;             /*   & reset the counter  */
    }
/*  if (p->tempo != 0.0)  */
    *p->kout = p->tempo;                    /* put current tempo */
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                               Str("tempest: not initialised"));
}
