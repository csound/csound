/* *********************************** */
/* *********************************** */
/* EXPERIMENTAL VERSION -- John ffitch */
/* *********************************** */
/* *********************************** */

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
/* Data structure for updating opcode */
typedef struct{
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
    unsigned long *f;
#endif
    long        idx, len, exti;
    int         id;
    void        *win;
} PSCSNUX;

/* Data structure for scanning opcode */
typedef struct{
    OPDS        h;
    MYFLT       *a_out;
    MYFLT       *k_amp, *k_freq, *i_trj, *i_id;
    MYFLT       *interp;
    AUXCH       aux_t;
    MYFLT       fix, phs;
    long        tlen, *t;
    int         oscil_interp;
    PSCSNUX     *p;
} PSCSNSX;

typedef struct{
    OPDS        h;
    MYFLT       *k_pos, *k_vel;
    MYFLT       *i_id, *k_pamp, *k_vamp, *k_which;
    PSCSNUX     *p;
} PSCSNMAPX;

