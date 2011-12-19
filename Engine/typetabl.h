/*
    typetabl.h:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_TYPETABL_H
#define CSOUND_TYPETABL_H

/* macros and tables for checking variable types (included from rdorch.c) */
/* written by Istvan Varga, Nov 2002 */

/* list of available Csound signal types (currently, there are 12) */

#define ARGTYP_a        0x00000001L     /* a-rate */
#define ARGTYP_k        0x00000002L     /* k-rate */
#define ARGTYP_i        0x00000004L     /* i-rate */
#define ARGTYP_p        0x00000008L     /* p-field (i-rate) */
#define ARGTYP_c        0x00000010L     /* constant (i-rate) */
#define ARGTYP_r        0x00000020L     /* reserved symbol (i-rate) */
#define ARGTYP_S        0x00000040L     /* string constant or variable */
#define ARGTYP_w        0x00000080L     /* spectral (?) */
#define ARGTYP_f        0x00000100L     /* streaming PVOC */
#define ARGTYP_B        0x00000200L     /* boolean (k-rate) */
#define ARGTYP_b        0x00000400L     /* boolean (i-rate) */
#define ARGTYP_l        0x00000800L     /* label */
#define ARGTYP_t        0x00001000L     /* table */

/* some common type combinations to save typing */

#define ARGTYP_ipcrk    0x0000003EL     /* any k- or i-rate */
#define ARGTYP_ipck     0x0000001EL     /* any k- or i-rate except reserved */
#define ARGTYP_ipcr     0x0000003CL     /* any i-rate */
#define ARGTYP_ipc      0x0000001CL     /* any i-rate except reserved */
#define ARGTYP_aipcrk   0x0000003FL     /* any a-, k-, or i-rate */

/* basic types */

static const int32 typetabl1[/*25*/] = {
    'a',    ARGTYP_a,       'k',    ARGTYP_k,       'i',    ARGTYP_i,
    'p',    ARGTYP_p,       'c',    ARGTYP_c,       'r',    ARGTYP_r,
    'S',    ARGTYP_S,       'w',    ARGTYP_w,       'f',    ARGTYP_f,
    'B',    ARGTYP_B,       'b',    ARGTYP_b,       'l',    ARGTYP_l,
    't',    ARGTYP_t,
    0L
};

/* the following input and output types are available additionally */
/* to the above defined ones */

/* input types */

static const int32 typetabl2[/*42*/] = {
    'z',    ARGTYP_ipcrk,               'y',    ARGTYP_a,
    'T',    (ARGTYP_S | ARGTYP_ipcr),   'U',    (ARGTYP_S | ARGTYP_ipcrk),
    'M',    ARGTYP_aipcrk,              'N',    (ARGTYP_S | ARGTYP_aipcrk),
    'B',    (ARGTYP_B | ARGTYP_b),      'k',    ARGTYP_ipcrk,
    'h',    ARGTYP_ipcr,    'i',    ARGTYP_ipcr,    'j',    ARGTYP_ipcr,
    'm',    ARGTYP_ipcr,    'n',    ARGTYP_ipcr,    'o',    ARGTYP_ipcr,
    'p',    ARGTYP_ipcr,    'q',    ARGTYP_ipcr,    'v',    ARGTYP_ipcr,
    'O',    ARGTYP_ipcrk,   'V',    ARGTYP_ipcrk,   'P',    ARGTYP_ipcrk,
    'J',    ARGTYP_ipcrk,
    0L
};

/* output types */

static const int32 typetabl3[/*17*/] = {
    's',    (ARGTYP_a | ARGTYP_k),
    'i',    (ARGTYP_i | ARGTYP_p),
    'B',    (ARGTYP_B | ARGTYP_b),
    'm',    ARGTYP_a,
    'z',    ARGTYP_k,
    'X',    (ARGTYP_a | ARGTYP_k | ARGTYP_i | ARGTYP_p),
    'N',    (ARGTYP_S | ARGTYP_a | ARGTYP_k | ARGTYP_i | ARGTYP_p),
    'I',    (ARGTYP_S | ARGTYP_i | ARGTYP_p),
    'F',    ARGTYP_f,
    't',    ARGTYP_t,
    0L
};

#endif          /* CSOUND_TYPETABL_H */

