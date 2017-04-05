/*
    cscore_ex4.c:

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
void cscore(CSOUND *cs)
 {
      EVENT  *e, *f;
      EVLIST *a, *b;
      int n, dim;                              /* declare two integer variables */

      a = cscoreListGetSection(cs);
      b = cscoreListSeparateF(cs, a);
      cscoreListPut(cs, b);
      cscoreListFreeEvents(cs, b);
      e = cscoreDefineEvent(cs, "t 0 120");
      cscorePutEvent(cs, e);
      cscoreListPut(cs, a);
      cscorePutString(cs, "s");
      cscorePutEvent(cs, e);                   /* write out another tempo statement */
      b = cscoreListCopyEvents(cs, a);

      dim = 0;                                 /* initialize dim to 0 */
      for (n = 1; n <= b->nevents; n++)
      {
          f = b->e[n];
          f->p[6] -= dim;                      /* subtract current value of dim */
          f->p[5] *= 0.5;                      /* transpose pitch down one octave */
          dim += 2000;                         /* increase dim for each note */
      }

      a = cscoreListAppendList(cs, a, b);      /* now add these notes to original pitches */
      cscoreListPut(cs, a);
      cscorePutString(cs, "e");
}
