/*
    disprep.h:

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

                        /*                                      DISPREP.H       */
#include "pstream.h"

typedef struct {
        OPDS    h;
        MYFLT   *iargs[VARGMAX];
} PRINTV;

typedef struct {
        OPDS    h;
        MYFLT   *signal, *iprd, *inprds, *iwtflg;
        int32    npts, nprds, bufpts, totpts, pntcnt;
        WINDAT  dwindow;
        MYFLT   *nxtp, *begp, *endp;
        AUXCH   auxch;
} DSPLAY;

typedef struct {
        OPDS    h;
        PVSDAT  *fin;
        MYFLT   *points, *flag;
        int     size;
        WINDAT  dwindow;
        AUXCH   fdata;
        uint32  lastframe;
} FSIGDISP;

#define WINDMAX 16384
#define WINDMIN 16

typedef struct {
        OPDS    h;
        MYFLT   *signal, *iprd, *inpts, *ihann, *idbout, *iwtflg, *imin, *imax;
        MYFLT   *sampbuf, *bufp, *endp, overN;
        int32   windsize, overlap, ncoefs;
        int     hanning, dbout;
        int     npts, start;
        WINDAT  dwindow;
        AUXCH   auxch;
        AUXCH  smpbuf;
} DSPFFT;

typedef struct {
        OPDS    h;
        MYFLT   *kout,*kin,*iprd,*imindur,*imemdur,*ihp,*ithresh,*ihtim,*ixfdbak;
        MYFLT   *istartempo,*ifn,*idisprd,*itweek;
        int     countdown, timcount, npts, minlam, maxlam;
        MYFLT   *hbeg, *hcur, *hend;
        MYFLT   *xbeg, *xcur, *xend;
        MYFLT   *stmemp, *linexp, *ftable, *xscale, *lmults;
        int16   *lambdas;
        MYFLT   *stmemnow, ncross, coef0, coef1, yt1, thresh;
        MYFLT   fwdcoef, fwdmask, xfdbak, avglam, tempscal, tempo, tweek;
        int     dcntdown, dtimcnt;
        WINDAT  dwindow;
        AUXCH   auxch;
} TEMPEST;

