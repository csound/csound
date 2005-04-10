/*
    xtrct.c

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
#ifdef LINUX
#include <unistd.h>
#endif
#include "cs.h"
#include "soundio.h"
#include <sndfile.h>

/* Constants */
#define NUMBER_OF_SAMPLES       (4096)
#define SHORTMAX                (32767)
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            csoundDie(&cenviron, MSG);

long        sample;         /* Time file starts in samples */
long        stop;           /* Time file ends in samples */
long        numsamps;       /* Length in samples */
MYFLT       stime;          /* Time file starts in secs */
MYFLT       endtime;        /* Time file ends in secs */
MYFLT       dur;            /* Length in secs */
int         outputs;        /* Number of out chanels */

SOUNDIN *   p;              /* Csound structure */

static int debug   = 0;

/* Static function prototypes */

static SNDFILE*  EXsndgetset(char *);
static void ExtractSound(SNDFILE*, SNDFILE*);

/* Externs */
extern int  openout(char *, int);
extern char *getstrformat(int);
extern short sfsampsize(int);

static void usage(char *mesg)
{
    err_printf( "%s\n", mesg);
    err_printf("Usage:\textracter [-flags] soundfile\n");
    err_printf( "Legal flags are:\n");
    err_printf("-o fname\tsound output filename\n");
    err_printf("-N\t\tnotify (ring the bell) when done\n");
    err_printf("-S integer\tsample number at which to start file\n");
    err_printf("-Z integer\tsample number at which to end file\n");
    err_printf("-Q integer\tnumber of samples to read\n");


    err_printf("-T fpnum\ttime in secs at which to start file\n");
    err_printf("-E fpnum\ttime in secs at which to end file\n");
    err_printf("-D fpnum\tduration in secs of extract\n");
    err_printf("-R\tRewrite header\n");
    err_printf("-H\t\tHeartbeat\n");
    err_printf("-v\t\tverbose mode for debugging\n");
    err_printf("-- fname\tLog output to file\n");
    err_printf("flag defaults: extracter -otest -S 0\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    ENVIRON     *csound = &cenviron;
    char        *inputfile = NULL;
    SNDFILE*    infd;
    SNDFILE*    outfd;
    char        c, *s, *filnamp;
    SF_INFO     sfinfo;

    init_getstring(argc, argv);
    csoundPreCompile(csoundCreate(NULL));
/*     response_expand(&argc, &argv); /\* Permits "@xxx" response files *\/ */
    /* Check arguments */
    O.filnamspace = filnamp = mmalloc(&cenviron, (long)1024);
    sample = -1; stime = -FL(1.0);
    stop  = -1; endtime = -FL(1.0);
    numsamps = -1; dur = -FL(1.0);
    if (!(--argc))
      usage("Insufficient arguments");
    do {
      s = *++argv;
      if (*s++ == '-')                      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND("no outfilename")
              O.outfilename = filnamp;            /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O.outfilename,"stdin") == 0)
              csoundDie(&cenviron, "-o cannot be stdin");
            if (strcmp(O.outfilename,"stdout") == 0) {
#ifdef THINK_C
              csoundDie(&cenviron, "stdout audio not supported");
#else
              if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                csoundDie(&cenviron, "too many open files");
              dup2(2,1);                /* & send 1's to stderr */
#endif
            }
            break;
          case 'S':
            FIND("no start sample");
            sample = atoi(s);
            while (*++s);
            if (stime >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-S overriding -T");
              stime = -1.0;
            }
            break;
          case 'T':
            FIND("no start time");
            stime = (MYFLT) atof(s);
            while (*++s);
            if (sample >= 0) {
              if (O.msglevel & WARNMSG)
                printf("-T overriding -S");
              sample = -1;
            }
            break;
          case 'Z':     /* Last sample */
            FIND("no end sample");
            stop = atoi(s);
            while (*++s);
            if (endtime >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-Z overriding -E");
              endtime = -1.0;
            }
            if (dur >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-Z overriding -D");
              dur = 0.1f;
            }
            if (numsamps >=0) {
              if (O.msglevel & WARNMSG)
                printf("-Z overriding -Q");
              numsamps = -1;
            }
            break;
          case 'E':     /* Last time */
            FIND("no end time");
            endtime = (MYFLT) atof(s);
            while (*++s);
            if (dur >= 0.0) {
              if (O.msglevel & WARNMSG)
                printf("-E overriding -D");
              dur = 0.1f;
            }
            if (numsamps >=0) {
              if (O.msglevel & WARNMSG)
                printf("-E overriding -Q");
              numsamps = -1;
            }
            if (stop >= 0) {
              if (O.msglevel & WARNMSG)
                printf("-E overriding -Z");
              stop = -1;
            }
            break;
          case 'D':
            FIND("no duration");
            dur = (MYFLT) atof(s);
            while (*++s);
            if (endtime >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-D overriding -E");
              endtime = -1.0;
            }
            if (numsamps >=0) {
              if (O.msglevel & WARNMSG)
                printf("-D overriding -Q");
              numsamps = -1;
            }
            if (stop >= 0) {
              if (O.msglevel & WARNMSG)
                printf("-D overriding -Z");
              stop = -1;
            }
            break;
          case 'Q':
            FIND("no sample count");
            numsamps = atoi(s);
            while (*++s);
            if (endtime >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-Q overriding -E");
              endtime = -1.0;
            }
            if (dur >= FL(0.0)) {
              if (O.msglevel & WARNMSG)
                printf("-Q overriding -D");
              dur = 0.1f;
            }
            if (stop >= 0) {
              if (O.msglevel & WARNMSG)
                printf("-Q overriding -Z");
              stop = -1;
            }
            break;
          case 'H':
            O.heartbeat = 1;
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
            sprintf(errmsg,"unknown flag -%c", c);
            usage(errmsg);
          }
      else {
        if (inputfile != NULL) usage("Too many inputs");
        inputfile = --s;
      }
    } while (--argc);

    dbfs_init(csound, DFLT_DBFS);
    /* Read sound file */
    if (inputfile == NULL) usage("No input");

    if (!(infd = EXsndgetset(inputfile))) {
      err_printf("%s: error while opening %s", argv[0], inputfile);
      exit(1);
    }

    if (debug) {
      err_printf("Times %f %f %f\nNums %ld %ld %ld\n",
                 stime, endtime, dur, sample, stop, numsamps);
    }
    if (stime >= FL(0.0)) sample = (long)(stime*p->sr);
    if (endtime >= FL(0.0)) numsamps = (long)(endtime*p->sr - sample);
    else if (dur >= FL(0.0)) numsamps = (long)(dur*p->sr);
    else if (stop >= 0) numsamps = stop - sample;
    else if (numsamps < 0) numsamps = p->getframes - sample;

    if (sample<0) sample = 0;
    err_printf("Extracting from sample %d for %d samples (%.5f secs)\n",
               sample, numsamps, (MYFLT)numsamps/p->sr);

    outputs = p->nchanls;

    O.outformat = p->format; /* Copy from input file */
    O.sfsampsize = sfsampsize(O.outformat);
    O.filetyp = p->filetyp; /* Copy from input file */
    O.sfheader = 1;
    if (O.outfilename == NULL)  O.outfilename = "test";
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (cenviron.esr + FL(0.5));
    sfinfo.channels = cenviron.nchnls;
    sfinfo.format = TYPE2SF(O.filetyp) | FORMAT2SF(O.outformat);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfd = sf_open_fd(openout(O.outfilename, 1),SFM_WRITE, &sfinfo, 1);
    cenviron.esr = (MYFLT)p->sr;
    cenviron.nchnls = outputs;
    ExtractSound(infd, outfd);
    sf_close(outfd);
    if (O.ringbell) putc(7, stderr);
    return 0;
}

static SNDFILE*
EXsndgetset(char *name)
{
    ENVIRON     *csound = &cenviron;
    SNDFILE*    infd;
    MYFLT       dur;

    csoundInitEnv(csound);      /* stand-alone init of SFDIR etc. */
    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    p->channel = ALLCHNLS;
    p->skiptime = FL(0.0);
    strcpy(p->sfname, name);
    if ((infd = csound->sndgetset(csound, p)) == 0) /*open sndfil, do skiptime*/
        return(0);
    p->getframes = p->framesrem;
    dur = (MYFLT) p->getframes / p->sr;
    printf("extracting from %ld sample frames (%3.1f secs)\n",
           (long) p->getframes, dur);
    return(infd);
}

static void
ExtractSound(SNDFILE* infd, SNDFILE* outfd)
{
    double buffer[NUMBER_OF_SAMPLES];
    long  read_in;
    long  bytes = 0;
    int   block = 0;


    sf_seek(infd, outputs*sample*O.sfsampsize, SEEK_CUR);
    while (numsamps>0) {
      int num = NUMBER_OF_SAMPLES;
      if (numsamps<num) num = numsamps;
      numsamps -= num;
      num *= O.sfsampsize*outputs;
      read_in = sf_read_double(infd, buffer, num);
      sf_write_double(outfd, buffer, read_in);
      block++;
      bytes += read_in;
      if (O.rewrt_hdr) {
        rewriteheader(outfd, bytes);
        sf_seek(outfd, 0L, SEEK_END); /* Place at end again */
      }
      if (O.heartbeat) {
        putc("|/-\\"[block&3], stderr);
        putc('\b',stderr);
      }
      if (read_in < num) break;
    }
    rewriteheader(outfd, bytes);
    return;
}

