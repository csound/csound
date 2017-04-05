/*
    cscore_ex2.c:

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
     EVENT  *e;                    /* a pointer to an event   */
     EVLIST *a;
     a = cscoreListGetSection(cs); /* read a score as a list of events */
     e = a->e[4];                  /* point to event 4 in event list a  */
     e->p[5] *= 2;                 /* find pfield 5, multiply its value by 2 */
     cscoreListPut(cs, a);         /* write out the list of events  */
     cscorePutString(cs, "e");     /* add a "score end" statement */
}
