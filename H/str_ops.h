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
*/

#ifndef CSOUND_STR_OPS_C

int strset_init(void *, void *);
int strget_init(void *, void *);
int strcpy_opcode_init(void *, void *);
int strcpy_opcode_perf(void *, void *);
int strcat_opcode_init(void *, void *);
int strcat_opcode_perf(void *, void *);
int strcmp_opcode(void *, void *);
int sprintf_opcode_init(void *, void *);
int sprintf_opcode_perf(void *, void *);
int printf_opcode_init(void *, void *);
int printf_opcode_set(void *, void *);
int printf_opcode_perf(void *, void *);
int puts_opcode_init(void *, void *);
int puts_opcode_perf(void *, void *);
int strtod_opcode_init(void *, void *);
int strtod_opcode_perf(void *, void *);
int strtol_opcode_init(void *, void *);
int strtol_opcode_perf(void *, void *);

#endif      /* CSOUND_STR_OPS_C */

#ifdef __cplusplus
};
#endif

#endif      /* CSOUND_STR_OPS_H */

