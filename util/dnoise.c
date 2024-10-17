/*
    dnoise.c:

    Copyright (C) 2000 Mark Dolson, John ffitch

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

/*
 *    PROGRAM:    dnoise - de-noise a recording
 *
 *    AUTHOR:     Mark Dolson
 *
 *    DATE:       August 26, 1989
 *
 *    COMMENTS:   dnoise takes floats from stdin and outputs them
 *                on stdout as a noise-reduced version of the input signal.
 *                dnoise uses the phase vocoder algorithm in which
 *                successsive windows are Fast Fourier Transformed,
 *                noise-gated, and then Inverse Fast Fourier Transformed
 *                and overlap-added back together.
 *
 *    REVISIONS:  John ffitch, September 1999, December 2000
 *                Writes any format using usual Csound functions.
 *
 */

/*
        This is a noise reduction scheme using frequency-
        domain noise-gating.  This should work best in
        the case of high signal-to-noise with hiss-type
        noise.  The algorithm is that suggested by
        Moorer & Berger in "Linear-Phase Bandsplitting:
        Theory and Applications" presented at the 76th
        Convention 1984 October 8-11 New York of the Audio
        Engineering Society (preprint #2132) except that
        it uses the Weighted Overlap-Add formulation for
        short-time Fourier analysis-synthesis in place of
        the recursive formulation suggested by Moorer &
        Berger.  The gain in each frequency bin is computed
        independently according to

        gain = g0 + (1-g0) * [avg / (avg + th*th*nref)] ^ sh

        where avg and nref are the mean squared signal and
        noise respectively for the bin in question.  (This
        is slightly different than in Moorer & Berger.)  The
        critical parameters th and g0 are specified in dB
        and internally converted to decimal values.  The nref
        values are computed at the start of the program on the
        basis of a noise_soundfile (specified in the command
        line) which contains noise without signal.  The avg
        values are computed over a rectangular window of m
        FFT frames looking both ahead and behind the current
        time.  This corresponds to a temporal extent of m*D/R
        (which is typically (m*N/8)/R).  The default settings
        of N, M, and D should be appropriate for most uses.  A
        higher sample rate than 16KHz might indicate a higher N.
*/

#include "std_util.h"
#include "soundio.h"
#include <math.h>
#include <ctype.h>
#include <inttypes.h>


#define ERR(x)                          \
{                                       \
    csound->Message(csound, "%s", x);   \
    return -1;                          \
}

#define FIND(x)                                                            \
{                                                                          \
    if (*s == '\0') {                                                      \
      if (UNLIKELY(!(--argc) || (((s = *argv++) != NULL) && *s == '-'))) { \
        csound->Message(csound, "%s\n", Str(x));                           \
        return dnoise_usage(csound, -1);                                   \
      }                                                                    \
    }                                                                      \
}

static  int32_t dnoise_usage(CSOUND *, int32_t);
static  void    hamming(MYFLT *, int32_t, int32_t);

static int32_t writebuffer(CSOUND *, SNDFILE *, MYFLT *,
                           int32_t, int32_t *, OPARMS *);

static inline void fast2(CSOUND *csound, void *setup, MYFLT *b)
{
    csound->RealFFT(csound, setup, b);
}

static inline void fsst2(CSOUND *csound, void *setup, MYFLT *b)
{
    csound->RealFFT(csound, setup, b);
}


static int32_t dnoise(CSOUND *csound, int32_t argc, char **argv)
{
    OPARMS  O;
    MYFLT   beg = -FL(1.0), end = -FL(1.0);
    int64_t Beg = 0, End = 99999999;

    MYFLT
        *ibuf1,     /* pointer to start of input buffer */
        *ibuf2,     /* pointer to start of input buffer */
        *obuf1,     /* pointer to start of output buffer */
        *obuf2,     /* pointer to start of output buffer */
        *fbuf,      /* pointer to start of FFT buffer */
        *aWin,      /* pointer to center of analysis window */
        *sWin,      /* pointer to center of synthesis window */
        *i0,        /* pointer to real channels */
        *i1,        /* pointer to imaginary channels */
        *j0,        /* pointer to real channels */
        *j1,        /* pointer to imaginary channels */
        *f,         /* pointer to FFT buffer */
        *f0,        /* pointer to real channels */
        *f1,        /* pointer to imaginary channels */
        *w,         /* pointer to window */
        *mbuf,      /* m most recent frames of FFT */
        *nbuf,      /* m most recent frames of FFT */
        *nref,      /* noise reference buffer */
        *rsum,      /* running sum of magnitude-squared spectrum */
        *ssum,      /* running sum of magnitude-squared spectrum */
        *ibp,       /* pointer to next input to be read */
        *ib0,       /* pointer to output buffer */
        *ib1,       /* pointer to output buffer */
        *ib2,       /* pointer to output buffer */
        *obp,       /* pointer to next output to be read */
        *ob0,       /* pointer to output buffer */
        *ob1,       /* pointer to output buffer */
        *ob2;       /* pointer to output buffer */

    int32_t
        N = 0,      /* number of phase vocoder channels (bands) */
        Np2,        /* N+2 */
        M = 0,      /* length of aWin impulse response */
        L = 0,      /* length of sWin impulse response */
        D = 0,      /* decimation factor (default will be M/8) */
        I = 0,      /* interpolation factor (default will be I=D)*/
        W = -1,     /* filter overlap factor (determines M, L) */
        ibuflen,    /* size of ibuf */
        obuflen,    /* size of obuf */
        aLen,       /* half-length of analysis window */
        sLen;       /* half-length of synthesis window */

    int64_t
        oCnt = 0L,  /* number of samples written to output */
        nI,         /* current input (analysis) sample */
        nO,         /* current output (synthesis) sample */
        nImodR,     /* current input sample mod R */
        nMaxOut,    /* last output (synthesis) sample */
        nMin,       /* first input (analysis) sample */
        nMax,       /* last input sample (unless EOF) */
        lnread,     /* total input samples read */
        lj,         /* to satisfy lame Microsoft compiler */
        lk;         /* to satisfy lame Microsoft compiler */

    SNDFILE *fp = NULL; /* noise reference file */

    MYFLT
        Ninv,       /* 1. / N */
        sum,        /* scale factor for renormalizing windows */
      //rIn,        /* decimated sampling rate */
      //rOut,       /* pre-interpolated sampling rate */
        invR,       /* 1. / srate */
        time,       /* nI / srate */
        gain,       /* gain of noise gate */
        g0 = -FL(40.0),/* minimum gain for noise gate */
        g0m,        /* 1. - g0 */
        th = FL(30.0), /* threshold above noise reference (dB) */
        avg,        /* average square energy */
        fac,        /* factor in gain computation */
        minv,       /* 1 / m */
        R = -FL(1.0);  /* input sampling rate */

    int32_t i,j,k,  /* index variables */
        ibs,        /* current starting location in input buffer */
        ibc,        /* current location in input buffer */
        obs,        /* current starting location in output buffer */
        obc,        /* current location in output buffer */
        m = 5,      /* number of frames to save in mbuf */
        mi = 0,     /* frame offset index in mbuf */
        mj,         /* delayed offset index in mbuf */
        md,         /* number of frames of delay in mbuf (m/2) */
        mp,         /* mi * Np2 */
        sh = 1,     /* sharpness control for noise gate gain */
        nread,      /* number of bytes read */
        N2,         /* N/2 */
        Meven = 0,  /* flag for even M */
        Leven = 0,  /* flag for even L */
        Verbose = 0,/* flag for verbose output to stderr */
        Chans = -1, /* number of audio input channels (stereo = 2) */
        chan,       /* channel counter */
        flag = 1,   /* end-of-input flag */
        first = 0;  /* first-time-thru flag */

    SOUNDIN     *p, *pn;
    char        *infile = NULL, *nfile = NULL;
    SNDFILE     *inf = NULL, *outfd = NULL;
    char        c, *s;
    int32_t     channel = ALLCHNLS;
    MYFLT       beg_time  = FL(0.0), input_dur  = FL(0.0), sr  = FL(0.0);
    MYFLT       beg_ntime = FL(0.0), input_ndur = FL(0.0), srn = FL(0.0);
    const char  *envoutyp = NULL;
    uint32_t    outbufsiz = 0U;
    int32_t     nrecs = 0;
    memcpy(&O, csound->GetOParms(csound), sizeof(OPARMS));


    /* audio is now normalised after call to getsndin  */
    /* csound->e0dbfs = csound->dbfs_to_float = FL(1.0); */

    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        O.filetyp = TYP_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        O.filetyp = TYP_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        O.filetyp = TYP_IRCAM;
      else {
        csound->Message(csound, Str("%s not a recognised SFOUTYP env setting"),
                                envoutyp);
        return -1;
      }
    }
    {
      ++argv;
      while (--argc>0) {
        s = *argv++;
        if (*s++ == '-') {                        /* read all flags:  */
          while ((c = *s++) != '\0') {
            switch (c) {
            case 'o':
              FIND("no outfilename");
              O.outfilename = s;                 /* soundout name */
              for ( ; *s != '\0'; s++) ;
              if (UNLIKELY(strcmp(O.outfilename, "stdin") == 0)) {
                csound->Message(csound, "%s", Str("-o cannot be stdin\n"));
                return -1;
              }
              break;
            case 'i':
              FIND("no noisefilename");
              nfile = s;
              for ( ; *s != '\0'; s++) ;
              break;
            case 'A':
              if (UNLIKELY(O.filetyp == TYP_WAV))
                csound->Warning(csound,
                                "%s", Str("-A overriding local default WAV out"));
              O.filetyp = TYP_AIFF;    /* AIFF output request*/
              break;
            case 'J':
              if (UNLIKELY(O.filetyp == TYP_AIFF || O.filetyp == TYP_WAV))
                csound->Warning(csound, "%s", Str("-J overriding local default "
                                            "AIFF/WAV out"));
              O.filetyp = TYP_IRCAM;   /* IRCAM output request */
              break;
            case 'W':
              if (UNLIKELY(O.filetyp == TYP_AIFF))
                csound->Warning(csound,
                                "%s", Str("-W overriding local default AIFF out"));
              O.filetyp = TYP_WAV;      /* WAV output request */
              break;
            case 'h':
              O.filetyp = TYP_RAW;
              break;
            case 'c':
              O.outformat = AE_CHAR;     /* 8-bit char soundfile */
              break;
            case '8':
              O.outformat = AE_UNCH;     /* 8-bit unsigned char file */
              break;
            case 'a':
              O.outformat = AE_ALAW;     /* a-law soundfile */
              break;
            case 'u':
              O.outformat = AE_ULAW;     /* mu-law soundfile */
              break;
            case 's':
              O.outformat = AE_SHORT;    /* short_int soundfile */
              break;
            case 'l':
              O.outformat = AE_LONG;     /* long_int soundfile */
              break;
            case 'f':
              O.outformat = AE_FLOAT;    /* float soundfile */
              break;
            case 'R':
              O.rewrt_hdr = 1;
              break;
            case 'H':
              if (isdigit(*s)) {
                int32_t n;
                csound->Sscanf(s, "%d%n", &O.heartbeat, &n);
                s += n;
              }
              else O.heartbeat = 1;
              break;
            case 't':
              FIND(Str("no t argument"));
#if defined(USE_DOUBLE)
              csound->Sscanf(s,"%lf",&th);
#else
              csound->Sscanf(s,"%f",&th);
#endif
              while (*++s);
              break;
            case 'S':
              FIND("no s arg");
              csound->Sscanf(s,"%d", &sh);
              while (*++s);
              break;
            case 'm':
              FIND("no m arg");
#if defined(USE_DOUBLE)
              csound->Sscanf(s,"%lf",&g0);
#else
              csound->Sscanf(s,"%f",&g0);
#endif
              while (*++s);
              break;
            case 'n':
              FIND(Str("no n argument"));
              csound->Sscanf(s,"%d", &m);
              while (*++s);
              break;
            case 'b':
              FIND(Str("no b argument"));
#if defined(USE_DOUBLE)
              csound->Sscanf(s,"%lf",&beg);
#else
              csound->Sscanf(s,"%f",&beg);
#endif
              while (*++s);
              break;
            case 'B': FIND(Str("no B argument"));
              csound->Sscanf(s,"%" SCNd64, &Beg);
              while (*++s);
              break;
            case 'e': FIND("no e arg");
#if defined(USE_DOUBLE)
              csound->Sscanf(s,"%lf",&end);
#else
              csound->Sscanf(s,"%f",&end);
#endif
              while (*++s);
              break;
            case 'E': FIND(Str("no E argument"));
              csound->Sscanf(s,"%" PRId64, &End);
              while (*++s);
              break;
            case 'N': FIND(Str("no N argument"));
              csound->Sscanf(s,"%d", &N);
              while (*++s);
              break;
            case 'M': FIND(Str("no M argument"));
              csound->Sscanf(s,"%d", &M);
              while (*++s);
              break;
            case 'L': FIND(Str("no L argument"));
              csound->Sscanf(s,"%d", &L);
              while (*++s);
              break;
            case 'w': FIND(Str("no w argument"));
              csound->Sscanf(s,"%d", &W);
              while (*++s);
              break;
            case 'D': FIND(Str("no D argument"));
              csound->Sscanf(s,"%d", &D);
              while (*++s);
              break;
            case 'V':
              Verbose = 1; break;
            default:
              csound->Message(csound, Str("Looking at %c\n"), c);
              return dnoise_usage(csound, -1);  /* this exits with error */
            }
          }
        }
        else if (infile==NULL) {
          infile = --s;
          csound->Message(csound, Str("Infile set to %s\n"), infile);
        }
        else {
          csound->Message(csound, Str("End with %s\n"), s);
          return dnoise_usage(csound, -1);
        }
      }
    }
    if (UNLIKELY(infile == NULL)) {
      csound->Message(csound, "%s", Str("dnoise: no input file\n"));
      return dnoise_usage(csound, -1);
    }
    if (UNLIKELY(nfile == NULL)) {
      csound->Message(csound, "%s",
                      Str("Must have an example noise file (-i name)\n"));
      return -1;
    }
    if (UNLIKELY((inf = (csound->GetUtility(csound))->SndinGetSetSA(csound, infile, &p, &beg_time,
                                            &input_dur, &sr, channel)) == NULL)) {
      csound->Message(csound, Str("error while opening %s"), infile);
      return -1;
    }
    if (O.outformat == 0) O.outformat = p->format;
    O.sfsampsize = csound->SndfileSampleSize(FORMAT2SF(O.outformat));
    if (O.filetyp == TYP_RAW) {
      O.rewrt_hdr = 0;
    }
    if (O.outfilename == NULL)
      O.outfilename = "test";
    {
      SFLIB_INFO sfinfo;
      char    *name;
      memset(&sfinfo, 0, sizeof(SFLIB_INFO));
      sfinfo.samplerate = (int32_t) p->sr;
      sfinfo.channels = (int32_t) p->nchanls;
      sfinfo.format = TYPE2SF(O.filetyp) | FORMAT2SF(O.outformat);
      if (strcmp(O.outfilename, "stdout") != 0) {
        name = csound->FindOutputFile(csound, O.outfilename, "SFDIR");
        if (name == NULL) {
          csound->Message(csound, Str("cannot open %s.\n"), O.outfilename);
          return -1;
        }
        outfd = csound->SndfileOpen(csound,name, SFM_WRITE, &sfinfo);
        if (outfd != NULL)
          csound->NotifyFileOpened(csound, name,
                      csound->Type2CsfileType(O.filetyp, O.outformat), 1, 0);
        csound->Free(csound, name);
      }
      else
        outfd = csound->SndfileOpenFd(csound,1, SFM_WRITE, &sfinfo, 1);
      if (UNLIKELY(outfd == NULL)) {
        csound->Message(csound, Str("cannot open %s."), O.outfilename);
        return -1;
      }
      /* register file to be closed by csoundReset() */
      (void)csound->CreateFileHandle(csound, &outfd, CSFILE_SND_W,
                                     O.outfilename);
      csound->SndfileCommand(csound,outfd, SFC_SET_CLIPPING, NULL, SFLIB_TRUE);
    }

    (csound->GetUtility(csound))->SetUtilSr(csound, (MYFLT)p->sr);
    (csound->GetUtility(csound))->SetUtilNchnls(csound, Chans = p->nchanls);

    /* read header info */
    if (R < FL(0.0))
      R = (MYFLT)p->sr;
    if (Chans < 0)
      Chans = (int32_t) p->nchanls;
    p->nchanls = Chans;

    if (UNLIKELY(Chans > 2)) {
      csound->Message(csound, "%s", Str("dnoise: input MUST be mono or stereo\n"));
      return -1;
    }

    /* read noise reference file */

    if (UNLIKELY((fp = (csound->GetUtility(csound))->SndinGetSetSA(csound, nfile, &pn, &beg_ntime,
                                           &input_ndur, &srn, channel)) == NULL)) {
      csound->Message(csound, "%s",
                      Str("dnoise: cannot open noise reference file\n"));
      return -1;
    }

    if (UNLIKELY(sr != srn)) {
      csound->Message(csound, "%s", Str("Incompatible sample rates\n"));
      return -1;
    }
    /* calculate begin and end times in NOISE file */
    if (beg >= FL(0.0)) Beg = (int64_t) (beg * R);
    if (end >= FL(0.0)) End = (int64_t) (end * R);
    else if (End == 99999999) End = (int64_t) (input_ndur * R);

    nMin = Beg * Chans;            /* total number of samples to skip */
    nMax = End - Beg;            /* number of samples per channel to process */

    /* largest valid FFT size is 8192 */
    if (N == 0)
      N = 1024;
    for (i = 1; i < 4096; i *= 2)
      if (i >= N)
        break;
    if (UNLIKELY(i != N))
      csound->Message(csound,
                      Str("dnoise: warning - N not a valid power of two; "
                          "revised N = %d\n"),i);
    //FFT setup
    //printf("NNN %d \n", N);
    void *fftsetup_fwd =  csound->RealFFTSetup(csound,N,FFT_FWD);
    void *fftsetup_inv =  csound->RealFFTSetup(csound,N,FFT_INV);

    N = i;
    N2 = N / 2;
    Np2 = N + 2;
    Ninv = FL(1.0) / N;

    if (W != -1) {
      if (UNLIKELY(M != 0))
        csound->Message(csound, "%s",
                        Str("dnoise: warning - do not specify both M and W\n"));
      else if (W == 0)
        M = 4*N;
      else if (W == 1)
        M = 2*N;
      else if (W == 2)
        M = N;
      else if (W == 3)
        M = N2;
      else
        csound->Message(csound, "%s", Str("dnoise: warning - invalid W ignored\n"));
    }

    if (M == 0)
      M = N;
    if ((M%2) == 0)
      Meven = 1;

    if (L == 0)
      L = M;
    if ((L%2) == 0)
      Leven = 1;

    if (UNLIKELY(M < 7)) {
      csound->Message(csound, "%s", Str("dnoise: warning - M is too small\n"));
      exit(~1);
    }
    if (D == 0)
      D = M / 8;

    I = D;

    lj = (int64_t) M + 3 * (int64_t) D;
    lj *= (int64_t) Chans;
    if (UNLIKELY(lj > 32767)) {
      csound->Message(csound, "%s", Str("dnoise: M too large\n"));
      return -1;
    }
    lj = (int64_t) L + 3 * (int64_t) I;
    lj *= (int64_t) Chans;
    if (UNLIKELY(lj > 32767)) {
      csound->Message(csound, "%s", Str("dnoise: L too large\n"));
      return -1;
    }

    ibuflen = Chans * (M + 3 * D);
    obuflen = Chans * (L + 3 * I);
    outbufsiz = obuflen * sizeof(MYFLT);                 /* calc outbuf size */
#if 0
    outbuf = csound->Malloc(csound, (size_t) outbufsiz); /* & alloc bufspace */
#endif
    csound->Message(csound, Str("writing %u-byte blks of %s to %s"),
                    outbufsiz, csound->GetStrFormat(O.outformat),
                    O.outfilename);
    csound->Message(csound, " (%s)\n",csound->Type2String(O.filetyp));
/*  spoutran = spoutsf; */

    minv = FL(1.0) / (MYFLT)m;
    md = m / 2;
    g0 = (MYFLT) pow(10.0,(double)(0.05*(double)g0));
    g0m = FL(1.0) - g0;
    th = (MYFLT) pow(10.0,(double)(0.05*(double)th));

    /* set up analysis window: The window is assumed to be symmetric
        with M total points.  After the initial memory allocation,
        aWin always points to the midpoint of the window (or one
        half sample to the right, if M is even); aLen is half the
        true window length (rounded down).  If the window duration
        is longer than the transform (M > N), then the window is
        multiplied by a sin(x)/x function to meet the condition:
        aWin[Ni] = 0 for i != 0.  In either case, the
        window is renormalized so that the phase vocoder amplitude
        estimates are properly scaled.  */

    if (UNLIKELY((aWin =
                  (MYFLT*) csound->Calloc(csound,
                                          (M+Meven) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    aLen = M/2;
    aWin += aLen;

    hamming(aWin, aLen, Meven);
    for (i = 1; i <= aLen; i++) {
      aWin[-i] = aWin[i-1];
    }

    if (M > N) {
      if (Meven)
        *aWin *= (MYFLT)N * (MYFLT) sin(HALFPI/(double)N) /( HALFPI_F);
      for (i = 1; i <= aLen; i++)
        aWin[i] *= (MYFLT) (N * sin(PI * ((double) i + 0.5 * (double) Meven)
                                    / (double) N)
                            / (PI * (i + 0.5 * (double) Meven)));
      for (i = 1; i <= aLen; i++)
        aWin[-i] = aWin[i - Meven];
    }

    sum = FL(0.0);
    for (i = -aLen; i <= aLen; i++)
      sum += aWin[i];

    sum = FL(2.0) / sum;    /* factor of 2 comes in later in trig identity */

    for (i = -aLen; i <= aLen; i++)
      aWin[i] *= sum;

    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    if (UNLIKELY((sWin =
                  (MYFLT*) csound->Calloc(csound,
                                          (L+Leven) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    sLen = L/2;
    sWin += sLen;

    if (M <= N) {
      hamming(sWin, sLen, Leven);
      for (i = 1; i <= sLen; i++)
        sWin[-i] = sWin[i - Leven];

      for (i = -sLen; i <= sLen; i++)
        sWin[i] *= sum;

      sum = FL(0.0);
      for (i = -sLen; i <= sLen; i+=I)
        sum += sWin[i] * sWin[i];

      sum = FL(1.0) / sum;

      for (i = -sLen; i <= sLen; i++)
        sWin[i] *= sum;
    }
    else {
      hamming(sWin, sLen, Leven);
      for (i = 1; i <= sLen; i++)
        sWin[-i] = sWin[i - Leven];

      if (Leven)
        *sWin *= (MYFLT) (I * sin(HALFPI/(double) I) / (HALFPI));
      for (i = 1; i <= sLen; i++)
        sWin[i] *= (MYFLT)(I * sin(PI * ((double) i + 0.5 * (double) Leven)
                                   / (double) I)
                           / (PI * ((double) i + 0.5 * (double) Leven)));
      for (i = 1; i <= sLen; i++)
        sWin[i] = sWin[i - Leven];

      sum = FL(1.0) / sum;

      for (i = -sLen; i <= sLen; i++)
        sWin[i] *= sum;
    }

    /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + aLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

    if (UNLIKELY((ibuf1 =
                  (MYFLT *) csound->Calloc(csound,
                                           ibuflen * sizeof(MYFLT))) == NULL)) {
      ERR("dnoise: insufficient memory\n");
    }
    if (UNLIKELY((ibuf2 =
                  (MYFLT *) csound->Calloc(csound,
                                           ibuflen * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if (UNLIKELY((obuf1 =
                  (MYFLT*) csound->Calloc(csound,
                                          obuflen * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }
    if (UNLIKELY((obuf2 =
                  (MYFLT*) csound->Calloc(csound,
                                          obuflen * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. */

    if (UNLIKELY((fbuf =
                  (MYFLT*) csound->Calloc(csound, Np2 * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

/* noise reduction: calculate noise reference by taking as many
        consecutive FFT's as possible in noise soundfile, and
        averaging them all together.  Multiply by th*th to
        establish threshold for noise-gating in each bin. */

    if (UNLIKELY((nref =
                  (MYFLT*) csound->Calloc(csound,
                                          (N2 + 1) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    if (UNLIKELY((mbuf =
                  (MYFLT*) csound->Calloc(csound,
                                          (m * Np2) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }
    if (UNLIKELY((nbuf =
                  (MYFLT*) csound->Calloc(csound,
                                          (m * Np2) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }
    if (UNLIKELY((rsum =
                  (MYFLT*) csound->Calloc(csound,
                                          (N2 + 1) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }
    if (UNLIKELY((ssum =
                  (MYFLT*) csound->Calloc(csound,
                                          (N2 + 1) * sizeof(MYFLT))) == NULL)) {
      ERR(Str("dnoise: insufficient memory\n"));
    }

    /* skip over nMin samples */
    while (nMin > (int64_t)ibuflen) {
      if (UNLIKELY(!csound->CheckEvents(csound)))
        csound->LongJmp(csound, 1);
      nread = (csound->GetUtility(csound))->Sndin(csound, fp, ibuf1, ibuflen, pn);
      for(i=0; i < nread; i++)
        ibuf1[i] *= 1.0/csound->Get0dBFS(csound);
      if (UNLIKELY(nread < ibuflen)) {
        ERR(Str("dnoise: begin time is greater than EOF of noise file!"));
      }
      nMin -= (int64_t) ibuflen;
    }
    if (UNLIKELY(!csound->CheckEvents(csound)))
      csound->LongJmp(csound, 1);
    i = (int32_t) nMin;
    nread = (csound->GetUtility(csound))->Sndin(csound, fp, ibuf1, i, pn);
    for(i=0; i < nread; i++)
        ibuf1[i] *= 1.0/csound->Get0dBFS(csound);
    if (UNLIKELY(nread < i)) {
      ERR(Str("dnoise: begin time is greater than EOF of noise file!"));
    }
    k = 0;
    lj = Beg;  /* single channel only */
    while (lj < End) {
      if (UNLIKELY(!csound->CheckEvents(csound)))
        csound->LongJmp(csound, 1);
      lj += (int64_t) N;
      nread = (csound->GetUtility(csound))->Sndin(csound, fp, fbuf, N, pn);
      for(i=0; i < nread; i++)
        fbuf[i] *= 1.0/csound->Get0dBFS(csound);
      if (nread < N)
        break;

      fbuf[N] = FL(0.0);
      fbuf[N + 1] = FL(0.0);

      //fast(csound, fbuf, N);
      fast2(csound, fftsetup_fwd, fbuf);

      for (i = 0; i <= N+1; i++)
        fbuf[i]  *= Ninv;

      i0 = fbuf;
      i1 = i0 + 1;
      for (i = 0; i <= N2; i++, i0 += 2, i1 += 2) {
        fac = fbuf[2*i] * fbuf[2*i];
        fac += fbuf[2*i+1] * fbuf[2*i+1];
        nref[i] += fac;
      }
      k++;
    }
    if (UNLIKELY(k == 0)) {
      ERR(Str("dnoise: not enough samples of noise reference\n"));
    }
    fac = th * th / k;
    for (i = 0; i <= N2; i++)
      nref[i] *= fac;                   /* nref[i] *= fac; */

    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time equals input time. */

    /* zero ibuf1 to start */
    memset(ibuf1, '\0', ibuflen*sizeof(MYFLT));
    /* f = ibuf1; */
    /* for (i = 0; i < ibuflen; i++, f++) */
    /*   *f = FL(0.0); */
    if (UNLIKELY(!csound->CheckEvents(csound)))
      csound->LongJmp(csound, 1);
    /* fill ibuf2 to start */
    nread = (csound->GetUtility(csound))->Sndin(csound, inf, ibuf2, ibuflen, p);
/*     nread = read(inf, ibuf2, ibuflen*sizeof(MYFLT)); */
/*     nread /= sizeof(MYFLT); */
    for(i=0; i < nread; i++)
        ibuf2[i] *= 1.0/csound->Get0dBFS(csound);
    lnread = nread;
    memset(ibuf2+nread, '\0', (ibuflen-nread)*sizeof(MYFLT));
    /* f = ibuf2 + nread; */
    /* for (i = nread; i < ibuflen; i++, f++) */
    /*   *f = FL(0.0); */

    //rIn = ((MYFLT) R / D);
    //rOut = ((MYFLT) R / I);
    invR = FL(1.0) / R;
    nI = -((int64_t)aLen / D) * D;    /* input time (in samples) */
    nO = nI;                 /* output time (in samples) */
    ibs = (int32_t) (ibuflen + Chans * (nI - aLen - 1));    /* starting position in ib1 */
    ib1 = ibuf1;        /* filled with zeros to start */
    ib2 = ibuf2;        /* first buffer of speech */
    obs = (int32_t)(Chans * (nO - sLen - 1));    /* starting position in ob1 */
    while (obs < 0) {
      obs += obuflen;
      first++;
    }
    ob1 = obuf1;        /* filled with garbage to start */
    ob2 = obuf2;        /* first output buffer */
    nImodR = nI;        /* for reporting progress */
    mi = 0;
    mj = m - md;
    if (mj >= m)
      mj = 0;
    mp = mi * Np2;

    nMax =  (int64_t)(input_dur * R);          /* Do it all */
    nMaxOut = (int64_t) (nMax * Chans);
    while (nI < (nMax + aLen)) {

      time = nI * invR;

      for (chan = 0; chan < Chans; chan++) {

    /* prepare for analysis: always begin reading from ib1 */
    /*                         always begin writing to ob1 */

        if (ibs >= ibuflen) {    /* done reading from ib1 */
          if (UNLIKELY(!csound->CheckEvents(csound)))
            csound->LongJmp(csound, 1);
          /* swap buffers */
          ib0 = ib1;
          ib1 = ib2;
          ib2 = ib0;
          ibs -= ibuflen;
          /* fill ib2 */
          nread = (csound->GetUtility(csound))->Sndin(csound, inf, ib2, ibuflen, p);
          for(i=0; i < nread; i++)
               ib2[i] *= 1.0/csound->Get0dBFS(csound);
          lnread += nread;
          memset(ib2+nread, '\0', (ibuflen-nread)*sizeof(MYFLT));
        /*   f = ib2 + nread; */
        /*   for (i = nread; i < ibuflen; i++, f++) */
        /*     *f = FL(0.0); */
        }
        ibc = ibs + chan;
        ibp = ib1 + ibs + chan;

        if (obs >= obuflen) {    /* done writing to ob1 */
          /* dump ob1 (except at beginning) */
          if (first > 0) {
            first--;
          }
          else {
            if ((oCnt + obuflen) < nMaxOut) {
              oCnt += writebuffer(csound, outfd, ob1, obuflen, &nrecs, &O);
            }
            else {
              i = (int32_t) (nMaxOut - oCnt);
              oCnt += writebuffer(csound, outfd, ob1, i, &nrecs, &O);
            }
          }
          /* zero ob1 */
          memset(ob1, '\0', ibuflen*sizeof(MYFLT));
          /* f = ob1; */
          /* for (i = 0; i < obuflen; i++, f++) */
          /*   *f = FL(0.0); */
          /* swap buffers */
          ob0 = ob1;
          ob1 = ob2;
          ob2 = ob0;
          obs -= obuflen;
        }
        obc = obs + chan;
        obp = ob1 + obs + chan;

    /* analysis: The analysis subroutine computes the complex output at
        time n of (N/2 + 1) of the phase vocoder channels.  It operates
        on input samples (n - aLen) thru (n + aLen).
        It expects aWin to point to the center of a
        symmetric window of length (2 * aLen + 1).  It is the
        responsibility of the main program to ensure that these values
        are correct.  The results are returned in fbuf as succesive
        pairs of real and imaginary values for the lowest (N/2 + 1)
        channels.   The subroutine fast implements an
        efficient FFT call for a real input sequence.  */

        memset(fbuf, '\0', (N+2)*sizeof(MYFLT));
        /* f = fbuf; */
        /* for (i = 0; i < N+2; i++, f++) */
        /*   *f = FL(0.0); */

        lk = nI - (int64_t) aLen - 1;            /*time shift*/
        while ((int64_t) lk < 0L)
          lk += (int64_t) N;
        k = (int32_t) (lk % (int64_t) N);

        f = fbuf + k;
        w = aWin - aLen;
        for (i = -aLen; i <= aLen; i++, k++, f++, w++) {
          ibp += Chans;
          ibc += Chans;
          if (ibc >= ibuflen) {
            ibc = chan;
            ibp = ib2 + chan;
          }
          if (k >= N) {
            k = 0;
            f = fbuf;
          }
          *f += *w * *ibp;
        }

        //fast(csound, fbuf, N);
        fast2(csound, fftsetup_fwd, fbuf);

        /* noise reduction: for each bin, calculate average magnitude-squared
            and calculate corresponding gain.  Apply this gain to delayed
            FFT values in mbuf[mj*Np2 + i?]. */

        if (chan == 0) {
          f = rsum;
          i0 = mbuf + mp;
          i1 = i0 + 1;
          j0 = mbuf + mj * Np2;
          j1 = j0 + 1;
          f0 = fbuf;
          f1 = f0 + 1;
          for (i = 0; i <= N2;
               i++, f++, i0+=2, i1+=2, f0+=2, f1+=2, j0+=2, j1+=2) {
            /*
             *  ii0 = 2 * i; // better as in by 2 or shift?
             *  ii1 = ii0 + 1;
             *
             *  rsum[i] -= mbuf[mp + ii0] * mbuf[mp + ii0];
             *  rsum[i] -= mbuf[mp + ii1] * mbuf[mp + ii1];
             *  rsum[i] += fbuf[ii0] * fbuf[ii0];
             *  rsum[i] += fbuf[ii1] * fbuf[ii1];
             */
            *f -= *i0 * *i0;
            *f -= *i1 * *i1;
            *f += *f0 * *f0;
            *f += *f1 * *f1;
            avg = minv * *f;        /* avg = minv * rsum[i]; */
            if (avg < FL(0.0))
              avg = FL(0.0);
            if (avg == FL(0.0))
              fac = FL(0.0);
            else
              fac = avg / (avg + nref[i]);
            for (j = 1; j < sh; j++)
              fac *= fac;
            gain = g0m * fac + g0;
            /*
             * mbuf[mp + ii0] = fbuf[ii0];
             * mbuf[mp + ii1] = fbuf[ii1];
             * fbuf[ii0] = gain * mbuf[mj*Np2 + ii0];
             * fbuf[ii1] = gain * mbuf[mj*Np2 + ii1];
             */
            *i0 = *f0;
            *i1 = *f1;
            *f0 = gain * *j0;
            *f1 = gain * *j1;
          }
        }
        else {
          f = ssum;
          i0 = nbuf + mp;
          i1 = i0 + 1;
          j0 = nbuf + mj * Np2;
          j1 = j0 + 1;
          f0 = fbuf;
          f1 = f0 + 1;
          for (i = 0; i <= N2;
               i++, f++, i0+=2, i1+=2, f0+=2, f1+=2, j0+=2, j1+=2) {
            /*
             *  ii0 = 2 * i;
             *  ii1 = ii0 + 1;
             *
             * ssum[i] -= nbuf[mp + ii0] * nbuf[mp + ii0];
             * ssum[i] -= nbuf[mp + ii1] * nbuf[mp + ii1];
             * ssum[i] += fbuf[ii0] * fbuf[ii0];
             * ssum[i] += fbuf[ii1] * fbuf[ii1];
             */
            *f -= *i0 * *i0;
            *f -= *i1 * *i1;
            *f += *f0 * *f0;
            *f += *f1 * *f1;
            avg = minv * *f;      /* avg = minv * ssum[i]; */
            if (avg < FL(0.0))
              avg = FL(0.0);
            if (avg == FL(0.0))
              fac = FL(0.0);
            else
              fac = avg / (avg + nref[i]);
            for (j = 1; j < sh; j++)
              fac *= fac;
            gain = g0m * fac + g0;
            /*
             * nbuf[mp + ii0] = fbuf[ii0];
             * nbuf[mp + ii1] = fbuf[ii1];
             * fbuf[ii0] = gain * nbuf[mj*Np2 + ii0];
             * fbuf[ii1] = gain * nbuf[mj*Np2 + ii1];
             */
            *i0 = *f0;
            *i1 = *f1;
            *f0 = gain * *j0;
            *f1 = gain * *j1;
          }
        }

        if (chan == (Chans - 1)) {
          if (++mi >= m)
            mi = 0;
          if (++mj >= m)
            mj = 0;
          mp = mi * Np2;
        }

    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
        technique to reconstruct the time-domain signal.  The (N/2 + 1)
        phase vocoder channel outputs at time n are inverse Fourier
        transformed, windowed, and added into the output array. */

        fsst2(csound, fftsetup_inv, fbuf);
        //fsst(csound, fbuf, N);

        lk = nO - (int64_t) sLen - 1;            /*time shift*/
        while (lk < 0)
          lk += (int64_t) N;
        k = (int32_t) (lk % (int64_t) N);

        f = fbuf + k;
        w = sWin - sLen;
        for (i = -sLen; i <= sLen; i++, k++, f++, w++) {
          obp += Chans;
          obc += Chans;
          if (obc >= obuflen) {
            obc = chan;
            obp = ob2 + chan;
          }
          if (k >= N) {
            k = 0;
            f = fbuf;
          }
          *obp += *w * *f;
        }

        if (flag) {
          if (nread < ibuflen) { /* EOF detected */
            flag = 0;
            if ((lnread / Chans) < nMax)
              nMax = (lnread / Chans);
          }
        }

      }

      ibs += (Chans * D);            /* starting point in ibuf */
      obs += (Chans * I);            /* starting point in obuf */

      nI += (int64_t) D;                /* increment time */
      nO += (int64_t) I;

      if (Verbose) {
        nImodR += D;
        if (nImodR > (int64_t) R) {
          nImodR -= (int64_t) R;
          csound->Message(csound,
                          Str("%5.1f seconds of input complete\n"),(time+D*invR));
        }
      }

    }

    nMaxOut = (int64_t) (nMax * Chans);
    i = (int32_t) (nMaxOut - oCnt);
    if (i > obuflen) {
      writebuffer(csound, outfd, ob1, obuflen, &nrecs, &O);
      i -= obuflen;
      ob1 = ob2;
    }
    if (i > 0)
      writebuffer(csound, outfd, ob1, i, &nrecs, &O);

/*  csound->RewriteHeader(outfd); */
    csound->Message(csound, "\n\n");
    if (Verbose) {
      csound->Message(csound, "%s", Str("processing complete\n"));
      csound->Message(csound, "N = %d\n", N);
      csound->Message(csound, "M = %d\n", M);
      csound->Message(csound, "L = %d\n", L);
      csound->Message(csound, "D = %d\n", D);
    }
    return 0;
}

static const char *usage_txt[] = {
  Str_noop("usage: dnoise [flags] input_file"),
    "",
  Str_noop("flags:"),
  Str_noop("i = noise reference soundfile"),
  Str_noop("o = output file"),
  Str_noop("N = # of bandpass filters (1024)"),
  Str_noop("w = filter overlap factor: {0,1,(2),3} DO NOT USE -w AND -M"),
  Str_noop("M = analysis window length (N-1 unless -w is specified)"),
  Str_noop("L = synthesis window length (M)"),
  Str_noop("D = decimation factor (M/8)"),
  Str_noop("b = begin time in noise reference soundfile (0)"),
  Str_noop("B = starting sample in noise reference soundfile (0)"),
  Str_noop("e = end time in noise reference soundfile (end)"),
  Str_noop("E = final sample in noise reference soundfile (end)"),
  Str_noop("t = threshold above noise reference in dB (30)"),
  Str_noop("S = sharpness of noise-gate turnoff (1) (1 to 5)"),
  Str_noop("n = number of FFT frames to average over (5)"),
  Str_noop("m = minimum gain of noise-gate when off in dB (-40)"),
  Str_noop("V : verbose - print status info"),
  Str_noop("A : AIFF format output"),
  Str_noop("W : WAV format output"),
  Str_noop("J : IRCAM format output"),
    NULL
};

static int32_t dnoise_usage(CSOUND *csound, int32_t exitcode)
{
    const char  **sp;

    for (sp = &(usage_txt[0]); *sp != NULL; sp++)
      csound->Message(csound, "%s\n", Str(*sp));

    return exitcode;
}

/* report soundfile write(osfd) error      */
/*    called after chk of write() bytecnt  */

static void sndwrterr(CSOUND *csound, int32_t nret, int32_t nput)
{
    csound->Message(csound, Str("soundfile write returned sample count of %d, "
                                "not %d\n"), nret, nput);
    csound->Message(csound, "%s", Str("(disk may be full...\n"
                                " closing the file ...)\n"));
    /* FIXME: should clean up */
    //csound->Die(csound, "%s", Str("\t... closed\n"));
}

static int32_t writebuffer(CSOUND *csound, SNDFILE *outfd,
                       MYFLT *outbuf, int32_t nsmps, int32_t *nrecs, OPARMS *O)
{
    int32_t n;

    if (UNLIKELY(outfd == NULL)) return 0;
    n = (int32_t) csound->SndfileWriteSamples(csound, outfd, outbuf, nsmps);

    if (UNLIKELY(n < nsmps)) {
      csound->SndfileClose(csound,outfd);
      sndwrterr(csound, n, nsmps);
      return -1;
    }
    if (UNLIKELY(O->rewrt_hdr))
      csound->RewriteHeader(csound, outfd);

    (*nrecs)++;                 /* JPff fix */
    switch (O->heartbeat) {
    case 1:
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[*nrecs & 3]);
      break;
    case 2:
      csound->MessageS(csound, CSOUNDMSG_REALTIME, ".");
      break;
    case 3:
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%d%n", *nrecs, &n);
      while (n--) csound->MessageS(csound, CSOUNDMSG_REALTIME, "\b");
      break;
    case 4:
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "\a");
      break;
    }

    return nsmps;
}

static void hamming(MYFLT *win, int32_t winLen, int32_t even)
{
    double  ftmp;
    int32_t i;

    ftmp = PI / winLen;

    if (even) {
      for (i = 0; i < winLen; i++)
        win[i] = (MYFLT) (0.54 + 0.46 * cos(ftmp * ((double)i + 0.5)));
      win[winLen] = FL(0.0);
    }
    else {
      win[0] = FL(1.0);
      for (i = 1; i <= winLen; i++)
        win[i] = (MYFLT) (0.54 + 0.46 * cos(ftmp * (double)i));
    }
}

/* module interface */

int32_t dnoise_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "dnoise", dnoise);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "dnoise",
                                      Str("Removes noise from a sound file"));
    }
    return retval;
}
