/* Scanned Synthesis Opcodes:
   Copyright, 1999 Paris Smaragdis
   An extended system from an algorithm by Bill Verplank, Max Mathews and Rob Shaw

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

#pragma once

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#include "interlocks.h"
#endif


typedef struct SCANSYN_GLOBALS_ SCANSYN_GLOBALS;

/* Data structure for updating opcode */

typedef struct {
    OPDS        h;
    MYFLT       *i_init, *i_rate, *i_v, *i_m, *i_f, *i_c, *i_d;
    MYFLT       *k_m, *k_f, *k_c, *k_d, *i_l, *i_r, *k_x, *k_y;
    MYFLT       *a_ext, *i_disp, *i_id;
    AUXCH       aux_f;
    AUXCH       aux_x;
    MYFLT       *x0, *x1, *x2, *x3, *ext, *v;
    MYFLT       *m, *f, *c, *d, *out;
    int32       idx, len, exti, rate;
    int32_t     id;
    void        *win;
    FUNC        *fi;
    SCANSYN_GLOBALS *pp;
    int         revised;
} PSCSNU;

/* Data structure for scanning opcode */

typedef struct {
    OPDS        h;
    MYFLT       *a_out, *k_amp, *k_freq, *i_trj, *i_id;
    MYFLT       *interp;
    AUXCH       aux_t;
    MYFLT       fix, phs;
    int32       tlen, *t;
    int32_t     oscil_interp;
    PSCSNU      *p;
} PSCSNS;

typedef struct {
    OPDS        h;
    MYFLT       *k_pos, *k_vel;
    MYFLT       *i_id, *k_pamp, *k_vamp, *k_which;
    PSCSNU      *p;
} PSCSNMAP;

typedef struct {
    OPDS        h;
    ARRAYDAT    *k_pos, *k_vel;
    MYFLT       *i_id, *k_pamp, *k_vamp;
    PSCSNU      *p;
} PSCSNMAPV;

/* *********************************** */
/* *********************************** */
/* EXPERIMENTAL VERSION -- John ffitch */
/* *********************************** */
/* *********************************** */

/* Data structure for updating opcode */

typedef struct {
    OPDS        h;
    MYFLT       *i_init, *i_rate, *i_v, *i_m, *i_f, *i_c, *i_d;
    MYFLT       *k_m, *k_f, *k_c, *k_d, *i_l, *i_r, *k_x, *k_y;
    MYFLT       *a_ext, *i_disp, *i_id;
    AUXCH       aux_f;
    AUXCH       aux_x;
    MYFLT       *x0, *x1, *x2, *x3, *ext, *v, rate;
    MYFLT       *m, *c, *d, *out;
#ifdef USING_CHAR
    char        *f;
#else
    uint32      *f;
#endif
    int32       idx, exti;
    uint32_t    len;
    int32_t       id;
    void        *win;
    FUNC        *fi;
    SCANSYN_GLOBALS *pp;
} PSCSNUX;

/* Data structure for scanning opcode */

typedef struct {
    OPDS        h;
    MYFLT       *a_out;
    MYFLT       *k_amp, *k_freq, *i_trj, *i_id;
    MYFLT       *interp;
    AUXCH       aux_t;
    MYFLT       fix, phs;
    int32       tlen, *t;
    int32_t     oscil_interp;
    PSCSNUX     *p;
} PSCSNSX;

typedef struct {
    OPDS        h;
    MYFLT       *k_pos, *k_vel;
    MYFLT       *i_id, *k_pamp, *k_vamp, *k_which;
    PSCSNUX     *p;
} PSCSNMAPX;

struct SCANSYN_GLOBALS_ {
    CSOUND      *csound;
    /* scansyn.c */
    MYFLT       *ewin;
    void        *scsn_list;
    /* scansynx.c */
    MYFLT       *ewinx;
    void        *scsnx_list;
};

extern int32_t scansynx_init_(CSOUND *);

static CS_NOINLINE SCANSYN_GLOBALS * scansyn_allocGlobals(CSOUND *csound)
{
    SCANSYN_GLOBALS *p;

    if (csound->CreateGlobalVariable(csound, "scansynGlobals",
                                             sizeof(SCANSYN_GLOBALS)) != 0)
      csound->Die(csound, "scansyn: error allocating globals");
    p = (SCANSYN_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                        "scansynGlobals");
    p->csound = csound;

    return p;
}

static inline SCANSYN_GLOBALS * scansyn_getGlobals(CSOUND *csound)
{
    SCANSYN_GLOBALS *p;

    p = (SCANSYN_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                        "scansynGlobals");
    if (p == NULL)
      return scansyn_allocGlobals(csound);

    return p;
}

