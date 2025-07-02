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
/* Restored version that does not load all Csound5                   */
/* ***************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
#include "lpc.h"

#define Str(x) x

void lpc_export_usage(void)
{
    printf(Str("usage: lpc_export lpc_file cstext-file\n"));
}

int32_t main(int32_t argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    uint32_t i, j;
    char *str = NULL;
    MYFLT *coef = NULL;

    if (argc!= 3) {
      lpc_export_usage();
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      printf(Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      printf(Str("Cannot open output file %s\n"), argv[2]);
      fclose(inf);
      return 1;
    }
    if (fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
        (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2)) {
      printf(Str("Failed to read LPC header\n"));
      fclose(inf);
      fclose(outf);
      return 1;
    }
    fprintf(outf, "%d,%d,%d,%d,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    if (hdr.headersize>1024 || hdr.headersize<sizeof(LPHEADER)) {
      fprintf(stderr, Str("corrupt header\n"));
      exit(1);
    }
    str = (char *)malloc((size_t)(hdr.headersize-sizeof(LPHEADER)+4));
    if (UNLIKELY(str == NULL)) exit(1);
    if (UNLIKELY(fread(&str, sizeof(char),
                       hdr.headersize-sizeof(LPHEADER)+4, inf)!=
                 hdr.headersize-sizeof(LPHEADER)+4)){
      fprintf(stderr, Str("Read failure\n"));
      exit(1);
    }
    if (UNLIKELY(hdr.headersize>100)) hdr.headersize = 101;
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    if (UNLIKELY(hdr.headersize > 100))
      putc('\n', outf);
    if (hdr.npoles+hdr.nvals > 0
        && hdr.npoles+hdr.nvals < 0x0FFFFFFF
        && hdr.npoles > 0) {
      coef = (MYFLT *)malloc(((uint64_t)hdr.npoles+hdr.nvals)*sizeof(MYFLT));
      for (i = 0; i<floor(hdr.framrate*hdr.duration); i++) {
        if (UNLIKELY(fread(coef, sizeof(MYFLT), hdr.npoles,inf) != hdr.npoles)) {
          fprintf(stderr, Str("Read failure\n"));
          exit(1);
        }
        for (j=0; j<hdr.npoles; j++)
          fprintf(outf, "%f%c", coef[j], (j==hdr.npoles-1 ? '\n' : ','));
      }
    }
    fclose(outf);
    fclose(inf);
    if (hdr.npoles+hdr.nvals > 0 && hdr.npoles > 0 && coef != NULL) free(coef);
    free(str);
    return 0;
}
