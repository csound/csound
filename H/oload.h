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
#define NGOTOS   40

#ifndef MIN_SHORT
#define MIN_SHORT (-32768)
#endif
extern  ENVIRON cenviron;
#define LABELIM  (MIN_SHORT + nlabels)
#define CHKING   0
#define CBAS    32250
#define VBAS    32400
#define GVBAS   (VBAS + PMAX)           /* 32550 */
#define VMAGIC  0x56444154L     /* "VDAT" */

#define STRSMAX 8

typedef struct  {
        char    *namep;
        short   type, count;
} NAME;

typedef struct  {
        char    *lbltxt;
        short   *ndxp;
} LBLARG;

typedef struct  {
        short   lblno;
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
        int     insno;
        long    ktime;
} TRNON;

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
