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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    int32_t    done, first_flag;
    double start, newtime;
    int32  pfn;
    MYFLT  *table, curr_unit_time;
} SEQTIM2;


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

static int32_t seqtim2_set(CSOUND *csound, SEQTIM2 *p)
{
    FUNC *ftp;
    int32 start, loop;
    int32 *ndx = &p->ndx;
    p->pfn = (int32) *p->kfn;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) {
      return csound->InitError(csound, "%s", Str("seqtim: incorrect table number"));
    }
    *ndx = (int32) *p->kinitndx;
    p->done=0;
    p->table =  ftp->ftable;
    p->newtime = p->table[p->ndx];
    p->start = CS_KCNT * CS_ONEDKR;
    start = (int32) *p->kstart;
    loop = (int32) *p->kloop;
    if (loop > 0 ) {
      (*ndx)++;
      *ndx %= loop;
      if (*ndx == 0)  {
        *ndx += start;
      }
    }
    else if (loop < 0 ){
      (*ndx)--;
      while (*ndx < start) {
        *ndx -= loop + start;
      }
    }
    p->curr_unit_time = *p->unit_time;
    p->first_flag = 1;
    return OK;
}

static int32_t seqtim2(CSOUND *csound, SEQTIM2 *p)
{
    if (*p->ktrigin) {
      p->ndx = (int32) *p->kinitndx;
    }

    if (p->done)
      goto end;
    else {
      int32 start = (int32) *p->kstart, loop = (int32) *p->kloop;
      int32 *ndx = &p->ndx;

      if (p->pfn != (int32)*p->kfn) {
        FUNC *ftp;
        if (UNLIKELY( (ftp = csound->FTFind(csound, p->kfn) ) == NULL)) goto err1;
        p->pfn = (int32)*p->kfn;
        p->table = ftp->ftable;
      }
      if (p->curr_unit_time != *p->unit_time) {
        double constant = p->start - CS_KCNT * CS_ONEDKR;
        double difference_new = p->newtime * p->curr_unit_time + constant;
        double difference_old = p->newtime * *p->unit_time     + constant;
        double difference = difference_new - difference_old;
        p->start = p->start + difference;
        p->curr_unit_time = *p->unit_time;
      }
      if (CS_KCNT * CS_ONEDKR
          > p->newtime * *p->unit_time + p->start) {
        float curr_val = p->table[p->ndx];
        p->newtime += p->table[p->ndx];
        if (loop > 0 ) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0){
            if (start == loop) {
              p->done =1;
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
          p->first_flag = 0;
        }
        else {
        end:
          *p->ktrig = FL(0.0);
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("seqtim: incorrect table number"));
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

