/*  
    ugens2.h:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*                                                              UGENS2.H        */

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *iphs;
        double  curphs;
} PHSOR;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *xndx, *xfn, *ixmode, *ixoff, *iwrap;
        MYFLT   offset;
        long    pfn; /* Previous function table number - used to
                        detect a change in table number when this is
                        supplied by a k rate input parameter. */
        long    xbmul, wrap;
        FUNC    *ftp;
} TABLE;

typedef struct {
        OPDS    h;
        MYFLT   *rslt, *idel, *kamp, *idur, *ifn;
        long    kinc, phs;
        long    dcnt;
        FUNC    *ftp;
} OSCIL1;

typedef struct  {
        OPDS    h;
        MYFLT   *rslt, *kamp, *ifrq, *ifn, *itimes;
        MYFLT   index, inc, maxndx;
        long    ntimes;
        FUNC    *ftp;
} OSCILN;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xamp, *xcps, *ifn, *iphs;
        long    lphs;
        FUNC    *ftp;
} OSC;
