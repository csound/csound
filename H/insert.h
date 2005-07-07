/*
    insert.h:

    Copyright (C) 1991, 2002 Barry Vercoe, Istvan Varga

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

typedef struct {                        /*       INSERT.H                */
        OPDS    h;
        LBLBLK  *lblblk;
} GOTO;

typedef struct {
        OPDS    h;
        int     *cond;
        LBLBLK  *lblblk;
} CGOTO;

typedef struct {
        OPDS    h;
        MYFLT   *idel, *idur;
        LBLBLK  *lblblk;
        long    cnt1, cnt2;
} TIMOUT;

typedef struct {
        OPDS    h;
} LINK;

/* the number of optional outputs defined in entry.c */
#define SUBINSTNUMOUTS  8       /* IV - Sep 8 2002: for subinstruments ... */
/* #define OPCODENUMOUTS   24      IV - Oct 24 2002: moved this to cs.h */

typedef struct {
        OPCODINFO *opcode_info;
        void    *uopcode_struct;
        INSDS   *parent_ip;
        MYFLT   *iobufp_ptrs[4];        /* expandable IV - Oct 26 2002 */
} OPCOD_IOBUFS;

typedef struct {                        /* IV - Oct 16 2002 */
        OPDS    h;
        MYFLT   *ar[VARGMAX];
        INSDS   *ip, *parent_ip;
        AUXCH   saved_spout;
        OPCOD_IOBUFS    buf;
} SUBINST;

typedef struct {                /* IV - Sep 8 2002: new structure: UOPCODE */
        OPDS    h;
        MYFLT   *ar[(OPCODENUMOUTS << 1) + 1];
        INSDS   *ip, *parent_ip;
        OPCOD_IOBUFS  *buf;
        int     l_ksmps, ksmps_scale;
        MYFLT   l_ekr, l_onedkr, l_onedksmps, l_kicvt;
} UOPCODE;

/* IV - Sep 8 2002: added opcodes: xin, xout, and setksmps */

typedef struct {
        OPDS        h;
        MYFLT       *args[OPCODENUMOUTS];
} XIN;

typedef struct {
        OPDS        h;
        MYFLT       *args[OPCODENUMOUTS];           /* IV - Oct 24 2002 */
} XOUT;

typedef struct {
        OPDS        h;
        MYFLT       *i_ksmps;
} SETKSMPS;

typedef struct {                /* IV - Oct 20 2002 */
        OPDS        h;
        MYFLT       *i_insno, *iname;
} NSTRNUM;

typedef struct {
        OPDS        h;
        MYFLT       *kInsNo, *kFlags, *kRelease;
} TURNOFF2;

