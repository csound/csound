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

static long *typemask_tabl = NULL;
static long *typemask_tabl_in = NULL, *typemask_tabl_out = NULL;

/* list of available Csound signal types (currently, there are 13) */

#define ARGTYP_a        0x00000001L     /* a-rate */
#define ARGTYP_k        0x00000002L     /* k-rate */
#define ARGTYP_i        0x00000004L     /* i-rate */
#define ARGTYP_p        0x00000008L     /* p-field (i-rate) */
#define ARGTYP_c        0x00000010L     /* constant (i-rate) */
#define ARGTYP_r        0x00000020L     /* reserved symbol (i-rate) */
#define ARGTYP_S        0x00000040L     /* string constant */
#define ARGTYP_d        0x00000080L     /* spectral #1 (?) */
#define ARGTYP_w        0x00000100L     /* spectral #2 (?) */
#define ARGTYP_f        0x00000200L     /* spectral #3 (?) */
#define ARGTYP_B        0x00000400L     /* boolean #1 */
#define ARGTYP_b        0x00000800L     /* boolean #2 */
#define ARGTYP_l        0x00001000L     /* label */

/* some common type combinations to save typing */

#define ARGTYP_ipcrk    0x0000003EL     /* any k- or i-rate */
#define ARGTYP_ipck     0x0000001EL     /* any k- or i-rate except reserved */
#define ARGTYP_ipcr     0x0000003CL     /* any i-rate */
#define ARGTYP_ipc      0x0000001CL     /* any i-rate except reserved */
#define ARGTYP_aipcrk   0x0000003FL     /* any a-, k-, or i-rate */

/* basic types */

static long typetabl1[27] = {
    'a',    ARGTYP_a,       'k',    ARGTYP_k,       'i',    ARGTYP_i,
    'p',    ARGTYP_p,       'c',    ARGTYP_c,       'r',    ARGTYP_r,
    'S',    ARGTYP_S,       'd',    ARGTYP_d,       'w',    ARGTYP_w,
    'f',    ARGTYP_f,       'B',    ARGTYP_B,       'b',    ARGTYP_b,
    'l',    ARGTYP_l,       0L
};

/* the following input and output types are available additionally */
/* to the above defined ones */

/* input types */

static long typetabl2[31] = {
    'z',    ARGTYP_ipcrk,               'y',    ARGTYP_a,
    'S',    (ARGTYP_S | ARGTYP_ipcr),   'M',    ARGTYP_aipcrk,
    'B',    (ARGTYP_B | ARGTYP_b),      'k',    ARGTYP_ipcrk,
    'h',    ARGTYP_ipcr,    'i',    ARGTYP_ipcr,    'j',    ARGTYP_ipcr,
    'm',    ARGTYP_ipcr,    'n',    ARGTYP_ipcr,    'o',    ARGTYP_ipcr,
    'p',    ARGTYP_ipcr,    'q',    ARGTYP_ipcr,    'v',    ARGTYP_ipcr,
    0L
};

/* output types */

static long typetabl3[13] = {
    's',    (ARGTYP_a | ARGTYP_k),      'i',    (ARGTYP_i | ARGTYP_p),
    'B',    (ARGTYP_B | ARGTYP_b),      'm',    ARGTYP_a,
    'z',    ARGTYP_k,   'X',    (ARGTYP_a | ARGTYP_k | ARGTYP_i | ARGTYP_p),
    0L
};

#endif          /* CSOUND_TYPETABL_H */

