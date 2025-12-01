/*
    schedule.h:

    Copyright (C) 1999, 2002 rasmus ekman, Istvan Varga, John ffitch,
                             Gabriel Maldonado, matt ingalls

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
*/

#pragma once

typedef struct {
        OPDS   h;
        MYFLT  *which, *when, *dur;
        MYFLT  *argums[VARGMAX-3];
        int32_t midi;
        INSDS  *kicked;
} SCHED;


typedef struct {
        OPDS   h;
        MYFLT  *argums[VARGMAX];
} SCHEDO;



typedef struct {
        OPDS   h;
        MYFLT  *trigger;
        MYFLT  *which, *when, *dur;
        MYFLT  *argums[VARGMAX-3];
        int32_t    todo;
        MYFLT  abs_when;
        int32_t    midi;
        INSDS  *kicked;
} WSCHED;

typedef struct {
        OPDS    h;
        MYFLT   *res;
        MYFLT   *kamp, *xcps, *type;
        AUXCH   auxd;
        MYFLT   *sine;
        int32_t lasttype;
        int32    phs;
} LFO;


typedef struct {
    OPDS   h;
    STRINGDAT *opcod;
    MYFLT  *args[VARGMAX];
    int32_t argno;
} LINEVENT;

typedef struct {
    OPDS   h;
    INSTANCEREF *inst;
    MYFLT  *args[VARGMAX];
    int32_t argno;
} LINEVENT2;

/*****************************************************************/
/* triginstr - Start instrument events at k-rate from orchestra. */
/* August 1999 by rasmus ekman.                                  */
/*****************************************************************/

typedef struct {
        OPDS   h;
        MYFLT  *trigger, *mintime, *maxinst;
        MYFLT  *args[PMAX+1];
        MYFLT  prvmintim;
        int32   timrem, prvktim, kadjust;
} TRIGINSTR;

/*****************************************************************/
/* trigseq, seqtime -                                            */
/* May 2000 by Gabriel Maldonado                                 */
/*****************************************************************/

typedef struct {
  OPDS  h;
  MYFLT *ktrig, *kstart, *kloop, *initndx, *kfn, *outargs[VARGMAX];
  int32  ndx;
  int32_t   nargs, done;
  int32  pfn;
  MYFLT *table;
} TRIGSEQ;

typedef struct {
  OPDS  h;
  MYFLT *ktrig, *unit_time, *kstart, *kloop, *initndx, *kfn;
  int32 ndx;
  int32_t   done;
  double start, newtime;
  int32 pfn;
  MYFLT *table;
} SEQTIM;


int32_t insert_score_args_at_sample(CSOUND *csound, const EVTBLK *ep,
                                    MYFLT *pfields[VARGMAX],
                                    int64_t time_ofs);
int32_t event_opcode_init(CSOUND *csound, LINEVENT *p, int32_t cnt, int32_t s, char p1);
int32_t event_opcode_perf(CSOUND *csound, LINEVENT *p, int32_t cnt, int32_t s, char p1);

typedef struct {
  OPDS h;
  MYFLT *arg[PMAX];
} RMEVT;


