/*
    scxtract.c:

    Copyright (C) 1991 Barry Vercoe; 2012 John ffitch

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

#include "csoundCore.h"                            /*  SCXTRACT.C  */
#include "corfile.h"

extern void readxfil(CSOUND *, FILE *), extract(CSOUND *);
extern void sfree(CSOUND *csound);
extern int  sread(CSOUND *csound);
extern void sread_init(CSOUND *csound);
extern void swritestr(CSOUND *csound);

/* called from xmain.c or some other main */
/*   extracts events from each score sect */
/*   according to the controlling xfile   */

extern void sread_initstr(CSOUND *);
int scxtract(CSOUND *csound, CORFIL *scin, FILE *xfile)
{
    int     n;

    csound->scoreout = NULL;
    csound->scorestr = scin;
    csound->scstr = corfile_create_w();
    csound->sectcnt = 0;
    readxfil(csound, xfile);
    sread_initstr(csound);

    while ((n = sread(csound)) > 0) {
      /*  allout();   */
      /*  textout();  */
      extract(csound);
      swritestr(csound);
    }
    corfile_flush(csound->scstr);
    sfree(csound);              /* return all memory used */
    return 0;
}
