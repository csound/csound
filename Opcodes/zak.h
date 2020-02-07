/*
    zak.h:

    Copyright (C) 2018 John ffitch

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

                                                        /*      ZAK.H */
typedef struct {
    MYFLT         *zkstart;
    int64_t       zklast;
    MYFLT         *zastart;
    int64_t       zalast;
} ZAK_GLOBALS;


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
        MYFLT   *dummy;
        void    *zz;
} ZKR;

/* ZKW data structure for ziw() and zkw(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;           /* Value to write */
        MYFLT   *ndx;           /* Locations to read */
        MYFLT   *dummy;
        void    *zz;
} ZKW;

/* ZKWM data structure for ziwm() and zkwm(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;           /* Value to write */
        MYFLT   *ndx;           /* Locations to read */
        MYFLT   *mix;           /* 0 for write directly;  !0 for mix - add in */
        void    *zz;
} ZKWM;

/* ZKMOD data structure for zkmod(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Points to where to write output */
        MYFLT   *sig;           /* Value to modulate */
        MYFLT   *zkmod;         /* Which zk variable to use to modulate sig */
        void    *zz;
} ZKMOD;

/* ZKCL data structure for zkcl(). */
typedef struct {
        OPDS    h;
        MYFLT   *first;         /* First variable to clear */
        MYFLT   *last;          /* Final variable to clear */
        MYFLT   *dummy;
        void    *zz;
} ZKCL;

/* ZAR data structure for zar(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Where to write the value */
        MYFLT   *ndx;           /* Location in za space to read */
        MYFLT   *dummy;
        void    *zz;
} ZAR;

/* ZARG data structure for zarg(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;          /* Where to write the zk location */
        MYFLT   *ndx;           /* Location in za space to read */
        MYFLT   *kgain;         /* Gain to be given to signal read */
        void    *zz;
} ZARG;

/* ZAW data structure for zaw(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig, *ndx;
        MYFLT   *dummy;
        void    *zz;
} ZAW;

/* ZAWM data structure for zawm(). */
typedef struct {
        OPDS    h;
        MYFLT   *sig;
        MYFLT   *ndx, *mix;     /* Locations to read;
                                   0 for write directly, or addd in */
        void    *zz;
} ZAWM;

/* ZAWOD data structure for zamod(). */
typedef struct {
        OPDS    h;
        MYFLT   *rslt;
        MYFLT   *sig, *zamod;   /* Value to modulate; Which za variable to use */
        void    *zz;
} ZAMOD;

/* ZACL data structure for zacl(). */
typedef struct {
        OPDS    h;
        MYFLT   *first, *last;
        MYFLT   *dummy;
        void    *zz;
} ZACL;

int zacl(CSOUND*,ZACL *p);
int zakinit(CSOUND*,ZAKINIT *p);
int zamod(CSOUND*,ZAMOD *p);
int zar(CSOUND*,ZAR *p);
int zarg(CSOUND*,ZARG *p);
int zaset(CSOUND*,ZAR *p);
int zaw(CSOUND*,ZAW *p);
int zawm(CSOUND*,ZAWM *p);
int zir(CSOUND*,ZKR *p);
int ziw(CSOUND*,ZKW *p);
int ziwm(CSOUND*,ZKWM *p);
int zkcl(CSOUND*,ZKCL *p);
int zkmod(CSOUND*,ZKMOD *p);
int zkr(CSOUND*,ZKR *p);
int zkset(CSOUND*,ZKR *p);
int zkw(CSOUND*,ZKW *p);
int zkwm(CSOUND*,ZKWM *p);

