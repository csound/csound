/*
    srconv.c

    Copyright (C) 1989, 2000 Mark Dolson, John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 *                    Q = quality factor (1, 2, 3, or 4: default = 2)
 *                    if a time-varying control file is given, it must be last
 *
 *    MODIFIED:  John ffitch December 2000; changes to Csound context
 */

#include "csdl.h"
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
        dieu(csound, MSG); goto err_return;                         \
      }                                                             \
}

static  void    kaiser(int, MYFLT *, int, int, MYFLT);
static  void    usage(ENVIRON *);
static  char    *type2string(int);
static  short   sfsampsize(int);
static  char    *getstrformat(ENVIRON *, int);

static int writebuffer(ENVIRON *csound, MYFLT *out_buf, int *block,
                                        SNDFILE *outfd, int length)
{
    sf_write_MYFLT(outfd, out_buf, length);
    (*block)++;
    if (csound->oparms->rewrt_hdr)
       csound->rewriteheader(outfd, 0);
    switch (csound->oparms->heartbeat) {
      case 1:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\010",
                                                     "|/-\\"[*block & 3]);
        break;
      case 2:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, ".");
        break;
      case 3:
        {
          int n;
          csound->MessageS(csound, CSOUNDMSG_REALTIME, "%d%n", *block, &n);
          while (n--) csound->MessageS(csound, CSOUNDMSG_REALTIME, "\010");
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "\007");
    }
    return length;
}

static char set_output_format(ENVIRON *csound, char c, char outformch)
{
    if (csound->oparms->outformat) {
      csound->Warning(csound, Str("Sound format -%c has been overruled by -%c"),
                              outformch, c);
    }
    switch (c) {
    case 'a':
      csound->oparms->outformat = AE_ALAW;  /* a-law soundfile */
      break;
    case 'c':
      csound->oparms->outformat = AE_CHAR;  /* signed 8-bit soundfile */
      break;
    case '8':
      csound->oparms->outformat = AE_UNCH;  /* unsigned 8-bit soundfile */
      break;
    case 'f':
      csound->oparms->outformat = AE_FLOAT; /* float soundfile */
      break;
    case 's':
      csound->oparms->outformat = AE_SHORT; /* short_int soundfile*/
      break;
    case 'l':
      csound->oparms->outformat = AE_LONG;  /* long_int soundfile */
      break;
    case 'u':
      csound->oparms->outformat = AE_ULAW;  /* mu-law soundfile */
      break;
    case '3':
      csound->oparms->outformat = AE_24INT; /* 24bit packed soundfile*/
      break;
    case 'e':
      csound->oparms->outformat = AE_FLOAT; /* float sndfile (for rescaling) */
      break;
    default:
      return outformch; /* do nothing */
    };
    return c;
}

static void dieu(ENVIRON *csound, char *s)
{
    csound->MessageS(csound, CSOUNDMSG_ERROR, "srconv: %s\n", s);
    usage(csound);
}

static int srconv(void *csound_, int argc, char **argv)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    MYFLT
      *input,     /* pointer to start of input buffer */
      *output,    /* pointer to start of output buffer */
      *nextIn,    /* pointer to next empty word in input */
      *nextOut,   /* pointer to next empty word in output */
      *window,    /* pointer to center of analysis window */
      *wj,        /* pointer to window */
      *wj1,       /* pointer to window */
      *fxval = 0, /* pointer to start of time-array for time-vary function */
      *fyval = 0, /* pointer to start of P-scale-array for time-vary func */
      *i0,        /* pointer */
      *i1;        /* pointer */

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
      n,          /* current input sample */
      nMax = 100000000;    /* last input sample (unless EOF) */

    MYFLT
      beta = FL(6.8),           /* parameter for Kaiser window */
      sum,                      /* scale factor for renormalizing windows */
      fdel,                     /* float del */
      idel,                     /* float del */
      fo,                       /* float o */
      of,                       /* fractional o */
      fL = FL(120.0),           /* float L */
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
      Chans = 0,                /* number of channels */
      chan,                     /* current channel */
      Q = 0;                    /* quality factor */

    FILE        *tvfp = NULL;   /* time-vary function file */
    SOUNDIN     *p;
    int         channel = ALLCHNLS;
    MYFLT       beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    char        *infile = NULL, *outfile = NULL, *bfile = NULL;
    SNDFILE     *inf = NULL;
    char        c, *s;
    char        *envoutyp = NULL;
    char        outformch = 's';
    unsigned    outbufsiz = 0U;
    SNDFILE     *outfd = NULL;
    OPARMS      *O = csound->oparms;
    int         block = 0;

    csound->e0dbfs = csound->dbfs_to_float = FL(1.0);

    O->filnamspace = outfile = (char*) csound->Malloc(csound, 1024);
    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        O->filetyp = TYP_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        O->filetyp = TYP_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        O->filetyp = TYP_IRCAM;
      else {
        sprintf(csound->errmsg, Str("%s not a recognized SFOUTYP env setting"),
                                envoutyp);
        dieu(csound, csound->errmsg);
        goto err_return;
      }
    }

    /* call getopt to interpret commandline */

    ++argv;
    while (--argc > 0) {
      s = *argv++;
      if (*s++ == '-') {                        /* read all flags:  */
        while ((c = *s++) != '\0') {
          switch (c) {
          case 'j':
            FIND("")
              while (*++s);
            break;
          case 'o':
            FIND(Str("no outfilename"))
              O->outfilename = outfile;            /* soundout name */
            while ((*outfile++ = *s++)); s--;
            if (strcmp(O->outfilename, "stdin") == 0) {
              csound->MessageS(csound, CSOUNDMSG_ERROR,
                                       Str("-o cannot be stdin"));
              goto err_return;
            }
#if defined mac_classic || defined WIN32
            if (strcmp(O->outfilename, "stdout") == 0) {
              csound->MessageS(csound, CSOUNDMSG_ERROR,
                                       Str("stdout audio not supported"));
              goto err_return;
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
            outformch = set_output_format(csound, c, outformch);
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
          case 'N':
            O->ringbell = 1;        /* notify on completion */
            break;
          case 'Q':
            FIND("No Q argument")
            sscanf(s,"%d", &Q);
            while (*++s);
            break;
          case 'P':
            FIND("No P argument")
#if defined(USE_DOUBLE)
            sscanf(s,"%lf", &P);
#else
            sscanf(s,"%f", &P);
#endif
            while (*++s);
            break;
          case 'r':
            FIND("No r argument")
#if defined(USE_DOUBLE)
            sscanf(s,"%lf", &Rout);
#else
            sscanf(s,"%f", &Rout);
#endif
            while (*++s);
            break;
          case 'i':
            FIND("No break file")
            tvflg = 1;
            bfile = s;
            while ((*s++)); s--;
            break;
          default:
            csound->Message(csound, "Looking at %c\n", c);
            usage(csound);    /* this exits with error */
            goto err_return;
          }
        }
      }
      else if (infile == NULL) {
        infile = --s;
        csound->Message(csound, "Infile set to %s\n", infile);
      }
      else {
        csound->Message(csound, "End with %s\n", s);
        usage(csound);
        goto err_return;
      }
    }
    if (infile == NULL) {
      csound->Message(csound, "No input given\n");
      usage(csound);
      goto err_return;
    }
    if ((inf = csound->SAsndgetset(csound, infile, &p, &beg_time,
                                   &input_dur, &sr, channel)) < 0) {
      csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error while opening %s"),
                                                infile);
      goto err_return;
    }
    if (Rin == FL(0.0))
      Rin = (MYFLT)p->sr;
    if (Chans == 0)
      Chans = (int) p->nchanls;
    if (Chans == 0)
      Chans = 1;

    if ((P != FL(0.0)) && (Rout != FL(0.0))) {
      sprintf(csound->errmsg, Str("srconv: can't specify both -r and -P"));
      goto err_rtn_msg;
    }
    if (P != FL(0.0))
      Rout = Rin / P;

    if (tvflg) {
      P = FL(0.0);        /* will be reset to max in time-vary function */
      if ((tvfp = fopen(bfile, "r")) == NULL) {
        sprintf(csound->errmsg,
                Str("srconv: can't open time-vary function file"));
        goto err_rtn_msg;
      }
      fscanf(tvfp, "%d", &tvlen);
      fxval = (MYFLT*) csound->Malloc(csound, tvlen * sizeof(MYFLT));
      fyval = (MYFLT*) csound->Malloc(csound, tvlen * sizeof(MYFLT));
      i0 = fxval;
      i1 = fyval;
      for (i = 0; i < tvlen; i++, i0++, i1++) {
#ifdef USE_DOUBLE
        if ((fscanf(tvfp, "%lf %lf", i0, i1)) == EOF) {
#else
        if ((fscanf(tvfp, "%f %f", i0, i1)) == EOF) {
#endif
          sprintf(csound->errmsg, Str("srconv: too few x-y pairs "
                                      "in time-vary function file"));
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
        sprintf(csound->errmsg, Str("srconv: first x value "
                                    "in time-vary function must be 0"));
        goto err_rtn_msg;
      }
      if (tvy0 <= FL(0.0)) {
        sprintf(csound->errmsg, Str("srconv: invalid initial y value "
                                    "in time-vary function"));
        goto err_rtn_msg;
      }
      if (tvdx <= FL(0.0)) {
        sprintf(csound->errmsg, Str("srconv: "
                                    "invalid x values in time-vary function"));
        goto err_rtn_msg;
      }
      tvdy = tvy1 - tvy0;
      tvslope = tvdy / tvdx;
      tvnxt = 1;
      /*
        csound->Message(csound, "len=%d\n", tvlen);
        csound->Message(csound, "x0: %f  y0: %f  x1: %f  y1: %f\n",
                                tvx0, tvy0, tvx1, tvy1);
      */
    }

    if (P != FL(0.0)) {         /* This is not right *********  */
      csound->esr = Rin;
    }
    if (P == FL(0.0)) {
      csound->esr = Rout;
    }
    if (O->outformat == 0)
      O->outformat = p->format;
    O->sfsampsize = sfsampsize(O->outformat);
    if (O->filetyp == TYP_RAW) {
      O->sfheader = 0;
      O->rewrt_hdr = 0;
    }
    else
      O->sfheader = 1;
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
      SF_INFO sfinfo;
      char    *name;
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) ((double) Rout + 0.5);
      sfinfo.channels = (int) p->nchanls;
      sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
      if (strcmp(O->outfilename, "stdout") != 0) {
        name = csound->FindOutputFile(csound, O->outfilename, "SFDIR");
        if (name == NULL) {
          sprintf(csound->errmsg, Str("cannot open %s."), O->outfilename);
          goto err_rtn_msg;
        }
        outfd = sf_open(name, SFM_WRITE, &sfinfo);
        csound->Free(csound, name);
      }
      else
        outfd = sf_open_fd(O->stdoutfd, SFM_WRITE, &sfinfo, 1);
      if (outfd == NULL) {
        sprintf(csound->errmsg, Str("cannot open %s."), O->outfilename);
        goto err_rtn_msg;
      }
      sf_command(outfd, SFC_SET_CLIPPING, NULL, SF_TRUE);
    }
    csound->esr = (MYFLT) p->sr;
    csound->nchnls = Chans = p->nchanls;

    outbufsiz = OBUF * O->sfsampsize;                   /* calc outbuf size */
    csound->Message(csound, Str("writing %d-byte blks of %s to %s"),
                            outbufsiz, getstrformat(csound, O->outformat),
                            O->outfilename);
    csound->Message(csound, " %s\n", type2string(O->filetyp));

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
    then window is impulse response of lowpass filter with cutoff frequency
    at half of Rin. */

    fdel = ((MYFLT) (L * Rin) / Rout);
    del = (int)fdel;
    idel = (MYFLT)del;
    if (del > L)
      N = del;
    if ((Q >= 1) && (Q <=4))
      M = Q * N * 10 + 1;
    if (tvflg)
      fdel = tvy0 * L;

    invRin  =  FL(1.0) / Rin;

    /* make window: the window is the product of a kaiser and a sin(x)/x */
    window = (MYFLT*) csound->Calloc(csound, (size_t) (M + 1) * sizeof(MYFLT));
    WinLen = (M-1)/2;
    window += WinLen;
    wLen = (M/2 - L) / L;

    kaiser(M, window, WinLen, 1, beta);
    for (i = 1; i <= WinLen; i++)
      *(window - i) = *(window + i);

    for (i = 1; i <= WinLen; i++){
      *(window - i) *= (MYFLT) N * (MYFLT) (sin((double) (PI * i) / (double) N)
                                            / (double) (PI * i));
      *(window + i) = *(window - i);
    }

    if (Rout < Rin){
      sum = *window;
      for (i = L-1; i <= WinLen; i += L)
        sum += *(window - i) + *(window + i);

      sum = FL(1.0) / sum;
    }
    else
      sum = FL(1.0) / *window;

    *window *= sum;
    for (i = 1; i <= WinLen; i++){
      *(window - i) *= sum;
      *(window + i) = *(window - i);
    }

    *(window + WinLen + 1) = FL(0.0);

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

    nread = csound->getsndin(csound, inf, input, IBUF2, p);
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
            *nextOut += *wj * *(input + k);
          }
          nextOut++;
          if (nextOut >= (output + OBUF)) {
            nextOut = output;
            writebuffer(csound, output, &block, outfd, OBUF);
          }
        }

        /* move window (window advances by del samples at L*Rin sample rate) */

        o += del;
        while (o >= L) {
          o -= L;
          n++;
          m++;
          if ((Chans * (m + wLen + 1)) >= mMax) {
            mMax += IBUF2;
            if (nextIn >= (input + IBUF))
              nextIn = input;
            nread = csound->getsndin(csound, inf, nextIn, IBUF2, p);
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
            iw = *wj + of * (*wj1 - *wj);
            *nextOut += iw * *(input + k);
          }
          nextOut++;
          if (nextOut >= (output + OBUF)) {
            nextOut = output;
            writebuffer(csound, output, &block, outfd, OBUF);
          }
        }

        /* move window */

        fo += fdel;
        while (fo >= fL) {
          fo -= fL;
          n++;
          m++;
          if ((Chans * (m + wLen + 1)) >= mMax) {
            mMax += IBUF2;
            if (nextIn >= (input + IBUF))
              nextIn = input;
            nread = csound->getsndin(csound, inf, nextIn, IBUF2, p);
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
                sprintf(csound->errmsg, Str("srconv: invalid x values "
                                            "in time-vary function"));
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
    writebuffer(csound, output, &block, outfd, nread);
    csound->Message(csound, "\n\n");
    if (tvfp != (FILE*) NULL)
      fclose(tvfp);
    sf_close(inf);
    sf_close(outfd);
    if (O->ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "\a");
    return 0;

 err_rtn_msg:
    csound->MessageS(csound, CSOUNDMSG_ERROR, "%s\n", csound->errmsg);
 err_return:
    if (tvfp != (FILE*) NULL)
      fclose(tvfp);
    if (inf != (SNDFILE*) NULL)
      sf_close(inf);
    if (outfd != (SNDFILE*) NULL)
      sf_close(outfd);
    return -1;
}

static const char *usage_txt[] = {
    "usage: srconv [flags] infile\n\nflags:",
    "-P num\tpitch transposition ratio (srate/r) [don't specify both P and r]",
    "-Q num\tquality factor (1, 2, 3, or 4: default = 2)",
    "-i filnam\tbreak file",
    "-r num\toutput sample rate (must be specified)",
    "-o fnam\tsound output filename\n",
    "-A\tcreate an AIFF format output soundfile",
    "-J\tcreate an IRCAM format output soundfile",
    "-W\tcreate a WAV format output soundfile",
    "-h\tno header on output soundfile",
    "-c\t8-bit signed_char sound samples",
    "-a\talaw sound samples",
    "-8\t8-bit unsigned_char sound samples",
    "-u\tulaw sound samples",
    "-s\tshort_int sound samples",
    "-l\tlong_int sound samples",
    "-f\tfloat sound samples",
    "-r N\torchestra srate override",
    "-K\tDo not generate PEAK chunks",
    "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)",
    "-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write",
    "-N\tnotify (ring the bell) when score or miditrack is done",
    "-- fnam\tlog output to file",
    NULL
};

static void usage(ENVIRON *csound)
{
    int i = -1;

    while (usage_txt[++i] != NULL)
      csound->Message(csound, "%s\n", Str(usage_txt[i]));
}

static char *type2string(int x)
{
    switch (x) {
    case TYP_WAV: return "WAV";
    case TYP_AIFF: return "AIFF";
    case TYP_AU: return "AU";
    case TYP_RAW: return "RAW";
    case TYP_PAF: return "PAF";
    case TYP_SVX: return "SVX";
    case TYP_NIST: return "NIST";
    case TYP_VOC: return "VOC";
    case TYP_IRCAM: return "IRCAM";
    case TYP_W64: return "W64";
    case TYP_MAT4: return "MAT4";
    case TYP_MAT5: return "MAT5";
    case TYP_PVF: return "PVF";
    case TYP_XI: return "XI";
    case TYP_HTK: return "HTK";
#ifdef SF_FORMAT_SDS
    case TYP_SDS: return "SDS";
#endif
    default:
      return "(unknown)";
    }
}

static short sfsampsize(int type)
{
    switch (type&SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_S8:
      return 1;
    case SF_FORMAT_ALAW:
      return 1;
    case SF_FORMAT_ULAW:
      return 1;
    case SF_FORMAT_PCM_16:
      return 2;       /* Signed 16 bit data */
    case SF_FORMAT_PCM_32:
      return 4;       /* Signed 32 bit data */
    case SF_FORMAT_FLOAT:
      return 4;       /* 32 bit float data */
    case SF_FORMAT_PCM_U8:
      return 1;       /* Unsigned 8 bit data (WAV and RAW only) */
    case SF_FORMAT_PCM_24:
      return 3;       /* Signed 24 bit data */
    case SF_FORMAT_DOUBLE:
      return 8;       /* 64 bit float data */
    }
    return 1;
}

static char *getstrformat(ENVIRON *csound, int format)
{
    switch (format) {
      case  AE_UNCH:    return Str("unsigned bytes"); /* J. Mohr 1995 Oct 17 */
      case  AE_CHAR:    return Str("signed chars");
      case  AE_ALAW:    return Str("alaw bytes");
      case  AE_ULAW:    return Str("ulaw bytes");
      case  AE_SHORT:   return Str("shorts");
      case  AE_LONG:    return Str("longs");
      case  AE_FLOAT:   return Str("floats");
      case  AE_24INT:   return Str("24bit ints");     /* RWD 5:2001 */
    }
    return Str("unknown");
}

static MYFLT ino(MYFLT x)
{
    MYFLT   y, t, e, de, sde, xi;
    int     i;

    y = x * FL(0.5);
    t = FL(1.0e-08);
    e = FL(1.0);
    de = FL(1.0);
    for (i = 1; i <= 25; i++) {
      xi = (MYFLT)i;
      de = de * y / xi;
      sde = de * de;
      e += sde;
      if (e * t > sde)
        break;
    }
    return(e);
}

static void kaiser(int nf, MYFLT *w, int n, int ieo, MYFLT beta)
{

/*
   nf = filter length in samples
    w = window array of size n
    n = filter half length=(nf+1)/2
  ieo = even odd indicator--ieo=0 if nf even
 beta = parameter of kaiser window
*/

    MYFLT   bes, xind, xi;
    int     i;

    bes = ino(beta);
    xind = (MYFLT)(nf-1)*(nf-1);

    for (i = 0; i < n; i++) {
      xi = (MYFLT)i;
      if (ieo == 0)
        xi += 0.5;
      xi = FL(4.0) * xi * xi;
      xi = (MYFLT)sqrt(1.0 - (double)(xi / xind));
      w[i] = ino(beta * xi);
      w[i] /= bes;
    }
    return;
}

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    return (((ENVIRON*) csound)->AddUtility(csound, "srconv", srconv));
}

