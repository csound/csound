/*  
    pv_import.c

    Copyright (C) 1995 John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* ***************************************************************** */
/* ******** Program to import pvoc files from tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 17                                           */
/* ***************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "pvoc.h"
#include "text.h"

void usage(int);
MYFLT getnum(FILE*, char *);

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}

int main(int argc, char **argv)
{
    PVSTRUCT pv;
    FILE *inf;
    FILE *outf;
    int i;

    if (argc!= 3)
      usage(argc);
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      fprintf(stderr, Str("Cannot open input file %s\n"), argv[1]);
      exit(1);
    }
    outf = fopen(argv[2], "w");
    if (inf == NULL) {
      fprintf(stderr, Str("Cannot open output file %s\n"), argv[2]);
      exit(1);
    }
    fscanf(inf,
           "ByteOffset,DataSize,dFormat,Rate,Channels,FrameSize,"
           "FrameInc,BSize,frameFormat,MinFreq,MaxFreq,LogLin\n");
#ifdef USE_DOUBLE
    fscanf(inf, "%ld,%ld,%ld,%lg,%ld,%ld,%ld,%ld,%ld,%lg,%lg,%ld\n",
            &pv.headBsize, &pv.dataBsize, &pv.dataFormat, &pv.samplingRate,
            &pv.channels, &pv.frameSize, &pv.frameIncr, &pv.frameBsize,
            &pv.frameFormat, &pv.minFreq, &pv.maxFreq, &pv.freqFormat);
#else
    fscanf(inf, "%ld,%ld,%ld,%g,%ld,%ld,%ld,%ld,%ld,%g,%g,%ld\n",
            &pv.headBsize, &pv.dataBsize, &pv.dataFormat, &pv.samplingRate,
            &pv.channels, &pv.frameSize, &pv.frameIncr, &pv.frameBsize,
            &pv.frameFormat, &pv.minFreq, &pv.maxFreq, &pv.freqFormat);
#endif
    i = pv.dataBsize/pv.frameBsize;
    pv.magic = PVMAGIC;

    fwrite(&pv, (size_t)1, (size_t)sizeof(pv), outf);
    for (; i!=0; i--) {
      int j;
      for (j = 0; j<pv.frameBsize/sizeof(MYFLT); j ++) {
        char term;
        MYFLT data = getnum(inf, &term);
        fwrite(&data, (size_t)1, (size_t)sizeof(MYFLT),outf);
        if (term!=',' && term!='\n') fprintf(stderr, Str("Sync error\n"));
      }
    }
    fclose(inf);
    fclose(outf);
    return 0;
}

MYFLT getnum(FILE* inf, char *term)
{
    char buff[100];
    int  cc;
    int p = 0;
    while ((cc=getc(inf))!=',' && cc!='\n') buff[p++] = cc;
    buff[p]='\0';
    *term = cc;
    return (MYFLT)atof(buff);
}

void usage(int argc)
{
    fprintf(stderr, Str("pv_export usage: pvfile commafile\n"));
    fprintf(stderr, "argc=%d\n", argc);
    exit(1);
}

