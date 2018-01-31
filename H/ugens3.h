/*
    ugens3.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

/*                                                              UGENS3.H        */

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xamp, *kcps, *xcar, *xmod, *kndx, *ifn, *iphs;
        int32   mphs, cphs;
        int16   ampcod, carcod, modcod;
        FUNC    *ftp;
} FOSC;

typedef struct {
        OPDS    h;
        MYFLT   *ar1,*ar2,*xamp,*kcps,*ifn,*ibas,*imod1,*ibeg1,*iend1,
                *imod2,*ibeg2,*iend2;
        MYFLT   cpscvt;
        MYFLT   lphs;
        int16   mod1, mod2;
        MYFLT   beg1, beg2;
        MYFLT   end1, end2;
        int16   seg1, curmod, looping, stereo;
        FUNC    *ftp;
} LOSC;

typedef struct {
        OPDS    h;
        MYFLT   *sphs, *ar1,*ar2,*xamp,*kcps,*ifn,*ibas,*imod1,*ibeg1,*iend1,
                *imod2,*ibeg2,*iend2;
        MYFLT   cpscvt;
        MYFLT   lphs;
        int16   mod1, mod2;
        MYFLT   beg1, beg2;
        MYFLT   end1, end2;
        int16   seg1, curmod, looping, stereo;
        FUNC    *ftp;
} LOSCPHS;

typedef struct {
        int16   tim;
        int16   val;
} DUPLE;

typedef struct ptlptr {
        struct ptlptr *nxtp;
        DUPLE   *ap;
        DUPLE   *fp;
        int16   amp,frq;
        int32   phs;
} PTLPTR;

#define MAXPTLS 50    /* must agree with hetro.c */

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *kamod, *kfmod, *ksmod, *ifilcod, *dum;
        MEMFIL  *mfp;
        int32   mksecs;
        AUXCH   aux;            /* PTLPTR  ptlptrs[MAXPTLS + 1]; make dynamic */
} ADSYN;
