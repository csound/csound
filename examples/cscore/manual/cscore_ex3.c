/*
    cscore_ex3.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include  "cscore.h"
void cscore(CSOUND *cs)
{
      EVENT  *e, *f;
      EVLIST *a, *b;
      int n;

      a = cscoreListGetSection(cs);            /* read score into event list "a" */
      b = cscoreListSeparateF(cs, a);          /* separate f statements */
      cscoreListPut(cs, b);                    /* write f statements out to score */
      cscoreListFreeEvents(cs, b);             /* and release the spaces used */
      e = cscoreDefineEvent(cs, "t 0 120");    /* define event for tempo statement */
      cscorePutEvent(cs, e);                   /* write tempo statement to score */
      cscoreListPut(cs, a);                    /* write the notes */
      cscorePutString(cs, "s");                /* section end */
      cscorePutEvent(cs, e);                   /* write tempo statement again */
      b = cscoreListCopyEvents(cs, a);         /* make a copy of the notes in "a" */
      for (n = 1; n <= b->nevents; n++)        /* iterate the following lines nevents times: */
      {
          f = b->e[n];
          f->p[5] *= 0.5;                      /* transpose pitch down one octave */
      }
      a = cscoreListAppendList(cs, a, b);      /* now add these notes to original pitches */
      cscoreListPut(cs, a);
      cscorePutString(cs, "e");
}
