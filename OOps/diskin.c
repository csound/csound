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
#include <math.h>

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

extern char* type2string(int);
extern char  *getstrformat(int format);
extern short sfsampsize(int);

static int sreadinew(           /* special handling of sound input       */
    SNDFILE *infd,              /* to accomodate reads thru pipes & net  */
    MYFLT   *inbuf,             /* where nbytes rcvd can be < n requested*/
    int     nsamples,           /*                                       */
    SOUNDINEW *p)               /* extra arg passed for filetyp testing  */
{                               /* on POST-HEADER reads of audio samples */
    MYFLT  scalefac;
    int    n, ntot = 0;

    do {
      if ((n = sf_read_MYFLT(infd, inbuf + ntot, nsamples - ntot)) < 0)
        csoundDie(&cenviron, Str("soundfile read error"));
    } while (n > 0 && (ntot += n) < nsamples);
    if (p->audrem > 0) {      /* AIFF:                  */
      if (ntot > p->audrem)   /*   chk haven't exceeded */
        ntot = p->audrem;     /*   limit of audio data  */
      p->audrem -= ntot;    /* FIXED VL, 02-11-04 */
    }
    else ntot = 0;

    /*RWD 3:2000 expanded format fixups ; more efficient here than in
      soundinew() ?  (well, saves a LOT of typing!) */
    scalefac = cenviron.e0dbfs;
    if (p->format == AE_FLOAT) {
      if (p->filetyp == TYP_WAV || p->filetyp == TYP_AIFF) {
        if (p->do_floatscaling)
          scalefac *= p->fscalefac;
      }
      else
        return ntot;
    }
    for (n = 0; n < ntot; n++)
      inbuf[n] *= scalefac;

    return ntot;
}


static int sngetset(SOUNDINEW *p, char *sfname)
{
    int     sinfd;
    SNDFILE *infile;
    SF_INFO sfinfo;
    SOUNDIN forReadHeader;

    if ((sinfd = openin(sfname)) < 0) {     /* open with full dir paths */
      sprintf(errmsg,Str("diskin cannot open %s"), sfname);
      goto errtn;
    }
    infile = sf_open_fd(sinfd, SFM_READ, &sfinfo, SF_TRUE);
    p->fdch.fd = infile;
    p->format = SF2FORMAT(sfinfo.format);
    sfname = retfilnam;                        /* & record fullpath filnam */
    if (*p->iformat > 0)  /* convert spec'd format code */
       p->format = ((short)*p->iformat) | 0x100;
    p->endfile = 0;
    p->begfile = 0;
    p->filetyp = 0;             /* initially non-typed for readheader */

    /******* construct the SOUNDIN struct to use old readheader ***********/
    forReadHeader.filetyp = p->filetyp;
    forReadHeader.audrem = p->audrem;

    if (sfinfo.samplerate != (int) cenviron.esr) {  /* non-anal:  cmp w. esr */
      if (O.msglevel & WARNMSG)
        printf(Str("WARNING: %s sr = %ld, orch sr = %7.1f\n"),
               sfname, sfinfo.samplerate, cenviron.esr);
    }

    if (sfinfo.channels != p->OUTOCOUNT) {         /*        chk nchanls */
      if (O.msglevel & WARNMSG) {
        printf(Str("WARNING: %s nchnls = %d, soundin reading as if nchnls = %d\n"),
               sfname, (int) sfinfo.channels, (int) p->OUTOCOUNT);
      }
      sfinfo.channels = p->OUTOCOUNT;
    }

    /***********  copy header data  *************/
    /*RWD 3:2000 copy scalefac stuff */
    p->do_floatscaling = forReadHeader.do_floatscaling;
    p->fscalefac = forReadHeader.fscalefac;

    p->format = (short) SF2FORMAT(sfinfo.format);
    p->sampframsiz = (short)sfsampsize(sfinfo.format) * sfinfo.channels;
    p->filetyp     = SF2TYPE(sfinfo.format);
    p->sr          = sfinfo.samplerate;
    p->nchanls     = (short)sfinfo.channels;
    p->audrem = p->audsize = sfinfo.frames;
    p->fdch.fd = infile;                  /*     store & log the fd     */
    return (TRUE);

 errtn:
    return (FALSE);                      /*              return empty handed */
}


int newsndinset(ENVIRON *csound, SOUNDINEW *p)       /* init routine for diskin   */
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
        14/02/94                -jpff
                A sndfile version
*****************************************************/
    int     n;
    char    *sfname, soundiname[128];
    SNDFILE *sinfd = NULL;
    long    nbytes, filno;
    MYFLT   skiptime = *p->iskptim;

    /* RWD 5:2001 need this as var, change size to read 24bit data */
    /* should go in SOUNDINEW struct eventually */
    long snewbufsize = SNDINEWBUFSIZ;

    if (*p->skipinit != FL(0.0)) return OK;
    if (skiptime < 0) {
      if (O.msglevel & WARNMSG)
        printf(Str("WARNING: negative skip time, substituting zero.\n"));
      skiptime = FL(0.0);
    }

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
          printf(Str(
                     "WARNING: skip time larger than audio data, "
                     "substituting zero.\n"));
        if (*p->ktransp < 0) {
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
                         (off_t)(nbytes+p->firstsampinfile)/sizeof(MYFLT),
                         SEEK_SET)) < 0)
        csound->Die(csound, Str("diskin seek error during reinit"));
      /* now rd fulbuf */
      if ((n = sreadinew(p->fdch.fd,p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
        p->endfile = 1;
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
      p->guardpt = p->bufend - csound->nchnls;
      p->phs = 0.0;

      return OK;
    }

    p->channel = ALLCHNLS;      /* reading all channels     */
    p->analonly = 0;

    /********  open the file  ***********/
    if ((n = p->OUTOCOUNT) &&
        n != 1 && n != 2 && n != 4 && n != 6 && n != 8) { /* if appl,chkchnls */
      sprintf(errmsg,Str("diskin: illegal no of receiving channels"));
      goto errtn;
    }
    if (*p->ifilno == SSTRCOD) { /* if char string name given */
      if (p->STRARG == NULL) strcpy(soundiname,unquote(currevent->strarg));
      else strcpy(soundiname,unquote(p->STRARG));    /* unquote it, else use */
    }
    else if ((filno=(long)*p->ifilno) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(soundiname, strsets[filno]);
    else sprintf(soundiname,"soundin.%ld",filno);  /* soundin.filno */
    sfname = soundiname;
    if (!sngetset(p, sfname))
      return FALSE;
    sinfd  = p->fdch.fd;

    /*******  display messages in verbose mode only */
    if (O.odebug) {
      printf(Str("audio sr = %ld, "), p->sr);
      if (p->nchanls == 1)
        printf(Str("monaural\n"));
      else {
        printf(Str("%s, reading "),
               p->nchanls == 2 ? Str("stereo") :
               p->nchanls == 4 ? Str("quad") :
               p->nchanls == 6 ? Str("hex") : Str("oct") );
        if (p->channel == ALLCHNLS)
          printf(Str("%s channels\n"),
                 p->nchanls == 2 ? Str("both") : Str("all"));
        else printf(Str("channel %d\n"), p->channel);
      }

      printf(Str("opening %s infile %s\n"),
             type2string(p->filetyp), sfname);
    }

    if (p->sampframsiz <= 0)    /* must know framsiz */
      csound->Die(csound, Str("illegal sampframsiz"));

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
    /*#### What does this actual;ly do??? */
    p->firstsampinfile = sf_seek(sinfd,(off_t)0L,SEEK_CUR);

    if ((p->audrem > 0) && (nbytes > p->audrem)) {
      if (O.msglevel & WARNMSG)
        printf(Str(
                   "WARNING: skip time larger than audio data, "
                   "substituting zero.\n"));
      nbytes = 0;
    }

    if (nbytes > 0) {
      if (p->audsize > 0 )      /* change audsize   */
        p->audrem = p->audsize-nbytes+p->firstsampinfile;

      if ((p->filepos =         /* seek to bndry */
           (long)sf_seek(sinfd,
                         (off_t)(nbytes)/sizeof(MYFLT)+p->firstsampinfile,
                         SEEK_SET)) < 0)
        csound->Die(csound, Str("diskin seek error: invalid skip time"));
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
    if (sinfd != NULL) {
      fdrecord(&p->fdch);              /*     instr will close later */

      p->guardpt = p->bufend - csound->nchnls;
      p->phs = 0.0;
      return OK;
    }
    else return csound->InitError(csound, errmsg);

 errtn:
    return NOTOK;                      /*              return empty handed */
}


/*  NB: floats not converted here, but in sreadinew():
    handles autorescale from PEAK, etc) */

void soundinew(ENVIRON *csound, SOUNDINEW *p)    /*  a-rate routine for soundinew */
{
    MYFLT       *r1, *r2, *r3, *r4, ktransp,looping;
    int         chnsout, n, ntogo, samplesLeft;
    double      phs,phsFract,phsTrunc;
    MYFLT       *inbufp = p->inbufp;
    long snewbufsize = SNDINEWBUFSIZ;            /*RWD 5:2001 */
    long oldfilepos = 0;

    if ((!p->bufend) || (!p->inbufp) || (!p->sampframsiz)) {
      csound->InitError(csound, Str("diskin: not initialised"));
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
    ntogo   = csound->ksmps;
    /*RWD 5:2001 need this when instr dur > filelen*/
    n = 0;
    /* RWD 5:2001 interesting issue - if ktransp starts at zero, we have
     * no idea what direction to go in!  below, it was "if ktransp > 0",
     * but the docs stipulate that only a negative transp signifies
     * backwards rendering, so really, ktransp=0 implies we go forwards */
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
/*             printf("*** [%d,%d] %d:(%f,%f)->%f\n", */
/*                    p->guardpt-inbufp, ntogo, */
/*                    (inbufp-p->inbuf), *inbufp, *(inbufp + 1), */
/*                    *inbufp + (*(inbufp + 1) - *inbufp) * phsFract); */
            *r1++ = inbufp[0] + (inbufp[1] - inbufp[0]) * phsFract;
            phs += ktransp;
            phsFract = modf(phs,&phsTrunc);
            inbufp = p->inbufp + (long)(phsTrunc);
            --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
          /*RWD 5:2001*/
          break;
        case 2:
          phsFract = modf(phs,&phsTrunc);
            do {
              *r1++ = inbufp[0] + (inbufp[2] - inbufp[0]) * phsFract;
              *r2++ = inbufp[1] + (inbufp[3] - inbufp[1]) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
            break;
        case 4:
          phsFract = modf(phs,&phsTrunc);
            do {
              *r1++ = inbufp[0] + (inbufp[4] - inbufp[0]) * phsFract;
              *r2++ = inbufp[1] + (inbufp[5] - inbufp[1]) * phsFract;
              *r3++ = inbufp[2] + (inbufp[6] - inbufp[2]) * phsFract;
              *r4++ = inbufp[3] + (inbufp[7] - inbufp[3]) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 4);
              --ntogo;
            } while ((inbufp < p->guardpt) && (ntogo));
        break;
        }

        samplesLeft = (int)(p->bufend - inbufp);
        if (samplesLeft <= csound->nchnls) {      /* first set file position
                                             to where inbuf p "thinks"
                                             its pointing to */
          p->filepos = (long)sf_seek(p->fdch.fd,
                                     (off_t)(-samplesLeft / chnsout),
                                     SEEK_CUR);
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
                                         (off_t)p->firstsampinfile/sizeof(MYFLT),
                                         SEEK_SET);
              if ((n = sreadinew(p->fdch.fd,
                                 p->inbuf,snewbufsize,p)) == 0) /*RWD 5:2001 */
                csound->Die(csound, Str("error trying to loop back to the beginning "
                        "of the sound file!?!??"));
              p->begfile = 1;
              phs = 0;
              inbufp = p->inbufp = p->inbuf;
              p->bufend = p->inbuf + n;
              /*RWD 5:2001 this cures the symptom (bad data in output sometimes,
               * when a transp sweep hits eof), but not, I suspect, the
               * underlying cause */
              if (n < snewbufsize)
                memset(p->bufend,0,sizeof(MYFLT)*(snewbufsize-n));
              p->guardpt = p->bufend - csound->nchnls;
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
              memset(p->bufend,0,sizeof(MYFLT)*(snewbufsize-n));
            p->guardpt = p->bufend - csound->nchnls;
            phs = modf(phs,&phsTrunc);
            p->begfile = FALSE;
          }
        }
#ifdef _DEBUG
        if (inbufp != p->bufend)
          assert(((p->bufend - inbufp) % p->sampframsiz)==0);
#endif
      }
    }
    else {      /* backwards...                 same thing but different */
      if (phs > 0 && p->endfile)  /*RWD 5:2001 as above */
        phs = 0; /* have just switched directions, forget (+ve) old phase */

      if (p->endfile) {   /* firewall-flag signaling when we are at either
                             end of the file */
        if (p->begfile)
          goto filend; /* make sure we are at beginning, not end */
        else {         /* RWD 5:2001: read in the first block (= last block of infile) */
          samplesLeft = (int)(inbufp - p->inbuf);
          if ((p->filepos = (long)sf_seek(p->fdch.fd,
                                          (off_t)(samplesLeft-snewbufsize),
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
               we're f****d because didn't get a full buffer and our
               present sample is the last sample of the buffer!!!  */
            csound->Die(csound, Str("diskin read error - during backwards playback"));
            return;
          }
          /* now get the correct remaining size */
          p->audrem = p->audsize - p->firstsampinfile - p->filepos;
          p->bufend = p->inbuf + n;
          /* point to the last sample in buffer */
          inbufp = p->inbufp = p->guardpt = p->bufend - csound->nchnls;

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
              *r1++ = inbufp[0] + (inbufp[0] - inbufp[-1]) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
        case 2:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
            do {
              *r1++ = inbufp[0] + (inbufp[0] - inbufp[-2]) * phsFract;
              *r2++ = inbufp[1] + (inbufp[1] - inbufp[-1]) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 2);
              --ntogo;
            } while ((inbufp > p->inbuf) && (ntogo));
            break;
        case 4:
          phsFract = modf(phs,&phsTrunc);       /*  phsFract will be negative */
          do {
              *r1++ = inbufp[0] + (inbufp[0] - inbufp[-4]) * phsFract;
              *r2++ = inbufp[1] + (inbufp[1] - inbufp[-3]) * phsFract;
              *r3++ = inbufp[2] + (inbufp[2] - inbufp[-2]) * phsFract;
              *r4++ = inbufp[3] + (inbufp[3] - inbufp[-1]) * phsFract;
              phs += ktransp;
              phsFract = modf(phs,&phsTrunc);
              inbufp = p->inbufp + (long)(phsTrunc * 8);
              --ntogo;
          } while ((inbufp > p->inbuf) && (ntogo));
          break;
        }

        if (inbufp <= p->inbuf) { /* we need to get some more samples!! */
          if (p->begfile) {
            if (looping) {      /* hopes this works -- set 1 buffer length
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
            samplesLeft = (int)(inbufp - p->inbuf + csound->nchnls);
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
                               (off_t)(samplesLeft-snewbufsize - snewbufsize),
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
            csound->Die(csound, Str("diskin read error - during backwards playback"));
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
          inbufp = p->inbufp = p->guardpt = p->bufend - csound->nchnls;
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
        do {
          *r1++ = FL(0.0);                /*    move in zeros     */
        } while (--ntogo);
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

int sndo1set(ENVIRON *csound, SNDOUT *p) /* init routine for instr soundout   */
{
    int    soutfd, filno;
    char   *sfname, sndoutname[128];
    SF_INFO sfinfo;
    SNDFILE *outfile;

    if (p->c.fdch.fd != NULL)   return OK;  /* if file already open, rtn  */
    if (*p->c.ifilcod == SSTRCOD)
      strcpy(sndoutname, unquote(p->STRARG));
    else if ((filno = (int)*p->c.ifilcod) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(sndoutname, strsets[filno]);
    else
      sprintf(sndoutname,"soundout.%d", filno);
    sfname = sndoutname;
    if ((soutfd = openout(sfname, 1)) < 0) {   /* if openout successful */
      sprintf(errmsg,Str("soundout cannot open %s"), sfname);
      goto errtn;
    }
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = 1;
    p->c.filetyp = TYP_RAW;
    switch ((int) (*(p->c.iformat) + FL(0.5))) {
      case 0: p->c.format = O.outformat; break;
      case 1: p->c.format = AE_CHAR; break;
      case 4: p->c.format = AE_SHORT; break;
      case 5: p->c.format = AE_LONG; break;
      case 6: p->c.format = AE_FLOAT; break;
      default:
        sprintf(errmsg, Str("soundout: invalid sample format: %d"),
                        (int) (*(p->c.iformat) + FL(0.5)));
        goto errtn;
    }
    sfinfo.format = TYPE2SF(p->c.filetyp) | FORMAT2SF(p->c.format);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfile = sf_open_fd(soutfd, SFM_WRITE, &sfinfo, SF_TRUE);
    if (p->c.format != AE_FLOAT)
      sf_command(outfile, SFC_SET_CLIPPING, NULL, SF_TRUE);
#ifdef USE_DOUBLE
    sf_command(outfile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
    sf_command(outfile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    sfname = retfilnam;
    printf(Str("opening %s outfile %s\n"), type2string(p->c.filetyp), sfname);
    p->c.outbufp = p->c.outbuf;             /* fix - isro 20-11-96 */
    p->c.bufend = p->c.outbuf + SNDOUTSMPS; /* fix - isro 20-11-96 */
    p->c.fdch.fd = outfile; /* WRONG */     /*     store & log the fd     */
    fdrecord(&p->c.fdch);                   /*     instr will close later */
    return OK;
 errtn:
    return csound->InitError(csound, errmsg);               /* else just print the errmsg */
}

int soundout(ENVIRON *csound, SNDOUT *p)
{
    MYFLT  *outbufp, *asig;
    int    nn, nsamps, ospace;

    asig = p->asig;
    outbufp = p->c.outbufp;
    nsamps = csound->ksmps;
    ospace = (p->c.bufend - outbufp);
 nchk:
    if ((nn = nsamps) > ospace)
      nn = ospace;
    nsamps -= nn;
    ospace -= nn;
    memmove(outbufp, asig, nn*sizeof(MYFLT));
    outbufp += nn; asig += nn;
/*     do { */
/*       *outbufp++ = *asig++; */
/*     } while (--nn); */
    if (!ospace) {              /* when buf is full  */
      sf_write_MYFLT(p->c.fdch.fd, p->c.outbuf, p->c.bufend - p->c.outbuf);
      outbufp = p->c.outbuf;
      ospace = SNDOUTSMPS;
      if (nsamps) goto nchk;    /*   chk rem samples */
    }
    p->c.outbufp = outbufp;
    return OK;
}

