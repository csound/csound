/*
    ugensa.h:

    Copyright (C) 1997 J. Michael Clarke, based on ideas from CHANT (IRCAM)

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

/*                                                      UGENSM.H  */

#pragma once

#define PFRAC1(x)   ((MYFLT)((x) & ftp1->lomask) * ftp1->lodiv)

typedef struct overlap {
  struct overlap *nxtact;
  struct overlap *nxtfree;
  int32          timrem, dectim, formphs, forminc;
  int32         risphs;
  int32          risinc, decphs, decinc;
  double         formphsf, formincf, risphsf, risincf, decphsf, decincf;
  MYFLT          curamp, expamp;
} OVERLAP;

typedef struct {
  OPDS  h;
  MYFLT *ar, *xamp, *xdens, *xtrans, *xspd, *koct, *kband, *kris, *kdur, *kdec;
  MYFLT *iolaps, *ifna, *ifnb, *itotdur, *iphs, *itmode, *iskip;
  OVERLAP       basovrlap;
  int32 durtogo, fundphs, fofcount, prvsmps, spdphs; /*last added JMC for FOG*/
  MYFLT fundphsf, spdphsf;
  MYFLT prvband, expamp, preamp, fogcvt; /*last added JMC for FOG*/
  int16 xincod, ampcod, fundcod;
  int16 formcod, fmtmod, speedcod; /*last added JMC for FOG*/
  AUXCH auxch;
  FUNC  *ftp1, *ftp2;
  int32   floatph;         /* floating-point phase */
} FOGS;

/*typedef struct {
        OPDS    h;
        MYFLT   *sr, *xamp, *xcps, *ifn, *iphs;
        int32   lphs;
        FUNC    *ftp;
} JMC;
*/
