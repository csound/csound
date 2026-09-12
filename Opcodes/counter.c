/*
    counter.c:

    Copyright (C) 2020 John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

/* Structure of a counter */
typedef struct {
  MYFLT val;
  MYFLT max;
  MYFLT min;
  MYFLT inc;
  uint64_t      cycles, generation;
  int32_t       active;
} COUNT;

/* for create counter ocde */
typedef struct {
  OPDS          h;
  MYFLT         *res;
  MYFLT         *max, *min, *inc;
} CNTSET;

typedef struct {
  OPDS          h;
  MYFLT         *res;
  MYFLT         *icnt;
  COUNT         *cnt;
  uint64_t      generation;
} COUNTER;

typedef struct {
  OPDS          h;
  MYFLT         *max, *min, *inc;
  MYFLT         *icnt;
  COUNT         *cnt;
  uint64_t      generation;
} CNTSTATE;


/* Global structure for all counters */
typedef  struct {
  int32_t           max_num;
  int32_t           used;
  int32_t           free;
  COUNT         **cnts;
} CNT_GLOBALS;


/* Create a counter */
static int32_t setcnt(CSOUND *csound, CNTSET *p)
{
    COUNT *y;
    CNT_GLOBALS *q = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    int32_t m = 0;
    if (q==NULL) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "counterGlobals_",
                                                    sizeof(CNT_GLOBALS)) != 0))
        return
          csound->InitError(csound, "%s",
                            Str("counter: failed to allocate globals"));
      q = (CNT_GLOBALS*)csound->QueryGlobalVariable(csound, "counterGlobals_");
      q->max_num = 10;
      q->cnts = (COUNT**)csound->Calloc(csound, 10*sizeof(COUNT*));
    }
    if (q->free) {
      while (q->cnts[m]->active) m++;
      q->free--;
    } else {
      /* Counter handles must remain exact in either MYFLT format. */
      const int32_t limit = sizeof(MYFLT) == sizeof(float) ? 16777216 : INT32_MAX;
      if (q->used == limit)
        return csound->InitError(csound, "%s", Str("counter: too many counters"));
      if (q->used == q->max_num) {
        int32_t capacity = q->max_num > limit - 10 ? limit : q->max_num + 10;
        if ((size_t)capacity > SIZE_MAX / sizeof(COUNT*))
          return csound->InitError(csound, "%s", Str("counter: too many counters"));
        COUNT **tt = (COUNT**)csound->ReAlloc(csound, q->cnts,
                                             (size_t)capacity * sizeof(COUNT*));
        if (tt == NULL)
          return csound->InitError(csound, "%s",
                                   Str("Failed to allocate counters\n"));
        q->cnts = tt;
        q->max_num = capacity;
      }
      m = q->used++;
      q->cnts[m] = (COUNT*)csound->Calloc(csound, sizeof(COUNT));
    }
    y = q->cnts[m];
    y->active = 1;
    y->generation++;
    y->cycles = 0;
    y->val = 0;
    y->min = *p->min;
    y->max = *p->max;
    y->inc = *p->inc;
    *p->res = (MYFLT)m;
    return OK;
}

/* Slots remain allocated until engine reset. A new generation on reuse keeps
   existing opcodes from silently attaching to a replacement counter. */
#define COUNTER_VALID(p) ((p)->cnt->active && \
                          (p)->generation == (p)->cnt->generation)

static COUNT* find_counter(CNT_GLOBALS *globals, MYFLT handle)
{
    double id = handle;
    if (UNLIKELY(globals == NULL || !(id >= 0.0 && id < globals->used)))
      return NULL;
    COUNT *cnt = globals->cnts[(int32_t)id];
    return cnt->active ? cnt : NULL;
}

static int32_t count_init(CSOUND *csound, COUNTER *p)
{
    CNT_GLOBALS *globals = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    COUNT *q = find_counter(globals, *p->icnt);
    if (UNLIKELY(q == NULL))
      return csound->InitError(csound, "%s", Str("counter: invalid handle"));
    p->cnt = q;
    p->generation = q->generation;
    return OK;
}

static int32_t count_init0(CSOUND *csound, COUNTER *p)
{
    CNT_GLOBALS *globals = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    COUNT *q = find_counter(globals, *p->res);
    if (UNLIKELY(q == NULL))
      return csound->InitError(csound, "%s", Str("counter: invalid handle"));
    p->cnt = q;
    p->generation = q->generation;
    return OK;
}

static int32_t count_perf(CSOUND *csound, COUNTER *p)
{
    if (UNLIKELY(!COUNTER_VALID(p)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("counter: counter has been deleted"));
    COUNT *q = p->cnt;
    if (q->val > q->max) {
      q->val = q->min;
      q->cycles ++;
    }
    else if (q->val < q->min) {
      q->val = q->max;
      q->cycles ++;
    }
    *p->res = q->val;
    q->val += q->inc;
    return OK;
}

static int32_t count_init_perf(CSOUND *csound, COUNTER *p)
{
    if (count_init(csound,p)==OK) return count_perf(csound,p);
    return NOTOK;
}

static int32_t count_cycles(CSOUND *csound, COUNTER* p)
{
    if (UNLIKELY(!COUNTER_VALID(p)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("counter: counter has been deleted"));
    *p->res = p->cnt->cycles;
    return OK;
}

static int32_t count_read(CSOUND *csound, COUNTER* p)
{
    if (UNLIKELY(!COUNTER_VALID(p)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("counter: counter has been deleted"));
    *p->res = p->cnt->val;
    return OK;
}

static int32_t count_reset(CSOUND *csound, COUNTER* p)
{
    if (UNLIKELY(!COUNTER_VALID(p)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("counter: counter has been deleted"));
    p->cnt->val = p->cnt->min;
    return OK;
}

static int32_t count_init3(CSOUND *csound, CNTSTATE *p)
{
    CNT_GLOBALS *globals = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    COUNT *q = find_counter(globals, *p->icnt);
    if (UNLIKELY(q == NULL))
      return csound->InitError(csound, "%s", Str("counter: invalid handle"));
    p->cnt = q;
    p->generation = q->generation;
    return OK;
}

static int32_t count_state(CSOUND *csound, CNTSTATE *p)
{
    if (UNLIKELY(!COUNTER_VALID(p)))
      return csound->PerfError(csound, &p->h, "%s",
                               Str("counter: counter has been deleted"));
    *p->max = p->cnt->max;
    *p->min = p->cnt->min;
    *p->inc = p->cnt->inc;
    return OK;
}

static int32_t count_del(CSOUND *csound, COUNTER* p)
{
    CNT_GLOBALS *q = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    COUNT *cnt = find_counter(q, *p->icnt);
    if (cnt == NULL) {
      *p->res = -FL(1.0);
      return OK;
    }
    int32_t n = (int32_t)*p->icnt;
    cnt->active = 0;
    q->free++;
    *p->res = (MYFLT)n;
    return OK;
}

#define S(x)    sizeof(x)

/* All counter operations share state, including across instrument instances. */
static OENTRY counter_localops[] = {
  { "cntCreate", S(CNTSET), IB,  "i", "pop", (SUBR)setcnt, NULL, NULL, NULL, 2   },
  { "count", S(COUNTER), IB,  "k", "o", (SUBR)count_init, (SUBR)count_perf },
  { "count_i", S(COUNTER), IB,  "i", "o", (SUBR)count_init_perf, NULL, NULL, NULL, 2 },
  { "cntCycles", S(COUNTER), IB,  "k", "o", (SUBR)count_init, (SUBR)count_cycles, NULL, NULL, 2 },
  { "cntRead", S(COUNTER), IB,  "k", "o", (SUBR)count_init, (SUBR)count_read, NULL, NULL, 2 },
  { "cntReset", S(COUNTER), IB,  "", "o", (SUBR)count_init0, (SUBR)count_reset, NULL, NULL, 2 },
  { "cntState", S(CNTSTATE), IB,  "kkk", "o", (SUBR)count_init3, (SUBR)count_state, NULL, NULL, 2 },
  { "cntDelete", S(COUNTER), IB,  "k", "k", NULL, (SUBR)count_del, NULL, NULL, 2 },
  { "cntDelete_i", S(COUNTER), IB,  "i", "i", (SUBR)count_del, NULL, NULL, NULL, 2 },
  /* aliases */
  { "cntcreate", S(CNTSET), IB,  "i", "pop", (SUBR)setcnt, NULL, NULL   },
  { "counti", S(COUNTER), IB,  "i", "o", (SUBR)count_init_perf, NULL },
  { "cntcycles", S(COUNTER), IB,  "k", "o", (SUBR)count_init, (SUBR)count_cycles },
  { "cntread", S(COUNTER), IB,  "k", "o", (SUBR)count_init, (SUBR)count_read },
  { "cntreset", S(COUNTER), IB,  "", "o", (SUBR)count_init0, (SUBR)count_reset },
  { "cntstate", S(CNTSTATE), IB,  "kkk", "o", (SUBR)count_init3, (SUBR)count_state },
  { "cntdelete", S(COUNTER), IB,  "k", "k", NULL, (SUBR)count_del, NULL },
  { "cntdeletei", S(COUNTER), IB,  "i", "i", (SUBR)count_del, NULL, NULL },
 };

LINKAGE_BUILTIN(counter_localops)
