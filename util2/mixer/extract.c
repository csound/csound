/*  
extract.c

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
*   extract.c                                           *
*   extract a section of a sound file                   *
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
#include "cs.h"
/* #include "ustub.h" */
#include "soundio.h"
#undef Str
#define Str(x,y) y

/* Constants */
#define NUMBER_OF_SAMPLES       (4096)
#define SHORTMAX                (32767)
#define FIND(MSG)   if (*s == '\0')  \
        if (!(--argc) || (((s = *++argv)) && *s == '-')) \
        die(MSG);


long        sample;         /* Time file starts in samples */
long        stop;           /* Time file ends in samples */
long        numsamps;       /* Length in samples */
MYFLT       time;           /* Time file starts in secs */
MYFLT       endtime;        /* Time file ends in secs */
MYFLT       dur;            /* Length in secs */
int         nouts;          /* Number of out chanels */

SOUNDIN *   p;              /* Csound structure */

int debug   = 0;

/* Static function prototypes */

static SNDFILE *EXsndgetset(char *);
static void ExtractSound(SNDFILE *, SNDFILE *);

/* Externs */
extern long getsndin(SNDFILE *, MYFLT *, long, SOUNDIN *);
extern int  openout(char *, int);
extern short sfsampsize(int);
extern char *getstrformat(int);
extern SNDFILE *sndgetset(SOUNDIN *);

ENVIRON *csound;

static void usage(char *mesg)
{
    fprintf(stderr, "%s\n", mesg);
    fprintf(stderr,Str(X_1426,"Usage:\textracter [-flags] soundfile\n"));
    fprintf(stderr,Str(X_9,"Legal flags are:\n"));
    fprintf(stderr,Str(X_160,"-o fnam\tsound output filename\n"));
    fprintf(stderr,Str(X_1427,"-N\t\tnotify (ring the bell) when done\n"));
    fprintf(stderr,Str(X_1428,
                       "-S integer\tsample number at which to start file\n"));
    fprintf(stderr,Str(X_1429,"-Z integer\tsample number at which to end file\n"));
    fprintf(stderr,Str(X_1430,"-Q integer\tnumber of samples to read\n"));
    fprintf(stderr,Str(X_1431,"-T fpnum\ttime in secs at which to start file\n"));
    fprintf(stderr,Str(X_1432,"-E fpnum\ttime in secs at which to end file\n"));
    fprintf(stderr,Str(X_1433,"-D fpnum\tduration in secs of extract\n"));
    fprintf(stderr,Str(X_125,"-R\tRewrite header\n"));
    fprintf(stderr,Str(X_108,"-H#\tprint a heartbeat style 1, 2 or 3 at "
                       "each soundfile write\n"));
    fprintf(stderr,Str(X_780,"-v\t\tverbose mode for debugging\n"));
    fprintf(stderr,Str(X_1434,"flag defaults: extracter -otest -S 0\n"));
    exit(1);
}

int
main(int argc, char **argv)
{
    char        *inputfile = NULL;
    SNDFILE     *infd, *outfd;
    char        c, *s, *filnamp;
    SF_INFO     sfinfo;

    init_getstring(argc, argv);
    csoundPreCompile(csoundCreate(NULL));
    /* Check arguments */
    O.filnamspace = filnamp = mmalloc(csound,(long)1024);
    sample = -1; time = -FL(1.0);
    stop  = -1; endtime = -FL(1.0);
    numsamps = -1; dur = -FL(1.0);
    if (!(--argc))
      usage(Str(X_939,"Insufficient arguments"));
    do {
      s = *++argv;
      if (*s++ == '-')                      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch (c) {
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
          case 'S':
            FIND(Str(X_1417,"no start sample"));
            sample = atoi(s);
            while (*++s);
            if (time >= FL(0.0)) {
              warning(Str(X_1418,"-S overriding -T"));
              time = -FL(1.0);
            }
            break;
          case 'T':
            FIND(Str(X_1419,"no start time"));
            time = (MYFLT) atof(s);
            while (*++s);
            if (sample >= 0) {
              warning(Str(X_1420,"-T overriding -S"));
              sample = -1;
            }
            break;
          case 'Z':     /* Last sample */
            FIND(Str(X_25,"no end sample"));
            stop = atoi(s);
            while (*++s);
            if (endtime >= 0.0) {
              warning(Str(X_1435,"-Z overriding -E"));
              endtime = -FL(1.0);
            }
            if (dur >= 0.0) {
              warning(Str(X_1436,"-Z overriding -D"));
              dur = FL(0.1);
            }
            if (numsamps >=0) {
              warning(Str(X_1437,"-Z overriding -Q"));
              numsamps = -1;
            }
            break;
          case 'E':     /* Last time */
            FIND(Str(X_1438,"no end time"));
            endtime = (MYFLT) atof(s);
            while (*++s);
            if (dur >= 0.0) {
              warning(Str(X_1439,"-E overriding -D"));
              dur = FL(0.1);
            }
            if (numsamps >=0) {
              warning(Str(X_1440,"-E overriding -Q"));
              numsamps = -1;
            }
            if (stop >= 0) {
              warning(Str(X_1441,"-E overriding -Z"));
              stop = -1;
            }
            break;
          case 'D':
            FIND(Str(X_1030,"no duration"));
            dur = (MYFLT) atof(s);
            while (*++s);
            if (endtime >= FL(0.0)) {
              warning(Str(X_1442,"-D overriding -E"));
              endtime = -1.0;
            }
            if (numsamps >=0) {
              warning(Str(X_1443,"-D overriding -Q"));
              numsamps = -1;
            }
            if (stop >= 0) {
              warning(Str(X_1444,"-D overriding -Z"));
              stop = -1;
            }
            break;
          case 'Q':
            FIND(Str(X_1445,"no sample count"));
            numsamps = atoi(s);
            while (*++s);
            if (endtime >= FL(0.0)) {
              warning(Str(X_1446,"-Q overriding -E"));
              endtime = -1.0;
            }
            if (dur >= FL(0.0)) {
              warning(Str(X_1447,"-Q overriding -D"));
              dur = 0.1;
            }
            if (stop >= 0) {
              warning(Str(X_1448,"-Q overriding -Z"));
              stop = -1;
            }
            break;
          case 'H':
            if (isdigit(*s)) {
              int n;
              sscanf(s, "%d%n", &O.heartbeat, &n);
              s += n;
            }
            else O.heartbeat = 1;
            break;
          case 'R':
            O.rewrt_hdr = 1;
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
        if (inputfile != NULL) usage(Str(X_1287,"Too many input args"));
        inputfile = --s;
      }
    } while (--argc);
    dbfs_init(DFLT_DBFS);
    /* Read sound file */
    if (inputfile == NULL) usage(Str(X_149,"No input"));

    if (!(infd = EXsndgetset(inputfile))) {
      fprintf(stderr,Str(X_76,"%s: error while opening %s"),
              argv[0], inputfile);
      exit(1);
    }

    if (debug) {
      fprintf(stderr, Str(X_1450,"Times %f %f %f\nNums %ld %ld %ld\n"),
              time, endtime, dur, sample, stop, numsamps);
    }
    if (time >= FL(0.0)) sample = time*p->sr;
    if (endtime >= FL(0.0)) numsamps = endtime*p->sr - sample;
    else if (dur >= FL(0.0)) numsamps = dur*p->sr;
    else if (stop >= 0) numsamps = stop - sample;
    else if (numsamps < 0) numsamps = p->getframes - sample;

    if (sample<0) sample = 0;
    fprintf(stderr,
            Str(X_1451,"Extracting from sample %ld for %ld samples (%.5f secs)\n"),
            sample, numsamps, (MYFLT)numsamps/p->sr);

    nouts = p->nchanls;

    O.outformat = p->format; /* Copy from input file */
    O.sfsampsize = sfsampsize(O.outformat);
    O.filetyp = p->filetyp; /* Copy from input file */
    O.sfheader = 1;
    if (O.outfilename == NULL) O.outfilename = "test";
    sfinfo.frames = -1;
    sfinfo.samplerate = esr;
    sfinfo.channels = nchnls;
    sfinfo.format = type2sf(O.filetyp)|format2sf(O.outformat);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfd = sf_open_fd(openout(O.outfilename, 1), SFM_WRITE, &sfinfo, 1);
    if (O.rewrt_hdr) sf_command(outfd, SFC_SET_UPDATE_HEADER_AUTO, NULL, 0);
    esr = p->sr;
    nchnls = nouts;
    ExtractSound(infd, outfd);
    sf_close(outfd);
    if (O.ringbell) putc(7, stderr);
    return 0;

}

static SNDFILE *EXsndgetset(char *name)
{
    SNDFILE *infd;
    MYFLT   dur;
    static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
    static  OPTXT    optxt;
    static  MYFLT    fzero = FL(0.0);
    char         quotname[80];
    static  MYFLT sstrcod = (MYFLT)SSTRCOD;

    sssfinit();                 /* stand-alone init of SFDIR etc. */
    esr = FL(0.0);              /* set esr 0. with no orchestra   */
    optxt.t.outoffs = &argoffs; /* point to dummy OUTOCOUNT       */
    p = (SOUNDIN *) mcalloc(csound, (long)sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->h.optext = &optxt;
    p->ifilno = &sstrcod;
    p->iskptim = &fzero;
    p->iformat = &fzero;
    sprintf(quotname,"%c%s%c",'"',name,'"');
    p->STRARG = quotname;
    if ((infd = sndgetset(p)) == NULL) /* open sndfil, do skiptime */
      return NULL;
    p->getframes = p->framesrem;
    dur = (MYFLT) p->getframes / p->sr;
    printf(Str(X_1452,"extracting from %ld sample frames (%3.1f secs)\n"),
           p->getframes, dur);
    return infd;
}

static void ExtractSound(SNDFILE *infd, SNDFILE *outfd)
{
    MYFLT buffer[NUMBER_OF_SAMPLES];
    long  read_in;
    long  bytes = 0;
    int   block = 0;

    sf_seek(infd, nouts, SEEK_CUR);
    while (numsamps>0) {
      int num = NUMBER_OF_SAMPLES/nchnls;
      if (numsamps<num) num = numsamps;
      numsamps -= num;
      num = nouts;
      read_in = sf_read_MYFLT(infd, buffer, num);
      sf_write_MYFLT(outfd, buffer, read_in);
      block++;
      bytes += read_in;
      if (O.heartbeat) {
        if (O.heartbeat==1) {
#ifdef SYMANTEC
          nextcurs();
#elif __BEOS__
          putc('.', stderr); fflush(stderr);
#else
          putc("|/-\\"[nrecs&3], stderr); putc(8,stderr);
#endif
        }
        else if (O.heartbeat==2) putc('.', stderr);
        else if (O.heartbeat==3) {
          int n;
          err_printf( "%d(%.3f)%n", nrecs, nrecs/ekr, &n);
          while (n--) putc(8, stderr);
        }
        else putc(7, stderr);
      }
      if (read_in < num) break;
    }
    rewriteheader(outfd, bytes);
    return;
}


