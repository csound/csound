/*  
    pvlook.c:

    Copyright (C) 1993 Richard Karpen

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
#include <stdio.h>
#include <stdlib.h>
#include "cs.h"

#define PVSHORT 2       /* for .format .. 16 bit linear data */
#define PVMYFLT 4       /* for .format .. 32 bit float data */

#define PVDFLTBYTS 4

#define PVMAGIC 517730  /* look at it upside-down, esp on a 7-seg display */

#define PVDFLTBYTS 4

typedef struct pvstruct
    {
    long        magic;                  /* magic number to identify */
    long        headBsize;              /* byte offset from start to data */
    long        dataBsize;              /* number of bytes of data */
    long        dataFormat;             /* (int) format specifier */
    float       samplingRate;           /* of original sample */
    long        channels;               /* (int) mono/stereo etc */
    long        frameSize;              /* size of FFT frames (2^n) */
    long        frameIncr;              /* # new samples each frame */
    long        frameBsize;             /* bytes in each file frame */
    long        frameFormat;            /* (int) how words are org'd in frms */
    float       minFreq;                /* freq in Hz of lowest bin (exists) */
    float       maxFreq;                /* freq in Hz of highest (or next) */
    long        freqFormat;             /* (int) flag for log/lin frq */
    char        info[PVDFLTBYTS];       /* extendable byte area */
    } PVSTRUCT;

int pvlook(int argc, char *argv[])
{
    int i, j, k, fd;
    FILE *fp, *outfd = stdout;
    float *pvdataF, *pvdataA;
    register PVSTRUCT *phdr;
    int numframes, framesize;
    int l, FuncSize,num=0;
    long int m;
    int firstBin, lastBin, numBins, lastFrame;
    int printInts=1;
    int firstFrame = 1;

    phdr =  (PVSTRUCT *)malloc( sizeof(PVSTRUCT)) ;
    pvdataF = (float *) malloc(sizeof(float));
    pvdataA = (float *) malloc(sizeof(float));

    if ( argc == 1 ) {
      fprintf( stderr,"pvlook is a program which reads a Csound pvanal's pvoc.n "
               "file and outputs frequency and magnitude trajectories for each "
               "of the analysis bins. \n");
      fprintf(stderr, "usage: pvlook [-bb X] [-eb X] [-bf X] [-ef X] [-i]  "
              "file > output\n" ) ;
      fprintf( stderr,
               "        -bb X  begin at anaysis bin X. Numbered from 1 "
               "[defaults to 1]\n" ) ;
      fprintf( stderr,
               "        -eb X  end at anaysis bin X [defaults to highest]\n" ) ;
      fprintf( stderr,
               "        -bf X  begin at anaysis frame X. Numbered from 1 "
               "[defaults to 1]\n" ) ;
      fprintf( stderr,
               "        -ef X  end at anaysis frame X [defaults to last]\n" ) ;
      fprintf( stderr,
               "        -i prints values as integers [defaults to "
               "floating point]\n" ) ;
      exit( -1 ) ;
    }

    if ( ( fd = open( argv[argc-1], O_RDONLY) ) < 0 ) {
      fprintf( stderr, "pvlook: Unable to open '%s'\n Does it exist?",
               argv[argc-1] ) ;
      exit( -1 ) ;
    }
    if ( ( fp = fdopen( fd, "r" ) ) == NULL ) {
      fprintf( stderr, "pvlook: Unable to fdopen '%s'\n", argv[argc-1] ) ;
      exit( -1 ) ;
    }

    rewind( fp ) ;
    fread(phdr, 1, sizeof(PVSTRUCT), fp);

    if (phdr->magic != PVMAGIC) {
      fprintf( stderr, "'%s' is not a pvoc file\n", argv[argc-1] ) ;
      exit( -1 ) ;
    }

    framesize = phdr->frameSize+2;
    numframes =((phdr->dataBsize/4) / framesize);
    firstBin = 1;
    lastBin = (framesize/2);
    lastFrame = numframes;

    for ( i = 1 ; i < argc ; i++ ) {
      if (!strcmp( argv[i], "-bb") )
        firstBin = atoi( argv[++i] ) ;
      if (!strcmp( argv[i], "-eb") )
        lastBin = atoi( argv[++i] );
      if (!strcmp( argv[i], "-bf") )
        firstFrame = atoi( argv[++i] ) ;
      if (!strcmp( argv[i], "-ef") )
        lastFrame = atoi( argv[++i] ) ;
      if (!strcmp( argv[i], "-i") )
        printInts = 0;

    }
    numframes = (lastFrame - firstFrame) + 1;
    numBins = (lastBin - firstBin) + 1;
    for (l = 2; l < numframes; l*=2);
    FuncSize = l+1;

    fprintf(outfd,"; Bins in Analysis: %d\n", framesize/2);
    fprintf(outfd,"; First Bin Shown: %d\n", firstBin);
    fprintf(outfd,"; Number of Bins Shown: %d\n", numBins);
    fprintf(outfd,"; Frames in Analysis: %ld\n",
            ((phdr->dataBsize/4) / framesize));
    fprintf(outfd,"; First Frame Shown: %d\n", firstFrame);
    fprintf(outfd,"; Number of Data Frames Shown: %d\n", numframes);

    if (outfd != stdout) {
      printf("; Bins in Analysis: %d\n", framesize/2);
      printf("; First Bin Shown: %d\n", firstBin);
      printf("; Number of Bins Shown: %d\n", numBins);
      printf("; Frames in Analysis: %d\n", ((phdr->dataBsize/4) / framesize));
      printf("; First Frame Shown: %d\n", firstFrame);
      printf("; Number of Data Frames Shown: %d\n", numframes);
    }
    rewind( fp ) ;
    if (printInts!=0) {
      for (k=0; k < numBins*2; k+=2) {
/*         if (!POLL_EVENTS()) exit(1); */
        j=((firstBin*2)-1) + k;
        fprintf(outfd,"\nBin %d Freqs.", firstBin+k/2);
        if (outfd != stdout) {
          printf("\nBin %d Freqs.", firstBin+k/2);
        }
        for (i = firstFrame-1; i < lastFrame; i++) {
/*           if (!POLL_EVENTS()) exit(1); */
          m = 56 + ((j + (i * framesize) ) * 4);
          fseek(fp, m, SEEK_SET);
          num+=fread( pvdataF, sizeof(float), 1, fp );
          fprintf(outfd,"%.3f ", *pvdataF);
        }
        fprintf(outfd,"\n");
        j=((firstBin*2)-2) + k;
        fprintf(outfd,"\nBin %d Amps. ", firstBin+k/2);
        if (outfd != stdout) {
          printf("\nBin %d Amps. ", firstBin+k/2);
        }
        for (i = firstFrame-1; i < lastFrame; i++) {
/*           if (!POLL_EVENTS()) exit(1); */
          m =  56 + ((j + (i * framesize) ) * 4);
          fseek(fp, m, SEEK_SET);
          num+=fread( pvdataA, sizeof(float), 1, fp );
          fprintf(outfd,"%.3f ", *pvdataA);
        }
        fprintf(outfd,"\n");
      }
    }
    else {
      for (k=0; k < numBins*2; k+=2) {
/*         if (!POLL_EVENTS()) exit(1); */
        j=((firstBin*2)-1) + k;
        fprintf(outfd,"\nBin %d Freqs.", firstBin+k/2);
        if (outfd!=stdout) printf("\nBin %d Freqs.", firstBin+k/2);
        for (i = firstFrame-1; i < lastFrame; i++) {
/*           if (!POLL_EVENTS()) exit(1); */
          m = 56 + ((j + (i * framesize) ) * 4);
          fseek(fp, m, SEEK_SET);
          num+=fread( pvdataF, sizeof(float), 1, fp );
          fprintf(outfd,"%d ", (short)*pvdataF);
        }
        fprintf(outfd,"\n");

        j=((firstBin*2)-2) + k;
        fprintf(outfd,"\nBin %d Amps. ", firstBin+k/2);
        if (outfd!=stdout) printf("\nBin %d Amps. ", firstBin+k/2);
        for (i = firstFrame-1; i < lastFrame; i++) {
/*           if (!POLL_EVENTS()) exit(1); */
          m =  56 + ((j + (i * framesize) ) * 4);
          fseek(fp, m, SEEK_SET);
          num+=fread( pvdataA, sizeof(float), 1, fp );
          fprintf(outfd,"%d ", (short)*pvdataA);
        }
        fprintf(outfd,"\n");
      }
    }

    fclose( fp ) ;
    if (outfd != stdout) fclose(outfd);
    free(pvdataF);
    free(pvdataA);
    free(phdr);
    return 0;
}

