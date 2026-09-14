/*
    seqtime.c:

    Copyright (C) 2000 Gabriel Maldonado

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

#include "stdopcod.h"

typedef struct {
    OPDS   h;
    MYFLT  *ktrig, *unit_time, *kstart, *kloop, *initndx, *kfn;
    int32  ndx;
    int32_t    done, first_flag;
    double start, newtime;
    int32  pfn;
    MYFLT  *table, curr_unit_time;
} SEQTIM;

typedef struct {
    OPDS   h;
    MYFLT  *ktrig, *ktrigin, *unit_time, *kstart, *kloop, *kinitndx, *kfn;
    int32  ndx;
    int32_t    done;
    double start, newtime;
    int32  pfn;
    uint32_t flen;
    MYFLT  *table, curr_unit_time;
} SEQTIM2;


/* Preserve seqtime's legacy startup and loop timing for existing scores.
   The timing corrections below apply to seqtime2. */
static int32_t seqtim_set(CSOUND *csound, SEQTIM *p)    /* by G.Maldonado */
{
    FUNC *ftp;
    int32 start, loop;
    int32 *ndx = &p->ndx;
    p->pfn = (int32) *p->kfn;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("seqtime: incorrect table number"));
    }
    *ndx = (int32) *p->initndx;
    p->done = 0;
    p->table =  ftp->ftable;
    if (p->ndx  > 0)
      p->newtime = p->table[p->ndx-1];
    else
      p->newtime = 0;
    p->start = (double)CS_KCNT * CS_ONEDKR;
    start = (int32) *p->kstart;
    loop = (int32) *p->kloop;
    if (loop > 0) {
      *ndx %= loop;
      if (*ndx == 0) {
        *ndx += start;
      }
    }
    else if (loop < 0) {
      (*ndx)--;
      while (*ndx < start) {
        *ndx -= loop + start;
      }
    }
    p->curr_unit_time = *p->unit_time;
    p->first_flag= 1;
    return OK;
}

static int32_t seqtim(CSOUND *csound, SEQTIM *p)
{
    if (p->done)
      *p->ktrig=FL(0.0);
    else {
      int32 start = (int32) *p->kstart, loop = (int32) *p->kloop;
      int32 *ndx = &p->ndx;
      if (p->pfn != (int32)*p->kfn) {
        FUNC *ftp;
        if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) goto err1;
        p->pfn = (int32)*p->kfn;
        p->table = ftp->ftable;
      }

      if (p->curr_unit_time != *p->unit_time) {
        double constant = p->start - (double)CS_KCNT * CS_ONEDKR;
        double difference_new = p->newtime * p->curr_unit_time + constant;
        double difference_old = p->newtime * *p->unit_time     + constant;
        double difference = difference_new - difference_old;
        p->start = p->start + difference;
        p->curr_unit_time = *p->unit_time;
      }
      if (CS_KCNT * CS_ONEDKR
          > p->newtime * *p->unit_time + p->start) {
        MYFLT curr_val = p->table[p->ndx];
        p->first_flag = 0;
        p->newtime += (double)curr_val;
        if (loop > 0) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0){
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx += start;
          }
        }
        else if (loop < 0 ){
          (*ndx)--;
          while (p->ndx < 0) {
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx -= loop + start;
          }
        }
        *p->ktrig = curr_val * p->curr_unit_time;
      }
      else {
        if (UNLIKELY(p->first_flag)) {
          *p->ktrig = p->table[p->ndx];
          p->first_flag=0;
        }
        else {
          *p->ktrig=FL(0.0);
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("seqtime: incorrect table number"));
}

/**---------------------------------------**/

/* A positive loop endpoint is exclusive. Equal start/end selects a one-shot
   sequence with an inclusive endpoint. A negative endpoint reverses the loop
   over [start, -loop). */
static int32_t seqtim2_range(SEQTIM2 *p, int32_t *start, int32_t *loop)
{
    double first = *p->kstart, last = *p->kloop;
    if (UNLIKELY(!(first >= 0.0 && first < p->flen &&
                   last >= -(double)p->flen && last <= p->flen)))
      return NOTOK;
    *start = (int32_t)first;
    *loop = (int32_t)last;
    return (*loop < 0 ? *start < -*loop : *start <= *loop) ? OK : NOTOK;
}

static void seqtim2_advance(SEQTIM2 *p, int32_t start, int32_t loop)
{
    if (start == loop) {
      if (p->ndx >= loop)
        p->done = 1;
      else
        p->ndx++;
    }
    else if (loop > 0) {
      p->ndx = (p->ndx + 1) % loop;
      if (p->ndx == 0)
        p->ndx = start;
    }
    else {
      p->ndx--;
      if (p->ndx < start) {
        int32_t span = -loop - start;
        p->ndx = start + (p->ndx - start) % span;
        if (p->ndx < start)
          p->ndx += span;
      }
    }
}

static int32_t seqtim2_set(CSOUND *csound, SEQTIM2 *p)
{
    FUNC *ftp;
    int32_t start, loop;
    double number = *p->kfn, index = *p->kinitndx;
    if (UNLIKELY(!(number >= INT32_MIN && number <= INT32_MAX) ||
                 (ftp = csound->FTFind(csound, p->kfn)) == NULL ||
                 ftp->flen > INT32_MAX))
      return csound->InitError(csound, "%s",
                               Str("seqtime2: incorrect table number or size"));
    p->pfn = (int32_t)number;
    p->table = ftp->ftable;
    p->flen = ftp->flen;
    if (UNLIKELY(!(index >= 0.0 && index < p->flen) ||
                 seqtim2_range(p, &start, &loop) != OK))
      return csound->InitError(csound, "%s",
                               Str("seqtime2: index or loop out of range"));
    p->ndx = (int32_t)index;
    p->done = 0;
    /* The initial element sets the delay before the first event. */
    p->newtime = p->table[p->ndx];
    p->start = (double)CS_KCNT * CS_ONEDKR;
    p->curr_unit_time = *p->unit_time;
    seqtim2_advance(p, start, loop);
    return OK;
}

static int32_t seqtim2(CSOUND *csound, SEQTIM2 *p)
{
    int32_t start, loop;
    double number = *p->kfn;
    double now = (double)CS_KCNT * CS_ONEDKR;
    if (p->done && *p->ktrigin == FL(0.0)) {
      *p->ktrig = FL(0.0);
      return OK;
    }
    if (UNLIKELY(!(number >= INT32_MIN && number <= INT32_MAX)))
      goto table_error;
    if (p->pfn != (int32_t)number) {
      FUNC *ftp;
      if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL ||
                   ftp->flen > INT32_MAX))
        goto table_error;
      p->pfn = (int32_t)number;
      p->table = ftp->ftable;
      p->flen = ftp->flen;
    }
    if (UNLIKELY(seqtim2_range(p, &start, &loop) != OK))
      goto range_error;
    if (*p->ktrigin != FL(0.0)) {
      double index = *p->kinitndx;
      if (UNLIKELY(!(index >= 0.0 && index < p->flen)))
        goto range_error;
      p->ndx = (int32_t)index;
      if (p->done) {
        /* A completed one-shot has no pending event. Resume this cycle. */
        p->done = 0;
        p->newtime = 0.0;
        p->start = now - CS_ONEDKR;
      }
    }
    if (UNLIKELY((uint32_t)p->ndx >= p->flen))
      goto range_error;
    if (p->curr_unit_time != *p->unit_time) {
      /* Keep the pending deadline; rescale subsequent intervals. */
      p->start += p->newtime * ((double)p->curr_unit_time - *p->unit_time);
      p->curr_unit_time = *p->unit_time;
    }
    if (now > p->newtime * p->curr_unit_time + p->start) {
      MYFLT curr_val = p->table[p->ndx];
      p->newtime += (double)curr_val;
      seqtim2_advance(p, start, loop);
      *p->ktrig = curr_val * p->curr_unit_time;
    }
    else
      *p->ktrig = FL(0.0);
    return OK;
 table_error:
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("seqtime2: incorrect table number or size"));
 range_error:
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("seqtime2: index or loop out of range"));
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "seqtime", S(SEQTIM),  TR,  "k",    "kkkkk", (SUBR)seqtim_set, (SUBR)seqtim   },
{ "seqtime2", S(SEQTIM2),TR,  "k",    "kkkkkk", (SUBR)seqtim2_set, (SUBR)seqtim2}
};

int32_t seqtime_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
