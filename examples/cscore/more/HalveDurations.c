/*      HalveDurations.c

        Cscore control program which multiplies the durations of all
        i-statements by 0.5.

        Anthony Kozar
        July 6, 2004

        Sep 30 2005: Modified for Csound 5.
 */


/*
    derived from
    cscore.c:

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

#include "cscore.h"

void cscore(CSOUND* cs)
{
    EVLIST *a;
    int i;

    while ((a = cscoreListGetSection(cs)) != NULL && a->nevents > 0) {
      /* read each sect from score */
      for (i = 1; i <= a->nevents; i++) {       /* iterate over the events */
          if ( a->e[i]->op == 'i' )             /* only change if a note */
            a->e[i]->p[3] *= 0.5;               /* halve duration */
        }
      a = cscoreListAppendStringEvent(cs,a,"s");/* re-append the s statement */
      cscoreListPlay(cs,a);                     /* play this section         */
      cscoreListFreeEvents(cs,a);               /* reclaim the space         */
    }

    if (a)  cscoreListFreeEvents(cs,a);         /* must reclaim space from last
                                                   lget() */
    a = cscoreListCreate(cs,1);                 /* create a new event list with
                                                   at least 1 event */
    a = cscoreListAppendStringEvent(cs,a,"e");  /* append the end-of-score event */
    cscoreListPlay(cs,a);                       /* end-of-score for summaries */
    cscoreListFreeEvents(cs,a);

    return;
}
