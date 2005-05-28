/*
    pvinterp.h:

    Copyright (C) 1996 Richard Karpen

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

/*                                                              pvinterp.h    */

#include "ugens8.h"

typedef struct {
    OPDS    h;
    MYFLT  *ktimpnt, *ifilno;
    MEMFIL      *mfp;
    long    maxFr, frSiz, prFlg;
    /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
    MYFLT   frPktim, frPrtim, asr, *frPtr;
    AUXCH   auxch;
    MYFLT   *lastPhase, *fftBuf;  /* [PVFFTSIZE] FFT works on Real & Imag */
        MYFLT   *buf;
} PVBUFREAD;

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *ktimpnt, *kfmod, *ifilno,
            *kfreqscale1, *kfreqscale2, *kampscale1, *kampscale2,
            *kfreqinterp, *kampinterp;

    MEMFIL  *mfp;
    long    kcnt;
    long    baseFr, maxFr, frSiz, prFlg, opBpos;
     /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
    MYFLT   frPktim, frPrtim, scale, asr, *frPtr, lastPex;
     /* asr is analysis sample rate */
     /* fft frames per k-time (equals phase change expansion factor) */
    AUXCH   auxch;      /* manage AUXDS for the following 5 buffer spaces */
    MYFLT   *lastPhase; /* [PVDATASIZE] Keep track of cum. phase */
    MYFLT   *fftBuf;    /* [PVFFTSIZE]  FFT works on Real & Imag */
    MYFLT   *dsBuf;     /* [PVFFTSIZE]  Output of downsampling may be 2x */
    MYFLT   *outBuf;    /* [PVFFTSIZE]  Output buffer over win length */
    MYFLT   *window;    /* [PVWINLEN]   Store 1/2 window */
    PVBUFREAD *pvbufread;

} PVINTERP;

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *ktimpnt, *kfmod, *ifilno,
            *kampscale1, *kampscale2, *ispecwp;
    MEMFIL  *mfp;
    long    kcnt;
    long    baseFr, maxFr, frSiz, prFlg, opBpos;
     /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
    MYFLT   frPktim, frPrtim, scale, asr, *frPtr, lastPex;
     /* asr is analysis sample rate */
     /* fft frames per k-time (equals phase change expansion factor) */
    AUXCH   auxch;      /* manage AUXDS for the following 5 buffer spaces */
    MYFLT   *lastPhase; /* [PVDATASIZE] Keep track of cum. phase */
    MYFLT   *fftBuf;    /* [PVFFTSIZE]  FFT works on Real & Imag */
    MYFLT   *dsBuf;     /* [PVFFTSIZE]  Output of downsampling may be 2x */
    MYFLT   *outBuf;    /* [PVFFTSIZE]  Output buffer over win length */
    MYFLT   *window;    /* [PVWINLEN]   Store 1/2 window */
    PVBUFREAD *pvbufread;

} PVCROSS;
