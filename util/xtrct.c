/*
    xtrct.c

    Copyright (C) 1995 John ffitch

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

#include "std_util.h"
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

static SNDFILE*  EXsndgetset(CSOUND *, XTRC*,char *);
static void ExtractSound(CSOUND *, XTRC *, SNDFILE*, SNDFILE*, OPARMS *);

static void usage(CSOUND *csound, char *mesg, ...)
{
    va_list args;

    csound->Message(csound,Str("Usage:\textractor [-flags] soundfile\n"));
    csound->Message(csound,Str("Legal flags are:\n"));
    csound->Message(csound,Str("-o fname\tsound output filename\n"));
    csound->Message(csound,Str("-N\t\tnotify (ring the bell) when done\n"));
    csound->Message(csound,Str("-S integer\tsample number at which"
                               " to start file\n"));
    csound->Message(csound,Str("-Z integer\tsample number at which to end file\n"));
    csound->Message(csound,Str("-Q integer\tnumber of samples to read\n"));

    csound->Message(csound,Str("-T fpnum\ttime in secs at which to start file\n"));
    csound->Message(csound,Str("-E fpnum\ttime in secs at which to end file\n"));
    csound->Message(csound,Str("-D fpnum\tduration in secs of extract\n"));
    csound->Message(csound,Str("-R\tRewrite header\n"));
    csound->Message(csound,Str("-H\t\tHeartbeat\n"));
    csound->Message(csound,Str("-v\t\tverbose mode for debugging\n"));
    csound->Message(csound,Str("-- fname\tLog output to file\n"));
    csound->Message(csound,Str("flag defaults: extractor -otest -S 0\n"));

    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("extractor: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static int xtrct(CSOUND *csound, int argc, char **argv)
{
    OPARMS      O;
    char        *inputfile = NULL;
    SNDFILE*    infd;
    SNDFILE*    outfd;
    void        *fd;
    char        c, *s;
    SF_INFO     sfinfo;
    int         debug   = 0;
    int         Omsg;
    XTRC        xtrc;

    csound->GetOParms(csound, &O);
    Omsg = O.msglevel;

    /* Check arguments */
    xtrc.sample = -1; xtrc.stime = -FL(1.0);
    xtrc.stop  = -1; xtrc.endtime = -FL(1.0);
    xtrc.numsamps = -1; xtrc.dur = -FL(1.0);
    if (!(--argc)) {
      usage(csound,Str("Insufficient arguments"));
      return 1;
    }
    do {
      s = *++argv;
      if (*s++ == '-')                  /* read all flags:  */
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no outfilename"))
            O.outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (strcmp(O.outfilename, "stdin") == 0)
              csound->Die(csound, Str("-o cannot be stdin"));
            break;
          case 'S':
            FIND(Str("no start sample"));
            xtrc.sample = atoi(s);
            while (*++s);
            if (xtrc.stime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-S overriding -T"));
              xtrc.stime = -1.0;
            }
            break;
          case 'T':
            FIND(Str("no start time"));
            xtrc.stime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.sample >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-T overriding -S"));
              xtrc.sample = -1;
            }
            break;
          case 'Z':     /* Last sample */
            FIND(Str("no end sample"));
            xtrc.stop = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Z overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Z overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Z overriding -Q"));
              xtrc.numsamps = -1;
            }
            break;
          case 'E':     /* Last time */
            FIND(Str("no end time"));
            xtrc.endtime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.dur >= 0.0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-E overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-E overriding -Q"));
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-E overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'D':
            FIND(Str("no duration"));
            xtrc.dur = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-D overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.numsamps >=0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-D overriding -Q"));
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-D overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'Q':
            FIND(Str("no sample count"));
            xtrc.numsamps = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Q overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Q overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.stop >= 0) {
              if (Omsg & WARNMSG)
                csound->Message(csound,Str("-Q overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'H':
            O.heartbeat = 1;
            break;
          case 'R':
            O.rewrt_hdr = 1;
            break;
          case 'N':
            O.ringbell = 1;            /* notify on completion */
            break;
          case 'v':                     /* Verbose mode */
            debug = 1;
            break;
          default:
            usage(csound, Str("unknown flag -%c"), c);
          }
      else {
        if (inputfile != NULL) usage(csound,Str("Too many inputs"));
        inputfile = --s;
      }
    } while (--argc);

    /* Read sound file */
    if (inputfile == NULL) usage(csound,Str("No input"));

    if (!(infd = EXsndgetset(csound, &xtrc, inputfile))) {
      csound->Message(csound,Str("%s: error while opening %s"), argv[0], inputfile);
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
    csound->Message(csound,
                    Str("Extracting from sample %ld for %ld samples (%.5f secs)\n"),
                    xtrc.sample, xtrc.numsamps, (MYFLT)xtrc.numsamps/xtrc.p->sr);

    xtrc.outputs = xtrc.p->nchanls;

    O.outformat = xtrc.p->format; /* Copy from input file */
    O.sfsampsize = csound->sfsampsize(FORMAT2SF(O.outformat));
    O.filetyp = xtrc.p->filetyp; /* Copy from input file */
    O.sfheader = 1;
    if (O.outfilename == NULL)
      O.outfilename = "test";

    csound->SetUtilSr(csound, (MYFLT)xtrc.p->sr);
    csound->SetUtilNchnls(csound, xtrc.outputs);
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) ((MYFLT)xtrc.p->sr + FL(0.5));
    sfinfo.channels = xtrc.outputs;
    sfinfo.format = TYPE2SF(O.filetyp) | FORMAT2SF(O.outformat);
    /* open file for write */
    fd = NULL;
    if (strcmp(O.outfilename, "stdout") == 0 ||
        strcmp(O.outfilename, "-") == 0) {
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
      fd = csound->FileOpen2(csound, &outfd, CSFILE_SND_W,
                       O.outfilename, &sfinfo, "SFDIR",
                       csound->type2csfiletype(O.filetyp, O.outformat), 0);
    if (fd == NULL)
      csound->Die(csound, Str("Failed to open output file %s"),
                          O.outfilename);
    ExtractSound(csound, &xtrc, infd, outfd, &O);
    if (O.ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c", '\007');
    return 0;
}

static SNDFILE*
EXsndgetset(CSOUND *csound, XTRC *x, char *name)
{
    SNDFILE*    infd;
    MYFLT       dur;

    csound->SetUtilSr(csound,FL(0.0));      /* set esr 0. with no orchestra   */
    x->p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    x->p->channel = ALLCHNLS;
    x->p->skiptime = FL(0.0);
    strncpy(x->p->sfname, name,  MAXSNDNAME-1);
    if ((infd = csound->sndgetset(csound, x->p)) == 0) /*open sndfil, do skiptime*/
        return(0);
    x->p->getframes = x->p->framesrem;
    dur = (MYFLT) x->p->getframes / x->p->sr;
    csound->Message(csound,Str("extracting from %ld sample frames (%3.1f secs)\n"),
           (long) x->p->getframes, dur);
    return(infd);
}

static void
ExtractSound(CSOUND *csound, XTRC *x, SNDFILE* infd, SNDFILE* outfd, OPARMS *oparms)
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
      if (oparms->rewrt_hdr) {
        sf_command(outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
        sf_seek(outfd, 0L, SEEK_END); /* Place at end again */
      }
      if (oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
      if (read_in < num) break;
    }
    sf_command(outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
    return;
}

/* module interface */

int xtrct_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "extractor", xtrct);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "extractor",
                                             Str("Extract part of a sound file"));
    }
    return retval;
}
