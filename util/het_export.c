/*
  het_export.c

    Copyright (C) 1995 John ffitch

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
/* ***************************************************************** */
/* ******** Program to export hetro files in tabular format. ******* */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 19                                           */
/* ***************************************************************** */

#include "std_util.h"
#include <stdio.h>
#include <stdarg.h>

#define END  32767

void het_export_usage(CSOUND *csound)
{
    csound->Message(csound, Str("Usage: het_export het_file cstext_file\n"));
}

static int het_export(CSOUND *csound, int argc, char **argv)
{
    MEMFIL *inf;
    FILE *outf;
    int16 *adp;
    int16 *endata;
    int cc = 0;

    if (argc!= 3) {
      het_export_usage(csound);
      return 1;
    }
    inf = csound->ldmemfile2withCB(csound, argv[1], CSFTYPE_HETRO,NULL);
    if (inf == NULL) {
      csound->Message(csound, Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      csound->Message(csound, Str("Cannot open output file %s\n"), argv[2]);
      return 1;
    }
    adp = (int16 *) inf->beginp;
    endata = (int16 *) inf->endp;
    cc = 0;

    fprintf(outf, "HETRO ");
    for (; adp<endata; adp++) {
      if (*adp == END) fputc('\n',outf), cc = 0;
      else fprintf(outf, "%s%hd", (cc ? ",":""), *adp), cc = 1;
    }
    fclose(outf);
    return 0;
}

/* module interface */

int het_export_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "het_export", het_export);
    if (!retval) {
      retval =
        csound->SetUtilityDescription(csound, "het_export",
                                      Str("translate hetro analysis file "
                                          "to text form"));
    }
    return retval;
}

