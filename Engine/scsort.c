/*
    scsort.c:

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

#include "cs.h"                                          /*   SCSORT.C  */

extern void sort(ENVIRON*), twarp(ENVIRON*), swrite(ENVIRON*);
extern void sfree(ENVIRON *csound);
extern void sread_init(ENVIRON *csound);
extern int  sread(ENVIRON *csound);

void scsort(ENVIRON *csound, FILE *scin, FILE *scout)
    /* called from smain.c or some other main */
    /* reads,sorts,timewarps each score sect in turn */
{
    int     n;

    csound->scorein = scin;
    csound->scoreout = scout;

    csound->sectcnt = 0;
    sread_init(csound);
    do {
      if ((n = sread(csound)) > 0) {
/*      csound->Message(csound, Str("sread returns with %d\n"), n); */
        sort(csound);
        twarp(csound);
        swrite(csound);
      }
    } while (n > 1);
    sfree(csound);              /* return all memory used */
}

