/*
    scxtract.c:

    Copyright (C) 1991 Barry Vercoe

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

extern void readxfil(CSOUND *, FILE *), extract(CSOUND *), swrite(CSOUND *);
extern void sfree(CSOUND *csound);
extern int  sread(CSOUND *csound);
extern void sread_init(CSOUND *csound);

/* called from xmain.c or some other main */
/*   extracts events from each score sect */
/*   according to the controlling xfile   */

int scxtract(CSOUND *csound, FILE *scin, FILE * scout, FILE *xfile)
{
    int     n;

    readxfil(csound, xfile);
    csound->scorein = scin;
    csound->scoreout = scout;
    csound->sectcnt = 0;
    sread_init(csound);

    while ((n = sread(csound)) > 0) {
      /*  allout();   */
      /*  textout();  */
      extract(csound);
      swrite(csound);
    }

    sfree(csound);              /* return all memory used */
    return 0;
}

