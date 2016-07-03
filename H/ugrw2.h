/*
    ugrw2.h:

    Copyright (C) 1995 Robin Whittle

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

/*                                                              UGRW2.H */
/* These files are based on Robin Whittle's
 *       ugrw2.c and ugrw2.h of 9 September 1995
 *
 * In February 1997, John Fitch reformatted the comments.
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

/*
 *      Unit generators by Robin Whittle                9 September 1995
 *
 *      Header file containing data structures for UGRW2.C.
 */
#ifdef SOME_FINE_DAY
#include "csoundCore.h"

/* KPORT data structure.
 *
 * Output and input pointers.
 *
 * kr           To k rate result.
 *
 * ksig         k rate input.
 *
 * khtim        k rate half time
 *
 * isig         Initial value for
 *              internal state.
 */
typedef struct {
        OPDS    h;
        MYFLT   *kr, *ksig, *khtim, *isig;
        MYFLT   c1;             /* Value to multiply with input value. */
        MYFLT   c2;             /* Value to multiply with previous state. */
        MYFLT   yt1;            /* Previous state. */
        MYFLT   prvhtim;        /* Previous khtim. */
} KPORT;

/*************************************/

/* KTONE data structure.
 *
 * For ktone and katone.
 *
 * Output and input pointers.
 *
 * kr           To k rate result.
 *
 * ksig         k rate input.
 *
 * khp          k rate half power frequency input.
 *
 * isig         Set to 0 to clear internal state.
 */

typedef struct {
        OPDS    h;
        MYFLT   *kr, *ksig, *khp, *istor;
        MYFLT   c1;             /* Value to multiply with input value  */
        MYFLT   c2;             /* Value to multiply with previous state */
        MYFLT   yt1;            /* Previous state */
        MYFLT   prvhp;          /* Previous half power frequency */
} KTONE;

/*************************************/

/* KRESON data structure.
 *
 * For kreson and kareson.
 *
 * Output and input pointers.
 *
 * kr           To k rate result.
 *
 * ksig         k rate input.
 *
 * kcf          k rate centre freq.
 *
 * kbw          k rate bandwidth
 *
 * iscl         i rate scaling factor * 0, 1 or 2.
 *
 * istor        Set to 0 to clear internal state.  */
typedef struct {
        OPDS    h;
        MYFLT   *kr, *ksig, *kcf, *kbw, *iscl, *istor;
        int     scale;
        MYFLT   c1, c2, c3;     /* Filter factors */
        MYFLT   yt1;            /* Delay 1 k sample */
        MYFLT   yt2;            /* Delay 2 k samples */
        MYFLT   cosf;           /* Intermediate variable to help calculations */
        MYFLT   prvcf, prvbw;   /* Previous centre freq and bandwidth  */
} KRESON;

/*************************************/

/* LIMIT data structure.
 *
 * For ilimit(), klimit() and limit().
 */
typedef struct {
        OPDS    h;
        MYFLT   *xdest, *xsig, *xlow, *xhigh;
} LIMIT;

int kporset(CSOUND*,KPORT *p);
int kport(CSOUND*,KPORT *p);
int ktonset(CSOUND*,KTONE *p);
int ktone(CSOUND*,KTONE *p);
int katone(CSOUND*,KTONE *p);
int krsnset(CSOUND*,KRESON *p);
int kreson(CSOUND*,KRESON *p);
int kareson(CSOUND*,KRESON *p);
int klimit(CSOUND*,LIMIT *p);
int limit(CSOUND*,LIMIT *p);

#endif /* SOME_FINE_DAY */
