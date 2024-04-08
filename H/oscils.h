/*
    oscils.h:

    Copyright (C) 2002 Istvan Varga

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

/* ------ oscils, lphasor, and tablexkt by Istvan Varga (Jan 5 2002) ------ */

#ifndef CSOUND_OSCILS_H
#define CSOUND_OSCILS_H

/* oscils opcode struct */

typedef struct {
    OPDS    h;
    MYFLT   *ar, *iamp, *icps, *iphs, *iflg;                /* opcode args  */
    /* internal variables */
    int     use_double;
    double  xd, cd, vd;
    MYFLT   x, c, v;
} OSCILS;

/* lphasor opcode struct */

typedef struct {
    OPDS    h;
    MYFLT   *ar, *xtrns, *ilps, *ilpe;                      /* opcode       */
    MYFLT   *imode, *istrt, *istor;                         /* args         */
    /* internal variables */
    int     loop_mode;
    double  phs, lps, lpe;
    int     dir;            /* playback direction (0: backward, 1: forward) */
} LPHASOR;

/* tablexkt opcode struct */

typedef struct {
    OPDS    h;
    MYFLT   *ar, *xndx, *kfn, *kwarp, *iwsize;              /* opcode       */
    MYFLT   *ixmode, *ixoff, *iwrap;                        /* args         */
    /* internal variables */
    int     raw_ndx, ndx_scl, wrap_ndx, wsize;
    MYFLT   win_fact;
/*  double  wsized2_d, pidwsize_d; */           /* for oscils_hann.c */
} TABLEXKT;

/* these functions are exported to entry*.c */

#ifndef CSOUND_OSCILS_C
extern int oscils_set (CSOUND *, void*);
extern int oscils (CSOUND *, void*);
extern int lphasor_set (CSOUND *, void*);
extern int lphasor (CSOUND *, void*);
extern int tablexkt_set (CSOUND *, void*);
extern int tablexkt (CSOUND *, void*);
#endif

#endif              /* CSOUND_OSCILS_H */

