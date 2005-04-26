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

#include "sysdep.h"

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
*/

#ifndef CSOUND_STR_OPS_C

int strset_init(void *, void *);
int strget_init(void *, void *);
int strcpy_opcode_init(void *, void *);
int strcpy_opcode_perf(void *, void *);
int strcat_opcode_init(void *, void *);
int strcat_opcode_perf(void *, void *);
int strcmp_opcode(void *, void *);

#endif      /* CSOUND_STR_OPS_C */

#ifdef __cplusplus
};
#endif

#endif      /* CSOUND_STR_OPS_H */

