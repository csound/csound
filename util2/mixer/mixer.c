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


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "cs.h"
#include "ustub.h"
#include "soundio.h"

/* Constants */

#define NUMBER_OF_SAMPLES (1024)

#define SHORTMAX 32767
#define NUMBER_OF_FILES (20)
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            die(MSG);
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
    int         fd;             /* File descriptor handle */
    short       channels[5];    /* destinations of channels */
    int         non_clear;      /* Boolean to say if fiddled mixing */
    SOUNDIN *   p;              /* Csound structure */
} inputs;

inputs mixin[NUMBER_OF_FILES];
int outputs = 0;
int debug   = 0;

/* Static function prototypes */

static void InitScaleTable(int);
static MYFLT gain(int, int);
static int  MXsndgetset(inputs *);
static void MixSound(int, int);
static void (*audtran)(char *, int);
static void (*spoutran)(MYFLT *);

/* Externs */
extern long getsndin(int, MYFLT *, long, SOUNDIN *);
extern void bytrev2(char *, int), bytrev4(char *, int);
extern int  openout(char *, int), bytrevhost(void), getsizformat(int);
extern void writeheader(int, char *);
extern char *getstrformat(int);
extern int  sndgetset(SOUNDIN *);
extern short sfsampsize(int);

/* Static global variables */
static unsigned    outbufsiz;
static MYFLT        *outbuf; 
static  int        outrange = 0;            /* Count samples out of range */

void *memfiles = NULL;
void rlsmemfiles(void)
{
}

static void usage(char *mesg)
{
    err_printf( "%s\n", mesg);
    err_printf(Str(X_339,"Usage:\tmixer [-flags] soundfile [-flags] soundfile ...\n"));
    err_printf(Str(X_9,"Legal flags are:\n"));
    err_printf(Str(X_157,"-o fnam\tsound output filename\n"));
    err_printf(Str(X_96,"-A\tcreate an AIFF format output soundfile\n"));
    err_printf(Str(X_132,"-W\tcreate a WAV format output soundfile\n"));
    err_printf(Str(X_149,"-h\tno header on output soundfile\n"));
    err_printf(Str(X_94,"-8\t8-bit unsigned_char sound samples\n")); 
    err_printf(Str(X_141,"-c\t8-bit signed_char sound samples\n"));
    err_printf(Str(X_94,"-8\t8-bit unsigned_char sound samples\n"));
#ifdef never
    err_printf(Str(X_136,"-a\talaw sound samples\n"));
#endif
#ifdef ULAW
    err_printf(Str(X_166,"-u\tulaw sound samples\n"));
#endif
    err_printf(Str(X_164,"-s\tshort_int sound samples\n"));
    err_printf(Str(X_153,"-l\tlong_int sound samples\n"));
    err_printf(Str(X_145,"-f\tfloat sound samples\n"));
    err_printf(Str(X_125,"-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
    err_printf(Str(X_108,"-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
    err_printf(Str(X_120,"-N\tnotify (ring the bell) when score or miditrack is done\n"));
    err_printf(Str(X_511,"-F fpnum\tamount to scale amplitude for next input\n"));
    err_printf(Str(X_547,"-F fname\tfile of a scale table for next input\n"));
    err_printf(Str(X_558,"-S integer\tsample number at which to insert file\n"));
    err_printf(Str(X_559,"-T fpnum\ttime at which to insert file\n"));
    err_printf(Str(X_569,"-1 -2 -3 -4\tinclude named channel\n"));
    err_printf(Str(X_596,"-^ n m\tinclude channel n and output as channel m\n"));
    err_printf(Str(X_780,"-v\tverbose mode for debugging\n"));
    err_printf(Str(X_91,"-- fname\tLog output to file\n"));
    err_printf(Str(X_930,"flag defaults: mixer -s -otest -F 1.0 -S 0\n"));
    exit(1);
}

#ifndef POLL_EVENTS
int POLL_EVENTS(void)
{
    return (1);
}
#endif

void pvsys_release(void) {};

int
main(int argc, char **argv)
{
    char        *inputfile = NULL;
    int         infd, outfd;
    int         i;
    char        outformch='s', c, *s, *filnamp;
    char        *envoutyp;
    OPARMS      OO;
    int         n = 0;

    init_getstring(argc, argv);
/*     response_expand(&argc, &argv); /\* Permits "@xxx" response files *\/ */
    memset(&OO, 0, sizeof(OO));
    /* Check arguments */
    {
      char *getenv();
      if ((envoutyp = getenv("SFOUTYP")) != NULL) {
        if (strcmp(envoutyp,"AIFF") == 0)
          OO.filetyp = TYP_AIFF;
        else if (strcmp(envoutyp,"WAV") == 0)
          OO.filetyp = TYP_WAV;
        else {
          err_printf(Str(X_61,"%s not a recognized SFOUTYP env setting"),
                     envoutyp);
          exit(1);
        }
      }
    }
    O.filnamspace = filnamp = mmalloc((long)1024);
    mixin[n].start = -1; mixin[n].time = -FL(1.0);
    mixin[n].factor = FL(1.0); mixin[n].non_clear = 0;
    mixin[n].fulltable = NULL; mixin[n].use_table = 0;
    for (i=1; i<5; i++) mixin[n].channels[i] = 0;
    if (!(--argc))
      usage(Str(X_939,"Insufficient arguments"));
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
            FIND(Str(X_1052,"no outfilename"))
              O.outfilename = filnamp;            /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O.outfilename,"stdin") == 0)
              die(Str(X_156,"-o cannot be stdin"));
            if (strcmp(O.outfilename,"stdout") == 0) {
#ifdef THINK_C
              die(Str(X_1244,"stdout audio not supported"));
#else
              if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                die(Str(X_1290,"too many open files"));
              dup2(2,1);                /* & send 1's to stderr */
#endif
            }
            break;
          case 'A':
            if (OO.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str(X_95,"-A overriding local default WAV out"));
            }
            OO.filetyp = TYP_AIFF;     /* AIFF output request  */
            break;
          case 'W':
            if (OO.filetyp == TYP_AIFF) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str(X_131,"-W overriding local default AIFF out"));
            }
            OO.filetyp = TYP_WAV;      /* WAV output request  */
            break;
          case 'F':
            FIND(Str(X_933,"no scale factor"));
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
            FIND(Str(X_1417,"no start sample"));
            mixin[n].start = atoi(s);
            while (*++s);
            if (mixin[n].time >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf(Str(X_1418,"-S overriding -T"));
              mixin[n].time = -FL(1.0);
            }
            break;
          case 'T':
            FIND(Str(X_1419,"no start time"));
            mixin[n].time = (MYFLT) atof(s);
            while (*++s);
            if (mixin[n].start >= 0) {
              if (O.msglevel & WARNMSG)
                printf(Str(X_1420,"-T overriding -S"));
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
              FIND(Str(X_1421,"no source channel number"));
              src = atoi(s);
              while (*++s);
              FIND(Str(X_1422,"no destination channel number"));
              dst = atoi(s);
              while (*++s);
              if (src>4 || src<1 || dst>4 || dst<1) {
                if (O.msglevel & WARNMSG)
                  printf(Str(X_1423,"illegal channel number ignored"));
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
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_CHAR;     /* 8-bit char soundfile */
            break;
          case '8':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_UNCH;     /* 8-bit unsigned char file */
            break;
#ifdef never
          case 'a':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_ALAW;     /* a-law soundfile */
            break;
#endif
#ifdef ULAW
          case 'u':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_ULAW;     /* mu-law soundfile */
            break;
#endif
          case 's':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_SHORT;    /* short_int soundfile */
            break;
          case 'l':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_LONG;     /* long_int soundfile */
            break;
          case 'f':
            if (OO.outformat) goto outform;
            outformch = c;
            OO.outformat = AE_FLOAT;    /* float soundfile */
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
            sprintf(errmsg,Str(X_1334,"unknown flag -%c"), c);
            usage(errmsg);
          }
      else {
        int i;
        mixin[n].name = --s;
        if (!mixin[n].non_clear)
          for (i=1; i<5; i++) mixin[n].channels[i] = i;
        if (n++ >= NUMBER_OF_FILES) {
          usage(Str(X_1425,"Too many mixin"));
        }
        mixin[n].start = -1;
        mixin[n].time = -1;
        mixin[n].factor = FL(1.0);
        mixin[n].non_clear = 0;
      }
    } while (--argc);

#ifdef RWD_DBFS
    dbfs_init(DFLT_DBFS);
#endif
    /* Read sound files */
    if (n==0) {
      if (O.msglevel & WARNMSG)
        printf("No mixin");
      exit(1);
    }
    for (i=0; i<n; i++) {
      if (!(infd = MXsndgetset(&mixin[i]))) {
        err_printf(Str(X_76,"%s: error while opening %s"),
                   argv[0], inputfile);
        exit(1);
      }
      mixin[i].p->channel = ALLCHNLS;
      if (i>0) {
        if (mixin[0].p->sr != mixin[i].p->sr) {
          if (O.msglevel & WARNMSG)
            printf(Str(X_920,"Input formats not the same"));
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
      if (mixin[i].use_table) InitScaleTable(i);
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
        die(Str(X_640,"can't write AIFF soundfile with no header"));
      if (
#ifdef never
          O.outformat == AE_ALAW ||
#endif
#ifdef ULAW
          O.outformat == AE_ULAW ||
#endif
          O.outformat == AE_FLOAT) {
        sprintf(errmsg,Str(X_180,"AIFF does not support %s encoding"),
                getstrformat(O.outformat));
        die(errmsg);
      }
    }
    if (O.filetyp == TYP_WAV) {
      if (!O.sfheader)
        die(Str(X_338,"can't write WAV soundfile with no header"));
      if (
#ifdef never
          O.outformat == AE_ALAW ||
#endif
#ifdef ULAW
          O.outformat == AE_ULAW ||
#endif
          O.outformat == AE_FLOAT) {
        sprintf(errmsg,Str(X_181,"WAV does not support %s encoding"),
                getstrformat(O.outformat));
        die(errmsg);
      }
    }
    if (O.rewrt_hdr && !O.sfheader)
      die(Str(X_628,"can't rewrite header if no header requested"));
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
    outfd = openout(O.outfilename, 1);
    esr = (MYFLT)mixin[0].p->sr;
    nchnls = outputs;
    outbufsiz = NUMBER_OF_SAMPLES * outputs * O.sfsampsize;/* calc outbuf size */
    outbuf = mmalloc((long)outbufsiz);                 /*  & alloc bufspace */
    printf(Str(X_1382,"writing %d-byte blks of %s to %s %s\n"),
           outbufsiz, getstrformat(O.outformat), O.outfilename,
           O.filetyp == TYP_AIFF ? "(AIFF)" :
           O.filetyp == TYP_WAV ? "(WAV)" : "");
    MixSound(n, outfd);
    close(outfd);
    if (O.ringbell) putc(7, stderr);
    return 0;

 outtyp:
    usage(Str(X_1113,"output soundfile cannot be both AIFF and WAV"));
    return 0;
 outform:
    sprintf(errmsg,Str(X_1198,"sound output format cannot be both -%c and -%c"),
            outformch, c);
    usage(errmsg);
    return 0;
}

static void 
InitScaleTable(int i)
{
    FILE *f = fopen(mixin[i].fname, "r");
    MYFLT samplepert = (MYFLT)mixin[i].p->sr;
    MYFLT x, y;
    scalepoint *tt = (scalepoint*)malloc(sizeof(scalepoint));
    if (f == NULL) {
      err_printf(Str(X_984,"Cannot open scale table file %s\n"),
                 mixin[i].fname);
      exit(1);
    }
    mixin[i].fulltable = mixin[i].table = tt;
    tt->x0 = 0; tt->y0 = FL(0.0); tt->x1 = 0; tt->y1 = FL(0.0);
    tt->yr = FL(0.0); tt->next = NULL;
    while (fscanf(f, "%f %f\n", &x, &y) == 2) {
      scalepoint *newpoint;
      newpoint = (scalepoint*) malloc(sizeof(scalepoint));
      if (newpoint == NULL) {
        err_printf(Str(X_945,"Insufficient memory\n"));
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
        err_printf( "Insufficient memory\n");
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
      err_printf( "Scale table is\n");
      while (tt != NULL) {
        err_printf( "(%d %f) -> %d %f [%f]\n",
                    tt->x0, tt->y0, tt->x1, tt->y1, tt->yr);
        tt = tt->next;
      }
      err_printf( "END of Table\n");
    }
    mixin[i].use_table = 1;
}

static MYFLT 
gain(int n, int i)
{
    if (!mixin[n].use_table) return mixin[n].factor;
    if (i<mixin[n].table->x0) mixin[n].table = mixin[n].fulltable;
    while (i<mixin[n].table->x0 ||
           i>=mixin[n].table->x1) {/* Get correct segment */
      if (debug)
        err_printf( "Table %d: %d (%d %f) -> %d %f [%f]\n",
                    n, i, mixin[n].table->x0, mixin[n].table->y0,
                    mixin[n].table->x1, mixin[n].table->y1,
                    mixin[n].table->yr);
      mixin[n].table = mixin[n].table->next;
    }
    return mixin[n].factor*(mixin[n].table->y0 +
                            mixin[n].table->yr*(MYFLT)(i - mixin[n].table->x0));
}

static int
MXsndgetset(inputs *ddd)
{
    int          infd;
    MYFLT        dur;
    static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
    static  OPTXT    optxt;
    static  MYFLT    fzero = FL(0.0);
    char         quotname[80];
    SOUNDIN *    p;
    static MYFLT sstrcod = (MYFLT)SSTRCOD;

    sssfinit();                 /* stand-alone init of SFDIR etc. */
    esr = FL(0.0);              /* set esr 0. with no orchestra   */
    optxt.t.outoffs = &argoffs; /* point to dummy OUTOCOUNT       */
    ddd->p = p = (SOUNDIN *) mcalloc((long)sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->h.optext = &optxt;
    p->ifilno = &sstrcod;
    p->iskptim = &fzero;
    p->iformat = &fzero;
    sprintf(quotname,"%c%s%c",'"',ddd->name,'"');
    p->STRARG = quotname;
    if ((infd = sndgetset(p)) == 0)            /* open sndfil, do skiptime */
      return(0);
    p->getframes = p->framesrem;
    dur = (MYFLT) p->getframes / p->sr;
    printf(Str(X_983,"mixing %ld sample frames (%3.1f secs)\n"),
           p->getframes, dur);
    ddd->fd = infd;
    return(infd);
}

static void 
MixSound(int n, int outfd)
{
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
          read_in = getsndin(mixin[i].fd, ibuffer,
                             size*mixin[i].p->nchanls, mixin[i].p);
          read_in /= mixin[i].p->nchanls;
          if (read_in > this_block) this_block = read_in;
          if (mixin[i].non_clear) {
            for (k = 1; k<=mixin[i].p->nchanls; k++) 
              if (mixin[i].channels[k]) {
                for (j=0; j<read_in; j++) {
                  buffer[j*outputs+mixin[i].channels[k]-1] +=
                    ibuffer[j*outputs+k-1] *
                    gain(i,sample+j+mixin[i].channels[k]-1);
                }
              }
            mixin[i].fulltable = mixin[i].table;
          }
          else {
            for (k = 1; k<=mixin[i].p->nchanls; k++) {
              for (j=0; j<read_in; j++) {
                buffer[j*outputs+k-1] +=
                  ibuffer[j*outputs+k-1] * gain(i,sample+j+k-1) ;
              }
            }
            mixin[i].fulltable = mixin[i].table;
          }
          if (read_in < size) {
            mixin[i].start = 0x7ffffff;
            close(mixin[i].fd);
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
      spoutran(buffer);
      audtran(outbuf, O.sfsampsize*this_block*outputs);
      write(outfd, outbuf, O.sfsampsize*this_block*outputs);
      block++;
      bytes += O.sfsampsize*this_block*outputs;
      if (O.rewrt_hdr) {
        rewriteheader(outfd, bytes);
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
      sample += size;

    }
    rewriteheader(outfd, bytes);
    printf(Str(X_336,"Max val %d at index %ld (time %.4f, chan %d) %d times\n"),
           (int)max,lmaxpos,tpersample*(lmaxpos/outputs),
           (int)lmaxpos%outputs,maxtimes);
    printf(Str(X_192,"Min val %d at index %ld (time %.4f, chan %d) %d times\n"),
           (int)min,lminpos,tpersample*(lminpos/outputs),
           (int)lminpos%outputs,mintimes);
    if (outrange)
      printf(Str(X_183,"%d sample%s out of range\n"), outrange, outrange==1 ? " " : "s ");
    else
      printf(Str(X_182,"Max scale factor = %.3f\n"), 
             (MYFLT)SHORTMAX/(MYFLT)((max>-min)?max:-min) );
    return;
}


