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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

                        /*                                      DISPREP.H       */

typedef struct {
        OPDS    h;
        MYFLT   *iargs[VARGMAX];
} PRINTV;

typedef struct {
        OPDS    h;
        MYFLT   *signal, *iprd, *inprds, *iwtflg;
        long    npts, nprds, bufpts, totpts, pntcnt;
        WINDAT  dwindow;
        MYFLT   *nxtp, *begp, *endp;
        AUXCH   auxch;
} DSPLAY;

#define WINDMAX 4096
#define WINDMIN 16

typedef struct {
        OPDS    h;
        MYFLT   *signal, *iprd, *inpts, *ihann, *idbout, *iwtflg;
        MYFLT   sampbuf[WINDMAX], *bufp, *endp, *fftlut, overN;
        long    windsize, overlap, ncoefs;
        int     hanning, dbout;
        WINDAT  dwindow;
        AUXCH   auxch;
} DSPFFT;

typedef struct {
        OPDS    h;
        MYFLT   *kout,*kin,*iprd,*imindur,*imemdur,*ihp,*ithresh,*ihtim,*ixfdbak;
        MYFLT   *istartempo,*ifn,*idisprd,*itweek;
        int     countdown, timcount, npts, minlam, maxlam;
        MYFLT   *hbeg, *hcur, *hend;
        MYFLT   *xbeg, *xcur, *xend;
        MYFLT   *stmemp, *linexp, *ftable, *xscale, *lmults;
        short   *lambdas;
        MYFLT   *stmemnow, ncross, coef0, coef1, yt1, thresh;
        MYFLT   fwdcoef, fwdmask, xfdbak, avglam, tempscal, tempo, tweek;
        int     dcntdown, dtimcnt;
        WINDAT  dwindow;
        AUXCH   auxch;
} TEMPEST;

