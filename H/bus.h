/*
    bus.h:

    Copyright (C) 2004 John ffitch
        (C) 2005, 2006 Istvan Varga
        (C) 2006 Victor Lazzarini

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

/*                                                      BUS.H           */

#ifndef CSOUND_BUS_H
#define CSOUND_BUS_H

#include "pstream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    OPDS    h;
    PVSDAT   *r;
    MYFLT    *a,*N, *overlap, *winsize, *wintype, *format;
    PVSDAT   init;
} FCHAN;

typedef struct {
    OPDS    h;
    MYFLT   *ans;
    MYFLT   *keyDown;
    int     evtbuf;
} KSENSE;

typedef struct {
    OPDS    h;
    MYFLT   *arg;
    MYFLT   *iname;
    MYFLT   *fp;
    int     *lock;
} CHNGET;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *fp;
    int     *lock;
} CHNCLEAR;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *imode;
    const char  *name;
    MYFLT   *fp;
    int     *lock;
    int     type;
} CHNSEND;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *imode;
    MYFLT   *itype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
    int     *lock;
} CHN_OPCODE_K;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *imode;
    int     *lock;
} CHN_OPCODE;

typedef struct {
    OPDS    h;
    MYFLT   *arg;
    MYFLT   *iname;
    MYFLT   *imode;
    MYFLT   *itype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
} CHNEXPORT_OPCODE;

typedef struct {
    OPDS    h;
    MYFLT   *itype;
    MYFLT   *imode;
    MYFLT   *ictltype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
    MYFLT   *iname;
} CHNPARAMS_OPCODE;

/*
 {  "chnget",      0xFFFF,              0,      NULL,           NULL,
    (SUBR) NULL, (SUBR) NULL, (SUBR) NULL                               },
 {  "chnget.i",    S(CHNGET),           1,      "i",            "S",
    (SUBR) chnget_opcode_init_i, (SUBR) NULL, (SUBR) NULL               },
 {  "chnget.k",    S(CHNGET),           3,      "k",            "S",
    (SUBR) chnget_opcode_init_k, (SUBR) notinit_opcode_stub, (SUBR) NULL },
 {  "chnget.a",    S(CHNGET),           5,      "a",            "S",
    (SUBR) chnget_opcode_init_a, (SUBR) notinit_opcode_stub, (SUBR) NULL },
 {  "chnget.S",    S(CHNGET),           1,      "S",            "S",
    (SUBR) chnget_opcode_init_S, (SUBR) NULL, (SUBR) NULL               },
 {  "chnset",      0xFFFB,              0,      NULL,           NULL,
    (SUBR) NULL, (SUBR) NULL, (SUBR) NULL                               },
 {  "chnset.i",    S(CHNGET),           1,      "",             "iS",
    (SUBR) chnset_opcode_init_i, (SUBR) NULL, (SUBR) NULL               },
 {  "chnset.r",    S(CHNGET),           1,      "",             "iS",
    (SUBR) chnset_opcode_init_i, (SUBR) NULL, (SUBR) NULL               },
 {  "chnset.c",    S(CHNGET),           1,      "",             "iS",
    (SUBR) chnset_opcode_init_i, (SUBR) NULL, (SUBR) NULL               },
 {  "chnset.k",    S(CHNGET),           3,      "",             "kS",
    (SUBR) chnset_opcode_init_k, (SUBR) notinit_opcode_stub, (SUBR) NULL },
 {  "chnset.a",    S(CHNGET),           5,      "",             "aS",
    (SUBR) chnset_opcode_init_a, (SUBR) notinit_opcode_stub, (SUBR) NULL },
 {  "chnset.S",    S(CHNGET),           1,      "",             "SS",
    (SUBR) chnset_opcode_init_S, (SUBR) NULL, (SUBR) NULL               },
 {  "chnmix",      S(CHNGET),           5,      "",             "aS",
    (SUBR) chnmix_opcode_init, (SUBR) NULL, (SUBR) notinit_opcode_stub  },
 {  "chnclear",    S(CHNCLEAR),         5,      "",             "S",
    (SUBR) chnclear_opcode_init, (SUBR) NULL, (SUBR) notinit_opcode_stub },
 {  "chn_k",       S(CHN_OPCODE_K),     1,      "",             "Sioooo",
    (SUBR) chn_k_opcode_init, (SUBR) NULL, (SUBR) NULL                  },
 {  "chn_a",       S(CHN_OPCODE),       1,      "",             "Si",
    (SUBR) chn_a_opcode_init, (SUBR) NULL, (SUBR) NULL                  },
 {  "chn_S",       S(CHN_OPCODE),       1,      "",             "Si",
    (SUBR) chn_S_opcode_init, (SUBR) NULL, (SUBR) NULL                  },
 {  "chnexport",   0xFFFF,              0,      NULL,           NULL,
    (SUBR) NULL, (SUBR) NULL, (SUBR) NULL                               },
 {  "chnexport.i", S(CHNEXPORT_OPCODE), 1,      "i",            "Sioooo",
    (SUBR) chnexport_opcode_init, (SUBR) NULL, (SUBR) NULL              },
 {  "chnexport.k", S(CHNEXPORT_OPCODE), 1,      "k",            "Sioooo",
    (SUBR) chnexport_opcode_init, (SUBR) NULL, (SUBR) NULL              },
 {  "chnexport.a", S(CHNEXPORT_OPCODE), 1,      "a",            "Si",
    (SUBR) chnexport_opcode_init, (SUBR) NULL, (SUBR) NULL              },
 {  "chnexport.S", S(CHNEXPORT_OPCODE), 1,      "S",            "Si",
    (SUBR) chnexport_opcode_init, (SUBR) NULL, (SUBR) NULL              },
 {  "chnparams",   S(CHNPARAMS_OPCODE), 1,      "iiiiii",       "S",
    (SUBR) chnparams_opcode_init, (SUBR) NULL, (SUBR) NULL              },
 {  "chnrecv",     S(CHNSEND),          3,      "",             "So",
    (SUBR) chnrecv_opcode_init, (SUBR) notinit_opcode_stub, (SUBR) NULL },
 {  "chnsend",     S(CHNSEND),          3,      "",             "So",
    (SUBR) chnsend_opcode_init, (SUBR) notinit_opcode_stub, (SUBR) NULL },
*/

#ifndef CSOUND_BUS_C

int     chano_opcode_perf_k(CSOUND *, void *);
int     chano_opcode_perf_a(CSOUND *, void *);
int     chani_opcode_perf_k(CSOUND *, void *);
int     chani_opcode_perf_a(CSOUND *, void *);
int     pvsin_init(CSOUND *, void *);
int     pvsin_perf(CSOUND *, void *);
int     pvsout_perf(CSOUND *, void *);

int     sensekey_perf(CSOUND *, void *);

int     notinit_opcode_stub(CSOUND *, void *);
int     chnget_opcode_init_i(CSOUND *, void *);
int     chnget_opcode_init_k(CSOUND *, void *);
int     chnget_opcode_init_a(CSOUND *, void *);
int     chnget_opcode_init_S(CSOUND *, void *);
int     chnset_opcode_init_i(CSOUND *, void *);
int     chnset_opcode_init_k(CSOUND *, void *);
int     chnset_opcode_init_a(CSOUND *, void *);
int     chnset_opcode_init_S(CSOUND *, void *);
int     chnmix_opcode_init(CSOUND *, void *);
int     chnclear_opcode_init(CSOUND *, void *);
int     chn_k_opcode_init(CSOUND *, void *);
int     chn_a_opcode_init(CSOUND *, void *);
int     chn_S_opcode_init(CSOUND *, void *);
int     chnexport_opcode_init(CSOUND *, void *);
int     chnparams_opcode_init(CSOUND *, void *);
int     chnrecv_opcode_init(CSOUND *, void *);
int     chnsend_opcode_init(CSOUND *, void *);

#endif      /* CSOUND_BUS_C */

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_BUS_H */

