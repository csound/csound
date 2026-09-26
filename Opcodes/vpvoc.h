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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once

typedef struct {
    FUNC    *function, *nxtfunction;
    int32   duration, cnt;
} TSEG;

typedef struct {
    OPDS    h;
    cs_float   *argums[VARGMAX];
    TSEG    *cursegp;
    FUNC    *outfunc;
    int32   nsegs;
    AUXCH   auxch, outaux;
    FUNC    outtable;
} TABLESEG;

typedef struct {
    OPDS    h;
    cs_float   *rslt, *ktimpnt, *kfmod, *ifilno, *ispecwp, *isegtab;
    char    *strarg;
    int32   kcnt;
    int32   baseFr, maxFr, frSiz, prFlg, opBpos;
    /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
    cs_float   frPktim, frPrtim, asr, scale, lastPex;
    float   *frPtr;
    /* asr is analysis sample rate */
    /* fft frames per k-time (equals phase change expansion factor) */
    AUXCH   auxch;          /* manage AUXDS for the following 5 buffer spaces */
    cs_float   *lastPhase;     /* [PVDATASIZE] Keep track of cum. phase */
    cs_float   *fftBuf;        /* [PVFFTSIZE]  FFT works on Real & Imag */
    cs_float   *dsBuf;         /* [PVFFTSIZE]  Output of downsampling may be 2x */
    cs_float   *outBuf;        /* [PVFFTSIZE]  Output buffer over win length */
    cs_float   *window;        /* [PVWINLEN]   Store 1/2 window */
    TABLESEG *tableseg;
    AUXCH   auxtab;         /* For table is all else fails */
    PVOC_GLOBALS  *pp;
    AUXCH memenv;
  void *setup;
} VPVOC;

