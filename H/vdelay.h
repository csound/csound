/*
    vdelay.h:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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

/*      vdelay, multitap, reverb2 coded by Paris Smaragdis              */
/*      Berklee College of Music Csound development team                */
/*      Copyright (c) December 1994.  All rights reserved               */

typedef struct {
        OPDS    h;
        MYFLT   *sr, *ain, *adel, *imaxd, *istod;
        uint32 maxd;
        AUXCH   aux;
        int32   left;
} VDEL;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *sr2, *sr3, *sr4;
        MYFLT   *ain1, *ain2, *ain3, *ain4, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1, aux2, aux3, aux4;
        uint32 maxd;
        int     interp_size;
        int32   left;
} VDELXQ;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *sr2, *ain1, *ain2, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1, aux2;
        uint32 maxd;
        int     interp_size;
        int32   left;
} VDELXS;

typedef struct {
        OPDS    h;
        MYFLT   *sr1, *ain1, *adel, *imaxd, *iquality, *istod;
        AUXCH   aux1;
        uint32 maxd;
        int     interp_size;
        int32   left;
} VDELX;

typedef struct {
        OPDS    h;
        MYFLT   *sr, *ain, *ndel[VARGMAX-1];
        AUXCH   aux;
        int32   left, max;
} MDEL;

#if 0

#define Combs   6
#define Alpas   5

typedef struct {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *cbuf_cur[Combs], *abuf_cur[Alpas];
        MYFLT   c_time[Combs], c_gain[Combs], a_time[Alpas], a_gain[Alpas];
        MYFLT   z[Combs], g[Combs];
        AUXCH   temp;
        AUXCH   caux[Combs], aaux[Alpas];
        MYFLT   prev_time, prev_hdif;
} STVB;

/*      nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998 */

typedef struct {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *cbuf_cur[Combs], *abuf_cur[Alpas];
        MYFLT   c_time[Combs], c_gain[Combs], a_time[Alpas], a_gain[Alpas];
        MYFLT   z[Combs], g[Combs];
        AUXCH   temp;
        AUXCH   caux[Combs], aaux[Alpas];
        MYFLT   prev_time, prev_hdif;
} NREV;

#endif

/*
 * Based on nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998.
 * Changes made to allow user-defined comb and alpas constant in a ftable.
 * Sept 2000, by rasmus ekman.
 * Memory allocation fixed April 2001 by JPff
 */
typedef struct {
        OPDS    h;
        MYFLT   *out, *in, *time, *hdif, *istor;
        MYFLT   *inumCombs, *ifnCombs, *inumAlpas, *ifnAlpas;
        /* Used to be [Combs]- and [Alpas]-sized arrays */
        int     numCombs, numAlpas;
        MYFLT   **cbuf_cur, **abuf_cur;
        MYFLT   **pcbuf_cur, **pabuf_cur;
        MYFLT   *c_time, *c_gain, *a_time, *a_gain;
        const MYFLT *c_orggains, *a_orggains;
        MYFLT   *z, *g;        /* [Combs] */
        AUXCH   temp;
        AUXCH   caux, aaux;
        AUXCH   caux2, aaux2;  /* Used to hold space for all dynamized arrays */
        MYFLT   prev_time, prev_hdif;
} NREV2;

int vdelset(CSOUND *, VDEL *p);
int vdelay(CSOUND *, VDEL *p);
int vdelay3(CSOUND *, VDEL *p);
int vdelxset(CSOUND *, VDELX *p);
int vdelxsset(CSOUND *, VDELXS *p);
int vdelxqset(CSOUND *, VDELXQ *p);
int vdelayx(CSOUND *, VDELX *p);
int vdelayxw(CSOUND *, VDELX *p);
int vdelayxs(CSOUND *, VDELXS *p);
int vdelayxws(CSOUND *, VDELXS *p);
int vdelayxq(CSOUND *, VDELXQ *p);
int vdelayxwq(CSOUND *, VDELXQ *p);
int multitap_set(CSOUND *, MDEL *p);
int multitap_play(CSOUND *, MDEL *p);
#if 0
int nreverb_set(CSOUND *, NREV *p);
int nreverb(CSOUND *, NREV *p);
#endif
int reverbx_set(CSOUND *, NREV2 *p);
int reverbx(CSOUND *, NREV2 *p);

