/*
    lpc_export.c

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
/* ***************************************************************** */
/* ******** Program to export lpanal files in tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 25                                           */
/* ***************************************************************** */

#include "std_util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
#include "lpc.h"
#include "text.h"

void lpc_export_usage(CSOUND *csound)
{
    csound->Message(csound, "%s", Str("usage: lpc_export lpc_file cstext-file\n"));
}

static int32_t lpc_export(CSOUND *csound, int32_t argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    uint32_t i, j;
    char *str;
    MYFLT *coef;

    if (UNLIKELY(argc!= 3)) {
      lpc_export_usage(csound);
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (UNLIKELY(inf == NULL)) {
      csound->Message(csound, Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (UNLIKELY(outf == NULL)) {
      csound->Message(csound, Str("Cannot open output file %s\n"), argv[2]);
      fclose(inf);
      return 1;
    }
    if (UNLIKELY(fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
                 (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2))) {
      csound->Message(csound, "%s", Str("Failed to read LPC header\n"));
      fclose(inf);
      fclose(outf);
      return 1;
    }
    fprintf(outf, "%d,%d,%d,%d,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    if (UNLIKELY(hdr.npoles<=0)) { fclose(inf); fclose(outf); return 1; }
    // to keep coverity happy
    if (UNLIKELY(hdr.headersize>0x40000000 ||
        hdr.headersize<sizeof(LPHEADER) ||
                 hdr.npoles+hdr.nvals > 0x10000000)) {
      fclose(inf); fclose(outf); return 2;
    }
    str = (char *)csound->Malloc(csound,hdr.headersize-sizeof(LPHEADER)+4);
    if (UNLIKELY(str==NULL)) {
        fclose(inf); fclose(outf); return 2;}
    if (UNLIKELY(fread(str, sizeof(char),
                       hdr.headersize-sizeof(LPHEADER)+4, inf)!=
                 hdr.headersize-sizeof(LPHEADER)+4))
      csound->Message(csound, "%s", Str("Read failure\n"));
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    putc('\n', outf);
    coef = (MYFLT *)csound->Malloc(csound,(hdr.npoles+hdr.nvals)*sizeof(MYFLT));
    if (UNLIKELY(coef==NULL)) {
      fclose(inf); fclose(outf); csound->Free(csound,str); return 3;}
    for (i = 0; i<(uint32_t)floor(hdr.framrate*hdr.duration); i++) {
      if (UNLIKELY(fread(&coef[0], sizeof(MYFLT), hdr.npoles, inf)!=hdr.npoles))
        csound->Message(csound, "%s", Str("Read failure\n"));
      for (j=0; j<hdr.npoles; j++)
        fprintf(outf, "%f%c", coef[j], (j==hdr.npoles-1 ? '\n' : ','));
    }
    fclose(outf);
    fclose(inf);
    csound->Free(csound,coef); csound->Free(csound,str);
    return 0;
}

/* module interface */

int32_t lpc_export_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "lpc_export", lpc_export);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "lpc_export",
                                      Str("translate linear predictive "
                                          "coding file to text file"));
    }
    return retval;
}

