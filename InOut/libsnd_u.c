#include "cs.h"
#include "soundio.h"

#ifdef  USE_DOUBLE
#define sf_write_MYFLT  sf_write_double
#define sf_read_MYFLT   sf_read_double
#else
#define sf_write_MYFLT  sf_write_float
#define sf_read_MYFLT   sf_read_float
#endif

SNDFILE *infile=NULL;
SNDFILE *sndgetset(SOUNDIN *);
char    *getstrformat(int format);
int sreadin(SNDFILE*, MYFLT *, int, SOUNDIN*);

int type2sf(int type)
{
    return (type<<16);
}

short sf2type(int format)
{
   return format>>16;
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
#ifdef SF_FORMAT_SDS
    case TYP_SDS: return "SDS";
#endif
    default:
      return "(unknown)";
    }
}

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

static MYFLT fzero = FL(0.0);

void rewriteheader(SNDFILE* ofd, int verbose)
{
    if (ofd!=NULL)
      sf_command(ofd, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

SNDFILE *SAsndgetset(
     char    *infilnam,                          /* Stand-Alone sndgetset() */
     SOUNDIN **ap,                               /* used by SoundAnal progs */
     MYFLT   *abeg_time,
     MYFLT   *ainput_dur,
     MYFLT   *asr,
     int     channel)
{                               /* Return -1 on failure */
    SOUNDIN *p;
    SNDFILE *infile=NULL;
    char    quotname[80];
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
      return(NULL);
    }
    p->channel = channel;
    p->analonly = 1;
    if ((infile = sndgetset(p)) == NULL)            /* open sndfil, do skiptime */
      return(NULL);
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
    return infile;
}

long getsndin(SNDFILE *fd, MYFLT *fp, long nlocs, SOUNDIN *p)
        /* a simplified soundin */
{
    long  n, nread;
    MYFLT *fbeg = fp, *fend = fp + nlocs, gain;
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
      MYFLT  *inbufp, *bufend;
      inbufp = p->inbufp;
      bufend = p->bufend;
      while (nlocs--) {
        if (inbufp >= bufend) {
          if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
            break;
          inbufp = p->inbuf;
          bufend = p->inbuf + n;
        }
        *fp++ = *inbufp++ * scalefac * gain;
      }
    }
    else {                                /* MULTI-CHANNEL, SELECT ONE */
      int chcnt = 0, chreq = p->channel, nchanls = p->nchanls;
      MYFLT  *inbufp, *bufend;
      inbufp = p->inbufp;
      bufend = p->bufend;
      nlocs *= nchanls;
      while (nlocs--) {
        if (inbufp >= bufend) {
          if ((n = sreadin(fd,p->inbuf,SNDINBUFSIZ,p)) == 0)
            break;
          inbufp = p->inbuf;
          bufend = p->inbuf + n;
        }
        if (++chcnt == chreq)
          *fp++ = *inbufp * scalefac * gain;
        inbufp++;
        if (chcnt == nchanls) chcnt = 0;
      }
    }
    nread = fp - fbeg;
    while (fp < fend)    /* if incomplete */
      *fp++ = FL(0.0);   /*  pad with 0's */
    return (nread);
}

SNDFILE *sndgetset(SOUNDIN *p)  /* core of soundinset                */
                                /* called from sndinset, SAsndgetset, & gen01 */
                                /* Return -1 on failure */
{
    int     n;
    long    framesinbuf, skipframes;
    char    *sfname, soundiname[128];
    int     sinfd=0;
    SNDFILE *infile;
    SF_INFO sfinfo;
    long    filno;

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
    p->nchanls = sfinfo.channels;
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
#ifdef WHEN_LOOPING_WRITTEN
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
      p->format = (short)sf2format(sfinfo.format);
      p->sampframsiz = (short)sfsampsize(sfinfo.format) * sfinfo.channels;
      p->filetyp = sf2type(sfinfo.format);            /* copy type from headata */
      p->aiffdata = NULL;       /* Something to di with looping!! */
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
      printf(Str(X_25,"opening %s infile %s\n"),
             type2string(p->filetyp), sfname);
      if (p->sampframsiz <= 0)                       /* must know framsiz */
        die(Str(X_882,"illegal sampframsiz"));
      p->audrem = sfinfo.frames * sfinfo.channels;
      p->framesrem = sfinfo.frames;    /*   find frames rem */
      skipframes = (long)(*p->iskptim * p->sr);
      framesinbuf = SNDINBUFSIZ / p->sampframsiz;
      if (skipframes < framesinbuf) {              /* if sound within 1st buf */
        int nreq = SNDINBUFSIZ;
        n = sreadin(infile, p->inbuf, nreq, p);
        p->bufend = p->inbuf+n;
        p->inbufp = p->inbuf + skipframes * p->sampframsiz;
      }
      else {                                          /* for greater skiptime: */
        if (sf_seek(infile, (off_t)skipframes, SEEK_SET) < 0)  /* else seek to bndry */
          die(Str(X_1208,"soundin seek error"));
        if ((n = sreadin(NULL,p->inbuf,SNDINBUFSIZ,p)) == 0) /* now rd fulbuf */
          p->endfile = 1;
        p->inbufp = p->inbuf;
        p->bufend = p->inbuf + n;
      }
      if (p->inbufp >= p->bufend)   /* needed? */
        p->endfile = 1;
      if (p->framesrem != -1)
        p->framesrem -= skipframes;                  /* sampleframes to EOF   */
      p->datpos = 0;
      return(infile);                                /* return the active fd  */

 errtn:
      return NULL;                        /*              return empty handed */
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

int sreadin(                    /* special handling of sound input       */
    SNDFILE *infd,              /* to accomodate reads thru pipes & net  */
    MYFLT   *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*  */
    SOUNDIN *p)                 /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    /* return the number of samples read */
    int    n, ntot=0;
    int nsamples = nbytes/sizeof(MYFLT);
    MYFLT *inb = (MYFLT*)inbuf;
    do {
/*       printf("***        : ntot=%d nbytes=%d nsamples=%d nchnls=%d\n", */
/*              ntot, nbytes, nsamples, p->nchanls); */
      n = sf_read_MYFLT(infd, inb+ntot, (nsamples-ntot)/p->nchanls);
/*       printf("***        : n=%d\n", n); */
      if (n<0)
        die(Str(X_1201,"soundfile read error"));
    } while (n > 0 && (ntot += n*p->nchanls) < nsamples);
    if (p->audrem > 0) {        /* AIFF:                  */
      if (ntot > p->audrem)     /*   chk haven't exceeded */
        ntot = p->audrem;       /*   limit of audio data  */
      p->audrem -= ntot*sizeof(MYFLT);
    }
    else ntot = 0;
    return ntot;
}

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

