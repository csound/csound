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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef RTAUDIO
extern  int     (*rtrecord)(void *, int);
extern  void    (*rtplay)(void *, int);
extern  void    (*rtclose)(void);
extern  void    (*recopen)(int, int, float, int);
extern  void    (*playopen)(int, int, float, int);
#endif

static  SNDFILE *outfile, *infile;
static  char    *sfoutname;                     /* soundout filename    */
static  MYFLT   *inbuf;
        MYFLT   *outbuf;                        /* contin sndio buffers */
static  MYFLT   *outbufp;                       /* MYFLT pntr           */
static  unsigned inbufrem, outbufrem;           /* in monosamps         */
                                                /* (see openin,iotranset)    */
static  unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
static  int     isfd, isfopen = 0, infilend = 0;/* (real set in sfopenin)    */
static  int     osfd, osfopen = 0;              /* (real set in sfopenout)   */
static  int     pipdevin = 0, pipdevout = 0;    /* mod by sfopenin,sfopenout */
unsigned long   nframes = 1;
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
extern  void    (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
static  int     (*audrecv)(void *, int);
static  void    (*audtran)(void *, int);
static  SOUNDIN *p;    /* to be passed via sreadin() */
int sreadin(SNDFILE*, MYFLT *, int, SOUNDIN*);
SNDFILE *sndgetset(SOUNDIN *);

extern  char    *getstrformat(int format);
static  void    sndwrterr(unsigned, unsigned);
extern  unsigned long   nframes;

#ifdef  USE_DOUBLE
#define sf_write_MYFLT	sf_write_double
#define sf_read_MYFLT	sf_read_double
#else
#define sf_write_MYFLT	sf_write_float
#define sf_read_MYFLT	sf_read_float
#endif

int type2sf(int type)
{
    return (type<<16);
/*     switch (type) { */
/*     case TYP_WAV: */
/*       return SF_FORMAT_WAV; */
/*     case TYP_AIFF: */
/*       return SF_FORMAT_AIFF; */
/*     case TYP_IRCAM: */
/*       return SF_FORMAT_IRCAM; */
/*     } */
/*     return SF_FORMAT_RAW; */
}

short sf2type(int format)
{
   return format>>16;
   /*   printf("sf2type(%x, %x)\n",format, format&SF_FORMAT_TYPEMASK); */
/*     switch (format&SF_FORMAT_TYPEMASK) { */
/*     case SF_FORMAT_WAV: */
/*       return TYP_WAV; */
/*     case SF_FORMAT_AIFF: */
/*       return TYP_AIFF; */
/*     default: */
/*       { */
/*         char buffer[100]; */
/*         sprintf(buffer, "Unsupported input file type %x\n", format); */
/*         die(buffer); */
/*       } */
/*     case SF_FORMAT_RAW: */
/*       return 0; */
/*     case SF_FORMAT_IRCAM: */
/*       return TYP_IRCAM; */
/*     } */
}

char* type2string(int x)
{
    switch (x) {
    case TYP_WAV: return "WAV";
    case TYP_AIFF: return "AIFF";
    case TYP_AU: return "AU";
    case TYP_RAW: return "RAW";
    case TYP_PAF: return "PAF";
    case TYP_SVX: return "SVX";
    case TYP_NIST: return "NIST";
    case TYP_VOC: return "VOC";
    case TYP_IRCAM: return "IRCAM";
    case TYP_W64: return "W64";
    case TYP_MAT4: return "MAT4";
    case TYP_MAT5: return "MAT5";
    case TYP_PVF: return "PVF";
    case TYP_XI: return "XI";
    case TYP_HTK: return "HTK";
    case TYP_SDS: return "SDS";
    }
}

#define format2sf(x) (x)
/* int format2sf(int format) */
/* { */
/*     switch (format) { */
/*     case AE_CHAR: */
/*       return SF_FORMAT_PCM_S8; */
/* #ifdef never */
/*     case AE_ALAW: */
/*       return SF_FORMAT_ALAW */
/* #endif */
/* #ifdef ULAW */
/*     case AE_ULAW: */
/*       return SF_FORMAT_ULAW */
/* #endif */
/*     case AE_SHORT: */
/*       return SF_FORMAT_PCM_16;       /\* Signed 16 bit data *\/ */
/*     case AE_LONG: */
/*       return SF_FORMAT_PCM_32;       /\* Signed 32 bit data *\/ */
/*     case AE_FLOAT: */
/*       return SF_FORMAT_FLOAT;       /\* 32 bit float data *\/ */
/*     case AE_UNCH: */
/*       return SF_FORMAT_PCM_U8;       /\* Unsigned 8 bit data (WAV and RAW only) *\/ */
/*     case AE_24INT: */
/*       return SF_FORMAT_PCM_24;       /\* Signed 24 bit data *\/ */
/*     case AE_DOUBLE: */
/*       return SF_FORMAT_DOUBLE;       /\* 64 bit float data *\/ */
/*     } */
/*     return SF_FORMAT_PCM_16; */
/* } */

#define sf2format(x) (x)
/* int sf2format(int type) */
/* { */
/* /\*     printf("sf2format(%x,%x)\n",type,type&SF_FORMAT_SUBMASK); *\/ */
/*     switch (type&SF_FORMAT_SUBMASK) { */
/*     case SF_FORMAT_PCM_S8: */
/*       return AE_CHAR; */
/* #ifdef never */
/*     case SF_FORMAT_ALAW: */
/*       return AE_ALAW; */
/* #endif */
/* #ifdef ULAW */
/*     case SF_FORMAT_ULAW: */
/*       return AE_ULAW; */
/* #endif */
/*     case SF_FORMAT_PCM_16: */
/*       return AE_SHORT;       /\* Signed 16 bit data *\/ */
/*     case SF_FORMAT_PCM_32: */
/*       return AE_LONG;       /\* Signed 32 bit data *\/ */
/*     case SF_FORMAT_FLOAT: */
/*       return AE_FLOAT;       /\* 32 bit float data *\/ */
/*     case SF_FORMAT_PCM_U8: */
/*       return AE_UNCH;       /\* Unsigned 8 bit data (WAV and RAW only) *\/ */
/*     case SF_FORMAT_PCM_24: */
/*       return AE_24INT;       /\* Signed 24 bit data *\/ */
/*     case SF_FORMAT_DOUBLE: */
/*       return AE_DOUBLE;       /\* 64 bit float data *\/ */
/*     } */
/*     return -1; */
/* } */


short sfsampsize(int type)
{
/*     printf("sfsampsize(%x,%x)\n",type, type&SF_FORMAT_SUBMASK); */
    switch (type&SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_S8:
      return 1;
    case SF_FORMAT_ALAW:
      return 1;
    case SF_FORMAT_ULAW:
      return 1;
    case SF_FORMAT_PCM_16:
      return 2;       /* Signed 16 bit data */
    case SF_FORMAT_PCM_32:
      return 4;       /* Signed 32 bit data */
    case SF_FORMAT_FLOAT:
      return 4;       /* 32 bit float data */
    case SF_FORMAT_PCM_U8:
      return 1;       /* Unsigned 8 bit data (WAV and RAW only) */
    case SF_FORMAT_PCM_24:
      return 3;       /* Signed 24 bit data */
    case SF_FORMAT_DOUBLE:
      return 8;       /* 64 bit float data */
    }
    return 4;
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

void rewriteheader(int ofd)
{
    if (ofd>=0)
      sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

static void writesf(void *outbuf, int nbytes)
                                /* diskfile write option for audtran's */
                                /*      assigned during sfopenout()    */
{
    int n;
    if (osfd<0) return;
    n = sf_write_MYFLT(outfile, outbuf, nbytes/sizeof(MYFLT));
    if (n < nbytes/sizeof(MYFLT))
      sndwrterr(n, nbytes);
    if (O.rewrt_hdr)
      rewriteheader(osfd);
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

static int readsf(void *inbuf, int nbytes)
{
    return sf_read_MYFLT(infile, inbuf, nbytes/nchnls);
}

static MYFLT fzero = FL(0.0);
int SAsndgetset(
     char    *infilnam,                          /* Stand-Alone sndgetset() */
     SOUNDIN **ap,                               /* used by SoundAnal progs */
     MYFLT   *abeg_time,
     MYFLT   *ainput_dur,
     MYFLT   *asr,
     int     channel)
{                               /* Return -1 on failure */
    SOUNDIN  *p;
    char     quotname[80];
    int      infd;
    static  ARGOFFS  argoffs = {0};     /* these for sndgetset */
    static  OPTXT    optxt;
    static  MYFLT sstrcod = (MYFLT)SSTRCOD;

    sssfinit();                    /* stand-alone init of SFDIR etc. */
    esr = FL(0.0);                 /* set esr 0. with no orchestra   */
    optxt.t.outoffs = &argoffs;    /* point to dummy OUTOCOUNT       */
    *ap = p = (SOUNDIN *) mcalloc((long)sizeof(SOUNDIN));
    p->h.optext = &optxt;
    p->ifilno = &sstrcod;
    p->iskptim = abeg_time;
    p->iformat = &fzero;
    sprintf(quotname,"%c%s%c",'"',infilnam,'"');
    p->STRARG = quotname;
    p->sr = (long)*asr;
/* G. Sullivan This modification is not really complete - calling routines
   should now really be modified to check for channel count > 1, when they
   are not able to handle this case. I have been lazy, and have not yet
   bothered to do this. The reason I made this change was so that
   cvanal could handle stereo, or quad, soundfiles */
    if (channel < 1 || ((channel > 4) && (channel != ALLCHNLS))) {
/*        if (channel < 1 || channel > 4)  { */   /* SAsnd is chan 1,2,3 or 4 */
      printf(Str(X_658,"channel request %d illegal\n"), channel);
      return(-1);
    }
    p->channel = channel;
    p->analonly = 1;
    if ((infile = sndgetset(p)) < 0)            /* open sndfil, do skiptime */
      return(-1);

    if (p->framesrem < 0 ) {
      if (O.msglevel & WARNMSG)
        printf(Str(X_1318,
                   "WARNING: undetermined file length, will attempt requested duration\n"));
    }
    else {
      if (*ainput_dur == FL(0.0)) {         /* 0 durtim, use to EOF */
        p->getframes = p->framesrem;
        *ainput_dur = (MYFLT) p->getframes / p->sr;
      }
      /* else chk that input dur is within filetime rem */
      else if ((p->getframes = (long)(p->sr * *ainput_dur)) > p->framesrem) {
        p->getframes = p->framesrem;
        if (O.msglevel & WARNMSG)
          printf(Str(X_789,"WARNING: full requested duration not available\n"));
      }
      printf(Str(X_598,"analyzing %ld sample frames (%3.1f secs)"),
             p->getframes, *ainput_dur);
      if (*abeg_time != 0.0)
        printf(Str(X_18," from timepoint %3.1f\n"), *abeg_time);
      else printf("\n");
    }
    return(infd);
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

SNDFILE *sndgetset(SOUNDIN *p)  /* core of soundinset                */
                                /* called from sndinset, SAsndgetset, & gen01 */
                                /* Return -1 on failure */
{
    int     n;
    long    hdrsize = 0, framesinbuf, skipframes;
    char    *sfname, soundiname[128];
    int     sinfd;
    SNDFILE *infile;
    SF_INFO sfinfo;
    long    filno;
    long    sndinbufsiz = SNDINBUFSIZ;

    if ((n = p->OUTOCOUNT) && n > 24) { /* if appl,chkchnls */
      sprintf(errmsg,Str(X_1209,"soundin: illegal no of receiving channels"));
      goto errtn;
    }
    if (*p->ifilno == SSTRCOD) {                 /* if char string name given */
      if (p->STRARG == NULL) strcpy(soundiname,unquote(currevent->strarg));
      else strcpy(soundiname,unquote(p->STRARG));    /* unquote it,  else use */
    }
    else if ((filno = (long)*p->ifilno) <= strsmax &&
             strsets != NULL && strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else
      sprintf(soundiname,"soundin.%ld",filno);       /* soundin.filno */
    sfname = soundiname;
    if ((sinfd = openin(sfname)) < 0) {              /* open with full dir paths */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_1206,"soundin cannot open %s"), sfname);
      else sprintf(errmsg,
                   Str(X_1205,"soundin cannot find \"%s\" in its search paths"),
                   sfname);
      goto errtn;
    }
    infile = sf_open_fd(sinfd, SFM_READ, &sfinfo, SF_TRUE);
#ifdef USE_DOUBLE
    sf_command(infile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
    sf_command(infile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    p->fdch.fd = infile;
/*     printf("*** sfinfo: frames=%lld\tsamplerate=%d\tchannels=%d\n" */
/*            "***       : format=%d\tsections=%d\tseekable=%d\n" */
/*            "***       : infile=%p\n", */
/*            sfinfo.frames, sfinfo.samplerate, sfinfo.channels, */
/*            sfinfo.format, sfinfo.sections, sfinfo.seekable, */
/*            p->fdch.fd); */
    sfname = retfilnam;                           /* & record fullpath filnam */
    p->format = sf2format(sfinfo.format);
    if ((short)*p->iformat > 0)   /* convert spec'd format code */
      p->format = ((short)*p->iformat) | 0x100;
    p->endfile = 0;
    p->filetyp = 0;         /* initially non-typed for readheader */
    curr_func_sr = (MYFLT)sfinfo.samplerate;
#ifdef never
      if (hdr->filetyp == TYP_AIFF                  /*    chk the hdr codes  */
          && hdr->aiffdata != NULL
          && hdr->aiffdata->loopmode1 != 0          /* looping aiff:         */
          && (p->analonly || p->OUTOCOUNT)) {       /*     ok for gen01 only */
        if (O.msglevel & WARNMSG)
          printf(Str(X_586,"WARNING: aiff looping file, once through only\n"));
      }
#endif
      if (p->analonly) {                          /* anal: if sr param val */
        if (p->sr != 0 && p->sr != sfinfo.samplerate) {  /*   use it       */
          if (O.msglevel & WARNMSG)
            printf(Str(X_162,"WARNING: -s %ld overriding soundfile sr %ld\n"),
                   p->sr, sfinfo.samplerate);
          /*           sfinfo.samplerate = p->sr; */
        }
      }
      else if (sfinfo.samplerate != esr &&
               (O.msglevel & WARNMSG)) {            /* non-anal:  cmp w. esr */
        if (O.msglevel & WARNMSG)
          printf(Str(X_62,"WARNING: %s sr = %ld, orch sr = %7.1f\n"),
                 sfname, sfinfo.samplerate, esr);
      }
      if (p->OUTOCOUNT) {                            /* for orch SOUNDIN: */
        if (sfinfo.channels != p->OUTOCOUNT) {       /*        chk nchanls */
          if (O.msglevel & WARNMSG) {
            if (O.msglevel & WARNMSG)
              printf(errmsg,
                     Str(X_58, "WARNING: %s nchnls = %d, soundin reading "
                         "as if nchnls = %d\n"),
                     sfname, sfinfo.channels , (int) p->OUTOCOUNT);
          }
          sfinfo.channels = p->OUTOCOUNT;
        }
      }                                            /* else chk sufficient */
      else if (p->channel != ALLCHNLS && p->channel > sfinfo.channels) {
        sprintf(errmsg,Str(X_1162,"req chan %d, file %s has only %ld"),
                p->channel, sfname, sfinfo.channels);
        die(errmsg);
      }
      if (p->format && sf2format(sfinfo.format) != p->format &&
          (O.msglevel & WARNMSG)) {
        printf(Str(X_1204,"WARNING: soundin %s superceded by "
                   "%s header format %s\n"),
               getstrformat((int)p->format), sfname,
               getstrformat((int)sf2format(sfinfo.format)));
      }
      switch ((p->format = (short)sf2format(sfinfo.format))) {
        /* & copy header data */
      case AE_UNCH:
      case AE_CHAR:
#ifdef ULAW
      case AE_ULAW:
#endif
      case AE_SHORT:
      case AE_LONG:
      case AE_FLOAT:
      case AE_24INT:
        break;            /*RWD 5:2001 */
      default:
        sprintf(errmsg,Str(X_52,"%s format %s not yet supported"),
                              sfname, getstrformat((int)p->format));
        goto errcls;
      }
      p->sampframsiz = (short)sfsampsize(sfinfo.format) * sfinfo.channels;
      p->filetyp = sf2type(sfinfo.format);            /* copy type from headata */
      /* ******      p->aiffdata = hdr->aiffdata; */
      p->sr = sfinfo.samplerate;
      p->nchanls = (short)sfinfo.channels;
      if (p->OUTOCOUNT)
        p->channel = p->OUTOCOUNT;
      else if (p->channel == ALLCHNLS)
        p->channel = 1;
      printf(Str(X_604,"audio sr = %ld, "), p->sr);
      if (p->nchanls == 1)
        printf(Str(X_1006,"monaural\n"));
      else {
        printf(Str(X_64,"%s, reading "),
               p->nchanls == 2 ? Str(X_1246,"stereo") :
               p->nchanls == 4 ? Str(X_1148,"quad") :
               p->nchanls == 6 ? Str(X_830,"hex") :
               p->nchanls == 8 ? Str(X_1088,"oct") :
               Str(X_1556,"chanels-"), p->nchanls);
        if (p->channel == ALLCHNLS)
          printf(Str(X_51,"%s channels\n"),
                 p->nchanls == 2 ? Str(X_619,"both") : Str(X_591,"all"));
        else printf(Str(X_655,"channel %d\n"), p->channel);
      }
#ifdef NeXT
      if (!p->filetyp)
        printf(Str(X_1095,"opening NeXT infile %s\n"), sfname);
#endif
      printf("opening %s infile %s\n",
             type2string(p->filetyp), sfname);
      if (p->sampframsiz <= 0)                       /* must know framsiz */
        die(Str(X_882,"illegal sampframsiz"));
      p->audrem = sfinfo.frames * sfinfo.channels;
      p->framesrem = sfinfo.frames;    /*   find frames rem */
      skipframes = (long)(*p->iskptim * p->sr);
      framesinbuf = sndinbufsiz / p->sampframsiz;
      if (skipframes < framesinbuf) {              /* if sound within 1st buf */
        int nreq;
        nreq = sndinbufsiz;
        n = sreadin(infile, p->inbuf, nreq, p);
        p->bufend = p->inbuf+n;
        p->inbufp = p->inbuf + skipframes * p->sampframsiz;
      }
      else {                                          /* for greater skiptime: */
        if (sf_seek(infile, (off_t)skipframes, SEEK_SET) < 0)  /* else seek to bndry */
          die(Str(X_1208,"soundin seek error"));
        if ((n = sreadin(NULL,p->inbuf,sndinbufsiz,p)) == 0) /* now rd fulbuf */
          p->endfile = 1;
        p->inbufp = p->inbuf;
        p->bufend = p->inbuf + n;
      }
      if (p->inbufp >= p->bufend)   /* needed? */
        p->endfile = 1;
      if (p->framesrem != -1)
        p->framesrem -= skipframes;                  /* sampleframes to EOF   */
      p->datpos = hdrsize;
      return(infile);                                /* return the active fd  */
 errcls:
      close(sinfd);                       /* init error:  close any open file */
 errtn:
      return NULL;                        /*              return empty handed */
}

int sreadin(                    /* special handling of sound input       */
    SNDFILE *infd,              /* to accomodate reads thru pipes & net  */
    MYFLT   *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*  */
    SOUNDIN *p)                 /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    /* return the number of samples read */
    int    n, ntot=0;
    SNDFILE *infile = p->fdch.fd;
    int nsamples = nbytes/sizeof(MYFLT);
    MYFLT *inb = (MYFLT*)inbuf;
/*     printf("*** sreadin: %p\n",infile); */
    do {
/*       printf("***        : ntot=%d nbytes=%d reading %d\n", */
/*              ntot, nbytes, (nsamples-ntot)/nchnls); */
      n = sf_read_MYFLT(infile, inb+ntot, (nsamples-ntot)/nchnls);
/*       printf("***        : n=%d\n", n); */
      if (n<0)
        die(Str(X_1201,"soundfile read error"));
    } while (n > 0 && (ntot += n*nchnls) < nsamples);
    if (p->audrem > 0) {        /* AIFF:                  */
      if (ntot > p->audrem)     /*   chk haven't exceeded */
        ntot = p->audrem;       /*   limit of audio data  */
      p->audrem -= ntot*sizeof(MYFLT);
    }
    else ntot = 0;
    return ntot;
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
    if ((sinfd = sndgetset(p)) >= 0) {       /* if soundinset successful  */
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
      audrecv = readsf;  /* will use standard audio gets  */
    }
#ifdef RTAUDIO
 inset:
#endif
    inbufsiz = (unsigned)O.inbufsamps * O.insampsiz;/* calc inbufsize reqd   */
    inbuf = mcalloc((long)inbufsiz); /* alloc inbuf space     */
    printf(Str(X_1151,"reading %d-byte blks of %s from %s (%s)\n"),
           inbufsiz, getstrformat(O.informat), sfname,
           type2string(p->filetyp));
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
#if defined(WIN32) || defined(HAVE_ALSA)
      if (strncmp(O.outfilename,"devaudio", 8) == 0)
        sscanf(O.outfilename+8, "%d", &rtout_dev);
      else if (strncmp(O.outfilename,"dac", 3) == 0)
        sscanf(O.outfilename+3, "%d", &rtout_dev);
#endif
      sfoutname = O.outfilename;
      playopen(nchnls, O.outsampsiz, (float)esr, 2);  /* open devaudio for out */
      audtran = rtplay;                        /* & redirect audio puts */
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
        sf_close(infile);
    isfopen = 0;
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
      printf(" %s\n", type2string(O.filetyp));
    osfopen = 0;
}

/*RWD 3:2000*/
/* quick hack to save typing etc... */
#undef INLONGFAC
#define INLONGFAC  (double)long_to_dbfs
/* #define INLONGFAC (1.0 / 65536.0)  /\* convert 32bit long to quasi 16 bit range *\/ */
/* #define INMYFLTFAC (FL(32767.0)) */


/* RWD 5:2001 version with 24bit support. defs in soundio.h  */

static  long    datpos= 0L;       /* Used in resetting only */

extern  HEADATA *readheader(int, char *, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  int     openin(char*);
extern  OPARMS  O;

void dbfs_init(MYFLT dbfs)
{
    dbfs_to_short = FL(32767.0) / dbfs;
    short_to_dbfs = dbfs / FL(32767.0);
    dbfs_to_float = FL(1.0) / dbfs;
    float_to_dbfs = dbfs / FL(1.0); /* Really!!! */
    dbfs_to_long  = FL(2147483647.0) / dbfs;
    long_to_dbfs  = dbfs / FL(2147483647.0);
    /* probably want this message written just before note messages start... */
    err_printf(Str(X_1715,"0dBFS level = %.1f\n"),dbfs);
}

char *getstrformat(int format)  /* used here, and in sfheader.c */
{
    switch(format) {
    case AE_UNCH:  return(Str(X_1356,"unsigned bytes"));   /* J. Mohr 1995 Oct 17 */
    case AE_CHAR:  return(Str(X_1190,"signed chars"));
    case AE_ALAW:  return(Str(X_589,"alaw bytes"));
    case AE_ULAW:  return(Str(X_1304,"ulaw bytes"));
    case AE_SHORT: return(Str(X_1189,"shorts"));
    case AE_LONG:  return(Str(X_969,"longs"));
    case AE_FLOAT: return(Str(X_769,"floats"));
    case AE_24INT: return "24bit ints";                 /*RWD 5:2001 */
    default:
      {
        char st[80];
        sprintf(st, Str(X_1343,"unknown sound format %d(0x%x)"), format, format);
        die(st); return(NULL);
      }
    }
}

int getsizformat(int format)
{
 static int formatsiz[] = {0, sizeof(char), sizeof(char), sizeof(char),
                            sizeof(short), sizeof(long), sizeof(float),
                            sizeof(char), 3};   /*RWD 5:2001 */
        if (format > AE_LAST)
                die(Str(X_857,"illegal input to getsizformat"));
        return(formatsiz[format & 0xF]);
}

extern  HEADATA *readheader(int, char*, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  OPARMS  O;

#ifdef RTAUDIO
# define DEVAUDIO 0x7fff         /* unique fd for rtaudio  */
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

static int audread(void *inbuf, int nbytes) /* diskfile read option for
                                               audrecv's */
                                            /*     assigned during sfopenin() */
{
    return(sreadin(infile,inbuf,nbytes,p));
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
      rewriteheader(osfd);
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
      *r++ = *inbuf++ * float_to_dbfs;
    } while (--n);
    if (!inbufrem) {
    echk:
      if (!infilend) {
        if ((n = audrecv(inbuf, inbufsiz)) != 0) {
          inbufrem = n;
          if (spinrem) goto nchk;
        }
        else clrspin1(r,spinrem);  /* 1st filend pass: partial clr  */
      }
      else clrspin2();           /* 2nd filend pass: zero the spinbuf */
    }
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
