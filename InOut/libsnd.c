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

#ifdef mills_macintosh
# include "MacTransport.h"
#endif

#ifdef RTAUDIO
extern  int     rtrecord_(MYFLT *, int);
extern  void    rtplay_(MYFLT *, int);
extern  void    rtclose_(void);
extern  void    recopen_(int, int, float, int);
extern  void    playopen_(int, int, float, int);
#endif

static  SNDFILE *outfile;
extern  SNDFILE *infile;
static  char    *sfoutname;                     /* soundout filename    */
        MYFLT   *inbuf;
        MYFLT   *outbuf;                        /* contin sndio buffers */
static  MYFLT   *outbufp;                       /* MYFLT pntr           */
        unsigned inbufrem;
        unsigned outbufrem;                     /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd;
        int     isfopen = 0, infilend = 0;      /* (real set in sfopenin)    */
static  int     osfd;
        int     osfopen = 0;                    /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
unsigned long   nframes = 1;
#ifdef RTAUDIO
#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
#endif
#ifdef PIPES
extern FILE* pin, *pout;
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif
extern  void    (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
int     (*audrecv)(MYFLT *, int);
void    (*audtran)(MYFLT *, int);
static  SOUNDIN *p;    /* to be passed via sreadin() */
SNDFILE *sndgetset(SOUNDIN *);

extern  char    *getstrformat(int format);
static  void    sndwrterr(unsigned, unsigned);
extern  unsigned long   nframes;

extern int type2sf(int);
extern short sf2type(int);
extern char* type2string(int);
extern short sfsampsize(int);
#ifdef  USE_DOUBLE
#define sf_write_MYFLT  sf_write_double
#define sf_read_MYFLT   sf_read_double
#else
#define sf_write_MYFLT  sf_write_float
#define sf_read_MYFLT   sf_read_float
#endif


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
    long   *rngp;                        /*RWD Nov 2001 */
    MYFLT *sp = spout;
    MYFLT       absamp;

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
        if (absamp > e0dbfs) {           /* out of range?     */
          /*   report it*/
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                             /* ditto -ive samp */
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
        audtran(outbuf,outbufsiz); /* Flush buffer */
        outbufp = (MYFLT *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

void zerosf(long len)
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
      do *outbufp++ = FL(0.0);
      while (--n);
    }
    else outbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);        /* Flush */
      outbufp = (MYFLT*)outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

static void writesf(MYFLT *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    int n;
    if (osfd<0) return;
    n = sf_write_MYFLT(outfile, outbuf, nbytes/sizeof(MYFLT));
    if (n < nbytes/sizeof(MYFLT))
      sndwrterr(n, nbytes);
    if (O.rewrt_hdr)
      rewriteheader(outfile,0);
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

static int readsf(MYFLT *inbuf, int nsamples)
{
/*     printf("***nsamples = %d (%d frames) to %p\n", */
/*            nsamples, nsamples/nchnls, inbuf); */
    return nchnls*sf_read_MYFLT(infile, inbuf, nsamples/nchnls);
}

HEADATA *readheader(            /* read soundfile hdr, fill HEADATA struct */
    int ifd,                    /*   called by sfopenin() and sndinset()   */
    char *sfname,               /* NULL => no header, nothing to preserve  */
    SOUNDIN *p)
{
    return NULL;
}

void writeheader(int ofd, char *ofname)
{
    sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

int sndinset(SOUNDIN *p)    /* init routine for instr soundin   */
                            /* shares above sndgetset with SAsndgetset, gen01*/
{
    SNDFILE *sinfd;
    int     reinit = 0;

    if (p->fdch.fd!=NULL) {                 /* if file already open, close it */
          /* RWD: it is not safe to assume all compilers init this to 0 */
          /* (IV: but it is allocated with mcalloc...) */
      /* reload the file */
      reinit++; sf_close(p->fdch.fd);
    }
    p->channel = ALLCHNLS;                   /* reading all channels      */
    p->analonly = 0;
    if ((sinfd = sndgetset(p)) != NULL) {    /* if soundinset successful  */
      p->fdch.fd = sinfd;                    /*    store & log the fd     */
      if (!reinit) fdrecord(&p->fdch);       /*    instr will close later */
      p->sampframsiz /= p->OUTOCOUNT;        /* IV - Nov 16 2002 */
    }
    else
      return initerror(errmsg);              /* else just print the errmsg*/
    return OK;
}

int soundin(SOUNDIN *p)
{
    MYFLT       *r[24], scalefac;
    int         nsmps, ntogo, blksiz, chnsout, i = 0, n;

    if (p->format == AE_FLOAT &&
        (p->filetyp == TYP_WAV || p->filetyp == TYP_AIFF)) {
      /* NB also INLONGFAC changed def */
      scalefac = e0dbfs;
      if (p->do_floatscaling)
        scalefac *= p->fscalefac;
    }
    else scalefac = FL(1.0);

    if (!p->inbufp) {
      return perferror(Str(X_1210,"soundin: not initialised"));
    }
    chnsout = p->OUTOCOUNT;
    blksiz = chnsout * ksmps;
/*     printf("***        : chnsout=%d blksiz=%d\n", chnsout, blksiz); */
/*     printf("***        : p->r[0]=%p, p->r[1]=%p\n", p->r[0], p->r[1]); */
    memcpy(r, p->r, chnsout * sizeof(MYFLT*));
    ntogo = blksiz;
    if (p->endfile)
      goto filend;
    nsmps = (p->bufend - p->inbufp);
    if (nsmps > blksiz)
      nsmps = blksiz;
    ntogo -= nsmps;
 sndin:
    {
      MYFLT *inbufp = p->inbufp;
/*       printf("***        : loop start p->fdch.fd=%p\n", p->fdch.fd); */
      do {
/*         printf("Writing %p %d <- %f\n", r[i], i, *inbufp * scalefac); */
        *(r[i]++) = *inbufp++ * scalefac;
/*         printf("        %f\n", *(r[i]-1)); */
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
/*       printf("***        : loop endp->fdch.fd=%p\n", p->fdch.fd); */
      p->inbufp = inbufp;
    }
    if (p->inbufp >= p->bufend) {
/*       printf("***        : need new data p->fdch.fd=%p\n", p->fdch.fd); */
      if ((n = sreadin(NULL, p->inbuf, SNDINBUFSIZ, p)) == 0) {
        p->endfile = 1;
        if (ntogo) goto filend;
        else return OK;
      }
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
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
#if defined(WIN32) || defined(LINUX)
      rtin_dev = 0;
      if (strncmp(O.infilename,"devaudio", 8) == 0) {
        if (O.infilename[8]==':')
          rtin_devs = &(O.infilename[9]);
        else
          sscanf(O.infilename+8, "%d", &rtin_dev);
      }
      else if (strncmp(O.infilename,"adc", 3) == 0) {
        if (O.infilename[3]==':')
          rtin_devs = &(O.infilename[4]);
        else
          sscanf(O.infilename+3, "%d", &rtin_dev);
      }
#endif
      sfname = O.infilename;
      recopen_(nchnls,O.insampsiz,(float)esr,2);  /* open devaudio for input */
      audrecv = rtrecord_;                /*  & redirect audio gets  */
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
/*    printf("***sfinfo: samplerate=%d channels=%d format=%.8x sections=%d\n", */
/*        sfinfo.samplerate, sfinfo.channels, sfinfo.format, sfinfo.sections); */
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
      O.informat = p->filetyp = sf2format(sfinfo.format);
      p->audrem = sfinfo.frames;
      audrecv = readsf;  /* will use standard audio gets  */
    }
#ifdef RTAUDIO
 inset:
#endif
    inbufsiz = (unsigned)(O.inbufsamps * sizeof(MYFLT));     /* calc inbufsize reqd   */
    inbuf = (MYFLT *)mcalloc(inbufsiz); /* alloc inbuf space */
    printf(Str(X_1151,"reading %d-sample blks of %s from %s (%s)\n"),
           inbufsiz, getstrformat(O.informat), sfname,
           type2string(p->filetyp));
    isfopen = 1;
    n = audrecv(inbuf, inbufsiz);          /*     file or devaudio  */
    //inbufrem = (unsigned int)n;            /* datasiz in monosamps  */
}

void sfopenout(void)                            /* init for sound out       */
{                                               /* (not called if nosound)  */
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
#if defined(WIN32) || defined(LINUX)
      if (strncmp(O.outfilename,"devaudio", 8) == 0) {
        if (O.outfilename[8]==':')
          rtout_devs = &(O.outfilename[9]);
        else
          sscanf(O.outfilename+8, "%d", &rtout_dev);
      }
      else if (strncmp(O.outfilename,"dac", 3) == 0) {
        if (O.outfilename[3]==':')
          rtout_devs = &(O.outfilename[4]);
        else
          sscanf(O.outfilename+3, "%d", &rtout_dev);
      }
#endif
      sfoutname = O.outfilename;
      playopen_(nchnls, O.sfsampsize, (float)esr, 2);  /* open devaudio for out */
      audtran = rtplay_;                        /* & redirect audio puts */
      osfd = DEVAUDIO;                         /* dummy file descriptor */
      pipdevout = 1;                           /* no backward seeks !   */
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
      if (dither_output)        /* This may not be written yet!! */
        sf_command(outfile, SFC_SET_DITHER_ON_WRITE, NULL, SF_TRUE);
      spoutran = spoutsf;       /* accumulate output */
      nzerotran = zerosf;       /* quick zeros */
      audtran = writesf;        /* flush buffer */
      osfopen = 1;
    }
#if defined(SYMANTEC)
    AddMacHeader(sfoutname,nchnls,esr,O.sfsampsize);  /* set Mac resource */
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
    else if (O.sfheader == 0) printf(Str(X_24," (raw)\n"));
    else
      printf(" %s\n", type2string(O.filetyp));
    osfopen = 1;
    outbufrem = O.outbufsamps;
}

void sfclosein(void)
{
    if (!isfopen) return;
#ifdef RTAUDIO
    if (isfd == DEVAUDIO) {
      if (!osfopen || osfd != DEVAUDIO)
        rtclose_();     /* close only if not open for output too */
    }
    else
#endif
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

void sfcloseout(void)
{
    int nb;
    if (!osfopen) return;
    if ((nb = (O.outbufsamps-outbufrem) * O.sfsampsize) > 0)/* flush outbuffer */
      audtran(outbuf, nb);
#ifdef RTAUDIO
    if (osfd == DEVAUDIO) {
      if (!isfopen || isfd != DEVAUDIO)
        rtclose_();     /* close only if not open for input too */
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
    else if (O.sfheader == 0) printf(Str(X_24," (raw)\n"));
    else
      printf(" %s\n", type2string(O.filetyp));
    osfopen = 0;
}

static  long    datpos= 0L;       /* Used in resetting only */

extern  HEADATA *readheader(int, char *, SOUNDIN*);
extern  int     openin(char*);
extern  OPARMS  O;

void soundinRESET(void)
{
    datpos = 0;
}

extern  HEADATA *readheader(int, char*, SOUNDIN*);
extern  OPARMS  O;

#ifdef RTAUDIO
# define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
#endif
#ifdef PIPES
FILE* pin=NULL, *pout=NULL;
/*sbrandon: added NeXT to line below*/
# if defined(SGI) || defined(LINUX) || defined(__BEOS__) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# endif
#endif

void (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);

static void sndwrterr(unsigned nret, unsigned nput) /* report soundfile write(osfd)
                                                error      */
  /* called after chk of write() bytecnt  */
{
    void sfcloseout(void);
    printf(Str(X_1203,"soundfile write returned bytecount of %d, not %d\n"),
           nret,nput);
    printf(Str(X_77,"(disk may be full...\n closing the file ...)\n"));
    outbufrem = O.outbufsamps;       /* consider buf is flushed */
    sfcloseout();                    /* & try to close the file */
    die(Str(X_563,"\t... closed\n"));
}

static void sndfilein(void);
void iotranset(void)
    /* direct recv & tran calls to the right audio formatter  */
{   /*                            & init its audio_io bufptr  */
    spinrecv = sndfilein;
}

#if !defined(SYMANTEC) && !defined(mac_classic) && !defined(LINUX) && !defined(__BEOS__) && !defined(__MACH__)
extern int write(int, const void*, unsigned int);
#endif

void sfnopenout(void)
{
    printf(Str(X_1079,"not writing to sound disk\n"));
    outbufrem = O.outbufsamps;          /* init counter, though not writing */
}

static void clrspin1(MYFLT *r, int spinrem) /* clear remainder of spinbuf
                                               to zeros */
{                                           /* called only once, at EOF   */
    infilend = 1;                        /* 1st filend pass:   */
    while (spinrem--)                    /*   clear spin rem   */
      *r++ = FL(0.0);
}

static void clrspin2(void)              /* clear spinbuf to zeros   */
{                                       /* called only once, at EOF */
    MYFLT *r = spin;
    int n = nspin;
    infilend = 2;                            /* at 2nd filend pass  */
    do *r++ = FL(0.0);                       /*   clr whole spinbuf */
    while (--n);
    printf(Str(X_713,"end of audio_in file\n"));
}

static void sndfilein(void)
{
    int samples = nchnls * ksmps;
    int i;
    audrecv(spin, sizeof(MYFLT) * samples);
    for(i = 0; i < samples; i++)
    {
        spin[i] *= float_to_dbfs;
    }
}

#ifdef OLD_CODE_DID_NOT_WORK

static void sndfilein(void)
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;
    MYFLT *bufp = &inbuf[inbufsiz-inbufrem];

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;                      /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do {
      *r++ = *bufp++ * float_to_dbfs;
    } while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          inbufrem = n;
          bufp = inbuf;
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
  }
}

#endif

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

