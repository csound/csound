/*
    pvlook.c:

    Copyright (C) 1993 Richard Karpen
                  2006 John ffitch

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

/******************************************************************/
/* PVLOOK.C by Richard Karpen 1993 */
/*******************************************************************/

#include "std_util.h"
#include "pvfileio.h"
#include <limits.h>
#include <stdarg.h>

typedef struct PVLOOK_ {
    CSOUND  *csound;
    FILE    *outfd;
    int     linePos;
    int     printInts;
} PVLOOK;

static CS_NOINLINE CS_PRINTF2 void pvlook_print(PVLOOK *p, const char *fmt, ...)
{
    char    buf[1024];  /* hopefully this is enough... */
    va_list args;
    char    *s, *tmp;
    int     len;

    s = &(buf[0]);
    va_start(args, fmt);
    len = (int) vsnprintf(s, 1024, fmt, args);
    va_end(args);
 /* fprintf(p->outfd, "%s", s); */
    p->csound->MessageS(p->csound, CSOUNDMSG_ORCH, s);
    tmp = strrchr(s, '\n');
    if (tmp == NULL)
      p->linePos += len;
    else
      p->linePos = (len - (int) (tmp - s)) - 1;
    if (p->linePos >= 70) {
      p->csound->MessageS(p->csound, CSOUNDMSG_ORCH, "\n");
      p->linePos = 0;
    }
}

static CS_NOINLINE void pvlook_printvalue(PVLOOK *p, float x)
{
    if (!p->printInts)
      pvlook_print(p, " %.3f", x);
    else {
      int   n = (int) (x < 0.0f ? x - 0.5f : x + 0.5f);
      pvlook_print(p, " %d", n);
    }
}

static const char *pvlook_usage_txt[] = {
  Str_noop("pvlook is a program which reads a Csound pvanal's pvoc"),
  Str_noop("file and outputs frequency and magnitude trajectories for each "),
  Str_noop("of the analysis bins."),
    "",
  Str_noop("usage: pvlook [-bb X] [-eb X] [-bf X] [-ef X] [-i]  file > output"),
    "",
  Str_noop(" -bb X  begin at analysis bin X. Numbered from 1 [defaults to 1]"),
  Str_noop(" -eb X  end at analysis bin X [defaults to highest]"),
  Str_noop(" -bf X  begin at analysis frame X. Numbered from 1 [defaults to 1]"),
  Str_noop(" -ef X  end at analysis frame X [defaults to last]"),
  Str_noop(" -i     prints values as integers [defaults to floating point]"),
    "",
    NULL
};

static int pvlook(CSOUND *csound, int argc, char *argv[])
{
    int     i, j, k;
    int     fp;
    FILE    *outfd = stdout;
    float   *frames;
    int     numframes, framesize;
    unsigned int     firstBin, lastBin, numBins, lastFrame;
    int     firstFrame = 1;
    int     nchnls;
    PVOCDATA data;
    WAVEFORMATEX fmt;
    PVLOOK  p;

    p.csound = csound;
    p.outfd = outfd;
    p.linePos = 0;
    p.printInts = 0;

    {
      int   tmp = 0;
      csound->SetConfigurationVariable(csound, "msg_color", (void*) &tmp);
    }

    if (argc < 2) {
      for (i = 0; pvlook_usage_txt[i] != NULL; i++)
        csound->Message(csound, "%s\n", Str(pvlook_usage_txt[i]));
      return -1;
    }

    if ((fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt)) < 0) {
      csound->ErrorMsg(csound, Str("pvlook: Unable to open '%s'\n Does it exist?"),
                               argv[argc - 1]);
      return -1;
    }

    nchnls = fmt.nChannels;
    firstFrame = firstBin = 1;
    lastFrame = UINT_MAX;
    lastBin = data.nAnalysisBins;

    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "-bb"))
        firstBin = atoi(argv[++i]);
      if (!strcmp(argv[i], "-eb"))
        lastBin = atoi(argv[++i]);
      if (!strcmp(argv[i], "-bf"))
        firstFrame = atoi(argv[++i]);
      if (!strcmp(argv[i], "-ef"))
        lastFrame = atoi(argv[++i]);
      if (!strcmp(argv[i], "-i"))
        p.printInts = 1;
    }
    if (firstBin < 1U)
      firstBin = 1U;
    if (lastBin > (unsigned int) data.nAnalysisBins)
      lastBin = (unsigned int) data.nAnalysisBins;
    numBins = (lastBin - firstBin) + 1;
    if (firstFrame < 1)
      firstFrame = 1;
    numframes = (int) csound->PVOC_FrameCount(csound, fp);
    if (lastFrame > (unsigned int) numframes)
      lastFrame = (unsigned int) numframes;
    numframes = (lastFrame - firstFrame) + 1;

    pvlook_print(&p, "; File name\t%s\n", argv[argc - 1]);
    pvlook_print(&p, "; Channels\t%d\n", nchnls);
    pvlook_print(&p, "; Word Format\t%s\n",
                 data.wWordFormat == PVOC_IEEE_FLOAT ? "float" : "double");
    pvlook_print(&p, "; Frame Type\t%s\n",
                 data.wAnalFormat == PVOC_AMP_FREQ ? "Amplitude/Frequency" :
                 data.wAnalFormat == PVOC_AMP_PHASE ? "Amplitude/Phase" :
                                                      "Complex");
    switch (data.wSourceFormat) {
    case 1:     /* WAVE_FORMAT_PCM */
      pvlook_print(&p, "; Source format\t%dbit\n", (int) fmt.wBitsPerSample);
      break;
    default:
      pvlook_print(&p, "; Source format\tfloat\n");
      break;
    }
    pvlook_print(&p, "; Window Type\t%s",
                 data.wWindowType == PVOC_DEFAULT ? "Default" :
                 data.wWindowType == PVOC_HAMMING ? "Hamming" :
                 data.wWindowType == PVOC_HANN ? "vonHann" :
                 data.wWindowType == PVOC_KAISER ? "Kaiser" :
                 data.wWindowType == PVOC_RECT ? "Rectangular" :
                 "Custom");
    if (data.wWindowType == PVOC_KAISER)
      pvlook_print(&p, "(%f)", data.fWindowParam);
    pvlook_print(&p, "\n; FFT Size\t%d\n", (int) (data.nAnalysisBins - 1) * 2);
    pvlook_print(&p, "; Window length\t%d\n", (int) data.dwWinlen);
    pvlook_print(&p, "; Overlap\t%d\n", (int) data.dwOverlap);
    pvlook_print(&p, "; Frame align\t%d\n", (int) data.dwFrameAlign);
    pvlook_print(&p, "; Analysis Rate\t%f\n", data.fAnalysisRate);

    if (numBins > 0U && numframes > 0) {
/*    pvlook_print(&p, "; Bins in Analysis: %d\n", (int) data.nAnalysisBins); */
      pvlook_print(&p, "; First Bin Shown: %d\n", (int) firstBin);
      pvlook_print(&p, "; Number of Bins Shown: %d\n", (int) numBins);
/*    pvlook_print(&p, "; Frames in Analysis: %ld\n",
                   (long) csound->PVOC_FrameCount(csound, fp)); */
      pvlook_print(&p, "; First Frame Shown: %d\n", (int) firstFrame);
      pvlook_print(&p, "; Number of Data Frames Shown: %d\n", (int) numframes);
      framesize = data.nAnalysisBins * 2 * sizeof(float);
      frames = (float*) csound->Malloc(csound, framesize * numframes);
      for (j = 1; j < firstFrame; j++)
        csound->PVOC_GetFrames(csound, fp, frames, 1);  /* Skip */
      csound->PVOC_GetFrames(csound, fp, frames, numframes);
      for (k = (firstBin - 1); k < (int) lastBin; k++) {
        pvlook_print(&p, "\nBin %d Freqs.\n", k + 1);
        for (j = 0; j < numframes; j++) {
          pvlook_printvalue(&p, frames[((j * data.nAnalysisBins) + k) * 2 + 1]);
        }
        if (p.linePos != 0)
          pvlook_print(&p, "\n");
        pvlook_print(&p, "\nBin %d Amps.\n", k + 1);
        for (j = 0; j < numframes; j++) {
          if (!p.printInts)
            pvlook_printvalue(&p, frames[((j * data.nAnalysisBins) + k) * 2]);
          else
            pvlook_printvalue(&p, frames[((j * data.nAnalysisBins) + k) * 2]
                              * (float) csound->Get0dBFS(csound));
        }
        if (p.linePos != 0)
          pvlook_print(&p, "\n");
      }
      csound->Free(csound, frames);
    }
    pvlook_print(&p, "\n");

    csound->PVOC_CloseFile(csound, fp);
    if (outfd != stdout)
      fclose(outfd);

    return 0;
}

/* module interface */

int pvlook_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "pvlook", pvlook);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "pvlook",
                    "Prints information about PVOC analysis files");
    }
    return retval;
}

