/*  
    diskin.c:

    Copyright (C) 1998, 2001 matt ingalls, Richard Dobson, John ffitch

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

/*               ugen developed by matt ingalls, ...            */
/*                                                              */
/*      diskin  -       a new soundin that shifts pitch         */
/*              based on the old soundin code                   */
/*              Adjusted to include sndfile library             */
#include "cs.h"
#include "soundio.h"
#include "diskin.h"
#include "oload.h" /* for strset */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* RWD 5:2001 added 24 bit file support */
/* also: my attempt to fix bugs in diskin, unrelated to sample formats
 * specifically: negative pitch: fails to load block at very end of infile
 *               glitch when backwards read arrives at first block of infile
 *               - bad pointer calc
 *               + ~possible~ bug using krate sig for transp
 *               - problems when this is zero?
 */

#ifdef _DEBUG
#include <assert.h>
#endif

#ifdef _SNDFILE_
#ifdef  USE_DOUBLE
#define sf_write_MYFLT	sf_write_double
#define sf_read_MYFLT	sf_read_double
#else
#define sf_write_MYFLT	sf_write_float
#define sf_read_MYFLT	sf_read_float
#endif
extern int type2sf(int);
extern int format2sf(int);
extern  char    *getstrformat(int format);
extern short sf2type(int);
extern short sfsampsize(int);
extern int sf2format(int);

static int sreadinew(           /* special handling of sound input       */
    SNDFILE *infd,              /* to accomodate reads thru pipes & net  */
    MYFLT   *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*                                       */
    SOUNDINEW *p)               /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    int    n, ntot=0;
    int    nsamples = nbytes/sizeof(MYFLT);
    do {
      if ((n = sf_read_MYFLT(infd, inbuf+ntot, (nsamples-ntot)/nchnls)) < 0)
        die(Str(X_1201,"soundfile read error"));
    } while (n > 0 && (ntot += n*nchnls) < nsamples);
    if (p->audrem > 0) {      /* AIFF:                  */
      if (ntot > p->audrem)   /*   chk haven't exceeded */
        ntot = p->audrem;     /*   limit of audio data  */
      p->audrem -= ntot*sizeof(MYFLT);
    }
    else ntot = 0;

    
    /*RWD 3:2000 expanded format fixups ; more efficient here than in
      soundinew() ?  (well, saves a LOT of typing!) */
    if (p->format==AE_FLOAT) {
      int i,cnt;
      float scalefac = (float)INMYFLTFAC;
      float *ptr = (float *) inbuf;
      
      if (p->do_floatscaling)
        scalefac *= p->fscalefac;
      cnt = ntot/sizeof(float);
      for (i=0; i<cnt; i++)
        *ptr++ *= scalefac;
    }

    return(ntot);
}


static int sngetset(SOUNDINEW *p, char *sfname)
{
    int     sinfd;
    SNDFILE *infile;
    SF_INFO sfinfo;
    SOUNDIN forReadHeader;

    if ((sinfd = openin(sfname)) < 0) {     /* open with full dir paths */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_696,"diskin cannot open %s"), sfname);
      else
        sprintf(errmsg,Str(X_695,
                           "diskin cannot find \"%s\" in its search paths"),
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
    p->format = sf2format(sfinfo.format);
    sfname = retfilnam;                        /* & record fullpath filnam */
    if (*p->iformat > 0)  /* convert spec'd format code */
       p->format = ((short)*p->iformat) | 0x100;
    p->endfile = 0;
    p->begfile = 0;
    p->filetyp = 0;             /* initially non-typed for readheader */
    
    /******* construct the SOUNDIN struct to use old readheader ***********/
    forReadHeader.filetyp = p->filetyp;
    forReadHeader.audrem = p->audrem;

    if (sfinfo.samplerate != (int)esr) {                       /* non-anal:  cmp w. esr */
      if (O.msglevel & WARNMSG)
        printf(Str(X_62,"WARNING: %s sr = %ld, orch sr = %7.1f\n"),
               sfname, sfinfo.samplerate, esr);
    }

    if (sfinfo.channels != p->OUTOCOUNT) {         /*        chk nchanls */
      if (O.msglevel & WARNMSG) {
        printf(Str(X_58, "WARNING: %s nchanls = %d, soundin reading as if nchanls = %d\n"),
               sfname, (int) sfinfo.channels, (int) p->OUTOCOUNT);
      }
      sfinfo.channels = p->OUTOCOUNT;
    }

    /***********  copy header data  *************/
    /*RWD 3:2000 copy scalefac stuff */
    p->do_floatscaling = forReadHeader.do_floatscaling;
    p->fscalefac = forReadHeader.fscalefac;
    
    switch ((p->format = (short)sf2format(sfinfo.format))) {
    case AE_CHAR:
    case AE_UNCH:
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
    p->filetyp     = sf2type(sfinfo.format);
    p->sr          = sfinfo.samplerate;
    p->nchanls     = (short)sfinfo.channels;
    p->audrem = p->audsize = sfinfo.frames;
    p->fdch.fd = infile;                  /*     store & log the fd     */
    return (TRUE);

 errcls:
    sf_close(infile);                        /* init error:  close any open file */
 errtn:
    return (FALSE);                      /*              return empty handed */
}


int newsndinset(SOUNDINEW *p)       /* init routine for diskin   */
{
/****************************************************
        revision history
        6/98                    -matt
                fixed headerless file defaults,
                allowed for reinits, and cleaned up code
        8/11/98                 -matt
                made backwards playback and 0 skiptime
                        set skiptime to end of file
        1/26/99                         -matt
                fixed bug when skiptime is default
*****************************************************/
    int     n;
    char    *sfname, soundiname[128];
    SNDFILE *sinfd = NULL;
    long    nbytes, filno;
    MYFLT   skiptime = *p->iskptim;

    /* RWD 5:2001 need this as var, change size to read 24bit data */
    /* should go in SOUNDINEW struct eventually */
    long snewbufsize = SNDINEWBUFSIZ;


    if (skiptime < 0) {
      if (O.msglevel & WARNMSG)
        printf(Str(X_1460,"WARNING: negative skip time, substituting zero.\n"));
      skiptime = FL(0.0);
    }

/* #####RWD: it is not safe to assume all compilers init this to 0 */
    if (p->fdch.fd != 0) {  /* if file already open, rtn */
      /*********** for reinits, we gotta do some stuff here ************/
      /* we get a crash if backwards and 0 skiptime, so lets set it to file
                        end instead..*/
      if (skiptime <= 0 && *p->ktransp < 0) {
        if (p->audsize > 0)
          skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
        else
          skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
      }

      nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
      if (nbytes > p->audrem) { /* RWD says p->audsize but that seems unlikely */
        if (O.msglevel & WARNMSG)
          printf(Str(X_1191,
                    "WARNING: skip time larger than audio data,substituting zero.\n"));
        if ( *p->ktransp < 0) {
          if (p->audsize > 0)
            skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
          else
            skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
          nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
        }
        else
          nbytes = 0;
      }
      p->endfile = 0;
      p->begfile = 0;

      if (nbytes > 0) {
        p->audrem = p->audsize-nbytes+p->firstsampinfile;
      }
      else {
        p->begfile = TRUE;
        if (*p->ktransp < 0)
          p->endfile = TRUE;
        p->audrem = p->audsize;
      }

      /* set file pointer */
      if ((p->filepos =         /* seek to bndry */
           (long)sf_seek(p->fdch.fd,
                         (off_t)(nbytes+p->firstsampinfile), SEEK_SET)) < 0)
        die(Str(X_698,"diskin seek error during reinit"));
      
      if ((n =                  /* now rd fulbuf */
           sreadinew(p->fdch.fd,p->inbuf,
                     snewbufsize/*SNDINEWBUFSIZ*/,p)) == 0)  /*RWD 5:2001 */
        p->endfile = 1;
      
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
      p->guardpt = p->bufend - p->sampframsiz;
      p->phs = 0.0;

      return OK;
    }

    p->channel = ALLCHNLS;      /* reading all channels     */
    p->analonly = 0;

    /********  open the file  ***********/
    if ((n = p->OUTOCOUNT) && n != 1 && n != 2 && n != 4 &&
        n != 6 && n!= 8) {      /* if appl,chkchnls */
      sprintf(errmsg,Str(X_700,"diskin: illegal no of receiving channels"));
      goto errtn;
    }
    if (*p->ifilno == SSTRCOD) { /* if char string name given */
      if (p->STRARG == NULL) strcpy(soundiname,unquote(currevent->strarg));
      else strcpy(soundiname,unquote(p->STRARG));    /* unquote it,  else use */
    }
    else if ((filno=(long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */
    sfname = soundiname;
    printf("**** sfname=%s\n", sfname);
    if (!sngetset(p, sfname))
      return FALSE;
    sinfd  = p->fdch.fd;

    /*******  display messages ####possibly this be verbose mode only??? */
    printf(Str(X_604,"audio sr = %ld, "), p->sr);
    if (p->nchanls == 1)
      printf(Str(X_1006,"monaural\n"));
    else {
      printf(Str(X_64,"%s, reading "),
             p->nchanls == 2 ? Str(X_1246,"stereo") :
             p->nchanls == 4 ? Str(X_1148,"quad") :
             p->nchanls == 6 ? Str(X_830,"hex") : Str(X_1088,"oct") );
      if (p->channel == ALLCHNLS)
        printf(Str(X_51,"%s channels\n"),
               p->nchanls == 2 ? Str(X_619,"both") : Str(X_591,"all"));
      else printf(Str(X_655,"channel %d\n"), p->channel);
    }

    /********  handle byte reversals  *******/
#ifdef NeXT
    if (!p->filetyp)
      printf(/*Str(X_1095,*/"opening NeXT infile %s\n"/*)*/, sfname);
    else
#endif
    printf("opening %s infile %s\n",
      p->filetyp == TYP_AIFF ? "AIFF" :
           p->filetyp == TYP_AIFC ? "AIFF-C" : "WAV", sfname);

    if (p->sampframsiz <= 0)    /* must know framsiz */
      die(Str(X_882,"illegal sampframsiz"));

    /*****  set file pointers, buffers, and diskin-specific stuff  ******/
    /* we get a crash if backwards and 0 skiptime, so lets set it to file
       end instead..*/
    if (skiptime <= 0 && *p->ktransp < 0) {
      if (p->audsize > 0)
        skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
      else
        skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
    }

    nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
                /*#### will this work for all header types??? */
    p->firstsampinfile = sf_seek(sinfd,(off_t)0L,SEEK_CUR);

    if ((p->audrem > 0) && (nbytes > p->audrem)) {
      if (O.msglevel & WARNMSG)
        printf(Str(X_1191,"WARNING: skip time larger than audio data,substituting zero.\n"));
      nbytes = 0;
    }

    if (nbytes > 0) {
      if (p->audsize > 0 )      /* change audsize   */
        p->audrem = p->audsize-nbytes+p->firstsampinfile;

      if ((p->filepos =         /* seek to bndry */
           (long)sf_seek(sinfd, (off_t)(nbytes+p->firstsampinfile), SEEK_SET)) < 0)
        die(Str(X_699,"diskin seek error: invalid skip time"));
    }
    else {
      p->begfile = TRUE;
      if (*p->ktransp < 0)
        p->endfile = TRUE;
    }

    if ((n =                    /* now rd fulbuf */
         sreadinew(sinfd,p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
      p->endfile = 1;
    p->inbufp = p->inbuf;
    p->bufend = p->inbuf + n;

    /*****  if soundinset successful  ********/
    if (sinfd > 0) {
      fdrecord(&p->fdch);              /*     instr will close later */

      p->guardpt = p->bufend - p->sampframsiz;
      p->phs = 0.0;
      return OK;
    }
    else return initerror(errmsg);

 errtn:
    return NOTOK;                      /*              return empty handed */
}


/*  NB: floats not converted here, but in sreadinew():
    handles autorescale from PEAK, etc) */

void soundinew(SOUNDINEW *p)    /*  a-rate routine for soundinew */
{
    MYFLT       *r1, *r2, *r3, *r4, ktransp,looping;
    int         chnsout, n, ntogo, bytesLeft;
    double      phs,phsFract,phsTrunc;
    MYFLT       *inbufp = p->inbufp;
    long snewbufsize = SNDINEWBUFSIZ;            /*RWD 5:2001 */
    long oldfilepos = 0;

#ifdef _DEBUG
    static long samplecount = 0;
    long tellpos;
    short *sbufp1,*sbufp2;
#endif

    if ((!p->bufend) || (!p->inbufp) || (!p->sampframsiz)) {
      initerror(Str(X_701,"diskin: not initialised"));
      return;
    }
    r1      = p->r1;
    r2      = p->r2;
    r3      = p->r3;
    r4      = p->r4;
    ktransp = *p->ktransp;
    looping = *p->ilooping;
    chnsout = p->OUTOCOUNT;
    phs     = p->phs;
    ntogo   = ksmps;
    /*RWD 5:2001 need this when instr dur > filelen*/
    n = 0;
    /* RWD 5:2001 interesting issue - if ktransp starts at zero, we have
     * no idea what direction to go in!  below, it was "if ktransp > 0",
     * but the docs stipulate that only a negative transp signifies
     * backwards rendering, so really, ktransp=0 implies we go forwards */
#ifdef _DEBUG
    if (inbufp != p->bufend)
      assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
    if (ktransp >= 0 ) {        /* forwards... */
/* RWD 5:2001 want to keep phase if reversing mid-data */
      if (phs < 0 && p->begfile)
        phs = 0; /* we have just switched directions, forget (-ve) old phase */
      if (p->endfile) {
        if (p->begfile) p->endfile = FALSE;
        else goto filend;
      }
      
      while (ntogo) {
        /* a lot of the following code has been "written out" for speed */
        switch (chnsout) {
        case 1:
          phsFract = modf(phs,&phsTrunc);
          do {
            *r1++ = *inbufp + (*(inbufp + 4) - *inbufp) * phsFract;
            phs += ktransp;
            phsFract = modf(phs,&phsTrunc);
            inbufp = p->inbufp + (long)(phsTrunc * 4);
            --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
          /*RWD 5:2001*/
          break;
        case 2:
          phsFract = modf(phs,&phsTrunc);
            do {
              *r1++ = *inbufp + (*(inbufp + 8) - *inbufp) * phsFract;
              *r2++ = *(inbufp + 4) + (*(inbufp + 12) - *(inbufp + 4)) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
        case 4:
          phsFract = modf(phs,&phsTrunc);
            do {
              *r1++ = *inbufp + (*(inbufp + 16) - *inbufp) * phsFract;
              *r2++ = *(inbufp + 4) + (*(inbufp + 20) - *(inbufp + 4)) * phsFract;
              *r3++ = *(inbufp + 8) + (*(inbufp + 24) - *(inbufp + 8)) * phsFract;
              *r4++ = *(inbufp + 12)+ (*(inbufp + 28) - *(inbufp + 12)) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
        break;
        }
      }

      bytesLeft = (int)(p->bufend - inbufp);
      if (bytesLeft <= p->sampframsiz) {      /* first set file position
                                                 to where inbuf p "thinks"
                                                 its pointing to */
        p->filepos = (long)sf_seek(p->fdch.fd,(off_t)(-bytesLeft),SEEK_CUR);
        if ((n = sreadinew(p->fdch.fd,
                           p->inbuf,snewbufsize,p)) == 0) {  /*RWD 5:2001 */
          if (looping) {
            /* go to beginning of file.
               depending on the pitch and
               phase, we might drop a few "guardpoint" samples, but
               this ugen is intended for large files anyway -- if a
               few end samples are critical for looping, use oscil or
               table!!!!  */
            p->audrem = p->audsize;
            p->filepos = (long)sf_seek(p->fdch.fd,
                                       (off_t)p->firstsampinfile,SEEK_SET);
            if ((n = sreadinew(p->fdch.fd,
                               p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
              die(Str(X_733,"error trying to loop back to the beginning "
                      "of the sound file!?!??"));
            p->begfile = 1;
            phs = 0;
            inbufp = p->inbufp = p->inbuf;
            p->bufend = p->inbuf + n;
            /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
             * when a transp sweep hits eof), but not, I suspect, the
             * underlying cause */
            if (n < snewbufsize)
              memset(p->bufend,0,snewbufsize-n);
            p->guardpt = p->bufend - p->sampframsiz;
          }
          else {
            p->endfile = TRUE;
            goto filend;
          }
        }
        else {
          inbufp = p->inbufp = p->inbuf;
          p->bufend = p->inbuf + n;
          /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
           * when a transp sweep hits eof), but not, I suspect, the
           * underlying cause */
            if (n < snewbufsize)
              memset(p->bufend,0,snewbufsize-n);
            p->guardpt = p->bufend - p->sampframsiz;
            phs = modf(phs,&phsTrunc);
            p->begfile = FALSE;
        }
      }
#ifdef _DEBUG
      if (inbufp != p->bufend)
        assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
    }
    else {      /* backwards...                 same thing but different */
      if (phs > 0 && p->endfile)  /*RWD 5:2001 as above */
        phs = 0; /* have just switched directions, forget (+ve) old phase */
      
      if (p->endfile) {   /* firewall-flag signaling when we are at either
                             end of the file */
        if (p->begfile)
          goto filend; /* make sure we are at beginning, not end */
        else {         /* RWD 5:2001: read in the first block (= last block of infile) */
          bytesLeft = (int)(inbufp - p->inbuf);
          if ((p->filepos = (long)sf_seek(p->fdch.fd,
                                          (off_t)(bytesLeft-snewbufsize),
                                          SEEK_CUR)) <= p->firstsampinfile) {
            p->filepos = (long)sf_seek(p->fdch.fd,
                                       (off_t)p->firstsampinfile,SEEK_SET);
            p->begfile = 1;
          }
          
          /* RWD 5:2001 but don't know if this is required here... */
          p->audrem = p->audsize; /* a hack to prevent errors (returning
                                     'ntot')in the sread for AIFF */
          if ((n = sreadinew(p->fdch.fd,p->inbuf,snewbufsize,p)) !=
              snewbufsize) {
            /* we should never get here. if we do,
               we're fucked because didn't get a full buffer and our
               present sample is the last sample of the buffer!!!  */
            die(Str(X_697,"diskin read error - during backwards playback"));
            return;
          }
          /* now get the correct remaining size */
          p->audrem = p->audsize - p->firstsampinfile - p->filepos;
          p->bufend = p->inbuf + n;
          /* point to the last sample in buffer */
          inbufp = p->inbufp = p->guardpt = p->bufend - p->sampframsiz;
          
          /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
           * when a transp sweep hits eof), but not, I suspect, the
           * underlying cause */
          if (n < snewbufsize)
            memset(p->bufend,0,snewbufsize-n);
          phs = modf(phs,&phsTrunc);
          p->endfile = FALSE;
        }
      }

      while (ntogo) {
        switch(chnsout) {
        case 1:
          phsFract = modf(phs,&phsTrunc); /* phsFract and phsTrunc will be
                                             non-positive */
          do {
              *r1++ = *inbufp + (*inbufp - *(inbufp - 4)) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
        case 2:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
            do {
              *r1++ = *inbufp + (*inbufp - *(inbufp - 8)) * phsFract;
              *r2++ = *(inbufp + 4) + (*(inbufp + 4) - *(inbufp - 4)) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
        case 4:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
          do {
              *r1++ = *inbufp + (*inbufp - *(inbufp - 16)) * phsFract;
              *r2++ = *(inbufp + 4) + (*(inbufp + 4) - *(inbufp - 12)) * phsFract;
              *r3++ = *(inbufp + 8) + (*(inbufp + 8) - *(inbufp - 8)) * phsFract;
              *r4++ = *(inbufp + 12)+ (*(inbufp + 12) - *(inbufp - 4)) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
          } while ((inbufp > p->inbuf) && (ntogo));
          break;
        }
        
        if (inbufp <= p->inbuf) { /* we need to get some more samples!! */
          if (p->begfile) {
            if (looping) {      /* hopes this works -- set 1 buffer lenght
                                   at end of sound file */
              p->filepos =
                (long)sf_seek(p->fdch.fd,
                              (off_t)(p->firstsampinfile+p->audsize-snewbufsize),
                              SEEK_SET);   /*RWD 5:2001*/
              phs = -0.0;
              p->begfile = 0;
            }
            else {
              p->endfile = 1;
              goto filend;
            }
          }
          else {
            bytesLeft = (int)(inbufp - p->inbuf + p->sampframsiz);
            /* we're going backwards, so bytesLeft should be
             * non-positive because inbufp should be pointing
             * to the first sample in the buffer or "in front"
             * the buffer.  But we must add a sample frame
             * (p->sampframsiz) to make sure the sample we are
             * pointing at right now becomes the last sample
             * in the next buffer*/
            /*RWD remember this for when lseek returns -1 */
            oldfilepos = p->filepos;
            
            if ((p->filepos =
                 (long)sf_seek(p->fdch.fd,
                               (off_t)(bytesLeft-snewbufsize - snewbufsize),
                               /*RWD 5:2001 was SNDINEWBUFSIZ*/
                               SEEK_CUR)) <= p->firstsampinfile) {
              p->filepos = (long)sf_seek(p->fdch.fd,
                                         (off_t)p->firstsampinfile,SEEK_SET);
              p->begfile = 1;
            }
          }
          
          p->audrem = p->audsize; /* a hack to prevent errors (returning
                                     'ntot') in the sread for AIFF */
          
          if ((n = sreadinew(p->fdch.fd,p->inbuf,snewbufsize,p)) !=
              snewbufsize) {       /* RWD 4:2001 was SNDINEWBUFSIZ*/
            /* we should never get here. if we do,
               we're fucked because didn't get a full buffer and our
               present sample is the last sample of the buffer!!!  */
            die(Str(X_697,"diskin read error - during backwards playback"));
            return;
          }
          /* now get the correct remaining size */
          p->audrem = p->audsize - p->firstsampinfile - p->filepos;
          /* RWD 5:2001  this clears a glitch doing
           * plain reverse looping (pitch  = -1) over file
           */
          if (p->begfile)
            n = oldfilepos - p->firstsampinfile;
          p->bufend = p->inbuf + n;
          /* point to the last sample in buffer */
          inbufp = p->inbufp = p->guardpt = p->bufend - p->sampframsiz;
          /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
           * when a transp sweep hits eof), but not, I suspect, the
           * underlying cause */
          if (n < snewbufsize)
            memset(p->bufend,0,snewbufsize-n);
          phs = modf(phs,&phsTrunc);
        }
      }
    }
    p->inbufp = inbufp;
    p->phs = modf(phs,&phsTrunc);
    return;

 filend:
    if (ntogo > n) {            /* At RWD's suggestion */
      switch(chnsout) {                   /* if past end of file, */
      case 1:
        do *r1++ = FL(0.0);               /*    move in zeros     */
        while (--ntogo);
        break;
      case 2:
        do {
          *r1++ = FL(0.0);
          *r2++ = FL(0.0);
        } while (--ntogo);
        break;
      case 4:
        do {
          *r1++ = FL(0.0);
          *r2++ = FL(0.0);
          *r3++ = FL(0.0);
          *r4++ = FL(0.0);
        } while (--ntogo);
      }
    }
}

/* RWD:DBFS: NB: thse funcs all supposed to write to a 'raw' file, so
   what will people want for 0dbfs handling? really need to update
   opcode with more options. */

int sndo1set(SNDOUT *p)            /* init routine for instr soundout   */
{
    int    soutfd, filno;
    char   *sfname, sndoutname[128];
    SF_INFO sfinfo;
    SNDFILE *outfile;

    if (p->c.fdch.fd != NULL)   return OK;        /* if file already open, rtn  */
    if (*p->c.ifilcod == SSTRCOD)
      strcpy(sndoutname, unquote(p->STRARG));
    else if ((filno = (int)*p->c.ifilcod) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(sndoutname, strsets[filno]);
    else
      sprintf(sndoutname,"soundout.%d", filno);
    sfname = sndoutname;
    if ((soutfd = openout(sfname, 1)) < 0) {   /* if openout successful */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_1212,"soundout cannot open %s"), sfname);
      else
        sprintf(errmsg,Str(X_1211,"soundout cannot find %s in search paths"),
                sfname);
      goto errtn;
    }
    sfinfo.frames = -1;
    sfinfo.samplerate = (int)esr;
    sfinfo.channels = nchnls;        /* WRONG *************************** */
    sfinfo.format = type2sf(p->c.filetyp)|format2sf(p->c.format);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfile = sf_open_fd(soutfd, SFM_WRITE, &sfinfo, SF_TRUE);
    sfname = retfilnam;
    if ((p->c.format = (short)*p->c.iformat) > 0)
      p->c.format |= 0x100;

    printf(/*Str(X_1094,*/"opening %s outfile %s\n"/*)*/,
             p->c.filetyp==TYP_AIFF ? "AIFF":
             p->c.filetyp==TYP_AIFC ? "AIFF-C":"WAV",
             sfname);
    p->c.outbufp = p->c.outbuf;         /* fix - isro 20-11-96 */
    p->c.bufend = p->c.outbuf + SNDOUTSMPS; /* fix - isro 20-11-96 */
    p->c.fdch.fd = outfile;                  /*     store & log the fd     */
    fdrecord(&p->c.fdch);                   /*     instr will close later */
    return OK;
 errtn:
    return initerror(errmsg);               /* else just print the errmsg */
}

int soundout(SNDOUT *p)
{
    MYFLT  *outbufp, *asig;
    int    nn, nsamps, ospace;

    asig = p->asig;
    outbufp = p->c.outbufp;
    nsamps = ksmps;
    ospace = (p->c.bufend - outbufp);
 nchk:
    if ((nn = nsamps) > ospace)
      nn = ospace;
    nsamps -= nn;
    ospace -= nn;
    do  *outbufp++ = *asig++;
    while (--nn);
    if (!ospace) {              /* when buf is full  */
      sf_read_MYFLT(p->c.fdch.fd, p->c.outbuf, p->c.bufend - p->c.outbuf);
      outbufp = p->c.outbuf;
      ospace = SNDOUTSMPS;
      if (nsamps) goto nchk;    /*   chk rem samples */
    }
    p->c.outbufp = outbufp;
    return OK;
}

void sndo2set(SNDOUTS *p)
{
    IGN(p);
}

void soundouts(SNDOUTS *p)
{
    IGN(p);
}

#else
extern  HEADATA *readheader(int, char *, SOUNDIN*);
extern  void bytrev2(char*, int);
extern  void bytrev4(char*, int);
extern  char *getstrformat(int);
extern  int getsizformat(int);
extern  int bytrevhost(void);
extern  void sndwrterr(unsigned, unsigned);

/*RWD 5:2001 */
extern void bytrev3(char *,int);        /* in soundin.c */


static int sreadinew(           /* special handling of sound input       */
    int     infd,               /* to accomodate reads thru pipes & net  */
    char    *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nbytes,             /*                                       */
    SOUNDINEW *p)               /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    int    n, ntot=0;

    do {
      if ((n = read(infd, inbuf+ntot, nbytes-ntot)) < 0)
        die(Str(X_1201,"soundfile read error"));
    } while (n > 0 && (ntot += n) < nbytes);
    if (p->filetyp
#ifdef NeXT
        || 1
#endif
        ) {             /* for AIFF and WAV samples */
      if (p->filetyp == TYP_AIFF ||
          p->filetyp == TYP_AIFC ||
          p->filetyp == TYP_WAV) {     /*RWD 3:2000*/
        if (p->audrem > 0) {      /* AIFF:                  */
          if (ntot > p->audrem)   /*   chk haven't exceeded */
            ntot = p->audrem;     /*   limit of audio data  */
          p->audrem -= ntot;
        }
        else ntot = 0;
      }
#ifndef _SNDFILE_
      if (ntot && p->bytrev != NULL)        /* for post-header of both */
        p->bytrev(inbuf, ntot);             /*   bytrev 2 or 4 as reqd */
#endif
    }
    /*RWD 3:2000 expanded format fixups ; more efficient here than in
      soundinew() ?  (well, saves a LOT of typing!) */
    if (p->filetyp==TYP_WAV  ||
        p->filetyp==TYP_AIFF ||
        p->filetyp==TYP_AIFC) {
      if (p->format==AE_FLOAT) {
        int i,cnt;
        float scalefac = (float)INMYFLTFAC;
        float *ptr = (float *) inbuf;

        if (p->do_floatscaling)
          scalefac *= p->fscalefac;
        cnt = ntot/sizeof(float);
        for (i=0; i<cnt; i++)
          *ptr++ *= scalefac;
      }
      else if (p->format==AE_LONG) {
        int i;
        int cnt = ntot/sizeof(long);
        long *ptr = (long*) inbuf;
        for (i=0; i<cnt; i++) {
          *ptr = (long) ((double) *ptr *  INLONGFAC);
          ptr++;
        }
      }
    }
    return(ntot);
}


static int sngetset(SOUNDINEW *p, char *sfname)
{
    HEADATA *hdr = NULL;
    int     sinfd = 0;
    SNDFILE *infile;
    SOUNDIN forReadHeader;
    SF_INFO sfinfo;
    long readlong = 0;

    if ((sinfd = openin(sfname)) < 0) {     /* open with full dir paths */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_696,"diskin cannot open %s"), sfname);
      else
        sprintf(errmsg,Str(X_695,
                           "diskin cannot find \"%s\" in its search paths"),
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
    sfname = retfilnam;                        /* & record fullpath filnam */
    p->format = sf2format(sfinfo.format);
    if ((p->format = (short)*p->iformat) > 0)   /* convert spec'd format code */
      p->format |= 0x100;
    p->endfile = 0;
    p->begfile = 0;
    p->filetyp = 0;             /* initially non-typed for readheader */

    /******* construct the SOUNDIN struct to use old readheader ***********/
    forReadHeader.filetyp = p->filetyp;
#ifndef _SNDFILE_
    forReadHeader.bytrev = p->bytrev;
#endif
    forReadHeader.audrem = p->audrem;

    if (sfinfo.sr != esr) {                       /* non-anal:  cmp w. esr */
        if (O.msglevel & WARNMSG)
          printf(Str(X_62,"WARNING: %s sr = %ld, orch sr = %7.1f\n"),
                 sfname, sfinfo.sr, esr);
      }

      if (sfinfo.channels != p->OUTOCOUNT) {         /*        chk nchanls */
        if (O.msglevel & WARNMSG) {
          printf(Str(X_58, "WARNING: %s nchanls = %d, soundin reading as if nchanls = %d\n"),
                sfname, sfinfo.channels, (int) p->OUTOCOUNT);
        }
        sfinfo.channels = p->OUTOCOUNT;
      }

      if (p->format && sf2format(sfinfo.format) != p->format &&
          (O.msglevel & WARNMSG)) {   /*    chk format */
        printf(Str(X_694,"WARNING: diskin %s superceded by %s header format %s\n"),
                getstrformat((int)p->format), sfname,
                getstrformat((int)sf2format(sfinfo.format)));
      }

      /***********  copy header data  *************/
          /*RWD 3:2000 copy scalefac stuff */
      p->do_floatscaling = forReadHeader.do_floatscaling;
      p->fscalefac = forReadHeader.fscalefac;

      switch ((p->format = (short)sf2format(sfinfo.format))) {
      case AE_CHAR:
      case AE_UNCH:
#ifdef ULAW
      case AE_ULAW:
#endif
      case AE_SHORT:
      case AE_LONG:
      case AE_FLOAT:
      case AE_24INT:
        break;            /*RWD 5:2001 */

      default: sprintf(errmsg,Str(X_52,"%s format %s not yet supported"),
                       sfname, getstrformat((int)p->format));
        goto errcls;
      }
      p->sampframsiz = (short)sfsampsize(sfinfo.format) * sfinfo.channels;
      p->filetyp     = sf2type(sfinfo.format);
      /* ******      p->aiffdata    = hdr->aiffdata; */
      p->sr          = sfinfo.samplerate;
      p->nchanls     = (short)sfinfo.channels;
      p->audrem = sfinfo.frames * sfinfo.channels; 
      if (O.msglevel & WARNMSG) {
        printf(Str(X_55,
                   "WARNING: %s has no soundfile header, reading as %s, %d chnl%s"),
               sfname, getstrformat((int)p->format), (int)p->channel,
               p->channel == 1 ? "" : "s");
      }

      p->sampframsiz = getsizformat((int)p->format) * p->channel;
      p->filetyp     = 0;           /*  in_type cannot be AIFF or WAV */
      p->aiffdata    = NULL;
      p->nchanls     = p->channel;
      p->audrem      = p->audsize  = -1;      /* mark unknown */
    }

    p->fdch.fd = sinfd;              /*     store & log the fd     */
    return(TRUE);

 errcls:
    close(sinfd);               /* init error:  close any open file */
 errtn:
    return(FALSE);           /*              return empty handed */
}


int newsndinset(SOUNDINEW *p)       /* init routine for diskin   */
{
/****************************************************
        revision history
        6/98                    -matt
                fixed headerless file defaults,
                allowed for reinits, and cleaned up code
        8/11/98                 -matt
                made backwards playback and 0 skiptime
                        set skiptime to end of file
        1/26/99                         -matt
                fixed bug when skiptime is default
*****************************************************/
    int     n;
    char    *sfname, soundiname[128];
    int     sinfd = 0;
    long    nbytes, filno;
    MYFLT   skiptime = *p->iskptim;

    /* RWD 5:2001 need this as var, change size to read 24bit data */
    /* should go in SOUNDINEW struct eventually */
    long snewbufsize = SNDINEWBUFSIZ;


    if (skiptime < 0) {
      if (O.msglevel & WARNMSG)
        printf(Str(X_1460,"WARNING: negative skip time, substituting zero.\n"));
      skiptime = FL(0.0);
    }

/* #####RWD: it is not safe to assume all compilers init this to 0 */
    if (p->fdch.fd != 0) {  /* if file already open, rtn */
      /*********** for reinits, we gotta do some stuff here ************/
      /* we get a crash if backwards and 0 skiptime, so lets set it to file
                        end instead..*/
      if (skiptime <= 0 && *p->ktransp < 0) {
        if (p->audsize > 0)
          skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
        else
          skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
      }

      nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
      if (nbytes > p->audrem) { /* RWD says p->audsize but that seems unlikely */
        if (O.msglevel & WARNMSG)
          printf(Str(X_1191,
                    "WARNING: skip time larger than audio data,substituting zero.\n"));
        if ( *p->ktransp < 0) {
          if (p->audsize > 0)
            skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
          else
            skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
          nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
        }
        else
          nbytes = 0;
      }
/*       if (nbytes > p->audrem) { */
/*         if (O.msglevel & WARNMSG) */
/*           printf(Str(X_1191, */
/*                "WARNING: skip time larger than audio data,substituting zero.\n")); */
/*         nbytes = 0; */
/*       } */

      p->endfile = 0;
      p->begfile = 0;

      if (nbytes > 0) {
        p->audrem = p->audsize-nbytes+p->firstsampinfile;
      }
      else {
        p->begfile = TRUE;
        if (*p->ktransp < 0)
          p->endfile = TRUE;
        p->audrem = p->audsize;
      }

      /* set file pointer */
      if ((p->filepos =         /* seek to bndry */
           (long)lseek(p->fdch.fd,
                       (off_t)(nbytes+p->firstsampinfile), SEEK_SET)) < 0)
        die(Str(X_698,"diskin seek error during reinit"));

      if ((n =                  /* now rd fulbuf */
           sreadinew(p->fdch.fd,p->inbuf,
                     snewbufsize/*SNDINEWBUFSIZ*/,p)) == 0)  /*RWD 5:2001 */
        p->endfile = 1;

      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
      p->guardpt = p->bufend - p->sampframsiz;
      p->phs = 0.0;

      return OK;
    }

    p->channel = ALLCHNLS;      /* reading all channels     */
    p->analonly = 0;

    /********  open the file  ***********/
    if ((n = p->OUTOCOUNT) && n != 1 && n != 2 && n != 4 &&
        n != 6 && n!= 8) {      /* if appl,chkchnls */
      sprintf(errmsg,Str(X_700,"diskin: illegal no of receiving channels"));
      goto errtn;
    }
    if (*p->ifilno == SSTRCOD) { /* if char string name given */
      if (p->STRARG == NULL) strcpy(soundiname,unquote(currevent->strarg));
      else strcpy(soundiname,unquote(p->STRARG));    /* unquote it,  else use */
    }
    else if ((filno=(long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */
    sfname = soundiname;
    if (!sngetset(p, sfname))
      return OK;
    sinfd  = p->fdch.fd;

    /*******  display messages ####possibly this be verbose mode only??? */
    printf(Str(X_604,"audio sr = %ld, "), p->sr);
    if (p->nchanls == 1)
      printf(Str(X_1006,"monaural\n"));
    else {
      printf(Str(X_64,"%s, reading "),
             p->nchanls == 2 ? Str(X_1246,"stereo") :
             p->nchanls == 4 ? Str(X_1148,"quad") :
             p->nchanls == 6 ? Str(X_830,"hex") : Str(X_1088,"oct") );
      if (p->channel == ALLCHNLS)
        printf(Str(X_51,"%s channels\n"),
               p->nchanls == 2 ? Str(X_619,"both") : Str(X_591,"all"));
      else printf(Str(X_655,"channel %d\n"), p->channel);
    }

#ifdef NeXT
        if (!p->filetyp)
          printf(/*Str(X_1095,*/"opening NeXT infile %s\n", sfname);
#endif
printf(/*Str(X_1093,*/"opening %s infile %s, with%s bytrev\n",
                 p->filetyp == TYP_AIFF ? "AIFF" : TYP_AIFC ? "AIFF-C" : "WAV",
                 sfname, p->bytrev == NULL ? Str(X_21," no") : "");

    /*RWD 5:2001  */
    if (p->sampframsiz <= 0)    /* must know framsiz */
      die(Str(X_882,"illegal sampframsiz"));

    /*****  set file pointers, buffers, and diskin-specific stuff  ******/
    /* we get a crash if backwards and 0 skiptime, so lets set it to file
       end instead..*/
    if (skiptime <= 0 && *p->ktransp < 0) {
      if (p->audsize > 0)
        skiptime = (MYFLT)p->audsize/(MYFLT)(p->sr * p->sampframsiz);
      else
        skiptime = FL(1.0)/(MYFLT)p->sr; /* one sample */
    }

    nbytes = (long)(skiptime * p->sr) * p->sampframsiz;
                /*#### will this work for all header types??? */
    p->firstsampinfile = lseek(sinfd,(off_t)0L,SEEK_CUR);

    if ((p->audrem > 0) && (nbytes > p->audrem)) {
      if (O.msglevel & WARNMSG)
        printf(Str(X_1191,"WARNING: skip time larger than audio data,substituting zero.\n"));
      nbytes = 0;
    }

    if (nbytes > 0) {
      if (p->audsize > 0 )      /* change audsize   */
        p->audrem = p->audsize-nbytes+p->firstsampinfile;

      if ((p->filepos =         /* seek to bndry */
           (long)lseek(sinfd, (off_t)(nbytes+p->firstsampinfile), SEEK_SET)) < 0)
        die(Str(X_699,"diskin seek error: invalid skip time"));
    }
    else {
      p->begfile = TRUE;
      if (*p->ktransp < 0)
        p->endfile = TRUE;
    }

    if ((n =                    /* now rd fulbuf */
         sreadinew(sinfd,p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
      p->endfile = 1;
    p->inbufp = p->inbuf;
    p->bufend = p->inbuf + n;

    /*****  if soundinset successful  ********/
    if (sinfd > 0) {
      fdrecord(&p->fdch);              /*     instr will close later */

      p->guardpt = p->bufend - p->sampframsiz;
      p->phs = 0.0;
      return OK;
    }
    else return initerror(errmsg);

 errtn:
    return NOTOK;                      /*              return empty handed */
}


/*  NB: floats not converted here, but in sreadinew():
    handles autorescale from PEAK, etc) */

void soundinew(SOUNDINEW *p)    /*  a-rate routine for soundinew */
{
    MYFLT       *r1, *r2, *r3, *r4, ktransp,looping;
    int         chnsout, n, ntogo, bytesLeft;
    double      phs,phsFract,phsTrunc;
    char        *inbufp = p->inbufp;
    long snewbufsize = SNDINEWBUFSIZ;            /*RWD 5:2001 */
    long oldfilepos = 0;

    if (p->format == AE_24INT)
      snewbufsize = SNDINEWBUFSIZ_24;

    if ((!p->bufend) || (!p->inbufp) || (!p->sampframsiz)) {
      initerror(Str(X_701,"diskin: not initialised"));
      return;
    }
    r1      = p->r1;
    r2      = p->r2;
    r3      = p->r3;
    r4      = p->r4;
    ktransp = *p->ktransp;
    looping = *p->ilooping;
    chnsout = p->OUTOCOUNT;
    phs     = p->phs;
    ntogo   = ksmps;
    /*RWD 5:2001 need this when instr dur > filelen*/
    n = 0;
    /* RWD 5:2001 interesting issue - if ktransp starts at zero, we have
     * no idea what direction to go in!  below, it was "if ktransp > 0",
     * but the docs stipulate that only a negative transp signifies
     * backwards rendering, so really, ktransp=0 implies we go forwards */
#ifdef _DEBUG
    if (inbufp != p->bufend)
      assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
    if (ktransp >= 0 ) {        /* forwards... */
/* RWD 5:2001 want to keep phase if reversing mid-data */
      if (phs < 0 && p->begfile)
        phs = 0; /* we have just switched directions, forget (-ve) old phase */
      if (p->endfile) {
        if (p->begfile) p->endfile = FALSE;
        else goto filend;
      }

      while (ntogo) {
        /* a lot of the following code has been "written out" for speed */
        switch (chnsout) {
        case 1:
          phsFract = modf(phs,&phsTrunc);
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)(inbufp + 1) -
                                *(char *)inbufp) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)(inbufp + 1) -
                                *(unsigned char *)inbufp) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_SHORT:
            do {
              *r1++ = short_to_dbfs *
                (MYFLT) ((*(short *)inbufp +
                          (*(short *)(inbufp + 2) -
                           *(short *)inbufp) * phsFract));
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)(inbufp + 2) - */
/*                                 *(short *)inbufp) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
#ifdef _DEBUG
              samplecount++;
#endif
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs *
                (MYFLT) (*(long *)inbufp +
                         (*(long *)(inbufp + 4) -
                          *(long *)inbufp) * phsFract);
/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)(inbufp + 4) - */
/*                                 *(long *)inbufp) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(MYFLT *)inbufp +
                               (*(MYFLT *)(inbufp + 4) -
                                *(MYFLT *)inbufp) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
            /*RWD 5:2001*/
          case AE_24INT:
            {
              SAMP24 s24_first,s24_next;
              MYFLT first,next;
              char *ptr;

              s24_first.lsamp = s24_next.lsamp =  0L;
              do {
                ptr = inbufp;
                /* get it right first, optimize later */
                s24_first.bytes[1] = *ptr++;
                s24_first.bytes[2] = *ptr++;
                s24_first.bytes[3] = *ptr++;
                s24_next.bytes[1]  = *ptr++;
                s24_next.bytes[2]  = *ptr++;
                s24_next.bytes[3]  = *ptr;
                /* we now have  quasi 32bit values */
                first = long_to_dbfs * (MYFLT) s24_first.lsamp;
                next  = long_to_dbfs * (MYFLT) s24_next.lsamp;
/*                 first = (MYFLT) (s24_first.lsamp * INLONGFAC); */
/*                 next  = (MYFLT) (s24_next.lsamp * INLONGFAC); */
                /* convert to a quasi 16bit value! */
                first    = (MYFLT) (s24_first.lsamp * INLONGFAC);
                next     = (MYFLT) (s24_next.lsamp * INLONGFAC);
                *r1++    = (MYFLT) (first + ((next-first) * phsFract));
                phs     += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp   = p->inbufp + (long)(phsTrunc * 3);
                --ntogo;
#ifdef _DEBUG
                samplecount++;
#endif
              } while ((inbufp < p->guardpt) && (ntogo));
            }
            break;
          }
          break;
        case 2:
          phsFract = modf(phs,&phsTrunc);
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)(inbufp + 2) -
                                *(char *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(char *)(inbufp + 1) +
                               (*(char *)(inbufp + 3) -
                                *(char *)(inbufp + 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)(inbufp + 2) -
                                *(unsigned char *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(unsigned char *)(inbufp + 1) +
                               (*(unsigned char *)(inbufp + 3) -
                                *(unsigned char *)(inbufp + 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_SHORT:
            do {
              *r1++ = short_to_dbfs *
                (MYFLT) (*(short *)inbufp +
                         (*(short *)(inbufp + 4) -
                          *(short *)inbufp) * phsFract);
              *r2++ = short_to_dbfs *
                (MYFLT) (*(short *)(inbufp + 2) +
                         (*(short *)(inbufp + 6) -
                          *(short *)(inbufp + 2)) * phsFract);
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)(inbufp + 4) - */
/*                                 *(short *)inbufp) * phsFract); */
/*               *r2++ = (MYFLT) (*(short *)(inbufp + 2) + */
/*                                (*(short *)(inbufp + 6) - */
/*                                 *(short *)(inbufp + 2)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_24INT:
            {
              SAMP24 s24_first_1,s24_first_2;
              SAMP24 s24_next_1,s24_next_2;
              MYFLT first,next;
              char *ptr;

              s24_first_1.lsamp = s24_next_1.lsamp =  0L;
              s24_first_2.lsamp = s24_next_2.lsamp =  0L;
              do {
                ptr = inbufp;
                /* get it right first, optimize later */
                s24_first_1.bytes[1] = *ptr++;
                s24_first_1.bytes[2] = *ptr++;
                s24_first_1.bytes[3] = *ptr++;
                s24_first_2.bytes[1] = *ptr++;
                s24_first_2.bytes[2] = *ptr++;
                s24_first_2.bytes[3] = *ptr++;
                s24_next_1.bytes[1]  = *ptr++;
                s24_next_1.bytes[2]  = *ptr++;
                s24_next_1.bytes[3]  = *ptr++;
                s24_next_2.bytes[1]  = *ptr++;
                s24_next_2.bytes[2]  = *ptr++;
                s24_next_2.bytes[3]  = *ptr;
                /* we now have  quasi 32bit values */
                first    = long_to_dbfs * (MYFLT) s24_first_1.lsamp;
                next     = long_to_dbfs * (MYFLT) s24_next_1.lsamp;
                *r1++    = (MYFLT) (first + ((next-first) * phsFract));
                first    = long_to_dbfs * (MYFLT) s24_first_2.lsamp;
                next     = long_to_dbfs * (MYFLT) s24_next_2.lsamp;
                *r2++    = (MYFLT) (first + ((next-first) * phsFract));
/*                 first    = (MYFLT) (s24_first_1.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_1.lsamp * INLONGFAC); */
/*                 *r1++    = (MYFLT) (first + ((next-first) * phsFract)); */
/*                 first    = (MYFLT) (s24_first_2.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_2.lsamp * INLONGFAC); */
/*                 *r2++    = (MYFLT) (first + ((next-first) * phsFract)); */
                phs     += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp   = p->inbufp + (long)(phsTrunc * 6);
                --ntogo;
#ifdef _DEBUG
                samplecount++;
#endif
              } while ((inbufp < p->guardpt) && (ntogo));
            }
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs *
                (MYFLT) (*(long *)inbufp +
                         (*(long *)(inbufp + 8) -
                          *(long *)inbufp) * phsFract);
              *r2++ = long_to_dbfs *
                (MYFLT) (*(long *)(inbufp + 4) +
                         (*(long *)(inbufp + 12) -
                          *(long *)(inbufp + 4)) * phsFract);
/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)(inbufp + 8) - */
/*                                 *(long *)inbufp) * phsFract); */
/*               *r2++ = (MYFLT) (*(long *)(inbufp + 4) + */
/*                                (*(long *)(inbufp + 12) - */
/*                                 *(long *)(inbufp + 4)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(float *)inbufp +
                               (*(float *)(inbufp + 8) -
                                *(float *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(float *)(inbufp + 4) +
                               (*(float *)(inbufp + 12) -
                                *(float *)(inbufp + 4)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          }
          break;
        case 4:
          phsFract = modf(phs,&phsTrunc);
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)(inbufp + 4) -
                                *(char *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(char *)(inbufp + 1) +
                               (*(char *)(inbufp + 5) -
                                *(char *)(inbufp + 1)) * phsFract);
              *r3++ = (MYFLT) (*(char *)(inbufp + 2) +
                               (*(char *)(inbufp + 6) -
                                *(char *)(inbufp + 2)) * phsFract);
              *r4++ = (MYFLT) (*(char *)(inbufp + 3) +
                               (*(char *)(inbufp + 7) -
                                *(char *)(inbufp + 3)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)(inbufp + 4) -
                                *(unsigned char *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(unsigned char *)(inbufp + 1) +
                               (*(unsigned char *)(inbufp + 5) -
                                *(unsigned char *)(inbufp + 1)) * phsFract);
              *r3++ = (MYFLT) (*(unsigned char *)(inbufp + 2) +
                               (*(unsigned char *)(inbufp + 6) -
                                *(unsigned char *)(inbufp + 2)) * phsFract);
              *r4++ = (MYFLT) (*(unsigned char *)(inbufp + 3) +
                               (*(unsigned char *)(inbufp + 7) -
                                *(unsigned char *)(inbufp + 3)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_SHORT:
            do {
              *r1++ = short_to_dbfs * (MYFLT) (*(short *)inbufp +
                               (*(short *)(inbufp + 8) -
                                *(short *)inbufp) * phsFract);
              *r2++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 2) +
                               (*(short *)(inbufp + 10) -
                                *(short *)(inbufp + 2)) * phsFract);
              *r3++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 4) +
                               (*(short *)(inbufp + 12) -
                                *(short *)(inbufp + 4)) * phsFract);
              *r4++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 6) +
                               (*(short *)(inbufp + 14) -
                                *(short *)(inbufp + 6)) * phsFract);
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)(inbufp + 8) - */
/*                                 *(short *)inbufp) * phsFract); */
/*               *r2++ = (MYFLT) (*(short *)(inbufp + 2) + */
/*                                (*(short *)(inbufp + 10) - */
/*                                 *(short *)(inbufp + 2)) * phsFract); */
/*               *r3++ = (MYFLT) (*(short *)(inbufp + 4) + */
/*                                (*(short *)(inbufp + 12) - */
/*                                 *(short *)(inbufp + 4)) * phsFract); */
/*               *r4++ = (MYFLT) (*(short *)(inbufp + 6) + */
/*                                (*(short *)(inbufp + 14) - */
/*                                 *(short *)(inbufp + 6)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_24INT:
            {
              SAMP24 s24_first_1,s24_first_2,s24_first_3,s24_first_4;
              SAMP24 s24_next_1,s24_next_2,s24_next_3,s24_next_4;
              MYFLT first,next;
              char *ptr;

              s24_first_1.lsamp = s24_next_1.lsamp =  0L;
              s24_first_2.lsamp = s24_next_2.lsamp =  0L;
              s24_first_3.lsamp = s24_next_3.lsamp =  0L;
              s24_first_4.lsamp = s24_next_4.lsamp =  0L;
              do {
                ptr = inbufp;
                /* get it right first, optimize later */
                s24_first_1.bytes[1] = *ptr++;
                s24_first_1.bytes[2] = *ptr++;
                s24_first_1.bytes[3] = *ptr++;
                s24_first_2.bytes[1] = *ptr++;
                s24_first_2.bytes[2] = *ptr++;
                s24_first_2.bytes[3] = *ptr++;
                s24_first_3.bytes[1] = *ptr++;
                s24_first_3.bytes[2] = *ptr++;
                s24_first_3.bytes[3] = *ptr++;
                s24_first_4.bytes[1] = *ptr++;
                s24_first_4.bytes[2] = *ptr++;
                s24_first_4.bytes[3] = *ptr++;
                s24_next_1.bytes[1]  = *ptr++;
                s24_next_1.bytes[2]  = *ptr++;
                s24_next_1.bytes[3]  = *ptr++;
                s24_next_2.bytes[1]  = *ptr++;
                s24_next_2.bytes[2]  = *ptr++;
                s24_next_2.bytes[3]  = *ptr++;
                s24_next_3.bytes[1]  = *ptr++;
                s24_next_3.bytes[2]  = *ptr++;
                s24_next_3.bytes[3]  = *ptr++;
                s24_next_4.bytes[1]  = *ptr++;
                s24_next_4.bytes[2]  = *ptr++;
                s24_next_4.bytes[3]  = *ptr;
                /* we now have  quasi 32bit values */
                first    = long_to_dbfs * (MYFLT) (s24_first_1.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_1.lsamp);
                *r1++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_2.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_2.lsamp);
                *r2++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_3.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_3.lsamp);
                *r3++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_4.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_4.lsamp);
                *r4++    = (MYFLT) (first + ((next-first) * phsFract));

/*                 first    = (MYFLT) (s24_first_1.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_1.lsamp * INLONGFAC); */
/*                 *r1++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_2.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_2.lsamp * INLONGFAC); */
/*                 *r2++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_3.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_3.lsamp * INLONGFAC); */
/*                 *r3++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_4.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_4.lsamp * INLONGFAC); */
/*                 *r4++    = (MYFLT) (first + ((next-first) * phsFract)); */
                phs     += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp   = p->inbufp + (long)(phsTrunc * 12);
                --ntogo;
#ifdef _DEBUG
                samplecount++;
#endif
              } while ((inbufp < p->guardpt) && (ntogo));
            }
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs * (MYFLT) (*(long *)inbufp +
                               (*(long *)(inbufp + 16) -
                                *(long *)inbufp) * phsFract);
              *r2++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 4) +
                               (*(long *)(inbufp + 20) -
                                *(long *)(inbufp + 4)) * phsFract);
              *r3++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 8) +
                               (*(long *)(inbufp + 24) -
                                *(long *)(inbufp + 8)) * phsFract);
              *r4++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 12) +
                               (*(long *)(inbufp + 28) -
                                *(long *)(inbufp + 12)) * phsFract);

/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)(inbufp + 16) - */
/*                                 *(long *)inbufp) * phsFract); */
/*               *r2++ = (MYFLT) (*(long *)(inbufp + 4) + */
/*                                (*(long *)(inbufp + 20) - */
/*                                 *(long *)(inbufp + 4)) * phsFract); */
/*               *r3++ = (MYFLT) (*(long *)(inbufp + 8) + */
/*                                (*(long *)(inbufp + 24) - */
/*                                 *(long *)(inbufp + 8)) * phsFract); */
/*               *r4++ = (MYFLT) (*(long *)(inbufp + 12) + */
/*                                (*(long *)(inbufp + 28) - */
/*                                 *(long *)(inbufp + 12)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(float *)inbufp +
                               (*(float *)(inbufp + 16) -
                                *(float *)inbufp) * phsFract);
              *r2++ = (MYFLT) (*(float *)(inbufp + 4) +
                               (*(float *)(inbufp + 20) -
                                *(float *)(inbufp + 4)) * phsFract);
              *r3++ = (MYFLT) (*(float *)(inbufp + 8) +
                               (*(float *)(inbufp + 24) -
                                *(float *)(inbufp + 8)) * phsFract);
              *r4++ = (MYFLT) (*(float *)(inbufp + 12) +
                               (*(float *)(inbufp + 28) -
                                *(float *)(inbufp + 12)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
          }
          break;
        }

        bytesLeft = (int)(p->bufend - inbufp);
        if (bytesLeft <= p->sampframsiz) {      /* first set file position
                                                   to where inbuf p "thinks"
                                                   its pointing to */
          p->filepos = (long)lseek(p->fdch.fd,(off_t)(-bytesLeft),SEEK_CUR);
          if ((n = sreadinew(p->fdch.fd,
                             p->inbuf,snewbufsize,p)) == 0) {  /*RWD 5:2001 */
            if (looping) {
              /* go to beginning of file.
                 depending on the pitch and
                 phase, we might drop a few "guardpoint" samples, but
                 this ugen is intended for large files anyway -- if a
                 few end samples are critical for looping, use oscil or
                 table!!!!  */
              p->audrem = p->audsize;
              p->filepos = (long)lseek(p->fdch.fd,
                                       (off_t)p->firstsampinfile,SEEK_SET);
              if ((n = sreadinew(p->fdch.fd,
                                 p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
                die(Str(X_733,"error trying to loop back to the beginning "
                        "of the sound file!?!??"));
              p->begfile = 1;
              phs = 0;
              inbufp = p->inbufp = p->inbuf;
              p->bufend = p->inbuf + n;
              /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
               * when a transp sweep hits eof), but not, I suspect, the
               * underlying cause */
              if (n < snewbufsize)
                memset(p->bufend,0,snewbufsize-n);
              p->guardpt = p->bufend - p->sampframsiz;
            }
            else {
              p->endfile = TRUE;
              goto filend;
            }
          }
          else {
            inbufp = p->inbufp = p->inbuf;
            p->bufend = p->inbuf + n;
            /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
             * when a transp sweep hits eof), but not, I suspect, the
             * underlying cause */
            if (n < snewbufsize)
              memset(p->bufend,0,snewbufsize-n);
            p->guardpt = p->bufend - p->sampframsiz;
            phs = modf(phs,&phsTrunc);
            p->begfile = FALSE;
          }
        }
      }
#ifdef _DEBUG
      if (inbufp != p->bufend)
        assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
    }

    else {      /* backwards...                 same thing but different */
      if (phs > 0 && p->endfile)      /*RWD 5:2001 as above */
        phs = 0; /* have just switched directions, forget (+ve) old phase */

      if (p->endfile) {   /* firewall-flag signaling when we are at either
                             end of the file */
        if (p->begfile)
          goto filend; /* make sure we are at beginning, not end */
        else
          /* RWD 5:2001: read in the first block (= last block of infile) */
          {
            bytesLeft = (int)(inbufp - p->inbuf);
            if ((p->filepos = (long)lseek(p->fdch.fd,
                                    (off_t)(bytesLeft-snewbufsize),
                                    SEEK_CUR)) <= p->firstsampinfile) {
              p->filepos = (long)lseek(p->fdch.fd,
                                       (off_t)p->firstsampinfile,SEEK_SET);
              p->begfile = 1;
            }

            /* RWD 5:2001 but don't know if this is required here... */
            p->audrem = p->audsize; /* a hack to prevent errors (returning
                                       'ntot')in the sread for AIFF */
            if ((n = sreadinew(p->fdch.fd,p->inbuf,snewbufsize,p)) !=
                snewbufsize) {
              /* we should never get here. if we do,
                 we're fucked because didn't get a full buffer and our
                 present sample is the last sample of the buffer!!!  */
              die(Str(X_697,"diskin read error - during backwards playback"));
              return;
            }
#ifdef _DEBUG
            sbufp1 = (short *) p->inbuf;
#endif
            /* now get the correct remaining size */
            p->audrem = p->audsize - p->firstsampinfile - p->filepos;
            p->bufend = p->inbuf + n;
            /* point to the last sample in buffer */
            inbufp = p->inbufp = p->guardpt = p->bufend - p->sampframsiz;

            /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
             * when a transp sweep hits eof), but not, I suspect, the
             * underlying cause */
            if (n < snewbufsize)
              memset(p->bufend,0,snewbufsize-n);
            phs = modf(phs,&phsTrunc);
            p->endfile = FALSE;
          }
      }

      while (ntogo) {
        switch(chnsout) {
        case 1:
          phsFract = modf(phs,&phsTrunc); /* phsFract and phsTrunc will be
                                             non-positive */
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)inbufp -
                                *(char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            /* make sure it
               points to the second sample or greater in the buffer,
               because we need at least two to interpolate */
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)inbufp -
                                *(unsigned char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_SHORT:
#ifdef _DEBUG
            sbufp1 = (short *) p->inbuf;
            sbufp2 = (short *) p->inbufp;
#endif
            do {
              *r1++ = short_to_dbfs * (MYFLT) (*(short *)inbufp +
                               (*(short *)inbufp -
                                *(short *)(inbufp - 2)) * phsFract);
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)inbufp - */
/*                                 *(short *)(inbufp - 2)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
#ifdef _DEBUG
              samplecount++;
#endif
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs * (MYFLT) (*(long *)inbufp +
                               (*(long *)inbufp -
                                *(long *)(inbufp - 4)) * phsFract);
/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)inbufp - */
/*                                 *(long *)(inbufp - 4)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(float *)inbufp +
                               (*(float *)inbufp -
                                *(float *)(inbufp - 4)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
/*RWD 5:2001 */
          case AE_24INT:
            {
              SAMP24 s24_first,s24_next;
              MYFLT first,next;
              char *ptr;

              s24_first.lsamp = s24_next.lsamp =  0L;
              do {
                ptr = inbufp - 3;
                s24_next.bytes[1] = *ptr++;
                s24_next.bytes[2] = *ptr++;
                s24_next.bytes[3] = *ptr++;
                s24_first.bytes[1] = *ptr++;
                s24_first.bytes[2] = *ptr++;
                s24_first.bytes[3] = *ptr;
                first = long_to_dbfs * (MYFLT) (s24_first.lsamp);
                next = long_to_dbfs *(MYFLT) (s24_next.lsamp);
/*                 first = (MYFLT) (s24_first.lsamp * INLONGFAC); */
/*                 next = (MYFLT) (s24_next.lsamp * INLONGFAC); */
                *r1++ = (MYFLT) (first + ((first-next) * phsFract));
                phs += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp = p->inbufp + (long)(phsTrunc * 3);
                --ntogo;
              } while ((inbufp > p->inbuf) && (ntogo));
            }
            break;
          }
          break;
        case 2:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)inbufp -
                                *(char *)(inbufp - 2)) * phsFract);
              *r2++ = (MYFLT) (*(char *)(inbufp + 1) +
                               (*(char *)(inbufp + 1) -
                                *(char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)inbufp -
                                *(unsigned char *)(inbufp - 2)) * phsFract);
              *r2++ = (MYFLT) (*(unsigned char *)(inbufp + 1) +
                               (*(unsigned char *)(inbufp + 1) -
                                *(unsigned char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_SHORT:
            do {
              *r1++ = short_to_dbfs * (MYFLT) (*(short *)inbufp +
                               (*(short *)inbufp -
                                *(short *)(inbufp - 4)) * phsFract);
              *r2++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 2) +
                               (*(short *)(inbufp + 2) -
                                *(short *)(inbufp - 2)) * phsFract);
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)inbufp - */
/*                                 *(short *)(inbufp - 4)) * phsFract); */
/*               *r2++ = (MYFLT) (*(short *)(inbufp + 2) + */
/*                                (*(short *)(inbufp + 2) - */
/*                                 *(short *)(inbufp - 2)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
/*RWD 5.2001*/
          case AE_24INT:
            {
              SAMP24 s24_first_1,s24_first_2;
              SAMP24 s24_next_1,s24_next_2;
              MYFLT first,next;
              char *ptr;

              s24_first_1.lsamp = s24_next_1.lsamp =  0L;
              s24_first_2.lsamp = s24_next_2.lsamp =  0L;
              do {
                ptr = inbufp - 6;
                /* get it right first, optimize later */
                s24_next_1.bytes[1]  = *ptr++;
                s24_next_1.bytes[2]  = *ptr++;
                s24_next_1.bytes[3]  = *ptr++;
                s24_next_2.bytes[1]  = *ptr++;
                s24_next_2.bytes[2]  = *ptr++;
                s24_next_2.bytes[3]  = *ptr++;
                s24_first_1.bytes[1] = *ptr++;
                s24_first_1.bytes[2] = *ptr++;
                s24_first_1.bytes[3] = *ptr++;
                s24_first_2.bytes[1] = *ptr++;
                s24_first_2.bytes[2] = *ptr++;
                s24_first_2.bytes[3] = *ptr;

                /* we now have  quasi 32bit values */
                first    = long_to_dbfs * (MYFLT) (s24_first_1.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_1.lsamp);
                *r1++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_2.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_2.lsamp);
                *r2++    = (MYFLT) (first + ((next-first) * phsFract));

/*                 first    = (MYFLT) (s24_first_1.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_1.lsamp * INLONGFAC); */
/*                 *r1++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_2.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_2.lsamp * INLONGFAC); */
/*                 *r2++    = (MYFLT) (first + ((next-first) * phsFract)); */
                phs     += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp   = p->inbufp + (long)(phsTrunc * 6);
                --ntogo;
#ifdef _DEBUG
                samplecount++;
#endif
              } while ((inbufp > p->inbuf) && (ntogo));
            }
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs * (MYFLT) (*(long *)inbufp +
                               (*(long *)inbufp -
                                *(long *)(inbufp - 8)) * phsFract);
              *r2++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 4) +
                               (*(long *)(inbufp + 4) -
                                *(long *)(inbufp - 4)) * phsFract);
/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)inbufp - */
/*                                 *(long *)(inbufp - 8)) * phsFract); */
/*               *r2++ = (MYFLT) (*(long *)(inbufp + 4) + */
/*                                (*(long *)(inbufp + 4) - */
/*                                 *(long *)(inbufp - 4)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(float *)inbufp +
                               (*(float *)inbufp -
                                *(float *)(inbufp - 8)) * phsFract);
              *r2++ = (MYFLT) (*(float *)(inbufp + 4) +
                               (*(float *)(inbufp + 4) -
                                *(float *)(inbufp - 4)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          }
          break;
        case 4:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
          switch (p->format) {
          case AE_CHAR:
            do {
              *r1++ = (MYFLT) (*(char *)inbufp +
                               (*(char *)inbufp -
                                *(char *)(inbufp - 4)) * phsFract);
              *r2++ = (MYFLT) (*(char *)(inbufp + 1) +
                               (*(char *)(inbufp + 1) -
                                *(char *)(inbufp - 3)) * phsFract);
              *r3++ = (MYFLT) (*(char *)(inbufp + 2) +
                               (*(char *)(inbufp + 2) -
                                *(char *)(inbufp - 2)) * phsFract);
              *r4++ = (MYFLT) (*(char *)(inbufp + 3) +
                               (*(char *)(inbufp + 3) -
                                *(char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_UNCH:
            do {
              *r1++ = (MYFLT) (*(unsigned char *)inbufp +
                               (*(unsigned char *)inbufp -
                                *(unsigned char *)(inbufp - 4)) * phsFract);
              *r2++ = (MYFLT) (*(unsigned char *)(inbufp + 1) +
                               (*(unsigned char *)(inbufp + 1) -
                                *(unsigned char *)(inbufp - 3)) * phsFract);
              *r3++ = (MYFLT) (*(unsigned char *)(inbufp + 2) +
                               (*(unsigned char *)(inbufp + 2) -
                                *(unsigned char *)(inbufp - 2)) * phsFract);
              *r4++ = (MYFLT) (*(unsigned char *)(inbufp + 3) +
                               (*(unsigned char *)(inbufp + 3) -
                                *(unsigned char *)(inbufp - 1)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_SHORT:
            do {
              *r1++ = short_to_dbfs * (MYFLT) (*(short *)inbufp +
                               (*(short *)inbufp -
                                *(short *)(inbufp - 8)) * phsFract);
              *r2++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 2) +
                               (*(short *)(inbufp + 2) -
                                *(short *)(inbufp - 6)) * phsFract);
              *r3++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 4) +
                               (*(short *)(inbufp + 4) -
                                *(short *)(inbufp - 4)) * phsFract);
              *r4++ = short_to_dbfs * (MYFLT) (*(short *)(inbufp + 6) +
                               (*(short *)(inbufp + 6) -
                              *(short *)(inbufp - 2)) * phsFract);
/*               *r1++ = (MYFLT) (*(short *)inbufp + */
/*                                (*(short *)inbufp - */
/*                                 *(short *)(inbufp - 8)) * phsFract); */
/*               *r2++ = (MYFLT) (*(short *)(inbufp + 2) + */
/*                                (*(short *)(inbufp + 2) - */
/*                                 *(short *)(inbufp - 6)) * phsFract); */
/*               *r3++ = (MYFLT) (*(short *)(inbufp + 4) + */
/*                                (*(short *)(inbufp + 4) - */
/*                                 *(short *)(inbufp - 4)) * phsFract); */
/*               *r4++ = (MYFLT) (*(short *)(inbufp + 6) + */
/*                                (*(short *)(inbufp + 6) - */
/*                               *(short *)(inbufp - 2)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
/*RWD 5.2001 */
          case AE_24INT:
            {
              SAMP24 s24_first_1,s24_first_2,s24_first_3,s24_first_4;
              SAMP24 s24_next_1,s24_next_2,s24_next_3,s24_next_4;
              MYFLT first,next;
              char *ptr;

              s24_first_1.lsamp = s24_next_1.lsamp =  0L;
              s24_first_2.lsamp = s24_next_2.lsamp =  0L;
              s24_first_3.lsamp = s24_next_3.lsamp =  0L;
              s24_first_4.lsamp = s24_next_4.lsamp =  0L;
              do {
                ptr = inbufp - 12;
                /* get it right first, optimize later */
                s24_next_1.bytes[1]  = *ptr++;
                s24_next_1.bytes[2]  = *ptr++;
                s24_next_1.bytes[3]  = *ptr++;
                s24_next_2.bytes[1]  = *ptr++;
                s24_next_2.bytes[2]  = *ptr++;
                s24_next_2.bytes[3]  = *ptr++;
                s24_next_3.bytes[1]  = *ptr++;
                s24_next_3.bytes[2]  = *ptr++;
                s24_next_3.bytes[3]  = *ptr++;
                s24_next_4.bytes[1]  = *ptr++;
                s24_next_4.bytes[2]  = *ptr++;
                s24_next_4.bytes[3]  = *ptr++;
                s24_first_1.bytes[1] = *ptr++;
                s24_first_1.bytes[2] = *ptr++;
                s24_first_1.bytes[3] = *ptr++;
                s24_first_2.bytes[1] = *ptr++;
                s24_first_2.bytes[2] = *ptr++;
                s24_first_2.bytes[3] = *ptr++;
                s24_first_3.bytes[1] = *ptr++;
                s24_first_3.bytes[2] = *ptr++;
                s24_first_3.bytes[3] = *ptr++;
                s24_first_4.bytes[1] = *ptr++;
                s24_first_4.bytes[2] = *ptr++;
                s24_first_4.bytes[3] = *ptr++;
                first    = long_to_dbfs * (MYFLT) (s24_first_1.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_1.lsamp);
                *r1++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_2.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_2.lsamp);
                *r2++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_3.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_3.lsamp);
                *r3++    = (MYFLT) (first + ((next-first) * phsFract));

                first    = long_to_dbfs * (MYFLT) (s24_first_4.lsamp);
                next     = long_to_dbfs * (MYFLT) (s24_next_4.lsamp);
                *r4++    = (MYFLT) (first + ((next-first) * phsFract));
/*                 first    = (MYFLT) (s24_first_1.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_1.lsamp * INLONGFAC); */
/*                 *r1++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_2.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_2.lsamp * INLONGFAC); */
/*                 *r2++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_3.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_3.lsamp * INLONGFAC); */
/*                 *r3++    = (MYFLT) (first + ((next-first) * phsFract)); */

/*                 first    = (MYFLT) (s24_first_4.lsamp * INLONGFAC); */
/*                 next     = (MYFLT) (s24_next_4.lsamp * INLONGFAC); */
/*                 *r4++    = (MYFLT) (first + ((next-first) * phsFract)); */
                phs     += ktransp;
                phsFract = modf(phs,&phsTrunc);
                inbufp   = p->inbufp + (long)(phsTrunc * 12);
                --ntogo;
#ifdef _DEBUG
                samplecount++;
#endif
              } while ((inbufp > p->inbuf) && (ntogo));
            }
            break;
          case AE_LONG:
            do {
              *r1++ = long_to_dbfs * (MYFLT) (*(long *)inbufp +
                               (*(long *)inbufp -
                                *(long *)(inbufp - 16)) * phsFract);
              *r2++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 4) +
                               (*(long *)(inbufp + 4) -
                                *(long *)(inbufp - 12)) * phsFract);
              *r3++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 8) +
                               (*(long *)(inbufp + 8) -
                                *(long *)(inbufp - 8)) * phsFract);
              *r4++ = long_to_dbfs * (MYFLT) (*(long *)(inbufp + 12) +
                               (*(long *)(inbufp + 12) -
                                *(long *)(inbufp - 4)) * phsFract);
/*               *r1++ = (MYFLT) (*(long *)inbufp + */
/*                                (*(long *)inbufp - */
/*                                 *(long *)(inbufp - 16)) * phsFract); */
/*               *r2++ = (MYFLT) (*(long *)(inbufp + 4) + */
/*                                (*(long *)(inbufp + 4) - */
/*                                 *(long *)(inbufp - 12)) * phsFract); */
/*               *r3++ = (MYFLT) (*(long *)(inbufp + 8) + */
/*                                (*(long *)(inbufp + 8) - */
/*                                 *(long *)(inbufp - 8)) * phsFract); */
/*               *r4++ = (MYFLT) (*(long *)(inbufp + 12) + */
/*                                (*(long *)(inbufp + 12) - */
/*                                 *(long *)(inbufp - 4)) * phsFract); */
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          case AE_FLOAT:
            do {
              *r1++ = (MYFLT) (*(float *)inbufp +
                               (*(float *)inbufp -
                                *(float *)(inbufp - 16)) * phsFract);
              *r2++ = (MYFLT) (*(float *)(inbufp + 4) +
                               (*(float *)(inbufp + 4) -
                                *(float *)(inbufp - 12)) * phsFract);
              *r3++ = (MYFLT) (*(float *)(inbufp + 8) +
                               (*(float *)(inbufp + 8) -
                                *(float *)(inbufp - 8)) * phsFract);
              *r4++ = (MYFLT) (*(float *)(inbufp + 12) +
                               (*(float *)(inbufp + 12) -
                                *(float *)(inbufp - 4)) * phsFract);
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 16);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
          }
          break;
        }

        if (inbufp <= p->inbuf) { /* we need to get some more samples!! */
          if (p->begfile) {
            if (looping) {      /* hopes this works -- set 1 buffer lenght
                                   at end of sound file */
              p->filepos =
                (long)lseek(p->fdch.fd,
                            (off_t)(p->firstsampinfile+p->audsize-snewbufsize),
                            SEEK_SET);   /*RWD 5:2001*/
              phs = -0.0;
              p->begfile = 0;
            }
            else {
              p->endfile = 1;
              goto filend;
            }
          }
          else {
            bytesLeft = (int)(inbufp - p->inbuf + p->sampframsiz);
            /* we're going backwards, so bytesLeft should be
             * non-positive because inbufp should be pointing
             * to the first sample in the buffer or "in front"
             * the buffer.  But we must add a sample frame
             * (p->sampframsiz) to make sure the sample we are
             * pointing at right now becomes the last sample
             * in the next buffer*/
            /*RWD remember this for when lseek returns -1 */
            oldfilepos = p->filepos;
#ifdef _DEBUG
            tellpos = lseek(p->fdch.fd,(off_t)0L,SEEK_CUR);
            sbufp1 = (short *) p->inbuf;
#endif

            if ((p->filepos =
                 (long)lseek(p->fdch.fd,
                             (off_t)(bytesLeft-snewbufsize - snewbufsize),
                             /*RWD 5:2001 was SNDINEWBUFSIZ*/
                             SEEK_CUR)) <= p->firstsampinfile) {
              p->filepos = (long)lseek(p->fdch.fd,
                                       (off_t)p->firstsampinfile,SEEK_SET);
              p->begfile = 1;
            }
          }

          p->audrem = p->audsize; /* a hack to prevent errors (returning
                                     'ntot') in the sread for AIFF */

          if ((n = sreadinew(p->fdch.fd,p->inbuf,snewbufsize,p)) !=
              snewbufsize) {       /* RWD 4:2001 was SNDINEWBUFSIZ*/
            /* we should never get here. if we do,
               we're fucked because didn't get a full buffer and our
               present sample is the last sample of the buffer!!!  */
            die(Str(X_697,"diskin read error - during backwards playback"));
            return;
          }
#ifdef _DEBUG
          sbufp1 = (short *) p->inbuf;
#endif
          /* now get the correct remaining size */
          p->audrem = p->audsize - p->firstsampinfile - p->filepos;
          /* RWD 5:2001  this clears a glitch doing
           * plain reverse looping (pitch  = -1) over file
           */
          if (p->begfile )
            n = oldfilepos - p->firstsampinfile;
          p->bufend = p->inbuf + n;
          /* point to the last sample in buffer */
          inbufp = p->inbufp = p->guardpt = p->bufend - p->sampframsiz;
          /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
           * when a transp sweep hits eof), but not, I suspect, the
           * underlying cause */
          if (n < snewbufsize)
            memset(p->bufend,0,snewbufsize-n);
          phs = modf(phs,&phsTrunc);
        }
#ifdef _DEBUG
        if (inbufp != p->bufend)
          assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
      }
    }

    p->inbufp = inbufp;
    p->phs = modf(phs,&phsTrunc);

    return;

 filend:
    if (ntogo > n) {            /* At RWD's suggestion */
      switch(chnsout) {                   /* if past end of file, */
      case 1:
        do *r1++ = FL(0.0);               /*    move in zeros     */
        while (--ntogo);
        break;
      case 2:
        do {
          *r1++ = FL(0.0);
          *r2++ = FL(0.0);
        } while (--ntogo);
        break;
      case 4:
        do {
          *r1++ = FL(0.0);
          *r2++ = FL(0.0);
          *r3++ = FL(0.0);
          *r4++ = FL(0.0);
        } while (--ntogo);
      }
    }
}

/* RWD:DBFS: NB: thse funcs all supposed to write to a 'raw' file, so
   what will people want for 0dbfs handling? really need to update
   opcode with more options. */

/* Empty buffer to file */
static void sndwrt1(int fd, MYFLT *buf, int nsamps)  /* diskfile write */
{
    int n;
    char buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      long x = (long) (buf[n] * dbfs_to_short);
      if (x > 32767) x = 32767;
      else if (x < -32768) x = 32768;
      buffer[n] = (char)(x >> 8);
    }
    if ((n = write(fd, (char*)buffer, nsamps)) < nsamps)
      sndwrterr(n, nsamps);
}

static void sndwrtu(int fd, MYFLT *buf, int nsamps)  /* diskfile write */
{
    int n;
    unsigned char buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      long x = (long) (buf[n] * dbfs_to_short);
      if (x > 32767) x = 32767;
      else if (x < -32768) x = 32768;
      buffer[n] = (unsigned char)(x >> 8)^0x80;
    }
    if ((n = write(fd, (char *)buffer, nsamps)) < nsamps)
      sndwrterr(n, nsamps);
}

static void sndwrt2rev(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    short buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      long x = (long) (buf[n] * dbfs_to_short);
      if (x > 32767) x = 32767;
      else if (x < -32768) x = 32768;
      buffer[n] = (short)x;
    }
    bytrev2((char *)buffer, nbytes = (nsamps<<1));    /* rev bytes in shorts  */
    if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

static void sndwrt2(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    short buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      long x = (long) (buf[n] * dbfs_to_short);
      if (x > 32767) x = 32767;
      else if (x < -32768) x = 32768;
      buffer[n] = (short)x;
    }
    nbytes = nsamps << 1;
    if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

/*RWD 5:2001 */
static void sndwrt3(int fd,MYFLT *buf,int nsamps)
{
    int n, nbytes;
    SAMP24 s24;
    char buffer[SNDOUTSMPS*3];
    char *bufp = buffer;
    for (n=0;n < SNDOUTSMPS; n++) {
      s24.lsamp = (long) (buf[n] * dbfs_to_long);
      /*TODO: add range clipping */
      *bufp++ = s24.bytes[1];
      *bufp++ = s24.bytes[2];
      *bufp++ = s24.bytes[3];
    }
    nbytes = nsamps*3;
    if ((n = write(fd,buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

static void sndwrt3rev(int fd,MYFLT *buf,int nsamps)
{
    int n, nbytes;
    SAMP24 s24;
    char buffer[SNDOUTSMPS*3];
    char *bufp = buffer;
    for (n=0;n < SNDOUTSMPS; n++) {
      s24.lsamp = (long) (buf[n] * dbfs_to_long);
      /*TODO: add range clipping */
      /* umm, can optimize this later.. */
      *bufp++ = s24.bytes[3];
      *bufp++ = s24.bytes[2];
      *bufp++ = s24.bytes[1];
    }
    nbytes = nsamps*3;
    if ((n = write(fd,buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

static void sndwrt4rev(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    long buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      buffer[n] = (long) (buf[n] * dbfs_to_long);
    }
    nbytes = nsamps << 2;
    bytrev4((char *)buffer, nbytes);    /* rev bytes in longs   */
    if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

static void sndwrt4(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    long buffer[SNDOUTSMPS];
    for (n=0; n<SNDOUTSMPS; n++) {
      buffer[n] = (long) (buf[n] * dbfs_to_long);
    }
    nbytes = nsamps << 2;
    if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
      sndwrterr(n, nbytes);
}

static void sndwrtf(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    float buffer[SNDOUTSMPS];
    nbytes = nsamps << 2;
    if (sizeof(MYFLT)==4) {
      if ((n = write(fd, (char*)buf, nbytes)) < nbytes)
        sndwrterr(n, nbytes);
    }
    else {   /* This code is only needed in MYFLT==double case */
      for (n=0; n<SNDOUTSMPS; n++) {
        buffer[n] = (float)buf[n];
      }
      if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
        sndwrterr(n, nbytes);
    }
}

/*RWD 5:2001 */
static void sndwrtfrev(int fd, MYFLT *buf, int nsamps) /* diskfile write */
{
    int n, nbytes;
    float buffer[SNDOUTSMPS];
      nbytes = nsamps << 2;
    if (sizeof(MYFLT)==4) {
      bytrev4((char *) buf,nbytes);
      if ((n = write(fd, (char*)buf, nbytes)) < nbytes)
        sndwrterr(n, nbytes);}
    else {   /* This code is only needed in MYFLT==double case*/
      for (n=0; n<SNDOUTSMPS; n++) {
        buffer[n] = (float)buf[n];
      }
      bytrev4((char *) buffer,nbytes);
      if ((n = write(fd, (char*)buffer, nbytes)) < nbytes)
        sndwrterr(n, nbytes);
    }
}

int sndo1set(SNDOUT *p)            /* init routine for instr soundout   */
{
    int    soutfd, filno;
    char   *sfname, sndoutname[128];

#ifdef never
    /*RWD 5:2002 : what is this for ???? */
    PSF_PROPS props;
    int psf_ok = 1;
    int do_clip = 1;
    p->c.is_portsf = 0;
    props.srate  =esr;
    props.chans = 1;
    props.chformat = STDWAVE;
    if (x(p->c.format = (short)*p->c.iformat) > 0)
      p->c.format |= 0x100;
#endif

    if (p->c.fdch.fd != 0)   return OK;        /* if file already open, rtn  */
    if (*p->c.ifilcod == SSTRCOD)
      strcpy(sndoutname, unquote(p->STRARG));
    else if ((filno = (int)*p->c.ifilcod) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(sndoutname, strsets[filno]);
    else
      sprintf(sndoutname,"soundout.%d", filno);
    sfname = sndoutname;
    if ((soutfd = openout(sfname, 1)) < 0) {   /* if openout successful */
      if (isfullpath(sfname))
        sprintf(errmsg,Str(X_1212,"soundout cannot open %s"), sfname);
      else
        sprintf(errmsg,Str(X_1211,"soundout cannot find %s in search paths"),
                sfname);
      goto errtn;
    }
    sfname = retfilnam;
    if ((p->c.format = (short)*p->c.iformat) > 0)
      p->c.format |= 0x100;

    if ((p->c.filetyp == TYP_AIFF && bytrevhost()) ||
        (p->c.filetyp == TYP_AIFC && bytrevhost()) ||
        (p->c.filetyp == TYP_WAV && !bytrevhost())) {
      int rev = 1;
      switch(p->c.format) {
      case AE_UNCH:  p->c.swrtmethod = sndwrtu;    rev = 0;   break;
      case AE_CHAR:
#ifdef never
      case AE_ALAW:
#endif
#ifdef ULAW
      case AE_ULAW:  
#endif
        p->c.swrtmethod = sndwrt1;    rev = 0;   break;
      case AE_SHORT: p->c.swrtmethod = sndwrt2rev; rev = 1;   break;
 /*RWD 5:2001 :  format = 8 from opcode */
      case AE_24INT: p->c.swrtmethod = sndwrt3rev; rev = 1;   break;

      case AE_LONG:  p->c.swrtmethod = sndwrt4rev; rev = 1;   break;
        /*RWD 5:2001 was sndwrtf,rev=0 : but we ~can~ reverse floatsams! */
      case AE_FLOAT: p->c.swrtmethod = sndwrtfrev;    rev = 1;   break;
        /*RWD 5:2001 might as well trap a bad format param */
      default:
        die(Str(X_1770,"Error: bad format parameter for soundout\n"));
      }
      printf(Str(X_1094,"opening %s outfile %s, with%s bytrev\n"),
             p->c.filetyp==TYP_AIFF ? "AIFF":
             p->c.filetyp==TYP_AIFC ? "AIFF-C":"WAV",
             sfname, rev ? "":Str(X_21," no"));
    }
    else switch(p->c.format) {
    case AE_UNCH:  p->c.swrtmethod = sndwrt1;  break;
    case AE_CHAR:
#ifdef never
    case AE_ALAW:
#endif
#ifdef ULAW
    case AE_ULAW:
#endif
      p->c.swrtmethod = sndwrt1;  break;
    case AE_SHORT: p->c.swrtmethod = sndwrt2;  break;
/*RWD 5:2001 */
    case AE_24INT: p->c.swrtmethod = sndwrt3;  break;

    case AE_LONG:  p->c.swrtmethod = sndwrt4;  break;
    case AE_FLOAT: p->c.swrtmethod = sndwrtf;  break;
/*RWD 5:2001 might as well trap a bad format param */
    default:
      die(Str(X_1771,"Error: undefined format parameter for soundout\n"));
    }
    p->c.outbufp = p->c.outbuf;         /* fix - isro 20-11-96 */
    p->c.bufend = p->c.outbuf + SNDOUTSMPS; /* fix - isro 20-11-96 */
    p->c.fdch.fd = soutfd;                  /*     store & log the fd     */
    fdrecord(&p->c.fdch);                   /*     instr will close later */
    return OK;
 errtn:
    return initerror(errmsg);               /* else just print the errmsg */
}

int soundout(SNDOUT *p)
{
    MYFLT  *outbufp, *asig;
    int    nn, nsamps, ospace;

    asig = p->asig;
    outbufp = p->c.outbufp;
    nsamps = ksmps;
    ospace = (p->c.bufend - outbufp);
 nchk:
    if ((nn = nsamps) > ospace)
      nn = ospace;
    nsamps -= nn;
    ospace -= nn;
    do  *outbufp++ = *asig++;
    while (--nn);
    if (!ospace) {              /* when buf is full  */
      p->c.swrtmethod(p->c.fdch.fd, (void *)p->c.outbuf,
                      p->c.bufend - p->c.outbuf);
      outbufp = p->c.outbuf;
      ospace = SNDOUTSMPS;
      if (nsamps) goto nchk;    /*   chk rem samples */
    }
    p->c.outbufp = outbufp;
    return OK;
}

void sndo2set(SNDOUTS *p)
{
    IGN(p);
}

void soundouts(SNDOUTS *p)
{
    IGN(p);
}
#endif
