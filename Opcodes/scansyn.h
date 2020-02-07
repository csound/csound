/* Scanned Synthesis Opcodes:
   scansyn.c, scansyn.csd, scansyn.h and related files
   are Copyright, 1999 by Interval Research.
   Coded by Paris Smaragdis
   From an algorithm by Bill Verplank, Max Mathews and Rob Shaw

   Permission to use, copy, or modify these programs and their documentation
   for educational and research purposes only and without fee is hereby
   granted, provided that this copyright and permission notice appear on all
   copies and supporting documentation. For any other uses of this software,
   in original or modified form, including but not limited to distribution in
   whole or in part, specific prior permission from Interval Research must be
   obtained. Interval Research makes no representations about the suitability
   of this software for any purpose. It is provided "as is" without express or
   implied warranty.
*/

#include "csdl.h"

typedef struct SCANSYN_GLOBALS_ SCANSYN_GLOBALS;

/* Data structure for updating opcode */

typedef struct {
    OPDS        h;
    MYFLT       *i_init, *i_rate, *i_v, *i_m, *i_f, *i_c, *i_d;
    MYFLT       *k_m, *k_f, *k_c, *k_d, *i_l, *i_r, *k_x, *k_y;
    MYFLT       *a_ext, *i_disp, *i_id;
    AUXCH       aux_f;
    AUXCH       aux_x;
    MYFLT       *x0, *x1, *x2, *x3, *ext, *v, rate;
    MYFLT       *m, *f, *c, *d, *out;
    int32        idx, len, exti;
    int32_t      id;
    void        *win;
    SCANSYN_GLOBALS *pp;
} PSCSNU;

/* Data structure for scanning opcode */

typedef struct {
    OPDS        h;
    MYFLT       *a_out, *k_amp, *k_freq, *i_trj, *i_id;
    MYFLT       *interp;
    AUXCH       aux_t;
    MYFLT       fix, phs;
    int32        tlen, *t;
    int32_t     oscil_interp;
    PSCSNU      *p;
} PSCSNS;

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
    int32_t         id;
    void        *win;
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
    int32_t         oscil_interp;
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

extern int32_t
scansynx_init_(CSOUND *);

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

