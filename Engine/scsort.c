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

extern void sort(CSOUND*);
extern void twarp(CSOUND*);
extern void swritestr(CSOUND*, CORFIL *sco, int first);
extern void sfree(CSOUND *csound);
//extern void sread_init(CSOUND *csound);
extern int  sread(CSOUND *csound);

/* called from smain.c or some other main */
/* reads,sorts,timewarps each score sect in turn */

extern void sread_initstr(CSOUND *, CORFIL *sco);
char *scsortstr(CSOUND *csound, CORFIL *scin)
{
    int     n;
    int     m = 0, first = 0;
    CORFIL *sco;

    csound->scoreout = NULL;
    if(csound->scstr == NULL && (csound->engineStatus & CS_STATE_COMP) == 0) {
       first = 1;
       sco = csound->scstr = corfile_create_w();
    }
    else sco = corfile_create_w();
    csound->sectcnt = 0;
    sread_initstr(csound, scin);

    while ((n = sread(csound)) > 0) {
      sort(csound);
      twarp(csound);
      swritestr(csound, sco, first);
      m++;
    }
    if (first) {
      if (m==0)
        corfile_puts("f0 800000000000.0\ne\n", sco); /* ~25367 years */
      else corfile_puts("e\n", sco);
    }
    corfile_flush(sco);
    sfree(csound);
    if (first) return sco->body;
    else {
      char *str = strdup(sco->body);
      corfile_rm(&(sco));
      return str;
    }
}

