/*  
    soundin.c:

    Copyright (C) 1991, 2000 Barry Vercoe, John ffitch, Richard Dobson

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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "cs.h"                 /*                      SOUNDIN.C       */
#include "soundio.h"
#include "oload.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * $Id$
 */

/*RWD 3:2000*/
/* quick hack to save typing etc... */
#undef INLONGFAC
#define INLONGFAC  (double)long_to_dbfs
/* #define INLONGFAC (1.0 / 65536.0)  /\* convert 32bit long to quasi 16 bit range *\/ */
/* #define INMYFLTFAC (FL(32767.0)) */


/* RWD 5:2001 version with 24bit support. defs in soundio.h  */

static  MYFLT   fzero = FL(0.0);
static  long    datpos= 0L;       /* Used in resetting only */

extern  HEADATA *readheader(int, char *, SOUNDIN*);
#ifdef ULAW
extern  short   ulaw_decode[];
#endif
extern  int     bytrevhost(void), openin(char*);
extern  OPARMS  O;
extern  void sndwrterr(int, int);

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

void soundinRESET(void)
{
    datpos = 0;
}

char *getstrformat(int format)  /* used here, and in sfheader.c */
{
    switch(format) {
    case AE_UNCH:  return(Str(X_1356,"unsigned bytes"));   /* J. Mohr 1995 Oct 17 */
    case AE_CHAR:  return(Str(X_1190,"signed chars"));
#ifdef never
    case AE_ALAW:  return(Str(X_589,"alaw bytes"));
#endif
#ifdef ULAW
    case AE_ULAW:  return(Str(X_1304,"ulaw bytes"));
#endif
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

/* RWD 5:2001 */
void bytrev3 (char *buf,int nbytes)
{
    char temp, *p = buf;
    /* only need to swap outer bytes */
    int samps = nbytes/3;

    do {
      temp = p[2];
      p[2] = p[0];
      p[0] = temp;
      p += 3;
    } while (--samps);
}

#ifndef HAVE_LIBSNDFILE

int sreadin(                    /* special handling of sound input       */
    int     infd,               /* to accomodate reads thru pipes & net  */
    char    *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*  */
    SOUNDIN *p)                 /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
        int    n, ntot=0;

        do if ((n = read(infd, inbuf+ntot, nbytes-ntot)) < 0)
                die(Str(X_1201,"soundfile read error"));
        while (n > 0 && (ntot += n) < nbytes);
        if (p->filetyp) {                           /* for AIFF and WAV samples */
                        /*RWD 3:2000 added AIFC */
            if (p->filetyp == TYP_AIFF ||
                p->filetyp==TYP_AIFC ||
                p->filetyp==TYP_WAV) {
                if (p->audrem > 0) {                /* AIFF:                  */
                    if (ntot > p->audrem)           /*   chk haven't exceeded */
                        ntot = p->audrem;           /*   limit of audio data  */
                    p->audrem -= ntot;
                }
                else ntot = 0;
            }
            if (ntot && p->bytrev != NULL)          /* for post-header of both */
                p->bytrev(inbuf, ntot);             /*   bytrev 2 or 4 as reqd */
        }
#ifdef NeXT
# ifdef __LITTLE_ENDIAN__
            if (!p->filetyp && ntot && p->bytrev != NULL)          /* for post-header of both */
                p->bytrev(inbuf, ntot);             /*   bytrev 2 or 4 as reqd */
# endif
#endif
                /*RWD 3:2000: removed my floatsam fixup code: now all handled in soundin, etc */
        return(ntot);
}

int sndgetset(SOUNDIN *p)       /* core of soundinset                */
                                /* called from sndinset, SAsndgetset, & gen01 */
                                /* Return -1 on failure */
{
    int    n;
    HEADATA *hdr;
    long    hdrsize = 0, readlong = 0, framesinbuf, skipframes;
    char    *sfname, soundiname[128];
    int     sinfd;
    long    filno;
    /* RWD 5:2001 : need to set SNDINBUFSIZ according to sample size,
       to support 24bit samples. Just hope for now nobody uses 24:32 format WAVE
       or AIFF, which is supposed to be illegal anyway;
       but very legal for WAVE-EX */
    long sndinbufsiz = SNDINBUFSIZ;

    if ((n = p->OUTOCOUNT) && n > 24) { /* if appl,chkchnls */
      sprintf(errmsg,Str(X_1209,"soundin: illegal no of receiving channels"));
      goto errtn;
    }
    if (*p->ifilno == SSTRCOD) {               /* if char string name given */
      if (p->STRARG == NULL) strcpy(soundiname,unquote(currevent->strarg));
      else strcpy(soundiname,unquote(p->STRARG));    /*     unquote it,  else use */
    }
    else if ((filno = (long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */
    sfname = soundiname;
    if ((sinfd = openin(sfname)) < 0) {      /* open with full dir paths */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_1206,"soundin cannot open %s"), sfname);
      else sprintf(errmsg,Str(X_1205,"soundin cannot find \"%s\" in its search paths"),
                   sfname);
      goto errtn;
    }
    sfname = retfilnam;                      /* & record fullpath filnam */
    if ((p->format = (short)*p->iformat) > 0)       /* convert spec'd format code */
      p->format |= 0x100;
    p->endfile = 0;
    p->filetyp = 0;         /* initially non-typed for readheader */
    if ((hdr=readheader(sinfd,sfname,p)) != NULL /* if headerblk returned */
        && !(readlong = hdr->readlong)) {  /* & hadn't readin audio */
      curr_func_sr = (MYFLT)hdr->sr;
#ifdef never
      if (hdr->filetyp == TYP_AIFF         /*    chk the hdr codes  */
          && hdr->aiffdata != NULL
          && hdr->aiffdata->loopmode1 != 0   /* looping aiff:         */
          && (p->analonly || p->OUTOCOUNT)) {  /*     ok for gen01 only */
        if (O.msglevel & WARNMSG)
          printf(Str(X_586,"WARNING: aiff looping file, once through only\n"));
      }
#endif
      if (p->analonly) {                          /* anal: if sr param val */
        if (p->sr != 0 && p->sr != hdr->sr) {   /*          use it       */
          if (O.msglevel & WARNMSG)
            printf(Str(X_162,"WARNING: -s %ld overriding soundfile sr %ld\n"),
                  p->sr, hdr->sr);
          hdr->sr = p->sr;
        }
      }
      else if (hdr->sr != esr &&
              (O.msglevel & WARNMSG)) {            /* non-anal:  cmp w. esr */
        if (O.msglevel & WARNMSG)
          printf(Str(X_62,"WARNING: %s sr = %ld, orch sr = %7.1f\n"),
                sfname, hdr->sr, esr);
      }
      if (p->OUTOCOUNT) {                          /* for orch SOUNDIN: */
        if (hdr->nchanls != p->OUTOCOUNT) {       /*        chk nchanls */
          if (O.msglevel & WARNMSG) {
            if (O.msglevel & WARNMSG)
              printf(errmsg,Str(X_58, "WARNING: %s nchnls = %d, soundin reading as if nchnls = %d\n"),
                  sfname, (int) hdr->nchanls, (int) p->OUTOCOUNT);
          }
          hdr->nchanls = p->OUTOCOUNT;
        }
      }                                            /* else chk sufficient */
      else if (p->channel != ALLCHNLS && p->channel > hdr->nchanls) {
        sprintf(errmsg,Str(X_1162,"req chan %d, file %s has only %ld"),
                p->channel, sfname, hdr->nchanls);
        die(errmsg);
      }
      if (p->format && hdr->format != p->format &&
          (O.msglevel & WARNMSG)) {
        printf(Str(X_1204,"WARNING: soundin %s superceded by %s header format %s\n"),
               getstrformat((int)p->format), sfname,
               getstrformat((int)hdr->format));
      }
      switch ((p->format = (short)hdr->format)) { /* & copy header data */
      case AE_UNCH:   break;
      case AE_CHAR:   break;
#ifdef ULAW
      case AE_ULAW:   break;
#endif
      case AE_SHORT:  break;
      case AE_LONG:   break;
      case AE_FLOAT:  break;
      case AE_24INT:  break;            /*RWD 5:2001 */
      default:        sprintf(errmsg,Str(X_52,"%s format %s not yet supported"),
                              sfname, getstrformat((int)p->format));
        goto errcls;
      }
      p->sampframsiz = (short)(hdr->sampsize * hdr->nchanls);
      hdrsize = hdr->hdrsize;
      p->filetyp = hdr->filetyp;            /* copy type from headata       */
      p->aiffdata = hdr->aiffdata;
      p->sr = hdr->sr;
      p->nchanls = (short)hdr->nchanls;
    }
    else {                                  /* no hdr:  find info elsewhere */
      if (p->analonly) {
        if (!p->sr) {
          p->sr = (long)DFLT_SR;
          printf(Str(X_1016,
                     "WARNING: no -s and no soundheader, using sr default %ld"),
                  p->sr);
        }
      }
      else {
        printf(Str(X_1062,"WARNING: no soundin header, presuming orchestra sr"));
        p->sr = (long) esr;
      }
      if (p->OUTOCOUNT)
        p->channel = p->OUTOCOUNT;
      else if (p->channel == ALLCHNLS)
        p->channel = 1;
      if (!p->format) {                     /* no format:                  */
        if (p->analonly)                    /*  analonly defaults to short */
          p->format = AE_SHORT;
        else p->format = O.outformat;       /*  orch defaults to outformat */
      }
      if (O.msglevel & WARNMSG)
        printf(Str(X_55,"WARNING: %s has no soundfile header, "
                   "reading as %s, %d chnl%s\n"),
               sfname, getstrformat((int)p->format), (int)p->channel,
               p->channel == 1 ? "" : "s");
      p->sampframsiz = getsizformat((int)p->format) * p->channel;
      p->filetyp = 0;                       /* in_type cannot be AIFF or WAV */
      p->aiffdata = NULL;
      p->nchanls = p->channel;
    }
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

    if ( (p->filetyp == TYP_AIFF && bytrevhost()) ||
         (p->filetyp == TYP_AIFC && bytrevhost()) ||
#ifdef NeXT
         (!p->filetyp && bytrevhost()) ||
#endif
         (p->filetyp == TYP_WAV && !bytrevhost())) {
      if (p->format == AE_SHORT)            /* if audio_in needs byte rev */
        p->bytrev = bytrev2;                /*     set on sample size     */
      /*RWD 3:2000 added AE_FLOAT*/
      else if (p->format == AE_LONG || p->format == AE_FLOAT)
        p->bytrev = bytrev4;
      /*RWD 5:2001 added 24bit */
      else if (p->format == AE_24INT)
        p->bytrev = bytrev3;
      else p->bytrev = NULL;
#ifdef NeXT
      if (!p->filetyp)
        printf(Str(X_1095,"opening NeXT infile %s, with%s bytrev\n"),
               sfname, p->bytrev == NULL ? Str(X_21," no") : "");
      else
#endif
        printf(Str(X_1093,"opening %s infile %s, with%s bytrev\n"),
               p->filetyp == TYP_AIFF ? "AIFF" :
               p->filetyp == TYP_AIFC ? "AIFF-C" : "WAV",
               sfname, p->bytrev == NULL ? Str(X_21, " no") : "");
    }
    else p->bytrev = NULL;
    if (p->sampframsiz <= 0)                           /* must know framsiz */
      die(Str(X_882,"illegal sampframsiz"));
    if (hdr != NULL && hdr->audsize > 0 ) {            /* given audiosize   */
      p->audrem = hdr->audsize;
      p->framesrem = hdr->audsize / p->sampframsiz;    /*   find frames rem */
    }
    else {
      p->audrem = -1;                                  /* else mark unknown */
      p->framesrem = -1;
    }
    skipframes = (long)(*p->iskptim * p->sr);
    /*RWD 5:2001; will probably need new field to disinguish sample size and word size.
      How about: dwBlockAlign? */
    if (p->format == AE_24INT)
      sndinbufsiz = SNDIN24BUFSIZ;
    framesinbuf = sndinbufsiz / p->sampframsiz;
    if (skipframes < framesinbuf) {              /* if sound within 1st buf */
      int nreq;
      if (readlong) {                               /*  fill by direct read */
        nreq = sndinbufsiz - sizeof(long);
        *(long *)p->inbuf = hdr->firstlong;
        n = sreadin(sinfd, p->inbuf+sizeof(long), nreq, p);
        p->bufend = p->inbuf + sizeof(long) + n;
      }
      else {
        nreq = sndinbufsiz;
        n = sreadin(sinfd, p->inbuf, nreq, p);
        p->bufend = p->inbuf + n;
      }
      p->inbufp = p->inbuf + skipframes * p->sampframsiz;
    }
    else {                                          /* for greater skiptime: */
      long nbytes = skipframes * p->sampframsiz;
      if (hdrsize < 0) {
        int nbufs = nbytes/sndinbufsiz;         /* if headersize unknown, */
        int nrem = nbytes - (long)nbufs * sndinbufsiz;
        while (nbufs--)                            /*  spinrd to req boundry */
          sreadin(sinfd,p->inbuf,sndinbufsiz,p);
        if (nrem)
          sreadin(sinfd,p->inbuf,nrem,p);
      }
      else if (lseek(sinfd, (off_t)(nbytes+hdrsize), 0) < 0)  /* else seek to bndry */
        die(Str(X_1208,"soundin seek error"));
      if ((n = sreadin(sinfd,p->inbuf,sndinbufsiz,p)) == 0)/* now rd fulbuf */
        p->endfile = 1;
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
    }
    if (p->inbufp >= p->bufend)   /* needed? */
      p->endfile = 1;
    if (p->framesrem != -1)
      p->framesrem -= skipframes;                  /* sampleframes to EOF   */
    p->datpos = hdrsize;
    return(sinfd);                                 /* return the active fd  */

 errcls:
    close(sinfd);                       /* init error:  close any open file */
 errtn:
    return(-1);                         /*              return empty handed */
}

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
    if ((infd = sndgetset(p)) < 0)            /* open sndfil, do skiptime */
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
    long  n, nread;
    MYFLT *fbeg = fp, *fend = fp + nlocs, gain;
    /*RWD 3:2000: let aiffdata->gainfac take precedence over PEAK
      chunk rescaling */
    /* I assume non-IFF formats don't have the PEAK chunk, for now...*/
    MYFLT scalefac = e0dbfs;

    if (p->aiffdata != NULL && p->aiffdata->gainfac > 0)
      gain = p->aiffdata->gainfac;
    else {
      gain = FL(1.0);
      /*RWD 3:2000*/
      if (p->do_floatscaling)
        scalefac *= p->fscalefac;
    }
    if (p->nchanls == 1 || p->channel == ALLCHNLS) {  /* MONO or ALLCHNLS */
      switch (p->format) {
      case AE_UNCH:
        {
          unsigned char *inbufp, *bufend;
          inbufp = (unsigned char *) p->inbufp;
          bufend = (unsigned char *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (unsigned char *) p->inbuf;
              bufend = (unsigned char *) (p->inbuf + n);
            }
            /* convert unsigned char to signed by XOR 0x80 */
            *fp++ = (MYFLT)( (short)((*inbufp++)^0x80) << 8 );
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      case AE_CHAR:
        {
          char *inbufp, *bufend;
          inbufp = p->inbufp;
          bufend = p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = p->inbuf;
              bufend = p->inbuf + n;
            }
            *fp++ = (MYFLT)( (short)*inbufp++ << 8 ) * gain;
          }
          p->inbufp = inbufp;
          p->bufend = bufend;   /* must reinit after EOF this file */
        }
        break;
#ifdef ULAW
      case AE_ULAW:
        {
          unsigned char *inbufp, *bufend;
          inbufp = (unsigned char *) p->inbufp;
          bufend = (unsigned char *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (unsigned char *) p->inbuf;
              bufend = (unsigned char *) (p->inbuf + n);
            }
            *fp++ = (MYFLT)ulaw_decode[*inbufp++] * gain;
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
#endif
      case AE_SHORT:
        {
          short  *inbufp, *bufend;
          inbufp = (short *) p->inbufp;
          bufend = (short *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (short *) p->inbuf;
              bufend = (short *) (p->inbuf + n);
            }
            *fp++ = short_to_dbfs * (MYFLT)( *inbufp++ * gain);
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        } break;
        /* RWD 5:2001  24bit packed support */
      case AE_24INT:
        {
          SAMP24 s24;
          char *inbufp,*bufend;

          inbufp = p->inbufp;
          bufend = p->bufend;
          s24.lsamp = 0L;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDIN24BUFSIZ,p)) == 0)
                break;
              inbufp = (char *) p->inbuf;
              bufend = (char *) (p->inbuf + n);
            }

            s24.bytes[1] = *inbufp++;
            s24.bytes[2] = *inbufp++;
            s24.bytes[3] = *inbufp++;
            /* we now have a quasi 32bit value */
            /* convert to a quasi 16bit value! */
            *fp++ = (MYFLT)((double)(s24.lsamp) * INLONGFAC ) * gain;
          }
          p->inbufp = inbufp;
          p->bufend = bufend;
        }
        break;
      case AE_LONG:
        {
          long  *inbufp, *bufend;
          inbufp = (long *) p->inbufp;
          bufend = (long *) p->bufend;
          /*RWD 3:2000 fixup formats */
          if (p->filetyp==TYP_WAV || p->filetyp==TYP_AIFF ||
              p->filetyp==TYP_AIFC) {
            MYFLT val;
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (long *) p->inbuf;
                bufend = (long *) (p->inbuf + n);
              }
              val = (MYFLT)((double)*inbufp++ * INLONGFAC);
              *fp++ = val * gain;
            }
          }
          else {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (long *) p->inbuf;
                bufend = (long *) (p->inbuf + n);
              }
              *fp++ = (MYFLT) *inbufp++ * gain;
            }
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      case AE_FLOAT:
        {
          float  *inbufp, *bufend;
          inbufp = (float *) p->inbufp;
          bufend = (float *) p->bufend;
          /*RWD 3:2000 fixup formats */
          if (p->filetyp==TYP_WAV || p->filetyp==TYP_AIFF ||
              p->filetyp==TYP_AIFC) {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (float *) p->inbuf;
                bufend = (float *) (p->inbuf + n);
              }
              *fp++ = (MYFLT)(*inbufp++) * scalefac * gain;
            }
          }
          else {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (float *) p->inbuf;
                bufend = (float *) (p->inbuf + n);
              }
              *fp++ = (MYFLT)(*inbufp++) * gain;
            }
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      default:
        goto getserr;
      }
    }
    else {                                /* MULTI-CHANNEL, SELECT ONE */
      int chcnt = 0, chreq = p->channel, nchanls = p->nchanls;
      nlocs *= nchanls;
      switch (p->format) {
      case AE_UNCH:
        {
          unsigned char *inbufp, *bufend;
          inbufp = (unsigned char *) p->inbufp;
          bufend = (unsigned char *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (unsigned char *) p->inbuf;
              bufend = (unsigned char *) (p->inbuf + n);
            }
            if (++chcnt == chreq)
              *fp++ = (MYFLT) ( (short)(*inbufp^0x80) << 8 );
            inbufp++;
            if (chcnt == nchanls) chcnt = 0;
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      case AE_CHAR:
        {
          char *inbufp, *bufend;
          inbufp = p->inbufp;
          bufend = p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = p->inbuf;
              bufend = p->inbuf + n;
            }
            if (++chcnt == chreq)
              *fp++ = (MYFLT) ( (short)*inbufp << 8 ) * gain;
            inbufp++;
            if (chcnt == nchanls) chcnt = 0;
          }
          p->inbufp = inbufp;
          p->bufend = bufend;
        }
        break;
#ifdef ULAW
      case AE_ULAW:
        {
          unsigned char *inbufp, *bufend;
          inbufp = (unsigned char *) p->inbufp;
          bufend = (unsigned char *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (unsigned char *) p->inbuf;
              bufend = (unsigned char *) (p->inbuf + n);
            }
            if (++chcnt == chreq)
              *fp++ = (MYFLT) ulaw_decode[*inbufp] * gain;
            inbufp++;
            if (chcnt == nchanls) chcnt = 0;
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
#endif
      case AE_SHORT:
        {
          short  *inbufp, *bufend;
          inbufp = (short *) p->inbufp;
          bufend = (short *) p->bufend;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                break;
              inbufp = (short *) p->inbuf;
              bufend = (short *) (p->inbuf + n);
            }
            if (++chcnt == chreq)
              *fp++ = (MYFLT) *inbufp * gain;
            inbufp++;
            if (chcnt == nchanls) chcnt = 0;
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
            } break;
        /* RWD 5:2001  24bit support */
      case AE_24INT:
        {
          SAMP24 s24;
          char *inbufp,*bufend;
          inbufp = p->inbufp;
          bufend = p->bufend;
          s24.lsamp = 0L;
          while (nlocs--) {
            if (inbufp >= bufend) {
              if ((n = sreadin(fd,p->inbuf,SNDIN24BUFSIZ,p)) == 0)
                break;
              inbufp = (char *) p->inbuf;
              bufend = (char *) (p->inbuf + n);
            }

            s24.bytes[1] = *inbufp++;
            s24.bytes[2] = *inbufp++;
            s24.bytes[3] = *inbufp++;
            /* we now have a quasi 32bit value */
            /* convert to a quasi 16bit value! */
            if (++chcnt == chreq)
              *fp++ = (MYFLT)((double)(s24.lsamp) * INLONGFAC ) * gain;
            if (chcnt == nchanls) chcnt = 0;
          }
          p->inbufp = inbufp;
          p->bufend = bufend;
        }
        break;
      case AE_LONG:
        {
          long  *inbufp, *bufend;
          inbufp = (long *) p->inbufp;
          bufend = (long *) p->bufend;
          /*RWD 3:2000 fixup formats */
          if (p->filetyp==TYP_WAV || p->filetyp==TYP_AIFF ||
              p->filetyp==TYP_AIFC) {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (long *) p->inbuf;
                bufend = (long *) (p->inbuf + n);
              }
              if (++chcnt == chreq)
                *fp++ = (MYFLT) (*inbufp * INLONGFAC) * gain;
              inbufp++;
              if (chcnt == nchanls) chcnt = 0;
            }
          }
          else {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (long *) p->inbuf;
                bufend = (long *) (p->inbuf + n);
              }
              if (++chcnt == chreq)
                *fp++ = (MYFLT) *inbufp * gain;
              inbufp++;
              if (chcnt == nchanls) chcnt = 0;
            }
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      case AE_FLOAT:
        {
          float  *inbufp, *bufend;
          inbufp = (float *) p->inbufp;
          bufend = (float *) p->bufend;
          /*RWD 3:2000 fixup formats */
          if (p->filetyp==TYP_WAV || p->filetyp==TYP_AIFF ||
              p->filetyp==TYP_AIFC) {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (float *) p->inbuf;
                bufend = (float *) (p->inbuf + n);
              }
              if (++chcnt == chreq)
                *fp++ = (MYFLT)(*inbufp) * scalefac * gain;
              inbufp++;
              if (chcnt == nchanls) chcnt = 0;
            }
          }
          else {
            while (nlocs--) {
              if (inbufp >= bufend) {
                if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
                  break;
                inbufp = (float *) p->inbuf;
                bufend = (float *) (p->inbuf + n);
              }
              if (++chcnt == chreq)
                *fp++ = (MYFLT)(*inbufp) * gain;
              inbufp++;
              if (chcnt == nchanls) chcnt = 0;
            }
          }
          p->inbufp = (char *) inbufp;
          p->bufend = (char *) bufend;
        }
        break;
      default:
        goto getserr;
      }
    }
    nread = fp - fbeg;
    while (fp < fend)    /* if incomplete */
      *fp++ = FL(0.0);   /*  pad with 0's */
    return(nread);

 getserr:
    printf(Str(X_648,"cannot read sformat %s\n"),
           getstrformat((int)p->format));
    return(-1L);
}

int sndinset(SOUNDIN *p)    /* init routine for instr soundin   */
                             /* shares above sndgetset with SAsndgetset, gen01*/
{
    int     sinfd, reinit = 0;

    if (p->fdch.fd) {   /* if file already open, close it */
          /* RWD: it is not safe to assume all compilers init this to 0 */
          /* (IV: but it is allocated with mcalloc...) */
      /* reload the file */
      reinit++; close(p->fdch.fd);
    }
    p->channel = ALLCHNLS;                   /* reading all channels      */
    p->analonly = 0;
    if ((sinfd = sndgetset(p)) >= 0) {       /* if soundinset successful  */
      p->fdch.fd = sinfd;              /*    store & log the fd     */
      if (!reinit) fdrecord(&p->fdch);       /*    instr will close later */
      p->sampframsiz /= p->OUTOCOUNT;   /* IV - Nov 16 2002 */
    }
    else
      return initerror(errmsg);              /* else just print the errmsg*/
    /* The rest is only used in soundin2, but
     * Does little harm so save code size */
/*         p->base_sample_gab = 0; */
/*         p->fl_buf = 0; */
/*         p->initflag_gab = 1; */
/*         p->phase_gab = (p->OUTOCOUNT == 1 ? 27 : 13) + *p->iskptim * p->sr; */
        /*        p->phase_gab = initphase + *p->iskptim * p->sr; */
    return OK;
}

int soundin(SOUNDIN *p)
{
    MYFLT       *r[24], scalefac;
    int         nsmps, ntogo, blksiz, chnsout, i = 0, n;
    
    if (p->format == AE_FLOAT &&
        (p->filetyp == TYP_WAV || p->filetyp == TYP_AIFF ||
         p->filetyp == TYP_AIFC)) {
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
    memcpy(r, p->r, chnsout * sizeof(MYFLT*));
    ntogo = blksiz;
    if (p->endfile)
      goto filend;
    nsmps = (p->bufend - p->inbufp) / p->sampframsiz;
    if (nsmps > blksiz)
      nsmps = blksiz;
    ntogo -= nsmps;
 sndin:
    switch (p->format) {
    case AE_UNCH: {
      unsigned char *inbufp = (unsigned char *)p->inbufp;
      do {
        *(r[i]++) = (MYFLT)((short)((*inbufp++)^0x80) << 8 ) * short_to_dbfs;
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
      p->inbufp = (char*) inbufp;
      break;
    }
    case AE_CHAR: {
      char *inbufp = (char *)p->inbufp;
      do {
        *(r[i]++) = (MYFLT)((short)(*inbufp++) << 8 ) * short_to_dbfs;
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
      p->inbufp = (char*) inbufp;
      break;
    }
#ifdef ULAW
    case AE_ULAW:
      {
        unsigned char *inbufp = (unsigned char *)p->inbufp;
        do {
          *(r[i]++) = (MYFLT)(ulaw_decode[*inbufp++]) * short_to_dbfs;
          if (++i >= chnsout) i = 0;
        } while (--nsmps);
        p->inbufp = (char*) inbufp;
        break;
      }
#endif
    case AE_SHORT: {
      short *inbufp = (short *)p->inbufp;
      do {
        *(r[i]++) = (MYFLT)(*inbufp++) * short_to_dbfs;
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
      p->inbufp = (char*) inbufp;
      break;
    }
      /* RWD 5:2001 */
    case AE_24INT: {
      unsigned char *inbufp = (unsigned char *)p->inbufp;
      SAMP24 s24;
      s24.lsamp = 0;
      do {
        s24.bytes[1] = *inbufp++;
        s24.bytes[2] = *inbufp++;
        s24.bytes[3] = *inbufp++;
        *(r[i]++) = (MYFLT)(s24.lsamp) * long_to_dbfs;
        if (++i >= chnsout) i = 0;
      } while (--nsmps);
      p->inbufp = (char*) inbufp;
      break;
    }
    case AE_LONG:
      {
        long *inbufp = (long *)p->inbufp;
        do {
          *(r[i]++) = (MYFLT)(*inbufp++) * long_to_dbfs;
          if (++i >= chnsout) i = 0;
        } while (--nsmps);
        p->inbufp = (char*) inbufp;
        break;
      }
    case AE_FLOAT:
      {
        float *inbufp = (float *)p->inbufp;
        do {
          *(r[i]++) = (MYFLT)(*inbufp++) * scalefac;
          if (++i >= chnsout) i = 0;
        } while (--nsmps);
        p->inbufp = (char*) inbufp;
        break;
      }
    default:
      dies(Str(X_1207,"soundin of %s not implemented"),
           getstrformat((int)p->format));
    }
    if (p->inbufp >= p->bufend) {
      if ((n = sreadin(p->fdch.fd, p->inbuf,
                       (p->format == AE_24INT ? SNDIN24BUFSIZ : SNDINBUFSIZ),
                       p)) == 0) {
        p->endfile = 1;
        if (ntogo) goto filend;
        else return OK;
      }
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
      if (ntogo > 0) {
        if ((nsmps = n / p->sampframsiz) > ntogo)
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

#endif
