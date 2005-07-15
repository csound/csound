/*
    mixer.c

    Copyright (C) 1995 John ffitch

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

#include "csoundCore.h"
#include "soundio.h"
#include <ctype.h>

/* Constants */

#define NUMBER_OF_SAMPLES (1024)

#define SHORTMAX 32767
#define NUMBER_OF_FILES (20)
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            csoundDie(csound, MSG);
typedef struct scalepoint {
    MYFLT y0;
    MYFLT y1;
    MYFLT yr;
    int x0;
    int x1;
    struct scalepoint *next;
} scalepoint;

typedef struct inputs {
    long        start;          /* Time this file starts in samples */
    MYFLT       time;           /* Time this file starts in secs */
    char *      name;           /* Name of file */
    int         use_table;      /* Should we use multiplier or table */
    MYFLT       factor;         /* Gain factor */
    char *      fname;          /* Name of scale table file */
    scalepoint *fulltable;      /* Scaling table */
    scalepoint *table;          /* current position in table */
    SNDFILE    *fd;             /* File descriptor handle */
    short       channels[5];    /* destinations of channels */
    int         non_clear;      /* Boolean to say if fiddled mixing */
    SOUNDIN *   p;              /* Csound structure */
} inputs;

static inputs mixin[NUMBER_OF_FILES];
static int outputs = 0;
static int debug   = 0;

/* Static function prototypes */

static void InitScaleTable(ENVIRON*,int);
static MYFLT gain(ENVIRON*, int, int);
static SNDFILE *MXsndgetset(ENVIRON*,inputs *);
static void MixSound(int, SNDFILE*);

/* Externs */
extern int  openout(char *, int), getsizformat(int);
extern char *getstrformat(int);
extern short sfsampsize(int);

/* Static global variables */
static  unsigned  outbufsiz;
static  MYFLT     *out_buf;
static  int       outrange = 0;             /* Count samples out of range */
static  OPARMS    OO;

static void usage(ENVIRON *csound, char *mesg)
{
    csound->Message(csound,  "%s\n", mesg);
    csound->Message(csound,
                    Str("Usage:\tmixer [-flags] soundfile [-flags] soundfile ...\n"));
    csound->Message(csound, Str("Legal flags are:\n"));
    csound->Message(csound, Str("-o fnam\tsound output filename\n"));
    csound->Message(csound, Str("-A\tcreate an AIFF format output soundfile\n"));
    csound->Message(csound, Str("-W\tcreate a WAV format output soundfile\n"));
    csound->Message(csound, Str("-h\tno header on output soundfile\n"));
    csound->Message(csound, Str("-8\t8-bit unsigned_char sound samples\n"));
    csound->Message(csound, Str("-c\t8-bit signed_char sound samples\n"));
    csound->Message(csound, Str("-8\t8-bit unsigned_char sound samples\n"));
    csound->Message(csound, Str("-a\talaw sound samples\n"));
    csound->Message(csound, Str("-u\tulaw sound samples\n"));
    csound->Message(csound, Str("-s\tshort_int sound samples\n"));
    csound->Message(csound, Str("-l\tlong_int sound samples\n"));
    csound->Message(csound, Str("-f\tfloat sound samples\n"));
    csound->Message(csound, Str("-R\tcontinually rewrite header while writing "
                                "soundfile (WAV/AIFF)\n"));
    csound->Message(csound, Str("-H#\tprint a heartbeat style 1, 2 or 3 at "
                                "each soundfile write\n"));
    csound->Message(csound, Str("-N\tnotify (ring the bell) when score or "
                                "miditrack is done\n"));
    csound->Message(csound, Str("-F fpnum\tamount to scale amplitude for "
                                "next input\n"));
    csound->Message(csound, Str("-F fname\tfile of a scale table for next input\n"));
    csound->Message(csound, Str("-S integer\tsample number at which to "
                                "insert file\n"));
    csound->Message(csound, Str("-T fpnum\ttime at which to insert file\n"));
    csound->Message(csound, Str("-1 -2 -3 -4\tinclude named channel\n"));
    csound->Message(csound, Str("-^ n m\tinclude channel n and output "
                                "as channel m\n"));
    csound->Message(csound, Str("-v\tverbose mode for debugging\n"));
    csound->Message(csound, Str("-- fname\tLog output to file\n"));
    csound->Message(csound, Str("flag defaults: mixer -s -otest -F 1.0 -S 0\n"));
    exit(1);
}

static char set_output_format(ENVIRON *csound, char c, char outformch)
{
    if (OO.outformat && (O.msglevel & WARNMSG)) {
      csound->Message(csound,
                      Str("WARNING: Sound format -%c has been overruled by -%c\n"),
                      outformch, c);
    }

    switch (c) {
    case 'a':
      OO.outformat = AE_ALAW;    /* a-law soundfile */
      break;

    case 'c':
      OO.outformat = AE_CHAR;    /* signed 8-bit soundfile */
      break;

    case '8':
      OO.outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
      break;

    case 'f':
      OO.outformat = AE_FLOAT;   /* float soundfile */
      break;

    case 's':
      OO.outformat = AE_SHORT;   /* short_int soundfile*/
      break;

    case 'l':
      OO.outformat = AE_LONG;    /* long_int soundfile */
      break;

    case 'u':
      OO.outformat = AE_ULAW;    /* mu-law soundfile */
      break;

    case '3':
      OO.outformat = AE_24INT;   /* 24bit packed soundfile*/
      break;

    case 'e':
      OO.outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
      break;

    default:
      return outformch; /* do nothing */
    };

  return c;
}

int main(int argc, char **argv)
{
    ENVIRON     *csound = &cenviron;
    char        *inputfile = NULL;
    SNDFILE     *infd, *outfd;
    int         i;
    char        outformch='s', c, *s, *filnamp;
    char        *envoutyp;
    int         n = 0;
    SF_INFO     sfinfo;

    init_getstring(argc, argv);
    csoundPreCompile(csoundCreate(NULL));
/*     response_expand(&argc, &argv); /\* Permits "@xxx" response files *\/ */
    memset(&OO, 0, sizeof(OO));
    /* Check arguments */
    {
      if ((envoutyp = csoundGetEnv(csound, "SFOUTYP")) != NULL) {
        if (strcmp(envoutyp,"AIFF") == 0)
          OO.filetyp = TYP_AIFF;
        else if (strcmp(envoutyp,"WAV") == 0)
          OO.filetyp = TYP_WAV;
        else if (strcmp(envoutyp,"IRCAM") == 0)
          OO.filetyp = TYP_IRCAM;
        else {
          csound->Message(csound, Str("%s not a recognized SFOUTYP env setting"),
                     envoutyp);
          exit(1);
        }
      }
    }
    O.filnamspace = filnamp = mmalloc(csound, (long)1024);
    mixin[n].start = -1; mixin[n].time = -FL(1.0);
    mixin[n].factor = FL(1.0); mixin[n].non_clear = 0;
    mixin[n].fulltable = NULL; mixin[n].use_table = 0;
    for (i=1; i<5; i++) mixin[n].channels[i] = 0;
    if (!(--argc))
      usage(csound,Str("Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'j':
            FIND("")
              while (*++s);
            break;
          case 'o':
            FIND(Str("no outfilename"))
              O.outfilename = filnamp;            /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O.outfilename,"stdin") == 0)
              csoundDie(csound, Str("-o cannot be stdin"));
            if (strcmp(O.outfilename,"stdout") == 0) {
#if defined mac_classic || defined SYMANTEC || defined BCC || defined __WATCOMC__ || defined WIN32
              csoundDie(csound, Str("stdout audio not supported"));
#else
              if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                csoundDie(csound, Str("too many open files"));
              dup2(2,1);                /* & send 1's to stderr */
#endif
            }
            break;
          case 'A':
            if (OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                csound->Message(csound,Str("-A overriding local default WAV out"));
            }
            OO.filetyp = TYP_AIFF;     /* AIFF output request  */
            break;
          case 'J':
            if (OO.filetyp == TYP_AIFF ||
                OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                csound->Message(csound,
                                Str("WARNING: -J overriding local default "
                                    "AIFF/WAV out\n"));
            }
            OO.filetyp = TYP_IRCAM;      /* IRCAM output request */
            break;
          case 'W':
            if (OO.filetyp == TYP_AIFF) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                csound->Message(csound,Str("-W overriding local default AIFF out"));
            }
            OO.filetyp = TYP_WAV;      /* WAV output request  */
            break;
          case 'F':
            FIND(Str("no scale factor"));
            if (isdigit(*s) || *s == '-' || *s == '+')
              mixin[n].factor = (MYFLT) atof(s);
            else {
              mixin[n].fname = (char *)malloc(strlen(s)+1);
              strcpy(mixin[n].fname, s);
              mixin[n].use_table = 1;
            }
            while (*++s);
            break;
          case 'S':
            FIND(Str("no start sample"));
            mixin[n].start = atoi(s);
            while (*++s);
            if (mixin[n].time >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                csound->Message(csound,Str("-S overriding -T"));
              mixin[n].time = -FL(1.0);
            }
            break;
          case 'T':
            FIND(Str("no start time"));
            mixin[n].time = (MYFLT) atof(s);
            while (*++s);
            if (mixin[n].start >= 0) {
              if (O.msglevel & WARNMSG)
                csound->Message(csound,Str("-T overriding -S"));
              mixin[n].start = -1;
            }
            break;
          case '1':
          case '2':
          case '3':
          case '4':
            {
              int src = c - '0';
              if (src>outputs) outputs = src;
              mixin[n].channels[src] = src;
              mixin[n].non_clear = 1;
              break;
            }
          case '^':
            {
              int src = c, dst;
              FIND(Str("no source channel number"));
              src = atoi(s);
              while (*++s);
              FIND(Str("no destination channel number"));
              dst = atoi(s);
              while (*++s);
              if (src>4 || src<1 || dst>4 || dst<1) {
                if (O.msglevel & WARNMSG)
                  csound->Message(csound,Str("illegal channel number ignored"));
                break;
              }
              if (dst>outputs) outputs = dst;
              mixin[n].channels[dst] = src;
              mixin[n].non_clear = 1;
              break;
            }
          case 'h':
            OO.sfheader = 0;           /* skip sfheader  */
            break;
          case 'c':
          case 'a':
          case 'u':
          case '8':
          case 's':
          case 'l':
          case 'f':
            outformch = set_output_format(csound, c, outformch);
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
          case 'N':
            O.ringbell = 1;             /* notify on completion */
            break;
          case 'v':                       /* Verbose mode */
            debug = 1;
            break;
          default:
            sprintf(csound->errmsg, Str("unknown flag -%c"), c);
            usage(csound, csound->errmsg);
          }
      else {
        int i;
        mixin[n].name = --s;
        if (!mixin[n].non_clear)
          for (i=1; i<5; i++) mixin[n].channels[i] = i;
        if (n++ >= NUMBER_OF_FILES) {
          usage(csound,Str("Too many mixin"));
        }
        mixin[n].start = -1;
        mixin[n].time = -1;
        mixin[n].factor = FL(1.0);
        mixin[n].non_clear = 0;
      }
    } while (--argc);

    dbfs_init(csound, DFLT_DBFS);
    /* Read sound files */
    if (n==0) {
      if (O.msglevel & WARNMSG)
        csound->Message(csound,"No mixin");
      exit(1);
    }
    for (i=0; i<n; i++) {
      if (!(infd = MXsndgetset(csound, &mixin[i]))) {
        csound->Message(csound, Str("%s: error while opening %s"),
                   argv[0], inputfile);
        exit(1);
      }
      mixin[i].p->channel = ALLCHNLS;
      if (i>0) {
        if (mixin[0].p->sr != mixin[i].p->sr) {
          if (O.msglevel & WARNMSG)
            csound->Message(csound,Str("Input formats not the same"));
          exit(1);
        }
      }
      if (mixin[i].non_clear) {
        int j;
        for (j = 1; j<5; j++)
          if (outputs < mixin[i].channels[j]) {
            outputs = mixin[i].channels[j];
          }
      }
      else if (outputs < mixin[i].p->nchanls) outputs = mixin[i].p->nchanls;
      if (mixin[i].time >= 0.0)
        mixin[i].start = (long)mixin[i].time*mixin[i].p->sr;
      else if (mixin[i].start < 0) mixin[i].start = 0;
      else mixin[i].start = mixin[0].p->nchanls*mixin[i].start;
      if (mixin[i].use_table) InitScaleTable(csound,i);
    }

    if (OO.outformat)                       /* if no audioformat yet  */
      O.outformat = OO.outformat;
    else O.outformat = mixin[0].p->format; /* Copy from first input file */
    O.sfsampsize = sfsampsize(O.outformat);
    if (OO.filetyp)
      O.filetyp = OO.filetyp;
    else O.filetyp = mixin[0].p->filetyp; /* Copy from input file */
    if (OO.sfheader)
      O.sfheader = OO.sfheader;
    else O.sfheader = 1;
    if (OO.filetyp) O.filetyp = OO.filetyp;
    if (O.filetyp == TYP_AIFF) {
      if (!O.sfheader)
        csoundDie(csound, Str("can't write AIFF soundfile with no header"));
      if (
          O.outformat == AE_ALAW ||
          O.outformat == AE_ULAW ||
          O.outformat == AE_FLOAT) {
        sprintf(csound->errmsg, Str("AIFF does not support %s encoding"),
                                getstrformat(O.outformat));
        csoundDie(csound, csound->errmsg);
      }
    }
    if (O.filetyp == TYP_WAV) {
      if (!O.sfheader)
        csoundDie(csound, Str("can't write WAV soundfile with no header"));
      if (
          O.outformat == AE_ALAW ||
          O.outformat == AE_ULAW ||
          O.outformat == AE_FLOAT) {
        sprintf(csound->errmsg, Str("WAV does not support %s encoding"),
                                getstrformat(O.outformat));
        csoundDie(csound, csound->errmsg);
      }
    }
    if (O.rewrt_hdr && !O.sfheader)
      csoundDie(csound, Str("can't rewrite header if no header requested"));
#ifdef NeXT
    if (O.outfilename == NULL && !O.filetyp) O.outfilename = "test.snd";
    else if (O.outfilename == NULL) O.outfilename = "test";
#else
    if (O.outfilename == NULL) {
      if (O.filetyp == TYP_WAV) O.outfilename = "test.wav";
      else if (O.filetyp == TYP_AIFF) O.outfilename = "test.aif";
      else O.outfilename = "test";
    }
#endif
    sfinfo.frames = -1;
    sfinfo.samplerate = (int)(csound->esr = mixin[0].p->sr);
    sfinfo.channels = csound->nchnls = mixin[0].p->nchanls ;
    sfinfo.format = TYPE2SF(O.filetyp) | FORMAT2SF(O.outformat);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfd = sf_open_fd(openout(O.outfilename, 1), SFM_WRITE, &sfinfo, 1);
    if (O.rewrt_hdr) sf_command(outfd, SFC_SET_UPDATE_HEADER_AUTO, NULL, 0);
    outbufsiz = NUMBER_OF_SAMPLES * outputs * O.sfsampsize;/* calc outbuf size */
    out_buf = mmalloc(csound, (long)outbufsiz);       /*  & alloc bufspace */
    csound->Message(csound,Str("writing %d-byte blks of %s to %s %s\n"),
                    outbufsiz, getstrformat(O.outformat), O.outfilename,
                    O.filetyp == TYP_AIFF ? "(AIFF)" :
                    O.filetyp == TYP_WAV ? "(WAV)" : "");
    MixSound(n, outfd);
    sf_close(outfd);
    if (O.ringbell) putc(7, stderr);
    return 0;

 outtyp:
    usage(csound,Str("output soundfile cannot be both AIFF and WAV"));
    return 0;
}

static void
InitScaleTable(ENVIRON *csound, int i)
{
    FILE *f = fopen(mixin[i].fname, "r");
    MYFLT samplepert = (MYFLT)mixin[i].p->sr;
    MYFLT x, y;
    scalepoint *tt = (scalepoint*)malloc(sizeof(scalepoint));
    if (f == NULL) {
      csound->Message(csound, Str("Cannot open scale table file %s\n"),
                 mixin[i].fname);
      exit(1);
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
      newpoint = (scalepoint*) malloc(sizeof(scalepoint));
      if (newpoint == NULL) {
        csound->Message(csound, Str("Insufficient memory\n"));
        exit(1);
      }
      newpoint->x0 = tt->x1;
      newpoint->y0 = tt->y1;
      newpoint->x1 = (int) (x*samplepert);
      newpoint->y1 = y;
      if (newpoint->x1 == newpoint->x0) {
        tt->y1 = y;
        tt->yr = (y - tt->y0)/((MYFLT)(tt->x1 - tt->x0));
        free(newpoint);
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
      scalepoint *newpoint = (scalepoint*) malloc(sizeof(scalepoint));
      if (newpoint == NULL) {
        csound->Message(csound,  "Insufficient memory\n");
        exit(1);
      }
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
    if (debug) {
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

static MYFLT
gain(ENVIRON *csound, int n, int i)
{
    if (!mixin[n].use_table) return mixin[n].factor;
    if (i<mixin[n].table->x0) mixin[n].table = mixin[n].fulltable;
    while (i<mixin[n].table->x0 ||
           i>=mixin[n].table->x1) {/* Get correct segment */
      if (debug)
        csound->Message(csound, "Table %d: %d (%d %f) -> %d %f [%f]\n",
                        n, i, mixin[n].table->x0, mixin[n].table->y0,
                        mixin[n].table->x1, mixin[n].table->y1,
                        mixin[n].table->yr);
      mixin[n].table = mixin[n].table->next;
    }
    return mixin[n].factor*(mixin[n].table->y0 +
                            mixin[n].table->yr*(MYFLT)(i - mixin[n].table->x0));
}

static SNDFILE*
MXsndgetset(ENVIRON *csound, inputs *ddd)
{
    SNDFILE *infd;
    MYFLT   dur;
    SOUNDIN *p;

    csoundInitEnv(csound);      /* stand-alone init of SFDIR etc. */
    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    ddd->p = p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strcpy(p->sfname, ddd->name);
    if ((infd = csound->sndgetset(csound, p)) == 0) /*open sndfil, do skiptime*/
      return(0);
    p->getframes = p->framesrem;
    dur = (MYFLT) p->getframes / p->sr;
    csound->Message(csound,Str("mixing %ld sample frames (%3.1f secs)\n"),
                    (long) p->getframes, dur);
    ddd->fd = infd;
    return(infd);
}

static void
MixSound(int n, SNDFILE *outfd)
{
    ENVIRON *csound = &cenviron;
    MYFLT buffer[4 * NUMBER_OF_SAMPLES];
    MYFLT ibuffer[4 * NUMBER_OF_SAMPLES];
    long  read_in;
    MYFLT tpersample;
    MYFLT max, min;
    long  lmaxpos, lminpos;
    int   maxtimes, mintimes;
    long  sample = 0;
    int   i, j, k;
    long  bytes = 0;
    int   block = 0;
    int   more_to_read = 1;
    int   size;
    int   this_block;

    tpersample = FL(1.0)/(MYFLT)mixin[0].p->sr;
    max = FL(0.0);  lmaxpos = 0; maxtimes = 0;
    min = FL(0.0);  lminpos = 0; mintimes = 0;
    while (more_to_read) {
      more_to_read = 0;
      size = NUMBER_OF_SAMPLES;
      for (i = 0; i<n; i++)
        if (mixin[i].start>sample && mixin[i].start - sample < size)
          size = mixin[i].start - sample;
      for (j=0; j<size*outputs; j++) buffer[j] = FL(0.0);
      this_block = 0;
      for (i = 0; i<n; i++) {
        if (sample >= mixin[i].start) {
          read_in = csound->getsndin(csound, mixin[i].fd, ibuffer,
                                     size*mixin[i].p->nchanls, mixin[i].p);
          read_in /= mixin[i].p->nchanls;
          if (read_in > this_block) this_block = read_in;
          if (mixin[i].non_clear) {
            for (k = 1; k<=mixin[i].p->nchanls; k++)
              if (mixin[i].channels[k]) {
                for (j=0; j<read_in; j++) {
                  buffer[j*outputs+mixin[i].channels[k]-1] +=
                    ibuffer[j*outputs+k-1] *
                    gain(csound,i,sample+j+mixin[i].channels[k]-1);
                }
              }
            mixin[i].fulltable = mixin[i].table;
          }
          else {
            for (k = 1; k<=mixin[i].p->nchanls; k++) {
              for (j=0; j<read_in; j++) {
                buffer[j*outputs+k-1] +=
                  ibuffer[j*outputs+k-1] * gain(csound,i,sample+j+k-1) ;
              }
            }
            mixin[i].fulltable = mixin[i].table;
          }
          if (read_in < size) {
            mixin[i].start = 0x7ffffff;
            sf_close(mixin[i].fd);
          }
          else more_to_read++;
        }
        else if (mixin[i].start > sample && mixin[i].start != 0x7ffffff)
          more_to_read++;
      }
      for (j=0; j<this_block*outputs; j++) {
        if (buffer[j] == max) maxtimes++;
        if (buffer[j] == min) mintimes++;
        if (buffer[j] > max) max = buffer[j], lmaxpos = sample+j, maxtimes=1;
        if (buffer[j] < min) min = buffer[j], lminpos = sample+j, mintimes=1;
      }
      sf_write_MYFLT(outfd, out_buf,
                     O.sfsampsize * this_block * outputs / csound->nchnls);
      block++;
      bytes += O.sfsampsize*this_block*outputs;
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
          csound->Message(csound, "%d%n", block, &n);
          while (n--) putc(8, stderr);
        }
        else putc(7, stderr);
      }
      sample += size;

    }
    rewriteheader(outfd, bytes);
    csound->Message(csound,
                    Str("Max val %d at index %ld (time %.4f, chan %d) %d times\n"),
           (int)max,lmaxpos,tpersample*(lmaxpos/outputs),
           (int)lmaxpos%outputs,maxtimes);
    csound->Message(csound,
                    Str("Min val %d at index %ld (time %.4f, chan %d) %d times\n"),
                    (int)min,lminpos,tpersample*(lminpos/outputs),
                    (int)lminpos%outputs,mintimes);
    if (outrange)
      csound->Message(csound,Str("%d sample%s out of range\n"),
                      outrange, outrange==1 ? " " : "s ");
    else
      csound->Message(csound,Str("Max scale factor = %.3f\n"),
                      (MYFLT)SHORTMAX/(MYFLT)((max>-min)?max:-min) );
    return;
}

