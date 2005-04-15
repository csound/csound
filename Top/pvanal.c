/*
    pvanal.c:

    Copyright (C) 1991, 1994  Dan Ellis, Dave Madole, John ffitch

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

/************************************************************************/
/*  pva.c           (after pvanal.c)                                    */
/*  Frequency Domain Analysis                                           */
/*  Takes a time domain soundfile and converts it into a file of        */
/*  This version just calculates pure FFT rather than PVOC's phi-dot    */
/*  More importantly, it reads from disk rather than using so much core */
/*  dpwe  14feb91                                                       */
/*  madole added usage call and Stasks() and transport state checks     */
/*  for macintosh AUgust 28, 1996                                       */
/************************************************************************/
/************************************************************************/

#include "cs.h"
#include "cwindow.h"
#include "soundio.h"
#include "dsputil.h"
#include "pvoc.h"
#include "pvxanal.h"
#include <ctype.h>
#include <sndfile.h>

                 /* prototype arguments */
extern int pvxanal(SOUNDIN *, SNDFILE *, const char *, long, long,
                   long, long, long, int, int);
static long takeFFTs(SOUNDIN *inputSound, PVSTRUCT *outputPVH,
                     SNDFILE *sndfd, int fftd, long oframeEst);
static void quit(ENVIRON *, char *msg);
static void PrintBuf(MYFLT *buf, long size, char *msg);

#define MINFRMMS        20      /* frame defaults to at least this many ms */
#define MAXFRMPTS       65536
#define MINFRMPTS       16      /* limits on fft size */
#define OVLP_DEF        4       /* default frame overlap factor */
#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

int         debug = 0;          /* tweaked inside ! */
char        *programName = "PVANAL";  /* TEMP FOR pvoc.c ******/

static  long frameSize  = 0;        /* size of FFT frames */
static  long frameIncr  = 0;        /* step between successive frames */
static  long fftfrmBsiz = 0;        /* bytes of fft output frame      */
static  WINDAT   dwindow;
/* static  MYFLT    max = 0.0; */
static  int      cnt = 0;
static  int      latch = 200;
static  int      verbose = 0;
static  FILE*    trfil = NULL; /* was stdout */
static  int WindowType = 1;

#ifdef mills_macintosh
#include "MacTransport.h"
#endif

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            quit(csound,MSG);

int pvanal(int argc, char **argv)
{
    ENVIRON *csound = &cenviron;
    PVSTRUCT *pvh;
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
    int     ofd, err, channel = 1;
    int     ovlp = 0;           /* number of overlapping windows to have */
    SOUNDIN  *p;                /* space allocated by SAsndgetset() */

    MYFLT    beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    long     oframeEst = 0, oframeAct;  /* output frms estimated, & actual */
    long     Estdatasiz;
    long     nb;
    char     *ext;

    /* must set this for 'standard' behaviour when analysing
       (assume re-entrant Csound) */
    dbfs_init(csound, DFLT_DBFS);
    trfil = stdout;
    O.displays = 0;
    WindowType = 1;
    if (!(--argc))
      quit(csound,Str("insufficient arguments"));
      do {
        char *s = *++argv;
        if (*s++ == '-')
          switch (*s++) {
          case 'j': FIND("");
            while (*s++); s--;
            break;
          case 's': FIND(Str("no sampling rate"));
#if defined(USE_DOUBLE)
            sscanf(s,"%lf",&sr);
#else
            sscanf(s,"%f",&sr);
#endif
            break;
          case 'c':  FIND(Str("no channel"));
            sscanf(s,"%d",&channel);
            break;
          case 'b':  FIND(Str("no begin time"));
#if defined(USE_DOUBLE)
            sscanf(s,"%lf",&beg_time);
#else
            sscanf(s,"%f",&beg_time);
#endif
            break;
          case 'd':  FIND(Str("no duration time"));
#if defined(USE_DOUBLE)
            sscanf(s,"%lf",&input_dur);
#else
            sscanf(s,"%f",&input_dur);
#endif
            break;
          case 'H':
            {
              int c = *s++;
              if (c=='M' || c=='\0') WindowType = 0;
            }
            break;
          case 'n':  FIND(Str("no framesize"));
            sscanf(s,"%ld",&frameSize);
            if (frameSize < MINFRMPTS || frameSize > MAXFRMPTS) {
              sprintf(errmsg,Str("frameSize must be between %d &%d\n"),
                      MINFRMPTS, MAXFRMPTS);
              quit(csound,errmsg);
            }
            if (frameSize < 1L || (frameSize & (frameSize - 1L)) != 0L) {
              sprintf(errmsg,Str("pvanal: frameSize must be 2^r"));
              quit(csound,errmsg);
            }
            break;
          case 'w':  FIND(Str("no windfact"));
            sscanf(s,"%d",&ovlp);
            break;
          case 'h':  FIND(Str("no hopsize"));
            sscanf(s,"%ld",&frameIncr);
            break;
          case 'g':  O.displays = 1;
            break;
          case 'G':  FIND(Str("no latch"));
            sscanf(s,"%d",&latch);
            O.displays = 1;
            break;
          case 'V':  FIND(Str("no output file for trace"));
            trfil = fopen(s,"w");
            if (trfil==NULL) quit(csound,Str("Failed to open text file"));
            printf(Str("Writing text form to file %s\n"), s);
          case 'v':
            verbose = 1;
            break;
          default:   quit(csound,Str("unrecognised switch option"));
          }
        else break;
      } while (--argc);

      if (argc !=  2) quit(csound,Str("illegal number of filenames"));
      infilnam = *argv++;
      outfilnam = *argv;

    if (ovlp && frameIncr)
      quit(csound,Str("pvanal cannot have both -w and -h"));
    /* open sndfil, do skiptime */
    channel = ALLCHNLS; /* we can analyse up to 8 chans with pvxanal! */
    if ((infd = csound->SAsndgetset(csound, infilnam, &p, &beg_time,
                                    &input_dur, &sr, channel)) == NULL) {
      sprintf(errmsg,Str("error while opening %s"), retfilnam);
      quit(csound,errmsg);
    }
    sr = (MYFLT)p->sr;
    /* setup frame size etc according to sampling rate */
    if (frameSize == 0) {           /* not specified on command line */
      int target;
      target = (int)(sr * (MYFLT)MINFRMMS / FL(1000.0));
      frameSize = MAXFRMPTS;      /* default frame size is > MINFRMMS msecs */
      while ((frameSize>>1) >= target && frameSize > MINFRMPTS)
        frameSize >>= 1;        /* divide down until just larger */
    }
    if (ovlp == 0 && frameIncr == 0) {
      ovlp = OVLP_DEF;            /* default overlap */
      frameIncr = frameSize / ovlp;
    }
    else if (ovlp == 0)
      ovlp = frameSize/frameIncr;
    else frameIncr = frameSize/ovlp;

    if (ovlp < 2 || ovlp > 16) {
      csound->Message(csound,Str("pvanal: %d is a bad window overlap index\n"),
                 ovlp);
      exit(1);
    }
    oframeEst = (p->getframes - frameSize/2) / frameIncr;
    csound->Message(csound,"%ld infrsize, %ld infrInc\n", frameSize, frameIncr);
    csound->Message(csound,Str("%ld output frames estimated\n"), oframeEst);
    
    ext = strrchr(outfilnam,'.');
    /* Look for .pvx extension in any case */
    if (ext != NULL && ext[0]=='.' && tolower(ext[1]) == 'p' &&
        tolower(ext[2]) == 'v' && tolower(ext[3]) == 'x' && ext[4] == '\0') {
      /* even for old pvoc file, is absence of extension OK? */
      if (p->nchanls > MAXPVXCHANS) {
        printf(Str("pvxanal - source has too many channels: Maxchans = %d.\n"),
               MAXPVXCHANS);
        return 1;
      }
      csound->Message(csound,Str("pvanal: creating pvocex file\n"));
      /* handle all messages in here, for now */
      if (pvxanal(p,infd,outfilnam,p->sr,p->nchanls,frameSize,frameIncr,
                  frameSize*2,PVOC_HAMMING,verbose))
        csoundDie(&cenviron, Str("error generating pvocex file.\n"));
    }
    else {
      fftfrmBsiz = sizeof(MYFLT) * 2 * (frameSize/2 + 1);
      Estdatasiz = oframeEst * fftfrmBsiz;
      /* alloc & fill PV hdrblk */
      if ((err = PVAlloc(&pvh, Estdatasiz, PVMYFLT, sr, p->nchanls, frameSize,
                         frameIncr, fftfrmBsiz, PVPVOC, FL(0.0), sr/FL(2.0),
                         PVLIN, 4))) {
        csound->Message(csound, "pvanal: %s\n", PVErrMsg(err));
        exit(1);
      }
      if ((ofd = openout(outfilnam, 1)) < 0)     /* open the output PV file */
        quit(csound,Str("cannot create output file"));
      /* & wrt hdr into the file */
      if ((nb = write(ofd,(char *)pvh,(int)pvh->headBsize)) < pvh->headBsize)
        quit(csound,Str("cannot write header"));

      dispinit();
      if (verbose) {
        fprintf(trfil, "Size=%ld Format=%ld Rate=%g Channels=%ld\n",
                pvh->dataBsize/pvh->frameBsize, pvh->dataFormat,
                pvh->samplingRate,pvh->channels);
        fprintf(trfil, "FrameSize=%ld FrameInc=%ld MinFreq=%g MaxFreq=%g\n",
                pvh->frameSize, pvh->frameIncr, pvh->minFreq, pvh->maxFreq);
        fprintf(trfil, "LogLin=%ld\n\n", pvh->freqFormat);
      }
      oframeAct = takeFFTs(p, pvh, infd, ofd, oframeEst);
      dispexit();
      csound->Message(csound,Str("%ld output frames written\n"), oframeAct);
    }
    sf_close(infd);
    /*     close(ofd); */
#if !defined(mills_macintosh)
    exit(0);
#endif
    return (-1);
}

static void quit(ENVIRON *csound, char *msg)
{
    csound->Message(csound, "pvanal error: %s\n", msg);
    csound->Message(csound,
                    Str("Usage: pvanal [-n<frmsiz>] [-w<windfact> | "
                        "-h<hopsize>] [-g | -G<latch>] [-v | -V txtfile] "
                        "inputSoundfile outputFFTfile\n"));
    exit(0);
}

/*    if (debug) */
/*      printf("Framesiz %ld, framInc %ld\n",frameSize, frameIncr); */
    /* If we dealt with frames that hit the ends properly, we'd have */
    /*         (size/sizeof(short))/frameIncr frames (>= 1/2 inside file) */
/*    frameWords = frameSize + 2L; */ /* each frame has Mag & phase for n/2 +1 */
/*    if (inputSound->dataBsize != SF_UNK_LEN) */
/*      printf("Est. frames %ld\n", inputSound->dataBsize/(dsize*frameIncr)); */

/*      chans = 1;  */    /* will write # chans into PV header tho' only mono */
/*
 * takeFFTs
 *  Go through the (mono) input sound frame by frame and find the
 *  magnitude and phase change for a string of FFT bins
 */

static long takeFFTs(
    SOUNDIN         *p,
    PVSTRUCT        *outputPVH,
    SNDFILE         *infd,
    int             ofd,
    long            oframeEst)
{
    ENVIRON *csound = &cenviron;
    long    i = -1, nn, read_in;
    MYFLT   *inBuf, *tmpBuf, *oldInPh, *winBuf;
    MYFLT   *v;
    MYFLT   sampRate = (MYFLT)p->sr;  /* not really */
    long    fsIndepVals = (frameSize/2L)+1L;
    MYFLT   *fp1, *fp2;
    IGN(outputPVH);

    inBuf   = (MYFLT *)MakeBuf(frameSize * 2L);
    tmpBuf  = MakeBuf(frameSize + 2L);
    v       = MakeBuf(frameSize);
    oldInPh = MakeBuf(frameSize);
    winBuf  = MakeHalfWin(frameSize,FL(1.0),WindowType);

    dispset(&dwindow,v,frameSize/2,"pvanalwin",0,"PVANAL");

                             /* initially, clear first half of buffer .. */
    for (fp1=inBuf, nn=frameSize/2; nn--; )
      *fp1++ = FL(0.0);
                             /* .. and read in second half from file */
    if ((read_in = csound->getsndin(csound, infd, fp1, (frameSize/2), p))
        < frameSize/2)
      csound->Die(csound, Str("insufficient sound for analysis"));
    for (nn = read_in; nn--; )
      /* IV - Jul 11 2002 */
      *fp1++ *= cenviron.dbfs_to_float;     /* normalize the samples read in */
    oframeEst -= 1;
    if (!O.displays && !verbose) printf(Str("frame: "));
    do {
      if (((++i)%20)==0)
        if (!O.displays && !verbose) {
          printf("%ld ", i); fflush(stdout); }
      /*        copy the current frame */
      for (fp1=inBuf, fp2=tmpBuf, nn=frameSize; nn--; )
        *fp2++ = *fp1++;
      /*    PrintBuf(tmpBuf, frameSize, "floated");  */
      ApplyHalfWin(tmpBuf,winBuf,frameSize);
      /*    PrintBuf(tmpBuf, frameSize, "windo'd"); */
      csoundRealFFT(&cenviron, tmpBuf, (int) frameSize);
      tmpBuf[frameSize] = tmpBuf[1];
      tmpBuf[1] = tmpBuf[frameSize + 1L] = FL(0.0);
      /*    PrintBuf(tmpBuf, frameSize, X_761,"fft'd"); */
      Rect2Polar(tmpBuf, fsIndepVals);
      /*    PrintBuf(tmpBuf, frameSize, "toPolar"); */
      UnwrapPhase(tmpBuf, fsIndepVals, oldInPh);
      /*    PrintBuf(tmpBuf, frameSize, "unWrapped"); */
      PhaseToFrq(tmpBuf, fsIndepVals, (MYFLT)frameIncr, (MYFLT)sampRate);
      /*    PrintBuf(tmpBuf, frameSize, "toFrq"); */
      /* write straight out, just the indep vals */
      write(ofd, (char *)tmpBuf, fftfrmBsiz);
      if (verbose) {
        char msg[20];
        sprintf(msg, Str("Frame %ld"), i);
        PrintBuf(tmpBuf, frameSize, msg);
      }
      if (O.displays) {
        int j;
        for (j=0; j<frameSize; j += 2) v[j/2] = tmpBuf[j];
        sprintf(dwindow.caption,"%ld",i);
        display(&dwindow);
        if (dwindow.oabsmax > dwindow.absmax) cnt++; else cnt = 0;
        if (cnt>latch) dwindow.oabsmax = dwindow.absmax;
      }
      if (!read_in)            /* if previous read had hit EOF, we're done */
        break;               /* mv conts fwrd by frameIncr, rd more pnts */
      for (fp1=inBuf+frameIncr, fp2=inBuf, nn=frameSize-frameIncr; nn--; )
        *fp2++ = *fp1++;     /* getsndin pads with zeros if not complete */
      read_in = csound->getsndin(csound, infd, inBuf+frameSize-frameIncr,
                                 frameIncr, p);
      for (fp1 = inBuf+frameSize-frameIncr, nn = read_in; nn--; )
        /* IV - Jul 11 2002 */
        *fp1++ *= csound->dbfs_to_float;    /* normalize samples just read in */
      /* debug = 0; */
      if (!csoundYield(csound)) break;
    } while (i < oframeEst);
    if (!O.displays && !verbose) printf("%ld\n",i);
    if (i < oframeEst)
      printf(Str("\tearly end of file\n"));
    return((long)i + 1);
}

#define DBGPTS 8
static void PrintBuf(MYFLT *buf, long size, char *msg)
{
    int   i;
/*      if (!debug) return;   */
    fprintf(trfil,"%s:",msg);
    for (i=0; i<size; ++i)
      fprintf(trfil,"%7.2f ",buf[i]);
    fprintf(trfil, "\n");
}

