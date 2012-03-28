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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

void lpc_export_usage(void)
{
    printf("usage: lpc_export lpc_file cstext-file\n");
}

int main(int argc, char **argv)
{
    FILE *inf;
    FILE *outf;
    LPHEADER hdr;
    int i, j;
    char *str;
    MYFLT *coef;

    if (argc!= 3) {
      lpc_export_usage();
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      printf("Cannot open input file %s\n", argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      printf("Cannot open output file %s\n", argv[2]);
      return 1;
    }
    if (fread(&hdr, sizeof(LPHEADER)-4, 1, inf) != 1 ||
        (hdr.lpmagic != LP_MAGIC && hdr.lpmagic != LP_MAGIC2)) {
      printf("Failed to read LPC header\n");
      return 1;
    }
    fprintf(outf, "%d,%d,%d,%d,%f,%f,%f",
            hdr.headersize, hdr.lpmagic, hdr.npoles, hdr.nvals,
            hdr.framrate, hdr.srate, hdr.duration);
    str = (char *)malloc(hdr.headersize-sizeof(LPHEADER)+4);
    if (UNLIKELY(fread(&hdr, sizeof(char), hdr.headersize-sizeof(LPHEADER)+4, inf)!=hdr.headersize-sizeof(LPHEADER)+4)){
      fprintf(stderr, "Read failure\n");
      exit(1);
    }
    for (i=0; i<hdr.headersize-sizeof(LPHEADER)+4; i++)
      putc(str[i],outf);
    putc('\n', outf);
    coef = (MYFLT *)malloc((hdr.npoles+hdr.nvals)*sizeof(MYFLT));
    for (i = 0; i<floor(hdr.framrate*hdr.duration); i++) {
      if (UNLIKELY(fread(&coef[0], sizeof(MYFLT), hdr.npoles, inf) != hdr.npoles)) {
        fprintf(stderr, "Read failure\n");
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
