/*
    oload.h:

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

#define NCONSTS 400    /* gbl */                /*      OLOAD.H         */
#define LNAMES  400    /* lcl */
#define GNAMES  100    /* gbl */
#define NLABELS 5      /* lcl */
#define NGOTOS  40

#define LABELOFS    (-0x40000000)
#define LABELIM     (-0x38000000)
#ifndef MIN_SHORT
#define MIN_SHORT (-32768)
#endif

#define STR_OFS 0x78000000      /* string constant index base */
#define VMAGIC  0x56444154      /* "VDAT" */

#define STRSMAX 8

typedef struct  {
        char    *namep;
        int     type, count;
} NAME;

typedef struct  {
        char    *lbltxt;
        int     *ndxp;
} LBLARG;

typedef struct  {
        int     lblno;
        MYFLT   **argpp;
} LARGNO;

typedef struct  {
        MYFLT   *ndx, *string;
} STRNG;

typedef struct  {
        MYFLT   *sets[PMAX];
} PVSET;

typedef struct  {
        MYFLT   *dimen;
} VDIMEN;

typedef struct  {
        OPDS    h;
        MYFLT   *pgm, *vals[32];
} PGM_INIT;

typedef struct inx {
        struct inx *nxtinx;
        int ctrlno;
        int inscnt;
        int inslst[1];
} INX;

typedef struct {
        OPDS   h;
        MYFLT  *insno, *itime;
} TURNON;

typedef struct  pgmbnk {
        struct  pgmbnk *nxtbnk;
        long    bankno;
        MYFLT   *vpgdat;
        MYFLT   *spldat;
} PGMBNK;

typedef struct  {
        INSTRTXT *instxt;
        PGMBNK  *banks;
} VPGLST;

PGMBNK *getchnbnk(MCHNBLK*);
MYFLT *getkeyparms(MCHNBLK*,int);

