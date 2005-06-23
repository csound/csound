/*
    sndlib.c:

    Copyright (C) 2004 John ffitch

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"                         /*             SNDLIB.C         */
#include "soundio.h"
#include <sndfile.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

typedef struct {
    SNDFILE       *outfile;
    SNDFILE       *infile;
    char          *sfoutname;           /* soundout filename            */
    MYFLT         *inbuf;
    MYFLT         *outbuf;              /* contin sndio buffers         */
    MYFLT         *outbufp;             /* MYFLT pntr                   */
    unsigned int  inbufrem;
    unsigned int  outbufrem;            /* in monosamps                 */
                                        /* (see openin,iotranset)       */
    unsigned int  inbufsiz,  outbufsiz; /* alloc in sfopenin/out        */
    int           isfopen;              /* (real set in sfopenin)       */
    int           osfopen;              /* (real set in sfopenout)      */
    int           pipdevin, pipdevout;  /* 0: file, 1: pipe, 2: rtaudio */
    unsigned long nframes /* = 1UL */;
    FILE          *pin, *pout;
} LIBSND_GLOBALS;

#ifdef PIPES
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) ||  \
     defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif

extern  char    *getstrformat(int format);
extern  char    *type2string(int);
extern  short   sfsampsize(int);
static  void    sndwrterr(void*, int, int);
static  void    sndfilein_noscale(void *csound);

#define ST(x)   (((LIBSND_GLOBALS*) ((ENVIRON*) csound)->libsndGlobals)->x)

static void alloc_globals(ENVIRON *csound)
{
    if (csound->libsndGlobals == NULL) {
      csound->libsndGlobals = csound->Calloc(csound, sizeof(LIBSND_GLOBALS));
      ST(nframes) = 1UL;
    }
}

/* The interface requires 2 functions:
   spoutran to transfer nspout items to buffer
   audtran to actually write the data

   spoutran is called with nchnls*ksamps items and this need to be
   buffered until outbufsiz items have been accumulated.  It will call
   audtran to flush when this happens.
*/

static void spoutsf(void *csound_)
{
    ENVIRON       *csound = (ENVIRON*) csound_;
    int           n, chn = 0, spoutrem = csound->nspout;
    MYFLT         *sp = csound->spout;
    MYFLT         absamp;

 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) ST(outbufrem))
      n = ST(outbufrem);
    spoutrem -= n;
    ST(outbufrem) -= n;
    do {
      absamp = *sp++;
      if (ST(osfopen))
        *ST(outbufp)++ = absamp * csound->dbfs_to_float;
      if (absamp < FL(0.0))
        absamp = -absamp;
      if (absamp > csound->maxamp[chn]) {   /*  maxamp this seg  */
        csound->maxamp[chn] = absamp;
        csound->maxpos[chn] = ST(nframes);
      }
      if (absamp > csound->e0dbfs) {        /* out of range?     */
        csound->rngcnt[chn]++;              /*  report it        */
        csound->rngflg = 1;
      }
      if (csound->multichan) {
        if (++chn >= csound->nchnls)
          chn = 0, ST(nframes)++;
      }
      else
        ST(nframes)++;
    } while (--n);

    if (!ST(outbufrem)) {
      if (ST(osfopen)) {
        csound->nrecs++;
        csound->audtran(csound, ST(outbuf), ST(outbufsiz)); /* Flush buffer */
        ST(outbufp) = (MYFLT*) ST(outbuf);
      }
      ST(outbufrem) = csound->oparms->outbufsamps;
      if (spoutrem) goto nchk;
    }
}

/* special version of spoutsf for "raw" floating point files */

static void spoutsf_noscale(void *csound_)
{
    ENVIRON       *csound = (ENVIRON*) csound_;
    int           n, chn = 0, spoutrem = csound->nspout;
    MYFLT         *sp = csound->spout;
    MYFLT         absamp;

 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) ST(outbufrem))
      n = ST(outbufrem);
    spoutrem -= n;
    ST(outbufrem) -= n;
    do {
      absamp = *sp++;
      if (ST(osfopen))
        *ST(outbufp)++ = absamp;
      if (absamp < FL(0.0))
        absamp = -absamp;
      if (absamp > csound->maxamp[chn]) {   /*  maxamp this seg  */
        csound->maxamp[chn] = absamp;
        csound->maxpos[chn] = ST(nframes);
      }
      if (++chn >= csound->nchnls)
        chn = 0, ST(nframes)++;
    } while (--n);

    if (!ST(outbufrem)) {
      if (ST(osfopen)) {
        csound->nrecs++;
        csound->audtran(csound, ST(outbuf), ST(outbufsiz)); /* Flush buffer */
        ST(outbufp) = (MYFLT*) ST(outbuf);
      }
      ST(outbufrem) = csound->oparms->outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void writesf(void *csound_, MYFLT *outbuf, int nbytes)
{                               /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
    ENVIRON *csound = (ENVIRON*) csound_;
    OPARMS  *O = csound->oparms;
    int     n;

    if (ST(outfile) == NULL)
      return;
    n = (int) sf_write_MYFLT(ST(outfile), outbuf, nbytes / sizeof(MYFLT))
        * (int) sizeof(MYFLT);
    if (n < nbytes)
      sndwrterr(csound, n, nbytes);
    if (O->rewrt_hdr)
      rewriteheader(ST(outfile), 0);
    switch (O->heartbeat) {
      case 1:
        csound->MessageS(csound, CSOUNDMSG_REALTIME,
                                 "%c\010", "|/-\\"[csound->nrecs & 3]);
        break;
      case 2:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, ".");
        break;
      case 3:
        {
          char    s[512];
          double  curTime = csound->sensEvents_state.curTime;
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs, curTime, &n);
          if (n > 0) {
            memset(&(s[n]), '\b', n);
            s[n + n] = '\0';
            csound->MessageS(csound, CSOUNDMSG_REALTIME, "%s", s);
          }
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "\a");
        break;
    }
}

static int readsf(void *csound, MYFLT *inbuf_, int inbufsize)
{
    int i, n;

    csound = csound;
    n = inbufsize / (int) sizeof(MYFLT);
    i = (int) sf_read_MYFLT(ST(infile), inbuf_, n);
    if (i < 0)
      i = 0;
    while (i < n)
      inbuf_[i++] = FL(0.0);
    return inbufsize;
}

void writeheader(ENVIRON *csound, int ofd, char *ofname)
{
    sf_command(ST(outfile), SFC_UPDATE_HEADER_NOW, NULL, 0);
}

void sfopenin(void *csound_)        /* init for continuous soundin */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    OPARMS  *O = csound->oparms;
    char    *sfname, *fullName;
    SF_INFO sfinfo;
    int     fileType = (int) TYP_RAW;
    int     isfd = 0;   /* stdin */

    alloc_globals(csound);
    ST(inbufrem) = (unsigned int) 0;    /* start with empty buffer */
    sfname = O->infilename;
    if (sfname == NULL || sfname[0] == '\0')
      csound->Die(csound, Str("error: no input file name"));
    csound->rtin_dev = 1024;
    csound->rtin_devs = NULL;

    if (strcmp(sfname, "stdin") == 0) {
      ST(pipdevin) = 1;
    }
#ifdef PIPES
    else if (sfname[0] == '|') {
      FILE *_popen(const char *, const char *);
      ST(pin) = _popen(sfname + 1, "r");
      isfd = fileno(ST(pin));
      ST(pipdevin) = 1;
    }
#endif
    else if (!strncmp(sfname, "devaudio", 8) || !strncmp(sfname, "adc", 3)) {
      csRtAudioParams   parm;
      /* get device name/number */
      if (!strncmp(sfname, "devaudio", 8) && sfname[8] != '\0') {
        if (sfname[8] == ':')   csound->rtin_devs = &(sfname[9]);
        else                    sscanf(sfname + 8, "%d", &(csound->rtin_dev));
      }
      else if (!strncmp(sfname, "adc", 3) && sfname[3] != '\0') {
        if (sfname[3] == ':')   csound->rtin_devs = &(sfname[4]);
        else                    sscanf(sfname + 3, "%d", &(csound->rtin_dev));
      }
      /* set device parameters (should get these from ENVIRON...) */
      parm.devName = csound->rtin_devs;
      parm.devNum = csound->rtin_dev;
      parm.bufSamp_SW = (int) O->inbufsamps / (int) csound->nchnls;
      parm.bufSamp_HW = O->oMaxLag;
      parm.nChannels = csound->nchnls;
      parm.sampleFormat = O->informat;
      parm.sampleRate = (float) csound->esr;
      /* open devaudio for input */
      if (csound->recopen_callback(csound, &parm) != 0)
        csoundDie(csound, Str("Failed to initialise real time audio input"));
      /*  & redirect audio gets  */
      csound->audrecv = (int (*)(void*, MYFLT*, int)) csound->rtrecord_callback;
      ST(pipdevin) = 2;         /* no backward seeks !     */
      goto inset;               /* no header processing    */
    }
    /* open file */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    if (ST(pipdevin)) {
      ST(infile) = sf_open_fd(isfd, SFM_READ, &sfinfo, 0);
      if (ST(infile) == NULL) {
        /* open failed: possibly raw file, but cannot seek back to try again */
        csoundDie(csound, Str("isfinit: cannot open %s"), sfname);
      }
    }
    else {
      fullName = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
      if (fullName == NULL)                     /* if not found */
        csoundDie(csound, Str("isfinit: cannot open %s"), sfname);
      ST(infile) = sf_open(fullName, SFM_READ, &sfinfo);
      if (ST(infile) == NULL) {
        /* open failed: maybe raw file ? */
        memset(&sfinfo, 0, sizeof(SF_INFO));
        sfinfo.samplerate = (int) (csound->esr + FL(0.5));
        sfinfo.channels = csound->nchnls;
        /* FIXME: assumes input sample format is same as output */
        sfinfo.format = TYPE2SF(TYP_RAW) | FORMAT2SF(O->outformat);
        ST(infile) = sf_open(fullName, SFM_READ, &sfinfo);  /* try again */
      }
      if (ST(infile) == NULL)
        csoundDie(csound, Str("isfinit: cannot open %s"), fullName);
      sfname = fullName;
    }
    /* chk the hdr codes  */
    if (sfinfo.samplerate != (int) (csound->esr + FL(0.5))) {
      csound->Warning(csound, Str("audio_in %s has sr = %d, orch sr = %d"),
                              sfname, (int) sfinfo.samplerate,
                              (int) (csound->esr + FL(0.5)));
    }
    if (sfinfo.channels != csound->nchnls) {
      csound->Warning(csound, Str("audio_in %s has %d chnls, orch %d chnls"),
                              sfname, (int) sfinfo.channels, csound->nchnls);
    }
    /* Do we care about the format?  Can assume float?? */
    O->informat = SF2FORMAT(sfinfo.format);
    fileType = (int) SF2TYPE(sfinfo.format);
    csound->audrecv = readsf;           /* will use standard audio gets  */
    if ((O->informat == AE_FLOAT || O->informat == AE_DOUBLE) &&
        !(fileType == TYP_WAV || fileType == TYP_AIFF || fileType == TYP_W64)) {
      /* do not scale "raw" floating point files */
      csound->spinrecv = sndfilein_noscale;
    }

 inset:
    O->insampsiz = (int) sfsampsize(FORMAT2SF(O->informat));
    /* calc inbufsize reqd */
    ST(inbufsiz) = (unsigned) (O->inbufsamps * sizeof(MYFLT));
    ST(inbuf) = (MYFLT*) mcalloc(csound, ST(inbufsiz)); /* alloc inbuf space */
    csound->Message(csound, Str("reading %d-byte blks of %s from %s (%s)\n"),
                            O->inbufsamps * O->insampsiz,
                            getstrformat(O->informat), sfname,
                            type2string(fileType));
    ST(isfopen) = 1;
}

void sfopenout(void *csound_)                   /* init for sound out       */
{                                               /* (not called if nosound)  */
    ENVIRON *csound = (ENVIRON*) csound_;
    OPARMS  *O = csound->oparms;
    char    *s, *fName, *fullName;
    SF_INFO sfinfo;
    int     osfd = 1;   /* stdout */

    alloc_globals(csound);
    if (O->outfilename == NULL) {
      if (O->filetyp == TYP_WAV) O->outfilename = "test.wav";
      else if (O->filetyp == TYP_AIFF) O->outfilename = "test.aif";
      else if (O->filetyp == TYP_AU) O->outfilename = "test.au";
      else O->outfilename = "test";
    }
    ST(sfoutname) = fName = O->outfilename;
    csound->rtout_dev = 1024;
    csound->rtout_devs = NULL;
    if (strcmp(fName, "stdout") == 0) {
      ST(pipdevout) = 1;
    }
#ifdef PIPES
    else if (fName[0] == '|') {
      FILE *_popen(const char *, const char *);
      ST(pout) = _popen(fName+1, "w");
      osfd = fileno(ST(pout));
      ST(pipdevout) = 1;
      if (O->filetyp == TYP_AIFF || O->filetyp == TYP_WAV) {
        csound->Message(csound,
                        Str("Output file type changed to IRCAM "
                            "for use in pipe\n"));
        O->filetyp = TYP_IRCAM;
      }
    }
#endif
    else if (!strncmp(fName, "devaudio", 8) || !strncmp(fName, "dac", 3)) {
      csRtAudioParams   parm;
      /* get device number/name */
      if (!strncmp(fName, "devaudio", 8) && fName[8] != '\0') {
        if (fName[8] == ':')    csound->rtout_devs = &(fName[9]);
        else                    sscanf(fName + 8, "%d", &(csound->rtout_dev));
      }
      else if (!strncmp(fName, "dac", 3) && fName[3] != '\0') {
        if (fName[3] == ':')    csound->rtout_devs = &(fName[4]);
        else                    sscanf(fName + 3, "%d", &(csound->rtout_dev));
      }
      /* set device parameters (should get these from ENVIRON...) */
      parm.devName = csound->rtout_devs;
      parm.devNum = csound->rtout_dev;
      parm.bufSamp_SW = (int) O->outbufsamps / (int) csound->nchnls;
      parm.bufSamp_HW = O->oMaxLag;
      parm.nChannels = csound->nchnls;
      parm.sampleFormat = O->outformat;
      parm.sampleRate = (float) csound->esr;
      csound->spoutran = spoutsf;
      /* open devaudio for output */
      if (csound->playopen_callback(csound, &parm) != 0)
        csoundDie(csound, Str("Failed to initialise real time audio output"));
      /*  & redirect audio puts  */
      csound->audtran = (void (*)(void*, MYFLT*, int)) csound->rtplay_callback;
      ST(outbufrem) = parm.bufSamp_SW * parm.nChannels;
      ST(pipdevout) = 2;                        /* no backward seeks !   */
      goto outset;                              /* no header needed      */
    }
    else if (strcmp(fName, "null") == 0) {
      ST(outfile) = NULL;
      csound->audtran = writesf;
      goto outset;
    }
    /* set format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = csound->nchnls;
    sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
    /* open file */
    if (ST(pipdevout)) {
      ST(outfile) = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 0);
    }
    else {
      fullName = csoundFindOutputFile(csound, fName, "SFDIR");
      if (fullName == NULL)
        csoundDie(csound, Str("sfinit: cannot open %s"), fName);
      ST(sfoutname) = fullName;
      ST(outfile) = sf_open(fullName, SFM_WRITE, &sfinfo);
      if (ST(outfile) == NULL)
        csoundDie(csound, Str("sfinit: cannot open %s"), fullName);
    }
    /* IV - Feb 22 2005: clip integer formats */
    if (O->outformat != AE_FLOAT && O->outformat != AE_DOUBLE)
      sf_command(ST(outfile), SFC_SET_CLIPPING, NULL, SF_TRUE);
    if (csound->peakchunks)
      sf_command(ST(outfile), SFC_SET_ADD_PEAK_CHUNK, NULL, SF_TRUE);
    if (csound->dither_output)          /* This may not be written yet!! */
      sf_command(ST(outfile), SFC_SET_DITHER_ON_WRITE, NULL, SF_TRUE);
    if (!(O->outformat == AE_FLOAT || O->outformat == AE_DOUBLE) ||
        (O->filetyp == TYP_WAV || O->filetyp == TYP_AIFF ||
         O->filetyp == TYP_W64))
      csound->spoutran = spoutsf;       /* accumulate output */
    else
      csound->spoutran = spoutsf_noscale;
    csound->audtran = writesf;          /* flush buffer */
    /* Write any tags. */
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_title");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_TITLE, s);
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::csd_licence");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_COPYRIGHT, s);
    else {
      s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_copyright");
      if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_COPYRIGHT, s);
    }
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_software");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_SOFTWARE, s);
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_artist");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_ARTIST, s);
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_comment");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_COMMENT, s);
    s = (char*) csound->QueryGlobalVariable(csound, "::SF::id_date");
    if (s && *s != '\0') sf_set_string(ST(outfile), SF_STR_DATE, s);
    /* file is now open */
    ST(osfopen) = 1;

 outset:
    O->sfsampsize = (int) sfsampsize(FORMAT2SF(O->outformat));
    /* calc outbuf size & alloc bufspace */
    ST(outbufsiz) = O->outbufsamps * sizeof(MYFLT);
    ST(outbufp) = ST(outbuf) = mmalloc(csound, ST(outbufsiz));
    csound->Message(csound,Str("writing %d-byte blks of %s to %s"),
                    O->outbufsamps * O->sfsampsize,
                    getstrformat(O->outformat), ST(sfoutname));
    if (ST(pipdevout) == 2)
      /* realtime output has no header */
      csound->Message(csound, "\n");
    else if (O->sfheader == 0)
      csound->Message(csound, Str(" (raw)\n"));
    else
      csound->Message(csound, " (%s)\n", type2string(O->filetyp));
    ST(osfopen) = 1;
    ST(outbufrem) = O->outbufsamps;
}

void sfclosein(void *csound_)
{
    ENVIRON *csound = (ENVIRON*) csound_;

    alloc_globals(csound);
    if (!ST(isfopen))
      return;
    if (ST(pipdevin) == 2 && (!ST(osfopen) || ST(pipdevout) != 2)) {
      /* close only if not open for output too */
      extern int rtrecord_dummy(void *csound, void *inBuf, int nBytes);
      /* make sure that rtrecord does not get called after */
      /* closing the device, by replacing it with the dummy function */
      csound->audrecv = (int (*)(void*, MYFLT*, int)) rtrecord_dummy;
      ((ENVIRON*) csound)->rtclose_callback(csound);
    }
    else if (ST(pipdevin) != 2) {
      if (ST(infile) != NULL)
        sf_close(ST(infile));
#ifdef PIPES
      if (ST(pin) != NULL) {
        int _pclose(FILE*);
        _pclose(ST(pin));
        ST(pin) = NULL;
      }
#endif
      ST(infile) = NULL;
    }
    ST(isfopen) = 0;
}

void sfcloseout(void *csound_)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    OPARMS  *O = csound->oparms;
    int     nb;

    alloc_globals(csound);
    if (!ST(osfopen))
      return;
    if ((nb = (O->outbufsamps - ST(outbufrem)) * sizeof(MYFLT)) > 0) {
      /* flush outbuffer */
      csound->nrecs++;
      csound->audtran(csound, ST(outbuf), nb);
    }
    if (ST(pipdevout) == 2 && (!ST(isfopen) || ST(pipdevin) != 2)) {
      /* close only if not open for input too */
      extern void rtplay_dummy(void *csound, void *outBuf, int nBytes);
      /* make sure that rtplay does not get called after */
      /* closing the device, by replacing it with the dummy function */
      csound->audtran = (void (*)(void*, MYFLT*, int)) rtplay_dummy;
      csound->rtclose_callback(csound);
    }
    if (ST(pipdevout) == 2)
      goto report;
    if (ST(outfile) != NULL) {
      if (!ST(pipdevout))
        sf_command(ST(outfile), SFC_UPDATE_HEADER_NOW, NULL, 0);
      sf_close(ST(outfile));
      ST(outfile) = NULL;
    }
#ifdef PIPES
    if (ST(pout) != NULL) {
      int _pclose(FILE*);
      _pclose(ST(pout));
      ST(pout) = NULL;
    }
#endif

 report:
    csound->Message(csound, Str("%ld %d-byte soundblks of %s written to %s"),
                    csound->nrecs, O->outbufsamps * O->sfsampsize,
                    getstrformat(O->outformat), ST(sfoutname));
    if (ST(pipdevout) == 2)
      /* realtime output has no header */
      csound->Message(csound, "\n");
    else if (O->sfheader == 0)
      csound->Message(csound, Str(" (raw)\n"));
    else
      csound->Message(csound, " (%s)\n", type2string(O->filetyp));
    ST(osfopen) = 0;
}

static void sndwrterr(void *csound, int nret, int nput)
{                                   /* report soundfile write(osfd) error   */
    ENVIRON *p = (ENVIRON*) csound; /* called after chk of write() bytecnt  */
    p->Message(csound,
               Str("soundfile write returned bytecount of %d, not %d\n"),
               nret, nput);
    p->Message(csound, Str("(disk may be full...\n closing the file ...)\n"));
    ST(outbufrem) = p->oparms->outbufsamps;     /* consider buf is flushed */
    sfcloseout(csound);                         /* & try to close the file */
    csoundDie(csound, Str("\t... closed\n"));
}

void sfnopenout(ENVIRON *csound)
{
    alloc_globals(csound);
    csound->Message(csound, Str("not writing to sound disk\n"));
    /* init counter, though not writing */
    ST(outbufrem) = csound->oparms->outbufsamps;
}

static inline void sndfilein_(ENVIRON *csound, MYFLT scaleFac)
{
    OPARMS  *O = csound->oparms;
    int     i, n, nsmps, bufpos;

    nsmps = csound->ksmps * csound->nchnls;
    bufpos = (int) O->inbufsamps - (int) ST(inbufrem);
    for (i = 0; i < nsmps; i++) {
      if ((int) ST(inbufrem) < 1) {
        ST(inbufrem) = (unsigned) 0;
        do {
          n = ((int) O->inbufsamps - (int) ST(inbufrem)) * (int) sizeof(MYFLT);
          n = csound->audrecv(csound, ST(inbuf) + (int) ST(inbufrem), n);
          ST(inbufrem) += (unsigned int) (n / (int) sizeof(MYFLT));
        } while ((int) ST(inbufrem) < (int) O->inbufsamps);
        bufpos = 0;
      }
      csound->spin[i] = ST(inbuf)[bufpos++] * scaleFac;
      ST(inbufrem)--;
    }
}

static void sndfilein(void *csound)
{
    sndfilein_((ENVIRON*) csound, ((ENVIRON*) csound)->e0dbfs);
}

/* special version of sndfilein for "raw" floating point files */

static void sndfilein_noscale(void *csound)
{
    sndfilein_((ENVIRON*) csound, FL(1.0));
}

/* direct recv & tran calls to the right audio formatter  */
/*                            & init its audio_io bufptr  */

void iotranset(ENVIRON *csound)
{
    csound->spinrecv = sndfilein;
    csound->spoutran = spoutsf;
}

PUBLIC void *csoundGetInputBuffer(void *csound)
{
    if (((ENVIRON*) csound)->libsndGlobals == NULL)
      return NULL;
    return ST(inbuf);
}

PUBLIC void *csoundGetOutputBuffer(void *csound)
{
    if (((ENVIRON*) csound)->libsndGlobals == NULL)
      return NULL;
    return ST(outbuf);
}

