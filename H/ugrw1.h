/*
    ugrw1.h:

    Copyright (C) 1997 Robin Whittle

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

                                                        /*      UGRW1.H */
/* These files are based on Robin Whittle's
 *       ugrw1.c of 27 August 1996
 * and   ugrw1.h of 7 January 1995
 *
 * In February 1997, John Fitch reformatted the comments and
 * cleaned up some code in printksset() - which was working fine
 * but was inelegantly coded.
 *
 *
 * Copyright notice - Robin Whittle  25 February 1997
 *
 * Documentation files, and the original .c and .h files, with more
 * spaced out comments, are available from http://www.firstpr.com.au
 *
 * The code in both ugrw1 and ugrw2 is copyright Robin Whittle.
 * Permission is granted to use this in whole or in part for any
 * purpose, provided this copyright notice remains intact and
 * any alterations to the source code, including comments, are
 * clearly indicated as such.
 */

#include "cs.h"

/*
 *      Unit generators by Robin Whittle                6 January 1996
 *      Header file containing data structures for UGRW1.C.
 */

/* TABLEW data structure used by all the tablew subroutines. */

typedef struct {
        OPDS    h;
        MYFLT   *xsig;          /* Input value to write to table. */
        MYFLT   *xndx;          /* Index into the table where want to write */
        MYFLT   *xfn;           /* Number of table we are writing to. */
        MYFLT   *ixmode;        /* Index mode (optional parm).
                                 * 0 --> Use raw xfn and ixoff.
                                 * 1 --> Use these with a range of 0 to 1 for
                                 * the entire range of the table. */
        MYFLT   *ixoff;         /* Offset (opt). Fixed value to add to ndx*/
        MYFLT   *iwgmode;       /* Wrap and guard point mode (optional)
                                 *      0 --> Limit indx to between 0 table len
                                 *      1 --> Index wraps around modulo len
                                 *      2 --> Write with 0.5 step offset, and
                                 *            write both 0 and guard with the
                                 *             same data.  */

        /* Internal variable for previous state of xfn.  */
        long    pfn;            /* Internal variable for what to multiply
                                 * the ndx and ixoff by. Set to 1 or table
                                 * length by tblwset() depending on ixmode.
                                 */
        long    xbmul;          /* Internal variable for iwrap and igmode. */
        int     iwgm;           /* Internal variable for offset. */
        MYFLT   offset;         /* Pointer to data structure used to access
                                 * function table. tblwset() writes this, based
                                 * on the value of xfn.
                                 */
        FUNC    *ftp;
} TABLEW;

/* TABLENG data structure used by function tableng to return the
 * length of the table. */

typedef struct {
        OPDS    h;
        MYFLT   *kout;          /* Output pointer */
        MYFLT   *xfn;           /* Points to the number of the table. */
} TABLENG;


/* TABLEGPW data structure used by function tablegpw to write the
 * guard point of a specified table. (No output arguments) */

typedef struct {
        OPDS    h;
        MYFLT   *xfn;           /* Points to number of table. */
} TABLEGPW;

/* TABLEMIX data structure used by function tablemix. */

typedef struct {
        OPDS    h;
        MYFLT   *dft, *doff, *len, *s1ft, *s1off, *s1g, *s2ft, *s2off, *s2g;

        /* Storage to remember what the table numbers were from a previous k
           cycle, and to store pointers to their FUNC data structures. */
        int     pdft;           /* Previous destination */
        int     ps1ft, ps2ft;   /* source function table numbers. */
        FUNC    *funcd, *funcs1, *funcs2;
} TABLEMIX;

/* TABLECOPY data structure used by function tablecopy. */

typedef struct {
        OPDS    h;
        MYFLT   *dft;           /* Destination function table number. */
        MYFLT   *sft;           /* Source function table number */

        /* Storage to remember what the table numbers were from a previous k
           cycle, and to store pointers to their FUNC data structures. */
        int     pdft;           /* Previous destination */
        int     psft;           /* source function table numbers. */
        FUNC    *funcd, *funcs;
} TABLECOPY;

/* TABLERA data structure used by tablera subroutine. */

typedef struct {
        OPDS    h;
        MYFLT   *adest;         /* A rate destination */
        MYFLT   *kfn;           /* Number of table read */
        MYFLT   *kstart;        /* Index mode within table */
        MYFLT   *koff;          /* Offset to add to table index */

        /* Internal variable for previous state of xfn. */
        long    pfn;            /* Pointer to function table data structure */
        FUNC    *ftp;
} TABLERA;

/* TABLEWA data structure used by tablewa subroutine. */

typedef struct {
        OPDS    h;
        MYFLT   *kstart;        /* Index mode within table we start reading.
                                 * Note this is also an input argument.  First
                                 * we read it to determine where we should start
                                 * writing the a rate samples. When we have
                                 * finished we write to it the number of the
                                 * next location which should be written. */
        MYFLT   *kfn;           /* Number of table we are reading from. */
        MYFLT   *asig;          /* a rate input signal. */
        MYFLT   *koff;          /* Offset to add to table index. */

        long    pfn;            /* Pointer to function table. */
        FUNC    *ftp;
} TABLEWA;

/*****************************************************************************/

/* Data structures for the zak family of ugens for patching data at i,
 * k or a rates.
 * See also four global variables declared in the C code.  */

/* ZAKINIT data structure for zakinit(). */
typedef struct {
        OPDS    h;
        MYFLT   *isizea;        /* Number of a locations, each an array of
                                 * ksmps long, to to reserve for a rate
                                 * patching */
        MYFLT   *isizek;        /* Number of locations for i or k rate
                                 * variables */
} ZAKINIT;

/* ZKR data structure for zir() and zkr(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Where to write the value read from zk */
        MYFLT   *ndx;           /* Location in zk space to read */
} ZKR;

/* ZKW data structure for ziw() and zkw(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;           /* Value to write */
        MYFLT   *ndx;           /* Locations to read */
} ZKW;

/* ZKWM data structure for ziwm() and zkwm(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;           /* Value to write */
        MYFLT   *ndx;           /* Locations to read */
        MYFLT   *mix;           /* 0 for write directly;  !0 for mix - add in */

} ZKWM;

/* ZKMOD data structure for zkmod(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Points to where to write output */
        MYFLT   *sig;           /* Value to modulate */
        MYFLT   *zkmod;         /* Which zk variable to use to modulate sig */

} ZKMOD;

/* ZKCL data structure for zkcl(). */
typedef struct {
        OPDS    h;
        MYFLT   *first;         /* First variable to clear */
        MYFLT   *last;          /* Final variable to clear */

} ZKCL;

/* ZAR data structure for zar(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Where to write the value */
        MYFLT   *ndx;           /* Location in za space to read */

} ZAR;

/* ZARG data structure for zarg(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Where to write the zk location */
        MYFLT   *ndx;           /* Location in za space to read */
        MYFLT   *kgain;         /* Gain to be given to signal read */
} ZARG;

/* ZAW data structure for zaw(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig, *ndx;
} ZAW;


/* ZAWM data structure for zawm(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;
        MYFLT   *ndx, *mix;     /* Locations to read; 0 for write directly, or addd in */
} ZAWM;

/* ZAWOD data structure for zamod(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;
        MYFLT   *sig, *zamod;   /* Value to modulate; Which za variable to use */
} ZAMOD;

/* ZACL data structure for zacl(). */
typedef struct {
        OPDS    h;
        MYFLT   *first, *last;
} ZACL;


/*****************************************************************************/

/* RDTIME data structure for timek(), times(), instimset(), instimek()
   and instimes().  */

typedef struct {
        OPDS    h;
        MYFLT   *rslt;
        long    instartk;
} RDTIME;


/*****************************************************************************/

        /* PRINTK data structure for printk() and printkset(). */
typedef struct {
        OPDS    h;
        MYFLT   *ptime;         /* How much time to leave between each print*/
        MYFLT   *val;           /* Value to print */
        MYFLT   *space;         /* Spaces to insert before printing */
        MYFLT   initime, ctime; /* Time when initialised; initialised */
        long    pspace;         /* How many spaces to print */
        long    cysofar;        /* Number of print cycles so far */
} PRINTK;

/* PRINTKS data structure for printks() and printksset()  */
typedef struct {
        OPDS    h;
        MYFLT   *ifilcod;       /* File name */
        MYFLT   *ptime;         /* How much time to leave between each print */
        MYFLT   *kvals[VARGMAX];/* values to print */
        MYFLT   initime, ctime; /* Time when initialised; Cycle time */
        long    cysofar;        /* Number of print cycles so far from 0 */
        char    txtstring[8192]; /* Place to store the string printed */
} PRINTKS;

/* an i-rate-only prints */
typedef struct {
        OPDS    h;
        MYFLT   *ifilcod;       /* File name */
        MYFLT   *kvals[VARGMAX];/* values to print */
} PRINTS;
/*****************************************************************************/


/* PEAK data structure for peakk() and peaka(). */
typedef struct {
        OPDS    h;
        MYFLT   *kpeakout;      /* Pointer to k or a rate input variable. */
        MYFLT   *xsigin;        /* Pointer to k rate input variable which,
                                 * if set to no zero, causes the ugen to
                                 * clear the accumulator.        */
} PEAK;

typedef struct {
        OPDS    h;
        MYFLT   *val, *space;
        MYFLT   oldvalue;
        int     pspace;

} PRINTK2;

typedef struct {
        OPDS    h;
        MYFLT   *ndx;
} IOZ;


int ftkrchkw(ENVIRON*,TABLEW *p);
int instimek(ENVIRON*,RDTIME *p);
int instimes(ENVIRON*,RDTIME *p);
int instimset(ENVIRON*,RDTIME *p);
int inz(ENVIRON*,IOZ *p);
int itablecopy(ENVIRON*,TABLECOPY *p);
int itablegpw(ENVIRON*,TABLEGPW *p);
int itablemix(ENVIRON*,TABLEMIX *p);
int itableng(ENVIRON*,TABLENG *p);
int itablew(ENVIRON*,TABLEW *p);
int itblchkw(ENVIRON*,TABLEW *p);
int ktablew(ENVIRON*,TABLEW  *p);
int ktablewkt(ENVIRON*,TABLEW *p);
int outz(ENVIRON*,IOZ *p);
int peaka(ENVIRON*,PEAK *p);
int peakk(ENVIRON*,PEAK *p);
int printk(ENVIRON*,PRINTK *p);
int printk2(ENVIRON*,PRINTK2 *p);
int printk2set(ENVIRON*,PRINTK2 *p);
int printks(ENVIRON*,PRINTKS *p);
int printkset(ENVIRON*,PRINTK *p);
int printksset(ENVIRON*,PRINTKS *p);
int printsset(ENVIRON*,PRINTS *p);
int ptblchkw(ENVIRON*,TABLEW *p);
int tablecopy(ENVIRON*,TABLECOPY *p);
int tablecopyset(ENVIRON*,TABLECOPY *p);
int tablegpw(ENVIRON*,TABLEGPW *p);
int tablemix(ENVIRON*,TABLEMIX *p);
int tablemixset(ENVIRON*,TABLEMIX *p);
int tableng(ENVIRON*,TABLENG *p);
int tablera(ENVIRON*,TABLERA *p);
int tableraset(ENVIRON*,TABLERA *p);
int tablew(ENVIRON*,TABLEW *p);
int tablewa(ENVIRON*,TABLEWA *p);
int tablewaset(ENVIRON*,TABLEWA *p);
int tablewkt(ENVIRON*,TABLEW *p);
int tblsetw(ENVIRON*,TABLEW *p);
int tblsetwkt(ENVIRON*,TABLEW *p);
int timek(ENVIRON*,RDTIME *p);
int timesr(ENVIRON*,RDTIME *p);
int zacl(ENVIRON*,ZACL *p);
int zakinit(ENVIRON*,ZAKINIT *p);
int zamod(ENVIRON*,ZAMOD *p);
int zar(ENVIRON*,ZAR *p);
int zarg(ENVIRON*,ZARG *p);
int zaset(ENVIRON*,ZAR *p);
int zaw(ENVIRON*,ZAW *p);
int zawm(ENVIRON*,ZAWM *p);
int zir(ENVIRON*,ZKR *p);
int ziw(ENVIRON*,ZKW *p);
int ziwm(ENVIRON*,ZKWM *p);
int zkcl(ENVIRON*,ZKCL *p);
int zkmod(ENVIRON*,ZKMOD *p);
int zkr(ENVIRON*,ZKR *p);
int zkset(ENVIRON*,ZKR *p);
int zkw(ENVIRON*,ZKW *p);
int zkwm(ENVIRON*,ZKWM *p);
void sprints(char *outstring, char *fmt, MYFLT **kvals, long numVals);



