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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "cs.h"
#include "sfheader.h"
#include "soundio.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern ENVIRON cenviron;

#define ERR(x)          perror(x); exit(1)
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || (((s = *argv++) != NULL) && *s == '-')) \
                            dieu(MSG);

void dnoise_usage(int);
void fast(MYFLT*, int);
void hamming(MYFLT *, int, int);
void fsst(MYFLT *, int);
#ifdef mills_macintoshxx
extern void do_mac_dialogs(void);
extern char *gargv[];
extern int gargc;
#endif

extern int  SAsndgetset(char *, SOUNDIN**, MYFLT*, MYFLT*, MYFLT*, int);
extern long getsndin(int, MYFLT *, long, SOUNDIN *);
extern void bytrev2(char *, int), bytrev4(char *, int), rewriteheader(SNDFILE *,int);
extern int  openout(char *, int), bytrevhost(void);
extern short sfsampsize(int);
extern void writeheader(int, char *);
extern char *getstrformat(int);
extern void dieu(char*);
extern char* type2string(int);

static void (*audtran)(char *, int), nullfn(char *, int);
static void (*spoutran)(MYFLT *, int);
static void chartran(MYFLT *, int),
            shortran(MYFLT *, int),
            longtran(MYFLT *, int), floatran(MYFLT *, int),
            bytetran(MYFLT *, int);
int writebuffer(MYFLT *, int);
/* Static global variables */
static unsigned outbufsiz;
static void     *outbuf;
static char     *choutbuf;               /* char  pntr to above  */
static short    *shoutbuf;               /* short pntr           */
static long     *lloutbuf;               /* long  pntr           */
static float    *floutbuf;               /* float pntr           */
static int      outrange = 0;            /* Count samples out of range */

static int outfd;
static int   block = 0;
static long  bytes = 0;

int dnoise(int argc, char **argv)
{

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

    int     fp;     /* noise reference file */

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
    int         inf;
    char        c, *s;
    int             channel = ALLCHNLS;
    MYFLT       beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    MYFLT       beg_ntime = FL(0.0), input_ndur = FL(0.0), srn = FL(0.0);
    char *envoutyp = NULL;
    char        outformch = 's';
    char *getenv(const char*);

#ifdef mills_macintoshxx
    argc = 0; argv = NULL;
#endif
    init_getstring(argc, argv);

    O.filnamspace = outfile = (char*)mmalloc((long)1024);
    nfile = (char*)mmalloc((long)1024);
    if ((envoutyp = getenv("SFOUTYP")) != NULL) {
      if (strcmp(envoutyp,"AIFF") == 0)
        O.filetyp = TYP_AIFF;
      else if (strcmp(envoutyp,"WAV") == 0)
        O.filetyp = TYP_WAV;
      else if (strcmp(envoutyp,"IRCAM") == 0)
        O.filetyp = TYP_IRCAM;
      else {
        sprintf(errmsg,
                Str(X_61,"%s not a recognized SFOUTYP env setting"),
                envoutyp);
        dieu(errmsg);
      }
    }
#ifdef mills_macintoshxx
    do_mac_dialogs();
    argc = gargc; argv = gargv;
#else
      {
        ++argv;
        while (--argc>0) {
          s = *argv++;
          if (*s++ == '-') {                        /* read all flags:  */
            while ((c = *s++) != '\0') {
              switch (c) {
              case 'j':
                FIND("")
                  while (*++s);
                break;
              case 'o':
                FIND(Str(X_1052,"no outfilename"))
                  O.outfilename = outfile;            /* soundout name */
                while ((*outfile++ = *s++)); s--;
                if (strcmp(O.outfilename,"stdin") == 0)
                  die(Str(X_156,"-o cannot be stdin"));
                if (strcmp(O.outfilename,"stdout") == 0) {
#if defined(THINK_C) || defined(mac_classic)
                  die(Str(X_1244,"stdout audio not supported"));
#else
                  if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                    die(Str(X_1290,"too many open files"));
                  dup2(2,1);                /* & send 1's to stderr */
#endif
                }
                break;
              case 'i':
                FIND(Str(X_1052,"no noisefilename"))
                  {
                    char *nn = nfile;
                    while ((*nn++ = *s++)); s--;
                  }
                break;
              case 'A':
                if (O.filetyp == TYP_WAV) {
                  if (envoutyp == NULL) goto outtyp;
                  if (O.msglevel & WARNMSG)
                    printf(Str(X_95,"WARNING: -A overriding local default WAV out\n"));
                }
                O.filetyp = TYP_AIFF;     /* AIFF output request*/
                break;
              case 'J':
                if (O.filetyp == TYP_AIFF ||
                    O.filetyp == TYP_WAV) {
                  if (envoutyp == NULL) goto outtyp;
                  if (O.msglevel & WARNMSG)
                    printf(Str(X_110,"WARNING: -J overriding local default AIFF/WAV out\n"));
                }
                O.filetyp = TYP_IRCAM;      /* IRCAM output request */
                break;
              case 'W':
                if (O.filetyp == TYP_AIFF) {
                  if (envoutyp == NULL) goto outtyp;
                  if (O.msglevel & WARNMSG)
                    printf(Str(X_131,"WARNING: -W overriding local default AIFF out\n"));
                }
                O.filetyp = TYP_WAV;      /* WAV output request */
                break;
              case 'h':
                O.sfheader = 0;           /* skip sfheader  */
                break;
              case 'c':
                if (O.outformat) goto outform;
                outformch = c;
                O.outformat = AE_CHAR;     /* 8-bit char soundfile */
                break;
              case '8':
                if (O.outformat) goto outform;
                outformch = c;
                O.outformat = AE_UNCH;     /* 8-bit unsigned char file */
                break;
/*               case 'a': */
/*                 if (O.outformat) goto outform; */
/*                 outformch = c; */
/*                 O.outformat = AE_ALAW;     /\* a-law soundfile *\/ */
/*                 break; */
/*               case 'u': */
/*                 if (O.outformat) goto outform; */
/*                 outformch = c; */
/*                 O.outformat = AE_ULAW;     /\* mu-law soundfile *\/ */
/*                 break; */
              case 's':
                if (O.outformat) goto outform;
                outformch = c;
                O.outformat = AE_SHORT;    /* short_int soundfile */
                break;
              case 'l':
                if (O.outformat) goto outform;
                outformch = c;
                O.outformat = AE_LONG;     /* long_int soundfile */
                break;
              case 'f':
                if (O.outformat) goto outform;
                outformch = c;
                O.outformat = AE_FLOAT;    /* float soundfile */
                break;
              case 'R':
                O.rewrt_hdr = 1;
                break;
              case 'H':
                if (isdigit(*s)) {
                  int n;
                  sscanf(s, "%d%n", &O.heartbeat, &n);
                  s += n;
                }
                else O.heartbeat = 1;
                break;
              case 't':
                FIND("no t arg");
                sscanf(s,"%f",&th);
                while (*++s);
                break;
              case 'S':
                FIND("no s arg");
                sscanf(s,"%d", &sh);
                while (*++s);
                break;
              case 'm':
                FIND("no m arg");
                sscanf(s,"%f",&g0);
                while (*++s);
                break;
              case 'n':
                FIND("no n arg");
                sscanf(s,"%d", &m);
                while (*++s);
                break;
              case 'b':
                FIND("no b arg");
                sscanf(s,"%f",&beg);
                while (*++s);
                break;
              case 'B': FIND("no B arg");
                sscanf(s,"%ld", &Beg);
                while (*++s);
                break;
              case 'e': FIND("no e arg");
                sscanf(s,"%f",&end);
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
                /*           case 'h': */
                /*             dnoise_usage(0);    /\* then exits normally *\/ */
              default:
                printf("Looking at %c\n", c);
                dnoise_usage(1);    /* this exits with error */
              }
            }
          }
          else if (infile==NULL) {
            infile = --s;
            printf("Infile set to %s\n", infile);
          }
          else {
            printf("End with %s\n", s);
            dnoise_usage(1);
          }
        }
      }
#endif
    if (nfile==NULL) {
      err_printf( "Must have an example noise file (-I name)\n");
      exit(1);
    }
    if ((inf = SAsndgetset(infile,&p,&beg_time,&input_dur,&sr,channel))<0) {
      err_printf(Str(X_735,"error while opening %s"), infile);
      exit(1);
    }
    if (O.outformat == 0) O.outformat = p->format;
    O.sfsampsize = sfsampsize(O.outformat);
    if (O.filetyp == TYP_AIFF) {
        if (!O.sfheader)
            die(Str(X_640,"cannot write AIFF soundfile with no header"));
        if (
            O.outformat == AE_ALAW || 
            O.outformat == AE_ULAW ||
            O.outformat == AE_FLOAT) {
            sprintf(errmsg,Str(X_180,"AIFF does not support %s encoding"),
                    getstrformat(O.outformat));
            die(errmsg);
        }
    }
    if (O.filetyp == TYP_WAV) {
        if (!O.sfheader)
            die(Str(X_338,"cannot write WAV soundfile with no header"));
        if (
            O.outformat == AE_ALAW || 
            O.outformat == AE_ULAW ||
            O.outformat == AE_FLOAT) {
            sprintf(errmsg,Str(X_181,"WAV does not support %s encoding"),
                    getstrformat(O.outformat));
            die(errmsg);
        }
    }
    if (O.rewrt_hdr && !O.sfheader)
        die(Str(X_628,"cannot rewrite header if no header requested"));
    if (O.outfilename == NULL)  O.outfilename = "test";
/*     ofd = fopen(outfile, "wb"); */
    outfd = openout(O.outfilename, 1);
    esr = (MYFLT)p->sr;
    nchnls = Chans = p->nchanls;
    if (O.sfheader)
        writeheader(outfd, O.outfilename);      /* write header as required   */
    if ((O.filetyp == TYP_AIFF && bytrevhost()) ||
        (O.filetyp == TYP_WAV && !bytrevhost())) {
        if (O.outformat == AE_SHORT)        /* if audio out needs byte rev  */
            audtran = bytrev2;           /*   redirect the audio puts    */
        else if (O.outformat == AE_LONG)
            audtran = bytrev4;
        else audtran = nullfn;
    }
    else audtran = nullfn;              /* else use standard audio puts */

    /* read header info */
    if (R < FL(0.0))
      R = (MYFLT)p->sr;
    if (Chans < 0)
      Chans = (int) p->nchanls;
    p->nchanls = Chans;

    if (Chans > 2) {
      perror("dnoise: input MUST be mono or stereo\n");
      exit(1);
    }

    /* read noise reference file */

    if ((fp = SAsndgetset(nfile,&pn,&beg_ntime,&input_ndur,&srn,channel))<0) {
      perror("dnoise: cannot open noise reference file\n");
      exit(1);
    }

    if (sr != srn) {
      err_printf( "Incompatible sample rates\n");
      exit(1);
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
      err_printf("dnoise: warning - N not a valid power of two; "
              "revised N = %d\n",i);
    N = i;
    N2 = N / 2;
    Np2 = N + 2;
    Ninv = FL(1.0) / N;

    if (W != -1) {
      if (M != 0)
        err_printf("dnoise: warning - don't specify both M and W\n");
      else
        if (W == 0)
          M = 4*N;
        else
          if (W == 1)
            M = 2*N;
          else
            if (W == 2)
              M = N;
            else
              if (W == 3)
                M = N2;
              else
                err_printf("dnoise: warning - invalid W ignored\n");
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
      err_printf("dnoise: warning - M is too small\n");

    if (D == 0)
      D = M / 8;

    I = D;

    lj = (long) M + 3 * (long) D;
    lj *= (long) Chans;
    if (lj > 32767) {
      perror("dnoise: M too large\n");
      exit(1);
    }
    lj = (long) L + 3 * (long) I;
    lj *= (long) Chans;
    if (lj > 32767) {
      perror("dnoise: L too large\n");
      exit(1);
    }

    ibuflen = Chans * (M + 3 * D);
    obuflen = Chans * (L + 3 * I);
    outbufsiz = obuflen * O.sfsampsize;/* calc outbuf size */
    outbuf = mmalloc((long)outbufsiz);                 /*  & alloc bufspace */
    printf(Str(X_1382,"writing %d-byte blks of %s to %s"),
           outbufsiz, getstrformat(O.outformat), O.outfilename);
    printf(" %s\n", type2string(O.filetyp));
    switch(O.outformat) {
    case AE_CHAR:
        spoutran = chartran;
        choutbuf = outbuf;
        break;
/*     case AE_ALAW: */
/*         spoutran = alawtran; */
/*         choutbuf = outbuf; */
/*         break; */
/*     case AE_ULAW: */
/*         spoutran = ulawtran; */
/*         choutbuf = outbuf; */
/*         break; */
    case AE_SHORT:
        spoutran = shortran;
        shoutbuf = (short *)outbuf;
        break;
    case AE_LONG:
        spoutran = longtran;
        lloutbuf = (long  *)outbuf;
        break;
    case AE_FLOAT:
        spoutran = floatran;
        floutbuf = (float *)outbuf;
        break;
    case AE_UNCH:
        spoutran = bytetran;
        choutbuf = outbuf;
        break;
    default:
        err_printf( "Type is %x\n", O.outformat);
        die(Str(X_1329,"unknown audio_out format"));
    }

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

    if ((aWin = (MYFLT *) calloc((size_t)(M+Meven),sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    aLen = M/2;
    aWin += aLen;

    hamming(aWin,aLen,Meven);
    for (i = 1; i <= aLen; i++) {
      aWin[-i] = aWin[i-1];
    }

    if (M > N) {
      if (Meven)
        *aWin *= (MYFLT)N * (MYFLT) sin(PI*0.5/(double)N) /( PI_F*FL(0.5));
      for (i = 1; i <= aLen; i++)
        aWin[i] *= (MYFLT) (N * sin(PI*((double) i+.5*(double) Meven)/(double) N)
          / (PI*(i+0.5*(double) Meven)));
      for (i = 1; i <= aLen; i++)
        aWin[-i] = aWin[i - Meven];
    }

    sum = FL(0.0);
    for (i = -aLen; i <= aLen; i++)
      sum += aWin[i];

    sum = FL(2.0) / sum;        /*factor of 2 comes in later in trig identity*/

    for (i = -aLen; i <= aLen; i++)
      aWin[i] *= sum;


    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    if ((sWin = (MYFLT *) calloc((size_t)(L+Leven),sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    sLen = L/2;
    sWin += sLen;

    if (M <= N) {
      hamming(sWin,sLen,Leven);
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
      hamming(sWin,sLen,Leven);
      for (i = 1; i <= sLen; i++)
        sWin[-i] = sWin[i - Leven];

      if (Leven)
        *sWin *= (MYFLT) (I * sin(PI*0.5/(double) I) / (PI*0.5));
      for (i = 1; i <= sLen; i++)
        sWin[i] *= (MYFLT)(I * sin(PI*((double) i+0.5*(double) Leven)/(double) I)
          / (PI*((double) i+0.5*(double) Leven)));
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

    if ((ibuf1 = (MYFLT *) calloc((size_t)ibuflen,sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((ibuf2 = (MYFLT *) calloc((size_t)ibuflen,sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if ((obuf1 = (MYFLT *) calloc((size_t)obuflen,sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }
    if ((obuf2 = (MYFLT *) calloc((size_t)obuflen,sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. */

    if ((fbuf = (MYFLT *) calloc((size_t)Np2,sizeof(MYFLT))) == NULL) {
      ERR("dnoise: insufficient memory\n");
    }

/* noise reduction: calculate noise reference by taking as many
        consecutive FFT's as possible in noise soundfile, and
        averaging them all together.  Multiply by th*th to
        establish threshold for noise-gating in each bin. */

    if ((nref = (MYFLT *) calloc((N2+1),sizeof(MYFLT))) == NULL) {
      ERR("denoise: insufficient memory\n");
    }

    if ((mbuf =(MYFLT *)calloc(m*Np2,sizeof(MYFLT))) == NULL) {
      ERR("denoise: insufficient memory\n");
    }
    if ((nbuf =(MYFLT *)calloc(m*Np2,sizeof(MYFLT))) == NULL) {
      ERR("denoise: insufficient memory\n");
    }
    if ((rsum =(MYFLT *)calloc(N2+1,sizeof(MYFLT))) == NULL) {
      ERR("denoise: insufficient memory\n");
    }
    if ((ssum =(MYFLT *)calloc(N2+1,sizeof(MYFLT))) == NULL) {
      ERR("denoise: insufficient memory\n");
    }

    /* skip over nMin samples */
    while (nMin > (long) ibuflen) {
      nread = getsndin(fp, ibuf1, ibuflen, pn);
      if (nread < ibuflen) {
        ERR("dnoise: begin time is greater than EOF of noise file!");
      }
      nMin -= (long) ibuflen;
    }
    i = (int) nMin;
    nread = getsndin(fp, ibuf1, i, pn);
    if (nread < i) {
      ERR("dnoise: begin time is greater than EOF of noise file!");
    }
    k = 0;
    lj = Beg;  /* single channel only */
    while (lj < End) {
      lj += (long) N;
      nread = getsndin(fp, fbuf, N, pn);
      if (nread < N)
        break;

      fbuf[N] = FL(0.0);
      fbuf[N + 1] = FL(0.0);

      fast(fbuf,N);

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
    nread = getsndin(inf, ibuf2, ibuflen, p);
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
          nread = getsndin(inf, ib2, ibuflen, p);
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
              oCnt += writebuffer(ob1, obuflen);
            }
            else {
              i = (int) (nMaxOut - oCnt);
              oCnt += writebuffer(ob1, i);
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

        fast(fbuf,N);

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
            if (avg < 0.)
              avg = FL(0.0);
            if (avg == 0.)
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

        fsst(fbuf,N);

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
          err_printf("%5.1f seconds of input complete\n",(time+D*invR));
        }
      }

    }

    nMaxOut = (long) (nMax * Chans);
    i = (int) (nMaxOut - oCnt);
    if (i > obuflen) {
      writebuffer(ob1, obuflen);
      i -= obuflen;
      ob1 = ob2;
    }
    if (i > 0)
      writebuffer(ob1, i);

/*     rewriteheader(outfd, 0); */
    printf("\n\n");
    close(outfd);
    if (Verbose) {
      err_printf("processing complete\n");
      err_printf("N = %d\n",N);
      err_printf("M = %d\n",M);
      err_printf("L = %d\n",L);
      err_printf("D = %d\n",D);
    }

    exit(0);

 outtyp:
    dieu(Str(X_1113,"output soundfile cannot be both AIFF and WAV"));
    exit(1);
 outform:
    sprintf(errmsg,Str(X_1198,"sound output format cannot be both -%c and -%c"),
            outformch, c);
    dnoise_usage(1);
    return 0;
}

void dnoise_usage(int exitcode)
{
    err_printf(
            "usage: dnoise [flags] input_file\n"
            "\nflags:\n"
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

    exit(exitcode);
}


int writebuffer(MYFLT * obuf, int length)
{
    spoutran(obuf, length);
    audtran(outbuf, O.sfsampsize*length);
    write(outfd, outbuf, O.sfsampsize*length);
    block++;
    bytes += O.sfsampsize*length;
    if (O.rewrt_hdr) {
      rewriteheader(outfd, 0);
      lseek(outfd, 0L, SEEK_END); /* Place at end again */
    }
    if (O.heartbeat) {
      if (O.heartbeat==1) {
#ifdef SYMANTEC
        nextcurs();
#elif __BEOS__
        putc('.', stderr); fflush(stderr);
#else
        putc("|/-\\"[block&3], stderr); putc(8,stderr);
#endif
      }
      else if (O.heartbeat==2) putc('.', stderr);
      else if (O.heartbeat==3) {
        int n;
        err_printf( "%d%n", block, &n);
        while (n--) putc(8, stderr);
      }
      else putc(7, stderr);
    }
    return length;
}

static void nullfn(char *outbuf, int nbytes)
{
    return;
}


static void bytetran(MYFLT *buffer, int size) /* after J. Mohr  1995 Oct 17 */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
    long   longsmp;
    int    n;

    for (n=0; n<size; n++) {
      if ((longsmp = (long)buffer[n]) >= 0) {/* +ive samp:   */
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;              /*   clip and report */
          outrange++;
        }
        else {                          /* ditto -ive samp */
          if (longsmp < -32768) {
            longsmp = -32768;
            outrange++;
          }
        }
      }
      choutbuf[n] = (unsigned char)(longsmp >> 8)^0x80;
    }
}

static void shortran(MYFLT *buffer, int size)   /* fix spout vals and put in outbuf */
{                                               /*      write buffer when full      */
    int n;
    long longsmp;

    for (n=0; n<size; n++) {
      if ((longsmp = (long)buffer[n]) >= 0) {               /* +ive samp:   */
        if (longsmp > 32767) {              /* out of range?     */
          longsmp = 32767;        /*   clip and report */
          outrange++;
        }
      }
      else {
        if (longsmp < -32768) {             /* ditto -ive samp */
          longsmp = -32768;
          outrange++;
        }
      }
      shoutbuf[n] = (short)longsmp;
    }
}

static void chartran(MYFLT *buffer, int size)   /* same as above, but 8-bit char output */
                                                /*   sends HI-ORDER 8 bits of shortsamp */
{
    int n;
    long longsmp;

    for (n=0; n<size; n++) {
      if ((longsmp = (long)buffer[n]) >= 0) {   /* +ive samp:   */
        if (longsmp > 32767) {                  /* out of range?     */
          longsmp = 32767;                      /*   clip and report */
          outrange++;
        }
      }
      else {
        if (longsmp < -32768) {                 /* ditto -ive samp */
          longsmp = -32768;
          outrange++;
        }
      }
      choutbuf[n] = (char)(longsmp >> 8);
    }
}

#ifdef never
static void
alawtran(MYFLT *buffer, int size)
{ die(Str(X_590,"alaw not yet implemented")); }
#endif

#define MUCLIP  32635
#define BIAS    0x84
#define MUZERO  0x02
#define ZEROTRAP

#ifdef ULAW
static void
ulawtran(MYFLT *buffer, int size) /* ulaw-encode spout vals & put in outbuf */
                                 /*     write buffer when full      */
{
    int  n;
    long longsmp;
    int  sign;
    extern char    exp_lut[];               /* mulaw encoding table */
    int sample, exponent, mantissa, ulawbyte;

    for (n=0; n<size; n++) {
      if ((longsmp = (long)buffer[n]) < 0) {  /* if sample negative   */
        sign = 0x80;
        longsmp = - longsmp;                  /*  make abs, save sign */
      }
      else sign = 0;
      if (longsmp > MUCLIP) {                 /* out of range?     */
        longsmp = MUCLIP;                     /*   clip and report */
        outrange++;
      }
      sample = longsmp + BIAS;
      exponent = exp_lut[( sample >> 8 ) & 0x7F];
      mantissa = ( sample >> (exponent+3) ) & 0x0F;
      ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
      if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
      choutbuf[n] = ulawbyte;
    }
}
#endif

static void longtran(MYFLT *buffer, int size) /* send long_int spout vals to outbuf */
                                              /*    write buffer when full      */
{
    int n;

    for (n=0; n<size; n++) {
      lloutbuf[n] = (long) buffer[n];
      if (buffer[n] > (MYFLT)(0x7fffffff)) {
        lloutbuf[n] = 0x7fffffff;
        outrange++;
      }
      else if (buffer[n] < - (MYFLT)(0x7fffffff)) {
        lloutbuf[n] = - 0x7fffffff;
        outrange++;
      }
      else lloutbuf[n] = (long) buffer[n];
    }
}

static void
floatran(MYFLT *buffer, int size)
{
    int n;
    for (n=0; n<size; n++) floutbuf[n] = (float)buffer[n];
}

#ifdef mills_macintoshxx
void die(char *s)
{
    printf("%s\n", s);
    while (!Button());
    ExitToShell();
}
#endif
