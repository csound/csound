/*
    lpc_import.c

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
/* ******** Program to import lpanal files in tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1998 Nov 15                                           */
/* ***************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "std_util.h"
#include "lpc.h"

void lpc_import_usage(CSOUND *csound)
{
    csound->Message(csound, Str("Usage: lpc_import cstext_file lpc_file\n"));
}

static int lpc_import(CSOUND *csound, int argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    unsigned int i, j;
    char *str;
    MYFLT *coef;

    if (argc != 3) {
      lpc_import_usage(csound);
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      fprintf(stderr, Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      csound->Message(csound, Str("Cannot open output file %s\n"), argv[2]);
      fclose(inf);
      return 1;
    }
    if (fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
        (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2)) {
      csound->Message(csound, Str("Failed to read LPC header\n"));
      fclose(outf);
      fclose(inf);
      return 1;
    }
    fprintf(outf, "%d,%d,%d,%d,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    if (UNLIKELY(hdr.npoles<=0)) { fclose(outf); fclose(inf); return 1;}
    str = (char *)csound->Malloc(csound,hdr.headersize-sizeof(LPHEADER)+4);
    if (UNLIKELY(fread(&hdr, sizeof(char),
                       hdr.headersize-sizeof(LPHEADER)+4, inf)!=
                 hdr.headersize-sizeof(LPHEADER)+4))
      csound->Message(csound, Str("Read failure\n"));
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    putc('\n', outf);
    coef = (MYFLT *)csound->Malloc(csound, (hdr.npoles+hdr.nvals)*sizeof(MYFLT));
    for (i = 0; i<hdr.nvals; i++) {
      if (UNLIKELY(fread(&coef[0], sizeof(MYFLT),
                         hdr.npoles, inf)!=(size_t)hdr.npoles))
        csound->Message(csound, Str("Read failure\n"));
      for (j=0; j<hdr.npoles; j++)
        fprintf(outf, "%f%c", coef[j], (j==hdr.npoles-1 ? '\n' : ','));
    }
    fclose(outf);
    fclose(inf);
    csound->Free(csound,coef); csound->Free(csound,str);
    return 0;
}

/* module interface */

int lpc_import_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "lpc_import", lpc_import);
    if (!retval) {
      retval =
        csound->SetUtilityDescription(csound, "lpc_import",
                                      Str("translate text file to "
                                          "linear predictive coding file"));
    }
    return retval;
}

