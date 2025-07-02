/*
    sfont.h:

    Copyright (C) 2000 Gabriel Maldonado

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

#pragma once

#include "sftype.h"
#include "sf.h"

typedef struct {
        OPDS    h;
        MYFLT   *ihandle, *fname;
} SFLOAD;

typedef struct {
        OPDS    h;
        MYFLT   *ihandle;
        STRINGDAT *Sprefix;
} SFPLIST;

typedef struct {
        OPDS    h;
  MYFLT   *startNum,*ihandle, *msgs;
} SFPASSIGN;

typedef struct {
        OPDS    h;
        MYFLT   *ipresethandle, *iprog, *ibank, *isfhandle, *iPresetHandle;
} SFPRESET;

#define MAXSPLT 10

typedef struct {
        OPDS    h;
        MYFLT   *out1, *out2, *ivel, *inotnum,*xamp, *xfreq;
        MYFLT   *ipresethandle, *iflag, *ioffset, *ienv, *iskip;
        int32_t     spltNum;
        SHORT   *base[MAXSPLT], mode[MAXSPLT];
        DWORD   end[MAXSPLT], startloop[MAXSPLT], endloop[MAXSPLT], ti[MAXSPLT];
        double  si[MAXSPLT],phs[MAXSPLT];
        MYFLT   leftlevel[MAXSPLT], rightlevel[MAXSPLT], attack[MAXSPLT],
                decay[MAXSPLT], sustain[MAXSPLT], release[MAXSPLT];
        MYFLT   attr[MAXSPLT], decr[MAXSPLT];
        MYFLT   env[MAXSPLT];
} SFPLAY;

typedef struct {
        OPDS    h;
        MYFLT   *out1, *ivel, *inotnum,*xamp, *xfreq, *ipresethandle,
                *iflag, *ioffset, *ienv, *iskip;
        int32_t     spltNum;
        SHORT   *base[MAXSPLT], mode[MAXSPLT];
        DWORD   end[MAXSPLT], startloop[MAXSPLT], endloop[MAXSPLT], ti[MAXSPLT];
        double  si[MAXSPLT],phs[MAXSPLT];
        MYFLT   attenuation[MAXSPLT],attack[MAXSPLT], decay[MAXSPLT],
                sustain[MAXSPLT], release[MAXSPLT];
        MYFLT   attr[MAXSPLT], decr[MAXSPLT];
        MYFLT   env[MAXSPLT];
} SFPLAYMONO;

typedef struct {
        OPDS    h;
        MYFLT   *out1, *ivel, *inotnum, *xamp, *xfreq, *instrNum;
        MYFLT   *sfBank, *iflag, *ioffset, *ienv, *iskip;
        int32_t     spltNum;
        SHORT   *base[MAXSPLT], mode[MAXSPLT];
        DWORD   end[MAXSPLT], startloop[MAXSPLT], endloop[MAXSPLT], ti[MAXSPLT];
        double  si[MAXSPLT],phs[MAXSPLT];
        MYFLT   attenuation[MAXSPLT],attack[MAXSPLT], decay[MAXSPLT],
                sustain[MAXSPLT], release[MAXSPLT];
        MYFLT   attr[MAXSPLT], decr[MAXSPLT];
        MYFLT   env[MAXSPLT];
} SFIPLAYMONO;

typedef struct {
        OPDS    h;
        MYFLT   *out1, *out2, *ivel, *inotnum, *xamp, *xfreq;
        MYFLT   *instrNum, *sfBank, *iflag, *ioffset, *ienv, *iskip;
        int32_t spltNum;
        SHORT   *base[MAXSPLT], mode[MAXSPLT];
        DWORD   end[MAXSPLT], startloop[MAXSPLT], endloop[MAXSPLT], ti[MAXSPLT];
        double  si[MAXSPLT],phs[MAXSPLT];
        MYFLT   leftlevel[MAXSPLT], rightlevel[MAXSPLT],attack[MAXSPLT],
                decay[MAXSPLT], sustain[MAXSPLT], release[MAXSPLT];
        MYFLT   attr[MAXSPLT], decr[MAXSPLT];
        MYFLT   env[MAXSPLT];
} SFIPLAY;
