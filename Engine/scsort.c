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

#include "csoundCore.h"                                  /*   SCSORT.C  */
#include "corfile.h"

extern void sort(CSOUND*), twarp(CSOUND*), swrite(CSOUND*), swritestr(CSOUND*);
extern void sfree(CSOUND *csound);
extern void sread_init(CSOUND *csound);
extern int  sread(CSOUND *csound);

/* called from smain.c or some other main */
/* reads,sorts,timewarps each score sect in turn */

#ifdef OLD_CODE
void scsort(CSOUND *csound, FILE *scin, FILE *scout)
{
    int     n;

    csound->scorein = scin;
    csound->scoreout = scout;

    csound->sectcnt = 0;
    sread_init(csound);
    while ((n = sread(csound)) > 0) {
      sort(csound);
      twarp(csound);
      swrite(csound);
    }
    sfree(csound);              /* return all memory used */
}
#endif

extern void sread_initstr(CSOUND *);
void scsortstr(CSOUND *csound, CORFIL *scin)
{
    int     n;
    int     m = 0;

    csound->scoreout = NULL;
    csound->scstr = corfile_create_w();
    csound->sectcnt = 0;
    sread_initstr(csound);

    while ((n = sread(csound)) > 0) {
      sort(csound);
      twarp(csound);
      swritestr(csound);
      m++;
    }
    if (m==0) corfile_puts("f0 800000000000.0\ne\n", csound->scstr); /* ~25367 years */
    else corfile_puts("e\n", csound->scstr);
    corfile_flush(csound->scstr);
    sfree(csound);              /* return all memory used */
}

