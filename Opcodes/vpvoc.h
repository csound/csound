/*
    vpvoc.h:

    Copyright (C) 1992 Richard Karpen

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

typedef struct {
    FUNC    *function, *nxtfunction;
    MYFLT   d;
    int32   cnt;
} TSEG;

typedef struct {
    OPDS    h;
    MYFLT   *argums[VARGMAX];
    TSEG    *cursegp;
    FUNC    *outfunc;
    int32   nsegs;
    AUXCH   auxch;
} TABLESEG;

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *ktimpnt, *kfmod, *ifilno, *ispecwp, *isegtab;
    char    *strarg;
    int32   kcnt;
    int32   baseFr, maxFr, frSiz, prFlg, opBpos;
    /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
    MYFLT   frPktim, frPrtim, asr, scale, lastPex;
    float   *frPtr;
    /* asr is analysis sample rate */
    /* fft frames per k-time (equals phase change expansion factor) */
    AUXCH   auxch;          /* manage AUXDS for the following 5 buffer spaces */
    MYFLT   *lastPhase;     /* [PVDATASIZE] Keep track of cum. phase */
    MYFLT   *fftBuf;        /* [PVFFTSIZE]  FFT works on Real & Imag */
    MYFLT   *dsBuf;         /* [PVFFTSIZE]  Output of downsampling may be 2x */
    MYFLT   *outBuf;        /* [PVFFTSIZE]  Output buffer over win length */
    MYFLT   *window;        /* [PVWINLEN]   Store 1/2 window */
    TABLESEG *tableseg;
    AUXCH   auxtab;         /* For table is all else fails */
    PVOC_GLOBALS  *pp;
    AUXCH memenv;
} VPVOC;

