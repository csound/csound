/*
    aops.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Gabriel Maldonado

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

/*                                                      AOPS.H          */

#define CSOUND_SPIN_SPINLOCK csoundSpinLock(&csound->spinlock);
#define CSOUND_SPIN_SPINUNLOCK csoundSpinUnLock(&csound->spinlock);
#define CSOUND_SPOUT_SPINLOCK csoundSpinLock(&csound->spoutlock);
#define CSOUND_SPOUT_SPINUNLOCK csoundSpinUnLock(&csound->spoutlock);

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} ASSIGN;

#define ASSIGNM_MAX (24)
typedef struct {
    OPDS    h;
    MYFLT   *r[ASSIGNM_MAX], *a[ASSIGNM_MAX];
} ASSIGNM;

typedef struct {
    OPDS    h;
    TABDAT  *a;
    MYFLT   *size, *value;
} INITT;

typedef struct {
    OPDS    h;
    TABDAT  *tab;
    MYFLT   *ind;
    MYFLT   *val;
} ASSIGNT;

typedef struct {
    OPDS    h;
    MYFLT   *ans;
    TABDAT  *tab;
    MYFLT   *ind;
} TABREF;


typedef struct {
    OPDS    h;
    int32_t     *rbool;
    MYFLT   *a, *b;
} RELAT;

typedef struct {
    OPDS    h;
    int32_t     *rbool, *ibool, *jbool;
} LOGCL;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    int32_t     *cond;
    MYFLT   *a, *b;
} CONVAL;

typedef struct {
    OPDS    h;
    MYFLT   *r, *a, *b;
} AOP;

typedef struct {
    OPDS    h;
    MYFLT   *r, *a, *b, *def;
} DIVZ;

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} EVAL;

typedef struct {
    OPDS    h;
    MYFLT   *ar;
} INM;

typedef struct {
    OPDS    h;
    ARRAYDAT   *tabout;
} INA;

typedef struct {
    OPDS    h;
    MYFLT   *ar1, *ar2;
} INS;

typedef struct {
    OPDS    h;
    MYFLT   *ar1, *ar2, *ar3, *ar4;
} INQ;

typedef struct {
    OPDS    h;
    MYFLT   *ar1, *ar2, *ar3, *ar4, *ar5, *ar6;
} INH;

typedef struct {
    OPDS    h;
    MYFLT   *ar1, *ar2, *ar3, *ar4, *ar5, *ar6, *ar7, *ar8;
} INO;

typedef struct {
    OPDS    h;
    MYFLT   *ar[40];    /* array size should be consistent with entry2.c */
} INALL;

typedef struct {
    OPDS    h;
    MYFLT   *ar[40];
    MYFLT   *ch[VARGMAX];
    int32_t     init;
} INCH;

typedef struct {
    OPDS    h;
    MYFLT   *ar;
    MYFLT   *ch;
    int32_t     init;
} INCH1;

typedef struct {
    OPDS    h;
    MYFLT   *asig[VARGMAX];
} OUTX;

typedef struct {
    OPDS       h;
    ARRAYDAT   *tabin;
    int32_t    nowarn;
} OUTARRAY;

typedef struct {
    OPDS    h;
    MYFLT   *asig;
} OUTM;

typedef struct {
    OPDS    h;
    MYFLT   *args[VARGMAX];
} OUTCH;

typedef struct {
    OPDS    h;
    MYFLT   *r, *pc, *et, *cy, *ref;
} XENH;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ktrig, *kinput, *tablenum;
    MYFLT   old_r;
} CPSTUN;

typedef struct {
    OPDS    h;
    MYFLT   *r, *input, *tablenum;
} CPSTUNI;

typedef struct {
    OPDS    h;
    MYFLT   *res, *arg;
} ERRFN;

typedef struct MONITOR_OPCODE_ {
    OPDS    h;
    MYFLT   *ar[24];
} MONITOR_OPCODE;

typedef struct {
        OPDS    h;
        MYFLT   *kstartChan, *argums[VARGMAX];
        int32_t narg;
} OUTRANGE;

int32_t monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p);
int32_t monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p);
int32_t outRange_i(CSOUND *csound, OUTRANGE *p);
int32_t outRange(CSOUND *csound, OUTRANGE *p);
int32_t hw_channels(CSOUND *csound, ASSIGN *p);
