/*
    str_ops.h:

    Copyright (C) 2005 Istvan Varga, Matt J. Ingalls, John ffitch

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

#ifndef CSOUND_STR_OPS_H
#define CSOUND_STR_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    OPDS    h;
    MYFLT   *indx;
    MYFLT   *str;
} STRSET_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    MYFLT   *indx;
} STRGET_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    MYFLT   *str;
} STRCPY_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    MYFLT   *str1;
    MYFLT   *str2;
} STRCAT_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    MYFLT   *sfmt;
    MYFLT   *args[64];
} SPRINTF_OP;

typedef struct {
    OPDS    h;
    MYFLT   *sfmt;
    MYFLT   *ktrig;
    MYFLT   *args[64];
    MYFLT   prv_ktrig;
} PRINTF_OP;

typedef struct {
    OPDS    h;
    MYFLT   *str;
    MYFLT   *ktrig;
    MYFLT   *no_newline;
    MYFLT   prv_ktrig;
    int     noNewLine;
} PUTS_OP;

typedef struct {
    OPDS    h;
    MYFLT   *arg;
    MYFLT   *iname;
    MYFLT   *fp;
} CHNGET;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *imode;
    MYFLT   *itype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
} CHN_OPCODE_K;

typedef struct {
    OPDS    h;
    MYFLT   *iname;
    MYFLT   *imode;
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
 {  "strset",   S(STRSET_OP),   1,  "",     "iS",
    (SUBR) strset_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strget",   S(STRGET_OP),   1,  "S",    "i",
    (SUBR) strget_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strcpy",   S(STRCPY_OP),   1,  "S",    "S",
    (SUBR) strcpy_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcpyk",  S(STRCPY_OP),   3,  "S",    "S",
    (SUBR) strcpy_opcode_init, (SUBR) strcpy_opcode_perf, (SUBR) NULL   },
 {  "strcat",   S(STRCAT_OP),   1,  "S",    "SS",
    (SUBR) strcat_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcatk",  S(STRCAT_OP),   3,  "S",    "SS",
    (SUBR) strcat_opcode_init, (SUBR) strcat_opcode_perf, (SUBR) NULL   },
 {  "strcmp",   S(STRCAT_OP),   1,  "i",    "SS",
    (SUBR) strcpy_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcmpk",  S(STRCAT_OP),   3,  "k",    "SS",
    (SUBR) strcmp_opcode, (SUBR) strcmp_opcode, (SUBR) NULL             },
 {  "sprintf",  S(SPRINTF_OP),  1,  "S",    "SN",
    (SUBR) sprintf_opcode_init, (SUBR) NULL, (SUBR) NULL                },
 {  "sprintfk", S(SPRINTF_OP),  3,  "S",    "SN",
    (SUBR) sprintf_opcode_init, (SUBR) sprintf_opcode_perf, (SUBR) NULL },
 {  "printf_i", S(PRINTF_OP),   1,  "",     "SiN",
    (SUBR) printf_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "printf",   S(PRINTF_OP),   3,  "",     "SkN",
    (SUBR) printf_opcode_set, (SUBR) printf_opcode_perf, (SUBR) NULL    },
 {  "puts",     S(PUTS_OP),     3,  "",     "Sko",
    (SUBR) puts_opcode_init, (SUBR) puts_opcode_perf, (SUBR) NULL       },
 {  "strtod",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtod_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strtodk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtod_opcode_init, (SUBR) strtod_opcode_perf, (SUBR) NULL   },
 {  "strtol",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtol_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strtolk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtol_opcode_init, (SUBR) strtol_opcode_perf, (SUBR) NULL   },
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
*/

#ifndef CSOUND_STR_OPS_C

int strset_init(CSOUND *, void *);
int strget_init(CSOUND *, void *);
int strcpy_opcode_init(CSOUND *, void *);
int strcpy_opcode_perf(CSOUND *, void *);
int strcat_opcode_init(CSOUND *, void *);
int strcat_opcode_perf(CSOUND *, void *);
int strcmp_opcode(CSOUND *, void *);
int sprintf_opcode_init(CSOUND *, void *);
int sprintf_opcode_perf(CSOUND *, void *);
int printf_opcode_init(CSOUND *, void *);
int printf_opcode_set(CSOUND *, void *);
int printf_opcode_perf(CSOUND *, void *);
int puts_opcode_init(CSOUND *, void *);
int puts_opcode_perf(CSOUND *, void *);
int strtod_opcode_init(CSOUND *, void *);
int strtod_opcode_perf(CSOUND *, void *);
int strtol_opcode_init(CSOUND *, void *);
int strtol_opcode_perf(CSOUND *, void *);
int notinit_opcode_stub(CSOUND *, void *);
int chnget_opcode_init_i(CSOUND *, void *);
int chnget_opcode_init_k(CSOUND *, void *);
int chnget_opcode_init_a(CSOUND *, void *);
int chnget_opcode_init_S(CSOUND *, void *);
int chnset_opcode_init_i(CSOUND *, void *);
int chnset_opcode_init_k(CSOUND *, void *);
int chnset_opcode_init_a(CSOUND *, void *);
int chnset_opcode_init_S(CSOUND *, void *);
int chn_k_opcode_init(CSOUND *, void *);
int chn_a_opcode_init(CSOUND *, void *);
int chn_S_opcode_init(CSOUND *, void *);
int chnexport_opcode_init(CSOUND *, void *);
int chnparams_opcode_init(CSOUND *, void *);

#endif      /* CSOUND_STR_OPS_C */

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_STR_OPS_H */

