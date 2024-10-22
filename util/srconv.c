/*
    srconv.c

    Copyright (C) 1989, 2000 Mark Dolson, John ffitch

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
 *    PROGRAM:   srconv - sample rate converter
 *
 *    AUTHOR:    Mark Dolson
 *
 *    DATE:      August 26, 1989
 *
 *    COMMENTS:  srconv takes floats on stdin at sample rate Rin and
 *               outputs them on stdout at sample rate Rout.  optionally,
 *               the ratio (Rin / Rout)  may be linearly time-varying
 *               according to a set of (time, ratio) pairs in an auxiliary file.
 *
 *               flags:
 *
 *                    r = output sample rate (must be specified)
 *                    R = input sample rate (must be specified)
 *                    P = input sample rate / output sample rate
 *                    Q = quality factor (1 to 8: default = 2)
 *                    if a time-varying control file is given, it must be last
 *
 *    MODIFIED:  John ffitch December 2000; changes to Csound context
 */

#include "std_util.h"
#include "soundio.h"
#include <math.h>
#include <ctype.h>

#define IBUF    (4096)
#define IBUF2   (IBUF/2)
#define OBUF    (4096)

#define FIND(MSG)                                                   \
{                                                                   \
    if (*s == '\0')                                                 \
      if (!(--argc) || (((s = *argv++) != NULL) && *s == '-')) {    \
        dieu(csound, MSG); return -1;                               \
      }                                                             \
}

//static  void    kaiser(int, float *, int, int, double);

#if 0
static  void    usage(CSOUND *);

static int32_t writebuffer(CSOUND *csound, MYFLT *out_buf, int32_t *block,
                       SNDFILE *outfd, int32_t length,const OPARMS **oparms)
{
    csound->SndfileWriteSamples(csound, outfd, out_buf, length);
    (*block)++;
    if (oparms->rewrt_hdr)
      csound->RewriteHeader(csound, SNDFILE *)outfd);
    switch (oparms->heartbeat) {
      case 1:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\010",
                                                     "|/-\\"[*block & 3]);
        break;
      case 2:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, ".");
        break;
      case 3:
        {
          int32_t n;
          csound->MessageS(csound, CSOUNDMSG_REALTIME, "%d%n", *block, &n);
          while (n--) csound->MessageS(csound, CSOUNDMSG_REALTIME, "\010");
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "\007");
    }
    return length;
}

static char set_output_format(CSOUND *csound, char c, char outformch,
                             const OPARMS **oparms)
{
    if (oparms->outformat) {
      csound->Warning(csound, Str("Sound format -%c has been overruled by -%c"),
                              outformch, c);
    }
    switch (c) {
    case 'a':
      oparms->outformat = AE_ALAW;  /* a-law soundfile */
      break;
    case 'c':
      oparms->outformat = AE_CHAR;  /* signed 8-bit soundfile */
      break;
    case '8':
      oparms->outformat = AE_UNCH;  /* unsigned 8-bit soundfile */
     break;
    case 'f':
      oparms->outformat = AE_FLOAT; /* float soundfile */
      break;
    case 's':
      oparms->outformat = AE_SHORT; /* short_int soundfile*/
      break;
    case 'l':
      oparms->outformat = AE_LONG;  /* long_int soundfile */
      break;
    case 'u':
      oparms->outformat = AE_ULAW;  /* mu-law soundfile */
      break;
    case '3':
      oparms->outformat = AE_24INT; /* 24bit packed soundfile*/
      break;
    case 'e':
      oparms->outformat = AE_FLOAT; /* float sndfile (for rescaling) */
      break;
    default:
      oparms->outformat = 0;
      csound->ErrorMsg(csound, Str("srconv: unknown outout format '%c'\n"), c);
      return outformch; /* do nothing */
    };
    return c;
}

static void dieu(CSOUND *csound, char *s)
{
    csound->ErrorMsg(csound, "srconv: %s", s);
    usage(csound);
}

static int32_t srconv(CSOUND *csound, int32_t argc, char **argv)
{
    MYFLT
      *input,     /* pointer to start of input buffer */
      *output,    /* pointer to start of output buffer */
      *nextIn,    /* pointer to next empty word in input */
      *nextOut,   /* pointer to next empty word in output */
      *fxval = 0, /* pointer to startb of time-array for time-vary function */
      *fyval = 0, /* pointer to start of P-scale-array for time-vary func */
      *i0,        /* pointer */
      *i1;        /* pointer */

    float
      *window,    /* pointer to center of analysis window */
      *wj,        /* pointer to window */
      *wj1;       /* pointer to window */

    int
      M = 2401,   /* length of window impulse response */
      N = 120,    /* length of sinc period */
      L = 120,    /* internal sample rate is L*Rin */
      m,          /* current input sample in buffer */
      o,          /* current input at L*Rin mod L */
      del,        /* increment */
      WinLen,     /* half-length of window at L*Rin */
      wLen,       /* half-length of window at Rin */
      jMin,       /* initial offset in window */
      mMax;       /* maximum valid m */

    long
      n,                        /* current input sample */
      nMax = 2000000000;        /* last input sample (unless EOF) */

    MYFLT
      beta = FL(6.8),           /* parameter for Kaiser window */
      sum,                      /* scale factor for renormalizing windows */
      fdel,                     /* float del */
      idel,                     /* float del */
      fo,                       /* float o */
      of,                       /* fractional o */
      fL = (MYFLT) L,           /* float L */
      iw,                       /* interpolated window */
      tvx0 = 0,                 /* current x value of time-var function */
      tvx1 = 0,                 /* next x value of time-var function */
      tvdx,                     /* tvx1 - tvx0 */
      tvy0 = 0,                 /* current y value of time-var function */
      tvy1 = 0,                 /* next y value of time-var function */
      tvdy,                     /* tvy1 - tvy0 */
      tvslope = 0,              /* tvdy / tvdx */
      time,                     /* n / Rin */
      invRin,                   /* 1. / Rin */
      P = FL(0.0),              /* Rin / Rout */
      Rin = FL(0.0),            /* input sampling rate */
      Rout = FL(0.0);           /* output sample rate */

    int
      i,k,                      /* index variables */
      nread,                    /* number of bytes read */
      tvflg = 0,                /* flag for time-varying time-scaling */
      tvnxt = 0,                /* counter for stepping thru time-var func */
      tvlen,                    /* length of time-varying function */
      Chans = 1,                /* number of channels */
      chan,                     /* current channel */
      Q = 2;                    /* quality factor */

    FILE        *tvfp = NULL;   /* time-vary function file */
    SOUNDIN     *p;
    int32_t         channel = ALLCHNLS;
    MYFLT       beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    char        *infile = NULL, *bfile = NULL;
    SNDFILE     *inf = NULL;
    char        c, *s;
    const char  *envoutyp;
    char        outformch = 's';
    unsigned    outbufsiz = 0U;
    SNDFILE     *outfd = NULL;
    OPARMS      *O = csound->Calloc(csound, sizeof(OPARMS));
    int32_t     block = 0;
    char        err_msg[256];

    O->outformat = AE_SHORT;
    /* csound->e0dbfs = csound->dbfs_to_float = FL(1.0);*/
    memcpy(O, csound->GetOParms(csound), sizeof(OPARMS));

    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        O->filetyp = TYP_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        O->filetyp = TYP_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        O->filetyp = TYP_IRCAM;
      else {
        snprintf(err_msg, 256, Str("%s not a recognized SFOUTYP env setting"),
                envoutyp);
        dieu(csound, err_msg);
        return -1;
      }
    }

    /* call getopt to interpret commandline */

    ++argv;
    while (--argc > 0) {
      s = *argv++;
      if (*s++ == '-') {                /* read all flags:  */
        while ((c = *s++) != '\0') {
          switch (c) {
          case 'o':
            FIND(Str("no outfilename"))
            O->outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (strcmp(O->outfilename, "stdin") == 0) {
              csound->ErrorMsg(csound, "%s", Str("-o cannot be stdin"));
              return -1;
            }
#if defined(WIN32)
            if (strcmp(O->outfilename, "stdout") == 0) {
              csound->ErrorMsg(csound, "%s", Str("stdout audio not supported"));
              return -1;
            }
#endif
            break;
          case 'A':
            O->filetyp = TYP_AIFF;      /* AIFF output request*/
            break;
          case 'J':
            O->filetyp = TYP_IRCAM;     /* IRCAM output request */
            break;
          case 'W':
            O->filetyp = TYP_WAV;       /* WAV output request */
            break;
          case 'h':
            O->filetyp = TYP_RAW;       /* skip sfheader  */
            break;
          case 'c':
          case '8':
          case 'a':
          case 'u':
          case 's':
          case 'l':
          case '3':
          case 'f':
            outformch = set_output_format(csound, c, outformch, ;
            break;
          case 'R':
            O->rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              int32_t n;
              csound->Sscanf(s, "%d%n", &O->heartbeat, &n);
              s += n;
            }
            else O->heartbeat = 1;
            break;
          case 'N':
            O->ringbell = 1;        /* notify on completion */
            break;
          case 'Q':
            FIND(Str("No Q argument"))
            csound->Sscanf(s,"%d", &Q);
            while (*++s);
            break;
          case 'P':
            FIND(Str("No P argument"))
#if defined(USE_DOUBLE)
            csound->Sscanf(s,"%lf", &P);
#else
            csound->Sscanf(s,"%f", &P);
#endif
            while (*++s);
            break;
          case 'r':
            FIND(Str("No r argument"))
#if defined(USE_DOUBLE)
            csound->Sscanf(s,"%lf", &Rout);
#else
            csound->Sscanf(s,"%f", &Rout);
#endif
            while (*++s);
            break;
          case 'i':
            FIND(Str("No break file"))
            tvflg = 1;
            bfile = s;
            while ((*s++)) {}; s--;
            break;
          default:
            csound->Message(csound, Str("Looking at %c\n"), c);
            usage(csound);    /* this exits with error */
            return -1;
          }
        }
      }
      else if (infile == NULL) {
        infile = --s;
        csound->Message(csound, Str("Infile set to %s\n"), infile);
      }
      else {
        csound->Message(csound, Str("End with %s\n"), s);
        usage(csound);
        return -1;
      }
    }
    if (infile == NULL) {
      csound->Message(csound, "%s", Str("No input given\n"));
      usage(csound);
      return -1;
    }
    if ((inf = (csound->GetUtility(csound))->SndinGetSetSA(csound, infile, &p, &beg_time,
                                   &input_dur, &sr, channel)) == NULL) {
      csound->ErrorMsg(csound, Str("error while opening %s"), infile);
      return -1;
    }
    if (Rin == FL(0.0))
      Rin = (MYFLT)p->sr;
    if (Chans == 0)
      Chans = (int) p->nchanls;
    if (Chans == 0)
      Chans = 1;

    if ((P != FL(0.0)) && (Rout != FL(0.0))) {
      strNcpy(err_msg, Str("srconv: cannot specify both -r and -P"), 256);
      goto err_rtn_msg;
    }
    if (P != FL(0.0))
      Rout = Rin / P;
    else if (Rout == FL(0.0))
      Rout = Rin;

    if (tvflg) {
      P = FL(0.0);        /* will be reset to max in time-vary function */
      if ((tvfp = fopen(bfile, "r")) == NULL) {
        strNcpy(err_msg,
                Str("srconv: cannot open time-vary function file"), 256);
        goto err_rtn_msg;
      }
      /* register file to be closed by csoundReset() */
      (void) csound->CreateFileHandle(csound, &tvfp, CSFILE_STD, bfile);
      if (UNLIKELY(fscanf(tvfp, "%d", &tvlen) != 1))
        csound->Message(csound, "%s", Str("Read failure\n"));
      if (UNLKELY(tvlen <= 0)) {
            strNcpy(err_msg, Str("srconv: tvlen <= 0 "), 256);
            goto err_rtn_msg;
       }
      fxval = (MYFLT*) csound->Malloc(csound, tvlen * sizeof(MYFLT));
      fyval = (MYFLT*) csound->Malloc(csound, tvlen * sizeof(MYFLT));
      i0 = fxval;
      i1 = fyval;
      for (i = 0; i < tvlen; i++, i0++, i1++) {
#ifdef USE_DOUBLE
        if ((fscanf(tvfp, "%lf %lf", i0, i1)) != 2)
#else
        if ((fscanf(tvfp, "%f %f", i0, i1)) != 2)
#endif
          {
            strNcpy(err_msg, Str("srconv: too few x-y pairs "
                                 "in time-vary function file"), 256);
            goto err_rtn_msg;
          }
        if (*i1 > P)
          P = *i1;
      }
      Rout = Rin / P;    /* this is min Rout */
      tvx0 = fxval[0];
      tvx1 = fxval[1];
      tvy0 = fyval[0];
      tvy1 = fyval[1];
      tvdx = tvx1 - tvx0;
      if (tvx0 != FL(0.0)) {
        strNcpy(err_msg, Str("srconv: first x value "
                             "in time-vary function must be 0"), 256);
        goto err_rtn_msg;
      }
      if (tvy0 <= FL(0.0)) {
        strNcpy(err_msg, Str("srconv: invalid initial y value "
                             "in time-vary function"),256);
        goto err_rtn_msg;
      }
      if (tvdx <= FL(0.0)) {
        strNcpy(err_msg,
                       Str("srconv: invalid x values in time-vary function"),
                       256);
        goto err_rtn_msg;
      }
      tvdy = tvy1 - tvy0;
      tvslope = tvdy / tvdx;
      tvnxt = 1;
    }
    /* This is not right *********  */
    if (P != FL(0.0)) {
      (csound->GetUtility(csound))->SetUtilSr(csound,Rin);
    }
    if (P == FL(0.0)) {
      (csound->GetUtility(csound))->SetUtilSr(csound,Rout);
    }

    if (O->outformat == 0)
      O->outformat = AE_SHORT;//p->format;
    O->sfsampsize = csound->SndfileSampleSize(FORMAT2SF(O->outformat));
    if (O->filetyp == TYP_RAW) {
      
      O->rewrt_hdr = 0;
    }
    else
      
#ifdef NeXT
    if (O->outfilename == NULL && !O->filetyp)
      O->outfilename = "test.snd";
    else if (O->outfilename == NULL)
      O->outfilename = "test";
#else
    if (O->outfilename == NULL) {
      if (O->filetyp == TYP_WAV)
        O->outfilename = "test.wav";
      else if (O->filetyp == TYP_AIFF)
        O->outfilename = "test.aif";
      else
        O->outfilename = "test";
    }
#endif
    {
      SFLIB_INFO sfinfo;
      char    *name;
      memset(&sfinfo, 0, sizeof(SFLIB_INFO));
      sfinfo.samplerate = (int) ((double) Rout + 0.5);
      sfinfo.channels = (int) p->nchanls;
      //printf("filetyp=%x outformat=%x\n", O->filetyp, O->outformat);
      sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
      if (strcmp(O->outfilename, "stdout") != 0) {
        name = csound->FindOutputFile(csound, O->outfilename, "SFDIR");
        if (name == NULL) {
          snprintf(err_msg, 256, Str("cannot open %s."), O->outfilename);
          goto err_rtn_msg;
        }
        outfd = csound->SndfileOpen(csound,name, SFM_WRITE, &sfinfo);
        if (outfd != NULL)
          csound->NotifyFileOpened(csound, name,
                                   csound->Type2CsfileType(O->filetyp,
                                                           O->outformat),
                                   1, 0);
        else {
          snprintf(err_msg, 256, Str("libsndfile error: %s\n"), csound->SndfileStrError(csound,NULL));
          goto err_rtn_msg;
        }
        csound->Free(csound, name);
      }
      else
        outfd = csound->SndfileOpenFd(csound,1, SFM_WRITE, &sfinfo, 1);
      if (outfd == NULL) {
        snprintf(err_msg, 256, Str("cannot open %s."), O->outfilename);
        goto err_rtn_msg;
      }
      /* register file to be closed by csoundReset() */
      (void) csound->CreateFileHandle(csound, &outfd, CSFILE_SND_W,
                                      O->outfilename);
      csound->SndfileCommand(csound,outfd, SFC_SET_CLIPPING, NULL, SFLIB_TRUE);
    }
    (csound->GetUtility(csound))->SetUtilSr(csound, (MYFLT)p->sr);
    (csound->GetUtility(csound))->SetUtilNchnls(csound, Chans = p->nchanls);

    outbufsiz = OBUF * O->sfsampsize;                   /* calc outbuf size */
    csound->Message(csound, Str("writing %d-byte blks of %s to %s"),
                    outbufsiz, csound->GetStrFormat(O->outformat),
                    O->outfilename);
    csound->Message(csound, " (%s)\n",csound->Type2String(O->filetyp));

 /* this program performs arbitrary sample-rate conversion
    with high fidelity.  the method is to step through the
    input at the desired sampling increment, and to compute
    the output points as appropriately weighted averages of
    the surrounding input points.  there are two cases to
    consider: 1) sample rates are in a small-integer ratio -
    weights are obtained from table, 2) sample rates are in
    a large-integer ratio - weights are linearly
    interpolated from table.  */

 /* calculate increment: if decimating, then window is impulse response of low-
    pass filter with cutoff frequency at half of Rout; if interpolating,
    then window is ipulse response of lowpass filter with cutoff frequency
    at half of Rin. */

    fdel = ((MYFLT) (L * Rin) / Rout);
    del = (int) ((double) fdel + 0.5);
    idel = (MYFLT) del;
    if (del > L)
      N = del;
    if ((Q >= 1) && (Q <= 8))
      M = Q * N * 10 + 1;
    if (tvflg)
      fdel = tvy0 * L;

    invRin  =  FL(1.0) / Rin;

    /* make window: the window is the product of a kaiser and a sin(x)/x */
    window = (float*) csound->Calloc(csound, (size_t) (M + 2) * sizeof(float));
    WinLen = (M-1)/2;
    window += WinLen;
    wLen = (M/2 - L) / L;

    kaiser(M, window, WinLen, 1, (double) beta);

    for (i = 1; i <= WinLen; i++) {
      double  tmp = (double) N;
      tmp = tmp * sin(PI * (double) i / tmp) / (PI * (double) i);
      window[i] = (float) ((double) window[i] * tmp);
    }

    if (Rout < Rin) {
#if 0
      sum = (MYFLT) window[0];
      for (i = L-1; i <= WinLen; i += L)
        sum += (MYFLT) window[i];
      sum = FL(2.0) / sum;
#else
      sum = Rout / (Rin * (MYFLT) window[0]);
#endif
    }
    else
      sum = FL(1.0) / (MYFLT) window[0];

    window[0] = (float) ((double) window[0] * (double) sum);
    for (i = 1; i <= WinLen; i++) {
      window[i] = (float) ((double) window[i] * (double) sum);
      *(window - i) = window[i];
    }

    window[WinLen + 1] = 0.0f;

 /* set up input buffer:  nextIn always points to the next empty
    word in the input buffer.  If the buffer is full, then
    nextIn jumps back to the beginning, and the old values
    are written over. */

    input = (MYFLT*) csound->Calloc(csound, (size_t) IBUF * sizeof(MYFLT));

 /* set up output buffer:  nextOut always points to the next empty
    word in the output buffer.  If the buffer is full, then
    it is flushed, and nextOut jumps back to the beginning. */

    output = (MYFLT*) csound->Calloc(csound, (size_t) OBUF * sizeof(MYFLT));
    nextOut = output;

 /* initialization: */

    nread = (csound->GetUtility(csound))->Sndin(csound, inf, input, IBUF2, p);
    for(i=0; i < nread; i++)
       input[i] *= 1.0/csound->Get0dBFS(csound);
    nMax = (long)(input_dur * p->sr);
    nextIn = input + nread;
    for (i = nread; i < IBUF2; i++)
      *(nextIn++) = FL(0.0);
    jMin = -(wLen + 1) * L;
    mMax = IBUF2;
    o = n = m = 0;
    fo = FL(0.0);

 /* main loop:   If nMax is not specified it is assumed to be very large
    and then readjusted when read detects the end of input. */

    while (n < nMax) {

      time = n * invRin;

      /* case 1:  (Rin / Rout) * 120 = integer  */

      if ((tvflg == 0) && (idel == fdel)) {
        /* apply window (window is sampled at L * Rin) */

        for (chan = 0; chan < Chans; chan++) {
          *nextOut = FL(0.0);
          k = Chans * (m - wLen) + chan - Chans;
          if (k < 0)
            k += IBUF;
          wj = window + jMin - o;
          for (i = -wLen; i <= wLen+1; i++){
            wj += L;
            k += Chans;
            if (k >= IBUF)
              k -= IBUF;
            *nextOut += (MYFLT) *wj * *(input + k);
          }
          nextOut++;
          if (nextOut >= (output + OBUF)) {
            nextOut = output;
            writebuffer(csound, output, &block, outfd, OBUF, ;
          }
        }

        /* move window (window advances by del samples at L*Rin sample rate) */

        o += del;
        while (o >= L) {
          o -= L;
          n++;
          m++;
          if ((Chans * (m + wLen + 1)) >= mMax) {
            if (!csound->CheckEvents(csound))
              csound->LongJmp(csound, 1);
            mMax += IBUF2;
            if (nextIn >= (input + IBUF))
              nextIn = input;
            nread = (csound->GetUtility(csound))->Sndin(csound, inf, nextIn, IBUF2, p);
            for(i=0; i < nread; i++)
               input[i] *= 1.0/csound->Get0dBFS(csound);
            nextIn += nread;
            if (nread < IBUF2)
              nMax = n + wLen + (nread / Chans) + 1;
            for (i = nread; i < IBUF2; i++)
              *(nextIn++) = FL(0.0);
          }
          if ((Chans * m) >= IBUF) {
            m = 0;
            mMax = IBUF2;
          }
        }
      }

      /* case 2: (Rin / Rout) * 120 = non-integer constant */

      else {

        /* apply window (window values are linearly interpolated) */

        for (chan = 0; chan < Chans; chan++) {
          *nextOut = FL(0.0);
          o = (int)fo;
          of = fo - o;
          wj = window + jMin - o;
          wj1 = wj + 1;
          k = Chans * (m - wLen) + chan - Chans;
          if (k < 0)
            k += IBUF;
          for (i = -wLen; i <= wLen+1; i++) {
            wj += L;
            wj1 += L;
            k += Chans;
            if (k >= IBUF)
              k -= IBUF;
            iw = (MYFLT) *wj + of * ((MYFLT) *wj1 - (MYFLT) *wj);
            *nextOut += iw * *(input + k);
          }
          nextOut++;
          if (nextOut >= (output + OBUF)) {
            nextOut = output;
            writebuffer(csound, output, &block, outfd, OBUF, ;
          }
        }

        /* move window */

        fo += fdel;
        while (fo >= fL) {
          fo -= fL;
          n++;
          m++;
          if ((Chans * (m + wLen + 1)) >= mMax) {
            if (!csound->CheckEvents(csound))
              csound->LongJmp(csound, 1);
            mMax += IBUF2;
            if (nextIn >= (input + IBUF))
              nextIn = input;
            nread = (csound->GetUtility(csound))->Sndin(csound, inf, nextIn, IBUF2, p);
            for(i=0; i < nread; i++)
               input[i] *= 1.0/csound->Get0dBFS(csound);
            nextIn += nread;
            if (nread < IBUF2)
              nMax = n + wLen + (nread / Chans) + 1;
            for (i = nread; i < IBUF2; i++)
              *(nextIn++) = FL(0.0);
          }
          if ((Chans * m) >= IBUF) {
            m = 0;
            mMax = IBUF2;
          }
        }

        if (tvflg && (time > FL(0.0))) {
          while (tvflg && (time >= tvx1)) {
            if (++tvnxt >= tvlen)
              tvflg = 0;
            else {
              tvx0 = tvx1;
              tvx1 = fxval[tvnxt];
              tvy0 = tvy1;
              tvy1 = fyval[tvnxt];
              tvdx = tvx1 - tvx0;
              if (tvdx <= FL(0.0)) {
                strNcpy(err_msg, Str("srconv: invalid x values "
                                     "in time-vary function"), 256);
                goto err_rtn_msg;
              }
              tvdy = tvy1 - tvy0;
              tvslope = tvdy / tvdx;
            }
          }
          P = tvy0 + tvslope * (time - tvx0);
          fdel = (MYFLT) L * P;
        }
      }

    }
    nread = nextOut - output;
    writebuffer(csound, output, &block, outfd, nread, ;
    csound->Message(csound, "\n\n");
    if (O->ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "\a");
    return 0;

 err_rtn_msg:
    csound->ErrorMsg(csound, err_msg);
    return -1;
}
#else

#ifndef WIN32
#include <unistd.h>
#endif

static int32_t srconv(CSOUND *csound, int32_t argc, char **argv)
{
  (void) argc;
    csound->Message(csound, "%s",
                    Str("Do not use srconv but the src_conv program\n"));
#if !defined(MSVC) && !defined(__wasm__)
    return execv("src_conv", argv);
#else
    return 0;
#endif
}
#endif

#if 0
static const char *usage_txt[] = {
  Str_noop("usage: srconv [flags] infile\n\nflags:"),
  Str_noop("-P num\tpitch transposition ratio (srate/r) [do not specify "
           "both P and r]"),
  Str_noop("-Q num\tquality factor (1 to 8: default = 2)"),
  Str_noop("-i filnam\tbreak file"),
  Str_noop("-r num\toutput sample rate (must be specified)"),
  Str_noop("-o fnam\tsound output filename\n"),
  Str_noop("-A\tcreate an AIFF format output soundfile"),
  Str_noop("-J\tcreate an IRCAM format output soundfile"),
  Str_noop("-W\tcreate a WAV format output soundfile"),
  Str_noop("-h\tno header on output soundfile"),
  Str_noop("-c\t8-bit signed_char sound samples"),
  Str_noop("-a\talaw sound samples"),
  Str_noop("-8\t8-bit unsigned_char sound samples"),
  Str_noop("-u\tulaw sound samples"),
  Str_noop("-s\tshort_int sound samples"),
  Str_noop("-l\tlong_int sound samples"),
  Str_noop("-f\tfloat sound samples"),
  Str_noop("-r N\torchestra srate override"),
  Str_noop("-K\tDo not generate PEAK chunks"),
  Str_noop("-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)"),
  Str_noop("-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write"),
  Str_noop("-N\tnotify (ring the bell) when score or miditrack is done"),
  Str_noop("-- fnam\tlog output to file"),
    NULL
};

static void usage(CSOUND *csound)
{
    int32_t i = -1;

    while (usage_txt[++i] != NULL)
      csound->Message(csound, "%s\n", Str(usage_txt[i]));
}

static double ino(double x)
{
    double  y, t, e, de, sde, xi;
    int32_t     i;

    y = x * 0.5;
    t = 1.0e-08;
    e = 1.0;
    de = 1.0;
    for (i = 1; i <= 25; i++) {
      xi = (double) i;
      de = de * y / xi;
      sde = de * de;
      e += sde;
      if (e * t > sde)
        break;
    }
    return e;
}

static void kaiser(int32_t nf, float *w, int32_t n, int32_t ieo, double beta)
{

/*
   nf = filter length in samples
    w = window array of size n
    n = filter half length=(nf+1)/2
  ieo = even odd indicator--ieo=0 if nf even
 beta = parameter of kaiser window
*/

    double  bes, xind, xi;
    int32_t     i;

    bes = ino(beta);
    xind = (double) ((nf - 1) * (nf - 1));

    for (i = 0; i < n; i++) {
      xi = (double) i;
      if (ieo == 0)
        xi += 0.5;
      xi = 4.0 * xi * xi;
      xi = sqrt(1.0 - (double) (xi / xind));
      w[i] = (float) (ino(beta * xi) / bes);
    }
}
#endif

/* module interface */

int32_t srconv_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "srconv", srconv);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "srconv",
                                             Str("Sample rate conversion"));
    }
    return retval;
}
