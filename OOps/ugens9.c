/*
    ugens9.c:

    Copyright (C) 1996 Greg Sullivan, 2004 ma++ ingalls

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

#include "csdl.h"   /*                                      UGENS9.C        */
#include <math.h>
#include "fft.h"
#include "dsputil.h"
#include "convolve.h"
#include "ugens9.h"
#include "soundio.h"
#include "oload.h"

int cvset(ENVIRON *csound, CONVOLVE *p)
{
    char     cvfilnam[MAXNAME];
    MEMFIL   *mfp;
    CVSTRUCT *cvh;
    long     Hlenpadded = 1, obufsiz, Hlen;
    int      nchanls;

    if (csound->oparms_->odebug) printf(CONVOLVE_VERSION_STRING);

    if (*p->ifilno == SSTRCOD) {                    /* if strg name given */
      if (p->STRARG == NULL) strcpy(cvfilnam,unquote(currevent->strarg));
      else strcpy(cvfilnam, unquote(p->STRARG));
    }
    else if ((long)*p->ifilno <= strsmax && strsets != NULL &&
             strsets[(long)*p->ifilno])
      strcpy(cvfilnam, strsets[(long)*p->ifilno]);
    else sprintf(cvfilnam,
                 "convolve.%d", (int)*p->ifilno); /* else convolve.filnum   */
    if ((mfp = p->mfp) == NULL || strcmp(mfp->filename, cvfilnam) != 0)
                                /* if file not already readin */
      if ( (mfp = ldmemfile(csound, cvfilnam)) == NULL) {
        sprintf(errmsg,Str("CONVOLVE cannot load %s"), cvfilnam);
        goto cverr;
      }
    cvh = (CVSTRUCT *)mfp->beginp;
    if (cvh->magic != CVMAGIC) {
      sprintf(errmsg,Str("%s not a CONVOLVE file (magic %ld)"),
              cvfilnam, cvh->magic );
      goto cverr;
    }

    nchanls = (cvh->channel == ALLCHNLS ? cvh->src_chnls : 1);

    if (*p->channel == 0.) {
      if (p->OUTOCOUNT == nchanls)
        p->nchanls = nchanls;
      else {
        sprintf(errmsg,
                Str("CONVOLVE: output channels not equal "
                    "to number of channels in source"));
        goto cverr;
      }
    }
    else {
      if (*p->channel <= nchanls) {
        if (p->OUTOCOUNT != 1) {
          sprintf(errmsg,
                  Str("CONVOLVE: output channels not equal "
                      "to number of channels in source"));
          goto cverr;
        }
        else
          p->nchanls = 1;
      }
      else {
        sprintf(errmsg,
                Str("CONVOLVE: channel number greater than "
                    "number of channels in source"));
        goto cverr;
      }
    }
    Hlen = p->Hlen = cvh->Hlen;
    while (Hlenpadded < 2*Hlen-1)
      Hlenpadded <<= 1;
    p->Hlenpadded = Hlenpadded;
    p->H = (MYFLT *) ((char *)cvh+cvh->headBsize);
    if ((p->nchanls == 1) && (*p->channel > 0))
      p->H += (Hlenpadded + 2) * (int)(*p->channel - 1);

    if ((cvh->samplingRate) != esr &&
        (csound->oparms_->msglevel & WARNMSG)) {  /* & chk the data */
      printf(Str("WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
              cvfilnam, cvh->samplingRate, esr);
    }
    if (cvh->dataFormat != CVMYFLT) {
      sprintf(errmsg,Str("unsupported CONVOLVE data format %ld in %s"),
              cvh->dataFormat, cvfilnam);
      goto cverr;
    }

   /* Initialise FFT lookup table */
    p->cvlut = (MYFLT *)AssignBasis(NULL, Hlenpadded);

/* Determine size of circular output buffer */
    if (Hlen >= ksmps)
      obufsiz = (long)ceil( (double)Hlen / (double)ksmps ) * ksmps;
    else
      obufsiz = (long)ceil( (double)ksmps / (double)Hlen ) * Hlen;

    if (p->auxch.auxp == NULL) {              /* if no buffers yet, alloc now */
      MYFLT *fltp;
      auxalloc(csound, (long)(((Hlenpadded+2) + p->nchanls*((Hlen - 1) + obufsiz)
                       + (p->nchanls > 1 ? (Hlenpadded+2) : 0))*sizeof(MYFLT)),
               &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->fftbuf = fltp;   fltp += (Hlenpadded + 2); /* and insert addresses */
      p->olap = fltp;     fltp += p->nchanls*(Hlen - 1);
      p->outbuf = fltp;   fltp += p->nchanls*obufsiz;
      p->X  = fltp;
    }
    p->obufsiz = obufsiz;
    p->outcnt = obufsiz;
    p->incount = 0;
    p->obufend = p->outbuf + obufsiz - 1;
    p->outhead = p->outail = p->outbuf;
    return OK;
 cverr:
    return initerror(errmsg);
}

extern void writeFromCircBuf(MYFLT**, MYFLT**, MYFLT*, MYFLT*, long);

int convolve(ENVIRON *csound, CONVOLVE *p)
{
    int    nsmpso=ksmps,nsmpsi=ksmps,nsmpso_sav,outcnt_sav;
    int    nchm1 = p->nchanls - 1,chn;
    long   i,j;
    MYFLT  *ar[4];
    MYFLT  *ai = p->ain;
    MYFLT  *fftbufind;
    long   outcnt = p->outcnt;
    long   incount=p->incount;
    long   Hlen = p->Hlen;
    long   Hlenm1 = Hlen - 1;
    long   obufsiz = p->obufsiz;
    MYFLT  *outhead;
    MYFLT  *outail = p->outail;
    MYFLT  *olap;
    MYFLT  *X;
    long   Hlenpadded = p->Hlenpadded;

    ar[0] = p->ar1;
    ar[1] = p->ar2;
    ar[2] = p->ar3;
    ar[3] = p->ar4;

    if (p->auxch.auxp==NULL) {
      return perferror(Str("convolve: not initialised"));
    }
  /* First dump as much pre-existing audio in output buffer as possible */
    if (outcnt > 0) {
      if (outcnt <= ksmps)
        i = outcnt;
      else
        i = ksmps;
      nsmpso -= i; outcnt -= i;
      for (chn = nchm1;chn >= 0;chn--) {
        outhead = p->outhead + chn*obufsiz;
        writeFromCircBuf(&outhead,&ar[chn],p->outbuf+chn*obufsiz,
                         p->obufend+chn*obufsiz,i);
      }
      p->outhead = outhead;
    }
    while (nsmpsi > 0) {
/* Read input audio and place into work buffer. */

      fftbufind = p->fftbuf + incount;
      if ((incount + nsmpsi) <= Hlen)
        i = nsmpsi;
      else
        i = Hlen - incount;
      nsmpsi -= i;
      incount += i;
      while (i--)
        *fftbufind++ = *ai++;
      if (incount == Hlen) {
        /* We have enough audio for a convolution. */
        incount = 0;
        /* FFT the input (to create X) */
        /*printf("CONVOLVE: ABOUT TO FFT\n"); */
        FFT2realpacked((complex *)p->fftbuf,Hlenpadded,
                       (complex *)(p->cvlut));
        /* save the result if multi-channel */
        if (nchm1) {
          fftbufind = p->fftbuf;
          X = p->X;
          for (i = Hlenpadded + 2;i > 0;i--)
            *X++ = *fftbufind++;
        }
        nsmpso_sav = nsmpso;
        outcnt_sav = outcnt;
        for (chn = nchm1;chn >= 0;chn--) {
          outhead = p->outhead + chn*obufsiz;
          outail = p->outail + chn*obufsiz;
          olap = p->olap + chn*Hlenm1;
          if (chn < nchm1) {
            fftbufind = p->fftbuf;
            X = p->X;
            for (i = Hlenpadded + 2;i> 0;i--)
              *fftbufind++ = *X++;
          }
          /*printf("CONVOLVE: ABOUT TO MULTIPLY\n");  */
          /* Multiply H * X, point for point */
          cxmult((complex *)(p->H+chn*(Hlenpadded+2)),
                 (complex *)(p->fftbuf),Hlenpadded/2 + 1);

          /*printf("CONVOLVE: ABOUT TO IFFT\n"); */
          /* Perform inverse FFT on X */

          FFT2torlpacked((complex*)(p->fftbuf),Hlenpadded,
                         FL(1.0)/Hlenpadded,(complex*)(p->cvlut));

          /* Take the first Hlen output samples and output them to
             either the real audio output buffer or the local circular
             buffer */
          nsmpso = nsmpso_sav;
          outcnt = outcnt_sav;
          fftbufind = p->fftbuf;
          if ( (nsmpso > 0)&&(outcnt == 0) ) {
            /*    printf("Outputting to audio buffer proper\n");*/
            /* space left in output buffer, and nothing currently in circular
               buffer, so write as much as possible to output buffer first */
            if (nsmpso >= Hlenm1) {
              nsmpso -= Hlenm1;
              for (i=Hlenm1;i > 0;--i)
                *ar[chn]++ = *fftbufind++ + *olap++;
              if (nsmpso > 0) {
                *ar[chn]++ = *fftbufind++;
                --nsmpso;
              }
            }
            else {
              for (;nsmpso > 0;--nsmpso)
                *ar[chn]++ = *fftbufind++ + *olap++;
            }
          }
/* Any remaining output must go into circular buffer */
/*printf("Outputting to circ. buffer\n");*/
          i = Hlen - (fftbufind - p->fftbuf);
          outcnt += i;
          i--; /* do first Hlen -1 samples with overlap */
          j = p->obufend+chn*obufsiz - outail + 1;
          if (i >= j) {
            i -= j;
            for (;j > 0;--j)
              *outail++ = *fftbufind++ + *olap++;
            outail = p->outbuf+chn*obufsiz;
          }
          for (;i > 0;--i)
            *outail++ = *fftbufind++ + *olap++;
/*  just need to do sample at Hlen now */
          *outail++ = *fftbufind++;
          if (outail > p->obufend+chn*obufsiz)
            outail = p->outbuf+chn*obufsiz;

/*  Pad the rest to zero, and save first remaining (Hlen - 1) to overlap
    buffer */
          olap = p->olap+chn*Hlenm1;
          for (i = Hlenm1;i > 0;--i) {
            *olap++ = *fftbufind;
            *fftbufind++ = FL(0.0);
          }
          olap = p->olap+chn*Hlenm1;
    /* Now pad the rest to zero as well. In theory, this shouldn't be
       necessary, however it's conceivable that rounding errors may
       creep in, and these cells won't be exactly zero. So, let's
       make absolutely sure */
          for (i = Hlenpadded - (Hlen+Hlenm1);i > 0;--i)
            *fftbufind++ = FL(0.0);
        } /* end main for loop */
        p->outhead = outhead;
        p->outail = outail;
      }
    } /* end while */

/* update state in p */
    p->incount = incount;
    p->outcnt = outcnt;
    p->outhead = outhead;
    p->outail = outail;
    return OK;
}


/* partitioned (low latency) overlap-save convolution.
   we break up the IR into separate blocks, then perform
   an FFT on each partition.  For this reason we ONLY accept
   soundfiles as input, and do all of the traditional 'cvanal'
   processing at i-time.  it would be nice to eventually
   have cvanal create a partitioned format, which in turn would
   allow this opcode to accept .con files.
   -ma++ april 2004 */

extern SNDFILE *sndgetset(SOUNDIN *p);
extern long getsndin(SNDFILE *fd, MYFLT *fp, long nlocs, SOUNDIN *p);

int pconvset(ENVIRON *csound, PCONVOLVE *p)
{
    int      channel = (*(p->channel) <= 0 ? ALLCHNLS : *(p->channel));
    SNDFILE *infd;
    SOUNDIN IRfile;
    complex *basis;
    MYFLT   *inbuf, *fp1,*fp2;
    long    i, j, read_in, part;
    MYFLT   *IRblock;
    MYFLT   ainput_dur;

    static  MYFLT    format = FL(0.0), skptm = FL(0.0);
    static  MYFLT    sstrcod = (MYFLT)SSTRCOD;
    static  ARGOFFS  argofs = {0};
    static  OPTXT    optxt;

    /* open impulse response soundfile [code derived from SAsndgetset()] */
    optxt.t.outoffs = &argofs;    /* point to dummy OUTOCOUNT       */
    IRfile.h.optext = &optxt;
    IRfile.ifilno = &sstrcod;
    IRfile.iskptim = &skptm;
    IRfile.iformat = &format;
    IRfile.STRARG = p->STRARG;
    IRfile.sr = 0;
    if (channel < 1 || ((channel > 4) && (channel != ALLCHNLS))) {
      sprintf(errmsg, "channel request %d illegal\n", channel);
      return perferror(errmsg);
    }
    IRfile.channel = channel;
    IRfile.analonly = 1;
    if ((infd = sndgetset(&IRfile))==NULL) {
      return perferror("pconvolve: error while impulse file");
    }

    if ((IRfile.framesrem < 0) && (csound->oparms_->msglevel & WARNMSG)) {
      printf(Str("WARNING: undetermined file length, will attempt "
                 "requested duration"));
      ainput_dur = FL(0.0);     /* This is probably wrong -- JPff */
    }
    else {
        IRfile.getframes = IRfile.framesrem;
        ainput_dur = (MYFLT) IRfile.getframes / IRfile.sr;
      }

    printf(Str("analyzing %ld sample frames (%3.1f secs)\n"),
           IRfile.getframes, ainput_dur);

    p->nchanls = (channel != ALLCHNLS ? 1 : IRfile.nchanls);
    if (p->nchanls != p->OUTOCOUNT) {
      return perferror("PCONVOLVE: number of output channels not equal "
                       "to input channels");
    }

    if (IRfile.sr != esr && (csound->oparms_->msglevel & WARNMSG)) {
      /* ## RWD suggests performing sr conversion here! */
      printf("WARNING: IR srate != orch's srate");
    }

    /* make sure the partition size is nonzero and a power of 2  */
    if (*p->partitionSize <= 0)
      *p->partitionSize = csound->oparms_->outbufsamps;

    p->Hlen = 1;
    while (p->Hlen < *p->partitionSize)
      p->Hlen <<= 1;

    p->Hlenpadded = 2*p->Hlen;

    /* determine the number of partitions */
    p->numPartitions = ceil((MYFLT)(IRfile.getframes) / (MYFLT)p->Hlen);

    /* set up FFT tables */
    basis = AssignBasis(NULL, p->Hlenpadded);
    inbuf = (MYFLT *)mmalloc(csound, p->Hlen * p->nchanls * sizeof(MYFLT));
    auxalloc(csound, p->numPartitions * (p->Hlenpadded + 2) *
             sizeof(MYFLT) * p->nchanls, &p->H);
    IRblock = (MYFLT *)p->H.auxp;

    /* form each partition and take its FFT */
    for (part = 0; part < p->numPartitions; part++) {
      /* get the block of input samples and normalize -- soundin code
         handles finding the right channel */
      if ((read_in = getsndin(infd, inbuf, p->Hlen*p->nchanls, &IRfile)) <= 0)
        die("PCONVOLVE: less sound than expected!");

      /* take FFT of each channel */
      for (i = 0; i < p->nchanls; i++) {
        fp1 = inbuf + i;
        fp2 = IRblock;
        for (j = 0; j < read_in/p->nchanls; j++) {
          *fp2++ = *fp1 * dbfs_to_float;
          fp1 += p->nchanls;
        }
        FFT2realpacked((complex *)IRblock, p->Hlenpadded , basis);
        IRblock += (p->Hlenpadded + 2);
      }
    }

    mfree(csound, inbuf);
    sf_close(infd);

    /* Initialise input FFT lookup table */
    p->cvlut = (complex *)AssignBasis(NULL, p->Hlenpadded);

    /* allocate the buffer saving recent input samples */
    auxalloc(csound, p->Hlen * sizeof(MYFLT), &p->savedInput);
    p->inCount = 0;

    /* allocate the convolution work buffer */
    auxalloc(csound, (p->Hlenpadded+2) * sizeof(MYFLT), &p->workBuf);
    p->workWrite = (MYFLT *)p->workBuf.auxp + p->Hlen;

    /* allocate the buffer holding recent past convolutions */
    auxalloc(csound, (p->Hlenpadded+2) * p->numPartitions
             * p->nchanls * sizeof(MYFLT), &p->convBuf);
    p->curPart = 0;

    /* allocate circular output sample buffer */
    p->outBufSiz = sizeof(MYFLT) * p->nchanls *
      (p->Hlen >= ksmps ? p->Hlenpadded : 2*ksmps);
    auxalloc(csound, p->outBufSiz, &p->output);
    p->outRead = (MYFLT *)p->output.auxp;

    /* if ksmps < hlen, we have to pad initial output to prevent a possible
       empty ksmps pass after a few initial generated buffers.  There is
       probably an equation to figure this out to reduce the delay, but
       I can't seem to figure it out */
    if (p->Hlen > ksmps) {
      p->outCount = p->Hlen + ksmps;
      p->outWrite = p->outRead + (p->nchanls * p->outCount);
    }
    else {
      p->outCount = 0;
      p->outWrite = p->outRead;
    }
    return OK;
}

int pconvolve(ENVIRON *csound, PCONVOLVE *p)
{
    int    nsmpsi = ksmps;
    MYFLT  *ai = p->ain;
    MYFLT *buf, *input = p->savedInput.auxp, *workWrite = p->workWrite;
    MYFLT  *a1 = p->ar1, *a2 = p->ar2, *a3 = p->ar3, *a4 = p->ar4;
    long hlenp1 = p->Hlen + 1;
    long   i, j, count = p->inCount;
    long   hlenpaddedplus2 = p->Hlenpadded+2;

    while (nsmpsi-- > 0) {
      /* Read input audio and place into buffer. */
      input[count++] = *workWrite++ = *ai++;

      /* We have enough audio for a convolution. */
      if (count == p->Hlen) {
        complex *dest = (complex *)((MYFLT *)p->convBuf.auxp +
                                    p->curPart *(p->Hlenpadded+2) *p->nchanls);
        complex *h = (complex *)p->H.auxp;
        complex *workBuf = (complex *)p->workBuf.auxp;

        /* FFT the input (to create X) */
        *workWrite = FL(0.0); /* zero out nyquist bin from last fft result - maybe
                                 is ignored for input(?) but just in case.. */
        FFT2realpacked(workBuf, p->Hlenpadded, p->cvlut);

        /* for every IR partition convolve and add to previous convolves */
        for (i = 0; i < p->numPartitions*p->nchanls; i++) {
          complex *src = workBuf;
          long n = hlenp1;
          while (n--) {
            dest->re += (h->re * src->re) - (h->im * src->im);
            dest->im += (h->im * src->re) + (h->re * src->im);
            dest++; src++; h++;
          }

          if (dest == p->convBuf.endp)
            dest = p->convBuf.auxp;
        }

        /* Perform inverse FFT of the ondeck partion block */
        buf = (MYFLT *)p->convBuf.auxp + p->curPart * p->nchanls * hlenpaddedplus2;
        for (i = 0; i < p->nchanls; i++)
          FFT2torlpacked((complex *)(buf + i*hlenpaddedplus2),
                         p->Hlenpadded,
                         FL(1.0)/((MYFLT)p->Hlenpadded),
                         /* *p->numPartitions --needed??? above */
                         p->cvlut);
        /* We only take only the last Hlen output samples so we first zero out
           the first half for next time, then we copy the rest to output buffer */
        for (j = 0; j < p->nchanls; j++) {
          MYFLT *outp = p->outWrite + j;
          for (i = 0; i < p->Hlen; i++)
            *buf++ = FL(0.0);
          for (i = 0; i < p->Hlen; i++) {
            *outp = *buf;
            *buf++ = FL(0.0);
            outp += p->nchanls;
            if (outp >= (MYFLT *)p->output.endp)
              outp = (MYFLT *)p->output.auxp + j;
          }
          buf += 2;
        }
        p->outWrite += p->Hlen*p->nchanls;
        if (p->outWrite >= (MYFLT *)p->output.endp)
          p->outWrite -= p->outBufSiz/sizeof(MYFLT);
        p->outCount += p->Hlen;
        if (++p->curPart == p->numPartitions) /* advance to the next partition */
          p->curPart = 0;
        /* copy the saved input into the work buffer for next time around */
        memcpy(p->workBuf.auxp, input, p->Hlen * sizeof(MYFLT));
        count = 0;
        workWrite = (MYFLT *)p->workBuf.auxp + p->Hlen;
      }
    } /* end while */


    /* copy to output if we have enough samples [we always should
       except the first Hlen samples] */
    if (p->outCount >= ksmps) {
      int n;
      p->outCount -= ksmps;
      for (n=0; n < ksmps; n++) {
        switch (p->nchanls) {
        case 1:
          *a1++ = *p->outRead++;
          break;
        case 2:
          *a1++ = *p->outRead++;
          *a2++ = *p->outRead++;
          break;
        case 3:
          *a1++ = *p->outRead++;
          *a2++ = *p->outRead++;
          *a3++ = *p->outRead++;
          break;
        case 4:
          *a1++ = *p->outRead++;
          *a2++ = *p->outRead++;
          *a3++ = *p->outRead++;
          *a4++ = *p->outRead++;
          break;
        }
        if (p->outRead == p->output.endp)
          p->outRead = p->output.auxp;
      }
    }

    /* update struct */
    p->inCount = count;
    p->workWrite = workWrite;
    return OK;
}

