/*
    fout.h:

    Copyright (C) 1999 Gabriel Maldonado, John ffitch

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

#ifndef FOUT_H
#define FOUT_H

#include "stdopcod.h"

typedef struct {
    OPDS        h;
    MYFLT       *fname, *iflag, *argums[VARGMAX];
    SNDFILE     *fp;
    MYFLT       scaleFac;
    int         idx;
    int         flag;
    int         nargs;
} OUTFILE;

typedef struct {
    OPDS        h;
    MYFLT       *fname, *iflag, *argums[VARGMAX];
    SNDFILE     *fp;
    MYFLT       scaleFac;
    int         idx;
    int         flag;
    int         nargs;
} KOUTFILE;

typedef struct {
    OPDS        h;
    MYFLT       *fname, *iskpfrms, *iflag, *argums[VARGMAX];
    SNDFILE     *fp;
    MYFLT       scaleFac;
    int         idx;
    long        currpos;
    int         flag;
    int         nargs;
} INFILE;

typedef struct {
    OPDS        h;
    MYFLT       *fname, *iskpfrms, *iflag, *argums[VARGMAX];
    SNDFILE     *fp;
    MYFLT       scaleFac;
    int         idx;
    long        currpos;
    int         flag;
    int         nargs;
} KINFILE;

typedef struct {
    OPDS        h;
    MYFLT       *fname, *iskpfrms, *iflag, *argums[VARGMAX];
    long        currpos;
    int         flag;
} I_INFILE;

typedef struct {
    OPDS        h;
    MYFLT       *avar, *aincr;
} INCR;

typedef struct {
    OPDS        h;
    MYFLT       *argums[VARGMAX];
} CLEARS;

typedef struct {
    OPDS        h;
    MYFLT       *ihandle, *fname;
    /* iascii=0 open ascii (default), iflag=1 open binary */
    MYFLT       *iascii;
} FIOPEN;

typedef struct {
    OPDS        h;
    MYFLT       *ihandle, *iascii, *iflag, *argums[VARGMAX];
} IOUTFILE;

typedef struct {
    OPDS        h;
    MYFLT       *ihandle, *iascii, *iflag, *argums[VARGMAX];
    long        counter;
    int         done;
} IOUTFILE_R;

typedef struct {
    OPDS        h;
    MYFLT       *fname, *fmt, *argums[VARGMAX];
    FILE        *fp;
    int         idx;
    char        txtstring[8192];    /* Place to store the string printed */
} FPRINTF;

#endif  /* FOUT_H */

