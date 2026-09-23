/*
    ugens5.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once

#include "lpc.h"        /*                               UGENS5.H        */

/* The tone pole is b-sqrt(b*b-1), where b=2-cos(omega).
   With s=abs(sin(omega/2)), compute its complement without cancellation.
   A macro keeps audio-rate cutoff changes free of extra function calls. */
#define TONE_COEFFICIENTS(omega, c1, c2) do {                           \
    double tone_s = fabs(sin(0.5 * (omega)));                          \
    double tone_c1 = 2.0 * tone_s / (sqrt(1.0 + tone_s*tone_s) + tone_s); \
    (c1) = tone_c1;                                                   \
    (c2) = 1.0 - tone_c1;                                             \
} while (0)

typedef struct {
        OPDS    h;
        cs_float   *kr, *ksig, *ihtim, *isig;
        cs_double   c1, c2, yt1;
        cs_float  ihtim_old;
} PORT;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *khp, *istor;
        cs_double  c1, c2, yt1, prvhp;
} TONE;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *kcf, *kbw, *iscl, *istor;
        int32_t     scale;
        cs_double  c1, c2, c3, yt1, yt2, cosf, prvcf, prvbw;
        int32_t     asigf, asigw;
} RESON;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *khp, *ord, *istor;
        cs_double  c1, c2, *yt1, prvhp;
        int32_t loop;
        AUXCH   aux;
} TONEX;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *kcf, *kbw, *ord, *iscl, *istor;
        int32_t     scale, loop;
        cs_double  c1, c2, c3, *yt1, *yt2, cosf, prvcf, prvbw;
        AUXCH   aux;
} RESONX;

typedef struct {
        OPDS    h;
        cs_float   *krmr, *krmo, *kerr, *kcps, *ktimpt, *ifilcod, *inpoles, *ifrmrate;
        int32   headlen, npoles, nvals, lastframe, lastmsg;
        cs_float   *kcoefs, framrate;
        int32_t     storePoles ;
        MEMFIL  *mfp;
        AUXCH   aux;
        cs_float   *data;
} LPREAD;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig;
        cs_float   *circbuf, *circjp, *jp2lim, *coefs;
        int32_t npoles;
        LPREAD  *lpread;
        AUXCH   aux;

} LPRESON;

typedef struct {
        OPDS    h;
        cs_float   *kcf,*kbw, *kfor;
        LPREAD  *lpread;
        AUXCH   aux;
} LPFORM;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *kfrqratio;
        cs_float   *past, *coefs, prvratio, d, prvout;
        int32_t npoles;
        LPREAD  *lpread;
        AUXCH   aux;
} LPFRESON;

typedef struct {
        OPDS    h;
        cs_float   *kr, *asig, *ihp, *istor;
        cs_double   c1, c2, prvq;
} RMS;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *krms, *ihp, *istor;
        cs_double  c1, c2, prvq, prva;
} GAIN;

typedef struct {
        OPDS    h;
        cs_float   *ar, *asig, *csig, *ihp, *istor;
        cs_double  c1, c2, prvq, prvr, prva;
} BALANCE;

typedef struct {
        OPDS    h;
        cs_float   *islotnum ; /* Assume sizeof(int32_t)== sizeof(cs_float) */
} LPSLOT ;

typedef struct {
        OPDS    h;
        cs_float   *islot1 ;
        cs_float   *islot2 ; /* Assume sizeof(pointer)== sizeof(cs_float) */
        cs_float   *kmix  ;
 cs_float   *fpad[5]; /* Pad for kcoef correctly put (Mighty dangerous) */
        int32    lpad,npoles ;
        LPREAD  *lp1,*lp2 ;
        int32    lastmsg;
        cs_float   *kcoefs/*[MAXPOLES*2]*/, framrat16;
        int32_t             storePoles ;
        AUXCH    aux, slotaux;
} LPINTERPOL ;

typedef struct {
        OPDS    h;
        cs_float   *ans, *sig, *min, *max;
} LIMIT;

int32_t kport(CSOUND*,PORT *p);
int32_t ktone(CSOUND*,TONE *p);
int32_t katone(CSOUND*,TONE *p);
int32_t kreson(CSOUND*,RESON *p);
int32_t kareson(CSOUND*,RESON *p);
int32_t klimit(CSOUND*,LIMIT *p);
int32_t limit(CSOUND*,LIMIT *p);
