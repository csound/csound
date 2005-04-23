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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

#include "csdl.h"
#include "soundio.h"
#include <math.h>
#include <ctype.h>

#define ERR(x)                          \
{                                       \
    csound->Message(csound, Str(x));    \
    goto err_return;                    \
}

#define FIND(x)                                                     \
{                                                                   \
    if (*s == '\0') {                                               \
      if (!(--argc) || (((s = *argv++) != NULL) && *s == '-')) {    \
        csound->Message(csound, "%s\n", Str(x));                    \
        return dnoise_usage(csound, -1);                            \
      }                                                             \
    }                                                               \
}

static  int     dnoise_usage(ENVIRON *, int);
static  void    hamming(MYFLT *, int, int);
static  char    *getStrFormat(ENVIRON *, int);
static  int     sfSampSize(int);
static  char    *type2string(int);

static int writebuffer(ENVIRON *, SNDFILE *, MYFLT *, int);

static void fast(ENVIRON *csound, MYFLT *b, int N)
{
  /* The DC term is returned in location b[0] with b[1] set to 0.
     Thereafter, the i'th harmonic is returned as a complex
     number stored as b[2*i] + j b[2*i+1].  The N/2 harmonic
     is returned in b[N] with b[N+1] set to 0.  Hence, b must
     be dimensioned to size N+2.  The subroutine is called as
     fast(b,N) where N=2**M and b is the real array described
     above.
  */

    csound->RealFFT(csound, b, N);
    b[N] = b[1];
    b[1] = b[N + 1] = FL(0.0);
}

static void fsst(ENVIRON *csound, MYFLT *b, int N)
{
  /* This subroutine synthesizes the real vector b[k] for k=0, 1,
     ..., N-1 from the fourier coefficients stored in the b
     array of size N+2.  The DC term is in location b[0] with
     b[1] equal to 0.  The i'th harmonic is a complex number
     stored as b[2*i] + j b[2*i+1].  The N/2 harmonic is in
     b[N] with b[N+1] equal to 0. The subroutine is called as
     fsst(b,N) where N=2**M and b is the real array described
     above.
  */
    MYFLT scaleVal;
    int   i;

    scaleVal = csound->GetInverseRealFFTScale(csound, N);
    b[1] = b[N];
    b[N] = b[N + 1] = FL(0.0);
    for (i = 0; i < N; i++)
      b[i] *= scaleVal;
    csound->InverseRealFFT(csound, b, N);
}

static int dnoise(void *csound_, int argc, char **argv)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    OPARMS  *O = csound->oparms;

    MYFLT   beg = -FL(1.0), end = -FL(1.0);
    long    Beg = 0, End = 99999999;

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

    int
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

    long
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
        RoverTwoPi, /* R/D divided by 2*Pi */
        TwoPioverR, /* 2*Pi divided by R/I */
        sum,        /* scale factor for renormalizing windows */
        rIn,        /* decimated sampling rate */
        rOut,       /* pre-interpolated sampling rate */
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

    int    i,j,k,   /* index variables */
        Dd,         /* number of new inputs to read (Dd <= D) */
        Ii,         /* number of new outputs to write (Ii <= I) */
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
    char        *infile = NULL, *outfile = NULL, *nfile = NULL;
    SNDFILE     *inf = NULL, *outfd = NULL;
    char        c, *s;
    int         channel = ALLCHNLS;
    MYFLT       beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    MYFLT       beg_ntime = FL(0.0), input_ndur = FL(0.0), srn = FL(0.0);
    char        *envoutyp = NULL;
    unsigned int  outbufsiz = 0U;
    char        outformch = 's';

    csound->e0dbfs = csound->dbfs_to_float = FL(1.0);

    O->filnamspace = outfile = (char*) csound->Calloc(csound, (size_t) 1024);
    nfile = (char*) csound->Calloc(csound, (size_t) 1024);
    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp,"AIFF") == 0)
        O->filetyp = TYP_AIFF;
      else if (strcmp(envoutyp,"WAV") == 0)
        O->filetyp = TYP_WAV;
      else if (strcmp(envoutyp,"IRCAM") == 0)
        O->filetyp = TYP_IRCAM;
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
            case 'j':
              FIND("");
              while (*++s);
              break;
            case 'o':
              FIND("no outfilename");
                O->outfilename = outfile;            /* soundout name */
              while ((*outfile++ = *s++)); s--;
              if (strcmp(O->outfilename, "stdin") == 0) {
                csound->Message(csound, Str("-o cannot be stdin\n"));
                return -1;
              }
              if (strcmp(O->outfilename, "stdout") == 0)
                O->stdoutfd = 1;
              break;
            case 'i':
              FIND("no noisefilename");
              {
                char *nn = nfile;
                while ((*nn++ = *s++)); s--;
              }
              break;
            case 'A':
              if (O->filetyp == TYP_WAV)
                csound->Warning(csound,
                                Str("-A overriding local default WAV out"));
              O->filetyp = TYP_AIFF;    /* AIFF output request*/
              break;
            case 'J':
              if (O->filetyp == TYP_AIFF || O->filetyp == TYP_WAV)
                csound->Warning(csound, Str("-J overriding local default "
                                            "AIFF/WAV out"));
              O->filetyp = TYP_IRCAM;   /* IRCAM output request */
              break;
            case 'W':
              if (O->filetyp == TYP_AIFF)
                csound->Warning(csound,
                                Str("-W overriding local default AIFF out"));
              O->filetyp = TYP_WAV;      /* WAV output request */
              break;
            case 'h':
              O->filetyp = TYP_RAW;
              O->sfheader = 0;           /* skip sfheader  */
              break;
            case 'c':
              outformch = c;
              O->outformat = AE_CHAR;     /* 8-bit char soundfile */
              break;
            case '8':
              outformch = c;
              O->outformat = AE_UNCH;     /* 8-bit unsigned char file */
              break;
            case 'a':
              outformch = c;
              O->outformat = AE_ALAW;     /* a-law soundfile */
              break;
            case 'u':
              outformch = c;
              O->outformat = AE_ULAW;     /* mu-law soundfile */
              break;
            case 's':
              outformch = c;
              O->outformat = AE_SHORT;    /* short_int soundfile */
              break;
            case 'l':
              outformch = c;
              O->outformat = AE_LONG;     /* long_int soundfile */
              break;
            case 'f':
              outformch = c;
              O->outformat = AE_FLOAT;    /* float soundfile */
              break;
            case 'R':
              O->rewrt_hdr = 1;
              break;
            case 'H':
              if (isdigit(*s)) {
                int n;
                sscanf(s, "%d%n", &O->heartbeat, &n);
                s += n;
              }
              else O->heartbeat = 1;
              break;
            case 't':
              FIND("no t arg");
#if defined(USE_DOUBLE)
              sscanf(s,"%lf",&th);
#else
              sscanf(s,"%f",&th);
#endif
              while (*++s);
              break;
            case 'S':
              FIND("no s arg");
              sscanf(s,"%d", &sh);
              while (*++s);
              break;
            case 'm':
              FIND("no m arg");
#if defined(USE_DOUBLE)
              sscanf(s,"%lf",&g0);
#else
              sscanf(s,"%f",&g0);
#endif
              while (*++s);
              break;
            case 'n':
              FIND("no n arg");
              sscanf(s,"%d", &m);
              while (*++s);
              break;
            case 'b':
              FIND("no b arg");
#if defined(USE_DOUBLE)
              sscanf(s,"%lf",&beg);
#else
              sscanf(s,"%f",&beg);
#endif
              while (*++s);
              break;
            case 'B': FIND("no B arg");
              sscanf(s,"%ld", &Beg);
              while (*++s);
              break;
            case 'e': FIND("no e arg");
#if defined(USE_DOUBLE)
              sscanf(s,"%lf",&end);
#else
              sscanf(s,"%f",&end);
#endif
              while (*++s);
              break;
            case 'E': FIND("no E arg");
              sscanf(s,"%ld", &End);
              while (*++s);
              break;
            case 'N': FIND("no N arg");
              sscanf(s,"%d", &N);
              while (*++s);
              break;
            case 'M': FIND("no M arg");
              sscanf(s,"%d", &M);
              while (*++s);
              break;
            case 'L': FIND("no L arg");
              sscanf(s,"%d", &L);
              while (*++s);
              break;
            case 'w': FIND("no w arg");
              sscanf(s,"%d", &W);
              while (*++s);
              break;
            case 'D': FIND("no D arg");
              sscanf(s,"%d", &D);
              while (*++s);
              break;
            case 'V':
              Verbose = 1; break;
            default:
              csound->Message(csound, "Looking at %c\n", c);
              return dnoise_usage(csound, -1);  /* this exits with error */
            }
          }
        }
        else if (infile==NULL) {
          infile = --s;
          csound->Message(csound, "Infile set to %s\n", infile);
        }
        else {
          csound->Message(csound, "End with %s\n", s);
          return dnoise_usage(csound, -1);
        }
      }
    }
    if (infile == NULL) {
      csound->Message(csound, "dnoise: no input file\n");
      return dnoise_usage(csound, -1);
    }
    if (nfile == NULL) {
      csound->Message(csound, "Must have an example noise file (-i name)\n");
      return -1;
    }
    if ((inf = csound->SAsndgetset(csound, infile, &p, &beg_time,
                                   &input_dur, &sr, channel)) < 0) {
      csound->Message(csound, Str("error while opening %s"), infile);
      return -1;
    }
    if (O->outformat == 0) O->outformat = p->format;
    O->sfsampsize = sfSampSize(O->outformat);
    if (O->filetyp == TYP_RAW) {
      O->sfheader = 0;
      O->rewrt_hdr = 0;
    }
    else
      O->sfheader = 1;
    if (O->outfilename == NULL)
      O->outfilename = "test";
    {
      SF_INFO sfinfo;
      char    *name;
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) p->sr;
      sfinfo.channels = (int) p->nchanls;
      sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
      if (strcmp(O->outfilename, "stdout") != 0) {
        name = csound->FindOutputFile(csound, O->outfilename, "SFDIR");
        if (name == NULL) {
          csound->Message(csound, Str("cannot open %s.\n"), O->outfilename);
          goto err_return;
        }
        outfd = sf_open(name, SFM_WRITE, &sfinfo);
        csound->Free(csound, name);
      }
      else
        outfd = sf_open_fd(O->stdoutfd, SFM_WRITE, &sfinfo, 1);
      if (outfd == NULL) {
        csound->Message(csound, Str("cannot open %s."), O->outfilename);
        goto err_return;
      }
      sf_command(outfd, SFC_SET_CLIPPING, NULL, SF_TRUE);
    }
    csound->esr = (MYFLT) p->sr;
    csound->nchnls = Chans = p->nchanls;

    /* read header info */
    if (R < FL(0.0))
      R = (MYFLT)p->sr;
    if (Chans < 0)
      Chans = (int) p->nchanls;
    p->nchanls = Chans;

    if (Chans > 2) {
      csound->Message(csound, "dnoise: input MUST be mono or stereo\n");
      goto err_return;
    }

    /* read noise reference file */

    if ((fp = csound->SAsndgetset(csound, nfile, &pn, &beg_ntime,
                                  &input_ndur, &srn, channel)) < 0) {
      csound->Message(csound, "dnoise: cannot open noise reference file\n");
      goto err_return;
    }

    if (sr != srn) {
      csound->Message(csound, "Incompatible sample rates\n");
      goto err_return;
    }
    /* calculate begin and end times in NOISE file */
    if (beg >= FL(0.0)) Beg = (long) (beg * R);
    if (end >= FL(0.0)) End = (long) (end * R);
    else if (End == 99999999) End = (long) (input_ndur * R);

    nMin = Beg * Chans;            /* total number of samples to skip */
    nMax = End - Beg;            /* number of samples per channel to process */

    /* largest valid FFT size is 8192 */
    if (N == 0)
      N = 1024;
    for (i = 1; i < 4096; i *= 2)
      if (i >= N)
        break;
    if (i != N)
      csound->Message(csound,
                       "dnoise: warning - N not a valid power of two; "
              "revised N = %d\n",i);
    N = i;
    N2 = N / 2;
    Np2 = N + 2;
    Ninv = FL(1.0) / N;

    if (W != -1) {
      if (M != 0)
        csound->Message(csound,
                        "dnoise: warning - don't specify both M and W\n");
      else if (W == 0)
        M = 4*N;
      else if (W == 1)
        M = 2*N;
      else if (W == 2)
        M = N;
      else if (W == 3)
        M = N2;
      else
        csound->Message(csound, "dnoise: warning - invalid W ignored\n");
    }

    if (M == 0)
      M = N;
    if ((M%2) == 0)
      Meven = 1;

    if (L == 0)
      L = M;
    if ((L%2) == 0)
      Leven = 1;

    if (M < 7)
      csound->Message(csound, "dnoise: warning - M is too small\n");

    if (D == 0)
      D = M / 8;

    I = D;

    lj = (long) M + 3 * (long) D;
    lj *= (long) Chans;
    if (lj > 32767) {
      csound->Message(csound, "dnoise: M too large\n");
      goto err_return;
    }
    lj = (long) L + 3 * (long) I;
    lj *= (long) Chans;
    if (lj > 32767) {
      csound->Message(csound, "dnoise: L too large\n");
      goto err_return;
    }

    ibuflen = Chans * (M + 3 * D);
    obuflen = Chans * (L + 3 * I);
    outbufsiz = obuflen * sizeof(MYFLT);                 /* calc outbuf size */
#if 0
    outbuf = csound->Malloc(csound, (size_t) outbufsiz); /* & alloc bufspace */
#endif
    csound->Message(csound, Str("writing %u-byte blks of %s to %s"),
                    outbufsiz, getStrFormat(csound, O->outformat),
                    O->outfilename);
    csound->Message(csound, " (%s)\n", type2string(O->filetyp));
/*  spoutran = spoutsf; */

    minv = FL(1.0) / (MYFLT)m;
    md = m / 2;
    g0 = (MYFLT) pow(10.0,(double) (.05*(double)g0));
    g0m = FL(1.0) - g0;
    th = (MYFLT) pow(10.0,(double) (.05*(double)th));

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

    if ((aWin = (MYFLT*) csound->Calloc(csound,
                                        (M+Meven) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    aLen = M/2;
    aWin += aLen;

    hamming(aWin, aLen, Meven);
    for (i = 1; i <= aLen; i++) {
      aWin[-i] = aWin[i-1];
    }

    if (M > N) {
      if (Meven)
        *aWin *= (MYFLT)N * (MYFLT) sin(PI*0.5/(double)N) /( PI_F*FL(0.5));
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

    if ((sWin = (MYFLT*) csound->Calloc(csound,
                                        (L+Leven) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
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
        *sWin *= (MYFLT) (I * sin(PI*0.5/(double) I) / (PI*0.5));
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

    if ((ibuf1 = (MYFLT *) csound->Calloc(csound,
                                          ibuflen * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((ibuf2 = (MYFLT *) csound->Calloc(csound,
                                          ibuflen * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if ((obuf1 = (MYFLT*) csound->Calloc(csound,
                                         obuflen * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((obuf2 = (MYFLT*) csound->Calloc(csound,
                                         obuflen * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. */

    if ((fbuf = (MYFLT*) csound->Calloc(csound, Np2 * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

/* noise reduction: calculate noise reference by taking as many
        consecutive FFT's as possible in noise soundfile, and
        averaging them all together.  Multiply by th*th to
        establish threshold for noise-gating in each bin. */

    if ((nref = (MYFLT*) csound->Calloc(csound,
                                        (N2 + 1) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    if ((mbuf = (MYFLT*) csound->Calloc(csound,
                                        (m * Np2) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((nbuf = (MYFLT*) csound->Calloc(csound,
                                        (m * Np2) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((rsum = (MYFLT*) csound->Calloc(csound,
                                        (N2 + 1) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((ssum = (MYFLT*) csound->Calloc(csound,
                                        (N2 + 1) * sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    /* skip over nMin samples */
    while (nMin > (long) ibuflen) {
      nread = csound->getsndin(csound, fp, ibuf1, ibuflen, pn);
      if (nread < ibuflen) {
        ERR("dnoise: begin time is greater than EOF of noise file!");
      }
      nMin -= (long) ibuflen;
    }
    i = (int) nMin;
    nread = csound->getsndin(csound, fp, ibuf1, i, pn);
    if (nread < i) {
      ERR("dnoise: begin time is greater than EOF of noise file!");
    }
    k = 0;
    lj = Beg;  /* single channel only */
    while (lj < End) {
      lj += (long) N;
      nread = csound->getsndin(csound, fp, fbuf, N, pn);
      if (nread < N)
        break;

      fbuf[N] = FL(0.0);
      fbuf[N + 1] = FL(0.0);

      fast(csound, fbuf, N);

      f = fbuf;
      for (i = 0; i <= N+1; i++, f++)
        *f  *= Ninv;

      f = nref;
      i0 = fbuf;
      i1 = i0 + 1;
      for (i = 0; i <= N2; i++, f++, i0 += 2, i1 += 2) {
        fac = *i0 * *i0;        /* fac = fbuf[2*i] * fbuf[2*i]; */
        fac += *i1 * *i1;       /* fac += fbuf[2*i+1] * fbuf[2*i+1]; */
        *f += fac;              /* nref[i] += fac; */
      }
      k++;
    }
    if (k == 0) {
      ERR("dnoise: not enough samples of noise reference\n");
    }
    fac = th * th / k;
    f = nref;
    for (i = 0; i <= N2; i++, f++)
      *f *= fac;                   /* nref[i] *= fac; */


    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time equals input time. */

    /* zero ibuf1 to start */
    f = ibuf1;
    for (i = 0; i < ibuflen; i++, f++)
        *f = FL(0.0);
    /* fill ibuf2 to start */
    nread = csound->getsndin(csound, inf, ibuf2, ibuflen, p);
/*     nread = read(inf, ibuf2, ibuflen*sizeof(MYFLT)); */
/*     nread /= sizeof(MYFLT); */
    lnread = nread;
    f = ibuf2 + nread;
    for (i = nread; i < ibuflen; i++, f++)
      *f = FL(0.0);

    rIn = ((MYFLT) R / D);
    rOut = ((MYFLT) R / I);
    invR = FL(1.0) / R;
    RoverTwoPi = rIn / TWOPI_F;
    TwoPioverR = TWOPI_F / rOut;
    nI = -(aLen / D) * D;    /* input time (in samples) */
    nO = nI;                 /* output time (in samples) */
    Dd = aLen + nI + 1;      /* number of new inputs to read */
    Ii = 0;                  /* number of new outputs to write */
    ibs = ibuflen + Chans * (nI - aLen - 1);    /* starting position in ib1 */
    ib1 = ibuf1;        /* filled with zeros to start */
    ib2 = ibuf2;        /* first buffer of speech */
    obs = Chans * (nO - sLen - 1);    /* starting position in ob1 */
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

    nMax =  (long)(input_dur * R);          /* Do it all */
    nMaxOut = (long) (nMax * Chans);
    while (nI < (nMax + aLen)) {

      time = nI * invR;

      for (chan = 0; chan < Chans; chan++) {

    /* prepare for analysis: always begin reading from ib1 */
    /*                         always begin writing to ob1 */

        if (ibs >= ibuflen) {    /* done reading from ib1 */
          /* swap buffers */
          ib0 = ib1;
          ib1 = ib2;
          ib2 = ib0;
          ibs -= ibuflen;
          /* fill ib2 */
          nread = csound->getsndin(csound, inf, ib2, ibuflen, p);
          lnread += nread;
          f = ib2 + nread;
          for (i = nread; i < ibuflen; i++, f++)
            *f = FL(0.0);
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
              oCnt += writebuffer(csound, outfd, ob1, obuflen);
            }
            else {
              i = (int) (nMaxOut - oCnt);
              oCnt += writebuffer(csound, outfd, ob1, i);
            }
          }
          /* zero ob1 */
          f = ob1;
          for (i = 0; i < obuflen; i++, f++)
            *f = FL(0.0);
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


        f = fbuf;
        for (i = 0; i < N+2; i++, f++)
          *f = FL(0.0);

        lk = nI - (long) aLen - 1;            /*time shift*/
        while ((long) lk < 0L)
          lk += (long) N;
        k = (int) (lk % (long) N);

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

        fast(csound, fbuf, N);

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
             *  ii0 = 2 * i;
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

        fsst(csound, fbuf, N);

        lk = nO - (long) sLen - 1;            /*time shift*/
        while (lk < 0)
          lk += (long) N;
        k = (int) (lk % (long) N);

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

      nI += (long) D;                /* increment time */
      nO += (long) I;

      if (Verbose) {
        nImodR += D;
        if (nImodR > (long) R) {
          nImodR -= (long) R;
          csound->Message(csound,
                           "%5.1f seconds of input complete\n",(time+D*invR));
        }
      }

    }

    nMaxOut = (long) (nMax * Chans);
    i = (int) (nMaxOut - oCnt);
    if (i > obuflen) {
      writebuffer(csound, outfd, ob1, obuflen);
      i -= obuflen;
      ob1 = ob2;
    }
    if (i > 0)
      writebuffer(csound, outfd, ob1, i);

/*  rewriteheader(outfd, 0); */
    csound->Message(csound, "\n\n");
    sf_close(outfd);
    sf_close(inf);
    sf_close(fp);
    if (Verbose) {
      csound->Message(csound, "processing complete\n");
      csound->Message(csound, "N = %d\n", N);
      csound->Message(csound, "M = %d\n", M);
      csound->Message(csound, "L = %d\n", L);
      csound->Message(csound, "D = %d\n", D);
    }

    return 0;

 err_return:
    if (outfd != NULL)  sf_close(outfd);
    if (inf != NULL)    sf_close(inf);
    if (fp != NULL)     sf_close(fp);
    return -1;
}

static int dnoise_usage(ENVIRON *csound, int exitcode)
{
    csound->Message(csound,
            "usage: dnoise [flags] input_file\n"
            "\nflags:\n"
            "i = noise reference soundfile\n"
            "o = output file\n"
            "N = # of bandpass filters (1024)\n"
            "w = filter overlap factor: {0,1,(2),3} DON'T USE -w AND -M\n"
            "M = analysis window length (N-1 unless -w is specified)\n"
            "L = synthesis window length (M) \n"
            "D = decimation factor (M/8)\n"
            "b = begin time in noise reference soundfile (0)\n"
            "B = starting sample in noise reference soundfile (0)\n"
            "e = end time in noise reference soundfile (end)\n"
            "E = final sample in noise reference soundfile (end)\n"
            "t = threshold above noise reference in dB (30)\n"
            "S = sharpness of noise-gate turnoff (1) (1 to 5)\n"
            "n = number of FFT frames to average over (5)\n"
            "m = minimum gain of noise-gate when off in dB (-40)\n"
            "V: verbose - print status info\n"
            "A : AIFF format output\n"
            "W : WAV format output\n"
            "J : IRCAM format output\n"
            );

    return exitcode;
}

static void sndwrterr(ENVIRON *csound, SNDFILE *outfd,
                      int nret, int nput)
                                /* report soundfile write(osfd) error      */
{                               /*    called after chk of write() bytecnt  */
    csound->Message(csound, Str("soundfile write returned sample count of %d, "
                                "not %d\n"), nret, nput);
    csound->Message(csound, Str("(disk may be full...\n"
                                " closing the file ...)\n"));
    sf_close(outfd);            /* & try to close the file */
    /* FIXME: should clean up */
    csound->Die(csound, Str("\t... closed\n"));
}

static int writebuffer(ENVIRON *csound, SNDFILE *outfd,
                       MYFLT *outbuf, int nsmps)
{
    OPARMS  *O = csound->oparms;
    int     n;
    if (outfd == NULL) return 0;
    n = sf_write_MYFLT(outfd, outbuf, nsmps);
    if (n < nsmps) {
      sndwrterr(csound, outfd, n, nsmps);
      return -1;
    }
    if (O->rewrt_hdr)
      csound->rewriteheader(outfd, 0);
    csound->nrecs++;           /* JPff fix */
    if (O->heartbeat) {
      if (O->heartbeat==1) {
        csound->Message(csound, "%c\010", "|/-\\"[csound->nrecs & 3]);
      }
      else if (O->heartbeat == 2)
        csound->Message(csound, ".");
      else if (O->heartbeat == 3) {
        int n;
        csound->Message(csound, "%ld(%.3f)%n",
                                (long) csound->nrecs,
                                (MYFLT) csound->nrecs / csound->ekr, &n);
        while (n--) csound->Message(csound,"\b");
      }
      else csound->Message(csound,"\a");
    }
    return nsmps;
}

static char *getStrFormat(ENVIRON *csound, int format)
{
    switch (format) {
      case  AE_UNCH:    return Str("unsigned bytes");
      case  AE_CHAR:    return Str("signed chars");
      case  AE_ALAW:    return Str("alaw bytes");
      case  AE_ULAW:    return Str("ulaw bytes");
      case  AE_SHORT:   return Str("shorts");
      case  AE_LONG:    return Str("longs");
      case  AE_FLOAT:   return Str("floats");
      case  AE_24INT:   return Str("24bit ints");
    }
    return Str("unknown");
}

static int sfSampSize(int type)
{
    switch (type & SF_FORMAT_SUBMASK) {
      case SF_FORMAT_PCM_16:    return 2;       /* Signed 16 bit data */
      case SF_FORMAT_PCM_24:    return 3;       /* Signed 24 bit data */
      case SF_FORMAT_PCM_32:                    /* Signed 32 bit data */
      case SF_FORMAT_FLOAT:     return 4;       /* 32 bit float data */
      case SF_FORMAT_DOUBLE:    return 8;       /* 64 bit float data */
    }
    return 1;
}

static char *type2string(int x)
{
    switch (x) {
      case TYP_WAV:     return "WAV";
      case TYP_AIFF:    return "AIFF";
      case TYP_AU:      return "AU";
      case TYP_RAW:     return "RAW";
      case TYP_PAF:     return "PAF";
      case TYP_SVX:     return "SVX";
      case TYP_NIST:    return "NIST";
      case TYP_VOC:     return "VOC";
      case TYP_IRCAM:   return "IRCAM";
      case TYP_W64:     return "W64";
      case TYP_MAT4:    return "MAT4";
      case TYP_MAT5:    return "MAT5";
      case TYP_PVF:     return "PVF";
      case TYP_XI:      return "XI";
      case TYP_HTK:     return "HTK";
#ifdef SF_FORMAT_SDS
      case TYP_SDS:     return "SDS";
#endif
      default:          return "(unknown)";
    }
}

static void hamming(MYFLT *win, int winLen, int even)
{
    double  ftmp;
    int     i;

    ftmp = PI / winLen;

    if (even) {
      for (i = 0; i < winLen; i++)
        win[i] = (MYFLT) (0.54 + 0.46 * cos(ftmp * ((double) i + 0.5)));
      win[winLen] = FL(0.0);
    }
    else {
      win[0] = FL(1.0);
      for (i = 1; i <= winLen; i++)
        win[i] = (MYFLT) (0.54 + 0.46 * cos(ftmp * (double) i));
    }
}

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    return (((ENVIRON*) csound)->AddUtility(csound, "dnoise", dnoise));
}

