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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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

#include "csoundCore.h"

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
        int32    pfn;            /* Internal variable for what to multiply
                                 * the ndx and ixoff by. Set to 1 or table
                                 * length by tblwset() depending on ixmode.
                                 */
        int32    xbmul;          /* Internal variable for iwrap and igmode. */
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
        int32    pfn;            /* Pointer to function table data structure */
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

        int32    pfn;            /* Pointer to function table. */
        FUNC    *ftp;
} TABLEWA;

/*****************************************************************************/
/*****************************************************************************/

/* RDTIME data structure for timek(), times(), instimset(), instimek()
   and instimes().  */

typedef struct {
        OPDS    h;
        MYFLT   *rslt;
        int32    instartk;
} RDTIME;

/*****************************************************************************/

        /* PRINTK data structure for printk() and printkset(). */
typedef struct {
        OPDS    h;
        MYFLT   *ptime;         /* How much time to leave between each print*/
        MYFLT   *val;           /* Value to print */
        MYFLT   *space;         /* Spaces to insert before printing */
        MYFLT   *named;
        MYFLT   printat, ctime; /* Time when initialised; initialised */
        int32   pspace;         /* How many spaces to print */
        int     initialised;    /* Non zero for initialised */
} PRINTK;

/* PRINTKS data structure for printks() and printksset()  */
typedef struct {
        OPDS    h;
        MYFLT   *ifilcod;       /* File name */
        MYFLT   *ptime;         /* How much time to leave between each print */
        MYFLT   *kvals[VARGMAX-2];/* values to print */
        MYFLT   printat, ctime; /* Time when initialised; Cycle time */
        int     initialised;
        char    txtstring[8192]; /* Place to store the string printed */
        char* old;
} PRINTKS;

/* an i-rate-only prints */
typedef struct {
        OPDS    h;
        MYFLT   *ifilcod;       /* File name */
        MYFLT   *kvals[VARGMAX-1];/* values to print */
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
        MYFLT   *val, *space, *named;
        MYFLT   oldvalue;
        int32_t pspace;
} PRINTK2;

typedef struct {
        OPDS    h;
        MYFLT   *iformat;
        MYFLT   *val;
        MYFLT   oldvalue;
        char    *sarg;
} PRINTK3;

typedef struct {
        OPDS    h;
        MYFLT   *ndx;
        MYFLT   *dummy, dummy1;
} IOZ;

int instimek(CSOUND*,RDTIME *p);
int instimes(CSOUND*,RDTIME *p);
int instimset(CSOUND*,RDTIME *p);

int eventcycles(CSOUND*,RDTIME *p);
int eventtime(CSOUND*,RDTIME *p);

int elapsedcycles(CSOUND*,RDTIME *p);
int elapsedtime(CSOUND*,RDTIME *p);


//int itablecopy(CSOUND*,TABLECOPY *p);
//int itablegpw(CSOUND*,TABLEGPW *p);
//int itablemix(CSOUND*,TABLEMIX *p);
//int itableng(CSOUND*,TABLENG *p);
//int itablew(CSOUND*,TABLEW *p);
//int ktablew(CSOUND*,TABLEW   *p);
//int ktablewkt(CSOUND*,TABLEW *p);
int peaka(CSOUND*,PEAK *p);
int peakk(CSOUND*,PEAK *p);
int printk(CSOUND*,PRINTK *p);
int printk2(CSOUND*,PRINTK2 *p);
int printk4(CSOUND*,PRINTK2 *p);
int printk2set(CSOUND*,PRINTK2 *p);
int printks(CSOUND*,PRINTKS *p);
int printkset(CSOUND*,PRINTK *p);
int printksset(CSOUND*,PRINTKS *p);
int printksset_S(CSOUND*,PRINTKS *p);
int printsset(CSOUND*,PRINTS *p);
int printsset_S(CSOUND*,PRINTS *p);
int printk3(CSOUND*,PRINTK3 *p);
int printk3set(CSOUND*,PRINTK3 *p);

//int tablecopy(CSOUND*,TABLECOPY *p);
//int tablecopyset(CSOUND*,TABLECOPY *p);
//int tablegpw(CSOUND*,TABLEGPW *p);
//int tablemix(CSOUND*,TABLEMIX *p);
//int tablemixset(CSOUND*,TABLEMIX *p);
//int tableng(CSOUND*,TABLENG *p);
//int tablera(CSOUND*,TABLERA *p);
//int tableraset(CSOUND*,TABLERA *p);
//int tablew(CSOUND*,TABLEW *p);
//int tablewa(CSOUND*,TABLEWA *p);
//int tablewaset(CSOUND*,TABLEWA *p);
//int tablewkt(CSOUND*,TABLEW *p);
//int tblsetw(CSOUND*,TABLEW *p);
//int tblsetwkt(CSOUND*,TABLEW *p);
int timek(CSOUND*,RDTIME *p);
int timesr(CSOUND*,RDTIME *p);
int inz(CSOUND*,IOZ *p);
int outz(CSOUND*,IOZ *p);

