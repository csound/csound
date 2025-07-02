/*
    pv_import.c

    Copyright (C) 1995 John ffitch
        modified  2006 John ffitch for Csound5 format

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
/* ******** Program to import pvoc files from tabular format. ****** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 17                                           */
/* ***************************************************************** */

#include "std_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "pvfileio.h"

static void pv_import_usage(CSOUND *csound)
{
    csound->Message(csound, "%s", Str("Usage: pv_import cstext_file pv_file\n"));
}

static float getnum(FILE* inf, int32_t *term)
{
    char buff[100];
    int32_t  cc;
    int32_t p = 0;
    while ((cc=getc(inf))!=',' && cc!='\n' && cc!=EOF && p<99) buff[p++] = cc;
    buff[p]='\0';
    *term = cc;
    return (float)atof(buff);
}

static int32_t pv_import(CSOUND *csound, int32_t argc, char **argv)
{
    FILE *inf;
    int32_t outf;
    PVOCDATA data;
    WAVEFORMATEX fmt;

    if (UNLIKELY(argc != 3)) {
      pv_import_usage(csound);
      return 1;
    }
    inf = fopen(argv[1], "rb");
    if (UNLIKELY(inf == NULL)) {
      csound->Message(csound, Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    if (UNLIKELY(UNLIKELY(EOF == fscanf(inf,
           "FormatTag,Channels,SamplesPerSec,AvgBytesPerSec,"
                                        "BlockAlign,BitsPerSample,cbSize\n")))) {
      csound->Message(csound, "%s", Str("Not a PV file\n"));
      exit(1);
    }
    {
      int32_t fmt1, fmt2, fmt3, fmt4, fmt5;
      if (UNLIKELY(7!=fscanf(inf, "%d,%d,%d,%d,%u,%u,%d\n",
             &fmt1, &fmt2, &fmt.nSamplesPerSec,
                             &fmt.nAvgBytesPerSec, &fmt3, &fmt4, &fmt5))) {
        printf("ill formed inout\n");
        exit(1);
      }
      fmt.wFormatTag = fmt1;
      fmt.nChannels = fmt2;
      fmt.nBlockAlign = fmt3;
      fmt.wBitsPerSample = fmt4;
      fmt.cbSize = fmt5;
    }
    if (UNLIKELY(EOF == fscanf(inf, "WordFormat,AnalFormat,SourceFormat,WindowType,"
            "AnalysisBins,Winlen,Overlap,FrameAlign,"
            "AnalysisRate,WindowParam\n"))) {
      csound->Message(csound, "%s", Str("Not a PV file\n"));
      exit(1);
    }
    {
      int32_t data1, data2, data3, data4;
      if (UNLIKELY(10!=fscanf(inf, "%d,%d,%d,%d,%d,%d,%d,%d,%g,%g\n",
                              &data1,&data2,&data3,&data4,&data.nAnalysisBins,
                              &data.dwWinlen, &data.dwOverlap,&data.dwFrameAlign,
                              &data.fAnalysisRate, &data.fWindowParam))) {
        printf("Ill formed data\n");
        exit(1);
      }
      data.wWordFormat = data1;
      data.wAnalFormat = data2;
      data.wSourceFormat = data3;
      data.wWindowType = data4;
    }

    {
      pv_stype stype = (fmt.wBitsPerSample==16?STYPE_16:
                        fmt.wBitsPerSample==24?STYPE_24:
                        fmt.wBitsPerSample==32?STYPE_32:STYPE_IEEE_FLOAT);
      outf = csound->PVOC_CreateFile(csound, argv[2],
                                     (data.nAnalysisBins-1)*2, data.dwOverlap,
                                     fmt.nChannels, data.wAnalFormat,
                                     fmt.nSamplesPerSec, stype,
                                     data.wWindowType, data.fWindowParam,
                                     NULL, data.dwWinlen);
    }
    if (UNLIKELY(outf < 0)) {
      csound->Message(csound, Str("Cannot open output file %s\n"), argv[2]);
      fclose(inf);
      return 1;
    }

    {
      float *frame =
        (float*) csound->Malloc(csound, data.nAnalysisBins*2*sizeof(float));
      int32_t i;
      if (UNLIKELY(frame==NULL)) {
        csound->Message(csound, "%s", Str("Memory failure\n"));
        exit(1);
      }
      for (i=1;;i++) {
        uint32_t j;
        for (j=0; j<data.nAnalysisBins*2; j++) {
          int32_t term;
          frame[j] = getnum(inf, &term);
          if (term==EOF) goto ending;
          if (feof(inf)) goto ending;
          if (UNLIKELY(term!=',' && term!='\n'))
            csound->Message(csound, "%s", Str("Sync error\n"));
        }
        if (UNLIKELY(i%100==0)) csound->Message(csound, "%d\n", i);
        csound->PVOC_PutFrames(csound, outf, frame, 1);
      }
    ending:
      csound->Free(csound,frame);
    }
    fclose(inf);
    csound->PVOC_CloseFile(csound, outf);
    return 0;
}

/* module interface */

int32_t pv_import_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "pv_import", pv_import);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "pv_import",
                                      Str("translate text form to "
                                          "PVOC analysis file"));
    }
    return retval;
}

