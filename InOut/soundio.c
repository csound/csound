/*  
    soundio.c:

    Copyright (C) 1991, 2000 Barry Vercoe, John ffitch, Richard Dobson, Istvan Varga

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

#include "cs.h"                 /*                      SOUNDIO.C       */
#include "soundio.h"
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef mills_macintosh
#include "MacTransport.h"
#endif

#ifdef RTAUDIO
extern  int     (*rtrecord)(char *, int);
extern  void    (*rtplay)(char *, int);
extern  void    (*rtclose)(void);
extern  void    (*recopen)(int, int, float, int);
extern  void    (*playopen)(int, int, float, int);
#endif

/******* RWD 3:2000 requirements and assumtions for sample formats:
 * (1)  the assumed 'normal' range for a Csound audio sample is +- 32767.0
 *      This will thus translate directly to 16bit storage, with a direct cast.
 * (2)  floating-point WAVE and AIFF-C files should contain normalised floats
 *      (+- 1.0) so the data must be scaled accordingly, based on (1).
 * (3)  32bit integer files contain samples in range +- (2^^31)-1, so again
 *      must be scaled from the Csound 'normal' range.
 * (4)  (if/when 24bit formats are supported, samples must be similarly scaled
 *      into the range (2^^23)-1.
 * (5)  PEAK data IN ALL CASES is normalised to +- 1.0. For floatsam files,
 *      where the values can exceed this range, the PEAK values will record
 *      that directly.  For all integer formats, PEAK will reflect clipping,
 *      so will never exceed +-1.0.
 */

static  char    *sfoutname;                     /* soundout filename    */
static  char    *inbuf;
        char    *outbuf;                        /* contin sndio buffers */
static  char    *chinbufp, *choutbufp;          /* char  pntr to above  */
static  short   *shinbufp, *shoutbufp;          /* short pntr           */
static  long    *llinbufp, *lloutbufp;          /* long  pntr           */
static  float   *flinbufp, *floutbufp;          /* MYFLT pntr           */
static  unsigned inbufrem, outbufrem;           /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd, isfopen = 0, infilend = 0;/* (real set in sfopenin)    */
static  int     osfd, osfopen = 0;              /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
unsigned long   nframes = 1;

extern  HEADATA *readheader(int, char*, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  OPARMS  O;

static  SOUNDIN *p;    /* to be passed via sreadin() */
static  int     (*audrecv)(char *, int), audread(char *, int);
static  void    (*audtran)(char *, int), audwrite(char *, int);
static  void    audwrtrev2(char *, int), audwrtrev4(char *, int);
/*RWD 5:2001*/
static void audwrtrev3(char *,int);

extern  void    bytrev2(char *, int), bytrev4(char *, int);
extern  void    rewriteheader(int, long, int);
extern  int     openin(char *), openout(char *, int);
extern  int     bytrevhost(void), getsizformat(int);
extern  char    *getstrformat(int);

/*RWD 5:2001 24bit support */
extern void     bytrev3(char *,int);
static void     int24ptran(void);
static void     int24pzerotran(long);
static char     *int24outbufp,*int24inbufp;
static void     int24precv(void);

#ifdef RTAUDIO
#define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
# ifdef sol
extern  int     audiofd;
# endif
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
static void byterecv(void), charrecv(void),
#ifdef never
            alawrecv(void),
#endif
#ifdef ULAW
            ulawrecv(void),
#endif
            shortrecv(void),longrecv(void), floatrecv(void);
static void bytetran(void), chartran(void),
#ifdef never
            alawtran(void), 
#endif
#ifdef ULAW
            ulawtran(void),
#endif
            shortran(void), longtran(void), floatran(void);
static void bytetran_d(void), chartran_d(void), 
#ifdef ULAW
            ulawtran_d(void),
#endif
            shortran_d(void), longtran_d(void);
static void bzerotran(long), czerotran(long),
#ifdef never
            azerotran(long), 
#endif
#ifdef ULAW
            uzerotran(long),
#endif
            szerotran(long), lzerotran(long), fzerotran(long);
static void floatran_noscl(void), floatrecv_noscl(void); /* IV - Jul 11 2002 */

void iotranset(void)
    /* direct recv & tran calls to the right audio formatter  */
{   /*                            & init its audio_io bufptr  */
        switch(O.informat) {
        case AE_UNCH:  spinrecv = byterecv;          /* J. Mohr  1995 Oct 17 */
                       chinbufp = inbuf;
                       break;
        case AE_CHAR:  spinrecv = charrecv;
                       chinbufp = inbuf;
                       break;
#ifdef never
        case AE_ALAW:  spinrecv = alawrecv;
                       chinbufp = inbuf;
                       break;
#endif
#ifdef ULAW
        case AE_ULAW:  spinrecv = ulawrecv;
                       chinbufp = inbuf;
                       break;
#endif
        case AE_SHORT: spinrecv = shortrecv;
                       shinbufp = (short *)inbuf;
                       break;
        case AE_LONG:  spinrecv = longrecv;
                       llinbufp = (long  *)inbuf;
                       break;
        case AE_FLOAT: if ((p != NULL   /* normalize WAV and AIFF only */
                            && ((p->filetyp == TYP_WAV)  /* IV - Jul 11 2002 */
                                || (p->filetyp == TYP_AIFF)
                                || (p->filetyp == TYP_AIFC)))
#ifdef RTAUDIO
                           || (audrecv == rtrecord)
#endif
                          )
                         spinrecv = floatrecv;
                       else spinrecv = floatrecv_noscl;
                       flinbufp = (float *)inbuf;
                       break;
        case AE_24INT:                            /*RWD 5:2001 */
                       spinrecv = int24precv;     /* 24bit Packed */
                       int24inbufp = inbuf;
                       break;
        default: die(Str(X_1328,"unknown audio_in format"));
        }

        switch(O.outformat) {
          /* J. Mohr  1995 Oct 17 */
        case AE_UNCH:  spoutran = (dither_output ? bytetran_d : bytetran);
                       nzerotran = bzerotran;
                       choutbufp = outbuf;
                       break;
        case AE_CHAR:  spoutran = (dither_output ? chartran_d : chartran);
                       nzerotran = czerotran;
                       choutbufp = outbuf;
                       break;
#ifdef never
        case AE_ALAW:  spoutran = alawtran;
                       nzerotran = azerotran;
                       choutbufp = outbuf;
                       break;
#endif
#ifdef ULAW
        case AE_ULAW:  spoutran = (dither_output ? ulawtran_d : ulawtran);
                       nzerotran = uzerotran;
                       choutbufp = outbuf;
                       break;
#endif
        case AE_SHORT: spoutran = (dither_output ? shortran_d : shortran);
                       nzerotran = szerotran;
                       shoutbufp = (short *)outbuf;
                       break;
        case AE_LONG:  spoutran = (dither_output ? longtran_d : longtran);
                       nzerotran = lzerotran;
                       lloutbufp = (long  *)outbuf;
                       break;
/* --------------------------- IV (Nov 05 2001) ---------------------------> */
        case AE_FLOAT: if ((O.filetyp == TYP_WAV) || (O.filetyp == TYP_AIFF)
                           || (O.filetyp == TYP_AIFC)   /* normalize WAV */
#ifdef RTAUDIO
                           || (audtran == rtplay)
#endif
                           )
                         spoutran = floatran;           /* and AIFF only */
                       else spoutran = floatran_noscl;
                       nzerotran = fzerotran;
                       floutbufp = (float *)outbuf;
                       break;
/* <-------------------------- IV (Nov 05 2001) ---------------------------- */
/*RWD 5:2001 */
        case AE_24INT:
                      /* no need to dither for 24bit writes! */
                       spoutran = int24ptran;                   /* Packed */
                       nzerotran = int24pzerotran;
                       int24outbufp = (char *)outbuf;
                       break;
        default: die(Str(X_1329,"unknown audio_out format"));
        }
}

void sndwrterr(unsigned nret, unsigned nput) /* report soundfile write(osfd)
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

static int audread(char *inbuf, int nbytes) /* diskfile read option for
                                               audrecv's */
                                            /*     assigned during sfopenin() */
{
    return(sreadin(isfd,inbuf,nbytes,p));
}

#if !defined(SYMANTEC) && !defined(mac_classic) && !defined(LINUX) && !defined(__BEOS__) && !defined(__MACH__)
extern int write(int, const void*, unsigned int);
#endif

static void audwrite(char *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    int n;
    if (osfd<0) return;
    if ((n = write(osfd, outbuf, nbytes)) < nbytes)
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
#ifdef CWIN
        char report[40];
        extern void cwin_report_right(char *);
        sprintf(report, "%d(%.3f)", nrecs, nrecs/ekr);
        cwin_report_right(report);
#else
        int n;
        err_printf( "%d(%.3f)%n", nrecs, nrecs/ekr, &n);
        while (n--) err_printf("\b");
#endif
      }
      else err_printf("\a");

    }
    if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
}
#ifdef NeXT /*sbrandon: for RT playback */
static void swaprtplay(char *outbuf, int nbytes)
                                /* soundout write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    bytrev2(outbuf, nbytes);    /* rev bytes in shorts  */
    rtplay(outbuf, nbytes);     /*   & send the data    */
}
#endif

static void audwrtrev2(char *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    bytrev2(outbuf, nbytes);    /* rev bytes in shorts  */
    audwrite(outbuf, nbytes);   /*   & send the data    */
}
/*RWD 5:2001 */
static void audwrtrev3(char *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    bytrev3(outbuf, nbytes);    /* rev bytes in shorts  */
    audwrite(outbuf, nbytes);   /*   & send the data    */
}


/*RWD 3:2000  format fixups: reversed.... */
#define OUTFLOATFAC (FL(1.0) / FL(32768.0))
#define OUTLONGFAC (16)
static void audwrtrev4(char *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    /* RWD 5:2001 removed format fixups! Now in audtran funcs, so good
     * for rtaudio too */
    bytrev4(outbuf, nbytes);    /* rev bytes in longs   */
    audwrite(outbuf, nbytes);   /*   & send the data    */
}

/*RWD 3:2000 .... and non-byte-reversed files*/
static void audwrt4(char *outbuf,int nbytes)
{
    /* RWD 5:2001 removed format fixups! Now in audtran funcs, so good
     * for rtaudio too */

    audwrite(outbuf, nbytes);   /*  & send the data    */
}

void sfopenin(void)             /* init for continuous soundin */
{                               /*    called only if -i flag   */
    HEADATA *hdr = NULL;
    char    *sfname = NULL;
    long     n, readlong = 0;

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
      if ((isfd = openin(O.infilename)) < 0)
        dies(Str(X_947,"isfinit: cannot open %s"), retfilnam);
      sfname = retfilnam;
    }
    p->filetyp = 0;               /* initially non-typed for readheader */
    hdr = readheader(isfd,sfname,p);
    if (hdr != NULL             /* if headerblk returned */
        && !(readlong = hdr->readlong)) {      /* & hadn't readin audio */
      if (hdr->sr != (long)esr &&
          (O.msglevel & WARNMSG)) {              /*    chk the hdr codes  */
        printf(Str(X_607,"WARNING: audio_in %s has sr = %ld, orch sr = %ld\n"),
                sfname, hdr->sr, (long)esr);
      }
      if (hdr->nchanls != nchnls) {
        sprintf(errmsg,Str(X_606,"audio_in %s has %ld chnls, orch %d chnls"),
                sfname, hdr->nchanls, nchnls);
        die(errmsg);
      }
      O.insampsiz = (int)hdr->sampsize;        /*    & cpy header vals  */
      O.informat = (int) hdr->format;
      p->filetyp = hdr->filetyp;
      p->audrem = hdr->audsize;
    }
    else {                                     /* no header:  defaults  */
      if (O.msglevel & WARNMSG) {
        printf(Str(X_54,"WARNING: %s has no soundfile header, assuming %s\n"),
              sfname, getstrformat(O.outformat) );
      }
      p->filetyp = 0;                          /*  (see also soundin.c) */
      p->audrem = -1;
    }
    if ((p->filetyp == TYP_AIFF && bytrevhost()) ||
        (p->filetyp == TYP_AIFC && bytrevhost()) ||
        (p->filetyp == TYP_WAV && !bytrevhost())) {
      if (O.informat == AE_SHORT)             /* if audio_in needs byte rev */
        p->bytrev = bytrev2;                  /*    set on sample size      */
      else if (O.informat == AE_LONG)
        p->bytrev = bytrev4;
      else if (O.informat == AE_FLOAT)
        p->bytrev = bytrev4;
      else p->bytrev = NULL;
      printf(Str(X_1093,"opening %s infile %s, with%s bytrev\n"),
             p->filetyp == TYP_AIFF ? "AIFF" :
             p->filetyp == TYP_AIFC ? "AIFF-C" : "WAV",
             sfname, p->bytrev == NULL ? Str(X_21," no") : "");
    }
    else p->bytrev = NULL;
    audrecv = audread;  /* will use standard audio gets  */

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
    if (readlong) {             /*     & fill it from    */
      *(long *)inbuf = hdr->firstlong;
      n = sreadin(isfd, inbuf+sizeof(long), inbufsiz-sizeof(long), p);
      n += sizeof(long);
    }
    else n = audrecv(inbuf, inbufsiz);          /*     file or devaudio  */
    inbufrem = (unsigned int)(n / O.insampsiz); /* datasiz in monosamps  */
}

void sfopenout(void)                            /* init for sound out       */
{                                               /* (not called if nosound)  */
    extern  void    writeheader(int, char*);

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
    else {
      if ((osfd = openout(O.outfilename, 3)) < 0)   /* else open sfdir or cwd */
        dies(Str(X_1187,"sfinit: cannot open %s"), retfilnam);
      sfoutname = mmalloc((long)strlen(retfilnam)+1);
      strcpy(sfoutname, retfilnam);       /*   & preserve the name */
      if (strcmp(sfoutname, "/dev/audio") == 0) {
        /*      ioctl(   );   */
        pipdevout = 1;
      }
    }
#if defined(SYMANTEC)
    AddMacHeader(sfoutname,nchnls,esr,O.outsampsiz);  /* set Mac resource */
    SetMacCreator(sfoutname);               /*   set creator & file type */
#endif
    if (O.sfheader) {
      writeheader(osfd, sfoutname);       /* write header as required   */
#ifdef mills_macintosh
      transport.osfd = osfd;
      transport.eoheader = lseek(osfd,(off_t)0L,SEEK_CUR);
#endif
    }
    if ((O.filetyp == TYP_AIFF && bytrevhost()) ||
        (O.filetyp == TYP_AIFC && bytrevhost()) ||
        (O.filetyp == TYP_WAV && !bytrevhost())) {
      if (O.outformat == AE_SHORT)        /* if audio out needs byte rev*/
        audtran = audwrtrev2;             /*   redirect the audio puts  */
          /*RWD 3:2000 add AE_FLOAT */
      else if (O.outformat == AE_LONG || O.outformat == AE_FLOAT)
        audtran = audwrtrev4;
          /*RWD 5:2001 */
          else if (O.outformat == AE_24INT)
                  audtran = audwrtrev3;
      else
                  audtran = audwrite;
    }
#ifdef NeXT /*sbrandon*/
# ifdef __LITTLE_ENDIAN__
      if (!O.filetyp && O.outformat == AE_SHORT) /* if audio out needs byte rev*/
        audtran = audwrtrev2;                    /*   redirect the audio puts  */
      else if (!O.filetyp && (O.outformat == AE_LONG || O.outformat == AE_FLOAT))
        audtran = audwrtrev4;
# endif
#endif
    /*RWD 3:2000 non-reversed fixups */
      else                      /* All other cases */
/*         if (O.filetyp == TYP_AIFF && !bytrevhost() || */
/*                  O.filetyp == TYP_AIFC && !bytrevhost() || */
/*                  O.filetyp == TYP_WAV && bytrevhost()   || */
/*               O.filetyp == TYP_IRCAM) */

                audtran = audwrite; /* else use standard audio puts */

#ifdef RTAUDIO
outset:
#endif
    outbufsiz = (unsigned)O.outbufsamps * O.outsampsiz;/* calc outbuf size */
    outbuf = mmalloc((long)outbufsiz); /*  & alloc bufspace */
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

void sfnopenout(void)
{
    printf(Str(X_1079,"not writing to sound disk\n"));
    outbufrem = O.outbufsamps;          /* init counter, though not writing */
}

extern int close(int);

void sfclosein(void)
{
    if (!isfopen) return;
#ifdef RTAUDIO
    if (isfd == DEVAUDIO) {
      if (!osfopen || osfd != DEVAUDIO)
        rtclose();     /* close only if not open for output too */
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
        close(isfd);
    isfopen = 0;
}

void sfcloseout(void)
{
        int     nb;

        if (!osfopen) return;
        if ((nb = (O.outbufsamps-outbufrem) *
             O.outsampsiz) > 0)/* flush outbuffer */
                audtran(outbuf, nb);
#ifdef RTAUDIO
        if (osfd == DEVAUDIO) {
            if (!isfopen || isfd != DEVAUDIO)
                rtclose();     /* close only if not open for input too */
            goto report;
        }
#endif
        if (O.sfheader && !pipdevout) { /* if header, & backward seeks ok */
            unsigned long datasize =
              nb ? (nrecs-1)*outbufsiz + nb : nrecs*outbufsiz;
            rewriteheader(osfd, datasize, 1); /*  rewrite  */
        }
#ifdef PIPES
        if (pout!=NULL) {
          int _pclose(FILE*);
          _pclose(pout);
          pout = NULL;
        }
#endif
#ifndef SFSUN41
        if (!pipdevout)
#endif
          close(osfd);
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

static void bytetran(void)                           /* J. Mohr  1995 Oct 17 */
                            /* same as above, but 8-bit unsigned char output */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
    MYFLT       *sp, *maxampp;
    unsigned long       *maxps;
    long        longsmp, *rngp;
    int         n, spoutrem;
    MYFLT       absamp;

    sp = spout;                 /* adr spout    */
    spoutrem = nspout;          /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;            /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((longsmp = (long)*sp) >= 0) { /* +ive samp:   */
        if (*sp > *maxampp) {           /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;      /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                                    /* ditto -ive samp */
        if ((absamp = -*sp) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *choutbufp++ = (unsigned char)(longsmp >> 8)^0x80;
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void bytetran_d(void)/* J. Mohr  1995 Oct 17 */
                            /* same as above, but 8-bit unsigned char output */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
    MYFLT       *sp, *maxampp;
    unsigned long       *maxps;
    long        longsmp, *rngp;
    int         n, spoutrem;
    MYFLT       absamp;

    sp = spout;                 /* adr spout    */
    spoutrem = nspout;          /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem)/* if nspout remaining > buf rem, */
      n = outbufrem;                    /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)floor(*sp + 128.0*(MYFLT)rand()/ RAND_MAX);
      if ((longsmp) >= 0) {             /* +ive samp:   */
        if (*sp > *maxampp) {           /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;              /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if ((absamp = -*sp) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *choutbufp++ = (unsigned char)(longsmp >> 8)^0x80;
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void shortran(void)      /* fix spout vals and put in outbuf */
{                               /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    MYFLT   longsmp;               /* MUST br MYFLT, as 0dbfs may be 1.0 !*/
    long *rngp;
    int    n, spoutrem;
    MYFLT   absamp;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = *sp;
      if (longsmp >= 0) { /* +ive samp:   */
        if (*sp > *maxampp) {             /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > e0dbfs) {          /* out of range?     */
          longsmp = e0dbfs;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if ((absamp = -*sp) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -e0dbfs) {
          longsmp = -e0dbfs;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *shoutbufp++ = (short) (longsmp * dbfs_to_short);
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
        shoutbufp = (short *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void shortran_d(void)    /* fix spout vals and put in outbuf */
{                               /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   longsmp, *rngp;
    int    n, spoutrem;
    MYFLT   absamp;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);
      if (longsmp >= 0) { /* +ive samp:   */
        if (*sp > *maxampp) {             /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if ((absamp = -*sp) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *shoutbufp++ = (short) longsmp;
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
        shoutbufp = (short *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void chartran(void)      /* same as above, but 8-bit char output */
{                               /*   sends HI-ORDER 8 bits of shortsamp */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   longsmp, *rngp;
    int    n, spoutrem;
    MYFLT   absamp;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)*sp;
      if (longsmp >= 0) { /* +ive samp:   */
        if (*sp > *maxampp) {           /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                                  /* ditto -ive samp */
        if ((absamp = (MYFLT)((long)(-(*sp)))) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *choutbufp++ = (char)(longsmp >> 8);
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void chartran_d(void)    /* same as above, but 8-bit char output */
{                               /*   sends HI-ORDER 8 bits of shortsamp */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   longsmp, *rngp;
    int    n, spoutrem;
    MYFLT   absamp;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)floor(*sp + 128.0*(MYFLT)rand()/ RAND_MAX);
      if (longsmp >= 0) { /* +ive samp:   */
        if (*sp > *maxampp) {           /*  maxamp this seg  */
          *maxampp = *sp;
          *maxps = nframes;
        }
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                                  /* ditto -ive samp */
        if ((absamp = (MYFLT)((long)(-(*sp)))) > *maxampp) {
          *maxampp = absamp;
          *maxps = nframes;
        }
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *choutbufp++ = (char)(longsmp >> 8);
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

#ifdef never
static void alawtran(void)
{
    die(Str(X_590,"alaw not yet implemented"));
}
#endif

#define MUCLIP  32635
#define BIAS    0x84
#define MUZERO  0x02
#define ZEROTRAP

#ifdef ULAW
static void ulawtran(void)      /* ulaw-encode spout vals & put in outbuf */
{                               /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   longsmp, *rngp;
    int    n, spoutrem, sign;
    extern  char    exp_lut[];               /* mulaw encoding table */

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)*sp;
      if (longsmp < 0L) {          /* if sample negative   */
        sign = 0x80;
        longsmp = - longsmp;        /*  make abs, save sign */
      }
      else sign = 0;
      if (longsmp > *maxampp) {             /* save maxamp this seg  */
        *maxampp = (MYFLT)longsmp;
        *maxps = nframes;
      }
      if (longsmp > MUCLIP) {             /* out of range?     */
        longsmp = MUCLIP;           /*   clip and report */
        rngp = rngcnt + (maxampp - maxamp);
        (*rngp)++;
        rngflg = 1;
      }
      if (osfopen) {
        int sample, exponent, mantissa, ulawbyte;
        sample = (int)(longsmp + BIAS);
        exponent = exp_lut[( sample >> 8 ) & 0x7F];
        mantissa = ( sample >> (exponent+3) ) & 0x0F;
        ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
        if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
        *choutbufp++ = ulawbyte;
      }
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void ulawtran_d(void)      /* ulaw-encode spout vals & put in outbuf */
{                                 /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   longsmp, *rngp;
    int    n, spoutrem, sign;
    extern  char    exp_lut[];               /* mulaw encoding table */

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);
      if (longsmp < 0L) {          /* if sample negative   */
        sign = 0x80;
        longsmp = - longsmp;        /*  make abs, save sign */
      }
      else sign = 0;
      if (longsmp > *maxampp) {             /* save maxamp this seg  */
        *maxampp = (MYFLT)longsmp;
        *maxps = nframes;
      }
      if (longsmp > MUCLIP) {             /* out of range?     */
        longsmp = MUCLIP;           /*   clip and report */
        rngp = rngcnt + (maxampp - maxamp);
        (*rngp)++;
        rngflg = 1;
      }
      if (osfopen) {
        int sample, exponent, mantissa, ulawbyte;
        sample = (int)(longsmp + BIAS);
        exponent = exp_lut[( sample >> 8 ) & 0x7F];
        mantissa = ( sample >> (exponent+3) ) & 0x0F;
        ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
        if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
        *choutbufp++ = ulawbyte;
      }
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
        choutbufp = outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}
#endif

static void longtran(void)          /* send long_int spout vals to outbuf */
{                                   /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    int    n, spoutrem;
    MYFLT  absamp,fltsmp,flimit;    /* RWD 1:3:2001 added fltsmp for 0dbfs = 1 */
    long   longsmp, *rngp;          /*RWD Nov 2001 */

    /* IV Jun 2002: added to avoid overflows with 32-bit floats */
    flimit = (sizeof(MYFLT) < 8 ? FL(0.999999) * e0dbfs : e0dbfs);
    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < FL(0.0))
        absamp = -absamp;
      if (absamp > *maxampp) {          /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      fltsmp = (MYFLT) *sp;
      /*RWD Nov 2001 add warnings */
      if (fltsmp >= 0) { /* +ive samp:   */
        if (fltsmp > flimit) {          /* out of range?     */
          fltsmp = flimit;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if (fltsmp < -flimit) {
          fltsmp = -flimit;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      longsmp = (long)(fltsmp * dbfs_to_long);
      if (osfopen)
        *lloutbufp++ = longsmp;
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
        lloutbufp = (long *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void longtran_d(void)            /* send long_int spout vals to outbuf */
{                                       /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    int    n, spoutrem;
    MYFLT  absamp;
    long   longsmp, *rngp;              /*RWD Nov 2001 */

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < FL(0.0))
        absamp = -absamp;
      if (absamp > *maxampp) {          /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      longsmp = (long)floor(*sp + (MYFLT)rand()/ RAND_MAX);

      /*RWD Nov 2001 add warnings */
      if (longsmp >= 0) { /* +ive samp:   */
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;        /*   clip and report */
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if (longsmp < -32768) {
          longsmp = -32768;
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      if (osfopen)
        *lloutbufp++ = longsmp;
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
        lloutbufp = (long *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void floatran(void)              /* send float spout vals to outbuf */
{                                       /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    int    n, spoutrem;
    MYFLT  absamp;
    float       fltsmp;
    long   *rngp;                       /*RWD Nov 2001 */
    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < 0.0f)
        absamp = -absamp;
      if (absamp > *maxampp) {         /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      /*RWD Nov 2001: range translated version: add WARNINGS for over-range */
      fltsmp = (float) *sp;
      if (fltsmp >= 0) { /* +ive samp:   */
        if (fltsmp > e0dbfs) {          /* out of range?     */
          /*   report it*/
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      else {                            /* ditto -ive samp */
        if (fltsmp < -e0dbfs) {
          rngp = rngcnt + (maxampp - maxamp);
          (*rngp)++;
          rngflg = 1;
        }
      }
      fltsmp = (float)(*sp * dbfs_to_float);
      if (osfopen)
        *floutbufp++ = fltsmp;
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
        floutbufp = (float *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

/*RWD 5:2001 */
/* 24:24 packed data */
static void int24ptran(void)
{
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    long   *rngp;
    int    n, spoutrem;
    MYFLT  absamp, fltsmp = FL(0.0);/* RWD 1:3:2001 need fltsmp for 0dbfs = 1 */
    SAMP24   s24;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;

 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < FL(0.0))
        absamp = -absamp;
      if (absamp > *maxampp) {          /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      /*  incorporate overrange detection, before scaling to 24bit range...
       *  but ONLY for WAVE/AIFF/AIFF_C
       */
      if (O.filetyp==TYP_WAV || O.filetyp== TYP_AIFF || O.filetyp== TYP_AIFC) {
        fltsmp = (MYFLT) *sp;
        if (fltsmp >= 0) { /* +ive samp:   */
          if (*sp > *maxampp) {             /*  maxamp this seg  */
            *maxampp = *sp;
            *maxps = nframes;
          }
          if (fltsmp > e0dbfs) {          /* out of range?     */
            fltsmp = e0dbfs;        /*   clip and report */
            rngp = rngcnt + (maxampp - maxamp);
            (*rngp)++;
            rngflg = 1;
          }
        }
        else {                            /* ditto -ive samp */
          if ((absamp = -*sp) > *maxampp) {
            *maxampp = absamp;
            *maxps = nframes;
          }
          if (fltsmp < -e0dbfs) {
            fltsmp = -e0dbfs;
            rngp = rngcnt + (maxampp - maxamp);
            (*rngp)++;
            rngflg = 1;
          }
        }
      }
      s24.lsamp = (long) (fltsmp * dbfs_to_long);

      if (osfopen) {
        /* this is for direct 24bit packed output */
#if defined(_macintosh)||defined(SGI)||defined(LINUX_BE)||defined(LINUX_PPC)
        /* and all similar byte order machines */
        *int24outbufp++ = s24.bytes[0];
        *int24outbufp++ = s24.bytes[1];
        *int24outbufp++ = s24.bytes[2];
#else
        *int24outbufp++ = s24.bytes[1];
        *int24outbufp++ = s24.bytes[2];
        *int24outbufp++ = s24.bytes[3];
#endif
      }
      if (multichan) {
        maxps++;
        if (++maxampp >= maxampend)
          maxampp = maxamp, maxps = maxpos, nframes++;
      }
      else
        nframes++;
      sp++;
    }
    while (--n);
    if (!outbufrem) {
      if (osfopen) {
        audtran(outbuf,outbufsiz);
        int24outbufp = (char *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void int24pzerotran(long kcnt)
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)
      return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do {
        *int24outbufp++ = 0x00;
        *int24outbufp++ = 0x00;
        *int24outbufp++ = 0x00;
      } while (--n);
    }
    else
      int24outbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      int24outbufp = outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem)
        goto nchk;
    }
}

static void szerotran(long kcnt) /* copy kcnt zerospouts to short soundbuf, */
                                /*      sending buffer whenever full     */
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do *shoutbufp++ = (short) 0;
      while (--n);
    }
    else shoutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      shoutbufp = (short *) outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

static void bzerotran(long kcnt)                 /* J. Mohr  1995 Oct 17 */
  /* copy kcnt zerospouts to (unsigned) char soundbuf, */
  /* sending buffer whenever full */
{
    int n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;       /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem) /* if smps remaining > buf rem, */
      n = outbufrem;                    /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;                    /* clear buf only till clean */
      do *choutbufp++ = (const char)0x80;
      while (--n);
    }
    else choutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      choutbufp = outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

static void czerotran(long kcnt)
                  /* copy kcnt zerospouts to (signed) char soundbuf, */
                  /*      sending buffer whenever full     */
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do *choutbufp++ = 0x00;
      while (--n);
    }
    else choutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      choutbufp = outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

#ifdef never
static void azerotran(long kcnt)
{
    die(Str(X_590,"alaw not yet implemented"));
}
#endif

#ifdef ULAW
static void uzerotran(long kcnt)/* copy kcnt zerospouts to ulaw soundbuf, */
                                /*      sending buffer whenever full     */
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do *choutbufp++ = (const char) 0xFF; /* no signal is 0xFF in mulaw */
      while (--n);
    }
    else choutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      choutbufp = outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}
#endif

static void lzerotran(long kcnt)
                           /* copy kcnt zerospouts to long_int soundbuf, */
                           /*      sending buffer whenever full          */
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;                     /*    prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;                     /* clear buf only till clean */
      do *lloutbufp++ = 0L;
      while (--n);
    }
    else lloutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      lloutbufp = (long *) outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
}

static void fzerotran(long kcnt)    /* copy kcnt zerospouts to float soundbuf, */
                                    /*      sending buffer whenever full     */
{
    int   n, smpsrem, clearcnt = 0;

    if (!osfopen)  return;
    smpsrem = nspout * (int)kcnt;        /* calculate total smps to go   */
 nchk:
    if ((n = smpsrem) > (int)outbufrem)  /* if smps remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    smpsrem -= n;
    outbufrem -= n;
    if (clearcnt < O.outbufsamps) {
      clearcnt += n;          /* clear buf only till clean */
      do *floutbufp++ = 0.0f;
      while (--n);
    }
    else floutbufp += n;
    if (!outbufrem) {
      audtran(outbuf,outbufsiz);
      floutbufp = (float *) outbuf;
      outbufrem = O.outbufsamps;
      if (smpsrem) goto nchk;
    }
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

static void byterecv(void)                    /* J. Mohr  1995 Oct 17 */
  /* get spin values from byte inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do *r++ = (MYFLT) ( (short)((*(unsigned char*)chinbufp++)^0x80) << 8 );
    while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          chinbufp = inbuf;
          inbufrem = n / sizeof(char);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

static void charrecv(void)              /* get spin values from char inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do *r++ = (MYFLT) ( (short)*(chinbufp++) << 8 );
    while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          chinbufp = inbuf;
          inbufrem = n / sizeof(char);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

#ifdef never
static void alawrecv(void)
{
    die(Str(X_588,"alaw audio_in not yet implemented"));
}
#endif

#ifdef ULAW
static void ulawrecv(void)              /* get spin values from ulaw inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do *r++ = (MYFLT) ulaw_decode[*(unsigned char *)chinbufp++];
    while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          chinbufp = inbuf;
          inbufrem = n / sizeof(char);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}
#endif

static void shortrecv(void)      /* get spin values from short_int inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do {
      *r++ = short_to_dbfs * (MYFLT) *shinbufp++;
    } while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          shinbufp = (short *) inbuf;
          inbufrem = n / sizeof(short);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

static void longrecv(void)              /* get spin values from long_int inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do {
      *r++ = long_to_dbfs * (MYFLT) *llinbufp++;
    }
    while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          llinbufp = (long *) inbuf;
          inbufrem = n / sizeof(long);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

static void floatrecv(void)              /* get spin values from float inbuf */
{
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;                      /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do {
      *r++ = *flinbufp++ * float_to_dbfs;
    } while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          flinbufp = (float*) inbuf;
          inbufrem = n / sizeof(float);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

static void int24precv(void)     /* get spin values from 24bit packed inbuf */
{
    MYFLT *r = spin;
    SAMP24 s24;
    int   n, spinrem = nspin;

    if (infilend == 2)
      return;
    if (!inbufrem)
      goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;               /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    s24.lsamp = 0L;
    do {
      s24.bytes[1] = *int24inbufp++;
      s24.bytes[2] = *int24inbufp++;
      s24.bytes[3] = *int24inbufp++;
      *r++ = (MYFLT) (s24.lsamp * INLONGFAC);
    }
    while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          int24inbufp = inbuf;
          inbufrem = n / 3;
          if (spinrem)
            goto nchk;
        }
        else
          clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else
        clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

/* --------------------------- IV (Nov 05 2001) ---------------------------> */
static void floatran_noscl(void)         /* send float spout vals to outbuf */
{                                       /*      write buffer when full      */
    MYFLT  *sp, *maxampp;
    unsigned long       *maxps;
    int    n, spoutrem;
    MYFLT  absamp;
    float       fltsmp;

    sp = spout;                     /* adr spout    */
    spoutrem = nspout;              /* smps to go   */
    maxampp = maxamp;
    maxps = maxpos;
 nchk:
    if ((n = spoutrem) > (int)outbufrem) /* if nspout remaining > buf rem, */
      n = outbufrem;          /*      prepare to send in parts  */
    spoutrem -= n;
    outbufrem -= n;
    do {
      if ((absamp = *sp) < 0.0f)
        absamp = -absamp;
      if (absamp > *maxampp) {         /*  maxamp this seg  */
        *maxampp = absamp;
        *maxps = nframes;
      }
      fltsmp = (float) *sp;
      if (osfopen)
        *floutbufp++ = fltsmp;
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
        floutbufp = (float *) outbuf;
      }
      outbufrem = O.outbufsamps;
      if (spoutrem) goto nchk;
    }
}

static void floatrecv_noscl(void)        /* get spin values from float inbuf */
{                                                       /* IV - Jul 11 2002 */
    MYFLT *r = spin;
    int   n, spinrem = nspin;

    if (infilend == 2) return;
    if (!inbufrem)  goto echk;
 nchk:
    if ((n = spinrem) > (int)inbufrem)   /* if nspin remaining > buf rem,  */
      n = inbufrem;                      /*       prepare to get in parts  */
    spinrem -= n;
    inbufrem -= n;
    do {
      *r++ = (MYFLT) *flinbufp++;
    } while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          flinbufp = (float*) inbuf;
          inbufrem = n / sizeof(float);
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
}

/* <-------------------------- IV (Nov 05 2001) ---------------------------- */
