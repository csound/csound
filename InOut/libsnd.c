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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef _SNDFILE_

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
static  int     isfd, isfopen = 0;              /* (real set in sfopenin)    */
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
extern  void    (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
static  int     (*audrecv)(void *, int);
extern  void    (*audtran)(void *, int);
static  SOUNDIN *p;    /* to be passed via sreadin() */
int sreadin(SNDFILE*, MYFLT *, int, SOUNDIN *);
SNDFILE *sndgetset(SOUNDIN *);

extern  char    *getstrformat(int format);
extern  void    sndwrterr(unsigned, unsigned);
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
    /*    return (type<<16); */
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

short sf2type(int format)
{
/* 
   return format>>16;
   printf("sf2type(%x, %x)\n",format, format&SF_FORMAT_TYPEMASK); */
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
/* printf("format2sf(%d)\n",format); */
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

int sf2format(int type)
{
	/*     printf("sf2format(%x,%x)\n",type,type&SF_FORMAT_SUBMASK); */
	switch (type&SF_FORMAT_SUBMASK) {
		case SF_FORMAT_PCM_S8:
			return AE_CHAR;
#ifdef never
		case SF_FORMAT_ALAW:
			return AE_ALAW;
#endif
#ifdef ULAW
		case SF_FORMAT_ULAW:
			return AE_ULAW;
#endif
		case SF_FORMAT_PCM_16:
			return AE_SHORT;       /* Signed 16 bit data */
		case SF_FORMAT_PCM_32:
			return AE_LONG;       /* Signed 32 bit data */
		case SF_FORMAT_FLOAT:
			return AE_FLOAT;       /* 32 bit float data */
		case SF_FORMAT_PCM_U8:
			return AE_UNCH;       /* Unsigned 8 bit data (WAV and RAW only) */
		case SF_FORMAT_PCM_24:
			return AE_24INT;       /* Signed 24 bit data */
		case SF_FORMAT_DOUBLE:
			return AE_DOUBLE;       /* 64 bit float data */
	}
	return -1;
}


short sfsampsize(int type)
{
	/*     printf("sfsampsize(%x,%x)\n",type, type&SF_FORMAT_SUBMASK); */
	switch (type&SF_FORMAT_SUBMASK) {
		case SF_FORMAT_PCM_S8:
			return 1;
#ifdef never
		case SF_FORMAT_ALAW:
			return 1;
#endif
#ifdef ULAW
		case SF_FORMAT_ULAW:
			return 1;
#endif
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

void writesf(void *outbuf, int nbytes)
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
				printf(/*Str(X_1093,*/"opening %s infile %s\n"/*)*/,
					p->filetyp == TYP_AIFF ? "AIFF" :
				p->filetyp == TYP_AIFC ? "AIFF-C" : "WAV",
					sfname);
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
#if defined(WIN32) || defined(HAVE_LIBASOUND)
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
#if defined(WIN32) || defined(HAVE_LIBASOUND)
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
	infile = 0;
	isfopen = 0;
	return;
}

void sfcloseout(void)
{
	int nb;
	if (!osfopen) return;
	if ((nb = (O.outbufsamps-outbufrem) * O.outsampsiz) > 0)/* flush outbuffer */
		audtran(outbuf, floor(((float) nb) / ((float) nchnls)) * nchnls);
#ifdef RTAUDIO
	if (osfd == DEVAUDIO) {
		if (!isfopen || isfd != DEVAUDIO)
			rtclose();     /* close only if not open for input too */
		goto report;
	}
#endif
	sf_command(outfile, SFC_UPDATE_HEADER_NOW, NULL, 0);
	nb = sf_close(outfile);
	outfile = 0;
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

