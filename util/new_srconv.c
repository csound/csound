/*
    srconv.c

    Copyright (C) 2015 John ffitch after Mark Dolson and Eric de Catro Lopo

    This file is associated with Csound.

** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

/*
 *    PROGRAM:   srconv - sample rate converter
 *
 *    COMMENTS:  srconv takes at sample rate Rin and
 *               outputs them at sample rate Rout.  optionally,
 *               the ratio (Rin / Rout)  may be linearly time-varying
 *               according to a set of (time, ratio) pairs in an auxiliary file.
 *
 *               flags:
 *
 *                    r = output sample rate (must be specified)
 *                    R = input sample rate (must be specified)
 *                    P = input sample rate / output sample rate
 *                    Q = quality factor (1 to 5: default = 3)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sndfile.h"
#include "samplerate.h"

#define IBUF    (8192)
#define OBUF    (8192)

#ifdef GNU_GETTEXT
# define Str(x) LocalizeString(x)
#else
# define Str(x)  (x)
#endif

#define Str_noop(x) x

static int rewrt_hdr = 0, heartbeat = 0, ringbell = 0;
static int filetyp, outformat;
static char* outfilename = NULL;
static block = 0;
#define FIND(MSG)                                                   \
{                                                                   \
    if (*s == '\0')                                                 \
      if (!(--argc) || (((s = *argv++) != NULL) && *s == '-')) {    \
        dieu(MSG); return -1;                               \
      }                                                             \
}


static  void    usage(void);

static void heartbeater(void)
{
    switch (heartbeat) {
    default:
    case 1:
      fprintf(stderr, "%c\010","|/-\\"[block & 3]);
      break;
    case 2:
      fprintf(stderr, ".");
      break;
    case 3:
      {
        int n;
        fprintf(stderr, "%d%n", block, &n);
        while (n--) fprintf(stderr, "\010");
      }
      break;
    case 4:
      fprintf(stderr, "\007");
    }
    ++block;
    return;
}

static char set_output_format(char c, char outformch, int *outformat)
{
    if (outformat) {
      fprintf(stderr, Str("Sound format -%c has been overruled by -%c"),
              outformch, c);
    }
    switch (c) {
    case 'a':
      *outformat = SF_FORMAT_ALAW;  /* a-law soundfile */
      break;
    case 'c':
      *outformat = SF_FORMAT_PCM_S8;  /* signed 8-bit soundfile */
      break;
    case '8':
      *outformat = SF_FORMAT_PCM_U8;  /* unsigned 8-bit soundfile */
     break;
    case 'f':
      *outformat = SF_FORMAT_FLOAT; /* float soundfile */
      break;
    case 's':
      *outformat = SF_FORMAT_PCM_16; /* short_int soundfile*/
      break;
    case 'l':
      *outformat = SF_FORMAT_PCM_32;  /* long_int soundfile */
      break;
    case 'u':
      *outformat = SF_FORMAT_ULAW;  /* mu-law soundfile */
      break;
    case '3':
      *outformat = SF_FORMAT_PCM_24; /* 24bit packed soundfile*/
      break;
    case 'e':
      *outformat = SF_FORMAT_FLOAT; /* float sndfile (for rescaling) */
      break;
    default:
      *outformat = 0;
      fprintf(stderr, Str("srconv: unknown outout format '%c'\n"), c);
      return outformch; /* do nothing */
    };
    return c;
}

static void dieu(char *s)
{
    fprintf(stderr, "srconv: %s", s);
    usage();
}

int main(int argc, char **argv)
{
    SF_INFO sfinfo;

    double
      *fxval = 0, /* pointer to start of time-array for time-vary function */
      *fyval = 0, /* pointer to start of P-scale-array for time-vary func */
      *i0,        /* pointer */
      *i1;        /* pointer */

    double
      tvx0 = 0,                 /* current x value of time-var function */
      tvx1 = 0,                 /* next x value of time-var function */
      tvdx,                     /* tvx1 - tvx0 */
      tvy0 = 0,                 /* current y value of time-var function */
      tvy1 = 0,                 /* next y value of time-var function */
      tvdy,                     /* tvy1 - tvy0 */
      tvslope = 0,              /* tvdy / tvdx */
      P = 0.0,              /* Rin / Rout */
      Rin = 0.0,            /* input sampling rate */
      Rout = 0.0;           /* output sample rate */

    int
      i,k,                      /* index variables */
      tvflg = 0,                /* flag for time-varying time-scaling */
      tvnxt = 0,                /* counter for stepping thru time-var func */
      tvlen,                    /* length of time-varying function */
      Chans = 1,                /* number of channels */
      Q = 3;                    /* quality factor */

    FILE        *tvfp = NULL;   /* time-vary function file */
    char        *infile = NULL, *bfile = NULL;
    SNDFILE     *inf = NULL;
    SNDFILE     *outf = NULL;
    char        c, *s;
    const char  *envoutyp;
    char        outformch = 's';
    char        err_msg[256];

    outformat = SF_FORMAT_PCM_16;

    if ((envoutyp = getenv("SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        filetyp = SF_FORMAT_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        filetyp = SF_FORMAT_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        filetyp = SF_FORMAT_RAW;
      /* Add new types here */
      else {
        snprintf(err_msg, 256, Str("%s not a recognized SFOUTYP env setting"),
                 envoutyp);
        dieu(err_msg);
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
            outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (strcmp(outfilename, "stdin") == 0) {
              fprintf(stderr, Str("-o cannot be stdin"));
              return -1;
            }
            break;
          case 'A':
            filetyp = SF_FORMAT_AIFF; /* AIFF output request*/
            break;
          case 'J':
            filetyp = SF_FORMAT_RAW; /* IRCAM output request */
            break;
          case 'W':
            filetyp = SF_FORMAT_WAV; /* WAV output request */
            break;
          case 'h':
            filetyp = SF_FORMAT_RAW; /* skip sfheader  */
            break;
          case 'c':
          case '8':
          case 'a':
          case 'u':
          case 's':
          case 'l':
          case '3':
          case 'f':
            outformch = set_output_format(c, outformch, &outformat);
            break;
          case 'R':
            rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              int n;
              sscanf(s, "%d%n", &heartbeat, &n);
              s += n;
            }
            else heartbeat = 1;
            break;
          case 'N':
            ringbell = 1;        /* notify on completion */
            break;
          case 'Q':
            FIND(Str("No Q argument"))
            sscanf(s,"%d", &Q);
            while (*++s);
            break;
          case 'P':
            FIND(Str("No P argument"))
            sscanf(s,"%lf", &P);
            while (*++s);
            break;
          case 'r':
            FIND(Str("No r argument"))
            sscanf(s,"%lf", &Rout);
            while (*++s);
            break;
          case 'i':
            FIND(Str("No break file"))
            tvflg = 1;
            bfile = s;
            while ((*s++)) {}; s--;
            break;
          default:
            printf(Str("Looking at %c\n"), c);
            usage();    /* this exits with error */
            return -1;
          }
        }
      }
      else if (infile == NULL) {
        infile = --s;
        //printf(Str("Infile set to %s\n"), infile);
      }
      else {
        printf(Str("End with %s\n"), s);
        usage();
        return -1;
      }
    }
    if (infile == NULL) {
      printf(Str("No input given\n"));
      usage();
      return -1;
    }
    switch (Q) {
    case 5:
      Q = SRC_SINC_BEST_QUALITY;
      break;
    case 4:
      Q = SRC_SINC_MEDIUM_QUALITY;
      break;
    case 3:
    default:
      Q = SRC_SINC_FASTEST;
      break;
    case 2:
      Q = SRC_ZERO_ORDER_HOLD;
      break;
    case 1:
      Q = SRC_LINEAR;
      break;
    }

    if ((P != 0.0) && (Rout != 0.0)) {
      strncpy(err_msg, Str("srconv: cannot specify both -r and -P"), 256);
      err_msg[255] = '\0';
      goto err_rtn_msg;
    }

    if ((inf = sf_open(infile, SFM_READ, &sfinfo)) == NULL) {
      fprintf(stderr, Str("error while opening %s"), infile);
      return -1;
    }

    Rin = sfinfo.samplerate;
    Chans = sfinfo.channels;
    if (P != 0.0)
      Rout = Rin / P;
    else if (Rout == 0.0)
      Rout = Rin;
    P = Rout/Rin;
    //printf("P=%f, Rin=%f, Rout=%f\n", P, Rin, Rout);
    if (tvflg) {
      P = 0.0;        /* will be reset to max in time-vary function */
      if ((tvfp = fopen(bfile, "r")) == NULL) {
        strncpy(err_msg,
                Str("srconv: cannot open time-vary function file"), 256);
        goto err_rtn_msg1;
      }
      if (fscanf(tvfp, "%d", &tvlen) != 1)
        printf(Str("Read failure\n"));
      if (tvlen <= 0) {
        strncpy(err_msg, Str("srconv: tvlen <= 0 "), 256);
        goto err_rtn_msg1;
      }
      fxval = (double*) malloc(tvlen * sizeof(double));
      fyval = (double*) malloc(tvlen * sizeof(double));
      i0 = fxval;
      i1 = fyval;
      for (i = 0; i < tvlen; i++, i0++, i1++) {
        if (fscanf(tvfp, "%lf %lf", i0, i1) != 2) {
          strncpy(err_msg, Str("srconv: too few x-y pairs "
                                "in time-vary function file"), 256);
          err_msg[255] = '\0';
          goto err_rtn_msg1;
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
      if (tvx0 != 0.0) {
        strncpy(err_msg, Str("srconv: first x value "
                             "in time-vary function must be 0"), 256);
        goto err_rtn_msg1;
      }
      if (tvy0 <= 0.0) {
        strncpy(err_msg, Str("srconv: invalid initial y value "
                             "in time-vary function"),256);
        goto err_rtn_msg1;
      }
      if (tvdx <= 0.0) {
        strncpy(err_msg,
                       Str("srconv: invalid x values in time-vary function"),
                       256);
        goto err_rtn_msg1;
      }
      tvdy = tvy1 - tvy0;
      tvslope = tvdy / tvdx;
      tvnxt = 1;
    }
        
    if (outformat == 0) outformat = SF_FORMAT_PCM_16;
    if (filetyp == SF_FORMAT_RAW) rewrt_hdr = 0;
    if (outfilename == NULL) {
      if (filetyp == SF_FORMAT_WAV)
        outfilename = "test.wav";
      else if (filetyp == SF_FORMAT_AIFF)
        outfilename = "test.aif";
      else
        outfilename = "test";
    }
    sfinfo.samplerate = (int) ((double) Rout + 0.5);
    //printf("filetyp=%x outformat=%x\n", filetyp, outformat);
    sfinfo.format = filetyp | outformat;
    outf = sf_open(outfilename, SFM_WRITE, &sfinfo);
    if (outf == NULL) {
      snprintf(err_msg, 256, Str("cannot open %s."), outfilename);
      goto err_rtn_msg1;
    }
    sf_command(outf, SFC_SET_CLIPPING, NULL, SF_TRUE);

    {
      SRC_STATE *state;
      SRC_DATA  data;
      int err;
      static float input[IBUF] ;
      static float output[OBUF];
      int C = OBUF/Chans;
      int count = 0;
      
      state = src_new(Q, Chans, &err);
      if (state==NULL) {
        printf("Error: failed to initialise SRC -- %s\n", src_strerror(err));
        sf_close(inf); sf_close(outf);
        usage();
        exit(1);
      }
      data.end_of_input = 0;  /* Not end yet */
      data.input_frames = 0;
      data.data_in = input;
      data.src_ratio = P;
      data.data_out = output;
      data.output_frames = C;
      for (;;) {
        if (data.input_frames==0) {
          if (C!= (data.input_frames = sf_readf_float(inf, input, C)))
            data.end_of_input = SF_TRUE;
          data.data_in = input;
        }
        err = src_process(state, &data);
        if (err) {
          printf("srconv: error: %s\n", src_strerror(err));
          sf_close(inf); sf_close(outf);
          exit(1);
        }
        if (data.end_of_input && data.output_frames_gen == 0) break;
        sf_writef_float(outf, output, data.output_frames_gen);
        if (rewrt_hdr)
          sf_command(outf, SFC_UPDATE_HEADER_NOW, NULL, 0);
        if (heartbeat) heartbeater();
        count += data.output_frames_gen;
        data.data_in += data.input_frames_used * Chans;
        data.input_frames -= data.input_frames_used;
      }
      state = src_delete(state);
      printf("wrote %d frames\n", count);
      sf_close(inf); sf_close(outf);
      exit(0);

    }
  err_rtn_msg1:
    sf_close(inf);
  err_rtn_msg:
    fprintf(stderr, err_msg);
    return -1;
}    

static const char *usage_txt[] = {
  Str_noop("usage: srconv [flags] infile\n\nflags:"),
  Str_noop("-P num\tpitch transposition ratio (srate/r) [do not specify "
           "both P and r]"),
  Str_noop("-Q num\tquality factor (1 to 5: default = 3)"),
  Str_noop("-i filnam\tbreak file"),
  Str_noop("-r num\toutput sample rate (must be specified)"),
  Str_noop("-o fnam\tsound output filename\n"),
  Str_noop("-A\tcreate an AIFF format output soundfile"),
  Str_noop("-J\tcreate an IRCAM format output soundfile"),
  Str_noop("-W\tcreate a WAV format output soundfile"),
  Str_noop("-c\t8-bit signed_char sound samples"),
  Str_noop("-a\talaw sound samples"),
  Str_noop("-8\t8-bit unsigned_char sound samples"),
  Str_noop("-u\tulaw sound samples"),
  Str_noop("-s\tshort_int sound samples"),
  Str_noop("-l\tlong_int sound samples"),
  Str_noop("-f\tfloat sound samples"),
  Str_noop("-K\tDo not generate PEAK chunks"),
  Str_noop("-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)"),
  Str_noop("-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write"),
  Str_noop("-N\tnotify (ring the bell) when done"),
  Str_noop("-- fnam\tlog output to file"),
    NULL
};

static void usage(void)
{
    int i = -1;

    while (usage_txt[++i] != NULL)
      printf("%s\n", Str(usage_txt[i]));
}
