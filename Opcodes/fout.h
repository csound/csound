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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef FOUT_H
#define FOUT_H

#include "stdopcod.h"

typedef struct FOUT_FILE_ {
    OPDS    h;
    SNDFILE *sf;
    FILE    *f;
    void    *fd;
    int32_t     bufsize;
    int32_t     nchnls;
    int32_t async;
    int32_t     idx;        /* file index + 1 */
} FOUT_FILE;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iflag, *argums[VARGMAX-2];
    MYFLT   scaleFac;
    int32_t     nargs;
    int32_t     buf_pos;
    int32_t     guard_pos;
    AUXCH   buf;
    FOUT_FILE f;
} OUTFILE;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iflag;
    ARRAYDAT* tabin;
    MYFLT   scaleFac;
    int32_t     buf_pos;
    int32_t     guard_pos;
    AUXCH   buf;
    FOUT_FILE f;
} OUTFILEA;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iflag, *argums[VARGMAX-2];
    MYFLT   scaleFac;
    uint32_t     nargs;
    int32_t     buf_pos;
    int32_t     guard_pos;
    AUXCH   buf;
    FOUT_FILE f;
} KOUTFILE;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX-3];
    MYFLT   scaleFac;
    int32   currpos;
    int32_t     flag;
    int32_t     nargs;
    int32_t     buf_pos;
    int32_t     guard_pos;
    int32_t     frames;
    uint32_t     remain;
    AUXCH   buf;
    FOUT_FILE f;
} INFILE;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iskpfrms, *iflag;
    ARRAYDAT *tabout;
    MYFLT   scaleFac;
    int32   currpos;
    int32_t     flag;
    int32_t     chn;
    int32_t     buf_pos;
    int32_t     guard_pos;
    int32_t     frames;
    uint32_t     remain;
    AUXCH   buf;
    FOUT_FILE f;
} INFILEA;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX-3];
    MYFLT   scaleFac;
    int32   currpos;
    int32_t     flag;
    int32_t     nargs;
    int32_t     buf_pos;
    int32_t     guard_pos;
    int32_t     frames;
    int32_t     remain;
    AUXCH   buf;
    FOUT_FILE f;
} KINFILE;

typedef struct {
    OPDS    h;
    MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX-3];
    int32   currpos;
    int32_t     flag;
} I_INFILE;

typedef struct {
    OPDS    h;
    MYFLT   *avar, *aincr;
} INCR;

typedef struct {
    OPDS    h;
    MYFLT   *argums[VARGMAX];
} CLEARS;

typedef struct {
    OPDS    h;
    MYFLT   *ihandle, *fname;
    /* iascii=0 open ascii (default), iflag=1 open binary */
    MYFLT   *iascii;
} FIOPEN;

typedef struct {
    OPDS    h;
    MYFLT   *iFile;
} FICLOSE;

typedef struct {
    OPDS    h;
    MYFLT   *ihandle, *iascii, *iflag, *argums[VARGMAX-3];
} IOUTFILE;

typedef struct {
    OPDS    h;
    MYFLT   *ihandle, *iascii, *iflag, *argums[VARGMAX-3];
    int32   counter;
    int32_t     done;
} IOUTFILE_R;

typedef struct {
    OPDS    h;
    MYFLT   *fname;
    STRINGDAT *fmt;
    MYFLT  *argums[VARGMAX-2];
    FOUT_FILE f;
    char    txtstring[8192];    /* Place to store the string printed */
} FPRINTF;

#endif  /* FOUT_H */

