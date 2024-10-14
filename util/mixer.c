/*
    mixer.c

    Copyright (C) 1995 John ffitch

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

/*******************************************************\
*   mixer.c                                             *
*   mix a set of sound files with arbitary starts       *
*   jpff 23 Sep 1994                                    *
*   including lifting from Csound itself                *
\*******************************************************/


/* Notes:
 *     This makes a mess of multichannel inputs.
 *     Needs to take much more care
 */

#include "std_util.h"
#include "soundio.h"
#include <ctype.h>
#include <inttypes.h>

/* Constants */

#define NUMBER_OF_SAMPLES (65536)
#define NUMBER_OF_FILES   (32)

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))            \
      csound->Die(csound, Str("mixer: error: %s"), MSG);

typedef struct scalepoint {
    MYFLT y0;
    MYFLT y1;
    MYFLT yr;
    int32_t x0;
    int32_t x1;
    struct scalepoint *next;
} scalepoint;

typedef struct inputs {
    long        start;          /* Time this file starts in samples */
    MYFLT       time;           /* Time this file starts in secs */
    char *      name;           /* Name of file */
    int32_t         use_table;      /* Should we use multiplier or table */
    MYFLT       factor;         /* Gain factor */
    char *      fname;          /* Name of scale table file */
    scalepoint *fulltable;      /* Scaling table */
    scalepoint *table;          /* current position in table */
    SNDFILE    *fd;             /* File descriptor handle */
    int16       channels[5];    /* destinations of channels */
    int32_t         non_clear;      /* Boolean to say if fiddled mixing */
    SOUNDIN *   p;              /* Csound structure */
} inputs;

typedef struct mixer_globals_ {
    CSOUND    *csound;
    inputs    mixin[NUMBER_OF_FILES];
    int32_t   outputs;
    int32_t   debug;
    uint32_t  outbufsiz;
    MYFLT     *out_buf;
    int32_t   outrange;                 /* Count samples out of range */
} MIXER_GLOBALS;

/* Static function prototypes */

static  void    InitScaleTable(MIXER_GLOBALS *, int32_t);
static  MYFLT   gain(MIXER_GLOBALS *, int32_t, int32_t);
static  SNDFILE *MXsndgetset(CSOUND*,inputs *);
static  void    MixSound(MIXER_GLOBALS *, int32_t, SNDFILE *, OPARMS *);

static const char *usage_txt[] = {
  Str_noop("Usage:\tmixer [-flags] soundfile [-flags] soundfile ..."),
  Str_noop("Legal flags are:"),
  Str_noop("-o fnam\tsound output filename"),
  Str_noop("-A\tcreate an AIFF format output soundfile"),
  Str_noop("-W\tcreate a WAV format output soundfile"),
  Str_noop("-h\tno header on output soundfile"),
  Str_noop("-8\t8-bit unsigned_char sound samples"),
  Str_noop("-c\t8-bit signed_char sound samples"),
  Str_noop("-8\t8-bit unsigned_char sound samples"),
  Str_noop("-a\talaw sound samples"),
  Str_noop("-u\tulaw sound samples"),
  Str_noop("-s\tshort_int sound samples"),
  Str_noop("-l\tlong_int sound samples"),
  Str_noop("-f\tfloat sound samples"),
  Str_noop("-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)"),
  Str_noop("-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write"),
  Str_noop("-N\tnotify (ring the bell) when score or miditrack is done"),
  Str_noop("-F fpnum\tamount to scale amplitude for next input"),
  Str_noop("-F fname\tfile of a scale table for next input"),
  Str_noop("-S integer\tsample number at which to insert file"),
  Str_noop("-T fpnum\ttime at which to insert file"),
  Str_noop("-1 -2 -3 -4\tinclude named channel"),
  Str_noop("-^ n m\tinclude channel n and output as channel m"),
  Str_noop("-v\tverbose mode for debugging"),
  Str_noop("-- fname\tLog output to file"),
  Str_noop("flag defaults: mixer -s -otest -F 1.0 -S 0"),
    NULL
};

static void usage(CSOUND *csound, const char *mesg, ...)
{
    const char  **sp;
    va_list     args;

    for (sp = &(usage_txt[0]); *sp != NULL; sp++)
      csound->Message(csound, "%s\n", Str(*sp));

    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("mixer: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static char set_output_format(CSOUND *csound, char c, char outformch, OPARMS *O)
{
  (void) csound;
    switch (c) {
      case 'a': O->outformat = AE_ALAW;   /* a-law soundfile */
                break;
      case 'c': O->outformat = AE_CHAR;   /* signed 8-bit soundfile */
                break;
      case '8': O->outformat = AE_UNCH;   /* unsigned 8-bit soundfile */
                break;
      case 'f': O->outformat = AE_FLOAT;  /* float soundfile */
                break;
      case 's': O->outformat = AE_SHORT;  /* short_int soundfile*/
                break;
      case 'l': O->outformat = AE_LONG;   /* long_int soundfile */
                break;
      case 'u': O->outformat = AE_ULAW;   /* mu-law soundfile */
                break;
      case '3': O->outformat = AE_24INT;  /* 24bit packed soundfile*/
                break;
      case 'e': O->outformat = AE_FLOAT;  /* float soundfile (for rescaling) */
                break;
      default:  return outformch;         /* do nothing */
    }
    return c;
}

static int32_t mixer_main(CSOUND *csound, int32_t argc, char **argv)
{
    OPARMS *     O = (OPARMS *) csound->Calloc(csound, sizeof(OPARMS));
    char        *inputfile = NULL;
    SNDFILE     *outfd;
    int32_t         i;
    char        outformch='s', c, *s;
    const char  *envoutyp;
    int32_t         n = 0;
    SFLIB_INFO     sfinfo;
    MIXER_GLOBALS *pp = (MIXER_GLOBALS*) csound->Calloc(csound,
                                                        sizeof(MIXER_GLOBALS));
    inputs      *mixin = &(pp->mixin[0]);

    memcpy(O,csound->GetOParms(csound), sizeof(OPARMS));

    pp->csound = csound;
    /*csound->dbfs_to_float = csound->e0dbfs = FL(1.0);*/
    /* Check arguments */
    if ((envoutyp = csound->GetEnv(csound, "SFOUTYP")) != NULL) {
      if (strcmp(envoutyp, "AIFF") == 0)
        O->filetyp = TYP_AIFF;
      else if (strcmp(envoutyp, "WAV") == 0)
        O->filetyp = TYP_WAV;
      else if (strcmp(envoutyp, "IRCAM") == 0)
        O->filetyp = TYP_IRCAM;
      else {
        csound->ErrorMsg(csound, Str("%s not a recognized SFOUTYP env setting"),
                                 envoutyp);
        return -1;
      }
    }
    mixin[n].start = -1; mixin[n].time = -FL(1.0);
    mixin[n].factor = FL(1.0); mixin[n].non_clear = 0;
    mixin[n].fulltable = NULL; mixin[n].use_table = 0;
    for (i=1; i<5; i++) mixin[n].channels[i] = 0;
    if (UNLIKELY(!(--argc)))
      usage(csound,"%s", Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                  /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            O->outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (UNLIKELY(strcmp(O->outfilename, "stdin") == 0))
              csound->Die(csound, "%s", Str("mixer: -o cannot be stdin"));
#if defined(WIN32)
            if (UNLIKELY(strcmp(O->outfilename,"stdout") == 0)) {
              csound->Die(csound, "%s", Str("mixer: stdout audio not supported"));
            }
#endif
            break;
          case 'A':
            O->filetyp = TYP_AIFF;      /* AIFF output request  */
            break;
          case 'J':
            O->filetyp = TYP_IRCAM;     /* IRCAM output request */
            break;
          case 'W':
            O->filetyp = TYP_WAV;       /* WAV output request  */
            break;
          case 'F':
            FIND(Str("no scale factor"));
            if (isdigit(*s) || *s == '-' || *s == '+')
              mixin[n].factor = (MYFLT) atof(s);
            else {
              mixin[n].fname = (char*) csound->Malloc(csound, strlen(s) + 1);
              strcpy(mixin[n].fname, s);
              mixin[n].use_table = 1;
           }
            while (*++s);
            break;
          case 'S':
            FIND(Str("no start sample"));
            mixin[n].start = atoi(s);
            while (*++s);
            if (UNLIKELY(mixin[n].time >= FL(0.0))) {
              csound->Warning(csound, "%s", Str("-S overriding -T"));
              mixin[n].time = -FL(1.0);
            }
            break;
          case 'T':
            FIND(Str("no start time"));
            mixin[n].time = (MYFLT) atof(s);
            while (*++s);
            if (UNLIKELY(mixin[n].start >= 0)) {
              csound->Warning(csound, "%s", Str("-T overriding -S"));
              mixin[n].start = -1;
            }
            break;
          case '1':
          case '2':
          case '3':
          case '4':
            {
              int32_t src = c - '0';
              if (src > pp->outputs)
                pp->outputs = src;
              mixin[n].channels[src] = src;
              mixin[n].non_clear = 1;
              break;
            }
          case '^':
            {
              int32_t src = c, dst;
              FIND(Str("no source channel number"));
              src = atoi(s);
              while (*++s);
              FIND(Str("no destination channel number"));
              dst = atoi(s);
              while (*++s);
              if (UNLIKELY(src > 4 || src < 1 || dst > 4 || dst < 1)) {
                csound->Warning(csound, "%s",
                                Str("illegal channel number ignored"));
                break;
              }
              if (dst > pp->outputs)
                pp->outputs = dst;
              mixin[n].channels[dst] = src;
              mixin[n].non_clear = 1;
              break;
            }
          case 'h':
            O->filetyp = TYP_RAW;       /* skip sfheader  */
            break;
          case 'c':
          case 'a':
          case 'u':
          case '8':
          case 's':
          case 'l':
          case 'f':
            outformch = set_output_format(csound, c, outformch, O);
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
            O->ringbell = 1;             /* notify on completion */
            break;
          case 'v':                       /* Verbose mode */
            pp->debug = 1;
            break;
          default:
            usage(csound, Str("unknown flag -%c"), c);
          }
      else {
        int32_t i;
        mixin[n].name = --s;
        if (!mixin[n].non_clear)
          for (i=1; i<5; i++) mixin[n].channels[i] = i;
        if (UNLIKELY(n++ >= NUMBER_OF_FILES-1)) {
          usage(csound,Str("Too many mixin"));
        }
        mixin[n].start = -1;
        mixin[n].time = -1;
        mixin[n].factor = FL(1.0);
        mixin[n].non_clear = 0;
      }
    } while (--argc);

    /* Read sound files */
    if (UNLIKELY(n == 0)) {
      csound->ErrorMsg(csound, "%s", Str("No mixin"));
      return -1;
    }
    for (i = 0; i < n; i++) {
      if (UNLIKELY(!MXsndgetset(csound, &mixin[i]))) {
        csound->ErrorMsg(csound, Str("%s: error while opening %s"),
                                 argv[0], inputfile);
        return -1;
      }
      mixin[i].p->channel = ALLCHNLS;
      if (i>0) {
        if (UNLIKELY(mixin[0].p->sr != mixin[i].p->sr)) {
          csound->ErrorMsg(csound, "%s", Str("Input formats not the same"));
          return -1;
        }
      }
      if (mixin[i].non_clear) {
        int32_t j;
        for (j = 1; j<5; j++)
          if (pp->outputs < mixin[i].channels[j]) {
            pp->outputs = mixin[i].channels[j];
          }
      }
      else if (pp->outputs < mixin[i].p->nchanls)
        pp->outputs = mixin[i].p->nchanls;
      if (mixin[i].time >= FL(0.0)) {
        MYFLT sval = (MYFLT) mixin[i].time * (MYFLT) mixin[i].p->sr;
        mixin[i].start = (long) MYFLT2LRND(sval);
      }
      else if (mixin[i].start < 0L)
        mixin[i].start = 0L;
      if (mixin[i].use_table) InitScaleTable(pp, i);
    }

    if (!O->outformat)                      /* if no audioformat yet  */
      O->outformat = mixin[0].p->format;    /* Copy from first input file */
    O->sfsampsize = csound->SndfileSampleSize(FORMAT2SF(O->outformat));
    if (!O->filetyp)
      O->filetyp = mixin[0].p->filetyp;     /* Copy from input file */
    
    if (O->filetyp == TYP_RAW)       /* can't rewrite header if no header requested */
      O->rewrt_hdr = 0;
#ifdef NeXT
    if (O->outfilename == NULL && !O->filetyp) O->outfilename = "test.snd";
    else if (O->outfilename == NULL) O->outfilename = "test";
#else
    if (O->outfilename == NULL) {
      if (O->filetyp == TYP_WAV) O->outfilename = "test.wav";
      else if (O->filetyp == TYP_AIFF) O->outfilename = "test.aif";
      else O->outfilename = "test";
    }
#endif
    (csound->GetUtility(csound))->SetUtilSr(csound, (MYFLT)mixin[0].p->sr);
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    //sfinfo.frames = 0/*was -1*/;
    sfinfo.samplerate = mixin[0].p->sr;
    sfinfo.channels /*= csound->nchnls*/ = (int32_t) mixin[0].p->nchanls;
    sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
    if (strcmp(O->outfilename, "stdout") == 0) {
      outfd = csound->SndfileOpenFd(csound,1, SFM_WRITE, &sfinfo, 0);
      if (outfd != NULL) {
        if (UNLIKELY(csound->CreateFileHandle(csound,
                                              &outfd, CSFILE_SND_W,
                                              "stdout") == NULL)) {
          csound->SndfileClose(csound,outfd);
          return -1;
        }
      }
    }
    else if (csound->FileOpen(csound, &outfd, CSFILE_SND_W, O->outfilename,
                       &sfinfo, "SFDIR", csound->Type2CsfileType(O->filetyp,
                       O->outformat), 0) == NULL)
      outfd = NULL;
    if (UNLIKELY(outfd == NULL)) {
      csound->ErrorMsg(csound, Str("mixer: error opening output file '%s': %s"),
                       O->outfilename, Str(csound->SndfileStrError(csound,NULL)));
      return -1;
    }
    if (UNLIKELY(O->rewrt_hdr))
      csound->SndfileCommand(csound,outfd, SFC_SET_UPDATE_HEADER_AUTO, NULL, 0);
    /* calc outbuf size & alloc bufspace */
    pp->outbufsiz = NUMBER_OF_SAMPLES * pp->outputs;
    pp->out_buf = csound->Malloc(csound, pp->outbufsiz * sizeof(MYFLT));
    pp->outbufsiz *= O->sfsampsize;
    csound->Message(csound, Str("writing %d-byte blks of %s to %s (%s)\n"),
                            pp->outbufsiz,
                            csound->GetStrFormat(O->outformat), O->outfilename,
                           csound->Type2String(O->filetyp));
    MixSound(pp, n, outfd, O);
    if (O->ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "\007");
    return 0;
}

static void
InitScaleTable(MIXER_GLOBALS *pp, int32_t i)
{
    CSOUND *csound = pp->csound;
    FILE    *f;
    inputs  *mixin = &(pp->mixin[0]);
    MYFLT   samplepert = (MYFLT) mixin[i].p->sr;
    MYFLT   x, y;
    scalepoint *tt = (scalepoint*) csound->Malloc(csound, sizeof(scalepoint));

    if (UNLIKELY(csound->FileOpen(csound, &f, CSFILE_STD, mixin[i].fname,
                                   "r", NULL, CSFTYPE_FLOATS_TEXT, 0) == NULL)) {
      csound->Die(csound, Str("Cannot open scale table file %s"),
                          mixin[i].fname);
      return;   /* not reached */
    }
    mixin[i].fulltable = mixin[i].table = tt;
    tt->x0 = 0; tt->y0 = FL(0.0); tt->x1 = 0; tt->y1 = FL(0.0);
    tt->yr = FL(0.0); tt->next = NULL;
#ifdef USE_DOUBLE
    while (fscanf(f, "%lf %lf\n", &x, &y) == 2) {
#else
    while (fscanf(f, "%f %f\n", &x, &y) == 2) {
#endif
      scalepoint *newpoint;
      newpoint = (scalepoint*) csound->Malloc(csound, sizeof(scalepoint));
      newpoint->x0 = tt->x1;
      newpoint->y0 = tt->y1;
      newpoint->x1 = (int32_t) (x*samplepert);
      newpoint->y1 = y;
      if (newpoint->x1 == newpoint->x0) {
        MYFLT div = (MYFLT)(tt->x1 - tt->x0);
        tt->y1 = y;
        if (LIKELY(div))
          tt->yr = (y - tt->y0)/div;
        else  tt->yr = y;
        csound->Free(csound, newpoint);
      }
      else {
        newpoint->yr =
          (y - newpoint->y0)/((MYFLT)(newpoint->x1 - newpoint->x0));
        tt->next = newpoint;
        newpoint->next = NULL;
        tt = newpoint;
      }
    }
    {
      scalepoint *newpoint =
          (scalepoint*) csound->Malloc(csound, sizeof(scalepoint));
      tt->next = newpoint;
      newpoint->x0 = tt->x1;
      newpoint->y0 = tt->y1;
      newpoint->x1 = 0x7fffffff;
      newpoint->y1 = FL(0.0);
      newpoint->next = NULL;
      newpoint->yr = (x == newpoint->x0 ?
                      -newpoint->y0 :
                      -newpoint->y0/((MYFLT)(0x7fffffff-newpoint->x0)));
    }
    if (pp->debug) {
      scalepoint *tt = mixin[i].table;
      csound->Message(csound, "Scale table is\n");
      while (tt != NULL) {
        csound->Message(csound,  "(%d %f) -> %d %f [%f]\n",
                    tt->x0, tt->y0, tt->x1, tt->y1, tt->yr);
        tt = tt->next;
      }
      csound->Message(csound,  "END of Table\n");
    }
    mixin[i].use_table = 1;
}

static MYFLT gain(MIXER_GLOBALS *pp, int32_t n, int32_t i)
{
    CSOUND *csound = pp->csound;
    inputs  *mixin = &(pp->mixin[0]);

    if (!mixin[n].use_table) return mixin[n].factor;
    if (i<mixin[n].table->x0) mixin[n].table = mixin[n].fulltable;
    while (i<mixin[n].table->x0 ||
           i>=mixin[n].table->x1) {/* Get correct segment */
      if (UNLIKELY(pp->debug))
        csound->Message(csound, "Table %d: %d (%d %f) -> %d %f [%f]\n",
                        n, i, mixin[n].table->x0, mixin[n].table->y0,
                        mixin[n].table->x1, mixin[n].table->y1,
                        mixin[n].table->yr);
      mixin[n].table = mixin[n].table->next;
    }
    return mixin[n].factor*(mixin[n].table->y0 +
                            mixin[n].table->yr*(MYFLT)(i - mixin[n].table->x0));
}

static SNDFILE *MXsndgetset(CSOUND *csound, inputs *ddd)
{
    SNDFILE *infd;
    MYFLT   dur;
    SOUNDIN *p;

    (csound->GetUtility(csound))->SetUtilSr(csound, FL(0.0));         /* set esr 0. with no orchestra   */
    ddd->p = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->analonly = 1;
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strNcpy(p->sfname, ddd->name, MAXSNDNAME-1);
    /* open sndfil, do skiptime */
    if (UNLIKELY((infd = (csound->GetUtility(csound))->SndinGetSet(csound, p)) == NULL))
      return NULL;
    p->getframes = p->framesrem;
    dur = (MYFLT) p->getframes / p->sr;
    csound->Message(csound, "%s %" PRId64 " %s (%3.1f secs)\n",
                    Str("mixing"), p->getframes, Str("sample frames"), dur);
    ddd->fd = infd;
    return infd;
}

 static void MixSound(MIXER_GLOBALS *pp, int32_t n, SNDFILE *outfd,OPARMS *O)
{
    CSOUND *csound = pp->csound;
    inputs  *mixin = &(pp->mixin[0]);
    MYFLT   *buffer = (MYFLT*) csound->Calloc(csound, sizeof(MYFLT)
                                                      * 6 * NUMBER_OF_SAMPLES);
    MYFLT   *ibuffer = (MYFLT*) csound->Calloc(csound, sizeof(MYFLT)
                                                       * 6 * NUMBER_OF_SAMPLES);
    long    read_in;
    MYFLT   tpersample;
    MYFLT   max, min;
    long    lmaxpos, lminpos;
    int32_t     maxtimes, mintimes;
    long    sample = 0;
    int32_t     i, j, k;
    //    long    bytes = 0;
    int32_t     block = 0;
    int32_t     more_to_read = 1;
    int32_t     size;
    int32_t     this_block;
    int32_t     outputs = pp->outputs;

    tpersample = FL(1.0)/(MYFLT)mixin[0].p->sr;
    max = FL(0.0);  lmaxpos = 0; maxtimes = 0;
    min = FL(0.0);  lminpos = 0; mintimes = 0;
    while (more_to_read) {
      more_to_read = 0;
      size = NUMBER_OF_SAMPLES;
      for (i = 0; i < n; i++)
        if (mixin[i].start > sample && mixin[i].start - sample < size)
          size = (int32_t)(mixin[i].start - sample);
      /* for (j=0; j<size*outputs; j++) buffer[j] = FL(0.0); */
      memset(buffer, 0, sizeof(MYFLT)*size*outputs);
      this_block = 0;
      for (i = 0; i<n; i++) {
        if (sample >= mixin[i].start) {
          read_in = (csound->GetUtility(csound))->Sndin(csound, mixin[i].fd, ibuffer,
                                     size*mixin[i].p->nchanls, mixin[i].p);
          if (csound->Get0dBFS(csound)!=FL(1.0)) { /* Optimisation? */
            MYFLT xx = 1.0/csound->Get0dBFS(csound);
            for(j=0; j < read_in; j++)
              ibuffer[j] *= xx;
          }
          read_in /= mixin[i].p->nchanls;
          if (read_in > this_block) this_block = (int32_t) read_in;
          if (mixin[i].non_clear) {
            for (k = 1; k<=mixin[i].p->nchanls; k++)
              if (mixin[i].channels[k]) {
                for (j=0; j<read_in; j++) {
                  buffer[j*outputs+mixin[i].channels[k]-1] +=
                    ibuffer[j*outputs+k-1] *
                    gain(pp, i, (int32_t)(sample + j + mixin[i].channels[k] - 1));
                }
              }
            mixin[i].fulltable = mixin[i].table;
          }
          else {
            for (k = 1; k<=mixin[i].p->nchanls; k++) {
              for (j=0; j<read_in; j++) {
                buffer[j*outputs+k-1] +=
                  ibuffer[j*outputs + k - 1] * gain(pp, i, (int32_t)(sample + j + k - 1));
              }
            }
            mixin[i].fulltable = mixin[i].table;
          }
          if (read_in < size) {
            mixin[i].start = 0x7ffffff;
          }
          else more_to_read++;
        }
        else if (mixin[i].start > sample && mixin[i].start != 0x7ffffff)
          more_to_read++;
      }
      for (j = 0; j < this_block * outputs; j++) {
        if (UNLIKELY(buffer[j] > 1.0 || buffer[j] < -(1.0)))
          pp->outrange++;
        if (buffer[j] == max) maxtimes++;
        if (buffer[j] == min) mintimes++;
        if (buffer[j] > max) max = buffer[j], lmaxpos = sample+j, maxtimes=1;
        if (buffer[j] < min) min = buffer[j], lminpos = sample+j, mintimes=1;
      }
      csound->SndfileWriteSamples(csound, outfd, buffer, this_block * outputs);
      block++;
      //      bytes += O->sfsampsize * this_block * outputs;
      switch (O->heartbeat) {
      case 1:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
        break;
      case 2:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, ".");
        break;
      case 3:
        {
          int32_t n;
          csound->MessageS(csound, CSOUNDMSG_REALTIME, "%d%n", block, &n);
          while (n--) csound->MessageS(csound, CSOUNDMSG_REALTIME, "\b");
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "\007");
        break;
      }
      sample += size;
    }
    csound->RewriteHeader(csound, outfd);
    min *= (DFLT_DBFS);
    max *= (DFLT_DBFS);
    csound->Message(csound, Str("Max val %d at index %ld (time %.4f, chan %d) "
                                "%d times\n"),
                            (int32_t) max, lmaxpos, tpersample * (lmaxpos/outputs),
                            (int32_t) lmaxpos % outputs, maxtimes);
    csound->Message(csound, Str("Min val %d at index %ld (time %.4f, chan %d) "
                                "%d times\n"),
                            (int32_t) min, lminpos, tpersample * (lminpos/outputs),
                            (int32_t) lminpos % outputs, mintimes);
    if (UNLIKELY(pp->outrange))
      csound->Message(csound, Str("%d sample%s out of range\n"),
                              pp->outrange, (pp->outrange == 1 ? "" : "s"));
    else
      csound->Message(csound, Str("Max scale factor = %.3f\n"),
                              DFLT_DBFS / ((max > -min) ? max : -min));
}

/* module interface */

int32_t mixer_init_(CSOUND *csound)
{
    char    buf[128];
    int32_t     retval = (csound->GetUtility(csound))->AddUtility(csound, "mixer", mixer_main);

    snprintf(buf, 128, Str("Mixes sound files (max. %d)"),
             (int32_t) NUMBER_OF_FILES);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "mixer", buf);
    }
    return retval;
}
