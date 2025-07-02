/*
    ugens8.h:

    Copyright (C) 1991, 1998, 2000 Dan Ellis, Richard Karpen, Richard Dobson

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

/*                                                              UGENS8.H    */
#ifndef _UGENS8_H_
#define _UGENS8_H_
#define     PVFRAMSIZE      8192                /* i.e. max FFT point size */
#define     PVFFTSIZE       (2*PVFRAMSIZE)      /* 2x for real + imag */
#define     PVDATASIZE      (1+PVFRAMSIZE/2)    /* Need 1/2 channels + mid */
#define     PVWINLEN        (4097)              /* time window 1st half */

/* PVDATASIZE reflects the fact that for n point _real_ time data, the fourier
 *  transform will only have n degrees of freedom, although it has 2n values
 *  (n bins x {re,im} or {mag,phase}). This constraint is reflected by the top
 *  n/2-1 bins (bins n/2+1 .. n-1) being complex conjugates of bins 1..n/2-1.
 *  Bins 0 and n/2 do not have conjugate images, but they ARE always real,
 *  so only contribute one degree of freedom each.  So the number of degrees of
 *  freedom in the complex FFT is {re,im}*(n/2 - 1) +2 = n , as expected.
 *  Thus we only need to store and process these independent values.  However,
 *  for convenience, and because our analysis system records the phase of
 *  bins 0 and n/2 as 0 or pi rather than making the magnitude negative, we
 *  allow these 2 bins to have imaginary components too, so that FFT frames are
 *  stored as Magnitude & phase for bins 0..n/2 = 2*(n/2 + 1) or n+2 values.
 *  These are the n+2 channels interleaved in the PVOC analysis file, and
 *  then stored and processed wherever you see PVDATA/FRDA (frame data) */

#define     pvfrsiz(p)      (p->frSiz)
#define     pvffsiz(p)      (2* p->frSiz)
#define     pvdasiz(p)      ((uint32_t)(1 + (p->frSiz)/2)) /* as above, based on */
#define     pvfdsiz(p)      (2 + p->frSiz)      /*  ACTUAL frameSize in use */

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *ktimpnt, *kfmod, *ifilno, *ispecwp, *imode;
    MYFLT   *ifreqlim, *igatefun;
    int32   mems;
    int32   kcnt, baseFr, maxFr, frSiz, prFlg, opBpos;
    /* RWD 8:2001 for pvocex: need these too */
    int32   frInc, chans;

    MYFLT   frPktim, frPrtim, scale, asr, lastPex;
    MYFLT   PvMaxAmp;
    float   *frPtr, *pvcopy;
    FUNC    *AmpGateFunc;
    AUXCH   auxch;
    MYFLT   *lastPhase; /* [PVDATASIZE] Keep track of cum. phase */
    MYFLT   *fftBuf;    /* [PVFFTSIZE]  FFT works on Real & Imag */
    MYFLT   *dsBuf;     /* [PVFFTSIZE]  Output of downsampling may be 2x */
    MYFLT   *outBuf;    /* [PVFFTSIZE]  Output buffer over win length */
    MYFLT   *window;    /* [PVWINLEN]   Store 1/2 window */
    MYFLT   *dsputil_env;
    AUXCH   memenv;
    PVOC_GLOBALS  *pp;
  void *setup;
} PVOC;

#endif

