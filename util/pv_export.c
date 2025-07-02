/*
    pv_export.c

    Copyright (C) 1995 John ffitch
                  2006 John ffitch for Csound5

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
/* ******** Program to export pvoc files in tabular format. ******** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 17                                           */
/* ***************************************************************** */

#include "std_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "pvfileio.h"

static void pv_export_usage(CSOUND *csound)
{
    csound->Message(csound, "%s", Str("Usage: pv_export pv_file cstext_file\n"));
}

static int32_t pv_export(CSOUND *csound, int32_t argc, char **argv)
{
    int32_t inf;
    FILE *outf;
    int32_t i;
    PVOCDATA data;
    WAVEFORMATEX fmt;

    if (argc != 3) {
      pv_export_usage(csound);
      return 1;
    }
    inf = csound->PVOC_OpenFile(csound, argv[1], &data, &fmt);
    if (UNLIKELY(inf<0)) {
      csound->Message(csound, Str("Cannot open input file %s\n"), argv[1]);
      return 1;
    }
    if (strcmp(argv[2], "-")==0) outf=stdout;
    else
      outf = fopen(argv[2], "w");
    if (UNLIKELY(outf == NULL)) {
      csound->Message(csound, Str("Cannot open output file %s\n"), argv[2]);
      csound->PVOC_CloseFile(csound, inf);
      return 1;
    }

    fprintf(outf, "FormatTag,Channels,SamplesPerSec,AvgBytesPerSec,"
            "BlockAlign,BitsPerSample,cbSize\n");
    fprintf(outf, "%d,%d,%d,%d,%u,%u,%d\n",
            fmt.wFormatTag, fmt.nChannels, fmt.nSamplesPerSec,
            fmt.nAvgBytesPerSec, fmt.nBlockAlign, fmt.wBitsPerSample,
            fmt.cbSize);
    fprintf(outf, "WordFormat,AnalFormat,SourceFormat,WindowType,"
            "AnalysisBins,Winlen,Overlap,FrameAlign,"
            "AnalysisRate,WindowParam\n");
    fprintf(outf, "%d,%d,%d,%d,%d,%d,%d,%d,%g,%g\n",
            data.wWordFormat,data.wAnalFormat,data.wSourceFormat,
            data.wWindowType,data.nAnalysisBins,data.dwWinlen,
            data.dwOverlap,data.dwFrameAlign,data.fAnalysisRate,
            data.fWindowParam);
/*     if (data.wWordFormat==PVOC_IEEE_FLOAT)  */
    {
      float *frame =
        (float*) csound->Malloc(csound, data.nAnalysisBins * 2 * sizeof(float));

      for (i=1;;i++) {
        uint32_t j;
        if (1!=csound->PVOC_GetFrames(csound, inf, frame, 1)) break;
        for (j=0; j<data.nAnalysisBins*2; j++)
          fprintf(outf, "%s%g", (j==0 ? "" : ","), frame[j]);
        fprintf(outf, "\n");
        if (i%50==0 && outf!=stdout) csound->Message(csound,"%d\n", i);
      }
      csound->Free(csound,frame);
    }
/*     else { */
/*       double *frame =
            (double*) malloc(data.nAnalysisBins * 2 * sizeof(double)); */
/*       for (; i!=0; i--) { */
/*         int32_t j; */
/*         csound->PVOC_GetFrames(csound, inf, frame, 1); */
/*         for (j = 0; j<data.nAnalysisBins*2; j ++) */
/*           fprintf(outf, "%s%g", (j==0 ? "" : ","), frame[j]); */
/*         fprintf(outf, "\n"); */
/*       } */
/*       free(frame); */
/*     }       */
    csound->PVOC_CloseFile(csound, inf);
    fclose(outf);
    return 0;
}



/* module interface */

int32_t pv_export_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "pv_export", pv_export);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "pv_export",
                                      Str("translate PVOC analysis file "
                                          "to text form"));
    }
    return retval;
}
