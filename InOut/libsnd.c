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

#include "cs.h"                         /*             SNDLIB.C       */
#include "soundio.h"
#include <sndfile.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

static  SNDFILE *outfile;
static  SNDFILE *infile;
static  char    *sfoutname;                     /* soundout filename    */
        MYFLT   *inbuf = NULL;
        MYFLT   *outbuf = NULL;                 /* contin sndio buffers */
static  MYFLT   *outbufp;                       /* MYFLT pntr           */
static  unsigned inbufrem;
static  unsigned outbufrem;                     /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd;
        int     isfopen = 0, infilend = 0;      /* (real set in sfopenin)    */
static  int     osfd;
        int     osfopen = 0;                    /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
unsigned long   nframes = 1;

#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */

#ifdef PIPES
FILE *pin, *pout;
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif

extern  void    (*spinrecv)(void*), (*spoutran)(void*);
extern  void    (*nzerotran)(void*, long);
int     (*audrecv)(void*, MYFLT*, int);
void    (*audtran)(void*, MYFLT*, int);

extern  char    *getstrformat(int format);
static  void    sndwrterr(void*, unsigned, unsigned);
extern  unsigned long   nframes;

extern char* type2string(int);
extern short sfsampsize(int);

extern  int     openin(char*);

/* return sample size (in bytes) of format 'fmt' */

static int format_nbytes(int fmt)
{
    switch (fmt) {
      case AE_SHORT:    return 2;
      case AE_24INT:    return 3;
      case AE_LONG:
      case AE_FLOAT:    return 4;
      case AE_DOUBLE:   return 8;
    }
    return 1;
}

/* The interface requires 3 functions:
   spoutran to transfer nspout items to buffer
   nzerotran to transfer nspout zeros to buffer
   audtran to actually write the data

   spoutran is called with nchnls*ksamps items and this need to be
   buffered until outbufsiz items have been accumulated.  It will call
   audtran to flush when this happens.
*/

void spoutsf(void *csound_)
{
    ENVIRON       *csound = (ENVIRON*) csound_;
    int           n, spoutrem = nspout;
    MYFLT         *maxampp = maxamp;
    unsigned long *maxps = maxpos;
    long          *rngp;                /* RWD Nov 2001 */
    MYFLT         *sp = spout;
    MYFLT         absamp;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;                     /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < FL(0.0))
        absamp = -absamp;
      if (absamp > *maxampp) {           /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      absamp = *sp;
      if (absamp >= 0) {                 /* +ive samp:   */
        if (absamp > csound->e0dbfs) {   /* out of range?     */
          /*   report it*/
          rngp = csound->rngcnt + (maxampp - maxamp);
          (*rngp)++;
          csound->rngflg = 1;
        }
      }
      else {                             /* ditto -ive samp */
        if (absamp < -(csound->e0dbfs)) {
          rngp = csound->rngcnt + (maxampp - maxamp);
          (*rngp)++;
          csound->rngflg = 1;
        }
      }
      absamp *= csound->dbfs_to_float;
      if (osfopen)
        *outbufp++ = absamp;
      if (csound->multichan) {
        maxps++;
        if (++maxampp >= maxampend)
          maxampp = maxamp, maxps = maxpos, nframes++;
      }
      else nframes++;
      sp++;
    } while (--n);
    if (!outbufrem) {
      if (osfopen) {
        csound->nrecs++;
        audtran(csound, outbuf, outbufsiz); /* Flush buffer */
        outbufp = (MYFLT *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

void zerosf(void *csound, long len)
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)len;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem) /* if smps remaining > buf rem, */
      n = outbufrem;                    /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;                    /* clear buf only till clean */
      do {
        *outbufp++ = FL(0.0);
      } while (--n);
    }
    else outbufp += n;
    if (!outbufrem) {
      ((ENVIRON*) csound)->nrecs++;
      audtran(csound, outbuf, outbufsiz);   /* Flush */
      outbufp = (MYFLT*)outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

static void writesf(void *csound_, MYFLT *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    int     n;
    if (osfd<0) return;
    n = sf_write_MYFLT(outfile, outbuf, nbytes/sizeof(MYFLT));
    if (n < nbytes/sizeof(MYFLT))
      sndwrterr(csound, n, nbytes);
    if (O.rewrt_hdr)
      rewriteheader(outfile,0);
    if (O.heartbeat) {
      if (O.heartbeat==1) {
#ifdef SYMANTEC
        nextcurs();
#else
        putc("|/-\\"[csound->nrecs & 3], stderr); putc(8,stderr);
#endif
      }
      else if (O.heartbeat==2) putc('.', stderr);
      else if (O.heartbeat==3) {
        int n;
        err_printf("%d(%.3f)%n", csound->nrecs, csound->nrecs/csound->ekr, &n);
        while (n--) err_printf("\b");
      }
      else err_printf("\a");
    }
}

static int readsf(void *csound, MYFLT *inbuf_, int inbufsize)
{
    int i, n;

    csound = csound;
    n = inbufsize / (int) sizeof(MYFLT);
    i = (int) sf_read_MYFLT(infile, inbuf_, n);
    if (i < 0)
      i = 0;
    while (i < n)
      inbuf_[i++] = FL(0.0);
    return inbufsize;
}

void writeheader(int ofd, char *ofname)
{
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

int sndinset(ENVIRON *csound, SOUNDIN_ *p) /* init routine for instr soundin */
                            /* shares above sndgetset with SAsndgetset, gen01*/
{
    SNDFILE *sinfd;

    if (p->fdch.fd != NULL) {           /* if file already open, close it */
      if (*p->skipinit != FL(0.0))
        return OK;
      /* reload the file */
      fdclose(&(p->fdch));
    }
    if (*p->ifilno == SSTRCOD) {                /* if char string name given */
      if (p->STRARG == NULL)
        strcpy(p->sndin_.sfname, unquote(currevent->strarg));
      else
        strcpy(p->sndin_.sfname, unquote(p->STRARG)); /* unquote it, else use */
    }
    else {
      long filno = (long) ((double) *p->ifilno + 0.5);
      if (filno >= 0 && filno <= strsmax && strsets != NULL && strsets[filno])
        strcpy(p->sndin_.sfname, strsets[filno]);
      else
        sprintf(p->sndin_.sfname, "soundin.%ld", filno);  /* soundin.filno */
    }
    switch ((int) (*p->iformat + FL(0.5))) {
      case 0: p->sndin_.format = AE_SHORT; break;
      case 1: p->sndin_.format = AE_CHAR;  break;
      case 2: p->sndin_.format = AE_ALAW;  break;
      case 3: p->sndin_.format = AE_ULAW;  break;
      case 4: p->sndin_.format = AE_SHORT; break;
      case 5: p->sndin_.format = AE_LONG;  break;
      case 6: p->sndin_.format = AE_FLOAT; break;
      default:
        csound->InitError(csound, Str("soundin: invalid sample format: %d"),
                                  (int) (*p->iformat + FL(0.5)));
        return NOTOK;
    }
    p->sndin_.outocount = (int) p->OUTOCOUNT;
    p->sndin_.channel = ALLCHNLS;           /* reading all channels      */
    p->sndin_.analonly = p->sndin_.endfile = p->sndin_.do_floatscaling = 0;
    p->sndin_.skiptime = *(p->iskptim);
    if ((sinfd = sndgetset(csound, &(p->sndin_))) != NULL) {
      /* if soundinset successful: */
      p->fdch.fd = sinfd;                   /*    store & log the fd     */
      fdrecord(&p->fdch);                   /*    instr will close later */
    }
    else {
      csound->inerrcnt_++;
      return NOTOK;
    }
    return OK;
}

int soundin(ENVIRON *csound, SOUNDIN_ *p)
{
    MYFLT       *r[24], scalefac;
    int         nsmps, ntogo, blksiz, chnsout, i = 0, n;

    if (p->sndin_.format == AE_FLOAT) {
      if (p->sndin_.filetyp == TYP_WAV || p->sndin_.filetyp == TYP_AIFF) {
        scalefac = csound->e0dbfs;
        if (p->sndin_.do_floatscaling)
          scalefac *= p->sndin_.fscalefac;
      }
      else
        scalefac = FL(1.0);
    }
    else
      scalefac = csound->e0dbfs;
    if (!p->sndin_.inbufp) {
      return csound->PerfError(csound, Str("soundin: not initialised"));
    }
    chnsout = p->sndin_.outocount;
    blksiz = chnsout * csound->ksmps;
    memcpy(r, p->r, chnsout * sizeof(MYFLT*));
    ntogo = blksiz;
    if (p->sndin_.endfile)
      goto filend;
    nsmps = (p->sndin_.bufend - p->sndin_.inbufp);
    if (nsmps > blksiz)
      nsmps = blksiz;
    ntogo -= nsmps;
 sndin:
    {
      MYFLT *inbufp = p->sndin_.inbufp;
      do {
        *(r[i]++) = *inbufp++ * scalefac;
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
      p->sndin_.inbufp = inbufp;
    }
    if (p->sndin_.inbufp >= p->sndin_.bufend) {
      if ((n = sreadin(csound, p->fdch.fd, p->sndin_.inbuf, SNDINBUFSIZ, p))
          == 0) {
        p->sndin_.endfile = 1;
        if (ntogo) goto filend;
        else return OK;
      }
      p->sndin_.inbufp = p->sndin_.inbuf;
      p->sndin_.bufend = p->sndin_.inbuf + n;
      if (ntogo > 0) {
        if ((nsmps = n)  > ntogo)
          nsmps = ntogo;
        ntogo -= nsmps;
        goto sndin;
      }
    }
    return OK;

 filend:
    if ((nsmps = ntogo) > 0) {            /* At RWD's suggestion  */
      do {                                /* if past end of file, */
        *(r[i]++) = FL(0.0);              /*    move in zeros     */
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
    }
    return OK;
}

void sfopenin(void *csound_)        /* init for continuous soundin */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    char    *sfname = NULL;
    int     fileType = (int) TYP_RAW;

    inbufrem = (unsigned int) 0;    /* start with empty buffer */

    if (O.infilename != NULL && strcmp(O.infilename,"stdin") == 0) {
      sfname = O.infilename;
      isfd = 0;         /* get sound from stdin if requested */
      pipdevin = 1;
    }
#ifdef PIPES
    else if (O.infilename != NULL && O.infilename[0]=='|') {
      FILE *_popen(const char *, const char *);
      pin = _popen(O.infilename+1, "r");
      isfd = fileno(pin);
      pipdevin = 1;
    }
#endif
    else if (O.infilename != NULL &&
             (strncmp(O.infilename, "devaudio", 8) == 0 ||
              strncmp(O.infilename, "adc", 3) == 0)) {
      if (strcmp(O.infilename, "devaudio") == 0 ||
          strcmp(O.infilename, "adc") == 0) {
        csound->rtin_dev = 1024;
        csound->rtin_devs = NULL;
      }
      else if (strncmp(O.infilename, "devaudio", 8) == 0) {
        if (O.infilename[8]==':') {
          csound->rtin_dev = 1024;
          csound->rtin_devs = &(O.infilename[9]);
        }
        else {
          sscanf(O.infilename+8, "%d", &(csound->rtin_dev));
          csound->rtin_devs = NULL;
        }
      }
      else if (strncmp(O.infilename,"adc", 3) == 0) {
        if (O.infilename[3]==':') {
          csound->rtin_dev = 1024;
          csound->rtin_devs = &(O.infilename[4]);
        }
        else {
          sscanf(O.infilename+3, "%d", &(csound->rtin_dev));
          csound->rtin_devs = NULL;
        }
      }
      sfname = O.infilename;
      {
        csRtAudioParams parm;
        /* set device parameters (should get these from ENVIRON...) */
        parm.devName = csound->rtin_devs;
        parm.devNum = csound->rtin_dev;
        parm.bufSamp_SW = (int) O.inbufsamps / (int) csound->nchnls;
        parm.bufSamp_HW = O.oMaxLag;
        parm.nChannels = csound->nchnls;
        parm.sampleFormat = O.informat;
        parm.sampleRate = (float) csound->esr;
        /* open devaudio for input */
        if (csound->recopen_callback(csound, &parm) != 0)
          csoundDie(csound, Str("Failed to initialise real time audio input"));
        /*  & redirect audio gets  */
        audrecv = (int (*)(void*, MYFLT*, int)) csound->rtrecord_callback;
      }
      isfd = DEVAUDIO;                    /* dummy file descriptor   */
      pipdevin   = 1;                     /* no backward seeks !     */
      goto inset;                         /* no header processing    */
    }
    else {                      /* else build filename and open that */
      SF_INFO sfinfo;
      if ((isfd = openin(O.infilename)) < 0)
        csoundDie(csound, Str("isfinit: cannot open %s"), retfilnam);
      sfname = retfilnam;
      memset(&sfinfo, 0, sizeof(SF_INFO));
      infile = sf_open_fd(isfd, SFM_READ, &sfinfo, SF_TRUE);
      if (infile == NULL)
        csoundDie(csound, Str("isfinit: cannot open %s"), retfilnam);
      if (sfinfo.samplerate != (long) (csound->esr + FL(0.5)) &&
          (O.msglevel & WARNMSG)) {              /*    chk the hdr codes  */
        printf(Str("WARNING: audio_in %s has sr = %ld, orch sr = %ld\n"),
               sfname, sfinfo.samplerate, (long) (csound->esr + FL(0.5)));
      }
      if (sfinfo.channels != csound->nchnls) {
        csoundDie(csound, Str("audio_in %s has %ld chnls, orch %d chnls"),
                          sfname, sfinfo.channels, csound->nchnls);
      }
      /* Do we care about the format?  Can assume float?? */
      O.insampsiz = sizeof(MYFLT);        /*    & cpy header vals  */
      O.informat = SF2FORMAT(sfinfo.format);
      fileType = (int) SF2TYPE(sfinfo.format);
      audrecv = readsf;  /* will use standard audio gets  */
    }

 inset:
    /* calc inbufsize reqd */
    inbufsiz = (unsigned) (O.inbufsamps * sizeof(MYFLT));
    inbuf = (MYFLT *)mcalloc(csound, inbufsiz); /* alloc inbuf space */
    printf(Str("reading %d-byte blks of %s from %s (%s)\n"),
           O.inbufsamps * format_nbytes(O.informat),
           getstrformat(O.informat), sfname,
           type2string(fileType));
    isfopen = 1;
}

void sfopenout(void *csound_)                   /* init for sound out       */
{                                               /* (not called if nosound)  */
    ENVIRON *csound = (ENVIRON*) csound_;

    if (O.outfilename == NULL) {
      if (O.filetyp == TYP_WAV) O.outfilename = "test.wav";
      else if (O.filetyp == TYP_AIFF) O.outfilename = "test.aif";
      else if (O.filetyp == TYP_AU) O.outfilename = "test.au";
      else O.outfilename = "test";
    }
    if (strcmp(O.outfilename,"stdout") == 0) {
      sfoutname = O.outfilename;
      osfd = O.stdoutfd;              /* send sound to stdout if requested */
      pipdevout = 1;
    }
#ifdef PIPES
    else if (O.outfilename != NULL && O.outfilename[0]=='|') {
      FILE *_popen(const char *, const char *);
      pout = _popen(O.outfilename+1, "w");
      osfd = fileno(pout);
      pipdevout = 1;
      if (O.filetyp == TYP_AIFF || O.filetyp == TYP_WAV) {
        printf(Str("Output file type changed to IRCAM for use in pipe\n"));
        O.filetyp = TYP_IRCAM;
      }
    }
#endif
    else if (O.outfilename != NULL &&
             (strncmp(O.outfilename, "devaudio", 8) == 0 ||
              strncmp(O.outfilename, "dac", 3) == 0)) {
      if (strcmp(O.outfilename, "devaudio") == 0 ||
          strcmp(O.outfilename, "dac") == 0) {
        csound->rtout_dev = 1024;
        csound->rtout_devs = NULL;
      }
      else if (strncmp(O.outfilename,"devaudio", 8) ==0) {
        if (O.outfilename[8]==':') {
          csound->rtout_dev = 1024;
          csound->rtout_devs = &(O.outfilename[9]);
        }
        else {
          sscanf(O.outfilename+8, "%d", &(csound->rtout_dev));
          csound->rtout_devs = NULL;
        }
      }
      else if (strncmp(O.outfilename,"dac", 3) == 0) {
        if (O.outfilename[3]==':') {
          csound->rtout_dev = 1024;
          csound->rtout_devs = &(O.outfilename[4]);
        }
        else {
          sscanf(O.outfilename+3, "%d", &(csound->rtout_dev));
          csound->rtout_devs = NULL;
        }
      }
      sfoutname = O.outfilename;
      {
        csRtAudioParams parm;
        /* set device parameters (should get these from ENVIRON...) */
        parm.devName = csound->rtout_devs;
        parm.devNum = csound->rtout_dev;
        parm.bufSamp_SW = (int) O.outbufsamps / (int) csound->nchnls;
        parm.bufSamp_HW = O.oMaxLag;
        parm.nChannels = csound->nchnls;
        parm.sampleFormat = O.outformat;
        parm.sampleRate = (float) csound->esr;
        spoutran = spoutsf;
        nzerotran = zerosf;
        /* open devaudio for output */
        if (csound->playopen_callback(csound, &parm) != 0)
          csoundDie(csound, Str("Failed to initialise real time audio output"));
        /*  & redirect audio puts  */
        audtran = (void (*)(void*, MYFLT*, int)) csound->rtplay_callback;
        outbufrem = parm.bufSamp_SW * parm.nChannels;
      }
      osfd = DEVAUDIO;                         /* dummy file descriptor */
      pipdevout = 1;                           /* no backward seeks !   */
      goto outset;                             /* no header needed      */
    }
    else if (strcmp(O.outfilename,"null") == 0) {
      osfd = -1;
      sfoutname = mmalloc(csound, (long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
    }
    else {  /* else open sfdir or cwd */
      SF_INFO sfinfo;
      sfinfo.frames = -1;
      sfinfo.samplerate = (int) (csound->esr + FL(0.5));
      sfinfo.channels = csound->nchnls;
      sfinfo.format = TYPE2SF(O.filetyp) | FORMAT2SF(O.outformat);
      sfinfo.sections = 0;
      sfinfo.seekable = 0;
      if ((osfd = openout(O.outfilename, 3)) < 0)
        csoundDie(csound, Str("sfinit: cannot open %s"), retfilnam);
      sfoutname = mmalloc(csound, (long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
      outfile = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 1);
      if (outfile == NULL)
        csoundDie(csound, Str("sfinit: cannot open %s"), sfoutname);
      /* IV - Feb 22 2005: clip integer formats */
      if (O.outformat != AE_FLOAT && O.outformat != AE_DOUBLE)
        sf_command(outfile, SFC_SET_CLIPPING, NULL, SF_TRUE);
      if (peakchunks)
        sf_command(outfile, SFC_SET_ADD_PEAK_CHUNK, NULL, SF_TRUE);
      if (strcmp(sfoutname, "/dev/audio") == 0) {
        /*      ioctl(   );   */
        pipdevout = 1;
      }
      if (dither_output)        /* This may not be written yet!! */
        sf_command(outfile, SFC_SET_DITHER_ON_WRITE, NULL, SF_TRUE);
      spoutran = spoutsf;       /* accumulate output */
      nzerotran = zerosf;       /* quick zeros */
      audtran = writesf;        /* flush buffer */
      /* Write any tags. */
      {
        ENVIRON *p;
        char    *s;
        p = (ENVIRON*) csound;
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_title"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_TITLE, s);
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_copyright"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_COPYRIGHT, s);
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_software"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_SOFTWARE, s);
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_artist"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_ARTIST, s);
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_comment"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_COMMENT, s);
        s = (char*) (p->QueryGlobalVariable(p, "::SF::id_date"));
        if (s != NULL && s[0] != '\0')
          sf_set_string(outfile, SF_STR_DATE, s);
      }
      osfopen = 1;
    }

outset:
    outbufsiz = (unsigned)O.outbufsamps * sizeof(MYFLT);/* calc outbuf size */
    outbufp = outbuf = mmalloc(csound, (long)outbufsiz); /*  & alloc bufspace */
    printf(Str("writing %d-byte blks of %s to %s"),
           O.outbufsamps * format_nbytes(O.outformat),
           getstrformat(O.outformat), sfoutname);
    if (strcmp(O.outfilename,"devaudio") == 0   /* realtime output has no
                                                   header */
        || strcmp(O.outfilename,"dac") == 0)  printf("\n");
    else if (O.sfheader == 0) printf(Str(" (raw)\n"));
    else
      printf(" (%s)\n", type2string(O.filetyp));
    osfopen = 1;
    outbufrem = O.outbufsamps;
}

void sfclosein(void *csound)
{
    if (!isfopen) return;
    if (isfd == DEVAUDIO) {
      /* close only if not open for output too */
      if (!osfopen || osfd != DEVAUDIO) {
        extern int rtrecord_dummy(void *csound, void *inBuf, int nBytes);
        /* make sure that rtrecord does not get called after */
        /* closing the device, by replacing it with the dummy function */
        audrecv = (int (*)(void*, MYFLT*, int)) rtrecord_dummy;
        ((ENVIRON*) csound)->rtclose_callback(csound);
      }
    }
    else
#ifdef PIPES
      if (pin != NULL) {
        int _pclose(FILE*);
        _pclose(pin);
        pin = NULL;
      }
      else
#endif
        sf_close(infile);
    isfopen = 0;
    return;
}

void sfcloseout(void *csound_)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    int     nb;

    if (!osfopen) return;
    if ((nb = (O.outbufsamps-outbufrem) * sizeof(MYFLT)) > 0) { /* flush     */
      csound->nrecs++;                                          /* outbuffer */
      audtran(csound, outbuf, nb);
    }
    if (osfd == DEVAUDIO) {
      /* close only if not open for input too */
      if (!isfopen || isfd != DEVAUDIO) {
        extern void rtplay_dummy(void *csound, void *outBuf, int nBytes);
        /* make sure that rtplay does not get called after */
        /* closing the device, by replacing it with the dummy function */
        audtran = (void (*)(void*, MYFLT*, int)) rtplay_dummy;
        csound->rtclose_callback(csound);
      }
      goto report;
    }
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
    nb = sf_close(outfile);
#ifdef PIPES
    if (pout!=NULL) {
      int _pclose(FILE*);
      _pclose(pout);
      pout = NULL;
    }
#endif

 report:
    printf(Str("%ld %d-byte soundblks of %s written to %s"),
           csound->nrecs, O.outbufsamps * format_nbytes(O.outformat),
           getstrformat(O.outformat), sfoutname);
    if (strcmp(O.outfilename,"devaudio") == 0       /* realtime output has no
                                                       header */
        || strcmp(O.outfilename,"dac") == 0) printf("\n");
    else if (O.sfheader == 0) printf(Str(" (raw)\n"));
    else
      printf(" (%s)\n", type2string(O.filetyp));
    osfopen = 0;
}

void soundinRESET(void)
{
    outfile = (SNDFILE*) NULL;
    infile = (SNDFILE*) NULL;
    sfoutname = (char*) NULL;
    inbuf = (MYFLT*) NULL;
    outbuf = (MYFLT*) NULL;
    outbufp = (MYFLT*) NULL;
    inbufrem = (unsigned) 0;
    outbufrem = (unsigned) 0;
}

void (*spinrecv)(void*), (*spoutran)(void*), (*nzerotran)(void*, long);

static void sndwrterr(void *csound, unsigned nret, unsigned nput)
  /* report soundfile write(osfd) error   */
  /* called after chk of write() bytecnt  */
{
    void sfcloseout(void*);
    printf(Str("soundfile write returned bytecount of %d, not %d\n"),
           nret,nput);
    printf(Str("(disk may be full...\n closing the file ...)\n"));
    outbufrem = O.outbufsamps;       /* consider buf is flushed */
    sfcloseout(csound);              /* & try to close the file */
    csoundDie(csound, Str("\t... closed\n"));
}

void sfnopenout(void)
{
    printf(Str("not writing to sound disk\n"));
    outbufrem = O.outbufsamps;          /* init counter, though not writing */
}

static void sndfilein(void *csound_)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    int     i, nsmps, bufpos;

    nsmps = csound->ksmps * csound->nchnls;
    bufpos = (int) O.inbufsamps - (int) inbufrem;
    for (i = 0; i < nsmps; i++) {
      if ((int) inbufrem < 1) {
        inbufrem = (unsigned) 0;
        do {
          inbufrem += (unsigned) (audrecv(csound, inbuf,
                                          ((int) O.inbufsamps - (int) inbufrem)
                                          * (int) sizeof(MYFLT))
                                  / (int) sizeof(MYFLT));
        } while ((int) inbufrem < (int) O.inbufsamps);
        bufpos = 0;
      }
      spin[i] = inbuf[bufpos++] * csound->e0dbfs;
      inbufrem--;
    }
}

void iotranset(void)
    /* direct recv & tran calls to the right audio formatter  */
{   /*                            & init its audio_io bufptr  */
    spinrecv = sndfilein;
}

void bytrev4(char *buf, int nbytes)     /* reverse bytes in buf of longs */
{
    char *p = buf, *q = buf;
    char c1, c2, c3, c4;
    int n = nbytes/4;

    do {
      c1 = *p++;
      c2 = *p++;
      c3 = *p++;
      c4 = *p++;
      *q++ = c4;
      *q++ = c3;
      *q++ = c2;
      *q++ = c1;
    } while (--n);
}

void bytrev2(char *buf, int nbytes)      /* reverse bytes in buf of shorts */
{
    char *p = buf, c1, c2;
    int n = nbytes/2;

    do {
      c1 = *p++;
      c2 = *p--;
      *p++ = c2;
      *p++ = c1;
    } while (--n);
}

