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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "stdopcod.h"   /*                                      UGENS9.C        */
#include <math.h>
#include "convolve.h"
#include "ugens9.h"
#include "soundio.h"
#include <inttypes.h>

static int32_t cvset_(CSOUND *csound, CONVOLVE *p, int32_t stringname)
{
  char     cvfilnam[MAXNAME];
  MEMFIL   *mfp;
  MYFLT    *fltp;
  CVSTRUCT *cvh;
  int32_t       siz;
  int32     Hlenpadded = 1, obufsiz, Hlen;
  uint32_t  nchanls;
  uint32_t  nsmps = CS_KSMPS;
  
  if (UNLIKELY(csound->GetDebug(csound)))
    csound->Message(csound, CONVOLVE_VERSION_STRING);

  if (stringname==0){
    if (IsStringCode(*p->ifilno))
      strncpy(cvfilnam,csound->GetString(csound, *p->ifilno), MAXNAME-1);
    else csound->StringArg2Name(csound, cvfilnam,p->ifilno, "convolve.",0);
  }
  else strncpy(cvfilnam, ((STRINGDAT *)p->ifilno)->data, MAXNAME-1);

  if ((mfp = p->mfp) == NULL || strcmp(mfp->filename, cvfilnam) != 0) {
    /* if file not already readin */
    if (UNLIKELY((mfp = csound->LoadMemoryFile(csound, cvfilnam,
                                               CSFTYPE_CVANAL,NULL))
                 == NULL)) {
      return csound->InitError(csound,
                               Str("CONVOLVE cannot load %s"), cvfilnam);
    }
  }
  cvh = (CVSTRUCT *)mfp->beginp;
  if (UNLIKELY(cvh->magic != CVMAGIC)) {
    return csound->InitError(csound,
                             Str("%s not a CONVOLVE file (magic %"PRIi32")"),
                             cvfilnam, cvh->magic);
  }

  nchanls = (cvh->channel == ALLCHNLS ? cvh->src_chnls : 1);

  if (*p->channel == FL(0.0)) {
    if (LIKELY(p->OUTOCOUNT == nchanls))
      p->nchanls = nchanls;
    else {
      return csound->InitError(csound,
                               "%s", Str("CONVOLVE: output channels not equal "
                                         "to number of channels in source"));
    }
  }
  else {
    if (*p->channel <= nchanls) {
      if (UNLIKELY(p->OUTOCOUNT != 1)) {
        return csound->InitError(csound,
                                 "%s", Str("CONVOLVE: output channels not equal "
                                           "to number of channels in source"));
      }
      else
        p->nchanls = 1;
    }
    else {
      return csound->InitError(csound,
                               "%s", Str("CONVOLVE: channel number greater than "
                                         "number of channels in source"));
    }
  }
  Hlen = p->Hlen = cvh->Hlen;
  while (Hlenpadded < 2*Hlen-1)
    Hlenpadded <<= 1;
  p->Hlenpadded = Hlenpadded;
  p->H = (MYFLT *) ((char *)cvh+cvh->headBsize);
  if ((p->nchanls == 1) && (*p->channel > 0))
    p->H += (Hlenpadded + 2) * (int32_t)(*p->channel - 1);

  if (UNLIKELY(cvh->samplingRate != CS_ESR)) {
    /* & chk the data */
    csound->Warning(csound, Str("%s's srate = %8.0f, orch's srate = %8.0f"),
                    cvfilnam, cvh->samplingRate, CS_ESR);
  }
  if (UNLIKELY(cvh->dataFormat != CVMYFLT)) {
    return csound->InitError(csound,
                             Str("unsupported CONVOLVE data "
                                 "format %"PRIi32" in %s"),
                             cvh->dataFormat, cvfilnam);
  }

  /* Determine size of circular output buffer */
  if (Hlen >= (int32)nsmps)
    obufsiz = (int32) CEIL((MYFLT) Hlen / nsmps) * nsmps;
  else
    obufsiz = (int32) CEIL(CS_KSMPS / (MYFLT) Hlen) * Hlen;

  siz = ((Hlenpadded + 2) + p->nchanls * ((Hlen - 1) + obufsiz)
         + (p->nchanls > 1 ? (Hlenpadded + 2) : 0));
  if (p->auxch.auxp == NULL || p->auxch.size < siz*sizeof(MYFLT)) {
    /* if no buffers yet, alloc now */
    csound->AuxAlloc(csound, (size_t) siz*sizeof(MYFLT), &p->auxch);
    fltp = (MYFLT *) p->auxch.auxp;
    p->fftbuf = fltp;   fltp += (Hlenpadded + 2); /* and insert addresses */
    p->olap = fltp;     fltp += p->nchanls*(Hlen - 1);
    p->outbuf = fltp;   fltp += p->nchanls*obufsiz;
    p->X  = fltp;
  }
  else {
    fltp = (MYFLT *) p->auxch.auxp;
    memset(fltp, 0, sizeof(MYFLT)*siz);
    /* for(i=0; i < siz; i++) fltp[i] = FL(0.0); */
  }
  p->obufsiz = obufsiz;
  p->outcnt = obufsiz;
  p->incount = 0;
  p->obufend = p->outbuf + obufsiz - 1;
  p->outhead = p->outail = p->outbuf;
  p->fwdsetup = csound->RealFFTSetup(csound, Hlenpadded, FFT_FWD);
  p->invsetup = csound->RealFFTSetup(csound, Hlenpadded, FFT_INV);
  return OK;
}

static int32_t cvset(CSOUND *csound, CONVOLVE *p){
  return cvset_(csound,p,0);

}

static int32_t cvset_S(CSOUND *csound, CONVOLVE *p){
  return cvset_(csound,p,1);

}
/* Write from a circular buffer into a linear output buffer without
   clearing data
   UPDATES SOURCE & DESTINATION POINTERS TO REFLECT NEW POSITIONS */
static void writeFromCircBuf(
                             MYFLT   **sce,
                             MYFLT   **dst,              /* Circular source and linear destination */
                             MYFLT   *sceStart,
                             MYFLT   *sceEnd,            /* Address of start & end of source buffer */
                             int32    numToDo)            /* How many points to write (<= circBufSize) */
{
  MYFLT   *srcindex = *sce;
  MYFLT   *dstindex = *dst;
  int32_t    breakPoint;     /* how many points to add before having to wrap */

  breakPoint = (int32_t) (sceEnd - srcindex + 1);
  if (numToDo >= breakPoint) { /*  we will do 2 in 1st loop, rest in 2nd. */
    numToDo -= breakPoint;
    for (; breakPoint > 0; --breakPoint) {
      *dstindex++ = *srcindex++;
    }
    srcindex = sceStart;
  }
  for (; numToDo > 0; --numToDo) {
    *dstindex++ = *srcindex++;
  }
  *sce = srcindex;
  *dst = dstindex;
  return;
}

static int32_t convolve(CSOUND *csound, CONVOLVE *p)
{
  int32_t    nsmpso=CS_KSMPS,nsmpsi=CS_KSMPS,outcnt_sav;
  int32_t    nchm1 = p->nchanls - 1,chn;
  int32  i,j;
  MYFLT  *ar[4];
  MYFLT  *ai = p->ain;
  MYFLT  *fftbufind;
  int32  outcnt = p->outcnt;
  int32  incount=p->incount;
  int32  Hlen = p->Hlen;
  int32  Hlenm1 = Hlen - 1;
  int32  obufsiz = p->obufsiz;
  MYFLT  *outhead = NULL;
  MYFLT  *outail = p->outail;
  MYFLT  *olap;
  MYFLT  *X;
  int32  Hlenpadded = p->Hlenpadded;
  MYFLT  scaleFac;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t nn,nsmpso_sav;

  scaleFac = csound->GetInverseRealFFTScale(csound, (int32_t) Hlenpadded);
  ar[0] = p->ar1;
  ar[1] = p->ar2;
  ar[2] = p->ar3;
  ar[3] = p->ar4;

  if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
  /* First dump as much pre-existing audio in output buffer as possible */
  if (outcnt > 0) {
    if (outcnt <= (int32_t)CS_KSMPS)
      i = outcnt;
    else
      i = CS_KSMPS;
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
    nsmpso_sav = CS_KSMPS-early;
    for (nn=0; i>0; nn++, i--) {
      if (nn<offset|| nn>(uint32_t)nsmpso_sav)
        *fftbufind++ = FL(0.0);
      else
        *fftbufind++ = scaleFac * ai[nn];
    }
    if (incount == Hlen) {
      /* We have enough audio for a convolution. */
      incount = 0;
      /* FFT the input (to create X) */
      /*csound->Message(csound, "CONVOLVE: ABOUT TO FFT\n"); */
      csound->RealFFT(csound, p->fwdsetup, p->fftbuf);
      p->fftbuf[Hlenpadded] = p->fftbuf[1];
      p->fftbuf[1] = p->fftbuf[Hlenpadded + 1L] = FL(0.0);
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
        /*csound->Message(csound, "CONVOLVE: ABOUT TO MULTIPLY\n");  */
        /* Multiply H * X, point for point */

        {
          MYFLT *a, *b, re, im;
          int32_t   i;
          a = (MYFLT*) p->H + (int32_t) (chn * (Hlenpadded + 2));
          b = (MYFLT*) p->fftbuf;
          for (i = 0; i <= (int32_t) Hlenpadded; i += 2) {
            re = a[i + 0] * b[i + 0] - a[i + 1] * b[i + 1];
            im = a[i + 0] * b[i + 1] + a[i + 1] * b[i + 0];
            b[i + 0] = re;
            b[i + 1] = im;
          }
        }

        /*csound->Message(csound, "CONVOLVE: ABOUT TO IFFT\n"); */
        /* Perform inverse FFT on X */

        p->fftbuf[1] = p->fftbuf[Hlenpadded];
        p->fftbuf[Hlenpadded] = p->fftbuf[Hlenpadded + 1L] = FL(0.0);
        csound->RealFFT(csound, p->invsetup, p->fftbuf);

        /* Take the first Hlen output samples and output them to
           either the real audio output buffer or the local circular
           buffer */
        nsmpso = nsmpso_sav;
        outcnt = outcnt_sav;
        fftbufind = p->fftbuf;
        if ( (nsmpso > 0)&&(outcnt == 0) ) {
          /*    csound->Message(csound, "Outputting to audio buffer proper\n");*/
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
        /*csound->Message(csound, "Outputting to circ. buffer\n");*/
        i = (int32_t) (Hlen - (fftbufind - p->fftbuf));
        outcnt += i;
        i--; /* do first Hlen -1 samples with overlap */
        j = (int32_t) (p->obufend+chn*obufsiz - outail + 1);
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
        //olap = p->olap+chn*Hlenm1;
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
 err1:
  return csound->PerfError(csound, &(p->h),
                           "%s", Str("convolve: not initialised"));
}

/* partitioned (low latency) overlap-save convolution.
   we break up the IR into separate blocks, then perform
   an FFT on each partition.  For this reason we ONLY accept
   soundfiles as input, and do all of the traditional 'cvanal'
   processing at i-time.  it would be nice to eventually
   have cvanal create a partitioned format, which in turn would
   allow this opcode to accept .con files.
   -ma++ april 2004 */

static int32_t pconvset_(CSOUND *csound, PCONVOLVE *p, int32_t stringname)
{
  int32_t     channel = (*(p->channel) <= 0 ? ALLCHNLS : (int32_t) *(p->channel));
  SNDFILE *infd;
  SFLIB_INFO  IRinfo;
  MYFLT   *inbuf, *fp1,*fp2;
  int32   i, j, part;
  MYFLT   *IRblock;
  MYFLT   ainput_dur, scaleFac;
  MYFLT   partitionSize;
  char *sfname = NULL;

  if (stringname==0){ 
    if (IsStringCode(*p->ifilno))
      sfname = csound->Strdup(csound, csound->GetString(csound, *p->ifilno));
    else 
      csound->StringArg2Name(csound, sfname, p->ifilno, "soundin.",0);
  }
  else sfname = ((STRINGDAT *)p->ifilno)->data;


  if (UNLIKELY(channel < 1 || ((channel > 4) && (channel != ALLCHNLS)))) {
    return csound->InitError(csound, Str("channel request %d illegal"), channel);
  }

  if (UNLIKELY((infd = csound->SndfileOpen(csound, sfname, SFM_READ, &IRinfo)) == NULL)) {
    return csound->InitError(csound, "%s", Str("pconvolve: error while impulse file"));
  }

  if (UNLIKELY(IRinfo.frames < 0)) {
    csound->Warning(csound, "%s", Str("undetermined file length, "
                                      "will attempt requested duration"));
    ainput_dur = FL(0.0);     /* This is probably wrong -- JPff */
  }
  else {
    if (UNLIKELY(IRinfo.samplerate==0)) return csound->InitError(csound, "%s", Str("SR zero"));
    ainput_dur = (MYFLT) IRinfo.frames / IRinfo.samplerate;
  }

  if(csound->GetDebug(csound))
  csound->Warning(csound, Str("analyzing %ld sample frames (%3.1f secs)\n"),
                  (long) IRinfo.frames, ainput_dur);

  p->nchanls = (channel != ALLCHNLS ? 1 : IRinfo.channels);
  if (UNLIKELY(p->nchanls != (int32_t)p->OUTOCOUNT)) {
    return csound->InitError(csound, "%s", Str("PCONVOLVE: number of output channels "
                                               "not equal to input channels"));
  }

  if (UNLIKELY(IRinfo.samplerate != (int32_t) CS_ESR)) {
    /* ## RWD suggests performing sr conversion here! */
    csound->Warning(csound, "%s", Str("IR srate != orch's srate"));
  }

  /* make sure the partition size is nonzero and a power of 2  */
  if (*p->partitionSize <= 0)
    partitionSize = (csound->GetOParms(csound))->outbufsamps / csound->GetNchnls(csound);
  else
    partitionSize = *p->partitionSize;

  p->Hlen = 1;
  while (p->Hlen < partitionSize)
    p->Hlen <<= 1;

  p->Hlenpadded = 2*p->Hlen;

  /* determine the number of partitions */
  p->numPartitions = CEIL((MYFLT)(IRinfo.frames) / (MYFLT)p->Hlen);

  /* set up FFT tables */
  inbuf = (MYFLT *) csound->Malloc(csound,
                                   p->Hlen * IRinfo.channels  * sizeof(MYFLT));
  csound->AuxAlloc(csound, p->numPartitions * (p->Hlenpadded + 2) *
                   sizeof(MYFLT) * p->nchanls, &p->H);
  IRblock = (MYFLT *)p->H.auxp;
  p->fwdsetup = csound->RealFFTSetup(csound,p->Hlenpadded, FFT_FWD);
  p->invsetup = csound->RealFFTSetup(csound,p->Hlenpadded, FFT_INV);
  
  /* form each partition and take its FFT */
  for (part = 0; part < p->numPartitions; part++) {
    int32_t start_chn = channel != ALLCHNLS ? channel-1 : 0;
    int64_t nframes;
    /* get the block of input frames */ 
    if (UNLIKELY((nframes = csound->SndfileRead(csound, infd, inbuf,
                                                p->Hlen)) <= 0))
      return csound->InitError(csound,
                               "%s", Str("PCONVOLVE: less sound than expected!"));

    /* take FFT of each channel */
    scaleFac = CS_ONEDDBFS
      * csound->GetInverseRealFFTScale(csound, (int32_t) p->Hlenpadded);
    for (i = start_chn; i < p->nchanls; i++) {
      fp1 = inbuf + i;
      fp2 = IRblock;
      for (j = 0; j < nframes/p->nchanls; j++) {
        *fp2++ = *fp1 * scaleFac;
        fp1 += p->nchanls;
      }

      csound->RealFFT(csound, p->fwdsetup, IRblock);
      IRblock[p->Hlenpadded] = IRblock[1];
      IRblock[1] = IRblock[p->Hlenpadded + 1L] = FL(0.0);
      IRblock += (p->Hlenpadded + 2);
    }
  }

  csound->Free(csound, inbuf);
  csound->SndfileClose(csound, infd);

  /* allocate the buffer saving recent input samples */
  csound->AuxAlloc(csound, p->Hlen * sizeof(MYFLT), &p->savedInput);
  p->inCount = 0;

  /* allocate the convolution work buffer */
  csound->AuxAlloc(csound, (p->Hlenpadded+2) * sizeof(MYFLT), &p->workBuf);
  p->workWrite = (MYFLT *)p->workBuf.auxp + p->Hlen;

  /* allocate the buffer holding recent past convolutions */
  csound->AuxAlloc(csound, (p->Hlenpadded+2) * p->numPartitions
                   * p->nchanls * sizeof(MYFLT), &p->convBuf);
  p->curPart = 0;

  /* allocate circular output sample buffer */
  p->outBufSiz = sizeof(MYFLT) * p->nchanls *
    (p->Hlen >= (int32_t)CS_KSMPS ? p->Hlenpadded : 2*(int32_t)CS_KSMPS);
  csound->AuxAlloc(csound, p->outBufSiz, &p->output);
  p->outRead = (MYFLT *)p->output.auxp;

  /* if ksmps < hlen, we have to pad initial output to prevent a possible
     empty ksmps pass after a few initial generated buffers.  There is
     probably an equation to figure this out to reduce the delay, but
     I can't seem to figure it out */
  if (p->Hlen > (int32_t)CS_KSMPS) {
    p->outCount = p->Hlen + CS_KSMPS;
    p->outWrite = p->outRead + (p->nchanls * p->outCount);
  }
  else {
    p->outCount = 0;
    p->outWrite = p->outRead;
  }
  return OK;
}

static int32_t pconvset(CSOUND *csound, PCONVOLVE *p){
  return pconvset_(csound,p,0);
}

static int32_t pconvset_S(CSOUND *csound, PCONVOLVE *p){
  return pconvset_(csound,p,1);
}

static int32_t pconvolve(CSOUND *csound, PCONVOLVE *p)
{
  uint32_t nn, nsmps = CS_KSMPS;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = nsmps - p->h.insdshead->ksmps_no_end;
  MYFLT  *ai = p->ain;
  MYFLT  *buf;
  MYFLT  *input = (MYFLT*) p->savedInput.auxp, *workWrite = p->workWrite;
  MYFLT  *a1 = p->ar1, *a2 = p->ar2, *a3 = p->ar3, *a4 = p->ar4;
  int32  i, j, count = p->inCount;
  int32  hlenpaddedplus2 = p->Hlenpadded+2;

  for (nn=0; nn<nsmps; nn++) {
    /* Read input audio and place into buffer. */
    input[count++] = *workWrite++ = (nn<offset||nn>early? FL(0.0) : ai[nn]);

    /* We have enough audio for a convolution. */
    if (count == p->Hlen) {
      MYFLT *dest = (MYFLT*) p->convBuf.auxp
        + p->curPart * (p->Hlenpadded + 2) * p->nchanls;
      MYFLT *h = (MYFLT*) p->H.auxp;
      MYFLT *workBuf = (MYFLT*) p->workBuf.auxp;

      /* FFT the input (to create X) */
      *workWrite = FL(0.0); /* zero out nyquist bin from last fft result
                               - maybe is ignored for input(?) but just in case.. */
      csound->RealFFT(csound, p->fwdsetup, workBuf);
      workBuf[p->Hlenpadded] = workBuf[1];
      workBuf[1] = workBuf[p->Hlenpadded + 1L] = FL(0.0);

      /* for every IR partition convolve and add to previous convolves */
      for (i = 0; i < p->numPartitions*p->nchanls; i++) {
        MYFLT *src = workBuf;
        int32_t n;
        for (n = 0; n <= (int32_t) p->Hlenpadded; n += 2) {
          dest[n + 0] += (h[n + 0] * src[n + 0]) - (h[n + 1] * src[n + 1]);
          dest[n + 1] += (h[n + 1] * src[n + 0]) + (h[n + 0] * src[n + 1]);
        }
        h += n; dest += n;
        if (UNLIKELY(dest == (MYFLT*)p->convBuf.endp))
          dest = (MYFLT*)p->convBuf.auxp;
      }

      /* Perform inverse FFT of the ondeck partion block */
      buf = (MYFLT*) p->convBuf.auxp
        + p->curPart * p->nchanls * hlenpaddedplus2;
      for (i = 0; i < p->nchanls; i++) {
        MYFLT *bufp;
        bufp = buf + i * hlenpaddedplus2;
        bufp[1] = bufp[p->Hlenpadded];
        bufp[p->Hlenpadded] = bufp[p->Hlenpadded + 1L] = FL(0.0);
        csound->RealFFT(csound, p->invsetup, bufp);
      }
      /* We only take only the last Hlen output samples so we first zero out
         the first half for next time, then we copy the rest to output buffer
      */
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
      if (++p->curPart == p->numPartitions)
        /* advance to the next partition */
        p->curPart = 0;
      /* copy the saved input into the work buffer for next time around */
      memcpy(p->workBuf.auxp, input, p->Hlen * sizeof(MYFLT));
      count = 0;
      workWrite = (MYFLT *)p->workBuf.auxp + p->Hlen;
    }
  } /* end while */

    /* copy to output if we have enough samples [we always should
       except the first Hlen samples] */
  if (p->outCount >= (int32_t)CS_KSMPS) {
    uint32_t n;
    p->outCount -= CS_KSMPS;
    for (n=0; n < CS_KSMPS; n++) {
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

static OENTRY localops[] =
  {
   { "convolve", sizeof(CONVOLVE),   0, "mmmm", "aSo",
            (SUBR) cvset_S,    (SUBR) convolve   },
   { "convle",   sizeof(CONVOLVE),   0,  "mmmm", "aSo",
            (SUBR) cvset_S,    (SUBR) convolve   },
   { "pconvolve",sizeof(PCONVOLVE),  0,  "mmmm", "aSoo",
      (SUBR) pconvset_S,    (SUBR) pconvolve  },
   { "convolve.i", sizeof(CONVOLVE),   0,  "mmmm", "aio",
            (SUBR) cvset,    (SUBR) convolve   },
   { "convle.i",   sizeof(CONVOLVE),   0,  "mmmm", "aio",
            (SUBR) cvset,    (SUBR) convolve   },
   { "pconvolve.i",sizeof(PCONVOLVE),  0, "mmmm", "aioo",
            (SUBR) pconvset,    (SUBR) pconvolve  }
};


int32_t ugens9_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}
