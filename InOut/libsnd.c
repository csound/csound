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

#include "csoundCore.h"                 /*             SNDLIB.C         */
#include "soundio.h"
#include <stdlib.h>
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
    uint32        inbufrem;
    uint32        outbufrem;            /* in monosamps                 */
                                        /* (see openin, iotranset)      */
    unsigned int  inbufsiz,  outbufsiz; /* alloc in sfopenin/out        */
    int           isfopen;              /* (real set in sfopenin)       */
    int           osfopen;              /* (real set in sfopenout)      */
    int           pipdevin, pipdevout;  /* 0: file, 1: pipe, 2: rtaudio */
    uint32        nframes               /* = 1UL */;
    FILE          *pin, *pout;
#ifndef SOME_FILE_DAY
    int           dither;
#endif
} LIBSND_GLOBALS;

#ifdef PIPES
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) ||  \
     defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
extern  FILE *  _popen(const char *, const char *);
extern  int     _pclose(FILE *);
#endif

static  void    sndwrterr(CSOUND *, int, int);
static  void    sndfilein_noscale(CSOUND *csound);

#define ST(x)   (((LIBSND_GLOBALS*) csound->libsndGlobals)->x)

static void alloc_globals(CSOUND *csound)
{
    if (csound->libsndGlobals == NULL) {
      csound->libsndGlobals = csound->Calloc(csound, sizeof(LIBSND_GLOBALS));
      ST(nframes) = (uint32)1;
    }
}

/* The interface requires 2 functions:
   spoutran to transfer nspout items to buffer
   audtran to actually write the data

   spoutran is called with nchnls*ksamps items and this need to be
   buffered until outbufsiz items have been accumulated.  It will call
   audtran to flush when this happens.
*/

static void spoutsf(CSOUND *csound)
{
    int     n, chn = 0, spoutrem = csound->nspout;
    MYFLT   *sp = csound->spout;
    MYFLT   absamp;
    uint32  nframes = ST(nframes);

 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) ST(outbufrem))
      n = (int)ST(outbufrem);
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
        csound->maxpos[chn] = nframes;
      }
      if (absamp > csound->e0dbfs) {        /* out of range?     */
        csound->rngcnt[chn]++;              /*  report it        */
        csound->rngflg = 1;
      }
      if (csound->multichan) {
        if (++chn >= csound->nchnls)
          chn = 0, nframes++;
      }
      else
        nframes++;
    } while (--n);

    if (!ST(outbufrem)) {
      if (ST(osfopen)) {
        csound->nrecs++;
        csound->audtran(csound, ST(outbuf), ST(outbufsiz)); /* Flush buffer */
        ST(outbufp) = (MYFLT*) ST(outbuf);
      }
      ST(outbufrem) = csound->oparms_.outbufsamps;
      if (spoutrem) goto nchk;
    }
    ST(nframes) = nframes;
}

/* special version of spoutsf for "raw" floating point files */

static void spoutsf_noscale(CSOUND *csound)
{
    int     n, chn = 0, spoutrem = csound->nspout;
    MYFLT   *sp = csound->spout;
    MYFLT   absamp;
    uint32  nframes = ST(nframes);

 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) ST(outbufrem))
      n = (int)ST(outbufrem);
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
        csound->maxpos[chn] = nframes;
      }
      if (++chn >= csound->nchnls)
        chn = 0, nframes++;
    } while (--n);

    if (!ST(outbufrem)) {
      if (ST(osfopen)) {
        csound->nrecs++;
        csound->audtran(csound, ST(outbuf), ST(outbufsiz)); /* Flush buffer */
        ST(outbufp) = (MYFLT*) ST(outbuf);
      }
      ST(outbufrem) = csound->oparms_.outbufsamps;
      if (spoutrem) goto nchk;
    }
    ST(nframes) = nframes;
}

/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */

static void writesf(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;

    if (UNLIKELY(ST(outfile) == NULL))
      return;
    n = (int) sf_write_MYFLT(ST(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader((void *)ST(outfile));
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
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
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

static void writesf_dither_16(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;

    if (UNLIKELY(ST(outfile) == NULL))
      return;

    for (n=0; n<m; n++) {
        int   tmp = ((ST(dither) * 15625) + 1) & 0xFFFF;
        int   rnd = ((tmp * 15625) + 1) & 0xFFFF;
        MYFLT result;
        ST(dither) = rnd;
        rnd = (rnd+tmp)>>1;           /* triangular distribution */
        result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
        result /= ((MYFLT) 0x7fff);
        buf[n] += result;
    }
    n = (int) sf_write_MYFLT(ST(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(ST(outfile));
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
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
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

static void writesf_dither_8(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;

    if (UNLIKELY(ST(outfile) == NULL))
      return;

    for (n=0; n<m; n++) {
      int   tmp = ((ST(dither) * 15625) + 1) & 0xFFFF;
      int   rnd = ((tmp * 15625) + 1) & 0xFFFF;
      MYFLT result;
      ST(dither) = rnd;
      rnd = (rnd+tmp)>>1;           /* triangular distribution */
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7f);
      buf[n] += result;
    }
    n = (int) sf_write_MYFLT(ST(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(ST(outfile));
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
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
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

static void writesf_dither_u16(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;

    if (UNLIKELY(ST(outfile) == NULL))
      return;

    for (n=0; n<m; n++) {
        int   rnd = ((ST(dither) * 15625) + 1) & 0xFFFF;
        MYFLT result;
        ST(dither) = rnd;
        result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
        result /= ((MYFLT) 0x7fff);
        buf[n] += result;
    }
    n = (int) sf_write_MYFLT(ST(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(ST(outfile));
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
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
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

static void writesf_dither_u8(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;

    if (UNLIKELY(ST(outfile) == NULL))
      return;

    for (n=0; n<m; n++) {
      int   rnd = ((ST(dither) * 15625) + 1) & 0xFFFF;
      MYFLT result;
      ST(dither) = rnd;
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7f);
      buf[n] += result;
    }
    n = (int) sf_write_MYFLT(ST(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(ST(outfile));
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
          sprintf(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
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

static int readsf(CSOUND *csound, MYFLT *inbuf, int inbufsize)
{
    int i, n;

    (void) csound;
    n = inbufsize / (int) sizeof(MYFLT);
    i = (int) sf_read_MYFLT(ST(infile), inbuf, n);
    if (UNLIKELY(i < 0))
      return inbufsize;
    memset(&inbuf[i], 0, (n-i)*sizeof(MYFLT));
    /* while (i < n) */
    /*   inbuf[i++] = FL(0.0); */
    return inbufsize;
}

/* Checks if the specified file name is a real-time audio device. */
/* Returns the device number (defaults to 1024) if it is, and -1 otherwise. */
/* If a device name is specified, and 'devName' is not NULL, a pointer to it */
/* is stored in *devName. */

int check_rtaudio_name(char *fName, char **devName, int isOutput)
{
    char  *s;

    if (devName != NULL)
      *devName = (char*) NULL;
    if (fName == NULL)
      return -1;
    s = fName;
    if ((isOutput && strncmp(fName, "dac", 3) == 0) ||
        (!isOutput && strncmp(fName, "adc", 3) == 0))
      s += 3;
    else if (strncmp(fName, "devaudio", 8) == 0)
      s += 8;
    else
      return -1;
    if (*s == (char) '\0')
      return 1024;
    if (*s == (char) ':') {
      if (devName != NULL)
        *devName = &(s[1]);
      return 1024;
    }
    else {
      int devNum = 0;
      while (*s >= (char) '0' && *s <= (char) '9') {
        devNum = devNum * 10 + ((int) *s - (int) '0');
        if (devNum >= 1024)
          break;
        if (*(++s) == (char) '\0')
          return devNum;
      }
    }
    return -1;
}

void sfopenin(CSOUND *csound)           /* init for continuous soundin */
{
    OPARMS  *O = csound->oparms;
    char    *sfname, *fullName;
    SF_INFO sfinfo;
    int     fileType = (int) TYP_RAW;
    int     isfd = 0;   /* stdin */

    alloc_globals(csound);
    ST(inbufrem) = (uint32) 0;    /* start with empty buffer */
    sfname = O->infilename;
    if (UNLIKELY(sfname == NULL || sfname[0] == '\0'))
      csound->Die(csound, Str("error: no input file name"));

    if (strcmp(sfname, "stdin") == 0) {
      ST(pipdevin) = 1;
    }
#ifdef PIPES
    else if (sfname[0] == '|') {
      ST(pin) = _popen(sfname + 1, "r");
      isfd = fileno(ST(pin));
      ST(pipdevin) = 1;
    }
#endif
    else {
      csRtAudioParams   parm;
      /* check for real time audio input, and get device name/number */
      parm.devNum = check_rtaudio_name(sfname, &(parm.devName), 0);
      if (parm.devNum >= 0) {
        /* set device parameters */
        parm.bufSamp_SW = (int) O->inbufsamps / (int) csound->inchnls;
        parm.bufSamp_HW = O->oMaxLag;
        parm.nChannels = csound->nchnls;
        parm.sampleFormat = O->informat;
        parm.sampleRate = (float) csound->esr;
        /* open devaudio for input */
        if (UNLIKELY(csound->recopen_callback(csound, &parm) != 0))
          csoundDie(csound, Str("Failed to initialise real time audio input"));
        /*  & redirect audio gets  */
        csound->audrecv = csound->rtrecord_callback;
        ST(pipdevin) = 2;       /* no backward seeks !     */
        goto inset;             /* no header processing    */
      }
    }
    /* open file */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    if (ST(pipdevin)) {
      ST(infile) = sf_open_fd(isfd, SFM_READ, &sfinfo, 0);
      if (UNLIKELY(ST(infile) == NULL)) {
        /* open failed: possibly raw file, but cannot seek back to try again */
        csoundDie(csound, Str("isfinit: cannot open %s"), sfname);
      }
    }
    else {
      fullName = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
      if (UNLIKELY(fullName == NULL))                     /* if not found */
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
      if (UNLIKELY(ST(infile) == NULL))
        csoundDie(csound, Str("isfinit: cannot open %s"), fullName);
      /* only notify the host if we opened a real file, not stdin or a pipe */
      csoundNotifyFileOpened(csound, fullName,
                              sftype2csfiletype(sfinfo.format), 0, 0);
      sfname = fullName;
    }
    /* chk the hdr codes  */
    if (sfinfo.samplerate != (int) (csound->esr + FL(0.5))) {
      csound->Warning(csound, Str("audio_in %s has sr = %d, orch sr = %d"),
                              sfname, (int) sfinfo.samplerate,
                              (int) (csound->esr + FL(0.5)));
    }
    if (sfinfo.channels != csound->inchnls) {
      csound->Warning(csound, Str("audio_in %s has %d chnls, orch %d chnls_i"),
                              sfname, (int) sfinfo.channels, csound->inchnls);
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
    /* calc inbufsize reqd */
    ST(inbufsiz) = (unsigned) (O->inbufsamps * sizeof(MYFLT));
    ST(inbuf) = (MYFLT*) mcalloc(csound, ST(inbufsiz)); /* alloc inbuf space */
    if (ST(pipdevout) == 2)
      csound->Message(csound,
                      Str("reading %d sample blks of %d-bit floats from %s \n"),
                      O->inbufsamps * O->sfsampsize, sizeof(MYFLT)*8, sfname);
    else {
    csound->Message(csound,
                    Str("reading %d-byte blks of %s from %s (%s)\n"),
                    O->inbufsamps * (int) sfsampsize(FORMAT2SF(O->informat)),
                    getstrformat(O->informat), sfname, type2string(fileType));
    }
    ST(isfopen) = 1;
}

void sfopenout(CSOUND *csound)                  /* init for sound out       */
{                                               /* (not called if nosound)  */
    OPARMS  *O = csound->oparms;
    char    *s, *fName, *fullName;
    SF_INFO sfinfo;
    int     osfd = 1;   /* stdout */

    alloc_globals(csound);
    if (O->outfilename == NULL) {
      if (O->filetyp == TYP_WAV)
        O->outfilename = "test.wav";
      else if (O->filetyp == TYP_AIFF)
        O->outfilename = "test.aif";
      else if (O->filetyp == TYP_AU)
        O->outfilename = "test.au";
      else
        O->outfilename = "test";
    }
    ST(sfoutname) = fName = O->outfilename;

    if (strcmp(fName, "stdout") == 0) {
      ST(pipdevout) = 1;
    }
#ifdef PIPES
    else if (fName[0] == '|') {
      ST(pout) = _popen(fName+1, "w");
      osfd = fileno(ST(pout));
      ST(pipdevout) = 1;
      if (O->filetyp == TYP_AIFF || O->filetyp == TYP_WAV) {
        csound->Message(csound, Str("Output file type changed to IRCAM "
                                    "for use in pipe\n"));
        O->filetyp = TYP_IRCAM;
      }
    }
#endif
    else {
      csRtAudioParams   parm;
      /* check for real time audio output, and get device name/number */
      parm.devNum = check_rtaudio_name(fName, &(parm.devName), 1);
      if (parm.devNum >= 0) {
        /* set device parameters */
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
        csound->audtran = csound->rtplay_callback;
        ST(outbufrem) = parm.bufSamp_SW * parm.nChannels;
        ST(pipdevout) = 2;      /* no backward seeks !   */
        goto outset;            /* no header needed      */
      }
      else if (strcmp(fName, "null") == 0) {
        ST(outfile) = NULL;
        if (csound->dither_output && csound->oparms->outformat!=AE_FLOAT) {
          if (csound->oparms->outformat==AE_SHORT)
            if (csound->dither_output==1)
              csound->audtran = writesf_dither_16;
            else
              csound->audtran = writesf_dither_u16;
          else if (csound->oparms->outformat==AE_CHAR)
            if (csound->dither_output==1)
              csound->audtran = writesf_dither_8;
            else
              csound->audtran = writesf_dither_u8;
          else
            csound->audtran = writesf;
        }
        else
          csound->audtran = writesf;
        goto outset;
      }
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
      if (UNLIKELY(fullName == NULL))
        csoundDie(csound, Str("sfinit: cannot open %s"), fName);
      ST(sfoutname) = fullName;
      ST(outfile) = sf_open(fullName, SFM_WRITE, &sfinfo);
      if (UNLIKELY(ST(outfile) == NULL))
        csoundDie(csound, Str("sfinit: cannot open %s"), fullName);
      /* only notify the host if we opened a real file, not stdout or a pipe */
      csoundNotifyFileOpened(csound, fullName,
                              type2csfiletype(O->filetyp, O->outformat), 1, 0);
    }
    /* IV - Feb 22 2005: clip integer formats */
    if (O->outformat != AE_FLOAT && O->outformat != AE_DOUBLE)
      sf_command(ST(outfile), SFC_SET_CLIPPING, NULL, SF_TRUE);
    sf_command(ST(outfile), SFC_SET_ADD_PEAK_CHUNK,
               NULL, (csound->peakchunks ? SF_TRUE : SF_FALSE));
#ifdef SOME_FINE_DAY
    if (csound->dither_output) {        /* This may not be written yet!! */
      SF_DITHER_INFO  ditherInfo;
      memset(&ditherInfo, 0, sizeof(SF_DITHER_INFO));
      ditherInfo.type = SFD_TRIANGULAR_PDF | SFD_DEFAULT_LEVEL;
      ditherInfo.level = 1.0;
      ditherInfo.name = (char*) NULL;
      sf_command(ST(outfile), SFC_SET_DITHER_ON_WRITE,
                 &ditherInfo, sizeof(SF_DITHER_INFO));
    }
#endif
    if (!(O->outformat == AE_FLOAT || O->outformat == AE_DOUBLE) ||
        (O->filetyp == TYP_WAV || O->filetyp == TYP_AIFF ||
         O->filetyp == TYP_W64))
      csound->spoutran = spoutsf;       /* accumulate output */
    else
      csound->spoutran = spoutsf_noscale;
    if (csound->dither_output && csound->oparms->outformat!=AE_FLOAT) {
      if (csound->oparms->outformat==AE_SHORT)
        csound->audtran = writesf_dither_16;
      else if (csound->oparms->outformat==AE_CHAR)
        csound->audtran = writesf_dither_8;
      else
        csound->audtran = writesf;
    }
    else
      csound->audtran = writesf;
    /* Write any tags. */
    if ((s = csound->SF_id_title) != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_TITLE, s);
    if ((s = csound->SF_csd_licence) == NULL || *s == '\0')
      s = csound->SF_id_copyright;
    if (s != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_COPYRIGHT, s);
    if ((s = csound->SF_id_software) != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_SOFTWARE, s);
    if ((s = csound->SF_id_artist) != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_ARTIST, s);
    if ((s = csound->SF_id_comment) != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_COMMENT, s);
    if ((s = csound->SF_id_date) != NULL && *s != '\0')
      sf_set_string(ST(outfile), SF_STR_DATE, s);
    /* file is now open */
    ST(osfopen) = 1;

 outset:
    O->sfsampsize = (int) sfsampsize(FORMAT2SF(O->outformat));
    /* calc outbuf size & alloc bufspace */
    ST(outbufsiz) = O->outbufsamps * sizeof(MYFLT);
    ST(outbufp) = ST(outbuf) = mmalloc(csound, ST(outbufsiz));
    if (ST(pipdevout) == 2)
      csound->Message(csound,
                      Str("writing %d sample blks of %d-bit floats to %s \n"),
                      O->outbufsamps, sizeof(MYFLT)*8, ST(sfoutname));
    else {
     csound->Message(csound, Str("writing %d-byte blks of %s to %s"),
                    O->outbufsamps * O->sfsampsize,
                    getstrformat(O->outformat), ST(sfoutname));

    if (O->sfheader == 0)
      csound->Message(csound, Str(" (raw)\n"));
    else
      csound->Message(csound, " (%s)\n", type2string(O->filetyp));
    }
    ST(osfopen) = 1;
    ST(outbufrem) = O->outbufsamps;
}

void sfclosein(CSOUND *csound)
{
    alloc_globals(csound);
    if (!ST(isfopen))
      return;
    if (ST(pipdevin) == 2 && (!ST(osfopen) || ST(pipdevout) != 2)) {
      /* close only if not open for output too */
      csound->rtclose_callback(csound);
    }
    else if (ST(pipdevin) != 2) {
      if (ST(infile) != NULL)
        sf_close(ST(infile));
#ifdef PIPES
      if (ST(pin) != NULL) {
        _pclose(ST(pin));
        ST(pin) = NULL;
      }
#endif
      ST(infile) = NULL;
    }
    ST(isfopen) = 0;
}

void sfcloseout(CSOUND *csound)
{
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
      _pclose(ST(pout));
      ST(pout) = NULL;
    }
#endif

 report:
    if (ST(pipdevout) == 2) {
      csound->Message(csound,
                      Str("%ld %d sample blks of %d-bit floats written to %s\n"),
                      csound->nrecs, O->outbufsamps,
                      sizeof(MYFLT)*8, ST(sfoutname));
    }
    else {
      csound->Message(csound, Str("%ld %d sample blks of %s written to %s"),
                      O->outbufsamps, O->outbufsamps * O->sfsampsize,
                      getstrformat(O->outformat), ST(sfoutname));
      if (O->sfheader == 0)
        csound->Message(csound, Str(" (raw)\n"));
      else
        csound->Message(csound, " (%s)\n", type2string(O->filetyp));
    }
    ST(osfopen) = 0;
}

/* report soundfile write(osfd) error   */
/* called after chk of write() bytecnt  */

static void sndwrterr(CSOUND *csound, int nret, int nput)
{
    csound->ErrorMsg(csound,
                     Str("soundfile write returned bytecount of %d, not %d"),
                     nret, nput);
    csound->ErrorMsg(csound,
                     Str("(disk may be full...\n closing the file ...)"));
    ST(outbufrem) = csound->oparms->outbufsamps;  /* consider buf is flushed */
    sfcloseout(csound);                           /* & try to close the file */
    csound->Die(csound, Str("\t... closed\n"));
}

void sfnopenout(CSOUND *csound)
{
    alloc_globals(csound);
    csound->Message(csound, Str("not writing to sound disk\n"));
    /* init counter, though not writing */
    ST(outbufrem) = csound->oparms->outbufsamps;
}

static inline void sndfilein_(CSOUND *csound, MYFLT scaleFac)
{
    OPARMS  *O = csound->oparms;
    int     i, n, nsmps, bufpos;

    nsmps = csound->nspin;
    bufpos = (int) O->inbufsamps - (int) ST(inbufrem);
    for (i = 0; i<nsmps; i++) {
      if ((int) ST(inbufrem) < 1) {
        ST(inbufrem) = 0U;
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

static void sndfilein(CSOUND *csound)
{
    sndfilein_(csound, csound->e0dbfs);
}

/* special version of sndfilein for "raw" floating point files */

static void sndfilein_noscale(CSOUND *csound)
{
    sndfilein_(csound, FL(1.0));
}

static int audrecv_dummy(CSOUND *csound, MYFLT *buf, int nbytes)
{
    (void) csound; (void) buf;
    return nbytes;
}

static void audtran_dummy(CSOUND *csound, const MYFLT *buf, int nbytes)
{
    (void) csound; (void) buf; (void) nbytes;
}

/* direct recv & tran calls to the right audio formatter  */
/*                            & init its audio_io bufptr  */

void iotranset(CSOUND *csound)
{
    OPARMS  *O;

    csound->spinrecv = sndfilein;
    csound->spoutran = spoutsf;
    if (!csound->enableHostImplementedAudioIO)
      return;
    alloc_globals(csound);
    O = csound->oparms;
    csound->audrecv = audrecv_dummy;
    csound->audtran = audtran_dummy;
    ST(inbufrem) = (unsigned int) O->inbufsamps;
    ST(outbufrem) = (unsigned int) O->outbufsamps;
    if (!csound->hostRequestedBufferSize) {
      O->sfread = 0;
      O->sfwrite = 0;
      ST(osfopen) = 0;
      return;
    }
    ST(inbufsiz) = (unsigned int) (O->inbufsamps * (int) sizeof(MYFLT));
    ST(inbuf) = (MYFLT*) mcalloc(csound, ST(inbufsiz));
    ST(outbufsiz) = (unsigned int) (O->outbufsamps * (int) sizeof(MYFLT));
    ST(outbuf) = (MYFLT*) mcalloc(csound, ST(outbufsiz));
    ST(outbufp) = ST(outbuf);
    O->sfread = 1;
    O->sfwrite = 1;
    ST(osfopen) = 1;
}

PUBLIC MYFLT *csoundGetInputBuffer(CSOUND *csound)
{
    if (csound->libsndGlobals == NULL)
      return NULL;
    return ST(inbuf);
}

PUBLIC MYFLT *csoundGetOutputBuffer(CSOUND *csound)
{
    if (csound->libsndGlobals == NULL)
      return NULL;
    return ST(outbuf);
}
