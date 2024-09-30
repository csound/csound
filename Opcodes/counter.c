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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#if !(defined(__wasi__))

#include "interlocks.h"

/* Structure of a counter */
typedef struct {
  MYFLT val;
  MYFLT max;
  MYFLT min;
  MYFLT inc;
  int32_t   cycles;
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
} COUNTER;

typedef struct {
  OPDS          h;
  MYFLT         *max, *min, *inc;
  MYFLT         *icnt;
  COUNT         *cnt;
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
      int32_t n = 0;
      while (q->cnts[n]!=NULL) n++;
      q->free--;
      m = n;
    } else {
      if (q->max_num >= q->used) {
        COUNT** tt = q->cnts;
        tt = (COUNT**)csound->ReAlloc(csound,
                                      tt, (q->max_num+10)*sizeof(COUNT*));
        if (tt == NULL)
          return csound->InitError(csound, "%s",
                                   Str("Failed to allocate counters\n"));
        q->cnts = tt;
        q->max_num += 10;
      }
      m = q->used;
      ++q->used;
    }
    q->cnts[m] = (COUNT*)csound->Calloc(csound,sizeof(COUNT));
    y = q->cnts[m];
    y->val = 0;
    y->min = *p->min;
    y->max = *p->max;
    y->inc = *p->inc;
    *p->res = (MYFLT)m;
    return OK;
}

COUNT* find_counter(CSOUND *csound, int32_t n)
{
    CNT_GLOBALS *q = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    if (UNLIKELY(q==NULL)) return NULL;
    if (n>q->max_num || n<0) return NULL;
    return q->cnts[n];
}

static int32_t count_init(CSOUND *csound, COUNTER *p)
{
    COUNT* q = find_counter(csound, (int)*p->icnt);
    if (q==NULL) return NOTOK;
    p->cnt = q;
    return OK;
}

static int32_t count_init0(CSOUND *csound, COUNTER *p)
{
    COUNT* q = find_counter(csound, (int)*p->res);
    if (q==NULL) return NOTOK;
    p->cnt = q;
    return OK;
}

static int32_t count_perf(CSOUND *csound, COUNTER *p)
{
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
    *p->res = p->cnt->cycles;
    return OK;
}

static int32_t count_read(CSOUND *csound, COUNTER* p)
{
    *p->res = p->cnt->val;
    return OK;
}

static int32_t count_reset(CSOUND *csound, COUNTER* p)
{
    p->cnt->val = p->cnt->min;
    return OK;
}

static int32_t count_init3(CSOUND *csound, CNTSTATE *p)
{
    COUNT* q = find_counter(csound, (int)*p->icnt);
    if (q==NULL) return NOTOK;
    p->cnt = q;
    return OK;
}

static int32_t count_state(CSOUND *csound, CNTSTATE *p)
{
    *p->max = p->cnt->max;
    *p->min = p->cnt->min;
    *p->inc = p->cnt->inc;
    return OK;
}

static int32_t count_del(CSOUND *csound, COUNTER* p)
{
    int32_t n = (int)*p->icnt;
    CNT_GLOBALS *q = (CNT_GLOBALS*)
      csound->QueryGlobalVariable(csound, "counterGlobals_");
    if (q==NULL || n>q->max_num || n<0 || q->cnts[n]==NULL) {
      *p->res = -FL(1.0);
      return OK;
    }
    csound->Free(csound, q->cnts[n]);
    q->cnts[n] = NULL;
    q->free++;
    *p->res = (MYFLT)n;
    return OK;
}

#else

static int32_t setcnt(CSOUND *csound, void *p) {
  IGN(csound); IGN(p);
    return OK;

}

static int32_t count_init(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;

}

static int32_t count_init0(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_perf(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;

}

static int32_t count_init_perf(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_cycles(CSOUND *csound, void p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_read(CSOUND *csound, void p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_reset(CSOUND *csound, void p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_init3(CSOUND *csound, void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_state(CSOUND *csound,void *p)
{
  IGN(csound); IGN(p);
    return OK;
}

static int32_t count_del(CSOUND *csound, void p)
{
  IGN(csound); IGN(p);
    return OK;
}

#endif // !wasi


#define S(x)    sizeof(x)

static OENTRY counter_localops[] = {
  { "cntCreate", S(CNTSET), 0,  "i", "pop", (SUBR)setcnt, NULL, NULL   },
  { "count", S(COUNTER), SK,  "k", "o", (SUBR)count_init, (SUBR)count_perf },
  { "count_i", S(COUNTER), SK,  "i", "o", (SUBR)count_init_perf, NULL },
  { "cntCycles", S(COUNTER), SK,  "k", "o", (SUBR)count_init, (SUBR)count_cycles },
  { "cntRead", S(COUNTER), SK,  "k", "o", (SUBR)count_init, (SUBR)count_read },
  { "cntReset", S(COUNTER), SK,  "", "o", (SUBR)count_init0, (SUBR)count_reset },
  { "cntState", S(CNTSTATE), SK,  "kkk", "o", (SUBR)count_init3, (SUBR)count_state },
  { "cntDelete", S(COUNTER), SK,  "k", "k", NULL, (SUBR)count_del, NULL },
  { "cntDelete_i", S(COUNTER), SK,  "i", "i", (SUBR)count_del, NULL, NULL },
 };

LINKAGE_BUILTIN(counter_localops)
