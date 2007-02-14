/*	ParallelFifths.c

	Cscore control program which adds a copy of the input score 
	to itself a perfect fifth higher.  Assumes that the p5 values
	of note events are frequency values in Hertz.
	
	Anthony Kozar
	June 10, 2004
	Feb. 12, 2007:  Updated for Csound 5.
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

#include "cscore.h"                              /*  CSCORE_DEFAULT.C   */

void cscore(CSOUND* cs)        /* callable from Csound or standalone cscore  */
                         	 /* csound -C will run Csound scores as normal */
{
    EVLIST *a, *b, *c;
    int i;
    
    /* read each section of the score, one at a time */
    while ( (a = cscoreListGetSection(cs)) != NULL && a->nevents > 0) {
      c = cscoreListSeparateTWF(cs, a);				// separate non-i-statements to avoid copying them
      b = cscoreListCopyEvents(cs, a);				// copy event list a into b
      a = cscoreListConcatenate(cs, a,c);				// recombine a & c
    	for ( i = 1; i <= b->nevents; i++ )				// iterate over b: events have indices 1 to nevents
    	  if ( b->e[i]->op == 'i' )
    	    b->e[i]->p[5] *= 1.50;					// transpose list b up a fifth
    	a = cscoreListConcatenate(cs, a,b);				// combine lists a and b
    	cscoreListSort(cs, a);						// must sort to play correctly
      a = cscoreListAppendStringEvent(cs, a,"s");       	// re-append the s statement -- must assign to a in case list grows!
      cscoreListPut(cs, a);               			// output this section
      cscoreListFreeEvents(cs, a);              		// reclaim the space
      cscoreListFree(cs, b);
      cscoreListFree(cs, c);
    }
    
    if (a)	cscoreListFreeEvents(cs, a);				// must reclaim space from lget() above if non-NULL
    a = cscoreListCreate(cs, 1);					// allocate space for 1, not 0!
    a = cscoreListAppendStringEvent(cs, a,"e");			// must reassign a in case it grows
    cscoreListPut(cs, a);               				// output the 'e'
    cscoreListFreeEvents(cs, a);
    
    return;
}
