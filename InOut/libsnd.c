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

#ifdef _SNDFILE_

#include "cs.h"                         /*             SNDLIB.C       */
#include "soundio.h"
#include <sndfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef RTAUDIO
extern  int     (*rtrecord)(char *, int);
extern  void    (*rtplay)(void *, int);
extern  void    (*rtclose)(void);
extern  void    (*recopen)(int, int, float, int);
extern  void    (*playopen)(int, int, float, int);
#endif

static  SNDFILE *outfile, *infile;
static  char    *sfoutname;                     /* soundout filename    */
static  MYFLT   *inbuf;
        MYFLT   *outbuf;                        /* contin sndio buffers */
static  MYFLT   *inbufp, *outbufp;              /* MYFLT pntr           */
static  unsigned inbufrem, outbufrem;           /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd, isfopen = 0, infilend = 0;/* (real set in sfopenin)    */
static  int     osfd, osfopen = 0;              /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
#ifdef RTAUDIO
#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
# ifdef sol
extern  int     audiofd;
# endif
#endif
#ifdef PIPES
extern FILE* pin, *pout;
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif
extern void (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
static  int     (*audrecv)(void *, int);
static  void    (*audtran)(void *, int);
static  SOUNDIN *p;    /* to be passed via sreadin() */

extern char *getstrformat(int format);
extern void sndwrterr(unsigned, unsigned);
extern unsigned long   nframes;

#ifdef  USE_DOUBLE
#define sf_write_MYFLT	sf_write_double
#define sf_read_MYFLT	sf_read_double
#else
#define sf_write_MYFLT	sf_write_float
#define sf_read_MYFLT	sf_read_float
#endif

static int type2sf(int type)
{
    switch (type) {
    case TYP_WAV:
      return SF_FORMAT_WAV;
    case TYP_AIFF:
      return SF_FORMAT_AIFF;
    case TYP_IRCAM:
      return SF_FORMAT_IRCAM;
    }
    return SF_FORMAT_RAW;
}

static int sf2type(format)
{
    switch (format&SF_FORMAT_TYPEMASK) {
    case SF_FORMAT_WAV:
      return TYP_WAV;
    case SF_FORMAT_AIFF:
      return TYP_AIFF;
    default:
      {
        char buffer[100];
        sprintf(buffer, "Unsupported input file type %x\n", format);
        die(buffer);
      }
    case SF_FORMAT_RAW:
      return 0;
    case SF_FORMAT_IRCAM:
      return TYP_IRCAM;
    }
}

int format2sf(int format)
{
    switch (format) {
    case AE_CHAR:
      return SF_FORMAT_PCM_S8;
#ifdef never
    case AE_ALAW:
      return SF_FORMAT_ALAW
#endif
#ifdef ULAW
    case AE_ULAW:
      return SF_FORMAT_ULAW
#endif
    case AE_SHORT:
      return SF_FORMAT_PCM_16;       /* Signed 16 bit data */
    case AE_LONG:
      return SF_FORMAT_PCM_32;       /* Signed 32 bit data */
    case AE_FLOAT:
      return SF_FORMAT_FLOAT;       /* 32 bit float data */
    case AE_UNCH:
      return SF_FORMAT_PCM_U8;       /* Unsigned 8 bit data (WAV and RAW only) */
    case AE_24INT:
      return SF_FORMAT_PCM_24;       /* Signed 24 bit data */
    case AE_DOUBLE:
      return SF_FORMAT_DOUBLE;       /* 64 bit float data */
    }
    return SF_FORMAT_PCM_16;
}

/* The interface requires 3 functions:
   spoutran to transfer nspout items to buffer
   nzerotran to transfer nspout zeros to buffer
   audtran to actually write the data

   spoutran is called with nchnls*ksamps items and this need to be
   buffered until outbufsiz items have been accumulated.  It will call
   audtran to flush when this happens.
*/

void spoutsf(void)
{
    int n, spoutrem = nspout;
    MYFLT *maxampp = maxamp;
    unsigned long       *maxps = maxpos;
    long   *rngp;                       /*RWD Nov 2001 */
    MYFLT *sp = spout;
    MYFLT       absamp;

nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < FL(0.0))
        absamp = -absamp;
      if (absamp > *maxampp) {         /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      absamp = *sp;
      if (absamp >= 0) { /* +ive samp:   */
        if (absamp > e0dbfs) {          /* out of range?     */
          /*   report it*/
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if (absamp < -e0dbfs) {
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      absamp *= dbfs_to_float;
      if (osfopen)
        *outbufp++ = absamp;
      if (multichan) {
        maxps++;
        if (++maxampp >= maxampend)
          maxampp = maxamp, maxps = maxpos, nframes++;
      }
      else nframes++;
      sp++;
    } while (--n);
    if (!outbufrem) {
      if (osfopen) {
        audtran(outbuf,outbufsiz);
        outbufp = (MYFLT *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

void zerosf(int len)
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)len;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do *outbufp++ = FL(0.0);
      while (--n);
    }
    else outbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      outbufp = (MYFLT*)outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

void rewriteheader(int ofd, long datasize, int verbose)
{
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
    return;
}

static void writesf(void *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    int n;
    if (osfd<0) return;
    if ((n = sf_write_MYFLT(outfile, outbuf, nbytes/sizeof(MYFLT))) < nbytes/sizeof(MYFLT))
      sndwrterr(n, nbytes);
    if (O.rewrt_hdr)
      rewriteheader(osfd, nbytes +(long)nrecs*outbufsiz, 0);
    nrecs++;                /* JPff fix */
    if (O.heartbeat) {
      if (O.heartbeat==1) {
#ifdef SYMANTEC
        nextcurs();
#else
        putc("|/-\\"[nrecs&3], stderr); putc(8,stderr);
#endif
      }
      else if (O.heartbeat==2) putc('.', stderr);
      else if (O.heartbeat==3) {
        int n;
        err_printf( "%d(%.3f)%n", nrecs, nrecs/ekr, &n);
        while (n--) err_printf("\b");
      }
      else err_printf("\a");
    }
}

static void readsf(void *inbuf, int nbytes)
{
    sf_read_MYFLT(infile, inbuf, nbytes/nchnls);
}

int SAsndgetset(
     char    *infilnam,                          /* Stand-Alone sndgetset() */
     SOUNDIN **ap,                               /* used by SoundAnal progs */
     MYFLT   *abeg_time,
     MYFLT   *ainput_dur,
     MYFLT   *asr,
     int     channel)
{                               /* Return -1 on failure */
    return -1;
}

long getsndin(int fd, MYFLT *fp, long nlocs, SOUNDIN *p)
        /* a simplified soundin */
{
    return 0;
}

HEADATA *readheader(            /* read soundfile hdr, fill HEADATA struct */
    int ifd,                    /*   called by sfopenin() and sndinset()   */
    char *sfname,               /* NULL => no header, nothing to preserve  */
    SOUNDIN *p)
{
    return NULL;
}

int sndgetset(SOUNDIN *p)       /* core of soundinset                */
                                /* called from sndinset, SAsndgetset, & gen01 */
                                /* Return -1 on failure */
{
    return -1;
}

int sreadin(                    /* special handling of sound input       */
    int     infd,               /* to accomodate reads thru pipes & net  */
    char    *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*  */
    SOUNDIN *p)                 /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    int    n, ntot=0;

    do {
      MYFLT *inb = (MYFLT*)inbuf;
      if ((n = sf_read_MYFLT(infile, inb+ntot, (nbytes-ntot)/nchnls)) < 0)
        die(Str(X_1201,"soundfile read error"));
    } while (n > 0 && (ntot += n) < nbytes);
    if (p->audrem > 0) {                /* AIFF:                  */
      if (ntot > p->audrem)           /*   chk haven't exceeded */
        ntot = p->audrem;           /*   limit of audio data  */
      p->audrem -= ntot;
    }
    else ntot = 0;
    return(ntot);
}

void writeheader(int ofd, char *ofname) 
{
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

int sndinset(SOUNDIN *p)    /* init routine for instr soundin   */
                             /* shares above sndgetset with SAsndgetset, gen01*/
{
    return 0;
}

int soundin(SOUNDIN *p)
{
    return 0;
}

void sfopenin(void)             /* init for continuous soundin */
{
    char    *sfname = NULL;
    long     n;

    if (p == NULL)
      p = (SOUNDIN *) mcalloc((long)sizeof(SOUNDIN));
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
#ifdef RTAUDIO
    else if (O.infilename != NULL &&
             (strcmp(O.infilename,"devaudio") == 0
#ifdef WIN32
              || strncmp(O.infilename,"devaudio", 8) == 0
              || strncmp(O.infilename,"adc", 3) == 0
#endif
              || strcmp(O.infilename,"adc") == 0)) {
#if defined(WIN32) || defined(HAVE_ALSA)
      rtin_dev = 0;
      if (strncmp(O.infilename,"devaudio", 8) == 0)
        sscanf(O.infilename+8, "%d", &rtin_dev);
      else if (strncmp(O.infilename,"adc", 3) == 0)
        sscanf(O.infilename+3, "%d", &rtin_dev);
#endif
      sfname = O.infilename;
      recopen(nchnls,O.insampsiz,(float)esr,2);  /* open devaudio for input */
      audrecv = rtrecord;                 /*  & redirect audio gets  */
      isfd = DEVAUDIO;                    /* dummy file descriptor   */
      pipdevin   = 1;                     /* no backward seeks !     */
      goto inset;                         /* no header processing    */
    }
#endif
    else {                      /* else build filename and open that */
      SF_INFO sfinfo;
      if ((isfd = openin(O.infilename)) < 0)
        dies(Str(X_947,"isfinit: cannot open %s"), retfilnam);
      sfname = retfilnam;
      memset(&sfinfo, '\0', sizeof(SF_INFO));
      infile= sf_open_fd(isfd, SFM_READ, &sfinfo, SF_TRUE); 
      p->filetyp = 0;               /* initially non-typed for readheader */
      if (sfinfo.samplerate != (long)esr &&
          (O.msglevel & WARNMSG)) {              /*    chk the hdr codes  */
        printf(Str(X_607,"WARNING: audio_in %s has sr = %ld, orch sr = %ld\n"),
                sfname, sfinfo.samplerate, (long)esr);
      }
      if (sfinfo.channels != nchnls) {
        sprintf(errmsg,Str(X_606,"audio_in %s has %ld chnls, orch %d chnls"),
                sfname, sfinfo.channels, nchnls);
        die(errmsg);
      }
      /* Do we care about the format?  Can assume float?? */
      O.insampsiz = sizeof(MYFLT);        /*    & cpy header vals  */
      O.informat = p->filetyp = sf2type(sfinfo.format);
      p->audrem = sfinfo.frames;
      if (O.informat==0) {      /* no header:  defaults  */
        if (O.msglevel & WARNMSG) {
          printf(Str(X_54,"WARNING: %s has no soundfile header, assuming %s\n"),
                 sfname, getstrformat(O.outformat) );
        }
        p->filetyp = 0;                          /*  (see also soundin.c) */
        p->audrem = -1;
      }
      p->bytrev = NULL;
      audrecv = readsf;  /* will use standard audio gets  */
    }
#ifdef RTAUDIO
 inset:
#endif
    inbufsiz = (unsigned)O.inbufsamps * O.insampsiz;/* calc inbufsize reqd   */
    inbuf = mcalloc((long)inbufsiz); /* alloc inbuf space     */
    printf(Str(X_1151,"reading %d-byte blks of %s from %s %s\n"),
           inbufsiz, getstrformat(O.informat), sfname,
           p->filetyp == TYP_AIFF ? "(AIFF)" :
           p->filetyp == TYP_AIFC ? "(AIFF-C)" :
           p->filetyp == TYP_WAV ? "(WAV)" : "");
    isfopen = 1;
    n = audrecv(inbuf, inbufsiz);          /*     file or devaudio  */
    inbufrem = (unsigned int)(n / O.insampsiz); /* datasiz in monosamps  */
}

void sfopenout(void)                            /* init for sound out       */
{                                               /* (not called if nosound)  */
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
      if (O.filetyp == TYP_AIFF ||
          O.filetyp == TYP_AIFC ||
          O.filetyp == TYP_WAV) {
        printf(Str(X_400,"Output file type changed to IRCAM for use in pipe\n"));
        O.filetyp = TYP_IRCAM;
      }
    }
#endif
#ifdef RTAUDIO
    else if (strcmp(O.outfilename,"devaudio") == 0
             || strncmp(O.outfilename,"devaudio", 8) ==0
             || strncmp(O.outfilename,"dac", 3) ==0
# ifdef LINUX
             || strcmp(O.outfilename,"/dev/dsp") ==0
# endif
             || strcmp(O.outfilename,"dac") == 0) {
#if defined(WIN32) || defined(HAVE_ALSA)
      if (strncmp(O.outfilename,"devaudio", 8) == 0)
        sscanf(O.outfilename+8, "%d", &rtout_dev);
      else if (strncmp(O.outfilename,"dac", 3) == 0)
        sscanf(O.outfilename+3, "%d", &rtout_dev);
#endif
      sfoutname = O.outfilename;
      playopen(nchnls, O.outsampsiz, (float)esr, 2);  /* open devaudio for out */
      audtran = rtplay;                        /* & redirect audio puts */
#ifdef NeXT /*sbrandon: even RT playback has to be swapped*/
# ifdef __LITTLE_ENDIAN__
        audtran = swaprtplay;
# endif
#endif
      osfd = DEVAUDIO;                         /* dummy file descriptor */
      pipdevout = 1;                           /* no backward seeks !   */
#ifdef sol
      if (O.sfheader)
        writeheader(audiofd,"devaudio");
#endif
#if defined(mills_macintosh) || defined(SYMANTEC)
      O.outformat = AE_SHORT;
#endif
#ifdef mills_macintosh
      transport.state |= kGenRealtime;
#endif
      goto outset;                        /* no header needed      */
    }
#endif
    else if (strcmp(O.outfilename,"null") == 0) {
      osfd = -1;
      sfoutname = mmalloc((long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
    }
    else {  /* else open sfdir or cwd */
      SF_INFO sfinfo;
      sfinfo.frames = -1;
      sfinfo.samplerate = (int)esr;
      sfinfo.channels = nchnls ;
      sfinfo.format = type2sf(O.filetyp)|format2sf(O.outformat);
      sfinfo.sections = 0;
      sfinfo.seekable = 0;
      if ((osfd = openout(O.outfilename, 3)) < 0) 
        dies(Str(X_1187,"sfinit: cannot open %s"), retfilnam);
      sfoutname = mmalloc((long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
      outfile = sf_open_fd(osfd, SFM_WRITE, &sfinfo, 1);
      if (peakchunks) 
        sf_command(outfile, SFC_SET_ADD_PEAK_CHUNK, NULL, SF_TRUE);
      if (strcmp(sfoutname, "/dev/audio") == 0) {
        /*      ioctl(   );   */
        pipdevout = 1;
      }
      spoutran = spoutsf;       /* accumulate output */
      nzerotran = zerosf;       /* quick zeros */
      audtran = writesf;        /* flush buffer */
      osfopen = 1;
    }
#if defined(SYMANTEC)
    AddMacHeader(sfoutname,nchnls,esr,O.outsampsiz);  /* set Mac resource */
    SetMacCreator(sfoutname);               /*   set creator & file type */
#endif
    if (O.sfheader) {
#ifdef mills_macintosh
      transport.osfd = osfd;
      transport.eoheader = lseek(osfd,(off_t)0L,SEEK_CUR);
#endif
    }
#ifdef RTAUDIO
outset:
#endif
    outbufsiz = (unsigned)O.outbufsamps * sizeof(MYFLT);/* calc outbuf size */
    outbufp = outbuf = mmalloc((long)outbufsiz); /*  & alloc bufspace */
    printf(Str(X_1382,"writing %d-byte blks of %s to %s\n"),
           outbufsiz, getstrformat(O.outformat), sfoutname);
    if (strcmp(O.outfilename,"devaudio") == 0   /* realtime output has no
                                                   header */
        || strcmp(O.outfilename,"dac") == 0)  printf("\n");
    else if (O.sfheader == 0) printf(" (raw)\n");
    else
      printf(" %s\n",
             O.filetyp == TYP_AIFF ? "(AIFF)" :
             O.filetyp == TYP_AIFC ? "(AIFF-C)" :
             O.filetyp == TYP_WAV ? "(WAV)" :
#ifdef mac_classic
             "(SDII)"
#elif defined(SFIRCAM)
             "(IRCAM)"
#elif defined(NeXT)
             "(NeXT)"
#else
             "(Raw)"
#endif
             );
    osfopen = 1;
    outbufrem = O.outbufsamps;
}

void iotranset(void)
{
    return;
}


void sfclosein(void)
{
    return;
}

void sfcloseout(void)
{
    int nb;
    if (!osfopen) return;
    if ((nb = (O.outbufsamps-outbufrem) * O.outsampsiz) > 0)/* flush outbuffer */
      audtran(outbuf, nb);
#ifdef RTAUDIO
    if (osfd == DEVAUDIO) {
      if (!isfopen || isfd != DEVAUDIO)
        rtclose();     /* close only if not open for input too */
      goto report;
    }
#endif
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
    nb = sf_close(outfile);
#ifdef PIPES
    if (pout!=NULL) {
      int _pclose(FILE*);
      _pclose(pout);
      pout = NULL;
    }
#endif
#ifdef RTAUDIO
 report:
#endif
    printf(Str(X_44,"%ld %d-byte soundblks of %s written to %s"),
           nrecs, outbufsiz, getstrformat(O.outformat), sfoutname);
    if (strcmp(O.outfilename,"devaudio") == 0       /* realtime output has no
                                                       header */
        || strcmp(O.outfilename,"dac") == 0) printf("\n");
    else if (O.sfheader == 0) printf(" (raw)\n");
    else
      printf(" %s\n",
             O.filetyp == TYP_AIFF ? "(AIFF)" :
             O.filetyp == TYP_AIFC ? "(AIFF-C)" :
             O.filetyp == TYP_WAV ? "(WAV)" :
#ifdef mac_classic
                                             "(SDII)"
#elif defined(SFIRCAM)
                                             "(IRCAM)"
#elif defined(NeXT)
                                             "(NeXT)"
#else
                                             "(Raw)"
#endif
             );
    osfopen = 0;
}

#endif

