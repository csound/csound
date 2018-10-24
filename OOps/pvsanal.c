/*
    pvsanal.c:

    Copyright (C) 2002 Richard Dobson
              (C) 2007 John ffitch/Richard Dobson (SDFT)
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

/* pvsanal.c */
/* functions based on CARL pvoc.c (c) Mark Dolson.
   The CARL software distribution is due to be released under the GNU LPGL.
*/

#include <math.h>
#include "csoundCore.h"
#include "pstream.h"

        double  besseli(double x);
static  void    hamming(MYFLT *win, int32_t winLen, int32_t even);
static  void    vonhann(MYFLT *win, int32_t winLen, int32_t even);

static  void    generate_frame(CSOUND *, PVSANAL *p);
static  void    process_frame(CSOUND *, PVSYNTH *p);

/* generate half-window */

static CS_NOINLINE int32_t PVS_CreateWindow(CSOUND *csound, MYFLT *buf,
                                        int32_t type, int32_t winLen)
{
    double  fpos, inc;
    MYFLT   *ftable;
    int32_t     i, n, flen, even;

    even = (winLen + 1) & 1;
    switch (type) {
      case 0:   /* Hamming */
        hamming(buf, (winLen >> 1), even);
        return OK;
      case 1:   /* Hanning */
        vonhann(buf, (winLen >> 1), even);
        return OK;
      case 2:   /* Kaiser */
        {
          double  beta = 6.8;
          double  x, flen2, besbeta;
          flen2 = 1.0 / ((double)(winLen >> 1) * (double)(winLen >> 1));
          besbeta = 1.0 / besseli(beta);
          n = winLen >> 1;
          x = (even ? 0.5 : 0.05);
          for (i = 0; i < n; i++, x += 1.0)
            buf[i] = (MYFLT)(besseli(beta * sqrt(1.0 - x * x * flen2))
                              * besbeta);
          buf[i] = FL(0.0);
        }
        return OK;
      default:
        if (UNLIKELY(type >= 0))
          return csound->InitError(csound, Str("invalid window type"));
    }
    /* use table created with GEN20 */
    flen = csoundGetTable(csound, &ftable, -(type));
    if (UNLIKELY(flen < 0))
      return csound->InitError(csound, Str("ftable for window not found"));
    inc = (double)flen / (double)(winLen & (~1));
    fpos = ((double)flen + (double)even * inc) * 0.5;
    n = winLen >> 1;
    /* this assumes that for a window with even size, space for an extra */
    /* sample is allocated */
    for (i = 0; i < n; i++) {
      double  frac, tmp;
      int32_t     pos;
      frac = modf(fpos, &tmp);
      pos = (int32_t) tmp;
      buf[i] = ftable[pos] + ((ftable[pos + 1] - ftable[pos]) * (MYFLT) frac);
      fpos += inc;
    }
    buf[n] = (even ? FL(0.0) : ftable[flen]);
    return OK;
}


int32_t pvssanalset(CSOUND *csound, PVSANAL *p)
{
    /* opcode params */
    int32_t N = MYFLT2LRND(*p->winsize);
    int32_t NB;
    int32_t i;
    int32_t wintype = MYFLT2LRND(*p->wintype);

    if (N<=0) return csound->InitError(csound, Str("Invalid window size"));
    /* deal with iinit and iformat later on! */

    N = N + N%2;               /* Make N even */
    NB = N/2+1;                 /* Number of bins */

    /* Need space for NB complex numbers for each of ksmps */
    if (p->fsig->frame.auxp==NULL ||
        CS_KSMPS*(N+2)*sizeof(MYFLT) > (uint32_t)p->fsig->frame.size)
      csound->AuxAlloc(csound, CS_KSMPS*(N+2)*sizeof(MYFLT),&p->fsig->frame);
    else memset(p->fsig->frame.auxp, 0, CS_KSMPS*(N+2)*sizeof(MYFLT));
    /* Space for remembering samples */
    if (p->input.auxp==NULL ||
        N*sizeof(MYFLT) > (uint32_t)p->input.size)
      csound->AuxAlloc(csound, N*sizeof(MYFLT),&p->input);
    else memset(p->input.auxp, 0, N*sizeof(MYFLT));
    csound->AuxAlloc(csound, NB * sizeof(double), &p->oldInPhase);
   if (p->analwinbuf.auxp==NULL ||
        NB*sizeof(CMPLX) > (uint32_t)p->analwinbuf.size)
      csound->AuxAlloc(csound, NB*sizeof(CMPLX),&p->analwinbuf);
    else memset(p->analwinbuf.auxp, 0, NB*sizeof(CMPLX));
    p->inptr = 0;                 /* Pointer in circular buffer */
    p->fsig->NB = p->Ii = NB;
    p->fsig->wintype = wintype;
    p->fsig->format = PVS_AMP_FREQ;      /* only this, for now */
    p->fsig->N = p->nI  = N;
    p->fsig->sliding = 1;
    /* Need space for NB sines, cosines and a scatch phase area */
    if (p->trig.auxp==NULL ||
        (2*NB)*sizeof(double) > (uint32_t)p->trig.size)
      csound->AuxAlloc(csound,(2*NB)*sizeof(double),&p->trig);
    {
      double dc = cos(TWOPI/(double)N);
      double ds = sin(TWOPI/(double)N);
      double *c = (double *)(p->trig.auxp);
      double *s = c+NB;
      p->cosine = c;
      p->sine = s;
      c[0] = 1.0; s[0] = 0.0; // assignment to s unnecessary as auxalloc zeros
        /*
          direct computation of c and s may be better for large n
          c[i] = cos(2*PI*i/n);
          s[i] = sin(2*PI*i/n);
          if (i % 16 == 15) {
          c[i] = cos(2*PI*(i+1)/n);
          s[i] = sin(2*PI*(i+1)/n);
        */
      for (i=1; i<NB; i++) {
          c[i] = dc*c[i-1] - ds*s[i-1];
          s[i] = ds*c[i-1] + dc*s[i-1];
      }
/*       for (i=0; i<NB; i++)  */
/*         printf("c[%d] = %f   \ts[%d] = %f\n", i, c[i], i, s[i]); */
    }
    return OK;
}

int32_t pvsanalset(CSOUND *csound, PVSANAL *p)
{
    MYFLT *analwinhalf,*analwinbase;
    MYFLT sum;
    int32_t halfwinsize,buflen;
    int32_t i,nBins,Mf/*,Lf*/;

    /* opcode params */
    uint32_t N =(int32_t) *(p->fftsize);
    uint32_t overlap = (uint32_t) *(p->overlap);
    uint32_t M = (uint32_t) *(p->winsize);
    int32_t wintype = (int32_t) *p->wintype;
    /* deal with iinit and iformat later on! */

    if (overlap<CS_KSMPS || overlap<=10) /* 10 is a guess.... */
      return pvssanalset(csound, p);
    if (UNLIKELY(N <= 32))
      return csound->InitError(csound,
                               Str("pvsanal: fftsize of 32 is too small!\n"));
    /* check N for powof2? CARL fft routines and FFTW are not limited to that */
    N = N  + N%2;       /* Make N even */
    if (UNLIKELY(M < N)) {
       csound->Warning(csound,
                               Str("pvsanal: window size too small for fftsize"));
       M = N;
    }
    if (UNLIKELY(overlap > N / 2))
      return csound->InitError(csound,
                               Str("pvsanal: overlap too big for fft size\n"));
#ifdef OLPC
    if (UNLIKELY(overlap < CS_KSMPS))
      return csound->InitError(csound,
                               Str("pvsanal: overlap must be >= ksmps\n"));
#endif
    halfwinsize = M/2;
    buflen = M*4;
    p->arate = (float)(csound->esr / (MYFLT) overlap);
    p->fund = (float)(csound->esr / (MYFLT) N);

    nBins = N/2 + 1;
    /* we can exclude/simplify all sorts of stuff in CARL
     * as we will never do time-scaling with this opcode
     */
    /*Lf =*/ Mf = 1 - M%2;

    csound->AuxAlloc(csound, overlap * sizeof(MYFLT), &p->overlapbuf);
    csound->AuxAlloc(csound, (N+2) * sizeof(MYFLT), &p->analbuf);
    csound->AuxAlloc(csound, (M+Mf) * sizeof(MYFLT), &p->analwinbuf);
    csound->AuxAlloc(csound, nBins * sizeof(MYFLT), &p->oldInPhase);
    csound->AuxAlloc(csound, buflen * sizeof(MYFLT), &p->input);
    /* the signal itself */
    csound->AuxAlloc(csound, (N+2) * sizeof(MYFLT), &p->fsig->frame);

    /* make the analysis window*/
    analwinbase = (MYFLT *) (p->analwinbuf.auxp);
    analwinhalf = analwinbase + halfwinsize;

    if (UNLIKELY(PVS_CreateWindow(csound, analwinhalf, wintype, M) != OK))
      return NOTOK;

    for (i = 1; i <= halfwinsize; i++)
      *(analwinhalf - i) = *(analwinhalf + i - Mf);
    if (M > N) {
      double dN = (double)N;
      /*  sinc function */
      if (Mf)
        *analwinhalf *= (MYFLT)(dN * sin(HALFPI/dN) / (HALFPI));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf + i) *= (MYFLT)
          (dN * sin((double)(PI*(i+0.5*Mf)/dN)) / (PI*(i+0.5*Mf)));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf - i) = *(analwinhalf + i - Mf);
    }
    /* get net amp */
    sum = FL(0.0);

    for (i = -halfwinsize; i <= halfwinsize; i++)
      sum += *(analwinhalf + i);
    sum = FL(2.0) / sum;  /* factor of 2 comes in later in trig identity */
    for (i = -halfwinsize; i <= halfwinsize; i++)
      *(analwinhalf + i) *= sum;


  /*    p->invR = (float)(FL(1.0) / csound->esr); */
    p->RoverTwoPi = (float)(p->arate / TWOPI_F);
    p->TwoPioverR = (float)(TWOPI_F / p->arate);
    p->Fexact =  (float)(csound->esr / (MYFLT)N);
    p->nI = -((int64_t)(halfwinsize/overlap))*overlap; /* input time (in samples) */
    /*Dd = halfwinsize + p->nI + 1;                     */
    /* in streaming mode, Dd = ovelap all the time */
    p->Ii = 0;
    p->IOi = 0;
    p->buflen = buflen;
    p->nextIn = (MYFLT *) p->input.auxp;
    p->inptr = 0;
    /* and finally, set up the output signal */
    p->fsig->N =  N;
    p->fsig->overlap = overlap;
    p->fsig->winsize = M;
    p->fsig->wintype = wintype;
    p->fsig->framecount = 1;
    p->fsig->format = PVS_AMP_FREQ;      /* only this, for now */
    p->fsig->sliding = 0;

    if (!(N & (N - 1))) /* if pow of two use this */
     p->setup = csound->RealFFT2Setup(csound,N,FFT_FWD);
    return OK;
}

static void generate_frame(CSOUND *csound, PVSANAL *p)
{
  int32_t got, tocp,i,j,k,ii;
    int32_t N = p->fsig->N;
    int32_t N2 = N/2;
    int32_t buflen = p->buflen;
    int32_t analWinLen = p->fsig->winsize/2;
    int32_t synWinLen = analWinLen;
    float *ofp;                 /* RWD MUST be 32bit */
    MYFLT *fp;
    MYFLT *anal = (MYFLT *) (p->analbuf.auxp);
    MYFLT *input = (MYFLT *) (p->input.auxp);
    MYFLT *analWindow = (MYFLT *) (p->analwinbuf.auxp) + analWinLen;
    MYFLT *oldInPhase = (MYFLT *) (p->oldInPhase.auxp);
    MYFLT angleDif,real,imag,phase;
    double rratio;

    got = p->fsig->overlap;      /*always assume */
    fp = (MYFLT *) (p->overlapbuf.auxp);
    tocp = (got<= input + buflen - p->nextIn ? got : input + buflen - p->nextIn);
    got -= tocp;
    while (tocp-- > 0)
      *(p->nextIn++) = *fp++;

    if (got > 0) {
      p->nextIn -= buflen;
      while (got-- > 0)
        *p->nextIn++ = *fp++;
    }
    if (p->nextIn >= (input + buflen))
      p->nextIn -= buflen;

    /* analysis: The analysis subroutine computes the complex output at
       time n of (N/2 + 1) of the phase vocoder channels.  It operates
       on input samples (n - analWinLen) thru (n + analWinLen) and
       expects to find these in input[(n +- analWinLen) mod ibuflen].
       It expects analWindow to point to the center of a
       symmetric window of length (2 * analWinLen +1).  It is the
       responsibility of the main program to ensure that these values
       are correct!  The results are returned in anal as succesive
       pairs of real and imaginary values for the lowest (N/2 + 1)
       channels.   The subroutines fft and reals together implement
       one efficient FFT call for a real input sequence.  */

    /* for (i = 0; i < N+2; i++)
     *(anal + i) = FL(0.0);  */     /*initialize*/
    memset(anal, 0, sizeof(MYFLT)*(N+2));

    j = (p->nI - analWinLen - 1 + buflen) % buflen;     /*input pntr*/

    k = p->nI - analWinLen - 1;                 /*time shift*/
    while (k < 0)
      k += N;
    k = k % N;
    for (i = -analWinLen; i <= analWinLen; i++) {
      if (UNLIKELY(++j >= buflen))
        j -= buflen;
      if (UNLIKELY(++k >= N))
        k -= N;
      /* *(anal + k) += *(analWindow + i) * *(input + j); */
      anal[k] += analWindow[i] * input[j];
    }
    if (!(N & (N - 1))) {
      /* csound->RealFFT(csound, anal, N);*/
      csound->RealFFT2(csound,p->setup,anal);
      anal[N] = anal[1];
      anal[1] = anal[N + 1] = FL(0.0);
    }
    else
      csound->RealFFTnp2(csound, anal, N);
    /* conversion: The real and imaginary values in anal are converted to
       magnitude and angle-difference-per-second (assuming an
       intermediate sampling rate of rIn) and are returned in
       anal. */
#ifdef NOTDEF
    /* may support this  later on */
    if (format == PVS_AMP_PHASE) {
      /* PVOCEX uses plain (wrapped) phase format, ref Soundhack */
      for (i=0,i0=anal,i1=anal+1,oi=p->oldInPhase;
           i <= N2;
           i++,i0+=2,i1+=2, oi++) {
        real = *i0;
        imag = *i1;
        *i0 = HYPOT(real, imag);
        /* phase unwrapping */
        /*if (*i0 == 0.)*/
        if (UNLIKELY(*i0 < FL(1.0E-10)))
          /* angleDif = 0.0f; */
          phase = FL(0.0);

        else {

          phase = ATAN2(imag,real);

          /*angleDif  = (phase = (float)rratio) - *oi;
           *oi = phase;
           phase = (MYFLT)rratio;
           */
        }

        *i1 = phase;
      }
    }
#endif
    /*if (format==PVS_AMP_FREQ) {*/
    for (i=ii=0    /*,i0=anal,i1=anal+1,oi=oldInPhase*/;
         i <= N2;
         i++,ii+=2 /*i0+=2,i1+=2, oi++*/) {
      real = anal[ii] /* *i0 */;
      imag = anal[ii+1] /* *i1 */;
      /**i0*/ anal[ii] = HYPOT(real, imag);
      /* phase unwrapping */
      /*if (*i0 == 0.)*/
      if (UNLIKELY(/* *i0 */ anal[ii] < FL(1.0E-10)))
        angleDif = FL(0.0);
      else {
        rratio =  atan2((double)imag,(double)real);
        angleDif  = (phase = (MYFLT)rratio) - /**oi*/ oldInPhase[i];
        /* *oi */ oldInPhase[i] = phase;
      }

      if (angleDif > PI_F)
        angleDif = angleDif - TWOPI_F;
      if (angleDif < -PI_F)
        angleDif = angleDif + TWOPI_F;

      /* add in filter center freq.*/
      /* *i1 */ anal[ii+1]  = angleDif * p->RoverTwoPi + ((MYFLT) i * p->Fexact);

    }
    /* } */
    /* else must be PVOC_COMPLEX */
    fp = anal;
    ofp = (float *) (p->fsig->frame.auxp);      /* RWD MUST be 32bit */
    for (i=0;i < N+2;i++)
      /* *ofp++ = (float)(*fp++); */
      ofp[i] = (float) fp[i];

    p->nI += p->fsig->overlap;                          /* increment time */
    if (p->nI > (synWinLen + p->fsig->overlap))
      p->Ii = /*I*/p->fsig->overlap;
    else
      if (p->nI > synWinLen)
        p->Ii = p->nI - synWinLen;
      else {
        p->Ii = 0;

        /*  for (i=p->nO+synWinLen; i<buflen; i++)
            if (i > 0)
            *(output+i) = 0.0f;
            */
      }

    p->IOi = p->Ii;
}

static void anal_tick(CSOUND *csound, PVSANAL *p,MYFLT samp)
{
    MYFLT *inbuf = (MYFLT *) (p->overlapbuf.auxp);

    if (p->inptr== p->fsig->overlap) {
      generate_frame(csound, p);
      p->fsig->framecount++;
      p->inptr = 0;

    }
    //printf("inptr = %d fsig->overlap=%d\n", p->inptr, p->fsig->overlap);
    inbuf[p->inptr++] = samp;

}

static inline double mod2Pi(double x)
{
    x = fmod(x,TWOPI);
    if (x <= -PI) {
        return x + TWOPI;
    }
    else if (x > PI) {
        return x - TWOPI;
    }
    else
      return x;
}

int32_t pvssanal(CSOUND *csound, PVSANAL *p)
{
    MYFLT *ain;
    int32_t NB = p->Ii, loc;
    int32_t N = p->fsig->N;
    MYFLT *data = (MYFLT*)(p->input.auxp);
    CMPLX *fw = (CMPLX*)(p->analwinbuf.auxp);
    double *c = p->cosine;
    double *s = p->sine;
    double *h = (double*)p->oldInPhase.auxp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t wintype = p->fsig->wintype;
    if (UNLIKELY(data==NULL)) {
      return csound->PerfError(csound,&(p->h),
                               Str("pvsanal: Not Initialised.\n"));
    }
    ain = p->ain;               /* The input samples */
    loc = p->inptr;             /* Circular buffer */
    nsmps -= early;
    for (i=offset; i < nsmps; i++) {
      MYFLT re, im, dx;
      CMPLX* ff;
      int32_t j;

/*       printf("%d: in = %f\n", i, *ain); */
      dx = *ain - data[loc];    /* Change in sample */
      data[loc] = *ain++;       /* Remember input sample */
      /* get the frame for this sample */
      ff = (CMPLX*)(p->fsig->frame.auxp) + i*NB;
      /* fw is the current frame at this sample */
      for (j = 0; j < NB; j++) {
        double ci = c[j], si = s[j];
        re = fw[j].re + dx;
        im = fw[j].im;
        fw[j].re = ci*re - si*im;
        fw[j].im = ci*im + si*re;
      }
      loc++; if (UNLIKELY(loc==p->nI)) loc = 0; /* Circular buffer */
      /* apply window and transfer to ff buffer*/
      /* Rectang :Fw_t =     F_t                          */
      /* Hamming :Fw_t = 0.54F_t - 0.23[ F_{t-1}+F_{t+1}] */
      /* Hamming :Fw_t = 0.5 F_t - 0.25[ F_{t-1}+F_{t+1}] */
      /* Blackman:Fw_t = 0.42F_t - 0.25[ F_{t-1}+F_{t+1}]+0.04[F_{t-2}+F_{t+2}] */
      /* Blackman_exact:Fw_t = 0.42659071367153912296F_t
         - 0.24828030954428202923 [F_{t-1}+F_{t+1}]
         + 0.038424333619948409286 [F_{t-2}+F_{t+2}]      */
      /* Nuttall_C3:Fw_t = 0.375  F_t - 0.25[ F_{t-1}+F_{t+1}] +
                                      0.0625 [F_{t-2}+F_{t+2}] */
      /* BHarris_3:Fw_t = 0.44959 F_t - 0.24682[ F_{t-1}+F_{t+1}] +
                                      0.02838 [F_{t-2}+F_{t+2}] */
      /* BHarris_min:Fw_t = 0.42323 F_t - 0.2486703 [ F_{t-1}+F_{t+1}] +
                                      0.0391396 [F_{t-2}+F_{t+2}] */
      switch (wintype) {
      case PVS_WIN_HAMMING:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.54)*fw[j].re;
          ff[j].im = FL(0.54)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.23)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.23)*(fw[j+1].im + fw[j-1].im);
        }
        ff[0].re -= FL(0.46)*fw[1].re;
        ff[NB-1].re -= FL(0.46)*fw[NB-2].re;
        break;
      case PVS_WIN_HANN:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.5)*fw[j].re;
          ff[j].im = FL(0.5)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.25)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.25)*(fw[j+1].im + fw[j-1].im);
        }
        ff[0].re -= FL(0.5)*fw[1].re;
        ff[NB-1].re -= FL(0.5)*fw[NB-2].re;
        break;
      default:
        csound->Warning(csound,
                        Str("Unknown window type; replaced by rectangular\n"));
        /* FALLTHRU */
      case PVS_WIN_RECT:
        memcpy(ff, fw, NB*sizeof(CMPLX));
        /* for (j=0; j<NB; j++) { */
        /*   ff[j].re = fw[j].re; */
        /*   ff[j].im = fw[j].im; */
        /* } */
        break;
      case PVS_WIN_BLACKMAN:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.42)*fw[j].re;
          ff[j].im = FL(0.42)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.25)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.25)*(fw[j+1].im + fw[j-1].im);
        }
        for (j=2; j<NB-2; j++) {
          ff[j].re += FL(0.04)*(fw[j+2].re + fw[j-2].re);
          ff[j].im += FL(0.04)*(fw[j+2].im + fw[j-2].im);
        }
        ff[0].re    += -FL(0.5)*fw[1].re + FL(0.08)*fw[2].re;
        ff[NB-1].re += -FL(0.5)*fw[NB-2].re + FL(0.08)*fw[NB-3].re;
        ff[1].re    += -FL(0.5)*fw[2].re + FL(0.08)*fw[3].re;
        ff[NB-2].re += -FL(0.5)*fw[NB-3].re + FL(0.08)*fw[NB-4].re;
        break;
    case PVS_WIN_BLACKMAN_EXACT:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.42659071367153912296)*fw[j].re;
          ff[j].im = FL(0.42659071367153912296)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.49656061908856405847)*FL(0.5)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.49656061908856405847)*FL(0.5)*(fw[j+1].im + fw[j-1].im);
        }
        for (j=2; j<NB-2; j++) {
          ff[j].re += FL(0.076848667239896818573)*FL(0.5)*(fw[j+2].re + fw[j-2].re);
          ff[j].im += FL(0.076848667239896818573)*FL(0.5)*(fw[j+2].im + fw[j-2].im);
        }
        ff[0].re    += -FL(0.49656061908856405847) * fw[1].re
                      + FL(0.076848667239896818573) * fw[2].re;
        ff[NB-1].re += -FL(0.49656061908856405847) * fw[NB-2].re
                      + FL(0.076848667239896818573) * fw[NB-3].re;
        ff[1].re    += -FL(0.49656061908856405847) * fw[2].re
                      + FL(0.076848667239896818573) * fw[3].re;
        ff[NB-2].re += -FL(0.49656061908856405847) * fw[NB-3].re
                      + FL(0.076848667239896818573) * fw[NB-4].re;
        break;
      case PVS_WIN_NUTTALLC3:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.375)*fw[j].re;
          ff[j].im = FL(0.375)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.5)*FL(0.5)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.5)*FL(0.5)*(fw[j+1].im + fw[j-1].im);
        }
        for (j=2; j<NB-2; j++) {
          ff[j].re += FL(0.125)*FL(0.5)*(fw[j+2].re + fw[j-2].re);
          ff[j].im += FL(0.125)*FL(0.5)*(fw[j+2].im + fw[j-2].im);
        }
        ff[0].re    += -FL(0.5) * fw[1].re    + FL(0.125) * fw[2].re;
        ff[NB-1].re += -FL(0.5) * fw[NB-2].re + FL(0.125) * fw[NB-3].re;
        ff[1].re    += -FL(0.5) * fw[2].re    + FL(0.125) * fw[3].re;
        ff[NB-2].re += -FL(0.5) * fw[NB-3].re + FL(0.125) * fw[NB-4].re;
        ff[1].re = 0.5 * (fw[2].re + fw[0].re); /* HACK???? */
        ff[1].im = 0.5 * (fw[2].im + fw[0].im);
        break;
      case PVS_WIN_BHARRIS_3:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.44959)*fw[j].re;
          ff[j].im = FL(0.44959)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.49364)*FL(0.5)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.49364)*FL(0.5)*(fw[j+1].im + fw[j-1].im);
        }
        for (j=2; j<NB-2; j++) {
          ff[j].re += FL(0.05677)*FL(0.5)*(fw[j+2].re + fw[j-2].re);
          ff[j].im += FL(0.05677)*FL(0.5)*(fw[j+2].im + fw[j-2].im);
        }
        ff[0].re    += -FL(0.49364) * fw[1].re    + FL(0.05677) * fw[2].re;
        ff[NB-1].re += -FL(0.49364) * fw[NB-2].re + FL(0.05677) * fw[NB-3].re;
        ff[1].re    += -FL(0.49364) * fw[2].re    + FL(0.05677) * fw[3].re;
        ff[NB-2].re += -FL(0.49364) * fw[NB-3].re + FL(0.05677) * fw[NB-4].re;
        ff[1].re = 0.5 * (fw[2].re + fw[0].re); /* HACK???? */
        ff[1].im = 0.5 * (fw[2].im + fw[0].im);
        break;
      case PVS_WIN_BHARRIS_MIN:
        for (j=0; j<NB; j++) {
          ff[j].re = FL(0.42323)*fw[j].re;
          ff[j].im = FL(0.42323)*fw[j].im;
        }
        for (j=1; j<NB-1; j++) {
          ff[j].re -= FL(0.4973406)*FL(0.5)*(fw[j+1].re + fw[j-1].re);
          ff[j].im -= FL(0.4973406)*FL(0.5)*(fw[j+1].im + fw[j-1].im);
        }
        for (j=2; j<NB-2; j++) {
          ff[j].re += FL(0.0782793)*FL(0.5)*(fw[j+2].re + fw[j-2].re);
          ff[j].im += FL(0.0782793)*FL(0.5)*(fw[j+2].im + fw[j-2].im);
        }
        ff[0].re    += -FL(0.4973406) * fw[1].re    + FL(0.0782793) * fw[2].re;
        ff[NB-1].re += -FL(0.4973406) * fw[NB-2].re + FL(0.0782793) * fw[NB-3].re;
        ff[1].re    += -FL(0.4973406) * fw[2].re    + FL(0.0782793) * fw[3].re;
        ff[NB-2].re += -FL(0.4973406) * fw[NB-3].re + FL(0.0782793) * fw[NB-4].re;
        ff[1].re = 0.5 * (fw[2].re + fw[0].re); /* HACK???? */
        ff[1].im = 0.5 * (fw[2].im + fw[0].im);
        break;
      }
/*       if (i==9) { */
/*         printf("Frame as Amp/Freq %d\n", i); */
/*         for (j = 0; j < NB; j++) */
/*           printf("%d: %f\t%f\n", j, ff[j].re, ff[j].im); */
/*       } */
      for (j = 0; j < NB; j++) { /* Convert to AMP_FREQ */
        double thismag = hypot(ff[j].re, ff[j].im);
        double phase = atan2(ff[j].im, ff[j].re);
        double angleDif  = phase -  h[j];
        h[j] = phase;
            /*subtract expected phase difference */
        angleDif -= (double)j * TWOPI/N;
        angleDif =  mod2Pi(angleDif);
        angleDif =  angleDif * N /TWOPI;
        ff[j].re = thismag;
        ff[j].im = csound->esr * (j + angleDif)/N;
      }
/*       if (i==9) { */
/*         printf("Frame as Amp/Freq %d\n", i); */
/*         for (j = 0; j < NB; j++) */
/*           printf("%d: %f\t%f\n", j, ff[j].re, ff[j].im); */
/*       } */
    }

    p->inptr = loc;
    return OK;
}

int32_t pvsanal(CSOUND *csound, PVSANAL *p)
{
    MYFLT *ain;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;

    ain = p->ain;

    if (UNLIKELY(p->input.auxp==NULL)) {
      return csound->PerfError(csound,&(p->h),
                               Str("pvsanal: Not Initialised.\n"));
    }
    {
      int32_t overlap = (int32_t)*p->overlap;
      if (overlap<(int32_t)nsmps || overlap<10) /* 10 is a guess.... */
        return pvssanal(csound, p);
    }
    nsmps -= early;
    for (i=offset; i < nsmps; i++)
      anal_tick(csound,p,ain[i]);
    return OK;
}

int32_t pvsynthset(CSOUND *csound, PVSYNTH *p)
{
    MYFLT *analwinhalf;
    MYFLT *synwinhalf;
    MYFLT sum;
    int32_t halfwinsize,buflen;
    int32_t i,nBins,Mf,Lf;
    double IO;

    /* get params from input fsig */
    /* we TRUST they are legal */
    int32_t N = p->fsig->N;
    int32_t overlap = p->fsig->overlap;
    int32_t M = p->fsig->winsize;
    int32_t wintype = p->fsig->wintype;

    p->fftsize = N;
    p->winsize = M;
    p->overlap = overlap;
    p->wintype = wintype;
    p->format = p->fsig->format;
    if (p->fsig->sliding) {
      /* get params from input fsig */
      /* we TRUST they are legal */
      int32_t wintype = p->fsig->wintype;
      /* and put into locals */
      p->wintype = wintype;
      p->format = p->fsig->format;
      csound->AuxAlloc(csound, p->fsig->NB * sizeof(double), &p->oldOutPhase);
      csound->AuxAlloc(csound, p->fsig->NB * sizeof(double), &p->output);
      return OK;
    }
    /* and put into locals */
    halfwinsize = M/2;
    buflen = M*4;
    IO = (double)overlap;         /* always, no time-scaling possible */

    p->arate = csound->esr / (MYFLT) overlap;
    p->fund = csound->esr / (MYFLT) N;
    nBins = N/2 + 1;
    Lf = Mf = 1 - M%2;
    /* deal with iinit later on! */
    csound->AuxAlloc(csound, overlap * sizeof(MYFLT), &p->overlapbuf);
    csound->AuxAlloc(csound, (N+2) * sizeof(MYFLT), &p->synbuf);
    csound->AuxAlloc(csound, (M+Mf) * sizeof(MYFLT), &p->analwinbuf);
    csound->AuxAlloc(csound, (M+Mf) * sizeof(MYFLT), &p->synwinbuf);
    csound->AuxAlloc(csound, nBins * sizeof(MYFLT), &p->oldOutPhase);
    csound->AuxAlloc(csound, buflen * sizeof(MYFLT), &p->output);




    synwinhalf = (MYFLT *) (p->synwinbuf.auxp) + halfwinsize;



    /* synthesis windows */
    if (M <= N) {
      if (UNLIKELY(PVS_CreateWindow(csound, synwinhalf, wintype, M) != OK))
        return NOTOK;

      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);


       sum = FL(0.0);
        for (i = -halfwinsize; i <= halfwinsize; i++)
           sum += *(synwinhalf + i);
         sum = FL(2.0) / sum;

       for (i = -halfwinsize; i <= halfwinsize; i++)
        *(synwinhalf + i) *= sum;

         sum = FL(0.0);
   /* no timescaling, so I(nterpolation) will always = D(ecimation) = overlap */
        for (i = -halfwinsize; i <= halfwinsize; i+=overlap)
          sum += *(synwinhalf + i) * *(synwinhalf + i);
    }
    else {
     /* have to make analysis window to get amp scaling */
    /* so this ~could~ be a local alloc and free...*/
      double dN = (double)N;
      analwinhalf = (MYFLT *) (p->analwinbuf.auxp) + halfwinsize;
    if (UNLIKELY(PVS_CreateWindow(csound, analwinhalf, wintype, M) != OK))
      return NOTOK;

    for (i = 1; i <= halfwinsize; i++){
      analwinhalf[-i] = analwinhalf[i - Mf];
    }

      // sinc function
    if (Mf) {
        *analwinhalf *= (MYFLT)(dN * sin(HALFPI/dN) / ( HALFPI));
    }
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf + i) *= (MYFLT)
          (dN * sin((double)(PI*(i+0.5*Mf)/dN)) / (PI*(i+0.5*Mf)));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf - i) = *(analwinhalf + i - Mf);

     /* get net amp */
    sum = FL(0.0);
    for (i = -halfwinsize; i <= halfwinsize; i++)
    sum += *(analwinhalf + i);
    sum = FL(2.0) / sum;  /* factor of 2 comes in later in trig identity */

      if (UNLIKELY(PVS_CreateWindow(csound, synwinhalf, wintype, M) != OK))
        return NOTOK;

      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);

      if (Lf)
        *synwinhalf *= (MYFLT)(IO * sin((double)(HALFPI/IO)) / (HALFPI));
      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf + i) *= (MYFLT)
          ((double)IO * sin((double)(PI*(i+0.5*Lf)/IO)) /
           (PI*(i+0.5*(double)Lf)));
      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);
    }


   if (!(N & (N - 1L)))
     sum = csound->GetInverseRealFFTScale(csound, (int32_t) N)/ sum;
    else
      sum = FL(1.0) / sum;

  for (i = -halfwinsize; i <= halfwinsize; i++)
      *(synwinhalf + i) *= sum;

/*  p->invR = FL(1.0) / csound->esr; */
    p->RoverTwoPi = p->arate / TWOPI_F;
    p->TwoPioverR = TWOPI_F / p->arate;
    p->Fexact =  csound->esr / (MYFLT)N;
    p->nO = -(halfwinsize / overlap) * overlap; /* input time (in samples) */
    p->Ii = 0;                          /* number of new outputs to write */
    p->IOi = 0;
    p->outptr = 0;
    p->nextOut = (MYFLT *) (p->output.auxp);
    p->buflen = buflen;

    if (!(N & (N - 1))) /* if pow of two use this */
      p->setup = csound->RealFFT2Setup(csound,N,FFT_INV);
    return OK;
}

static MYFLT synth_tick(CSOUND *csound, PVSYNTH *p)
{
    MYFLT *outbuf = (MYFLT *) (p->overlapbuf.auxp);

    if (p->outptr== p->fsig->overlap) {
      process_frame(csound, p);
      p->outptr = 0;
    }
    return outbuf[p->outptr++];
}

static void process_frame(CSOUND *csound, PVSYNTH *p)
{
    int32_t i,j,k,ii,NO,NO2;
    float *anal;                                        /* RWD MUST be 32bit */
    MYFLT *syn, *output;
    MYFLT *oldOutPhase = (MYFLT *) (p->oldOutPhase.auxp);
    int32_t N = p->fsig->N;
    MYFLT *obufptr,*outbuf,*synWindow;
    MYFLT mag,phase,angledif, the_phase;
    int32_t synWinLen = p->fsig->winsize / 2;
    int32_t overlap = p->fsig->overlap;
    /*int32 format = p->fsig->format; */

    /* fsigs MUST be corect format, as we offer no mechanism for
       assignment to a different one*/

    NO = N;        /* always the same */
    NO2 = NO/2;
    syn = (MYFLT *) (p->synbuf.auxp);
    anal = (float *) (p->fsig->frame.auxp);             /* RWD MUST be 32bit */
    output = (MYFLT *) (p->output.auxp);
    outbuf = (MYFLT *) (p->overlapbuf.auxp);
    synWindow = (MYFLT *) (p->synwinbuf.auxp) + synWinLen;

    /* reconversion: The magnitude and angle-difference-per-second in syn
       (assuming an intermediate sampling rate of rOut) are
       converted to real and imaginary values and are returned in syn.
       This automatically incorporates the proper phase scaling for
       time modifications. */

    if (LIKELY(NO <= N)) {
      for (i = 0; i < NO+2; i++)
        syn[i] = (MYFLT) anal[i];
    }
    else {
      for (i = 0; i <= N+1; i++)
        syn[i] = (MYFLT) anal[i];
      for (i = N+2; i < NO+2; i++)
        syn[i] = FL(0.0);
    }
#ifdef NOTDEF
    if (format==PVS_AMP_PHASE) {
      for (ii=0 /*, i0=syn, i1=syn+1*/; ii<= NO2; ii+=2 /* i++, i0+=2,  i1+=2*/) {
        mag = syn[ii];    /* *i0; */
        phase = syn[ii+1]; /* *i1; */
        /* *i0 */ syn[ii] = (MYFLT)((double)mag * cos((double)phase));
        /* *i1 */ syn[ii+1] = (MYFLT)((double)mag * sin((double)phase));
      }
    }
    else if (format == PVS_AMP_FREQ) {
#endif
      for (i=ii=0 /*, i0=syn, i1=syn+1*/; i<= NO2; i++, ii+=2 /*i0+=2,  i1+=2*/) {
        mag = syn[ii]; /* *i0; */
        /* RWD variation to keep phase wrapped within +- TWOPI */
        /* this is spread across several frame cycles, as the problem does not
           develop for a while */

        angledif = p->TwoPioverR * ( /* *i1 */ syn[ii+1] - ((MYFLT)i * p->Fexact));
        the_phase = /* *(oldOutPhase + i) */ oldOutPhase[i] + angledif;
        if (i== p->bin_index)
          the_phase = (MYFLT) fmod(the_phase,TWOPI);
        /* *(oldOutPhase + i) = the_phase; */
        oldOutPhase[i] = the_phase;
        phase = the_phase;
        /* *i0 */ syn[ii]  = (MYFLT)((double)mag * cos((double)phase));
        /* *i1 */ syn[ii+1] = (MYFLT)((double)mag * sin((double)phase));
      }
#ifdef NOTDEF
    }
#endif

    /* for phase normalization */
    if (++(p->bin_index) == NO2+1)
      p->bin_index = 0;

    /* else it must be PVOC_COMPLEX */

    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
       technique to reconstruct the time-domain signal.  The (N/2 + 1)
       phase vocoder channel outputs at time n are inverse Fourier
       transformed, windowed, and added into the output array.  The
       subroutine thinks of output as a shift register in which
       locations are referenced modulo obuflen.  Therefore, the main
       program must take care to zero each location which it "shifts"
       out (to standard output). The subroutines reals and fft
       together perform an efficient inverse FFT.  */
    if (!(NO & (NO - 1))) {
      /*printf("N %d %d \n", NO, NO & (NO-1));*/
      syn[1] = syn[NO];
      /* csound->InverseRealFFT(csound, syn, NO);*/
      csound->RealFFT2(csound,p->setup,syn);
      syn[NO] = syn[NO + 1] = FL(0.0);
    }
    else
      csound->InverseRealFFTnp2(csound, syn, NO);
    j = p->nO - synWinLen - 1;
    while (j < 0)
      j += p->buflen;
    j = j % p->buflen;

    k = p->nO - synWinLen - 1;
    while (k < 0)
      k += NO;
    k = k % NO;

    for (i = -synWinLen; i <= synWinLen; i++) { /*overlap-add*/
      if (++j >= p->buflen)
        j -= p->buflen;
      if (++k >= NO)
        k -= NO;
      /* *(output + j) += *(syn + k) * *(synWindow + i); */
      output[j] += syn[k] * synWindow[i];
    }

    obufptr = outbuf;

    for (i = 0; i < p->IOi;) {  /* shift out next IOi values */
      int32_t todo = (p->IOi-i <= output+p->buflen - p->nextOut ?
                  p->IOi-i : output+p->buflen - p->nextOut);
      /*outfloats(nextOut, todo, ofd);*/
      /*copy data to external buffer */
      /*for (n=0;n < todo;n++)
       *obufptr++ = p->nextOut[n]; */
      memcpy(obufptr, p->nextOut, sizeof(MYFLT)*todo);
      obufptr += todo;

      i += todo;

      /* for (j = 0; j < todo; j++)
       *p->nextOut++ = FL(0.0); */
      memset(p->nextOut, 0, sizeof(MYFLT)*todo);
      p->nextOut += todo;

      if (p->nextOut >= (output + p->buflen))
        p->nextOut -= p->buflen;
    }

    /* increment time */
    p->nO += overlap;

    if (p->nO > (synWinLen + /*I*/overlap))
      p->Ii = overlap;
    else
      if (p->nO > synWinLen)
        p->Ii = p->nO - synWinLen;
      else {
        p->Ii = 0;
        for (i=p->nO+synWinLen; i<p->buflen; i++)
          if (i > 0)
            output[i] = FL(0.0);
      }
    p->IOi =  p->Ii;
}

int32_t pvssynth(CSOUND *csound, PVSYNTH *p)
{
    int32_t i, k;
    int32_t ksmps = CS_KSMPS;
    int32_t N = p->fsig->N;
    int32_t NB = p->fsig->NB;
    MYFLT *aout = p->aout;
    CMPLX *ff;
    double *h = (double*)p->oldOutPhase.auxp;
    double *output = (double*)p->output.auxp;

    /* Get real part from AMP/FREQ */
    for (i=0; i<ksmps; i++) {
      MYFLT a;
      ff = (CMPLX*)(p->fsig->frame.auxp) + i*NB;
      for (k=0; k<NB; k++) {
        double tmp, phase;

        tmp = ff[k].im; /* Actually frequency */
        /* subtract bin mid frequency */
        tmp -= (double)k * csound->esr/N;
        /* get bin deviation from freq deviation */
        tmp *= TWOPI /csound->esr;
        /* add the overlap phase advance back in */
        tmp += (double)k*TWOPI/N;
        h[k] = phase = mod2Pi(h[k] + tmp);
        output[k] = ff[k].re*cos(phase);
      }
      a = FL(0.0);
      for (k=1; k<NB-1; k++) {
        a -= output[k];
        if (k+1<NB-1) a+=output[++k];
      }
      aout[i] = (a+a+output[0]-output[NB-1])/N;
    }
    return OK;
}

int32_t pvsynth(CSOUND *csound, PVSYNTH *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT *aout = p->aout;

    if (UNLIKELY(p->output.auxp==NULL)) {
      return csound->PerfError(csound,&(p->h),
                               Str("pvsynth: Not Initialised.\n"));
    }
    if (p->fsig->sliding) return pvssynth(csound, p);
    if (UNLIKELY(offset)) memset(aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i<nsmps; i++)
      aout[i] = synth_tick(csound, p);
    return OK;
}

static void hamming(MYFLT *win, int32_t winLen, int32_t even)
{
    double ftmp;
    int32_t i;

    ftmp = PI/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        win[i] = (MYFLT)(0.54 + 0.46*cos(ftmp*((double)i+0.5)));
      win[winLen] = FL(0.0);
    }
    else {
      win[0] = FL(1.0);
      for (i=1; i<=winLen; i++)
        win[i] = (MYFLT)(0.54 + 0.46*cos(ftmp*(double)i));
    }

}

double besseli(double x)
{
    double ax, ans;
    double y;

    if (( ax = fabs( x)) < 3.75)     {
      y = x / 3.75;
      y *= y;
      ans = (1.0 + y * ( 3.5156229 +
                         y * ( 3.0899424 +
                               y * ( 1.2067492 +
                                     y * ( 0.2659732 +
                                           y * ( 0.360768e-1 +
                                                 y * 0.45813e-2))))));
    }
    else {
      y = 3.75 / ax;
      ans = ((exp ( ax) / sqrt(ax))
             * (0.39894228 +
                y * (0.1328592e-1 +
                     y * (0.225319e-2 +
                          y * (-0.157565e-2 +
                               y * (0.916281e-2 +
                                    y * (-0.2057706e-1 +
                                         y * (0.2635537e-1 +
                                              y * (-0.1647633e-1 +
                                                   y * 0.392377e-2)))))))));
    }
    return ans;
}

static void vonhann(MYFLT *win, int32_t winLen, int32_t even)
{
    MYFLT ftmp;
    int32_t i;

    ftmp = PI_F/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        win[i] = (MYFLT)(0.5 + 0.5 * cos(ftmp*((double)i+0.5)));
      win[winLen] = FL(0.0);
    }
    else {
      win[0] = FL(1.0);
      for (i=1; i<=winLen; i++)
        win[i] = (MYFLT)(0.5 + 0.5 * cos(ftmp*(double)i));
    }
}
