/*
    dsputil.c:

    Copyright (C) 1991 Dan Ellis, Barry Vercoe, John ffitch,
    Copyright (C) 1988 Codemist Ltd

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

/******************************************************************/
/*  dsputil.c                                                     */
/* DSP utility functions for Csound - dispfft, pvoc, and convolve */
/* 20apr90 dpwe                                                   */
/******************************************************************/

#include "sysdep.h"
#include "dpwelib.h"
#include "fft.h"
#include "dsputil.h"
#include <math.h>
#ifdef MSVC                   /* Thanks to Richard Dobson */
# define hypot _hypot
#endif
#if defined(__WATCOMC__) || defined(__ncc)
extern double hypot(double,double);
#endif

/* Do we do the whole buffer, or just indep vals? */
#define someof(s)       (s)
    /* (1L+s/2L) is the indep vals in an FFT frame of length (s).. well */
    /* (change to just (s) to work on whole buffer) */
#define actual(s)       ((s-1L)*2L)
    /* if callers are passing whole frame size, make this (s) */
    /* where callers pass s/2+1, this recreates the parent fft frame size */

void CopySamps(MYFLT *sce, MYFLT *dst, long size) /* just move samples */
{
    ++size;
    while (--size)
      *dst++ = *sce++;
}

/* Allocate a cleared buffer */
MYFLT *MakeBuf(long size)
{
    MYFLT *res,*p;
    long  i;

    res = (MYFLT *) malloc((long)size * sizeof(MYFLT));
    p = res;
    for (i=0; i<size; ++i)
      *p++ = FL(0.0);
    return(res);
}

/* Write window coefs into buffer, don't malloc */
void FillHalfWin(MYFLT *wBuf, long size, MYFLT max, int hannq)
    /* 1 => hanning window else hamming */
{
    MYFLT       a,b;
    long        i;

    if (hannq) {
      a = FL(0.50);
      b = FL(0.50);
    }
    else {
      a = FL(0.54);
      b = FL(0.46);
    }

    if (wBuf!= NULL) {        /* NB: size/2 + 1 long - just indep terms */
      size /= 2;              /* to fix scaling */
      for (i=0; i<=size;++i)
        wBuf[i] = max * (a-b*(MYFLT)cos(PI*(MYFLT)i/(MYFLT)size ) );
    }
    return;
}

/* Make 1st half of hamming window for those as desire */
MYFLT *MakeHalfWin(long size, MYFLT max, int hannq)
                /* Effective window size (altho only s/2+1 alloc'd */
                /* 1 => hanning window else hamming */
{
    MYFLT       *wBuf;

    wBuf = (MYFLT *) malloc((long)(size/2+1) * sizeof(MYFLT));
                                /* NB: size/2 + 1 long - just indep terms */
    FillHalfWin(wBuf,size,max,hannq);
    return(wBuf);
}

void UnpackReals(MYFLT *dst, long size) /* expand out size to re,0,re,0 etc */
{
    MYFLT       *d2;

    d2 = dst + 2*size - 1;      /* start at ends .. */
    dst += size - 1;
    ++size;
    while (--size) {            /* .. & copy backwards */
      *d2-- = FL(0.0);
      *d2-- = *dst--;
    }
}

void PackReals(MYFLT *buffer, long size) /* pack re,im,re,im into re,re */
{
    MYFLT       *b2 = buffer;

    ++size;
    while (--size) {
      *b2++ = *buffer++;
      ++buffer;
    }
}

/* Convert Real & Imaginary spectra into Amplitude & Phase */
void Rect2Polar(MYFLT *buffer, long size)
{
    long        i;
    MYFLT       *real,*imag;
    double      re,im;
    MYFLT       mag;

    real = buffer;      imag = buffer+1;
    for (i = 0; i< someof(size); ++i) {
      re = (double)real[2L*i];
      im = (double)imag[2L*i];
      real[2L*i] = mag = (MYFLT)hypot(re,im);
      if (mag == FL(0.0))
        imag[2L*i] = FL(0.0);
      else
        imag[2L*i] = (MYFLT)atan2(im,re);
    }
}


void Polar2Rect(MYFLT *buffer, long size) /* reverse above */
{
    long    i;
    MYFLT   *magn,*phas;
    MYFLT   mag;
    double  pha;

    magn = buffer;  phas = buffer+1;
    for (i = 0; i< someof(size); ++i) {
      mag =         magn[2L*i];
      pha = (double)phas[2L*i];
      magn[2L*i] = mag*(MYFLT)cos(pha);
      phas[2L*i] = mag*(MYFLT)sin(pha);
    }
}

void Lin2DB(MYFLT *buffer, long size)   /* packed buffer i.e. reals, not complex */
{
    while (size--) {
      *buffer = /* FL(20.0)*log10 */ FL(8.68589)*(MYFLT)log(*buffer);
      buffer++;
    }
}

void DB2Lin(MYFLT *buffer, long size)   /* packed buffer i.e. reals, not complex */
{
    while (size--) {
      *buffer = (MYFLT)exp( /* 1/20.0*log10 */ 0.1151292*(*buffer) );
      buffer++;
    }
}

MYFLT maskPhs(MYFLT phs)        /* do it inline instead! */
{
    while (phs > PI_F) {

    }
    while (phs < -PI_F) {
      phs += FL(2.0)*PI_F;
    }
    return(phs);
}

#define MmaskPhs(phs)   /* macro version of phase masking */ \
    while (phs > PI_F)    \
        phs -= FL(2.0)*PI_F; \
    while (phs < -PI_F)   \
        phs += FL(2.0)*PI_F; \

#define MMmaskPhs(p,q,s) /* p is pha, q is as int, s is 1/PI */ \
    q = (int)(s*p);     \
    p -= PI_F*(MYFLT)((int)((q+((q>=0)?(q&1):-(q&1) )))); \


void UnwrapPhase(MYFLT *buf, long size, MYFLT *oldPh)
{
    long    i;
    MYFLT   *pha;
    MYFLT   p, oneOnPi;
    int     z;

    pha = buf + 1;
    oneOnPi = FL(1.0)/PI_F;
    for (i=0; i<someof(size); ++i) {
      p = pha[2L*i];
      p -= oldPh[i];              /* find change since last frame */
      MMmaskPhs(p,z,oneOnPi);
      /* MmaskPhs(p); */
      oldPh[i] = pha[2L*i];       /* hold actual phase for next diffce */
      pha[2L*i] = p;              /* .. but write back phase change */
    }
}

void RewrapPhase(MYFLT *buf, long size, MYFLT *oldPh)
{
    long    i;
    MYFLT   *pha;
    MYFLT   p,oneOnPi;
    int     z;

    /* Phase angle was properly scaled when it came out of frqspace */
    /* .. so just add it */
    pha = buf + 1;
    oneOnPi = FL(1.0)/PI_F;
    for (i=0; i<someof(size); ++i) {
      p = (pha[2L*i]+oldPh[i]);
      MMmaskPhs(p,z,oneOnPi);
      /* MmaskPhs(p);     */
      oldPh[i] = pha[2L*i] = p;
    }
}

/* Convert a batch of phase differences to actual target freqs */
void PhaseToFrq(MYFLT *buf, long size, MYFLT incr, MYFLT sampRate)
{
    long    i;
    MYFLT   *pha,p,q,oneOnPi;
    int     z;
    MYFLT   srOn2pi, binMidFrq, frqPerBin;
    MYFLT   expectedDphas,eDphIncr;

    pha = buf + 1;
    srOn2pi = sampRate/(FL(2.0)*PI_F*incr);
    frqPerBin = sampRate/((MYFLT)actual(size));
    binMidFrq = FL(0.0);
    /* Of course, you get some phase shift with spot-on frq coz time shift */
    expectedDphas = FL(0.0);
    eDphIncr = FL(2.0)*PI_F*incr/((MYFLT)actual(size));
    oneOnPi = FL(1.0)/PI_F;
    for (i=0; i<someof(size); ++i) {
      q = p = pha[2L*i]-expectedDphas;
      MMmaskPhs(p,z,oneOnPi);
/*      MmaskPhs(q);    */
      pha[2L*i] = p;
      pha[2L*i] *= srOn2pi;
      pha[2L*i] += binMidFrq;

      expectedDphas += eDphIncr;
      expectedDphas -= TWOPI_F*(MYFLT)((int)(expectedDphas*oneOnPi));
      binMidFrq += frqPerBin;
    }
    /* Doesn't deal with 'phases' of DC & fs/2 any different */
}

/* Undo a pile of frequencies back into phase differences */
void FrqToPhase(MYFLT *buf, long size, MYFLT incr, MYFLT sampRate, MYFLT fixUp)
    /* the fixup phase shift ... ? */
{
    MYFLT   *pha;
    MYFLT   twoPiOnSr, binMidFrq, frqPerBin;
    MYFLT   expectedDphas,eDphIncr;
    MYFLT   p;
    long    i;
    int     j;
    MYFLT   oneOnPi;

    oneOnPi = FL(1.0)/PI_F;
    pha = buf + 1;
    twoPiOnSr = FL(2.0)*PI_F*((MYFLT)incr)/sampRate;
    frqPerBin = sampRate/((MYFLT)actual(size));
    binMidFrq = FL(0.0);
    /* Of course, you get some phase shift with spot-on frq coz time shift */
    expectedDphas = FL(0.0);
    eDphIncr = FL(2.0)*PI_F*((incr)/((MYFLT)actual(size)) + fixUp);
    for (i=0; i<someof(size); ++i) {
      p = pha[2L*i];
      p -= binMidFrq;
      p *= twoPiOnSr;
      p += expectedDphas;
      MMmaskPhs(p,j,oneOnPi);
      /* MmaskPhs(p);     */
      pha[2L*i] = p;
      expectedDphas += eDphIncr;
      expectedDphas -= TWOPI_F*(MYFLT)((int)(expectedDphas*oneOnPi));
      binMidFrq += frqPerBin;
    }
    /* Does not deal with 'phases' of DC & fs/2 any different */
}

/* Unpack stored mag/pha data into buffer */
void FetchIn(
    MYFLT   *inp,       /* pointer to input data */
    MYFLT   *buf,       /* where to put our nice mag/pha pairs */
    long    fsize,      /* frame size we're working with */
    MYFLT   pos)        /* fractional frame we want */
{
    long    j;
    MYFLT   *frm0,*frm1;
    long    base;
    MYFLT   frac;

    /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
    base = (long)pos;               /* index of basis frame of interpolation */
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frm0 = inp + ((long)fsize+2L)*base;
    frm1 = frm0 + ((long)fsize+2L);         /* addresses of both frames */
    if (frac != FL(0.0)) {         /* must have 2 cases to avoid poss seg vlns */
                                   /* and failed computes, else may interp   */
                                   /* bd valid data */
      for (j=0; j<(fsize/2L + 1L); ++j) {  /* mag/pha for just over 1/2 */
                                           /* Interpolate both mag and phase */
        buf[2L*j   ] = frm0[2L*j   ] + frac*(frm1[2L*j   ]-frm0[2L*j   ]);
        buf[2L*j+1L] = frm0[2L*j+1L] + frac*(frm1[2L*j+1L]-frm0[2L*j+1L]);
      }
    }
    else {       /* frac is 0.0 i.e. just copy the source frame */
      for (j=0; j<(fsize/2L + 1L); ++j) { /* no need to interpolate */
        buf[2L*j  ] = frm0[2L*j];
        buf[2L*j+1] = frm0[2L*j+1L];
      }
    }
}

/* Fill out the dependent 2nd half of iFFT data; scale down & opt conjgt */
void FillFFTnConj(
    MYFLT   *buf,
    long     size,              /* full length of FFT ie 2^n */
    MYFLT   scale,              /* can apply a scale factor.. */
    int     conj)               /* flag to conjugate at same time */
{
    MYFLT   miscale;            /* scaling for poss. conj part */
    MYFLT   *mag,*pha;
    long    j;
    long    hasiz = 1L + size/2L; /* the indep values */

    if (scale == FL(0.0))
      scale = FL(1.0);
    if (conj)
      miscale = -scale;
    else
      miscale = scale;
    mag = buf;      pha = buf+1;
    for (j=0; j<hasiz; ++j) {       /* i.e. mag/pha for just over 1/2 */
      mag[2L*j] *= scale;
      pha[2L*j] *= miscale;
    }
    for (j=hasiz; j<size; ++j) {        /* .. the rest is mirrored .. */
      mag[2L*j] = mag[2L*(size-j)];     /* For the symmetry extension, */
      pha[2L*j] = -pha[2L*(size-j)];    /*  conjugate of 1st 1/2 rgdls */
    }
}

void ApplyHalfWin(MYFLT *buf, MYFLT *win, long len)
    /* Window only store 1st half, is symmetric */
{
    long j;
    long    lenOn2 = (len/2L);

    for (j = lenOn2 + 1; j--; )
      *buf++ *= *win++;
    for (j = len - lenOn2 - 1, win--; j--; )
      *buf++ *= *--win;
}

/* Overlap (some of) new data window with stored previous data
   in circular buffer */
void addToCircBuf(
    MYFLT   *sce, MYFLT *dst, /* linear source and circular destination */
    long    dstStart,         /* Current starting point index in circular dst */
    long    numToDo,          /* how many points to add ( <= circBufSize ) */
    long    circBufSize)      /* Size of circ buf i.e. dst[0..circBufSize-1] */
{
    long    i;
    long    breakPoint;     /* how many points to add before having to wrap */

    breakPoint = circBufSize-dstStart;/* i.e. if we start at (dIndx = lim -2) */
    if (numToDo > breakPoint) {   /*   we will do 2 in 1st loop, rest in 2nd. */
      for (i=0; i<breakPoint; ++i)
        dst[dstStart+i] += sce[i];
      dstStart -= circBufSize;
      for (i=breakPoint; i<numToDo; ++i)
        dst[dstStart+i] += sce[i];
    }
    else                                /* All fits without wraparound */
      for (i=0; i<numToDo; ++i)
        dst[dstStart+i] += sce[i];
    return;
}

/* Write from a circular buffer into a linear output buffer without
   clearing data
   UPDATES SOURCE & DESTINATION POINTERS TO REFLECT NEW POSITIONS */
void writeFromCircBuf(
    MYFLT   **sce,
    MYFLT   **dst,              /* Circular source and linear destination */
    MYFLT   *sceStart,
    MYFLT   *sceEnd,            /* Address of start & end of source buffer */
    long    numToDo)            /* How many points to write (<= circBufSize) */
{
    MYFLT   *srcindex = *sce;
    MYFLT   *dstindex = *dst;
    long    breakPoint;     /* how many points to add before having to wrap */

    breakPoint = sceEnd - srcindex + 1;
    if (numToDo >= breakPoint) { /*  we will do 2 in 1st loop, rest in 2nd. */
      numToDo -= breakPoint;
      for (; breakPoint > 0; --breakPoint) {
/*                printf("Input audio l1 = %f\n",*srcindex);  */
        *dstindex++ = *srcindex++;
      }
      srcindex = sceStart;
    }
    for (; numToDo > 0; --numToDo) {
/*        printf("Input audio l2 = %f\n",*srcindex);  */
      *dstindex++ = *srcindex++;
    }
    *sce = srcindex;
    *dst = dstindex;
    return;
}

/* Write from a circular buffer into a linear output buffer CLEARING DATA */
void writeClrFromCircBuf(
    MYFLT   *sce, MYFLT *dst, /* Circular source and linear destination */
    long    sceStart,         /* Current starting point index in circular sce */
    long    numToDo,          /* How many points to write ( <= circBufSize ) */
    long    circBufSize)      /* Size of circ buf i.e. sce[0..circBufSize-1] */
{
    long    i;
    long    breakPoint;     /* how many points to add before having to wrap */

    breakPoint = circBufSize-sceStart; /* i.e. if we start at (Indx = lim -2) */
    if (numToDo > breakPoint) { /*  we will do 2 in 1st loop, rest in 2nd. */
      for (i=0; i<breakPoint; ++i) {
        dst[i] = sce[sceStart+i];
        sce[sceStart+i] = FL(0.0);
      }
      sceStart -= circBufSize;
      for (i=breakPoint; i<numToDo; ++i) {
        dst[i] = sce[sceStart+i];
        sce[sceStart+i] = FL(0.0);
      }
    }
    else {                             /* All fits without wraparound */
      for (i=0; i<numToDo; ++i) {
        dst[i] = sce[sceStart+i];
        sce[sceStart+i] = FL(0.0);
      }
    }
    return;
}

/* Add source array to dest array, results in dest */
void FixAndAdd(MYFLT *samplSce, short *shortDest, long size)
{
    long i;
    for (i = 0; i < size; i++)
      shortDest[i] += (short)samplSce[i];
}

/* Rules to convert between samples and frames, given frSiz & frIncr */
long NumFrames(long dataSmps, long frSiz, long frInc)
{
    return( 1L + (dataSmps - (long)frSiz)/(long)frInc );
}

long NumSampls(long frames, long frSiz, long frIncr)
{
    return(((long)frSiz)+((long)frIncr)*(frames-1L));
}

/********************************************************************/
/*  udsample.c      -       from dsampip.c                          */
/*  Performs sample rate conversion by interpolated FIR LPF approx  */
/*  VAX, CSOUND VERSION                                             */
/*  1700 07feb90 taken from rational-only version                   */
/*  1620 06dec89 started dpwe                                       */
/********************************************************************/

/* (ugens7.h) #define   SPDS (8) */    /* How many sinc lobes to go out */
/* Static function prototypes - moved to top of file */
static MYFLT *sncTab = NULL;    /* point to our sin(x)/x lookup table */

void UDSample(
    MYFLT   *inSnd,
    MYFLT   stindex,
    MYFLT   *outSnd,
    long     inLen,
    long     outLen,
    MYFLT   fex)
/*  Perform the sample rate conversion:
    inSnd   is the existing sample to be converted
    outSnd  is a pointer to the (pre-allocated) new soundspace
    inLen   is the number of points in the input sequence
    outLen  is the number of output sample points.  e.g inLen/fex
    fex is the factor by which frequencies are increased
    1/fex = lex, the factor by which the output will be longer
    i.e. if the input sample is at 12kHz and we want to produce an
    8kHz sample, we will want it to be 8/12ths as many samples, so
    lex will be 0.75
 */
{
    int     in2out;
    long    i,j,x;
    MYFLT   a;
    MYFLT   phasePerInStep, fracInStep;
    MYFLT   realInStep, stepInStep;
    long    nrstInStep;
    MYFLT   posPhase, negPhase;
    MYFLT   lex = FL(1.0)/fex;
    int     nrst;
    MYFLT   frac;

    phasePerInStep = ((lex>1)? FL(1.0) : lex)* (MYFLT)SPTS;
    /* If we are upsampling, LPF is at input frq => sinc pd matches */
    /*  downsamp => lpf at output rate; input steps at some fraction */
    in2out = (int)( ((MYFLT)SPDS) * ( (fex<1)? 1.0 : fex ) );
    /* number of input points contributing to each op: depends on LPF */
    realInStep = stindex;
    stepInStep = fex;
    for (i = 0; i<outLen; ++i) {      /* output sample loop */
                                      /* i = lex*nrstIp, so .. */
      nrstInStep = (long)realInStep;  /* imm. prec actual sample */
      fracInStep = realInStep-(MYFLT)nrstInStep;  /* Fractional part */
      negPhase = phasePerInStep * fracInStep;
      posPhase = -negPhase;
      /* cum. sinc arguments for +ve & -ve going spans into input */
      nrst = (int)negPhase;       frac = negPhase - (MYFLT)nrst;
      a = (sncTab[nrst]+frac*(sncTab[nrst+1]-sncTab[nrst]))*
        (MYFLT)inSnd[nrstInStep];
      for (j=1L; j<in2out; ++j) { /* inner FIR convolution loop */
        posPhase += phasePerInStep;
        negPhase += phasePerInStep;
        if ( (x = nrstInStep-j)>=0L )
          nrst = (int)negPhase;   frac = negPhase - (MYFLT)nrst;
        a += (sncTab[nrst]+frac*(sncTab[nrst+1]-sncTab[nrst]))
          * (MYFLT)inSnd[x];
        if ( (x = nrstInStep+j)<inLen )
          nrst = (int)posPhase;   frac = posPhase - (MYFLT)nrst;
        a += (sncTab[nrst]+frac*(sncTab[nrst+1]-sncTab[nrst]))
          * (MYFLT)inSnd[x];
      }
      outSnd[i] = (float)a;
      realInStep += stepInStep;
    }
}

void FloatAndCopy(short *sce, MYFLT *dst, long size)
{
    while (size--)
      *dst++ = (MYFLT)*sce++;
}

/* Copy converted frame to the output data */
void    WriteOut(MYFLT *sce, MYFLT **pdst, long fsize)
                /* the frame size - but we may not copy them all! */
{
    int     j;

    for (j=0; j<(2L*(fsize/2L + 1L)); ++j ) /* i.e. mg/ph for just over 1/2 */
      *(*pdst)++ = sce[j];                /* pointer updated for next time */
}

/*--------------------------------------------------------------------*/
/*---------------------------- sinc module ---------------------------*/
/*--------------------------------------------------------------------*/

/* (ugens7.h) #define SPTS (16) */ /* How many points in each lobe */

void MakeSinc(void)             /* initialise our static sinc table */
{
    int     i;
    int     stLen = SPDS*SPTS;  /* sinc table is SPDS/2 periods of sinc */
    MYFLT   theta   = FL(0.0);     /* theta (sinc arg) reaches pi in SPTS */
    MYFLT   dtheta  = (MYFLT)(SBW*PI)/(MYFLT)SPTS;/* SBW lowers cutoff to redcali */
    MYFLT   phi     = FL(0.0);     /* phi (hamm arg) reaches pi at max ext */
    MYFLT   dphi    = PI_F/(MYFLT)(SPDS*SPTS);


    if (sncTab == NULL)
        sncTab = (MYFLT *)malloc((long)(stLen+1) * sizeof(MYFLT));
    /* (stLen+1 to include final zero; better for interpolation etc) */
/*    printf("Make sinc : pts = %d, table = %lx \n",stLen,sncTab);   */
    sncTab[0] =  FL(1.0);
    for (i=1; i<=stLen; ++i) { /* build table of sin x / x */
      theta += dtheta;
      phi   += dphi;
      sncTab[i] = (MYFLT)sin(theta)/theta * (FL(0.54) + FL(0.46)*(MYFLT)cos(phi));
      /* hamming window on top of sinc */
    }
}

#ifdef never
void DestroySinc(void)  /* relase the lookup table */
{
    free(sncTab);
}
#endif

MYFLT SincIntp(MYFLT index)
/* Calculate the sinc of the 'index' value by interpolating the table */
/* <index> is scaled s.t. 1.0 is first zero crossing */
/* ! No checks ! */
{
    int     nrst;
    MYFLT   frac,scaledUp;

    scaledUp = index * SPTS;
    nrst = (int)scaledUp;
    frac = scaledUp - (MYFLT)nrst;
    return(sncTab[nrst] + frac*(sncTab[nrst+1]-sncTab[nrst]) );
}

/****************************************/
/** prewarp.c module                    */
/****************************************/

/* spectral envelope detection: this is a very crude peak picking algorithm
        which is used to detect and pre-warp the spectral envelope so that
        pitch transposition can be performed without altering timbre.
        The basic idea is to disallow large negative slopes between
        successive values of magnitude vs. frequency. */

static  MYFLT   *env = (MYFLT *)NULL;   /* Scratch buffer to hold 'envelope' */

void PreWarpSpec(
    MYFLT   *spec,      /* spectrum as magnitude,phase */
    long     size,      /* full frame size, tho' we only use n/2+1 */
    MYFLT warpFactor) /* How much pitches are being multd by */
{
    MYFLT   eps,slope;
    MYFLT   mag, lastmag, nextmag, pkOld;
    long     pkcnt, i, j;

    if (env==(MYFLT *)NULL)
        env = (MYFLT *)malloc((long)size * sizeof(MYFLT));
    /*!! Better hope <size> in first call is as big as it gets !! */
    eps = -FL(64.0) / size;              /* for spectral envelope estimation */
    lastmag = *spec;
    mag = spec[2*1];
    pkOld = lastmag;
    *env = pkOld;
    pkcnt = 1;

    for (i = 1; i < someof(size); i++) {  /* step thru spectrum */
      if (i < someof(size)-1)
        nextmag = spec[2*(i+1)];
      else nextmag = FL(0.0);

      if (pkOld != FL(0.0))
        slope = ((MYFLT) (mag - pkOld)/(pkOld * pkcnt));
      else
        slope = -FL(10.0);

      /* look for peaks */
      if ((mag>=lastmag)&&(mag>nextmag)&&(slope>eps)) {
        env[i] = mag;
        pkcnt--;
        for (j = 1; j <= pkcnt; j++) {
          env[i - pkcnt + j - 1] = pkOld * (FL(1.0) + slope * j);
        }
        pkOld = mag;
        pkcnt = 1;
      }
      else
        pkcnt++;                    /* not a peak */

      lastmag = mag;
      mag = nextmag;
    }

    if (pkcnt > 1) {                /* get final peak */
      mag = spec[2*(size/2)];
      slope = ((MYFLT) (mag - pkOld) / pkcnt);
      env[size/2] = mag;
      pkcnt--;
      for (j = 1; j <= pkcnt; j++) {
        env[size/2 - pkcnt + j - 1] = pkOld + slope * j;
      }
    }

    for (i = 0; i < someof(size); i++) {  /* warp spectral env.*/
      j = (long)((MYFLT) i * warpFactor);
      mag = spec[2*i];
      if ((j < someof(size)) && (env[i] != FL(0.0)))
        spec[2*i] *= env[j]/env[i];
      else
        spec[2*i] = FL(0.0);
/*      printf("I<%d>J<%d>S<%.0f>E<%.0f>F<%.0f>T<%0.f>",
        i, j, mag, env[i], env[j], spec[2*i]);  */
    }
}


