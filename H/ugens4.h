/*  
    ugens4.h:

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

/*                                                      UGENS4.H        */

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xcps, *knh, *ifn, *iphs;
        short   ampcod, cpscod;
        long    lphs;
        FUNC    *ftp;
        int     reported;
} BUZZ;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xcps, *kn, *kk, *kr, *ifn, *iphs;
        short   ampcod, cpscod, prvn;
        MYFLT   prvr, twor, rsqp1, rtn, rtnp1, rsumr;
        long    lphs;
        FUNC    *ftp;
        int     reported;
        MYFLT   last;
} GBUZZ;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *kamp, *kcps, *icps, *ifn, *imeth, *ipar1, *ipar2;
        MYFLT   sicps, param1, param2;
        short   thresh1, thresh2, method;
        long    phs256, npts, maxpts;
        AUXCH   auxch;
} PLUCK;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *iseed, *sel, *base;
        int     rand;
        short   ampcod;
        short   new;
} RAND;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xcps, *iseed, *sel, *base;
        short   ampcod, cpscod, new;
        int     rand;
        long    phs;
        MYFLT   num1;
} RANDH;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xcps, *iseed, *sel, *base;
        short   ampcod, cpscod, new;
        int     rand;
        long    phs;
        MYFLT   num1, num2, dfdmax;
} RANDI;
