/*  
    pv_export.c

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
/* ******** Program to export pvoc files in tabular format. ******** */
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

int main(int argc, char **argv)
{
    PVSTRUCT pv;
    FILE *inf;
    FILE *outf;
    int i;
    MYFLT *data;

    init_getstring(0, NULL);
    if (argc!= 3)
      usage(argc);
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
      fprintf(stderr, Str("Cannot open input file %s\n"), argv[1]);
      exit(1);
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      fprintf(stderr, Str("Cannot open output file %s\n"), argv[2]);
      exit(1);
    }
    if ((i = PVReadHdr(inf, &pv)) != PVE_OK) {
      char *msg;
      switch (i) {
      case PVE_NOPEN:
        msg = Str("could not open file");
        break;
      case PVE_NPV:
        msg = Str("not a PVOC file");
        break;
      case PVE_MALLOC:
        msg = Str("could not allocate memory");
        break;
      case PVE_RDERR:
        msg = Str("read error");
        break;
      case PVE_WRERR:
        msg = Str("write error");
        break;
      default:
        msg = "???";
        break;
      }
      fprintf(stderr, Str("Error reading PV header: %s\n"), msg);
      exit(1);
    }
    fprintf(outf,
            "ByteOffset,DataSize,dFormat,Rate,Channels,FrameSize,"
            "FrameInc,BSize,frameFormat,MinFreq,MaxFreq,LogLin\n");
    fprintf(outf, "%ld,%ld,%ld,%g,%ld,%ld,%ld,%ld,%ld,%g,%g,%ld\n",
            pv.headBsize, pv.dataBsize, pv.dataFormat, pv.samplingRate,
            pv.channels, pv.frameSize, pv.frameIncr, pv.frameBsize,
            pv.frameFormat, pv.minFreq, pv.maxFreq, pv.freqFormat);
    i = pv.dataBsize/pv.frameBsize;
    data = (MYFLT*) malloc(pv.frameBsize);
    for (; i!=0; i--) {
      int j;
      fread(data, (size_t)1, (size_t)pv.frameBsize, inf);
      for (j = 0; j<pv.frameBsize/sizeof(MYFLT); j ++)
        fprintf(outf, "%s%g", (j==0 ? "" : ","), data[j]);
      fprintf(outf, "\n");
    }
    free(data);
    fclose(inf);
    fclose(outf);
    return 0;
}

void usage(int argc)
{
    fprintf(stderr, Str("pv_export usage: pvfile commafile\n"));
    fprintf(stderr, "argc=%d\n", argc);
    exit(1);
}

