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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

/******************************************************************/
/*  dsputil.c                                                     */
/* DSP utility functions for Csound - dispfft, pvoc, and convolve */
/* 20apr90 dpwe                                                   */
/******************************************************************/

#include "pvoc.h"
#include <math.h>

/* Do we do the whole buffer, or just indep vals? */
#define someof(s)       (s)
    /* (1L+s/2L) is the indep vals in an FFT frame of length (s).. well */
    /* (change to just (s) to work on whole buffer) */
#define actual(s)       ((s-1L)*2L)
    /* if callers are passing whole frame size, make this (s) */
    /* where callers pass s/2+1, this recreates the parent fft frame size */

/* assumes that FFTsize is an integer multiple of 4 */
void Polar2Real_PVOC(CSOUND *csound, MYFLT *buf, CSOUND_FFT_SETUP * setup)
{
    MYFLT re, im;
    int32_t   i;
    int32_t FFTsize = setup->N;

    for (i = 0; i < FFTsize; i += 4) {
      re = buf[i] * COS(buf[i + 1]);
      im = buf[i] * SIN(buf[i + 1]);
      buf[i    ] = re;
      buf[i + 1] = im;
      /* Offset the phase to align centres of stretched windows, not starts */
      re = -(buf[i + 2] * COS(buf[i + 3]));
      im = -(buf[i + 2] * SIN(buf[i + 3]));
      buf[i + 2] = re;
      buf[i + 3] = im;
    }
    /* kill spurious imag at dc & fs/2 */
    buf[1] = buf[i]; buf[i] = buf[i + 1] = FL(0.0);
    /* calculate inverse FFT */
    csound->RealFFT(csound, setup, buf);
}

#define MMmaskPhs(p,q,s) /* p is pha, q is as int32_t, s is 1/PI */ \
    q = (int32_t)(s*p);     \
    p -= PI_F*(MYFLT)((int32_t)((q+((q>=0)?(q&1):-(q&1) )))); \

void RewrapPhase(MYFLT *buf, int32 size, MYFLT *oldPh)
{
    int32    i;
    MYFLT   *pha;
    MYFLT   p,oneOnPi;
    int32_t     z;

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

/* Undo a pile of frequencies back into phase differences */
void FrqToPhase(MYFLT *buf, int32 size, MYFLT incr, MYFLT sampRate, MYFLT fixUp)
    /* the fixup phase shift ... ? */
{
    MYFLT   *pha;
    MYFLT   twoPiOnSr, binMidFrq, frqPerBin;
    MYFLT   expectedDphas,eDphIncr;
    MYFLT   p;
    int32    i;
    int32_t     j;
    MYFLT   oneOnPi;

    oneOnPi = FL(1.0)/PI_F;
    pha = buf + 1;
    twoPiOnSr = FL(2.0)*PI_F*((MYFLT)incr)/sampRate;
    frqPerBin = sampRate/((MYFLT)actual(size));
    binMidFrq = FL(0.0);
    /* Of course, you get some phase shift with spot-on frq coz time shift */
    expectedDphas = FL(0.0);
    eDphIncr = TWOPI_F*((incr)/((MYFLT)actual(size)) + fixUp);
    for (i=0; i<someof(size); ++i) {
      p = pha[2L*i];
      p -= binMidFrq;
      p *= twoPiOnSr;
      p += expectedDphas;
      MMmaskPhs(p,j,oneOnPi);
      /* MmaskPhs(p);     */
      pha[2L*i] = p;
      expectedDphas += eDphIncr;
      expectedDphas -= TWOPI_F*(MYFLT)((int32_t)(expectedDphas*oneOnPi));
      binMidFrq += frqPerBin;
    }
    /* Does not deal with 'phases' of DC & fs/2 any different */
}

/* Unpack stored mag/frq data into buffer */
void FetchIn(
    float   *inp,       /* pointer to input data */
    MYFLT   *buf,       /* where to put our nice mag/frq pairs */
    int32    fsize,      /* frame size we're working with */
    MYFLT   pos)        /* fractional frame we want */
{
    int32    j;
    float   *frm_0, *frm_1;
    int32    base;
    MYFLT   frac;

    /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
    base = (int32) pos;          /* index of basis frame of interpolation */
    frac = (MYFLT) pos - (MYFLT) base;
    /* & how close to get to next */
    frm_0 = inp  + ((int32) fsize + 2L) * base;
    frm_1 = frm_0 + ((int32) fsize + 2L);          /* addresses of both frames */
    if (frac != FL(0.0)) {      /* must have 2 cases to avoid poss seg vlns */
                                /* and failed computes, else may interp     */
                                        /* bd valid data                    */
      for (j = 0; j <= fsize; j += 2) { /* mag/frq for just over 1/2        */
                                        /* Interpolate both mag and freq    */
        buf[j     ] = frm_0[j     ] + frac * (frm_1[j     ] - frm_0[j     ]);
        buf[j + 1L] = frm_0[j + 1L] + frac * (frm_1[j + 1L] - frm_0[j + 1L]);
      }
    }
    else {                  /* frac is 0.0 i.e. just copy the source frame */
      for (j = 0; j <= fsize; j += 2) { /* no need to interpolate */
        buf[j     ] = frm_0[j     ];
        buf[j + 1L] = frm_0[j + 1L];
      }
    }
}

void ApplyHalfWin(MYFLT *buf, MYFLT *win, int32 len)
    /* Window only store 1st half, is symmetric */
{
    int32 j;
    int32    lenOn2 = (len/2L);

    for (j = lenOn2 + 1; j--; )
      *buf++ *= *win++;
    for (j = len - lenOn2 - 1, win--; j--; )
      *buf++ *= *--win;
}

/* Overlap (some of) new data window with stored previous data
   in circular buffer */
void addToCircBuf(
    MYFLT   *sce, MYFLT *dst, /* linear source and circular destination */
    int32    dstStart,         /* Current starting point index in circular dst */
    int32    numToDo,          /* how many points to add ( <= circBufSize ) */
    int32    circBufSize)      /* Size of circ buf i.e. dst[0..circBufSize-1] */
{
    int32    i;
    int32    breakPoint;     /* how many points to add before having to wrap */

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

/* Write from a circular buffer into a linear output buffer CLEARING DATA */
void writeClrFromCircBuf(
    MYFLT   *sce, MYFLT *dst, /* Circular source and linear destination */
    int32    sceStart,         /* Current starting point index in circular sce */
    int32    numToDo,          /* How many points to write ( <= circBufSize ) */
    int32    circBufSize)      /* Size of circ buf i.e. sce[0..circBufSize-1] */
{
    int32    i;
    int32    breakPoint;     /* how many points to add before having to wrap */

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

/********************************************************************/
/*  udsample.c      -       from dsampip.c                          */
/*  Performs sample rate conversion by interpolated FIR LPF approx  */
/*  VAX, CSOUND VERSION                                             */
/*  1700 07feb90 taken from rational-only version                   */
/*  1620 06dec89 started dpwe                                       */
/********************************************************************/

/* (ugens7.h) #define   SPDS (8) */    /* How many sinc lobes to go out */
/* Static function prototypes - moved to top of file */

void UDSample(
    PVOC_GLOBALS  *p,
    MYFLT   *inSnd,
    MYFLT   stindex,
    MYFLT   *outSnd,
    int32    inLen,
    int32    outLen,
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
    int32_t     in2out;
    int32    i,j,x;
    MYFLT   a;
    MYFLT   phasePerInStep, fracInStep;
    MYFLT   realInStep, stepInStep;
    int32    nrstInStep;
    MYFLT   posPhase, negPhase;
    MYFLT   lex = FL(1.0)/fex;
    int32_t     nrst;
    MYFLT   frac;

    phasePerInStep = (lex > 1 ? FL(1.0) : lex) * (MYFLT) SPTS;
    /* If we are upsampling, LPF is at input frq => sinc pd matches */
    /*  downsamp => lpf at output rate; input steps at some fraction */
    in2out = (int32_t) ((MYFLT) SPDS * (fex < FL(1.0) ? FL(1.0) : fex));
    /* number of input points contributing to each op: depends on LPF */
    realInStep = stindex;
    stepInStep = fex;
    for (i = 0; i<outLen; ++i) {       /* output sample loop      */
                                       /* i = lex*nrstIp, so ..   */
      nrstInStep = (int32)realInStep;  /* imm. prec actual sample */
      fracInStep = realInStep-(MYFLT)nrstInStep;  /* Fractional part */
      negPhase = phasePerInStep * fracInStep;
      posPhase = -negPhase;
      /* cum. sinc arguments for +ve & -ve going spans into input */
      nrst = (int32_t)negPhase;       frac = negPhase - (MYFLT)nrst;
      a = (p->dsputil_sncTab[nrst]
           + frac * (p->dsputil_sncTab[nrst + 1]
                     - p->dsputil_sncTab[nrst]))
          * (MYFLT) inSnd[nrstInStep];
      for (j=1L; j<in2out; ++j) { /* inner FIR convolution loop */
        posPhase += phasePerInStep;
        negPhase += phasePerInStep;
        if ( (x = nrstInStep-j)>=0L ) { /* Brackets inserted here */
          nrst = (int32_t)negPhase;   frac = negPhase - (MYFLT)nrst;
        }
        a += (p->dsputil_sncTab[nrst]
              + frac * (p->dsputil_sncTab[nrst + 1]
                        - p->dsputil_sncTab[nrst]))
             * (MYFLT) inSnd[x];
        if ( (x = nrstInStep+j)<inLen ) { /* Brackets inserted here */
          nrst = (int32_t)posPhase;   frac = posPhase - (MYFLT)nrst;
        }
        a += (p->dsputil_sncTab[nrst]
              + frac * (p->dsputil_sncTab[nrst + 1]
                        - p->dsputil_sncTab[nrst]))
             * (MYFLT) inSnd[x];
      }
      outSnd[i] = (float)a;
      realInStep += stepInStep;
    }
}

/*--------------------------------------------------------------------*/
/*---------------------------- sinc module ---------------------------*/
/*--------------------------------------------------------------------*/

/* (ugens7.h) #define SPTS (16) */ /* How many points in each lobe */

void MakeSinc(PVOC_GLOBALS *p)  /* initialise our static sinc table */
{
    int32_t     i;
    int32_t     stLen = SPDS*SPTS;  /* sinc table is SPDS/2 periods of sinc */
    MYFLT   theta   = FL(0.0);  /* theta (sinc arg) reaches pi in SPTS */
    MYFLT   dtheta  = (MYFLT)(SBW*PI)/(MYFLT)SPTS;/* SBW lowers cutoff to redcali */
    MYFLT   phi     = FL(0.0);     /* phi (hamm arg) reaches pi at max ext */
    MYFLT   dphi    = PI_F/(MYFLT)(SPDS*SPTS);

    if (p->dsputil_sncTab == NULL)
      p->dsputil_sncTab =
          (MYFLT*) p->csound->Malloc(p->csound, (stLen + 1) * sizeof(MYFLT));
    /* (stLen+1 to include final zero; better for interpolation etc) */
    p->dsputil_sncTab[0] = FL(1.0);
    for (i = 1; i <= stLen; ++i) { /* build table of sin x / x */
      theta += dtheta;
      phi   += dphi;
      p->dsputil_sncTab[i] =
          SIN(theta) / theta * (FL(0.54) + FL(0.46) * COS(phi));
      /* hamming window on top of sinc */
    }
}

/****************************************/
/** prewarp.c module                    */
/****************************************/

/* spectral envelope detection: this is a very crude peak picking algorithm
   which is used to detect and pre-warp the spectral envelope so that
   pitch transposition can be performed without altering timbre.
   The basic idea is to disallow large negative slopes between
   successive values of magnitude vs. frequency.
*/

void PreWarpSpec(
                 //PVOC_GLOBALS  *p,
    MYFLT   *spec,      /* spectrum as magnitude,phase */
    int32    size,       /* full frame size, tho' we only use n/2+1 */
    MYFLT   warpFactor, /* How much pitches are being multd by */
     MYFLT *dsputil_env)
{
    MYFLT   eps,slope;
    MYFLT   mag, lastmag, nextmag, pkOld;
    int32    pkcnt, i, j;


    eps = -FL(64.0) / size;              /*  for spectral envelope estimation */
    lastmag = *spec;
    mag = spec[2*1];
    pkOld = lastmag;
    dsputil_env[0] = pkOld;
    pkcnt = 1;

    for (i = 1; i < someof(size); i++) {  /*  step thru spectrum */
      if (i < someof(size)-1)
        nextmag = spec[2*(i+1)];
      else nextmag = FL(0.0);

      if (pkOld != FL(0.0))
        slope = ((MYFLT) (mag - pkOld)/(pkOld * pkcnt));
      else
        slope = -FL(10.0);

      /*  look for peaks */
      if ((mag>=lastmag)&&(mag>nextmag)&&(slope>eps)) {
        dsputil_env[i] = mag;
        pkcnt--;
        for (j = 1; j <= pkcnt; j++) {
          dsputil_env[i - pkcnt + j - 1] =
            pkOld*(FL(1.0) + slope * j);
        }
        pkOld = mag;
        pkcnt = 1;
      }
      else
        pkcnt++;                    /*  not a peak */

      lastmag = mag;
      mag = nextmag;
    }

    if (pkcnt > 1) {                /*  get final peak */
      int32_t posi;
      mag = spec[2*(size/2)];
      slope = ((MYFLT) (mag - pkOld) / pkcnt);
      dsputil_env[size / 2] = mag;
      pkcnt--;
      for (j = 1; j <= pkcnt; j++) {
        posi = size / 2 - pkcnt + j - 1;
        if (posi > 0 && posi < size)
          dsputil_env[posi] = pkOld + slope * j;
      }
    }

    for (i = 0; i < someof(size); i++) {  /*  warp spectral env. */
      j = (int32)((MYFLT) i * warpFactor);
      //mag = spec[2*i];
      if ((j < someof(size)) && (dsputil_env[i] != FL(0.0)))
        spec[2 * i] *= dsputil_env[j] / dsputil_env[i];
      else
        spec[2*i] = FL(0.0);
    }
}

