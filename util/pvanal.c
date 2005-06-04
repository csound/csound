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

#include "csdl.h"
#include "cwindow.h"
#include "soundio.h"
#include "pvoc.h"
#include "pvxanal.h"
#include <math.h>
#include <ctype.h>
#include <sndfile.h>

/* prototype arguments */

#ifdef oldcode
static long takeFFTs(ENVIRON *csound, SOUNDIN *inputSound, PVSTRUCT *outputPVH,
                     SNDFILE *sndfd, FILE *ofd, long oframeEst, long frameSize,
                     int WindowType, long frameIncr, long fftfrmBsiz,
                     int verbose);
#endif
static int  quit(ENVIRON *, char *msg);

#define MINFRMMS        20      /* frame defaults to at least this many ms */
#define MAXFRMPTS       65536
#define MINFRMPTS       16      /* limits on fft size */
#define OVLP_DEF        4       /* default frame overlap factor */
#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            return quit(csound, MSG);

#ifdef oldcode
static MYFLT *MakeBuf(ENVIRON *csound, long size)
{
    MYFLT *res, *p;
    long  i;
    p = res = (MYFLT *) csound->Malloc(csound, size * sizeof(MYFLT));
    for (i = 0; i < size; ++i) *p++ = FL(0.0);
    return res;
}

static void FillHalfWin(MYFLT *wBuf, long size, MYFLT max, int hannq)
    /* 1 => hanning window else hamming */
{
    MYFLT       a, b;
    long        i;
    if (hannq)  a = FL(0.50), b = FL(0.50);
    else        a = FL(0.54), b = FL(0.46);
    /* NB: size/2 + 1 long - just indep terms */
    size /= 2;              /* to fix scaling */
    for (i = 0; i <= size; ++i)
      wBuf[i] = max * (a - b * (MYFLT) cos(PI * (MYFLT) i / (MYFLT) size));
}

static MYFLT *MakeHalfWin(ENVIRON *csound, long size, MYFLT max, int hannq)
                /* Effective window size (altho only s/2+1 alloc'd */
                /* 1 => hanning window else hamming */
{   /* NB: size/2 + 1 long - just indep terms */
    MYFLT *wBuf = (MYFLT*) csound->Malloc(csound, (size/2 + 1) * sizeof(MYFLT));
    FillHalfWin(wBuf, size, max, hannq);
    return wBuf;
}

static void ApplyHalfWin(MYFLT *buf, MYFLT *win, long len)
{   /* Window only store 1st half, is symmetric */
    long j, lenOn2 = (len/2L);
    for (j = lenOn2 + 1; j--; )                 *buf++ *= *win++;
    for (j = len - lenOn2 - 1, win--; j--; )    *buf++ *= *--win;
}

static void Rect2Polar(MYFLT *buffer, long size)
{
    long        i;
    MYFLT       *real, *imag;
    double      re, im;
    MYFLT       mag;

    real = buffer;
    imag = buffer + 1;
    for (i = 0; i < size; ++i) {
      re = (double) real[2L * i];
      im = (double) imag[2L * i];
      real[2L * i] = mag = (MYFLT) sqrt(re*re + im*im);
      if (mag == FL(0.0))
        imag[2L * i] = FL(0.0);
      else
        imag[2L * i] = (MYFLT) atan2(im, re);
    }
}

#define MMmaskPhs(p,q,s) /* p is pha, q is as int, s is 1/PI */ \
    q = (int) (s * p);                                          \
    p -= PI_F * (MYFLT) ((int) ((q + ((q >= 0) ? (q & 1) : -(q & 1)))));

static void UnwrapPhase(MYFLT *buf, long size, MYFLT *oldPh)
{
    long    i;
    MYFLT   *pha;
    MYFLT   p, oneOnPi;
    int     z;

    pha = buf + 1;
    oneOnPi = FL(1.0) / PI_F;
    for (i = 0; i < size; ++i) {
      p = pha[2L * i];
      p -= oldPh[i];              /* find change since last frame */
      MMmaskPhs(p, z, oneOnPi);
      /* MmaskPhs(p); */
      oldPh[i] = pha[2L * i];   /* hold actual phase for next diffce */
      pha[2L * i] = p;          /* .. but write back phase change */
    }
}

static void PhaseToFrq(MYFLT *buf, long size, MYFLT incr, MYFLT sampRate)
{
    long    i;
    MYFLT   *pha,p,q,oneOnPi;
    int     z;
    MYFLT   srOn2pi, binMidFrq, frqPerBin;
    MYFLT   expectedDphas,eDphIncr;

    pha = buf + 1;
    srOn2pi = sampRate / (FL(2.0) * PI_F * incr);
    frqPerBin = sampRate / ((MYFLT) ((size - 1L) * 2L));
    binMidFrq = FL(0.0);
    /* Of course, you get some phase shift with spot-on frq coz time shift */
    expectedDphas = FL(0.0);
    eDphIncr = FL(2.0) * PI_F * incr / ((MYFLT) ((size - 1L) * 2L));
    oneOnPi = FL(1.0) / PI_F;
    for (i = 0; i < size; ++i) {
      q = p = pha[2L * i] - expectedDphas;
      MMmaskPhs(p, z, oneOnPi);
      pha[2L * i] = p;
      pha[2L * i] *= srOn2pi;
      pha[2L * i] += binMidFrq;
      expectedDphas += eDphIncr;
      expectedDphas -= TWOPI_F * (MYFLT) ((int) (expectedDphas * oneOnPi));
      binMidFrq += frqPerBin;
    }
}

static char *PVErrMsg(ENVIRON *csound, int err)
{                                       /* return string for error code */
    switch (err) {
      case PVE_OK:        return Str("No PV error");
      case PVE_NOPEN:     return Str("Cannot open PV file");
      case PVE_NPV:       return Str("Object/file not PVOC");
      case PVE_MALLOC:    return Str("No memory for PVOC");
      case PVE_RDERR:     return Str("Error reading PVOC file");
      case PVE_WRERR:     return Str("Error writing PVOC file");
    }
    return Str("Unspecified error");
}

static int PVAlloc(
    ENVIRON     *csound,
    PVSTRUCT    **pphdr,        /* returns address of new block */
    long        dataBsize,      /* desired bytesize of datablock */
    int         dataFormat,     /* data format - PVMYFLT etc */
    MYFLT       srate,          /* sampling rate of original in Hz */
    int         chans,          /* channels of original .. ? */
    long        frSize,         /* frame size of analysis */
    long        frIncr,         /* frame increment (hop) of analysis */
    long        fBsize,         /* bytes in each data frame of file */
    int         frMode,         /* format of frames: PVPOLAR, PVPVOC etc */
    MYFLT       minF,           /* frequency of lowest bin */
    MYFLT       maxF,           /* frequency of highest bin */
    int         fqMode,         /* freq. spacing mode - PVLIN / PVLOG */
    int         infoBsize)      /* bytes to allocate in info region */
    /* Allocate memory for a new PVSTRUCT+data block;
       fill in header according to passed in data.
       Returns PVE_MALLOC  (& **pphdr = NULL) if malloc fails
               PVE_OK      otherwise  */
{
    long        bSize, hSize;

    hSize = sizeof(PVSTRUCT) + infoBsize - PVDFLTBYTS;
    if (dataBsize == PV_UNK_LEN)
        bSize = hSize;
    else
        bSize = dataBsize + hSize;
    if (( (*pphdr) = (PVSTRUCT*) csound->Malloc(csound, bSize)) == NULL )
      return(PVE_MALLOC);
    (*pphdr)->magic        = PVMAGIC;
    (*pphdr)->headBsize    = hSize;
    (*pphdr)->dataBsize    = dataBsize;
    (*pphdr)->dataFormat   = dataFormat;
    (*pphdr)->samplingRate = srate;
    (*pphdr)->channels     = chans;
    (*pphdr)->frameSize    = frSize;
    (*pphdr)->frameIncr    = frIncr;
    (*pphdr)->frameBsize   = fBsize;
    (*pphdr)->frameFormat  = frMode;
    (*pphdr)->minFreq      = minF;
    (*pphdr)->maxFreq      = maxF;
    (*pphdr)->freqFormat   = fqMode;
    /* leave info bytes undefined */
    return(PVE_OK);
}
#endif

static int pvanal(void *csound_, int argc, char **argv)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
#ifdef oldcode
    PVSTRUCT *pvh;
    FILE    *ofd;
    void    *ofd_handle;
    int     err;
    long    Estdatasiz;
    long    fftfrmBsiz = 0;     /* bytes of fft output frame      */
    long    oframeAct;          /* output frms  actual */
#endif
    int     channel = ALLCHNLS;
    int     ovlp = 0;           /* number of overlapping windows to have */
    SOUNDIN *p;                 /* space allocated by SAsndgetset() */

    MYFLT   beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    long    oframeEst = 0;      /* output frms estimated */
    long    frameSize  = 0;     /* size of FFT frames */
    long    frameIncr  = 0;     /* step between successive frames */
#if 0
    WINDAT  dwindow;
#endif
 /* MYFLT   max = 0.0; */
 /* int     cnt = 0; */
    int     latch = 200;
    FILE    *trfil = stdout;
    int     WindowType = 1;
    char    err_msg[512];
    char    *ext;
    int     verbose = 0;

    csound->oparms->displays = 0;
    if (!(--argc))
      return quit(csound, Str("insufficient arguments"));
      do {
        char *s = *++argv;
        if (*s++ == '-')
          switch (*s++) {
          case 'j': FIND("");
            while (*s++); s--;
            break;
          case 's': FIND(Str("no sampling rate"));
#if defined(USE_DOUBLE)
            sscanf(s, "%lf", &sr);
#else
            sscanf(s, "%f", &sr);
#endif
            break;
          case 'c':  FIND(Str("no channel"));
            sscanf(s, "%d", &channel);
            break;
          case 'b':  FIND(Str("no begin time"));
#if defined(USE_DOUBLE)
            sscanf(s, "%lf", &beg_time);
#else
            sscanf(s, "%f", &beg_time);
#endif
            break;
          case 'd':  FIND(Str("no duration time"));
#if defined(USE_DOUBLE)
            sscanf(s, "%lf", &input_dur);
#else
            sscanf(s, "%f", &input_dur);
#endif
            break;
          case 'H':
            {
              int c = *s++;
              if (c == 'M' || c == '\0')
                WindowType = 0;
            }
            break;
          case 'n':  FIND(Str("no framesize"));
            sscanf(s, "%ld", &frameSize);
            if (frameSize < MINFRMPTS || frameSize > MAXFRMPTS) {
              sprintf(err_msg, Str("frameSize must be between %d &%d\n"),
                               MINFRMPTS, MAXFRMPTS);
              return quit(csound, err_msg);
            }
            if (frameSize < 1L || (frameSize & (frameSize - 1L)) != 0L) {
              sprintf(err_msg, Str("pvanal: frameSize must be 2^r"));
              return quit(csound, err_msg);
            }
            break;
          case 'w':  FIND(Str("no windfact"));
            sscanf(s, "%d", &ovlp);
            break;
          case 'h':  FIND(Str("no hopsize"));
            sscanf(s, "%ld", &frameIncr);
            break;
          case 'g':  csound->oparms->displays = 1;
            break;
          case 'G':  FIND(Str("no latch"));
            sscanf(s, "%d", &latch);
            csound->oparms->displays = 1;
            break;
          case 'V':  FIND(Str("no output file for trace"));
            {
              void  *dummy = csound->FileOpen(csound, &trfil, CSFILE_STD, s,
                                                      "w", NULL);
              if (dummy == NULL)
                return quit(csound, Str("Failed to open text file"));
              csound->Message(csound, Str("Writing text form to file %s\n"), s);
            }
          case 'v':
            verbose = 1;
            break;
          default:
            return quit(csound, Str("unrecognised switch option"));
          }
        else break;
      } while (--argc);

      if (argc != 2)
        return quit(csound, Str("illegal number of filenames"));
      infilnam = *argv++;
      outfilnam = *argv;

    if (ovlp && frameIncr)
      return quit(csound, Str("pvanal cannot have both -w and -h"));
    /* open sndfil, do skiptime */
    if ((infd = csound->SAsndgetset(csound, infilnam, &p, &beg_time,
                                    &input_dur, &sr, channel)) == NULL) {
      sprintf(err_msg, Str("error while opening %s"), infilnam);
      return quit(csound, err_msg);
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
      csound->Message(csound, Str("pvanal: %d is a bad window overlap index\n"),
                              (int) ovlp);
      return -1;
    }
    oframeEst = (p->getframes - frameSize/2) / frameIncr;
    csound->Message(csound, Str("%ld infrsize, %ld infrInc\n"),
                            (long) frameSize, (long) frameIncr);
    csound->Message(csound, Str("%ld output frames estimated\n"),
                            (long) oframeEst);

    ext = strrchr(outfilnam, '.');
    /* Look for .pvx extension in any case */
    if (ext != NULL && ext[0] == '.' && tolower(ext[1]) == 'p' &&
        tolower(ext[2]) == 'v' && tolower(ext[3]) == 'x' && ext[4] == '\0') {
      /* even for old pvoc file, is absence of extension OK? */
      if (p->nchanls > MAXPVXCHANS) {
        csound->Message(csound, Str("pvxanal - source has too many channels: "
                                    "Maxchans = %d.\n"), MAXPVXCHANS);
        return -1;
      }
      csound->Message(csound, Str("pvanal: creating pvocex file\n"));
      /* handle all messages in here, for now */
      if (csound->pvxanal(csound, p, infd, outfilnam, p->sr, p->nchanls,
                                  frameSize, frameIncr, frameSize * 2,
                                  PVOC_HAMMING, verbose) != 0) {
        csound->Message(csound, Str("error generating pvocex file.\n"));
        return -1;
      }
    }
    else {
#ifdef oldcode
      fftfrmBsiz = sizeof(MYFLT) * 2 * (frameSize/2 + 1);
      Estdatasiz = oframeEst * fftfrmBsiz;
      /* alloc & fill PV hdrblk */
      if ((err = PVAlloc(csound,
                         &pvh, Estdatasiz, PVMYFLT, sr, p->nchanls, frameSize,
                         frameIncr, fftfrmBsiz, PVPVOC, FL(0.0), sr/FL(2.0),
                         PVLIN, 4))) {
        csound->Message(csound, "pvanal: %s\n", PVErrMsg(csound, err));
        return -1;
      }
      ofd_handle = csound->FileOpen(csound, &ofd, CSFILE_STD, outfilnam, "wb",
                                            "SADIR");
      if (ofd_handle == NULL) {         /* open the output PV file */
        return quit(csound, Str("cannot create output file"));
      }
      /* & wrt hdr into the file */
      if ((long) fwrite(pvh, 1, pvh->headBsize, ofd) < pvh->headBsize) {
        return quit(csound, Str("cannot write header"));
      }
#if 0
      dispinit(csound);
      if (verbose) {
        fprintf(trfil, "Size=%ld Format=%ld Rate=%g Channels=%ld\n",
                pvh->dataBsize/pvh->frameBsize, pvh->dataFormat,
                pvh->samplingRate, pvh->channels);
        fprintf(trfil, "FrameSize=%ld FrameInc=%ld MinFreq=%g MaxFreq=%g\n",
                pvh->frameSize, pvh->frameIncr, pvh->minFreq, pvh->maxFreq);
        fprintf(trfil, "LogLin=%ld\n\n", pvh->freqFormat);
      }
#endif
      oframeAct = takeFFTs(csound, p, pvh, infd, ofd, oframeEst,
                           frameSize, WindowType, frameIncr, fftfrmBsiz,
                           verbose);
      if (oframeAct < 0L)
        return -1;
  /*  dispexit();   */
      csound->Message(csound, Str("%ld output frames written\n"),
                              (long) oframeAct);
#else
      csound->Message(csound,
                      Str("Old c=format pvanal being withdrawn; use pvx\n"));
#endif
    }
    return 0;
}

static const char *pvanal_usage_txt[] = {
    "Usage: pvanal [options...] inputSoundfile outputFFTfile",
    "Options:",
    "    -c <channel>",
    "    -b <beginTime>",
    "    -d <duration>",
    "    -n <frameSize>",
    "    -w <windowOverlap> | -h <hopSize>",
    "    -g | -G <latch>",
    "    -v | -V <txtFile>",
    "    -H: use Hamming window instead of Hanning",
    NULL
};

static int quit(ENVIRON *csound, char *msg)
{
    int i;

    csound->Message(csound, "pvanal error: %s\n", msg);
    for (i = 0; pvanal_usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(pvanal_usage_txt[i]));
    return -1;
}

#ifdef oldcode
/*
 * takeFFTs
 *  Go through the (mono) input sound frame by frame and find the
 *  magnitude and phase change for a string of FFT bins
 */

static long takeFFTs(ENVIRON *csound, SOUNDIN *p, PVSTRUCT *outputPVH,
                     SNDFILE *infd, FILE *ofd, long oframeEst, long frameSize,
                     int WindowType, long frameIncr, long fftfrmBsiz,
                     int verbose)
{
    long    i = -1, nn, read_in;
    MYFLT   *inBuf, *tmpBuf, *oldInPh, *winBuf;
    MYFLT   *v;
    MYFLT   sampRate = (MYFLT)p->sr;  /* not really */
    long    fsIndepVals = (frameSize/2L)+1L;
    MYFLT   *fp1, *fp2;
    IGN(outputPVH);

    inBuf   = (MYFLT*) MakeBuf(csound, frameSize * 2L);
    tmpBuf  = MakeBuf(csound, frameSize + 2L);
    v       = MakeBuf(csound, frameSize);
    oldInPh = MakeBuf(csound, frameSize);
    winBuf  = MakeHalfWin(csound, frameSize, FL(1.0), WindowType);
#if 0
    csound->dispset(csound, &dwindow, v, frameSize/2, "pvanalwin", 0, "PVANAL");
#endif
                             /* initially, clear first half of buffer .. */
    for (fp1 = inBuf, nn = frameSize/2; nn--; )
      *fp1++ = FL(0.0);
                             /* .. and read in second half from file */
    if ((read_in = csound->getsndin(csound, infd, fp1, (frameSize/2), p))
        < frameSize/2) {
      csound->Message(csound, Str("insufficient sound for analysis\n"));
      return -1L;
    }
    for (nn = read_in; nn--; )
      /* IV - Jul 11 2002 */
      *fp1++ *= csound->dbfs_to_float;      /* normalize the samples read in */
    oframeEst -= 1;
    if (!csound->oparms->displays /* && !verbose */)
      csound->Message(csound, Str("frame: "));
    do {
      if (((++i)%20) == 0)
        if (!csound->oparms->displays /* && !verbose */) {
          csound->Message(csound, "%ld ", (long) i);
        }
      /*        copy the current frame */
      for (fp1 = inBuf, fp2 = tmpBuf, nn = frameSize; nn--; )
        *fp2++ = *fp1++;
      ApplyHalfWin(tmpBuf, winBuf, frameSize);
      csound->RealFFT(csound, tmpBuf, (int) frameSize);
      tmpBuf[frameSize] = tmpBuf[1];
      tmpBuf[1] = tmpBuf[frameSize + 1L] = FL(0.0);
      Rect2Polar(tmpBuf, fsIndepVals);
      UnwrapPhase(tmpBuf, fsIndepVals, oldInPh);
      PhaseToFrq(tmpBuf, fsIndepVals, (MYFLT)frameIncr, (MYFLT)sampRate);
      /* write straight out, just the indep vals */
      fwrite(tmpBuf, 1, fftfrmBsiz, ofd);
#if 0
      if (csound->oparms->displays) {
        int j;
        for (j = 0; j<frameSize; j += 2) v[j/2] = tmpBuf[j];
        sprintf(dwindow.caption, "%ld", i);
        display(&dwindow);
        if (dwindow.oabsmax > dwindow.absmax) cnt++; else cnt = 0;
        if (cnt>latch) dwindow.oabsmax = dwindow.absmax;
      }
#endif
      if (!read_in)          /* if previous read had hit EOF, we're done */
        break;               /* mv conts fwrd by frameIncr, rd more pnts */
      for (fp1 = inBuf+frameIncr, fp2 = inBuf, nn = frameSize-frameIncr; nn--; )
        *fp2++ = *fp1++;     /* getsndin pads with zeros if not complete */
      read_in = csound->getsndin(csound, infd, inBuf+frameSize-frameIncr,
                                 frameIncr, p);
      for (fp1 = inBuf+frameSize-frameIncr, nn = read_in; nn--; )
        /* IV - Jul 11 2002 */
        *fp1++ *= csound->dbfs_to_float;    /* normalize samples just read in */
#if 0
      if (!csound->Yield(csound)) break;
#endif
    } while (i < oframeEst);
    if (!csound->oparms->displays && !verbose)
      csound->Message(csound, "%ld\n", (long) i);
    if (i < oframeEst)
      csound->Message(csound, Str("\tearly end of file\n"));
    return((long)i + 1);
}
#endif

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    int retval = ((ENVIRON*) csound)->AddUtility(csound, "pvanal", pvanal);
    if (!retval) {
      retval = ((ENVIRON*) csound)->SetUtilityDescription(csound, "pvanal",
                    "Soundfile analysis for pvoc");
    }
    return retval;
}

