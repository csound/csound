#ifndef FOUT_H
#define FOUT_H
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
#include "cs.h"

typedef struct {
        OPDS    h;
        MYFLT   *fname,*iflag, *argums[VARGMAX];
        SNDFILE *fp;
        int     idx;
        SUBR    outfilep;
        int     flag;
        int     nargs;
        long    cnt;
} OUTFILE;

typedef struct {
        OPDS    h;
        MYFLT   *fname,*iflag, *argums[VARGMAX];
        SNDFILE *fp;
        int     idx;
        SUBR    koutfilep;
        int     flag;
        int nargs;
        long    cnt;
} KOUTFILE;

typedef struct {
        OPDS    h;
        MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX];
        SNDFILE *fp;
        int     idx;
        SUBR    infilep;
        long currpos;
        int     flag;
        int nargs;
} INFILE;

typedef struct {
        OPDS    h;
        MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX];
        SNDFILE *fp;
        int     idx;
        SUBR kinfilep;
        long currpos;
        int     flag;
        int nargs;
} KINFILE;

typedef struct {
        OPDS    h;
        MYFLT   *fname, *iskpfrms, *iflag, *argums[VARGMAX];
        long    currpos;
        int     flag;
} I_INFILE;


typedef struct {
        OPDS    h;
        MYFLT   *avar,*aincr;
} INCR;


typedef struct {
        OPDS    h;
        MYFLT   *argums[VARGMAX];
} CLEARS;


typedef struct {
        OPDS    h;
        MYFLT   *ihandle, *fname;
        MYFLT   *iascii; /* iascii=0 open ascii (default), iflag=1 open binary */
} FIOPEN;


typedef struct {
        OPDS    h;
        MYFLT   *ihandle, *iascii, *iflag, *argums[VARGMAX];
        /*void (*ioutfilep) (void *); */
} IOUTFILE;


typedef struct {
        OPDS    h;
        MYFLT   *ihandle, *iascii, *iflag, *argums[VARGMAX];
        long counter;
        int done;
        /*  void (*ioutfilep) (void *); */
} IOUTFILE_R;


typedef struct {
        OPDS    h;
        MYFLT   *fname, *fmt, *argums[VARGMAX];
        FILE    *fp;
        int     idx;
        char    txtstring[8192]; /* Place to store the string printed */
} FPRINTF;

int fprintf_i(FPRINTF *);
int fprintf_k(FPRINTF *);
int fprintf_set(FPRINTF *p);
int incr(INCR *p);
int clear(CLEARS *p);
int outfile_set(OUTFILE *p);
int outfile (OUTFILE *p);
int koutfile_set(KOUTFILE *p);
int koutfile(KOUTFILE *p);
int fiopen(FIOPEN *p);
int ioutfile_set(IOUTFILE *p);
int ioutfile_set_r(IOUTFILE_R *p);
int ioutfile_r(IOUTFILE_R *p);
int infile_set(INFILE *p);
int infile(INFILE *p);
int kinfile_set(KINFILE *p);
int kinfile(KINFILE *p);
int i_infile(I_INFILE *p);
 
#endif /* FOUT_H */
