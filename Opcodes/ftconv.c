/*
    ftconv.c:

    Copyright (C) 2005 Istvan Varga

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

#include "stdopcod.h"
#include <math.h>

#define FTCONV_MAXCHN   8

typedef struct {
    OPDS    h;
    MYFLT   *aOut[FTCONV_MAXCHN];
    MYFLT   *aIn;
    MYFLT   *iFTNum;
    MYFLT   *iPartLen;
    MYFLT   *iSkipSamples;
    MYFLT   *iTotLen;
    MYFLT   *iSkipInit;
 /* ------------------------- */
    int32_t     initDone;
    int32_t     nChannels;
    int32_t     cnt;            /* buffer position, 0 to partSize - 1       */
    int32_t     nPartitions;    /* number of convolve partitions            */
    int32_t     partSize;       /* partition length in sample frames        */
    int32_t     rbCnt;          /* ring buffer index, 0 to nPartitions - 1  */
    MYFLT   *tmpBuf;            /* temporary buffer for accumulating FFTs   */
    MYFLT   *ringBuf;           /* ring buffer of FFTs of input partitions  */
    MYFLT   *IR_Data[FTCONV_MAXCHN];    /* impulse responses (scaled)       */
    MYFLT   *outBuffers[FTCONV_MAXCHN]; /* output buffer (size=partSize*2)  */
    void  *fwdsetup, *invsetup;
    AUXCH   auxData;
} FTCONV;

static void multiply_fft_buffers(MYFLT *outBuf, MYFLT *ringBuf,
                                 MYFLT *IR_Data, int32_t partSize,
                                 int32_t nPartitions,
                                 int32_t ringBuf_startPos)
{
    MYFLT   re, im, re1, re2, im1, im2;
    MYFLT   *rbPtr, *irPtr, *outBufPtr, *outBufEndPm2, *rbEndP;

    /* note: partSize must be at least 2 samples */
    partSize <<= 1;
    outBufEndPm2 = (MYFLT*) outBuf + (int32_t) (partSize - 2);
    rbEndP = (MYFLT*) ringBuf + (int32_t) (partSize * nPartitions);
    rbPtr = &(ringBuf[ringBuf_startPos]);
    irPtr = IR_Data;
    //outBufPtr = outBuf;
    /* clear output buffer to zero */
    memset(outBuf, 0, sizeof(MYFLT)*(partSize));
    /* do { */
    /*   *(outBufPtr++) = FL(0.0); */
    /*   *(outBufPtr++) = FL(0.0); */
    /* } while (outBufPtr <= outBufEndPm2); */
    /* multiply FFTs for each partition, and mix to output buffer */
    /* note: IRs are stored in reverse partition order */
    do {
      /* wrap ring buffer position */
      if (rbPtr >= rbEndP)
        rbPtr = ringBuf;
      outBufPtr = outBuf;
      *(outBufPtr++) += *(rbPtr++) * *(irPtr++);    /* convolve DC */
      *(outBufPtr++) += *(rbPtr++) * *(irPtr++);    /* convolve Nyquist */
      re1 = *(rbPtr++);
      im1 = *(rbPtr++);
      re2 = *(irPtr++);
      im2 = *(irPtr++);
      re = re1 * re2 - im1 * im2;
      im = re1 * im2 + re2 * im1;
      while (outBufPtr < outBufEndPm2) {
        /* complex multiply */
        re1 = rbPtr[0];
        im1 = rbPtr[1];
        re2 = irPtr[0];
        im2 = irPtr[1];
        outBufPtr[0] += re;
        outBufPtr[1] += im;
        re = re1 * re2 - im1 * im2;
        im = re1 * im2 + re2 * im1;
        re1 = rbPtr[2];
        im1 = rbPtr[3];
        re2 = irPtr[2];
        im2 = irPtr[3];
        outBufPtr[2] += re;
        outBufPtr[3] += im;
        re = re1 * re2 - im1 * im2;
        im = re1 * im2 + re2 * im1;
        outBufPtr += 4;
        rbPtr += 4;
        irPtr += 4;
      }
      outBufPtr[0] += re;
      outBufPtr[1] += im;
    } while (--nPartitions);
}

static inline int32_t buf_bytes_alloc(int32_t nChannels,
                                      int32_t partSize, int32_t nPartitions)
{
    int32_t nSmps;

    nSmps = (partSize << 1);                                /* tmpBuf     */
    nSmps += ((partSize << 1) * nPartitions);               /* ringBuf    */
    nSmps += ((partSize << 1) * nChannels * nPartitions);   /* IR_Data    */
    nSmps += ((partSize << 1) * nChannels);                 /* outBuffers */

    return ((int32_t) sizeof(MYFLT) * nSmps);
}

static void set_buf_pointers(FTCONV *p,
                             int32_t nChannels, int32_t partSize,
                             int32_t nPartitions)
{
    MYFLT *ptr;
    int32_t   i;

    ptr = (MYFLT*) (p->auxData.auxp);
    p->tmpBuf = ptr;
    ptr += (partSize << 1);
    p->ringBuf = ptr;
    ptr += ((partSize << 1) * nPartitions);
    for (i = 0; i < nChannels; i++) {
      p->IR_Data[i] = ptr;
      ptr += ((partSize << 1) * nPartitions);
    }
    for (i = 0; i < nChannels; i++) {
      p->outBuffers[i] = ptr;
      ptr += (partSize << 1);
    }
}

static int32_t ftconv_init(CSOUND *csound, FTCONV *p)
{
    FUNC    *ftp;
    int32_t     i, j, k, n, nBytes, skipSamples;
    //MYFLT   FFTscale;

    /* check parameters */
    p->nChannels = (int32_t) p->OUTOCOUNT;
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > FTCONV_MAXCHN)) {
      return csound->InitError(csound, "%s", Str("ftconv: invalid number of channels"));
    }
    /* partition length */
    p->partSize = MYFLT2LRND(*(p->iPartLen));
    if (UNLIKELY(p->partSize < 4 || (p->partSize & (p->partSize - 1)) != 0)) {
      return csound->InitError(csound, "%s", Str("ftconv: invalid impulse response "
                                           "partition length"));
    }
    ftp = csound->FTFind(csound, p->iFTNum);
    if (UNLIKELY(ftp == NULL))
      return NOTOK; /* ftfind should already have printed the error message */
    /* calculate total length / number of partitions */
    n = (int32_t) ftp->flen / p->nChannels;
    skipSamples = MYFLT2LRND(*(p->iSkipSamples));
    n -= skipSamples;
    if (MYFLT2LRND(*(p->iTotLen)) > 0 && n > MYFLT2LRND(*(p->iTotLen)))
      n = MYFLT2LRND(*(p->iTotLen));
    if (UNLIKELY(n <= 0)) {
      return csound->InitError(csound,
                               "%s", Str("ftconv: invalid length, or insufficient"
                                   " IR data for convolution"));
    }
    p->nPartitions = (n + (p->partSize - 1)) / p->partSize;
    /* calculate the amount of aux space to allocate (in bytes) */
    nBytes = buf_bytes_alloc(p->nChannels, p->partSize, p->nPartitions);
    if (nBytes != (int32_t) p->auxData.size)
      csound->AuxAlloc(csound, (int32) nBytes, &(p->auxData));
    else if (p->initDone > 0 && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    /* if skipping samples: check for possible truncation of IR */
    /*
      if (skipSamples > 0 && (parm->msglevel & WARNMSG)) {
      n = skipSamples * p->nChannels;
      if (n > (int32_t) ftp->flen)
        n = (int32_t) ftp->flen;
      for (i = 0; i < n; i++) {
        if (UNLIKELY(ftp->ftable[i] != FL(0.0))) {
          csound->Warning(csound,
                          "%s", Str("ftconv: skipped non-zero samples, "
                              "impulse response may be truncated\n"));
          break;
        }
      }
      }*/
    /* initialise buffer pointers */
    set_buf_pointers(p, p->nChannels, p->partSize, p->nPartitions);
    /* clear ring buffer to zero */
    n = (p->partSize << 1) * p->nPartitions;
    memset(p->ringBuf, 0, n*sizeof(MYFLT));
    /* for (i = 0; i < n; i++) */
    /*   p->ringBuf[i] = FL(0.0); */
    /* initialise buffer index */
    p->cnt = 0;
    p->rbCnt = 0;
    /* calculate FFT of impulse response partitions, in reverse order */
    /* also apply FFT amplitude scale here */
    //FFTscale = csound->GetInverseRealFFTScale(csound, (p->partSize << 1));
    p->fwdsetup = csound->RealFFTSetup(csound,(p->partSize << 1), FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound,(p->partSize << 1), FFT_INV);
    for (j = 0; j < p->nChannels; j++) {
      i = (skipSamples * p->nChannels) + j;           /* table read position */
      n = (p->partSize << 1) * (p->nPartitions - 1);  /* IR write position */
      do {
        for (k = 0; k < p->partSize; k++) {
          if (i >= 0 && i < (int32_t) ftp->flen)
            p->IR_Data[j][n + k] = ftp->ftable[i];// * FFTscale;
          else
            p->IR_Data[j][n + k] = FL(0.0);
          i += p->nChannels;
        }
        /* pad second half of IR to zero */
        for (k = p->partSize; k < (p->partSize << 1); k++)
          p->IR_Data[j][n + k] = FL(0.0);
        /* calculate FFT */
        csound->RealFFT(csound, p->fwdsetup, &(p->IR_Data[j][n]));
        n -= (p->partSize << 1);
      } while (n >= 0);
    }
    /* clear output buffers to zero */
    /*memset(p->outBuffers, 0, p->nChannels*(p->partSize << 1)*sizeof(MYFLT));*/
    for (j = 0; j < p->nChannels; j++) {
      for (i = 0; i < (p->partSize << 1); i++)
        p->outBuffers[j][i] = FL(0.0);
    }
    p->initDone = 1;

    return OK;
}

static int32_t ftconv_perf(CSOUND *csound, FTCONV *p)
{
    MYFLT         *x, *rBuf;
    int32_t           i, n, nSamples, rBufPos;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;

    if (p->initDone <= 0) goto err1;
    nSamples = p->partSize;
    rBuf = &(p->ringBuf[p->rbCnt * (nSamples << 1)]);
    if (UNLIKELY(offset))
      for (n = 0; n < p->nChannels; n++)
        memset(p->aOut[n], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (n = 0; n < p->nChannels; n++)
        memset(&p->aOut[n][nsmps], '\0', early*sizeof(MYFLT));
    }
    for (nn = offset; nn < nsmps; nn++) {
      /* store input signal in buffer */
      rBuf[p->cnt] = p->aIn[nn];
      /* copy output signals from buffer */
      for (n = 0; n < p->nChannels; n++)
        p->aOut[n][nn] = p->outBuffers[n][p->cnt];
      /* is input buffer full ? */
      if (++p->cnt < nSamples)
        continue;                   /* no, continue with next sample */
      /* reset buffer position */
      p->cnt = 0;
      /* calculate FFT of input */
      for (i = nSamples; i < (nSamples << 1); i++)
        rBuf[i] = FL(0.0);          /* pad to double length */
      csound->RealFFT(csound, p->fwdsetup, rBuf);
      /* update ring buffer position */
      p->rbCnt++;
      if (p->rbCnt >= p->nPartitions)
        p->rbCnt = 0;
      rBufPos = p->rbCnt * (nSamples << 1);
      rBuf = &(p->ringBuf[rBufPos]);
      /* for each channel: */
      for (n = 0; n < p->nChannels; n++) {
        /* multiply complex arrays */
        multiply_fft_buffers(p->tmpBuf, p->ringBuf, p->IR_Data[n],
                             nSamples, p->nPartitions, rBufPos);
        /* inverse FFT */
        csound->RealFFT(csound, p->invsetup, p->tmpBuf);
        /* copy to output buffer, overlap with "tail" of previous block */
        x = &(p->outBuffers[n][0]);
        for (i = 0; i < nSamples; i++) {
          x[i] = p->tmpBuf[i] + x[i + nSamples];
          x[i + nSamples] = p->tmpBuf[i + nSamples];
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("ftconv: not initialised"));
}

/* module interface functions */

int32_t ftconv_init_(CSOUND *csound)
{
    return csound->AppendOpcode(csound, "ftconv",
                                (int32_t) sizeof(FTCONV), TR,
                                "mmmmmmmm", "aiiooo",
                                (int32_t (*)(CSOUND *, void *)) ftconv_init,
                                (int32_t (*)(CSOUND *, void *)) ftconv_perf,
                                NULL);
}

