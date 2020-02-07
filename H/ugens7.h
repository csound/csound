/*
    ugens7.h:

    Copyright (C) 1995 Michael Clarke, based on ideas from CHANT (IRCAM)
                  1996 Barry Vercoe

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

/*                                                      UGENS7.H        */

#define PFRAC1(x)   ((MYFLT)((x) & ftp1->lomask) * ftp1->lodiv)

typedef struct ovrlap {
        struct ovrlap  *nxtact, *nxtfree;
        int32    timrem, dectim, formphs, forminc, risphs, risinc, decphs, decinc;
        MYFLT   curamp, expamp;
        MYFLT   glissbas;/* Gliss factor to add to forminc (ifna index incr) */
        int32    sampct;         /* Sample count since grain started */
} OVRLAP;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *xamp, *xfund, *xform, *koct, *kband, *kris, *kdur, *kdec;
        MYFLT   *iolaps, *ifna, *ifnb, *itotdur, *iphs, *ifmode, *iskip;
  /* kgliss and ifmode are same field */
        OVRLAP  basovrlap;
        int32    durtogo, fundphs, fofcount, prvsmps;
        MYFLT   prvband, expamp, preamp;
        int16   foftype;        /* Distinguish fof and fof2 */
        int16   xincod, ampcod, fundcod, formcod, fmtmod;
        AUXCH   auxch;
        FUNC    *ftp1, *ftp2;
} FOFS;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *kest, *kvar, *kfrq1, *kfrq2;
        MYFLT   *icpsmode, *ilowest, *iptrkprd;
        int32   nbufsmps, n2bufsmps, phase1, phase2, period, autoktim, autokcnt;
        int32   mindist, maxdist, max2dist, lomaxdist, cpsmode;
        MYFLT   c1, c2, prvq, prvest, prvar, minfrq, estprd, lsicvt;
        MYFLT   *bufp, *midp, *inp1, *inp2;
        MYFLT   *bufq, *midq, *inq1, *inq2, *autobuf;
        MYFLT   *puls1, *puls2, *puls3, lin1, lin2, lin3;
        MYFLT   inc1, inc2, inc11, inc12, inc21, inc22, inc31, inc32;
        int32   cnt1, cnt2, cnt3, pnt1, pnt2, pnt3;
        int32   pnt11, pnt12, pnt13, pnt21, pnt22, pnt23, pnt31, pnt32, pnt33;
        AUXCH   auxch;
} HARMON;
