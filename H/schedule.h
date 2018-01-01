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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

typedef struct {
        OPDS   h;
        MYFLT  *which, *when, *dur;
        MYFLT  *argums[VARGMAX-3];
        int    midi;
        INSDS  *kicked;
} SCHED;

typedef struct {
        OPDS   h;
        MYFLT  *trigger;
        MYFLT  *which, *when, *dur;
        MYFLT  *argums[VARGMAX-3];
        int    todo;
        MYFLT  abs_when;
        int    midi;
        INSDS  *kicked;
} WSCHED;

typedef struct {
        OPDS    h;
        MYFLT   *res;
        MYFLT   *kamp, *xcps, *type;
        AUXCH   auxd;
        MYFLT   *sine;
        int     lasttype;
        int32    phs;
} LFO;

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
  int   nargs, done;
  int32  pfn;
  MYFLT *table;
} TRIGSEQ;

typedef struct {
  OPDS  h;
  MYFLT *ktrig, *unit_time, *kstart, *kloop, *initndx, *kfn;
  int32 ndx;
  int   done;
  double start, newtime;
  int32 pfn;
  MYFLT *table;
} SEQTIM;

