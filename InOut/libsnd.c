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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"                 /*             SNDLIB.C         */
#include "soundio.h"
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

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

#define STA(x)   (csound->libsndStatics.x)

static inline void alloc_globals(CSOUND *csound)
{
    csound->libsndStatics.nframes = (uint32)1;
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
    OPARMS  *O = csound->oparms;
    uint32_t   chn = 0;
    int     n;
    int spoutrem = csound->nspout;
    MYFLT   *sp = csound->spout;
    MYFLT   absamp = FL(0.0);
    uint32  nframes = csound->libsndStatics.nframes;
    MYFLT lim = O->limiter*csound->e0dbfs;
    MYFLT rlim = lim==0 ? 0 : FL(1.0)/lim;
    MYFLT k1 = FL(1.0)/TANH(FL(1.0)); /*  1.31304 */
 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) csound->libsndStatics.outbufrem) {
      n = (int) csound->libsndStatics.outbufrem;
    }
    spoutrem -= n;
    csound->libsndStatics.outbufrem -= n;
    do {
      // built inlimiter start ****
      // There is a rather awkward problem in reporting out of range not being
      // confused by the limited value but passing the clipped values to the
      // output.  Current solution is nasty and should be easier
      if (O->limiter) {
        MYFLT x = *sp;
        absamp = x;
        if (UNLIKELY(x>=lim))
          x = lim;
        else if (UNLIKELY(x<= -lim))
          x = -lim;
        else
          x = lim*k1*TANH(x*rlim);
        //printf("*** %g -> %g\n", *(sp-1), x);
        *sp++ = x;
        if (csound->libsndStatics.osfopen) {
          *csound->libsndStatics.outbufp++ = (x * csound->dbfs_to_float);
        }
      }
      // limiter end ****
      else {
        absamp = *sp++;
        if (csound->libsndStatics.osfopen) {
          *csound->libsndStatics.outbufp++ = (absamp * csound->dbfs_to_float);
        }
      }
      if (absamp < FL(0.0)) {
        absamp = -absamp;
      }
      if (absamp > csound->maxamp[chn]) {   /*  maxamp this seg  */
        csound->maxamp[chn] = absamp;
         csound->maxpos[chn] = nframes;
      }
      if (absamp > csound->e0dbfs) {        /* out of range?     */
        csound->rngcnt[chn]++;              /*  report it        */
        csound->rngflg = 1;
      }
      if (csound->multichan) {
        if (++chn >= csound->nchnls) {
            chn = 0;
            nframes++;
        }
      } else {
        nframes++;
      }
    } while (--n);
    if (!csound->libsndStatics.outbufrem) {
      if (csound->libsndStatics.osfopen) {
        csound->nrecs++;
        csound->audtran(csound, csound->libsndStatics.outbuf,
                        csound->libsndStatics.outbufsiz); /* Flush buffer */
        csound->libsndStatics.outbufp = (MYFLT*) csound->libsndStatics.outbuf;
      }
      csound->libsndStatics.outbufrem = csound->oparms_.outbufsamps;
      if (spoutrem) {
        goto nchk;
      }
    }
    csound->libsndStatics.nframes = nframes;
}

/* special version of spoutsf for "raw" floating point files */

static void spoutsf_noscale(CSOUND *csound)
{
    uint32_t chn = 0;
    int      n, spoutrem = csound->nspout;
    MYFLT    *sp = csound->spout;
    MYFLT    absamp = FL(0.0);
    uint32   nframes = csound->libsndStatics.nframes;

 nchk:
    /* if nspout remaining > buf rem, prepare to send in parts */
    if ((n = spoutrem) > (int) csound->libsndStatics.outbufrem)
      n = (int)csound->libsndStatics.outbufrem;
    spoutrem -= n;
    csound->libsndStatics.outbufrem -= n;
    do {
      absamp = *sp++;
      if (csound->libsndStatics.osfopen)
        *csound->libsndStatics.outbufp++ = absamp;
      if (absamp < FL(0.0))
        absamp = -absamp;
      if (absamp > csound->maxamp[chn]) {   /*  maxamp this seg  */
        csound->maxamp[chn] = absamp;
        csound->maxpos[chn] = nframes;
      }
      if (++chn >= csound->nchnls)
        chn = 0, nframes++;
    } while (--n);

    if (!csound->libsndStatics.outbufrem) {
      if (csound->libsndStatics.osfopen) {
        csound->nrecs++;
        csound->audtran(csound, csound->libsndStatics.outbuf,
                        csound->libsndStatics.outbufsiz); /* Flush buffer */
        csound->libsndStatics.outbufp = (MYFLT*) csound->libsndStatics.outbuf;
      }
      csound->libsndStatics.outbufrem = csound->oparms_.outbufsamps;
      if (spoutrem) goto nchk;
    }
    csound->libsndStatics.nframes = nframes;
}

/* diskfile write option for audtran's */
/*      assigned during sfopenout()    */

static void writesf(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;

    if (UNLIKELY(STA(outfile) == NULL))
      return;
    n = (int) sf_write_MYFLT(STA(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader((void *)STA(outfile));
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
          CS_SPRINTF(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
          if (n > 0) {
            memset(&(s[n]), '\b', n);
            s[n + n] = '\0';
            csound->MessageS(csound, CSOUNDMSG_REALTIME, "%s",  s);
          }
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%s", "\a");
        break;
    }
}

static void writesf_dither_16(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;
    int    dith;

    if (UNLIKELY(STA(outfile) == NULL))
      return;
    dith = STA(dither);
    for (n=0; n<m; n++) {
      int   tmp = ((dith * 15625) + 1) & 0xFFFF;
      int   rnd = ((tmp * 15625) + 1) & 0xFFFF;
      MYFLT result;
      dith = rnd;
      rnd = (rnd+tmp)>>1;           /* triangular distribution */
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7fff);
      buf[n] += result;
    }
    STA(dither) = dith;
    n = (int) sf_write_MYFLT(STA(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(STA(outfile));
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
          CS_SPRINTF(s, "%ld(%.3f)%n", (long) csound->nrecs,
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
    int dith;

    if (UNLIKELY(STA(outfile) == NULL))
      return;
    dith = STA(dither);
    for (n=0; n<m; n++) {
      int   tmp = ((dith * 15625) + 1) & 0xFFFF;
      int   rnd = ((tmp * 15625) + 1) & 0xFFFF;
      MYFLT result;
      dith = rnd;
      rnd = (rnd+tmp)>>1;           /* triangular distribution */
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7f);
      buf[n] += result;
    }
    STA(dither) = dith;
    n = (int) sf_write_MYFLT(STA(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(STA(outfile));
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
          CS_SPRINTF(s, "%ld(%.3f)%n", (long) csound->nrecs,
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
    int dith;

    if (UNLIKELY(STA(outfile) == NULL))
      return;
    dith = STA(dither);
    for (n=0; n<m; n++) {
      int   rnd = ((dith * 15625) + 1) & 0xFFFF;
      MYFLT result;
      dith =  rnd;
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7fff);
      buf[n] += result;
    }
    STA(dither) = dith;
    n = (int) sf_write_MYFLT(STA(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(STA(outfile));
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
          CS_SPRINTF(s, "%ld(%.3f)%n", (long) csound->nrecs,
                  csound->icurTime/csound->esr, &n);
          if (n > 0) {
            memset(&(s[n]), '\b', n);
            s[n + n] = '\0';
            csound->MessageS(csound, CSOUNDMSG_REALTIME, "%s",  s);
          }
        }
        break;
      case 4:
        csound->MessageS(csound, CSOUNDMSG_REALTIME, "%s",  "\a");
        break;
    }
}

static void writesf_dither_u8(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    OPARMS  *O = csound->oparms;
    int     n;
    int m = nbytes / sizeof(MYFLT);
    MYFLT *buf = (MYFLT*) outbuf;
    int dith;

    if (UNLIKELY(STA(outfile) == NULL))
      return;
    dith = STA(dither);
    for (n=0; n<m; n++) {
      int   rnd = ((dith * 15625) + 1) & 0xFFFF;
      MYFLT result;
      STA(dither) = rnd;
      result = (MYFLT) (rnd - 0x8000)  / ((MYFLT) 0x10000);
      result /= ((MYFLT) 0x7f);
      buf[n] += result;
    }
    STA(dither) = dith;
    n = (int) sf_write_MYFLT(STA(outfile), (MYFLT*) outbuf,
                             nbytes / sizeof(MYFLT)) * (int) sizeof(MYFLT);
    if (UNLIKELY(n < nbytes))
      sndwrterr(csound, n, nbytes);
    if (UNLIKELY(O->rewrt_hdr))
      rewriteheader(STA(outfile));
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
          CS_SPRINTF(s, "%ld(%.3f)%n", (long) csound->nrecs,
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
    i = (int) sf_read_MYFLT(STA(infile), inbuf, n);
    if (UNLIKELY(i < 0))
      return inbufsize;
    memset(&inbuf[i], 0, (n-i)*sizeof(MYFLT));
    return inbufsize;
}

/* Checks if the specified file name is a real-time audio device. */
/* Returns the device number (defaults to 1024) if it is, and -1 otherwise. */
/* If a device name is specified, and 'devName' is not NULL, a pointer to it */
/* is stored in *devName. */
/* Called from musmon, str_ops and here */

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
      if (devName != NULL) {
       *devName = &(s[1]);
      }
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

    if(csound->inchnls < 1)
       csound->Die(csound,
                 Str("error: cannot run input audio with nchnls_i=0"));


    alloc_globals(csound);
    STA(inbufrem) = (uint32) 0;    /* start with empty buffer */
    sfname = O->infilename;
    if (UNLIKELY(sfname == NULL || sfname[0] == '\0'))
      csound->Die(csound, Str("error: no input file name"));

    if (strcmp(sfname, "stdin") == 0) {
      STA(pipdevin) = 1;
    }
#ifdef PIPES
    else if (sfname[0] == '|') {
      STA(pin) = _popen(sfname + 1, "r");
      isfd = fileno(STA(pin));
      STA(pipdevin) = 1;
    }
#endif
    else {
      csRtAudioParams   parm;
      /* check for real time audio input, and get device name/number */
      parm.devNum = check_rtaudio_name(sfname, &(parm.devName), 0);
      if (parm.devNum >= 0) {
        /* set device parameters */
        parm.bufSamp_SW   =
          (unsigned int) O->inbufsamps / (unsigned int) csound->inchnls;
        parm.bufSamp_HW   = O->oMaxLag;
        parm.nChannels    = csound->inchnls;
        parm.sampleFormat = O->informat;
        parm.sampleRate   = (float) csound->esr;
        /* open devaudio for input */
        if (UNLIKELY(csound->recopen_callback(csound, &parm) != 0))
          csoundDie(csound, Str("Failed to initialise real time audio input"));
        /*  & redirect audio gets  */
        csound->audrecv = csound->rtrecord_callback;
        STA(pipdevin) = 2;       /* no backward seeks !     */
        goto inset;             /* no header processing    */
      }
    }
    /* open file */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    if (STA(pipdevin)) {
      STA(infile) = sf_open_fd(isfd, SFM_READ, &sfinfo, 0);
      if (UNLIKELY(STA(infile) == NULL)) {
        /* open failed: possibly raw file, but cannot seek back to try again */
        const char *sfError = Str(sf_strerror(NULL));
        csoundDie(csound, Str("isfinit: cannot open %s -- %s"), sfname, sfError);
      }
    }
    else {
      fullName = csoundFindInputFile(csound, sfname, "SFDIR;SSDIR");
      if (UNLIKELY(fullName == NULL))                     /* if not found */
        csoundDie(csound, Str("isfinit: cannot open %s"), sfname);
      STA(infile) = sf_open(fullName, SFM_READ, &sfinfo);
      if (STA(infile) == NULL) {
        /* open failed: maybe raw file ? */
        memset(&sfinfo, 0, sizeof(SF_INFO));
        sfinfo.samplerate = (int) MYFLT2LRND(csound->esr);
        sfinfo.channels = csound->nchnls;
        /* FIXME: assumes input sample format is same as output */
        sfinfo.format = TYPE2SF(TYP_RAW) | FORMAT2SF(O->outformat);
        STA(infile) = sf_open(fullName, SFM_READ, &sfinfo);  /* try again */
      }
      if (UNLIKELY(STA(infile) == NULL)) {
        const char *sfError = Str(sf_strerror(NULL));
        csoundDie(csound, Str("isfinit: cannot open %s -- %s"), fullName, sfError);
      }
      /* only notify the host if we opened a real file, not stdin or a pipe */
      csoundNotifyFileOpened(csound, fullName,
                              sftype2csfiletype(sfinfo.format), 0, 0);
      sfname = fullName;
    }
    /* chk the hdr codes  */
    if (sfinfo.samplerate != (int) MYFLT2LRND(csound->esr)) {
      csound->Warning(csound, Str("audio_in %s has sr = %d, orch sr = %d"),
                              sfname, (int) sfinfo.samplerate,
                              (int) MYFLT2LRND(csound->esr));
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
    STA(inbufsiz) = (unsigned) (O->inbufsamps * sizeof(MYFLT));
    STA(inbuf) = (MYFLT*) csound->Calloc(csound,
                                         STA(inbufsiz)); /* alloc inbuf space */
    if (STA(pipdevout) == 2) {
       csound->Message(csound,
                      Str("reading %d sample blks of %lu-bit floats from %s\n"),
                      O->inbufsamps * O->sfsampsize,
                      (unsigned long) sizeof(MYFLT)*8, sfname);
    }
    else {
       csound->Message(csound,
                      Str("reading %d-byte blks of %s from %s (%s)\n"),
                      O->inbufsamps * (int) sfsampsize(FORMAT2SF(O->informat)),
                      getstrformat(O->informat), sfname, type2string(fileType));
    }
    STA(isfopen) = 1;
}

static char* copyrightcode(int n)
{
      char* a[] = {
        "All Rights Reserved\n",
        "Creative Commons Attribution-NonCommercial-NoDerivatives\nCC BY-NC-ND\n)",
        "Creative Commons Attribution-NonCommercial-ShareAlike\nCC BY-NC-SA\n",
        "Creative Commons Attribution-NonCommercial\nCC BY-NC\n",
        "Creative Commons Attribution-NoDerivatives\nCC BY-ND\n",
        "Creative Commons Attribution-ShareAlike\nCC BY-SA\n",
        "Creative Commons Attribution\nCC BY\n",
        "Licenced under BSD\n"
      };
      if (n>=8) n = 0;
      return a[n];
}

void sfopenout(CSOUND *csound)                  /* init for sound out       */
{                                               /* (not called if nosound)  */
    OPARMS  *O = csound->oparms;
    char    *s, *fName, *fullName;
    SF_INFO sfinfo;
    int     osfd = 1;   /* stdout */

    alloc_globals(csound);
    if (O->outfilename == NULL) {
      switch (O->filetyp) {
      case TYP_WAV:
      case TYP_W64:
      case TYP_WAVEX:
      case TYP_RF64:
        O->outfilename = "test.wav";
        break;
      case TYP_AIFF:
        O->outfilename = "test.aif";
        break;
      case TYP_AU:
        O->outfilename = "test.au";
        break;
      case TYP_PAF:
        O->outfilename = "test.paf";
        break;
      case TYP_SVX:
        O->outfilename = "test.svx";
        break;
      case TYP_NIST:
        O->outfilename = "test.sph";
        break;
      case TYP_VOC:
        O->outfilename = "test.voc";
        break;
      /* case TYP_IRCAM: */
      /*   O->outfilename = ""; */
      /*   break; */
      /* case TYP_MAT4: */
      /*   O->outfilename = ""; */
      /*   break;  */
      /* case TYP_MAT5: */
      /*   O->outfilename = ""; */
      /*   break;  */
      /* case TYP_PVF: */
      /*   O->outfilename = ""; */
      /*   break;   */
      case TYP_XI:
        O->outfilename = "test.xi";
        break;
      /* case TYP_HTK: */
      /*   O->outfilename = ""; */
      /*   break;   */
      /* case TYP_SDS: */
      /*   O->outfilename = "test.sds"; */
      /*   break;   */
      case TYP_AVR:
        O->outfilename = "test.avr";
        break;
      case TYP_SD2:
        O->outfilename = "test.sd2";
        break;
      case TYP_FLAC:
        O->outfilename = "test.flac";
        break;
      case TYP_CAF:
        O->outfilename = "test.caf";
        break;
      case TYP_OGG:
        O->outfilename = "test.ogg";
        break;
      case TYP_MPEG:
        O->outfilename = "test.mp3";
        break;
      /* case TYP_MPC2K: */
      /*   O->outfilename = ""; */
      /*   break; */
      default:
        O->outfilename = "test";
        break;
      }
    }
    STA(sfoutname) = fName = O->outfilename;

    if (strcmp(fName, "stdout") == 0) {
      STA(pipdevout) = 1;
    }
#ifdef PIPES
    else if (fName[0] == '|') {
      STA(pout) = _popen(fName+1, "w");
      osfd = fileno(STA(pout));
      STA(pipdevout) = 1;
      if (O->filetyp == TYP_AIFF || O->filetyp == TYP_WAV) {
        char fmt_name[6];
        if (O->sfsampsize == 8) {
          strcpy(fmt_name, "AU");
          O->filetyp = TYP_AU;
        }
        else {
          strcpy(fmt_name, "IRCAM");
          O->filetyp = TYP_IRCAM;
        }
        csound->Message(csound, Str("Output file type changed to %s "
                                    "for use in pipe\n"), fmt_name);
      }
    }
#endif
    else {
      csRtAudioParams   parm;
      /* check for real time audio output, and get device name/number */
      parm.devNum = check_rtaudio_name(fName, &(parm.devName), 1);
      if (parm.devNum >= 0) {
        /* set device parameters */
        parm.bufSamp_SW   = (unsigned int) O->outbufsamps / csound->nchnls;
        parm.bufSamp_HW   = O->oMaxLag;
        parm.nChannels    = csound->nchnls;
        parm.sampleFormat = O->outformat;
        parm.sampleRate   = (float) csound->esr;
        csound->spoutran  = spoutsf;
        /* open devaudio for output */
        if (UNLIKELY(csound->playopen_callback(csound, &parm) != 0))
          csoundDie(csound, Str("Failed to initialise real time audio output"));
        /*  & redirect audio puts  */
        csound->audtran = csound->rtplay_callback;
        STA(outbufrem)  = parm.bufSamp_SW * parm.nChannels;
        STA(pipdevout)  = 2;      /* no backward seeks !   */
        if (O->realtime == 1)     /* set realtime priority mode */
          csound->realtime_audio_flag = 1;
        goto outset;              /* no header needed      */
      }
      else if (strcmp(fName, "null") == 0) {
        STA(outfile) = NULL;
        if (csound->dither_output && csound->oparms->outformat!=AE_FLOAT &&
            csound->oparms->outformat!=AE_DOUBLE) {
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
    //sfinfo.frames     = 0;
    sfinfo.samplerate = (int) MYFLT2LRND(csound->esr);
    sfinfo.channels   = csound->nchnls;
    sfinfo.format     = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
    /* open file */
    if (STA(pipdevout)) {
      STA(outfile) = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 0);
#ifdef PIPES
      if (STA(outfile) == NULL) {
        char fmt_name[6];
        if (O->sfsampsize == 8) {
          if (UNLIKELY(O->filetyp == TYP_AU))
            csoundDie(csound, Str("sfinit: cannot open fd %d\n%s"), osfd,
                      Str(sf_strerror(NULL)));
          strcpy(fmt_name, "AU");
          O->filetyp = TYP_AU;
        }
        else {
          if (UNLIKELY(O->filetyp == TYP_IRCAM))
            csoundDie(csound, Str("sfinit: cannot open fd %d\n%s"), osfd,
                      Str(sf_strerror(NULL)));
          strcpy(fmt_name, "IRCAM");
          O->filetyp = TYP_IRCAM;
        }
        csound->Message(csound, Str("Output file type changed to %s "
                                    "for use in pipe\n"), fmt_name);
        sfinfo.format = TYPE2SF(O->filetyp) | FORMAT2SF(O->outformat);
        STA(outfile) = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 0);
      }
#endif
      if (UNLIKELY(STA(outfile) == NULL))
        csoundDie(csound, Str("sfinit: cannot open fd %d\n%s"), osfd,
                  Str(sf_strerror(NULL)));
      sf_command(STA(outfile), SFC_SET_VBR_ENCODING_QUALITY,
                 &O->quality, sizeof(double));
    }
    else {
      fullName = csoundFindOutputFile(csound, fName, "SFDIR");
      if (UNLIKELY(fullName == NULL))
        csoundDie(csound, Str("sfinit: cannot open %s"), fName);
      STA(sfoutname) = fullName;
      STA(outfile)   = sf_open(fullName, SFM_WRITE, &sfinfo);
      if (UNLIKELY(STA(outfile) == NULL))
        csoundDie(csound, Str("sfinit: cannot open %s\n%s"),
                  fullName, sf_strerror (NULL));
      sf_command(STA(outfile), SFC_SET_VBR_ENCODING_QUALITY,
                 &O->quality, sizeof(double));
      /* only notify the host if we opened a real file, not stdout or a pipe */
      csoundNotifyFileOpened(csound, fullName,
                              type2csfiletype(O->filetyp, O->outformat), 1, 0);
    }

 #ifdef SNDFILE_MP3
    // VL: setting bitrate to constant improves quality
    if(O->filetyp == TYP_MPEG) {
      csound->Message(csound, "Setting MP3 bitrate to %s\n", csound->mp3_mode ? "variable" : "constant" );;
      sf_command(STA(outfile), SFC_SET_BITRATE_MODE,
                 &(csound->mp3_mode), sizeof(int));
    }
 #endif
    
    /* IV - Feb 22 2005: clip integer formats */
    if (O->outformat != AE_FLOAT && O->outformat != AE_DOUBLE)
      sf_command(STA(outfile), SFC_SET_CLIPPING, NULL, SF_TRUE);
    sf_command(STA(outfile), SFC_SET_ADD_PEAK_CHUNK,
               NULL, (csound->peakchunks ? SF_TRUE : SF_FALSE));
#ifdef SOME_FINE_DAY
    if (csound->dither_output) {        /* This may not be written yet!! */
      SF_DITHER_INFO  ditherInfo;
      memset(&ditherInfo, 0, sizeof(SF_DITHER_INFO));
      ditherInfo.type  = SFD_TRIANGULAR_PDF | SFD_DEFAULT_LEVEL;
      ditherInfo.level = 1.0;
      ditherInfo.name  = (char*) NULL;
      sf_command(STA(outfile), SFC_SET_DITHER_ON_WRITE,
                 &ditherInfo, sizeof(SF_DITHER_INFO));
    }
#endif
    if (!(O->outformat == AE_FLOAT || O->outformat == AE_DOUBLE) ||
        (O->filetyp == TYP_WAV || O->filetyp == TYP_AIFF ||
         O->filetyp == TYP_W64))
      csound->spoutran = spoutsf;       /* accumulate output */
    else
      csound->spoutran = spoutsf_noscale;
    if (csound->dither_output && csound->oparms->outformat!=AE_FLOAT &&
        csound->oparms->outformat!=AE_DOUBLE) {
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
      sf_set_string(STA(outfile), SF_STR_TITLE, s);
    if ((s = csound->SF_csd_licence) == NULL || *s == '\0')
      s = csound->SF_id_copyright;
    if (s != NULL && *s != '\0')
      sf_set_string(STA(outfile), SF_STR_COPYRIGHT, s);
    else if (csound->SF_id_scopyright>=0) {
      char buff[256];
      time_t tt = time(NULL);
      strftime(buff, 256, "Copyright %Y: ", gmtime(&tt));
      strncat(buff,copyrightcode(csound->SF_id_scopyright), 255);
      buff[255] = '\0';
      sf_set_string(STA(outfile), SF_STR_COPYRIGHT, buff);
    }
    if ((s = csound->SF_id_software) != NULL && *s != '\0')
      sf_set_string(STA(outfile), SF_STR_SOFTWARE, s);
    if ((s = csound->SF_id_artist) != NULL && *s != '\0')
      sf_set_string(STA(outfile), SF_STR_ARTIST, s);
    if ((s = csound->SF_id_comment) != NULL && *s != '\0')
      sf_set_string(STA(outfile), SF_STR_COMMENT, s);
    if ((s = csound->SF_id_date) != NULL && *s != '\0')
      sf_set_string(STA(outfile), SF_STR_DATE, s);
    /* file is now open */
    STA(osfopen) = 1;


 outset:
    O->sfsampsize = (int) sfsampsize(FORMAT2SF(O->outformat));
    /* calc outbuf size & alloc bufspace */
    STA(outbufsiz) = O->outbufsamps * sizeof(MYFLT);
    STA(outbufp)   = STA(outbuf) = csound->Malloc(csound, STA(outbufsiz));
    if (STA(pipdevout) == 2) {
      csound->Message(csound,
                      Str("writing %d sample blks of %lu-bit floats to %s\n"),
                      O->outbufsamps, (unsigned long) sizeof(MYFLT)*8,
                      STA(sfoutname));

    }
    else {
      csound->Message(csound, Str("writing %d-byte blks of %s to %s"),
                    O->outbufsamps * O->sfsampsize,
                    getstrformat(O->outformat), STA(sfoutname));

    if (O->sfheader == 0)
      csound->Message(csound, Str(" (raw)\n"));
    else
      csound->Message(csound, " (%s)\n", type2string(O->filetyp));
    }
    STA(osfopen)   = 1;
    STA(outbufrem) = O->outbufsamps;
}

void sfclosein(CSOUND *csound)
{
    alloc_globals(csound);
    if (!STA(isfopen))
      return;
    if (STA(pipdevin) == 2 && (!STA(osfopen) || STA(pipdevout) != 2)) {
      /* close only if not open for output too */
      csound->rtclose_callback(csound);
    }
    else if (STA(pipdevin) != 2) {
      if (STA(infile) != NULL)
        sf_close(STA(infile));
#ifdef PIPES
      if (STA(pin) != NULL) {
        _pclose(STA(pin));
        STA(pin) = NULL;
      }
#endif
      STA(infile) = NULL;
    }
    STA(isfopen) = 0;
}

void sfcloseout(CSOUND *csound)
{
    OPARMS  *O = csound->oparms;
    int     nb;

    alloc_globals(csound);
    if (!STA(osfopen))
      return;
    if ((nb = (O->outbufsamps - STA(outbufrem)) * sizeof(MYFLT)) > 0) {
      /* flush outbuffer */
      csound->nrecs++;
      csound->audtran(csound, STA(outbuf), nb);
    }
    if (STA(pipdevout) == 2 && (!STA(isfopen) || STA(pipdevin) != 2)) {
      /* close only if not open for input too */
      csound->rtclose_callback(csound);
    }
    if (STA(pipdevout) == 2)
      goto report;
    if (STA(outfile) != NULL) {
      if (!STA(pipdevout) && O->outformat != AE_VORBIS)
        sf_command(STA(outfile), SFC_UPDATE_HEADER_NOW, NULL, 0);
      sf_close(STA(outfile));
      STA(outfile) = NULL;
    }
#ifdef PIPES
    if (STA(pout) != NULL) {
      _pclose(STA(pout));
      STA(pout) = NULL;
    }
#endif

 report:
    if (STA(pipdevout) == 2) {
      csound->Message(csound,
                      "%"PRIi32" %d %s%lu%s%s\n",
                      csound->nrecs, O->outbufsamps, Str("sample blks of "),
                      (unsigned long)sizeof(MYFLT)*8,Str("-bit floats written to "),
                      STA(sfoutname));
    }
    else {
      csound->Message(csound, Str("%"PRIi32" %d sample blks of %s written to %s"),
                      O->outbufsamps, O->outbufsamps * O->sfsampsize,
                      getstrformat(O->outformat), STA(sfoutname));
      if (O->sfheader == 0)
        csound->Message(csound, Str(" (raw)\n"));
      else
        csound->Message(csound, " (%s)\n", type2string(O->filetyp));
      
    }
    STA(osfopen) = 0;
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
    STA(outbufrem) = csound->oparms->outbufsamps;  /* consider buf is flushed */
    sfcloseout(csound);                           /* & try to close the file */
    csound->Die(csound, Str("\t... closed\n"));
}

void sfnopenout(CSOUND *csound)
{
    alloc_globals(csound);
    csound->Message(csound, Str("not writing to sound disk\n"));
    
    /* init counter, though not writing */
    STA(outbufrem) = csound->oparms->outbufsamps;
}

static inline void sndfilein_(CSOUND *csound, MYFLT scaleFac)
{
    OPARMS  *O = csound->oparms;
    int     i, n, nsmps, bufpos;

    nsmps = csound->nspin;
    bufpos = (int) O->inbufsamps - (int) STA(inbufrem);
    for (i = 0; i<nsmps; i++) {
      if ((int) STA(inbufrem) < 1) {
        STA(inbufrem) = 0U;
        do {
          n = ((int) O->inbufsamps - (int) STA(inbufrem)) * (int) sizeof(MYFLT);
          n = csound->audrecv(csound, STA(inbuf) + (int) STA(inbufrem), n);
          STA(inbufrem) += (unsigned int) (n / (int) sizeof(MYFLT));
        } while ((int) STA(inbufrem) < (int) O->inbufsamps);
        bufpos = 0;
      }
      csound->spin[i] = STA(inbuf)[bufpos++] * scaleFac;
      STA(inbufrem)--;
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
    O               = csound->oparms;
    csound->audrecv = audrecv_dummy;
    csound->audtran = audtran_dummy;
    STA(inbufrem)   = (unsigned int) O->inbufsamps;
    STA(outbufrem)  = (unsigned int) O->outbufsamps;
    if (!csound->hostRequestedBufferSize) {
      O->sfread    = 0;
      O->sfwrite   = 0;
      STA(osfopen) = 0;
      return;
    }
    STA(inbufsiz)  = (unsigned int) (O->inbufsamps * (int) sizeof(MYFLT));
    STA(inbuf)     = (MYFLT*) csound->Calloc(csound, STA(inbufsiz));
    STA(outbufsiz) = (unsigned int) (O->outbufsamps * (int) sizeof(MYFLT));
    STA(outbuf)    = (MYFLT*) csound->Calloc(csound, STA(outbufsiz));
    STA(outbufp)   = STA(outbuf);
    O->sfread      = 1;
    O->sfwrite     = 1;
    STA(osfopen)   = 1;
}

PUBLIC MYFLT *csoundGetInputBuffer(CSOUND *csound)
{
    return STA(inbuf);
}

PUBLIC MYFLT *csoundGetOutputBuffer(CSOUND *csound)
{
    return STA(outbuf);
}
