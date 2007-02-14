/*
    cscore_ex5.c:

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

int cue[3] = {0,10,17};                        /* declare an array of 3 integers */ 

void cscore(CSOUND *cs) 
{
      EVENT  *e, *f;
      EVLIST *a, *b;
      int n, dim, cuecount;                    /* declare new variable cuecount */
      
      a = cscoreListGetSection(cs);
      b = cscoreListSeparateF(cs, a);
      cscoreListPut(cs, b);
      cscoreListFreeEvents(cs, b);
      e = cscoreDefineEvent(cs, "t 0 120");
      cscorePutEvent(cs, e);
      dim = 0; 
      for (cuecount = 0; cuecount <= 2; cuecount++) /* elements of cue are numbered 0, 1, 2 */
      {
          for (n = 1; n <= a->nevents; n++)
          { 
              f = a->e[n]; 
              f->p[6] -= dim; 
              f->p[2] += cue[cuecount];        /* add values of cue */ 
          } 
          printf("; diagnostic:  cue = %d\n", cue[cuecount]); 
          dim += 2000; 
          cscoreListPut(cs, a);
      } 
      cscorePutString(cs, "e");
}
