/*
    pvadd.h:

    Copyright (C) 1998 Richard Karpen

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

/*                                                      PVADD.H    */

#pragma once

#define     MAXBINS         4096
#ifndef PVFRAMSIZE
#define     PVFRAMSIZE      8192                /* i.e. max FFT point size */
#endif
#define     PVFFTSIZE       (2*PVFRAMSIZE)      /* 2x for real + imag */
#define     pvfrsiz(p)      (p->frSiz)

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *ktimpnt, *kfmod, *ifilno, *ifn, *ibins;
    MYFLT   *ibinoffset, *ibinincr, *imode, *ifreqlim, *igatefun;
    FUNC    *ftp, *AmpGateFunc;
    AUXCH   auxch;
    MYFLT   *oscphase, *buf, PvMaxAmp;
    MYFLT   frPrtim, asr;
    float   *frPtr, *pvcopy;
    int32   maxFr, frSiz, prFlg, mems;
    int32_t maxbin, floatph;
} PVADD;

