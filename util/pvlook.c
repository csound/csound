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

static int pvlook(CSOUND *csound, int argc, char *argv[])
{
    int     i, j, k;
    int     fp;
    FILE    *outfd = stdout;
    float   *frame;
    int     numframes, framesize;
    int     l, FuncSize;
    unsigned int     firstBin, lastBin, numBins, lastFrame;
    int     printInts=1;
    int     firstFrame = 1;
    int     nchnls;
    PVOCDATA data;
    WAVEFORMATEX fmt;

    if (argc < 2) {
      csound->Message(csound,
               "pvlook is a program which reads a Csound pvanal's pvoc.n "
               "file and outputs frequency and magnitude trajectories for each "
               "of the analysis bins.\n");
      csound->Message(csound,
              "usage: pvlook [-bb X] [-eb X] [-bf X] [-ef X] [-i X]  "
              "file > output\n");
      csound->Message(csound,
              " -bb X  begin at anaysis bin X. Numbered from 1 "
              "[defaults to 1]\n");
      csound->Message(csound,
              " -eb X  end at anaysis bin X [defaults to highest]\n");
      csound->Message(csound,
              " -bf X  begin at anaysis frame X. Numbered from 1 "
              "[defaults to 1]\n");
      csound->Message(csound,
              " -ef X  end at anaysis frame X [defaults to last]\n");
      csound->Message(csound,
              " -i  prints values as integers [defaults to "
              "floating point]\n");
      return -1;
    }

    if ((fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt)) < 0) {
      csound->Message(csound, "pvlook: Unable to open '%s'\n Does it exist?",
                              argv[argc - 1]);
      return -1;
    }

    nchnls = fmt.nChannels;
    firstFrame = firstBin = 1;
    lastFrame = UINT_MAX;
    lastBin = data.nAnalysisBins;
    frame = (float*)csound->Malloc(csound, 2*data.nAnalysisBins*sizeof(float));

    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "-bb"))  firstBin = atoi(argv[++i]);
      if (!strcmp(argv[i], "-eb"))  lastBin = atoi(argv[++i]);
      if (!strcmp(argv[i], "-bf"))  firstFrame = atoi(argv[++i]);
      if (!strcmp(argv[i], "-ef"))  lastFrame = atoi(argv[++i]);
      if (!strcmp(argv[i], "-i"))   printInts = 0;
    }
    numframes = (lastFrame - firstFrame) + 1;
    numBins = (lastBin - firstBin) + 1;
    for (l = 2; l < numframes; l *= 2);
    FuncSize = l + 1;

    csound->Message(csound, "; Channels\t%d\n", nchnls);
    csound->Message(csound, "; Word Format\t%s\n",
                    data.wWordFormat==PVOC_IEEE_FLOAT?"float":"double");
    csound->Message(csound, "; Frame Type\t%s\n",
                    data.wAnalFormat==PVOC_AMP_FREQ?"Amplitude/Frequency":
                    data.wAnalFormat==PVOC_AMP_PHASE?"Amplitude/Phase":"Complex");
    csound->Message(csound, "; Source format\t%s\n",
                    data.wSourceFormat==STYPE_16?"16bit":
                    data.wSourceFormat==STYPE_24?"24bit":
                    data.wSourceFormat==STYPE_32?"32bit":
                    "float");
    csound->Message(csound, "; Window Type\t%s",
                    data.wWindowType==PVOC_DEFAULT?"Default":
                    data.wWindowType==PVOC_HAMMING?"Hamming":
                    data.wWindowType==PVOC_HANN?"vonHann":
                    data.wWindowType==PVOC_KAISER?"Kaiser":
                    data.wWindowType==PVOC_RECT?"Rectangular":
                    "Custom");
    if (data.wWindowType==PVOC_KAISER)
      csound->Message(csound, "(%f)",data.fWindowParam);
    csound->Message(csound, "\n; FFT Size\t%d\n",data.nAnalysisBins);
    csound->Message(csound, "; Window length\t%d\n",data.dwWinlen);
    csound->Message(csound, "; Overlap\t%d\n",data.dwOverlap);
    csound->Message(csound, "; Frame align\t%d\n",data.dwFrameAlign);
    csound->Message(csound, "; Analysis Rate\t%f\n",data.fAnalysisRate);

    framesize = data.nAnalysisBins * 2 *sizeof(float);

/*     csound->Message(csound, "; Bins in Analysis: %d\n", data.nAnalysisBins); */
    csound->Message(csound, "; First Bin Shown: %d\n", firstBin);
    csound->Message(csound, "; Number of Bins Shown: %d\n", numBins);
/*     csound->Message(csound, "; Frames in Analysis: %ld\n", */
/*                             ((phdr->dataBsize / 4) / framesize)); */
/*     csound->Message(csound, "; First Frame Shown: %d\n", firstFrame); */
/*     csound->Message(csound, "; Number of Data Frames Shown: %d\n", numframes); */
    if (printInts != 0) {
      for (k = 0; k<numBins; k++) {
        csound->Message(csound, "\nBin %d Freqs.", firstBin + k);
        for (j=1; j<firstFrame; j++)
          csound->PVOC_GetFrames(csound,fp,frame,1); /* Skip */
        for (j=firstFrame; j<=lastFrame; j++) {
          if (csound->PVOC_GetFrames(csound,fp,frame,1)<0) break;
          csound->Message(csound, "%.3f ", frame[(firstBin + k)*2]);
        }
        csound->Message(csound, "\n");
        csound->PVOC_CloseFile(csound, fp);
        fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt);
        for (j=1; j<firstFrame; j++)
          csound->PVOC_GetFrames(csound,fp,frame,1); /* Skip */
        csound->Message(csound, "\nBin %d Amps. ", firstBin + k);
        for (j=firstFrame; j<=lastFrame; j++) {
          if (csound->PVOC_GetFrames(csound,fp,frame,1)!=1) break;
          csound->Message(csound, "%.3f ", frame[1+(firstBin + k)*2]);
        }
        csound->Message(csound, "\n");
        csound->PVOC_CloseFile(csound, fp);
        fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt);
      }
    }
    else {
      for (k = 0; k<numBins; k++) {
        fprintf(outfd, "\nBin %d Freqs.", firstBin+k);
        csound->Message(csound, "\nBin %d Freqs.", firstBin + k);
        for (j=1; j<firstFrame; j++)
          csound->PVOC_GetFrames(csound,fp,frame,1); /* Skip */
        for (j=firstFrame; j<=lastFrame; j++) {
          if (csound->PVOC_GetFrames(csound,fp,frame,1)!=1) break;
          csound->Message(csound, "%d ", (int)frame[(firstBin + k)*2]);
        }
        csound->Message(csound, "\n");
        csound->PVOC_CloseFile(csound, fp);
        fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt);
        csound->Message(csound, "\nBin %d Amps. ", firstBin + k);
        for (j=1; j<firstFrame; j++)
          csound->PVOC_GetFrames(csound,fp,frame,1); /* Skip */
        for (j=firstFrame; j<=lastFrame; j++) {
          if (csound->PVOC_GetFrames(csound,fp,frame,1)!=1) break;
          csound->Message(csound, "%d ", (int)frame[1+(firstBin + k)*2]);
        }
        csound->Message(csound, "\n");
        csound->PVOC_CloseFile(csound, fp);
        fp = csound->PVOC_OpenFile(csound, argv[argc - 1], &data, &fmt);
      }
    }
    
    csound->PVOC_CloseFile(csound,fp);
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

