/*
    pvsanal.c:

    Copyright (C) 2002 Richard Dobson

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

/* pvsanal.c */
/* functions based on CARL pvoc.c (c) Mark Dolson.
   The CARL software distribution is due to be released under the GNU LPGL.
*/

#include <math.h>
#include "cs.h"
#include "pstream.h"
#include "oload.h"



/* CARL/CDP fft routines  */
extern void fft_(MYFLT *, MYFLT *,int,int,int,int);
extern void fftmx(MYFLT *,MYFLT *,int,int,int,int,int,int *,
                  MYFLT *,MYFLT *,MYFLT *,MYFLT *,int *,int[]);
extern void reals_(MYFLT *,MYFLT *,int,int);


static void generate_frame(PVSANAL *p);
static void process_frame(PVSYNTH *p);
void    hamming(MYFLT *win,int winLen,int even);
void    vonhann(MYFLT *win,int winLen,int even);


int pvsanalset(ENVIRON *csound, PVSANAL *p)
{
    MYFLT *analwinhalf,*analwinbase;
    MYFLT sum;
    long halfwinsize,buflen;
    int i,nBins,Mf,Lf;

    /* opcode params */
    long N =(long) *(p->fftsize);
    long overlap = (long) *(p->overlap);
    long M = (long) *(p->winsize);
    int wintype = (int) *p->wintype;
    /* deal with iinit and iformat later on! */

    if (N <= 32)
      die(Str("pvsanal: fftsize of 32 is too small!\n"));
    /* check N for powof2? CARL fft routines and FFTW are not limited to that. */
    N = N  + N%2;       /* Make N even */
    if (M < N)
      die(Str("pvsanal: window size too small for fftsize\n"));
    if (overlap > N / 2)
      die(Str("pvsanal: overlap too big for fft size\n"));
    if (overlap < ksmps)
      die(Str("pvsanal: overlap must be >= ksmps\n"));

    halfwinsize = M/2;
    buflen = M*4;
    p->arate = (float)(esr / (MYFLT) overlap);
    p->fund = (float)(esr / (MYFLT) N);

    nBins = N/2 + 1;
    /* we can exclude/simplify all sorts of stuff in CARL
     * as we will never do time-scaling with this opcode
     */
    Lf = Mf = 1 - M%2;

    if (p->overlapbuf.auxp==NULL ||
        overlap * sizeof(MYFLT) > (unsigned int)p->overlapbuf.size)
      auxalloc(overlap * sizeof(MYFLT),&p->overlapbuf);
    if (p->analbuf.auxp==NULL ||
        (N+2) * sizeof(MYFLT) > (unsigned int)p->analbuf.size)
      auxalloc((N+2) * sizeof(MYFLT),&p->analbuf);
    if (p->analwinbuf.auxp==NULL ||
        (M+Mf) * sizeof(MYFLT) > (unsigned int)p->analwinbuf.size)
      auxalloc((M+Mf) * sizeof(MYFLT),&p->analwinbuf);
    if (p->oldInPhase.auxp==NULL ||
        nBins * sizeof(MYFLT) > (unsigned int)p->oldInPhase.size)
      auxalloc(nBins * sizeof(MYFLT),&p->oldInPhase);
    if (p->input.auxp==NULL ||
        buflen*sizeof(MYFLT) > (unsigned int)p->input.size)
      auxalloc(buflen*sizeof(MYFLT),&p->input);
    /* the signal itself */
    if (p->fsig->frame.auxp==NULL ||
        (N+2)*sizeof(MYFLT) > (unsigned int)p->fsig->frame.size)
      auxalloc((N+2)*sizeof(MYFLT),&p->fsig->frame);

    /* make the analysis window*/
    analwinbase = (MYFLT *) (p->analwinbuf.auxp);
    analwinhalf = analwinbase + halfwinsize;

    switch (wintype) {
    case PVS_WIN_HAMMING:
      hamming(analwinhalf,halfwinsize,Mf);
      break;
    case PVS_WIN_HANN:
      vonhann(analwinhalf,halfwinsize,Mf);
      break;
      /* KAISER ... etc.... ? */
    default:
      die(Str("pvsanal: unsupported value for iwintype\n"));
      break;
    }

    for (i = 1; i <= halfwinsize; i++)
      *(analwinhalf - i) = *(analwinhalf + i - Mf);
    if (M > N) {
      /*  sinc function */
      if (Mf)
        *analwinhalf *= (MYFLT)((double)N * sin(PI*0.5/(double)N) /
                                (PI*0.5));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf + i) *= (MYFLT)
          ((double)N * sin((double) (PI*(i+0.5*Mf)/N)) / (PI*(i+0.5*Mf)));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf - i) = *(analwinhalf + i - Mf);
    }
    /* get net amp */
    sum = FL(0.0);
    for (i = -halfwinsize; i <= halfwinsize; i++)
      sum += *(analwinhalf + i);
    sum = FL(2.0) / sum;  /*factor of 2 comes in later in trig identity*/
    for (i = -halfwinsize; i <= halfwinsize; i++)
      *(analwinhalf + i) *= sum;


  /*    p->invR = (float)(FL(1.0) / esr); */
    p->RoverTwoPi = (float)(p->arate / TWOPI_F);
    p->TwoPioverR = (float)(TWOPI_F / p->arate);
    p->Fexact =  (float)(esr / (MYFLT)N);
    p->nI = -(halfwinsize / overlap) * overlap; /* input time (in samples) */
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
    return OK;
}

static void generate_frame(PVSANAL *p)
{
    int got, tocp,i,j,k;
    int N = p->fsig->N;
    int N2 = N/2;
    long buflen = p->buflen;
    long analWinLen = p->fsig->winsize/2;
    long synWinLen = analWinLen;
    float *ofp;                 /* RWD MUST be 32bit */
    MYFLT *fp,*oi,*i0,*i1;
    MYFLT *anal = (MYFLT *) (p->analbuf.auxp);
    MYFLT *banal = anal + 1;
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


    for (i = 0; i < N+2; i++)
      *(anal + i) = FL(0.0);       /*initialize*/

    j = (p->nI - analWinLen - 1 + buflen) % buflen;     /*input pntr*/

    k = p->nI - analWinLen - 1;                 /*time shift*/
    while (k < 0)
      k += N;
    k = k % N;
    for (i = -analWinLen; i <= analWinLen; i++) {
      if (++j >= buflen)
        j -= buflen;
      if (++k >= N)
        k -= N;
      *(anal + k) += *(analWindow + i) * *(input + j);
    }
#ifdef USE_FFTW
    rfftwnd_one_real_to_complex(forward_plan,anal,NULL);

#else
    fft_(anal,banal,1,N2,1,-2);
    reals_(anal,banal,N2,-2);
#endif
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
        *i0 =(MYFLT) sqrt((double)(real * real + imag * imag));
        /* phase unwrapping */
        /*if (*i0 == 0.)*/
        if (*i0 < FL(1.0E-10))
          /* angleDif = 0.0f; */
          phase = FL(0.0);

        else {

          rratio = atan2((double)imag,(double)real);

          /*angleDif  = (phase = (float)rratio) - *oi;
           *oi = phase;
           */
          phase = (MYFLT)rratio;
        }

        *i1 = phase;
      }
    }
#endif
    /*if (format==PVS_AMP_FREQ) {*/
    for (i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2, oi++) {
      real = *i0;
      imag = *i1;
      *i0 =(MYFLT) sqrt((double)(real * real + imag * imag));
      /* phase unwrapping */
      /*if (*i0 == 0.)*/
      if (*i0 < FL(1.0E-10))
        angleDif = FL(0.0);
      else {
        rratio =  atan2((double)imag,(double)real);
        angleDif  = (phase = (MYFLT)rratio) - *oi;
        *oi = phase;
      }

      if (angleDif > PI_F)
        angleDif = angleDif - TWOPI_F;
      if (angleDif < -PI_F)
        angleDif = angleDif + TWOPI_F;

      /* add in filter center freq.*/
      *i1 = angleDif * p->RoverTwoPi + ((MYFLT) i * p->Fexact);
    }
    /* } */
    /* else must be PVOC_COMPLEX */
    fp = anal;
    ofp = (float *) (p->fsig->frame.auxp);      /* RWD MUST be 32bit */
    for (i=0;i < N+2;i++)
      *ofp++ = (float)(*fp++);

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


static void anal_tick(PVSANAL *p,MYFLT samp)
{
    MYFLT *inbuf = (MYFLT *) (p->overlapbuf.auxp);

    if (p->inptr== p->fsig->overlap) {
      generate_frame(p);
      p->fsig->framecount++;
      p->inptr = 0;

    }
    inbuf[p->inptr++] = samp;

}


int pvsanal(ENVIRON *csound, PVSANAL *p)
{
    MYFLT *ain;
    int i;

    ain = p->ain;

    if (p->input.auxp==NULL) {
      die(Str("pvsanal: Not Initialised.\n"));
    }

    for (i=0; i < ksmps; i++)
      anal_tick(p,ain[i]);
    return OK;
}


int pvsynthset(ENVIRON *csound, PVSYNTH *p)
{
    MYFLT *analwinhalf;
    MYFLT *synwinhalf;
    MYFLT sum;
    long halfwinsize,buflen;
    int i,nBins,Mf,Lf;
    double IO;

    /* get params from input fsig */
    /* we TRUST they are legal */
    long N = p->fsig->N;
    long overlap = p->fsig->overlap;
    long M = p->fsig->winsize;
    int wintype = p->fsig->wintype;
    /* and put into locals */
    p->fftsize = N;
    p->winsize = M;
    p->overlap = overlap;
    p->wintype = wintype;
    p->format = p->fsig->format;
    halfwinsize = M/2;
    buflen = M*4;
    IO = (double) overlap;         /* always, no time-scaling possible */


    p->arate = esr / (MYFLT) overlap;
    p->fund = esr / (MYFLT) N;
    nBins = N/2 + 1;
    Lf = Mf = 1 - M%2;
    /* deal with iinit later on! */
    if (p->overlapbuf.auxp==NULL ||
        overlap * sizeof(MYFLT) > (unsigned int)p->overlapbuf.size)
      auxalloc(overlap * sizeof(MYFLT),&p->overlapbuf);
    if (p->synbuf.auxp==NULL ||
        (N+2) * sizeof(MYFLT) > (unsigned int)p->synbuf.size)
      auxalloc((N+2) * sizeof(MYFLT),&p->synbuf);
    if (p->analwinbuf.auxp==NULL ||
        (M+Mf) * sizeof(MYFLT) > (unsigned int)p->analwinbuf.size)
      auxalloc((M+Mf) * sizeof(MYFLT),&p->analwinbuf);
    if (p->synwinbuf.auxp==NULL ||
        (M+Mf) * sizeof(MYFLT) > (unsigned int)p->synwinbuf.size)
      auxalloc((M+Mf) * sizeof(MYFLT),&p->synwinbuf);
    if (p->oldOutPhase.auxp==NULL ||
        nBins * sizeof(MYFLT) > (unsigned int)p->oldOutPhase.size)
      auxalloc(nBins * sizeof(MYFLT),&p->oldOutPhase);
    if (p->output.auxp==NULL ||
        buflen*sizeof(MYFLT) > (unsigned int)p->output.size)
      auxalloc(buflen*sizeof(MYFLT),&p->output);

    /* have to make analysis window to get amp scaling */
    /* so this ~could~ be a local alloc and free...*/

    analwinhalf = (MYFLT *) (p->analwinbuf.auxp) + halfwinsize;
    synwinhalf = (MYFLT *) (p->synwinbuf.auxp) + halfwinsize;

    switch (wintype) {
    case PVS_WIN_HAMMING:
      hamming(analwinhalf,halfwinsize,Mf);
      break;
    case PVS_WIN_HANN:
      vonhann(analwinhalf,halfwinsize,Mf);
      break;
    default:
      die(Str("pvsanal: unsupported value for iwintype\n"));
      break;
    }

    for (i = 1; i <= halfwinsize; i++)
      *(analwinhalf - i) = *(analwinhalf + i - Mf);
    if (M > N) {
      /*  sinc function */
      if (Mf)
        *analwinhalf *= (MYFLT)((double)N * sin(PI*0.5/(double)N) /
                                ( PI*0.5));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf + i) *= (MYFLT)
          ((double)N * sin((double) (PI*(i+0.5*Mf)/N)) / (PI*(i+0.5*Mf)));
      for (i = 1; i <= halfwinsize; i++)
        *(analwinhalf - i) = *(analwinhalf + i - Mf);
    }
    /* get net amp */
    sum = FL(0.0);
    for (i = -halfwinsize; i <= halfwinsize; i++)
      sum += *(analwinhalf + i);
    sum = FL(2.0) / sum;  /*factor of 2 comes in later in trig identity*/
    for (i = -halfwinsize; i <= halfwinsize; i++)
      *(analwinhalf + i) *= sum;

    /* synthesis windows*/
    if (M <= N) {
      switch (wintype) {
      case PVS_WIN_HAMMING:
        hamming(synwinhalf,halfwinsize,Lf);
        break;
      case PVS_WIN_HANN:
        vonhann(synwinhalf,halfwinsize,Lf);
        break;
      default:
        die(Str("pvsynth: internal error: "
                "fsig has unrecognised value for iwintype\n"));
        break;
      }

      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);

      for (i = -halfwinsize; i <= halfwinsize; i++)
        *(synwinhalf + i) *= sum;

      sum = FL(0.0);
   /* no timescaling, so I(nterpolation) will always = D(ecimation) = overlap*/
      for (i = -halfwinsize; i <= halfwinsize; i+=overlap)
        sum += *(synwinhalf + i) * *(synwinhalf + i);

      sum = FL(1.0)/ sum;
#ifdef USE_FFTW
      sum *= Ninv;
#endif
      for (i = -halfwinsize; i <= halfwinsize; i++)
        *(synwinhalf + i) *= sum;
    }
    else {
      switch (wintype) {
      case PVS_WIN_HAMMING:
        hamming(synwinhalf,halfwinsize,Lf);
        break;
      case PVS_WIN_HANN:
        vonhann(synwinhalf,halfwinsize,Lf);
        break;
      default:
        die(Str("pvsynth: internal error: "
                "fsig has unrecognised value for iwintype\n"));
        break;
      }

      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);

      if (Lf)
        *synwinhalf *= (MYFLT)(IO * sin((double) (PI*0.5/IO)) / (PI*0.5));
      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf + i) *= (MYFLT)
          ((double)IO * sin((double) (PI*(i+0.5*Lf)/IO)) /
           (PI*(i+0.5*(double)Lf)));
      for (i = 1; i <= halfwinsize; i++)
        *(synwinhalf - i) = *(synwinhalf + i - Lf);

      sum = FL(1.0)/sum;
#ifdef USE_FFTW
      sum *= Ninv;
#endif
      for (i = -halfwinsize; i <= halfwinsize; i++)
        *(synwinhalf + i) *= sum;
    }
/*     p->invR = FL(1.0) / esr; */
    p->RoverTwoPi = p->arate / TWOPI_F;
    p->TwoPioverR = TWOPI_F / p->arate;
    p->Fexact =  esr / (MYFLT)N;
    p->nO = -(halfwinsize / overlap) * overlap; /* input time (in samples) */
    p->Ii = 0;                          /* number of new outputs to write */
    p->IOi = 0;
    p->outptr = 0;
    p->nextOut = (MYFLT *) (p->output.auxp);
    p->buflen = buflen;
    return OK;
}

static MYFLT synth_tick(PVSYNTH *p)
{
    MYFLT *outbuf = (MYFLT *) (p->overlapbuf.auxp);

    if (p->outptr== p->fsig->overlap) {
      process_frame(p);
      p->outptr = 0;
    }
    return outbuf[p->outptr++];
}

static void process_frame(PVSYNTH *p)
{
    int n,i,j,k,NO,NO2;
    float *anal;                                        /* RWD MUST be 32bit */
    MYFLT *syn,*bsyn,*i0,*i1,*output;
    MYFLT *oldOutPhase = (MYFLT *) (p->oldOutPhase.auxp);
    long N = p->fsig->N;
    MYFLT *obufptr,*outbuf,*synWindow;
    MYFLT mag,phase,angledif, the_phase;
    long synWinLen = p->fsig->winsize / 2;
    long overlap = p->fsig->overlap;
    /*long format = p->fsig->format; */

    /* fsigs MUST be corect format, as we offer no mechanism for
       assignment to a different one*/

    NO = N;        /* always the same */
    NO2 = NO/2;
    syn = (MYFLT *) (p->synbuf.auxp);
    anal = (float *) (p->fsig->frame.auxp);             /* RWD MUST be 32bit */
    output = (MYFLT *) (p->output.auxp);
    outbuf = (MYFLT *) (p->overlapbuf.auxp);
    synWindow = (MYFLT *) (p->synwinbuf.auxp) + synWinLen;

    bsyn = syn+1;
    /* reconversion: The magnitude and angle-difference-per-second in syn
       (assuming an intermediate sampling rate of rOut) are
       converted to real and imaginary values and are returned in syn.
       This automatically incorporates the proper phase scaling for
       time modifications. */

    if (NO <= N) {
      for (i = 0; i < NO+2; i++)
        *(syn+i) = (MYFLT) *(anal+i);
    }
    else {
      for (i = 0; i <= N+1; i++)
        *(syn+i) = (MYFLT) *(anal+i);
      for (i = N+2; i < NO+2; i++)
        *(syn+i) = FL(0.0);
    }
#ifdef NOTDEF
    if (format==PVS_AMP_PHASE) {
      for (i=0, i0=syn, i1=syn+1; i<= NO2; i++, i0+=2,  i1+=2) {
        mag = *i0;
        phase = *i1;
        *i0 = (MYFLT)((double)mag * cos((double)phase));
        *i1 = (MYFLT)((double)mag * sin((double)phase));
      }
    }
    else if (format == PVS_AMP_FREQ) {
#endif
      for (i=0, i0=syn, i1=syn+1; i<= NO2; i++, i0+=2,  i1+=2) {
        mag = *i0;
        /* RWD variation to keep phase wrapped within +- TWOPI */
        /* this is spread across several frame cycles, as the problem does not
           develop for a while */

        angledif = p->TwoPioverR * (*i1  - ((MYFLT) i * p->Fexact));
        the_phase = *(oldOutPhase + i) +angledif;
        if (i== p->bin_index)
          the_phase = (MYFLT) fmod(the_phase,TWOPI);
        *(oldOutPhase + i) = the_phase;
        phase = the_phase;
        *i0 = (MYFLT)((double)mag * cos((double)phase));
        *i1 = (MYFLT)((double)mag * sin((double)phase));
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
#ifdef USE_FFTW
    rfftwnd_one_complex_to_real(inverse_plan,(fftw_complex * )syn,NULL);
#else
    reals_(syn,bsyn,NO2,2);
    fft_(syn,bsyn,1,NO2,1,2);
#endif
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
      *(output + j) += *(syn + k) * *(synWindow + i);
    }

    obufptr = outbuf;

    for (i = 0; i < p->IOi;) {  /* shift out next IOi values */
      int j;
      int todo = (p->IOi-i <= output+p->buflen - p->nextOut ?
                  p->IOi-i : output+p->buflen - p->nextOut);
      /*outfloats(nextOut, todo, ofd);*/
      /*copy data to external buffer */
      for (n=0;n < todo;n++)
        *obufptr++ = p->nextOut[n];

      i += todo;

      for (j = 0; j < todo; j++)
        *p->nextOut++ = FL(0.0);
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
            *(output+i) = FL(0.0);
      }
    p->IOi =  p->Ii;
}



int pvsynth(ENVIRON *csound, PVSYNTH *p)
{
    int i;
    MYFLT *aout = p->aout;

    if (p->output.auxp==NULL) {
      die(Str("pvsynth: Not Initialised.\n"));
    }
    for (i=0;i < ksmps;i++)
      aout[i] = synth_tick(p);
    return OK;
}
