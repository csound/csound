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

extern void sort(void), twarp(void), swrite(void), sfree(void);
extern void sread_init(void);
extern int sread(void);

void scsort(FILE *scin, FILE *scout)
    /* called from smain.c or some other main */
    /* reads,sorts,timewarps each score sect in turn */
{
    int n;

    SCOREIN = scin;
    SCOREOUT = scout;

    sectcnt = 0;
    printf("Calling sread_init\n");
    sread_init();
    do {
      if ((n = sread()) > 0) {
        printf("sread returns with %d\n", n);
        sort();
/*         if (!POLL_EVENTS()) break; /\* on Mac/Win, system events *\/ */
        twarp();
        swrite();
      }
    } while (/* POLL_EVENTS() &&*/ n > 1); /* on Mac/Win, allow system events */
    sfree();        /* return all memory used */
}

