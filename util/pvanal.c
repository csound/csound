/*
    pvanal.c:

    Copyright (C) 1991, 1994  Dan Ellis, Dave Madole,
              (C) 2000        John ffitch, Richard Dobson

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

#include "std_util.h"
#include "cwindow.h"
#include "soundio.h"
#include "pvfileio.h"
#include <math.h>
#include <ctype.h>


typedef struct pvocex_ch {
        double  rratio;
        MYFLT   *input,         /* pointer to start of input buffer */
                *anal,          /* pointer to start of analysis buffer */
                *nextIn,        /* pointer to next empty word in input */
                *analWindow,    /* pointer to center of analysis window */
                *i0,            /* pointer to amplitude channels */
                *i1,            /* pointer to frequency channels */
                *oi,            /* pointer to old phase channels */
                *oldInPhase;    /* pointer to start of input phase buffer */

        int32_t     m, n;

        int32_t     N ,             /* number of phase vocoder channels (bands) */
                M,              /* length of analWindow impulse response */
                L,              /* length of synWindow impulse response */
                D,              /* decimation factor (default will be M/8) */
                I,              /* interpolation factor (default will be I=D)*/
                W ,             /* filter overlap factor (determines M, L) */
                analWinLen,     /* half-length of analysis window */
                synWinLen;      /* half-length of synthesis window */

                MYFLT Fexact;

        int64_t /* tmprate, */     /* temporary variable */
                ibuflen,        /* length of input buffer */
                nI,             /* current input (analysis) sample */
                nMin,           /* first input (analysis) sample */
                nMax;           /* last input sample (unless EOF) */
/***************************** 6:2:91  OLD CODE **************
                                                int64_t    origsize;
*******************************NEW CODE **********************/
        MYFLT   real,           /* real part of analysis data */
                imag,           /* imaginary part of analysis data */
                mag,            /* magnitude of analysis data */
                phase,          /* phase of analysis data */
                angleDif,       /* angle difference */
                RoverTwoPi,     /* R/D divided by 2*Pi */
                TwoPioverR,     /* 2*Pi divided by R/I */
                sum,            /* scale factor for renormalizing windows */
                ftot,           /* scale factor for calculating statistics */
                rIn,            /* decimated sampling rate */
                invR,           /* 1. / srate */
                time,           /* nI / srate */
                R ;             /* input sampling rate */
        int32_t     i,j,k,          /* index variables */
                Dd,             /* number of new inputs to read (Dd <= D) */
                N2,             /* N/2 */
                NO,             /* synthesis NO = N / P */
                NO2,            /* NO/2 */
                Mf,             /* flag for even M */
                Lf,             /* flag for even L */
                flag,           /* end-of-input flag */
                X;              /* flag for magnitude output */

        float   srate;          /* sample rate from header on stdin */
        float   timecheckf;
        int64_t    isr,            /* sampling rate */
                Nchans;         /* no of chans */

        /* my vars */
/*      pvocmode m_mode; */
        int64_t  bin_index;     /* index into oldOutPhase to do fast norm_phase */
        float *synWindow_base;
        MYFLT *analWindow_base;
  void *setup;

} PVX;

/* prototype arguments */

static  int32_t pvxanal(CSOUND *csound, SOUNDIN *p, SNDFILE *fd,
                        const char *fname,
                        int64_t srate, int64_t chans, int64_t fftsize,
                        int64_t overlap, int64_t winsize,
                        pv_wtype wintype,
                        double beta, int32_t displays);
static  int64_t    generate_frame(CSOUND*, PVX *pvx, MYFLT *fbuf, float *outanal,
                                        int64_t samps, int32_t frametype);
static  void    chan_split(CSOUND*, const MYFLT *inbuf, MYFLT **chbuf,
                                    int64_t insize, int64_t chans);
static  int32_t     init(CSOUND *csound,
                     PVX **pvx, int64_t srate, int64_t fftsize, int64_t winsize,
                     int64_t overlap, pv_wtype wintype, double beta);
/* from elsewhere in Csound! But special form for CARL code*/
static  void    hamming(MYFLT *win, int32_t winLen, int32_t even);
static  double  besseli(double x);
static  void    kaiser(MYFLT *win, int32_t len, double Beta);
static  void    vonhann(MYFLT *win, int32_t winLen, int32_t even);
static  int32_t     quit(CSOUND *, char *msg);

#define MINFRMMS        20      /* frame defaults to at least this many ms */
#define MAXFRMPTS       65536
#define MINFRMPTS       16      /* limits on fft size */
#define OVLP_DEF        4       /* default frame overlap factor */
#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
      return quit(csound, MSG);

#define MAXPVXCHANS     (8)
#define DEFAULT_BUFLEN  (8192)  /* per channel */
#define DISPFRAMES      30

static int32_t pvanal(CSOUND *csound, int32_t argc, char **argv)
{
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
    int32_t     channel = ALLCHNLS;
    int32_t     ovlp = 0;           /* number of overlapping windows to have */
    SOUNDIN *p;                 /* space allocated by SAsndgetset() */

    MYFLT   beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    int64_t    oframeEst = 0;      /* output frms estimated */
    int64_t    frameSize  = 0;     /* size of FFT frames */
    int64_t    frameIncr  = 0;     /* step between successive frames */
    int32_t     latch = 200;
    FILE    *trfil = stdout;
    pv_wtype  WindowType = PVOC_HANN;
    char    err_msg[512];
    double  beta = 6.8;
    int32_t displays = 0;


    if (UNLIKELY(!(--argc)))
      return quit(csound, Str("insufficient arguments"));
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's': FIND(Str("no sampling rate"));
#if defined(USE_DOUBLE)
          csound->Sscanf(s, "%lf", &sr);
#else
          csound->Sscanf(s, "%f", &sr);
#endif
          break;
        case 'c':  FIND(Str("no channel"));
          csound->Sscanf(s, "%d", &channel);
          break;
        case 'b':  FIND(Str("no begin time"));
#if defined(USE_DOUBLE)
          csound->Sscanf(s, "%lf", &beg_time);
#else
          csound->Sscanf(s, "%f", &beg_time);
#endif
          break;
        case 'd':  FIND(Str("no duration time"));
#if defined(USE_DOUBLE)
          csound->Sscanf(s, "%lf", &input_dur);
#else
          csound->Sscanf(s, "%f", &input_dur);
#endif
          break;
        case 'H':
          WindowType = PVOC_HAMMING;
          break;
        case 'K':
          WindowType = PVOC_KAISER;
          break;
        case 'B':
          FIND(Str("no beta given"));
            csound->Sscanf(s, "%lf", &beta);
            break;
        case 'n':  FIND(Str("no framesize"));
          csound->Sscanf(s, "%"PRId64, &frameSize);
          if (UNLIKELY(frameSize < MINFRMPTS || frameSize > MAXFRMPTS)) {
            snprintf(err_msg, 512, Str("frameSize must be between %d and %d"),
                     MINFRMPTS, MAXFRMPTS);
            return quit(csound, err_msg);
          }
          if (UNLIKELY(frameSize & 1L))
            return quit(csound, Str("pvanal: frameSize must be even"));
          break;
        case 'w':  FIND(Str("no windfact"));
          csound->Sscanf(s, "%d", &ovlp);
          break;
        case 'h':  FIND(Str("no hopsize"));
          csound->Sscanf(s, "%"PRId64, &frameIncr);
          break;
        case 'g':  displays = 1;
            break;
        case 'G':  FIND(Str("no latch"));
          csound->Sscanf(s, "%d", &latch);
          displays = 1;
          break;
        case 'V':  FIND(Str("no output file for trace"));
          {
            void  *dummy = csound->FileOpen(csound, &trfil, CSFILE_STD, s,
                                             "w", NULL, CSFTYPE_OTHER_TEXT, 0);
            if (UNLIKELY(dummy == NULL))
              return quit(csound, Str("Failed to open text file"));
            csound->Message(csound, Str("Writing text form to file %s\n"), s);
          }
          /* FALLTHRU */
        default:
          return quit(csound, Str("unrecognised switch option"));
        }
      else break;
    } while (--argc);

    if (UNLIKELY(argc != 2))
      return quit(csound, Str("illegal number of filenames"));
    infilnam = *argv++;
    outfilnam = *argv;

    if (UNLIKELY(ovlp && frameIncr))
      return quit(csound, Str("pvanal cannot have both -w and -h"));
    /* open sndfil, do skiptime */
    if (UNLIKELY((infd = (csound->GetUtility(csound))->SndinGetSetSA(csound, infilnam, &p, &beg_time,
                                             &input_dur, &sr, channel)) == NULL)) {
      snprintf(err_msg, 512, Str("error while opening %s"), infilnam);
      return quit(csound, err_msg);
    }
    sr = (MYFLT)p->sr;
    /* setup frame size etc according to sampling rate */
    if (frameSize == 0) {       /* not specified on command line */
      int32_t target;
      target = (int32_t)(sr * (MYFLT)MINFRMMS / FL(1000.0));
      frameSize = MAXFRMPTS;    /* default frame size is > MINFRMMS msecs */
      while ((frameSize>>1) >= target && frameSize > MINFRMPTS)
        frameSize >>= 1;        /* divide down until just larger */
    }
    if (ovlp == 0 && frameIncr == 0) {
      csound->Message(csound, "frameSize=%"PRId64"\n", frameSize);
      ovlp = OVLP_DEF;          /* default overlap */
      frameIncr = frameSize / ovlp;
    }
    else if (ovlp == 0)
      ovlp = (int32_t)(frameSize/frameIncr);
    else frameIncr = frameSize/ovlp;

    if (UNLIKELY(ovlp < 2 || ovlp > 64)) {
      csound->Message(csound,
                      Str("WARNING: pvanal: %d might be a bad window "
                          "overlap index\n"),
                      (int32_t) ovlp);
      /* return -1; */
      /* VL: removed this restriction, which sounds a bit drastic */
    }
    oframeEst = (p->getframes - frameSize/2) / frameIncr;
    csound->Message(csound, Str("%"PRId64" infrsize, %"PRId64" infrInc\n"),
                            (int64_t) frameSize, (int64_t) frameIncr);
    csound->Message(csound, Str("%"PRId64" output frames estimated\n"),
                            (int64_t) oframeEst);

    /* even for old pvoc file, is absence of extension OK? */
    if (UNLIKELY(p->nchanls > MAXPVXCHANS)) {
      csound->Message(csound, Str("pvxanal - source has too many channels: "
                                  "Maxchans = %d.\n"), MAXPVXCHANS);
      return -1;
    }
    csound->Message(csound, "%s", Str("pvanal: creating pvocex file\n"));
    /* handle all messages in here, for now */
    if (UNLIKELY(displays))
        csound->InitDisplay(csound);
    if (UNLIKELY(pvxanal(csound, p, infd, outfilnam, p->sr,
                        ((!channel || channel == ALLCHNLS) ? p->nchanls : 1),
                        frameSize, frameIncr, frameSize * 2,
                         WindowType, beta, displays) != 0)) {
      csound->Message(csound, "%s", Str("error generating pvocex file.\n"));
      return -1;
    }
    if (displays)
      csound->DeinitDisplay(csound);

    return 0;
}

static const char *pvanal_usage_txt[] = {
  Str_noop("Usage: pvanal [options...] inputSoundfile outputFFTfile.pvx"),
  Str_noop("Options:"),
  Str_noop("    -c <channel>"),
  Str_noop("    -b <beginTime>"),
  Str_noop("    -d <duration>"),
  Str_noop("    -n <frameSize>"),
  Str_noop("    -w <windowOverlap> | -h <hopSize>"),
  Str_noop("    -g | -G <latch>"),
  Str_noop("    -v | -V <txtFile>"),
  Str_noop("    -H: use Hamming window instead of the default (von Hann)"),
  Str_noop("    -K: use Kaiser window"),
  Str_noop("    -B <beta>: parameter for Kaiser window"),
    NULL
};

static int32_t quit(CSOUND *csound, char *msg)
{
    int32_t i;

    csound->Message(csound, Str("pvanal error: %s\n"), msg);
    for (i = 0; pvanal_usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(pvanal_usage_txt[i]));
    return -1;
}

/* module interface */

int32_t pvanal_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "pvanal", pvanal);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "pvanal",
                                             Str("Soundfile analysis for pvoc"));
    }
    return retval;
}

typedef struct PVDISPLAY_ {
    CSOUND  *csound;
    WINDAT  dwindow;
    MYFLT   *dispBufs[DISPFRAMES];
    int32_t     npts, dispCnt, dispCntMax, dispFrame;
} PVDISPLAY;

static void PVDisplay_Init(CSOUND *csound, PVDISPLAY *p,
                           int32_t fftSize, int32_t dispCntMax)
{
    int32_t     i;

    memset(p, 0, sizeof(PVDISPLAY));
    p->csound = csound;
    p->npts = (fftSize / 2) + 1;
    p->dispCntMax = dispCntMax;
    for (i = 0; i < DISPFRAMES; i++)
      p->dispBufs[i] = (MYFLT*) csound->Calloc(csound, p->npts * sizeof(MYFLT));
}

static void PVDisplay_Update(PVDISPLAY *p, const float *buf)
{
    int32_t     i;

    if (p->dispFrame >= DISPFRAMES)
      return;
    for (i = 0; i < p->npts; i++)
      p->dispBufs[p->dispFrame][i] += ((MYFLT) buf[i * 2] * (MYFLT) buf[i * 2]);
    p->dispCnt++;
}

static void PVDisplay_Display(PVDISPLAY *p, int32_t frame)
{
    int32_t     i;

    if (p->dispFrame >= DISPFRAMES ||
        p->dispCnt < p->dispCntMax)
      return;
    for (i = 0; i < p->npts; i++)
      p->dispBufs[p->dispFrame][i] =
          (MYFLT) sqrt((double) (p->dispBufs[p->dispFrame][i]
                                 / (MYFLT) p->dispCnt));
    p->csound->SetDisplay(p->csound, &(p->dwindow), p->dispBufs[p->dispFrame],
                       p->npts, "pvanalwin", 0, "PVANAL");
    snprintf(&(p->dwindow.caption[0]), CAPSIZE, "%"PRId64, (int64_t) frame);
    p->csound->Display(p->csound, &(p->dwindow));
    p->dispCnt = 0;
    p->dispFrame++;
}

/* Only supports PVOC_AMP_FREQ format for now */

/* cannot add display code, as we may have 8 channels here...*/

static int32_t pvxanal(CSOUND *csound, SOUNDIN *p, SNDFILE *fd, const char *fname,
                   int64_t srate, int64_t chans, int64_t fftsize, int64_t overlap,
                   int64_t winsize, pv_wtype wintype, double beta, int32_t displays)
{
    int32_t         i, k, pvfile = -1, rc = 0;
    pv_stype    stype = STYPE_16;
    int64_t        buflen, buflen_samps;
    int64_t        sampsread;
    int64_t        blocks_written = 0;     /* m/c framecount for user */
    PVX         *pvx[MAXPVXCHANS];
    MYFLT       *inbuf_c[MAXPVXCHANS];
    float       *frame_c[MAXPVXCHANS];  /* RWD : MUST be 32bit  */
    MYFLT       *inbuf = NULL;
    float       *frame;                 /* RWD : MUST be 32bit  */
    MYFLT       *chanbuf;
    int64_t        total_sampsread = 0;
    PVDISPLAY   disp;

    switch (p->format) {
      case AE_SHORT:  stype = STYPE_16; break;
      case AE_24INT:  stype = STYPE_24; break;
      case AE_LONG:   stype = STYPE_32; break;
      case AE_FLOAT:  stype = STYPE_IEEE_FLOAT; break;
    }

    //nbins = (fftsize/2) + 1;
    //nyquist = (MYFLT) srate * FL(0.5);
    //    chwidth = nyquist/(MYFLT)(nbins - 1);
    for (i = 0; i < MAXPVXCHANS; i++) {
      pvx[i] = NULL;
      inbuf_c[i] = NULL;
      frame_c[i] = NULL;
    }

    /* TODO: save some memory and create analysis window once! */

    for (i = 0; i < chans; i++)
      rc += init(csound,
                 &pvx[i], srate, fftsize, winsize, overlap, wintype, beta);

    if (rc)
      goto error;

    /* alloc all buffers */
    buflen = DEFAULT_BUFLEN;
    /* snap to overlap size*/
    buflen = (buflen/overlap) * overlap;
    buflen_samps = buflen * chans;
    inbuf = (MYFLT *) csound->Malloc(csound, buflen_samps * sizeof(MYFLT));
    for (i=0;i < chans;i++) {
      inbuf_c[i] = (MYFLT *) csound->Malloc(csound, buflen * sizeof(MYFLT));
      frame_c[i] = (float*) csound->Malloc(csound,      /* RWD 32bit */
                                           (fftsize + 2) * sizeof(float));
    }

    pvfile  = csound->PVOC_CreateFile(csound, fname, (int32_t) fftsize, (int32_t) overlap, (int32_t) chans,
                                      PVOC_AMP_FREQ, (int32_t) srate, stype,
                                      wintype, 0.0f, NULL, (int32_t) winsize);
    if (UNLIKELY(pvfile < 0)) {
      csound->Message(csound,
                      Str("pvxanal: unable to create analysis file: %s"),
                      csound->PVOC_ErrorString(csound));
      rc = 1;
      goto error;
    }
    if (displays)
    PVDisplay_Init(csound, &disp, (int32_t) fftsize,
                   (int32_t) (((int64_t) p->getframes * chans / overlap)
                          / DISPFRAMES));

    while ((sampsread = (csound->GetUtility(csound))->Sndin(csound,
                                         fd, inbuf, (int32_t) buflen_samps, p)) > 0) {

      total_sampsread += sampsread;
      /* zeropad to full buflen */
      if (sampsread < buflen_samps) {
        /* for (i = sampsread; i < buflen_samps; i++) */
        /*   inbuf[i] = FL(0.0); */
        memset(inbuf, 0, sizeof(MYFLT)*buflen_samps);
        sampsread = buflen_samps;
      }
      chan_split(csound, inbuf, inbuf_c, sampsread, chans);

      for (i = 0; i < sampsread/chans; i+= overlap) {
        for (k = 0; k < chans; k++) {
          frame = frame_c[k];
          chanbuf = inbuf_c[k];
          if (UNLIKELY(!csound->CheckEvents(csound)))
            csound->LongJmp(csound, 1);
          generate_frame(csound, pvx[k],chanbuf+i,frame,overlap,PVOC_AMP_FREQ);
          if (UNLIKELY(!csound->PVOC_PutFrames(csound, pvfile, frame, 1))) {
            csound->Message(csound,
                            Str("pvxanal: error writing analysis frames: %s\n"),
                            csound->PVOC_ErrorString(csound));
            rc = 1;
            goto error;
          }
          blocks_written++;
          if (displays) PVDisplay_Update(&disp, frame);
          if ((blocks_written/chans) % 20 == 0) {
            csound->Message(csound, "%"PRId64"\n", blocks_written/chans);
          }
          if (displays)
            PVDisplay_Display(&disp, (int32_t) (blocks_written / chans));
        }
      }
      if (total_sampsread >= p->getframes*chans)
        break;
    }

    /* write out remaining frames */
    sampsread = fftsize * chans;
    /* for (i = 0;i< sampsread;i++) */
    /*   inbuf[i] = FL(0.0); */
    memset(inbuf, 0, sizeof(MYFLT)*sampsread);
    chan_split(csound,inbuf,inbuf_c,sampsread,chans);
    for (i = 0; i < sampsread/chans; i+= overlap) {
      for (k = 0; k < chans; k++) {
        frame = frame_c[k];
        chanbuf = inbuf_c[k];
        if (!csound->CheckEvents(csound))
          csound->LongJmp(csound, 1);
        generate_frame(csound,pvx[k],chanbuf+i,frame,overlap,PVOC_AMP_FREQ);
        if (UNLIKELY(!csound->PVOC_PutFrames(csound, pvfile, frame, 1))) {
          csound->Message(csound,
                          Str("pvxanal: error writing analysis frames: %s\n"),
                          csound->PVOC_ErrorString(csound));
          rc = 1;
          goto error;
        }
        blocks_written++;
        if (displays) PVDisplay_Update(&disp, frame);
      }
      if (displays) PVDisplay_Display(&disp, (int32_t) (blocks_written / chans));
    }
    csound->Message(csound, Str("\n%"PRId64" %d-chan blocks written to %s\n"),
                    (int64_t) blocks_written / (int64_t) chans,
                    (int32_t) chans, fname);

 error:
    if (pvfile >= 0)
      csound->PVOC_CloseFile(csound, pvfile);
    return rc;
}

static int32_t init(CSOUND *csound,
                PVX **pvx, int64_t srate, int64_t fftsize, int64_t winsize,
                int64_t overlap, pv_wtype wintype, double beta)
{
    int32_t     i;
    int64_t    N, N2, M, Mf, D;
    MYFLT   sum;
    PVX     *thispvx;

    if (pvx == NULL)
      return 1;

    thispvx  = (PVX *) csound->Calloc(csound, sizeof(PVX));
    /* init all vars, as for a constructor */
    thispvx->input      =  NULL;
    thispvx->anal       =  NULL;
    thispvx->nextIn     =  NULL;    /* pointer to next empty word in input */
    thispvx->analWindow =  NULL;    /* pointer to center of analysis window */
    thispvx->analWindow_base = NULL;
    thispvx->i0         =  NULL;    /* pointer to amplitude channels */
    thispvx->i1         =  NULL;    /* pointer to frequency channels */
    thispvx->oi         =  NULL;    /* pointer to old phase channels */
    thispvx->oldInPhase =  NULL;    /* pointer to start of input phase buffer */
    thispvx->ibuflen    = 0;        /* length of input buffer */

    thispvx->nI   = 0;              /* current input (analysis) sample */
    thispvx->Mf   = 0;              /* flag for even M */
    N = fftsize;
    M = winsize;
    D = overlap;

    if (UNLIKELY(N <= 0))
      return 1;
    if (UNLIKELY(D < 0))
      return 1;
    if (UNLIKELY(M < 0))
      return 1;

    thispvx->isr         = srate;
    thispvx->R           = (MYFLT) srate;
    thispvx->N           =(int32_t)( N  + N%2);    /* Make N even */
    thispvx->N2          = (int32_t)(N2 = (int32_t)(N / 2));
    thispvx->Fexact      = (MYFLT)srate / (MYFLT)N;         /* RWD */
    thispvx->M           = (int32_t) M;
    thispvx->Mf          = Mf = 1 - M%2;
    thispvx->ibuflen     = 4 * M;
    thispvx->setup = csound->RealFFTSetup(csound,  thispvx->N , FFT_FWD);

    D = (int32_t)((D != 0 ? D : M/(8.0))); /* Why floating 8?? */
    if (D == 0) {
      D = 1;
    }
    thispvx->D       = (int32_t) D;
    thispvx->I       = (int32_t) D;
    thispvx->nMin    = 0;       /* first input (analysis) sample */
    thispvx->nMax    = 100000000;
    thispvx->nMax   -= thispvx->nMin;

    /* set up analysis window: The window is assumed to be symmetric
       with M total points.  After the initial memory allocation,
       analWindow always points to the midpoint of the window
       (or one half sample to the right, if M is even); analWinLen
       is half the true window length (rounded down). Any low pass
       window will work; a Hamming window is generally fine,
       but a Kaiser is also available.  If the window duration is
       longer than the transform (M > N), then the window is
       multiplied by a sin(x)/x function to meet the condition:
       analWindow[Ni] = 0 for i != 0.  In either case, the
       window is renormalised so that the phase vocoder amplitude
       estimates are properly scaled.  The maximum allowable
       window duration is ibuflen/2. */

    thispvx->analWindow_base =
        (MYFLT *) csound->Malloc(csound, (M + Mf) * sizeof(MYFLT));

    thispvx->analWindow =
      thispvx->analWindow_base + (thispvx->analWinLen = thispvx->M/2);

    switch (wintype) {
    case PVOC_HAMMING:
      hamming(thispvx->analWindow, thispvx->analWinLen, (int32_t) Mf);
      break;
    case PVOC_KAISER:
      kaiser(thispvx->analWindow_base, (int32_t)(M + Mf), beta);   /* ??? or just M? */
      break;
    default:
      /* for now, anything else just sets von Hann window! */
      vonhann(thispvx->analWindow, thispvx->analWinLen, (int32_t)Mf);
      break;
    }

    for (i = 1; i <= thispvx->analWinLen; i++)
      *(thispvx->analWindow - i) = *(thispvx->analWindow + i - Mf);

    if (M > N) {
      if (Mf)
        *thispvx->analWindow *=
          (MYFLT)((double)N*sin(HALFPI/(double)N)/(HALFPI));
      for (i = 1; i <= thispvx->analWinLen; i++)
        *(thispvx->analWindow + i) *=   /* D.T. 2000*/
          (MYFLT)((double)N*sin((double)(PI*(i+0.5*Mf)/N))/(PI*(i+0.5*Mf)));
      for (i = 1; i <= thispvx->analWinLen; i++)
        *(thispvx->analWindow - i) = *(thispvx->analWindow + i - Mf);
    }

    sum = FL(0.0);
    for (i = -thispvx->analWinLen; i <= thispvx->analWinLen; i++)
      sum += *(thispvx->analWindow + i);

    sum = FL(2.0) / sum;        /*factor of 2 comes in later in trig identity*/

    for (i = -thispvx->analWinLen; i <= thispvx->analWinLen; i++)
      *(thispvx->analWindow + i) *= sum;

    /* set up input buffer:  nextIn always points to the next empty
       word in the input buffer (i.e., the sample following
       sample number (n + analWinLen)).  If the buffer is full,
       then nextIn jumps back to the beginning, and the old
       values are written over. */

    thispvx->input =
        (MYFLT *) csound->Malloc(csound, thispvx->ibuflen * sizeof(MYFLT));
    thispvx->nextIn = thispvx->input;

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
       so the other channels are redundant. oldInPhase is used
       in the conversion to remember the previous phase when
       calculating phase difference between successive samples. */

    thispvx->anal =
        (MYFLT *) csound->Malloc(csound, (N + 2) * sizeof(MYFLT));
    thispvx->oldInPhase =
        (MYFLT *) csound->Malloc(csound, (N2 + 1) * sizeof(MYFLT));

    thispvx->rIn = ((MYFLT) thispvx->R / D);
    thispvx->invR =(FL(1.0) / thispvx->R);
    thispvx->RoverTwoPi = thispvx->rIn / TWOPI_F;
    /* input time (in samples) */
    thispvx->nI = -(thispvx->analWinLen / D) * D;
    /* number of new inputs to read */
    thispvx->Dd = (int32_t)(thispvx->analWinLen + thispvx->nI + 1);
    thispvx->flag = 1;

    /* for (i=0;i < thispvx->ibuflen;i++) */
    /*   thispvx->input[i] = FL(0.0); */
    memset(thispvx->input, 0, sizeof(MYFLT)*thispvx->ibuflen);
    /* for (i=0;i < N+2;i++) */
    /*   thispvx->anal[i] = FL(0.0); */
    memset(thispvx->anal, 0, sizeof(MYFLT)*(N+2));
    /* for (i=0;i < N2+1;i++) */
    /*   thispvx->oldInPhase[i] = FL(0.0); */
    memset(thispvx->oldInPhase, 0, sizeof(MYFLT)*(N2+1));
    *pvx = thispvx;
    return 0;
}

#define MAX(a,b) (a>b ? a : b)
#define MIN(a,b) (a<b ? a : b)

/* RWD outanal MUST be 32bit */

static int64_t generate_frame(CSOUND *csound, PVX *pvx,
                                           MYFLT *fbuf, float *outanal,
                                           int64_t samps, int32_t frametype)
{
    int32_t     got, tocp, i, j, k;
    int64_t    N = pvx->N;
    MYFLT   *fp, *oi, *i0, *i1, real, imag, *anal, angleDif;
    double  rratio, phase;
    float   *ofp;           /* RWD MUST be 32bit */

    anal = pvx->anal;

    got = (int32_t) samps;            /* always assume */
    if (got < pvx->Dd)
      pvx->Dd = got;

    fp = fbuf;

    tocp = MIN(got, (int32_t)(pvx->input+pvx->ibuflen-pvx->nextIn));
    got -= tocp;
    while (tocp-- > 0)
      *pvx->nextIn++ = *fp++;

    if (got > 0) {
      pvx->nextIn -= pvx->ibuflen;
      while (got-- > 0)
        *pvx->nextIn++ = *fp++;
    }
    if (pvx->nextIn >= (pvx->input + pvx->ibuflen))
      pvx->nextIn -= pvx->ibuflen;

    if (pvx->nI > 0)
      for (i = pvx->Dd; i < pvx->D; i++) {      /* zero fill at EOF */
        *(pvx->nextIn++) = FL(0.0);
        if (pvx->nextIn >= (pvx->input + pvx->ibuflen))
          pvx->nextIn -= pvx->ibuflen;
      }

    /* analysis: The analysis subroutine computes the complex output at
       time n of (N/2 + 1) of the phase vocoder channels.  It operates
       on input samples (n - analWinLen) thru (n + analWinLen) and
       expects to find these in input[(n +- analWinLen) mod ibuflen].
       It expects analWindow to point to the center of a
       symmetric window of length (2 * analWinLen +1).  It is the
       responsibility of the main program to ensure that these values
       are correct!  The results are returned in anal as succesive
       pairs of real and imaginary values for the lowest (N/2 + 1)
       channels.   The subroutines fft and reals together implement
       one efficient FFT call for a real input sequence.  */

/* initialize */
    /* for (i = 0; i < N+2; i++) */
    /*   *(anal + i) = FL(0.0); */
    memset(anal, 0, sizeof(MYFLT)*(N+2));

    j = (int32_t)((pvx->nI - pvx->analWinLen-1+pvx->ibuflen)%pvx->ibuflen);  /*input pntr*/

    k = (int32_t)( pvx->nI - pvx->analWinLen - 1);                  /*time shift*/
    while (k < 0)
      k += N;
    k = k % N;
    for (i = -pvx->analWinLen; i <= pvx->analWinLen; i++) {
      if (++j >= pvx->ibuflen)
        j -= pvx->ibuflen;
      if (++k >= N)
        k -= N;
      *(anal + k) += *(pvx->analWindow + i) * *(pvx->input + j);
    }
    csound->RealFFT(csound, pvx->setup, anal);
    /* conversion: The real and imaginary values in anal are converted to
       magnitude and angle-difference-per-second (assuming an
       intermediate sampling rate of rIn) and are returned in
       anal. */
    /* only support this format for now, in Csound */
    if (frametype == PVOC_AMP_FREQ) {
      for (i=0,i0=anal,i1=anal+1,oi=pvx->oldInPhase;
           i <= pvx->N2;
           i++,i0+=2,i1+=2, oi++) {
        real = *i0;
        imag = *i1;
        *i0 =(MYFLT) hypot((double)real, (double)imag);
        /* phase unwrapping */
        /*if (*i0 == 0.)*/
        if (*i0 < FL(1.0E-10))        /* RWD don't mess with v small numbers! */
          angleDif = FL(0.0);

        else {
          rratio = atan2((double)imag,(double)real);
          angleDif  = (MYFLT)((phase = rratio) - *oi);
          *oi = (MYFLT) phase;
        }

        if (angleDif > PI)
          angleDif = (MYFLT)(angleDif - TWOPI);
        if (angleDif < -PI)
          angleDif = (MYFLT)(angleDif + TWOPI);

        /* add in filter center freq.*/
        *i1 = angleDif * pvx->RoverTwoPi + ((MYFLT) i * pvx->Fexact);
      }
    }
    /* else must be PVOC_COMPLEX */
    fp = anal;
    ofp = outanal;
    for (i=0;i < N+2;i++)
      *ofp++ = (float) *fp++;  /* RWD need 32bit cast incase MYFLT is double */

    pvx->nI += pvx->D;                          /* increment time */
    pvx->Dd = MIN(pvx->D,                       /* CARL */
                  MAX(0, (int32_t)(pvx->D + pvx->nMax - pvx->nI - pvx->analWinLen)));

    return pvx->D;
}

static void chan_split(CSOUND *csound, const MYFLT *inbuf, MYFLT **chbuf,
                                        int64_t insize, int64_t chans)
{
    int64_t i,j,len;
    MYFLT ampfac;
    MYFLT *buf_c[MAXPVXCHANS] = { NULL };

    const MYFLT *p_inbuf = inbuf;

    len = insize/chans;
    ampfac = (1.0/csound->Get0dBFS(csound));

    for (i=0;i < chans;i++)
      buf_c[i] = chbuf[i];

    for (i = len;i;--i)
      for (j=0;j < chans;j++)
        *(buf_c[j]++) = ampfac * *p_inbuf++; /* ** this line is odd ** */

}

static void hamming(MYFLT *win, int32_t winLen, int32_t even)
{
    double ftmp;
    int32_t i;

    ftmp = PI/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        *(win+i) = (MYFLT)(0.54 + 0.46*cos(ftmp*((double)i+0.5)));
      *(win+winLen) = FL(0.0);
    }
    else {
      *(win) = FL(1.0);
      for (i=1; i<=winLen; i++)
        *(win+i) = (MYFLT)(0.54 + 0.46*cos(ftmp*(double)i));
    }

}

static double besseli(double x)
{
    double ax, ans;
    double y;

    if (( ax = fabs( x)) < 3.75)     {
      y = x / 3.75;
      y *= y;
      ans = (1.0 + y * ( 3.5156229 +
                         y * ( 3.0899424 +
                               y * ( 1.2067492 +
                                     y * ( 0.2659732 +
                                           y * ( 0.360768e-1 +
                                                 y * 0.45813e-2))))));
    }
    else {
      y = 3.75 / ax;
      ans = ((exp ( ax) / sqrt(ax))
             * (0.39894228 +
                y * (0.1328592e-1 +
                     y * (0.225319e-2 +
                          y * (-0.157565e-2 +
                               y * (0.916281e-2 +
                                    y * (-0.2057706e-1 +
                                         y * (0.2635537e-1 +
                                              y * (-0.1647633e-1 +
                                                   y * 0.392377e-2)))))))));
    }
    return ans;
}

static void kaiser(MYFLT *win, int32_t len, double Beta)
{
    MYFLT *ft = win;
    double i, xarg = 1.0;        /*xarg = amp scalefactor */
    for (i = -(double)len/2.0 + 0.1 ; i < (double)len/2.0 ; i++) {
      double z = 2.0*i/(double)(len - 1);
      *ft++ = (MYFLT) (xarg *
                       besseli(Beta * sqrt(1.0-z*z)) /
                       besseli(Beta));
    }
    /*  assymetrical hack: sort out first value! */
    win[0] = win[len-1];
}

static void vonhann(MYFLT *win, int32_t winLen, int32_t even)
{
    MYFLT ftmp;
    int32_t i;

    ftmp = PI_F/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        win[i] = (MYFLT)(0.5 + 0.5 * cos(ftmp*((double)i+0.5)));
      win[winLen] = FL(0.0);
    }
    else {
      win[0] = FL(1.0);
      for (i=1; i<=winLen; i++)
        win[i] = (MYFLT)(0.5 + 0.5 * cos(ftmp*(double)i));
    }
}
