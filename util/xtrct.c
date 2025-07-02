/*
    xtrct.c!

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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


/* Constants */
#define NUMBER_OF_SAMPLES       (4096)
#define SHORTMAX                (32767)
#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
      csound->Die(csound, "%s", MSG);

typedef struct {
  long        sample;         /* Time file starts in samples */
  long        stop;           /* Time file ends in samples */
  long        numsamps;       /* Length in samples */
  MYFLT       stime;          /* Time file starts in secs */
  MYFLT       endtime;        /* Time file ends in secs */
  MYFLT       dur;            /* Length in secs */
  int32_t         outputs;        /* Number of out chanels */
  SOUNDIN *   p;              /* Csound structure */
} XTRC;

/* Static function prototypes */

static SNDFILE*  EXsndgetset(CSOUND *, XTRC*,char *);
static void ExtractSound(CSOUND *, XTRC *, SNDFILE*, SNDFILE*, OPARMS *);

static void usage(CSOUND *csound, char *mesg, ...)
{
    va_list args;

    csound->Message(csound,"%s", Str("Usage:\textractor [-flags] soundfile\n"));
    csound->Message(csound,"%s", Str("Legal flags are:\n"));
    csound->Message(csound,"%s", Str("-o fname\tsound output filename\n"));
    csound->Message(csound,"%s", Str("-N\t\tnotify (ring the bell) when done\n"));
    csound->Message(csound,"%s", Str("-S integer\tsample number at which"
                                     " to start file\n"));
    csound->Message(csound,"%s", Str("-Z integer\tsample number at which"
                                     " to end file\n"));
    csound->Message(csound,"%s", Str("-Q integer\tnumber of samples to read\n"));

    csound->Message(csound,"%s", Str("-T fpnum\ttime in secs at which"
                                     " to start file\n"));
    csound->Message(csound,"%s", Str("-E fpnum\ttime in secs at which"
                                     " to end file\n"));
    csound->Message(csound,"%s", Str("-D fpnum\tduration in secs of extract\n"));
    csound->Message(csound,"%s", Str("-R\tRewrite header\n"));
    csound->Message(csound,"%s", Str("-H\t\tHeartbeat\n"));
    csound->Message(csound,"%s", Str("-v\t\tverbose mode for debugging\n"));
    csound->Message(csound,"%s", Str("-- fname\tLog output to file\n"));
    csound->Message(csound,"%s", Str("flag defaults: extractor -otest -S 0\n"));

    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("extractor: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static int32_t xtrct(CSOUND *csound, int32_t argc, char **argv)
{
    OPARMS *     O = (OPARMS *) csound->Calloc(csound, sizeof(OPARMS));
    char        *inputfile = NULL;
    SNDFILE*    infd;
    SNDFILE*    outfd;
    void        *fd;
    char        c, *s;
    SFLIB_INFO     sfinfo;
    int32_t         debug   = 0;
    int32_t         Omsg;
    XTRC        xtrc;
    memcpy(O,csound->GetOParms(csound), sizeof(OPARMS));

    Omsg = O->msglevel;

    /* Check arguments */
    xtrc.sample = -1; xtrc.stime = -FL(1.0);
    xtrc.stop  = -1; xtrc.endtime = -FL(1.0);
    xtrc.numsamps = -1; xtrc.dur = -FL(1.0);
    if (UNLIKELY(!(--argc))) {
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
            O->outfilename = s;         /* soundout name */
            for ( ; *s != '\0'; s++) ;
            if (UNLIKELY(strcmp(O->outfilename, "stdin") == 0))
              csound->Die(csound, "%s", Str("-o cannot be stdin"));
            break;
          case 'S':
            FIND(Str("no start sample"));
            xtrc.sample = atoi(s);
            while (*++s);
            if (xtrc.stime >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-S overriding -T"));
              xtrc.stime = -1.0;
            }
            break;
          case 'T':
            FIND(Str("no start time"));
            xtrc.stime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.sample >= 0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-T overriding -S"));
              xtrc.sample = -1;
            }
            break;
          case 'Z':     /* Last sample */
            FIND(Str("no end sample"));
            xtrc.stop = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Z overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Z overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Z overriding -Q"));
              xtrc.numsamps = -1;
            }
            break;
          case 'E':     /* Last time */
            FIND(Str("no end time"));
            xtrc.endtime = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.dur >= 0.0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-E overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.numsamps >=0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-E overriding -Q"));
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-E overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'D':
            FIND(Str("no duration"));
            xtrc.dur = (MYFLT) atof(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-D overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.numsamps >=0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-D overriding -Q"));
              xtrc.numsamps = -1;
            }
            if (xtrc.stop >= 0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-D overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'Q':
            FIND(Str("no sample count"));
            xtrc.numsamps = atoi(s);
            while (*++s);
            if (xtrc.endtime >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Q overriding -E"));
              xtrc.endtime = -1.0;
            }
            if (xtrc.dur >= FL(0.0)) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Q overriding -D"));
              xtrc.dur = FL(0.1);
            }
            if (xtrc.stop >= 0) {
              if (UNLIKELY(Omsg & CS_WARNMSG))
                csound->Message(csound,"%s", Str("-Q overriding -Z"));
              xtrc.stop = -1;
            }
            break;
          case 'H':
            O->heartbeat = 1;
            break;
          case 'R':
            O->rewrt_hdr = 1;
            break;
          case 'N':
            O->ringbell = 1;            /* notify on completion */
            break;
          case 'v':                     /* Verbose mode */
            debug = 1;
            break;
          default:
            usage(csound, Str("unknown flag -%c"), c);
          }
      else {
        if (UNLIKELY(inputfile != NULL)) usage(csound,Str("Too many inputs"));
        inputfile = --s;
      }
    } while (--argc);

    /* Read sound file */
    if (UNLIKELY(inputfile == NULL)) usage(csound,Str("No input"));

    if (UNLIKELY(!(infd = EXsndgetset(csound, &xtrc, inputfile)))) {
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

    O->outformat = xtrc.p->format; /* Copy from input file */
    O->sfsampsize = csound->SndfileSampleSize(FORMAT2SF(O->outformat));
    O->filetyp = xtrc.p->filetyp; /* Copy from input file */
    
    if (O->outfilename == NULL)
      O->outfilename = "test";

    (csound->GetUtility(csound))->SetUtilSr(csound, (MYFLT)xtrc.p->sr);
    (csound->GetUtility(csound))->SetUtilNchnls(csound, xtrc.outputs);
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    //sfinfo.frames = 0/*was -1*/;
    sfinfo.samplerate = (int32_t) ((MYFLT)xtrc.p->sr + FL(0.5));
    sfinfo.channels = xtrc.outputs;
    sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
    /* open file for write */
    fd = NULL;
    if (strcmp(O->outfilename, "stdout") == 0 ||
        strcmp(O->outfilename, "-") == 0) {
      outfd = csound->SndfileOpenFd(csound,1, SFM_WRITE, &sfinfo, 0);
      if (outfd != NULL) {
        fd = csound->CreateFileHandle(csound, &outfd, CSFILE_SND_W, "stdout");
        if (UNLIKELY(fd == NULL)) {
          csound->SndfileClose(csound,outfd);
          csound->Die(csound, "%s", Str("Memory allocation failure"));
        }
      }
    }
    else
      fd = csound->FileOpen(csound, &outfd, CSFILE_SND_W,
                       O->outfilename, &sfinfo, "SFDIR",
                       csound->Type2CsfileType(O->filetyp, O->outformat), 0);
    if (UNLIKELY(fd == NULL))
      csound->Die(csound, Str("Failed to open output file %s: %s"),
                  O->outfilename, Str(csound->SndfileStrError(csound,NULL)));
    ExtractSound(csound, &xtrc, infd, outfd, O);
    if (O->
        ringbell)
      csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c", '\007');
    return 0;
}

static SNDFILE*
EXsndgetset(CSOUND *csound, XTRC *x, char *name)
{
    SNDFILE*    infd;
    MYFLT       dur;

    (csound->GetUtility(csound))->SetUtilSr(csound,FL(0.0));      /* set esr 0. with no orchestra   */
    x->p = (SOUNDIN *) csound->Calloc(csound, sizeof(SOUNDIN));
    x->p->channel = ALLCHNLS;
    x->p->skiptime = FL(0.0);
    strNcpy(x->p->sfname, name,  MAXSNDNAME-1);
    if ((infd = (csound->GetUtility(csound))->SndinGetSet(csound, x->p)) == 0) /*open sndfil, do skiptime*/
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
    MYFLT buffer[NUMBER_OF_SAMPLES];
    long  read_in;
    //    long  frames = 0;
    int32_t   block = 0;

    csound->SndfileSeek(csound, infd, x->sample, SEEK_CUR);
    while (x->numsamps>0) {
      int32_t num = NUMBER_OF_SAMPLES / x->outputs;
      if (x->numsamps < num)
        num = (int32_t) x->numsamps;
      x->numsamps -= num;
      read_in = csound->SndfileRead(csound, infd, buffer, num);
      csound->SndfileWrite(csound, outfd, buffer, read_in);
      block++;
      //frames += read_in;
      if (oparms->rewrt_hdr) {
        csound->SndfileCommand(csound,outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
        csound->SndfileSeek(csound, outfd, 0L, SEEK_END); /* Place at end again */
      }
      if (oparms->heartbeat) {
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%c\b", "|/-\\"[block&3]);
      }
      if (read_in < num) break;
    }
    csound->SndfileCommand(csound,outfd, SFC_UPDATE_HEADER_NOW, NULL, 0);
    return;
}

/* module interface */

int32_t xtrct_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "extractor", xtrct);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "extractor",
                                             Str("Extract part of a sound file"));
    }
    return retval;
}
