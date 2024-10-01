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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
/* ***************************************************************** */
/* ******** Program to import lpanal files in tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1998 Nov 15                                           */
/* Version that does not need to load rest of csound5                */
/* ***************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
#include "lpc.h"

#define Str(x) x

void lpc_import_usage(void)
{
    printf("Usage: lpc_import cstext_file lpc_file\n");
}

int32_t main(int32_t argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    uint32_t i, j;
    char *str;
    MYFLT *coef;

    if (argc != 3) {
      lpc_import_usage();
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      fprintf(stderr, "Cannot open input file %s\n", argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      fprintf(stderr, "Cannot open output file %s\n", argv[2]);
      fclose(inf);
      return 1;
    }
    if (fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
        (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2)) {
      fprintf(stderr, "Failed to read LPC header\n");
      fclose(inf);
      fclose(outf);
      return 1;
    }
    fprintf(outf, "%d,%d,%d,%d,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    if (UNLIKELY(hdr.headersize < sizeof(LPHEADER)-4) || hdr.headersize>1024)
      return 1;
    str = (char *)malloc(hdr.headersize-sizeof(LPHEADER)+4);
    if (str==NULL) {
      printf(Str("memory allocation failure\n"));
      exit(1);
    }

    if (hdr.headersize-sizeof(LPHEADER)+4 !=
        fread(&str, sizeof(char), hdr.headersize-sizeof(LPHEADER)+4, inf)) {
      printf(Str("Ill formed data\n"));
      exit(1);
    }
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    putc('\n', outf);
    coef = (MYFLT *)malloc(hdr.npoles*sizeof(MYFLT));
    if (coef==NULL) {
      printf("memory allocation failure\n");
      exit(1);
    }
    for (i = 0; i<hdr.nvals; i++) {
      if (hdr.npoles != fread(coef, sizeof(MYFLT), hdr.npoles, inf)) {
        printf("Ill formed data\n");
        exit(1);
      }
      for (j=0; j<hdr.npoles; j++)
        fprintf(outf, "%f%c", coef[j], (j==hdr.npoles-1 ? '\n' : ','));
    }
    fclose(outf);
    fclose(inf);
    free(coef); free(str);
    return 0;
}
