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

#include "csdl.h"
#include "soundio.h"
#include <ctype.h>
#include <sndfile.h>

/* Constants */
#define NUMBER_OF_SAMPLES       (4096)
#define SHORTMAX                (32767)
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-')) \
                            csound->Die(csound, MSG);

typedef struct {
  long        sample;         /* Time file starts in samples */
  long        stop;           /* Time file ends in samples */
  long        numsamps;       /* Length in samples */
  MYFLT       stime;          /* Time file starts in secs */
  MYFLT       endtime;        /* Time file ends in secs */
  MYFLT       dur;            /* Length in secs */
  int         outputs;        /* Number of out chanels */
  SOUNDIN *   p;              /* Csound structure */
} XTRC;

/* Static function prototypes */

static SNDFILE*  EXsndgetset(ENVIRON *, XTRC*,char *);
static void ExtractSound(ENVIRON *, XTRC *, SNDFILE*, SNDFILE*);

static short sfsampsize(int type)
{
    switch (type & SF_FORMAT_SUBMASK) {
      case SF_FORMAT_PCM_16:  return 2;     /* Signed 16 bit data */
      case SF_FORMAT_PCM_32:  return 4;     /* Signed 32 bit data */
      case SF_FORMAT_FLOAT:   return 4;     /* 32 bit float data */
      case SF_FORMAT_PCM_24:  return 3;     /* Signed 24 bit data */
      case SF_FORMAT_DOUBLE:  return 8;     /* 64 bit float data */
    }
    return 1;
}

static void usage(ENVIRON *csound, char *mesg)
{
    csound->Message(csound, "%s\n", mesg);
    csound->Message(csound,"Usage:\textracter [-flags] soundfile\n");
    csound->Message(csound, "Legal flags are:\n");
    csound->Message(csound,"-o fname\tsound output filename\n");
    csound->Message(csound,"-N\t\tnotify (ring the bell) when done\n");
    csound->Message(csound,"-S integer\tsample number at which to start file\n");
    csound->Message(csound,"-Z integer\tsample number at which to end file\n");
    csound->Message(csound,"-Q integer\tnumber of samples to read\n");

    csound->Message(csound,"-T fpnum\ttime in secs at which to start file\n");
    csound->Message(csound,"-E fpnum\ttime in secs at which to end file\n");
    csound->Message(csound,"-D fpnum\tduration in secs of extract\n");
    csound->Message(csound,"-R\tRewrite header\n");
    csound->Message(csound,"-H\t\tHeartbeat\n");
    csound->Message(csound,"-v\t\tverbose mode for debugging\n");
    csound->Message(csound,"-- fname\tLog output to file\n");
    csound->Message(csound,"flag defaults: extracter -otest -S 0\n");
}

static int xtrct(void *csound_, int argc, char **argv)
{
    ENVIRON     *csound = csound_;
    char        *inputfile = NULL;
    SNDFILE*    infd;
    SNDFILE*    outfd;
    void        *fd;
    char        c, *s, *filnamp;
    SF_INFO     sfinfo;
    int         debug   = 0;
    int         Omsg = csound->oparms->msglevel;
    XTRC        xtrc;

    /* Check arguments */
    csound->oparms->filnamspace = filnamp = csound->Malloc(csound, (long)1024);
    xtrc.sample = -1; xtrc.stime = -FL(1.0);
    xtrc.stop  = -1; xtrc.endtime = -FL(1.0);
    xtrc.numsamps = -1; xtrc.dur = -FL(1.0);
    if (!(--argc)) {
      usage(csound,"Insufficient arguments");
      return 1;
    }
    do {
      s = *++argv;
      if (*s++ == '-')                      /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND("no outfilename")
            csound->oparms->outfilename = filnamp;      /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(csound->oparms->outfilename, "stdin") == 0)
              csound->Die(csound, "-o cannot be stdin");
#ifdef THINK_C
            if (strcmp(csound->oparms->outfilename, "stdout") == 0) {
              csound->Die(csound, "stdout audio not supported");
            }
#endif
            break;
          case 'S':
            FIND("no start sample");
            xtrc.sample = atoi(s);
            while (*++s);
            if (xtrc.stime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-S overriding -T");
              xtrc.stime = -1.0;
            }
            break;
          case 'T':
            FIND("no start time");
            xtrc.stime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.sample >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-T overriding -S");
              xtrc.sample = -1;
            }
            break;
          case 'Z':     /* Last sample */
            FIND("no end sample");
            xtrc.stop = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Z overriding -E");
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Z overriding -D");
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Z overriding -Q");
              xtrc.numsamps = -1;
            }
            break;
          case 'E':     /* Last time */
            FIND("no end time");
            xtrc.endtime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.dur >= 0.0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-E overriding -D");
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-E overriding -Q");
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-E overriding -Z");
              xtrc.stop = -1;
            }
            break;
          case 'D':
            FIND("no duration");
            xtrc.dur = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-D overriding -E");
              xtrc.endtime = -1.0;
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-D overriding -Q");
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-D overriding -Z");
              xtrc.stop = -1;
            }
            break;
          case 'Q':
            FIND("no sample count");
            xtrc.numsamps = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Q overriding -E");
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Q overriding -D");
              xtrc.dur = FL(0.1);
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,"-Q overriding -Z");
              xtrc.stop = -1;
            }
            break;
          case 'H':
            csound->oparms->heartbeat = 1;
            break;
          case 'R':
            csound->oparms->rewrt_hdr = 1;
            break;
          case 'N':
            csound->oparms->ringbell = 1;             /* notify on completion */
            break;
          case 'v':                       /* Verbose mode */
            debug = 1;
            break;
          default:
            sprintf(csound->errmsg, "unknown flag -%c", c);
            usage(csound,csound->errmsg);
          }
      else {
        if (inputfile != NULL) usage(csound,"Too many inputs");
        inputfile = --s;
      }
    } while (--argc);

    /* Read sound file */
    if (inputfile == NULL) usage(csound,"No input");

    if (!(infd = EXsndgetset(csound, &xtrc, inputfile))) {
      csound->Message(csound,"%s: error while opening %s", argv[0], inputfile);
      return 1;
    }

    if (debug) {
      csound->Message(csound,"Times %f %f %f\nNums %ld %ld %ld\n",
                 xtrc.stime, xtrc.endtime, xtrc.dur, xtrc.sample,
                      xtrc.stop, xtrc.numsamps);
    }
    if (xtrc.stime >= FL(0.0)) xtrc.sample = (long)(xtrc.stime*xtrc.p->sr);
    if (xtrc.endtime >= FL(0.0))
      xtrc.numsamps = (long)(xtrc.endtime*xtrc.p->sr - xtrc.sample);
    else if (xtrc.dur >= FL(0.0)) xtrc.numsamps = (long)(xtrc.dur*xtrc.p->sr);
    else if (xtrc.stop >= 0) xtrc.numsamps = xtrc.stop - xtrc.sample;
    else if (xtrc.numsamps < 0) xtrc.numsamps =xtrc. p->getframes - xtrc.sample;

    if (xtrc.sample<0) xtrc.sample = 0;
    csound->Message(csound,"Extracting from sample %ld for %ld samples (%.5f secs)\n",
                    xtrc.sample, xtrc.numsamps, (MYFLT)xtrc.numsamps/xtrc.p->sr);

    xtrc.outputs = xtrc.p->nchanls;

    csound->oparms->outformat = xtrc.p->format; /* Copy from input file */
    csound->oparms->sfsampsize = sfsampsize(csound->oparms->outformat);
    csound->oparms->filetyp = xtrc.p->filetyp; /* Copy from input file */
    csound->oparms->sfheader = 1;
    if (csound->oparms->outfilename == NULL)
      csound->oparms->outfilename = "test";
    csound->esr = (MYFLT)xtrc.p->sr;
    csound->nchnls = xtrc.outputs;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = csound->nchnls;
    sfinfo.format = TYPE2SF(csound->oparms->filetyp) |
                    FORMAT2SF(csound->oparms->outformat);
    /* open file for write */
    fd = NULL;
    if (strcmp(csound->oparms->outfilename, "stdout") == 0 ||
        strcmp(csound->oparms->outfilename, "-") == 0) {
      outfd = sf_open_fd(1, SFM_WRITE, &sfinfo, 0);
      if (outfd != NULL) {
        fd = csound->CreateFileHandle(csound, &outfd, CSFILE_SND_W, "stdout");
        if (fd == NULL) {
          sf_close(outfd);
          csound->Die(csound, Str("Memory allocation failure"));
        }
      }
    }
    else
      fd = csound->FileOpen(csound, &outfd, CSFILE_SND_W,
                                    csound->oparms->outfilename, &sfinfo,
                                    "SFDIR");
    if (fd == NULL)
      csound->Die(csound, Str("Failed to open output file %s"),
                          csound->oparms->outfilename);
    ExtractSound(csound, &xtrc, infd, outfd);
    if (csound->oparms->ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c", '\007');
    return 0;
}

static SNDFILE*
EXsndgetset(ENVIRON *csound, XTRC *x, char *name)
{
    SNDFILE*    infd;
    MYFLT       dur;

    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    x->p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    x->p->channel = ALLCHNLS;
    x->p->skiptime = FL(0.0);
    strcpy(x->p->sfname, name);
    if ((infd = csound->sndgetset(csound, x->p)) == 0) /*open sndfil, do skiptime*/
        return(0);
    x->p->getframes = x->p->framesrem;
    dur = (MYFLT) x->p->getframes / x->p->sr;
    csound->Message(csound,"extracting from %ld sample frames (%3.1f secs)\n",
           (long) x->p->getframes, dur);
    return(infd);
}

static void
ExtractSound(ENVIRON *csound, XTRC *x, SNDFILE* infd, SNDFILE* outfd)
{
    double buffer[NUMBER_OF_SAMPLES];
    long  read_in;
    long  frames = 0;
    int   block = 0;

    sf_seek(infd, x->sample, SEEK_CUR);
    while (x->numsamps>0) {
      int num = NUMBER_OF_SAMPLES / x->outputs;
      if (x->numsamps < num)
        num = x->numsamps;
      x->numsamps -= num;
      read_in = sf_readf_double(infd, buffer, num);
      sf_writef_double(outfd, buffer, read_in);
      block++;
      frames += read_in;
      if (csound->oparms->rewrt_hdr) {
        sf_command(outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
        sf_seek(outfd, 0L, SEEK_END); /* Place at end again */
      }
      if (csound->oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
      if (read_in < num) break;
    }
    sf_command(outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
    return;
}

/* module interface */

PUBLIC int csoundModuleCreate(void *csound)
{
    int retval = ((ENVIRON*) csound)->AddUtility(csound, "extractor", xtrct);
    if (!retval) {
      retval = ((ENVIRON*) csound)->SetUtilityDescription(csound, "extractor",
                    "Extract part of a sound file");
    }
    return retval;
}

