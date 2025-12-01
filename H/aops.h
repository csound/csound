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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
*/

/*                                                      AOPS.H          */

#pragma once

#include "csoundCore.h"

#define CSOUND_SPIN_SPINLOCK csoundSpinLock(&csound->spinlock);
#define CSOUND_SPIN_SPINUNLOCK csoundSpinUnLock(&csound->spinlock);
#define CSOUND_SPOUT_SPINLOCK csoundSpinLock(&csound->spoutlock);
#define CSOUND_SPOUT_SPINUNLOCK csoundSpinUnLock(&csound->spoutlock);

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} ASSIGN;

typedef struct {
  OPDS    h;
  MYFLT   *r, *a;
  MYFLT   mem;
} STOREI;



#define ASSIGNM_MAX (24)
typedef struct {
    OPDS    h;
    MYFLT   *r[ASSIGNM_MAX], *a[ASSIGNM_MAX];
} ASSIGNM;


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
    int32_t     *rbool;
    MYFLT       *a, *b;
} LOGCL_KK;

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

typedef struct {
        OPDS    h;
        MYFLT   *kstartChan, *argums[VARGMAX];
        int32_t numChans, narg;
} INRANGE;

typedef struct {
        OPDS    h;
        MYFLT   *ians;
        MYFLT   *index;
} PFIELD;

typedef struct {
        OPDS    h;
        STRINGDAT   *ians;
        MYFLT   *index;
} PFIELDSTR;

typedef struct {
        OPDS    h;
        MYFLT   *inits[24];
        MYFLT   *start;
        MYFLT   *end;
} PINIT;

typedef struct {
        OPDS    h;
        ARRAYDAT *inits;
        MYFLT   *start;
        MYFLT   *end;
} PAINIT;

typedef struct iref_init {
  OPDS  h;
  INSTREF *out;
  MYFLT  *in;
} IREF_INIT;

typedef struct iref_num {
  OPDS  h;
  MYFLT  *out;
  INSTREF *in;
  MYFLT  *offs;
} IREF_NUM;

typedef struct {
  OPDS h;
  ARRAYDAT *tabin;
  uint32_t    len;
} MONITOR_A;

int32_t init_instr_ref(CSOUND *csound, IREF_INIT *p);
int32_t get_instr_num(CSOUND *csound, IREF_NUM *p);
int32_t get_instr_name(CSOUND *csound, IREF_NUM *p);

int32_t monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p);
int32_t monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p);
int32_t outRange_i(CSOUND *csound, OUTRANGE *p);
int32_t outRange(CSOUND *csound, OUTRANGE *p);
int32_t hw_channels(CSOUND *csound, ASSIGN *p);
void csound_aops_init_tables(CSOUND *);
MYFLT MOD(MYFLT, MYFLT);
int32_t inarray_set(CSOUND *csound, INA *p);
int32_t monitora_perf(CSOUND *csound, MONITOR_A *p);
int32_t monitora_init(CSOUND *csound, MONITOR_A *p);
int32_t bassign(CSOUND *csound, RELAT *p);
int32_t and_kk_bool(CSOUND *csound, LOGCL_KK *p);
int32_t or_kk_bool(CSOUND *csound, LOGCL_KK *p);
int32_t b2s(CSOUND *csound, ASSIGN *p);
int32_t b2b(CSOUND *csound, ASSIGN *p);
int32_t binit(CSOUND *csound, ASSIGNM *p);
int32_t mainit2(CSOUND *csound, ASSIGNM *p);
int32_t storei(CSOUND *csound, STOREI *p);
int32_t retrievek(CSOUND *csound, STOREI *p);
