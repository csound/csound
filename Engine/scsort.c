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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"                                  /*   SCSORT.C  */
#include "corfile.h"
#include <ctype.h>

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
    int     first = 0;
    CORFIL *sco;

    csound->scoreout = NULL;
    if (csound->scstr == NULL && (csound->engineStatus & CS_STATE_COMP) == 0) {
      first = 1;
      sco = csound->scstr = corfile_create_w(csound);
    }
    else sco = corfile_create_w(csound);
    csound->sectcnt = 0;
    sread_initstr(csound, scin);

    while ((n = sread(csound)) > 0) {
      if (csound->frstbp->text[0] == 's') { // ignore empty segment
        // should this free memory?
        //printf("repeated 's'\n");
        continue;
      }
      sort(csound);
      twarp(csound);
      swritestr(csound, sco, first);
      //printf("sorted: >>>%s<<<\n", sco->body);
    }
    //printf("**** first = %d body = >>%s<<\n", first, sco->body);
    if (first) {
      int i = 0;
      while (isspace(sco->body[i])) i++;
      if (sco->body[i] == 'e' && sco->body[i+1] == '\n' && sco->body[i+2] != 'e') {
        corfile_rewind(sco);
        corfile_puts(csound, "f0 800000000000.0\ne\n", sco); /* ~25367 years */
      }
      else corfile_puts(csound, "e\n", sco);
      //printf("body >>%s<<\n", sco->body);
    }
    corfile_flush(csound, sco);
    sfree(csound);
    if (first) {
      return sco->body;
    }
    else {
      char *str = cs_strdup(csound,sco->body);
      corfile_rm(csound, &(sco));
      return str;
    }
}

