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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
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
    int     initDone;
    int     nChannels;
    int     cnt;                /* buffer position, 0 to partSize - 1       */
    int     nPartitions;        /* number of convolve partitions            */
    int     partSize;           /* partition length in sample frames        */
    int     rbCnt;              /* ring buffer index, 0 to nPartitions - 1  */
    MYFLT   *tmpBuf;            /* temporary buffer for accumulating FFTs   */
    MYFLT   *ringBuf;           /* ring buffer of FFTs of input partitions  */
    MYFLT   *IR_Data[FTCONV_MAXCHN];    /* impulse responses (scaled)       */
    MYFLT   *outBuffers[FTCONV_MAXCHN]; /* output buffer (size=partSize*2)  */
    AUXCH   auxData;
} FTCONV;

static void multiply_fft_buffers(MYFLT *outBuf, MYFLT *ringBuf,
                                 MYFLT *IR_Data, int partSize, int nPartitions,
                                 int ringBuf_startPos)
{
    MYFLT   re, im, re1, re2, im1, im2;
    MYFLT   *rbPtr, *irPtr, *outBufPtr, *outBufEndPm2, *rbEndP;

    /* note: partSize must be at least 2 samples */
    partSize <<= 1;
    outBufEndPm2 = (MYFLT*) outBuf + (int) (partSize - 2);
    rbEndP = (MYFLT*) ringBuf + (int) (partSize * nPartitions);
    rbPtr = &(ringBuf[ringBuf_startPos]);
    irPtr = IR_Data;
    outBufPtr = outBuf;
    /* clear output buffer to zero */
    do {
      *(outBufPtr++) = FL(0.0);
      *(outBufPtr++) = FL(0.0);
    } while (outBufPtr <= outBufEndPm2);
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

static int buf_bytes_alloc(int nChannels, int partSize, int nPartitions)
{
    int nSmps;

    nSmps = (partSize << 1);                                /* tmpBuf     */
    nSmps += ((partSize << 1) * nPartitions);               /* ringBuf    */
    nSmps += ((partSize << 1) * nChannels * nPartitions);   /* IR_Data    */
    nSmps += ((partSize << 1) * nChannels);                 /* outBuffers */

    return ((int) sizeof(MYFLT) * nSmps);
}

static void set_buf_pointers(FTCONV *p,
                             int nChannels, int partSize, int nPartitions)
{
    MYFLT *ptr;
    int   i;

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

#define RNDINT(x) ((int) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

static int ftconv_init(ENVIRON *csound, FTCONV *p)
{
    FUNC    *ftp;
    int     i, j, k, n, nBytes, skipSamples;
    MYFLT   FFTscale;

    /* check parameters */
    p->nChannels = (int) p->OUTOCOUNT;
    if (p->nChannels < 1 || p->nChannels > FTCONV_MAXCHN) {
      csound->InitError(csound, Str("ftconv: invalid number of channels"));
      return NOTOK;
    }
    /* partition length */
    p->partSize = RNDINT(*(p->iPartLen));
    if (p->partSize < 4 || (p->partSize & (p->partSize - 1)) != 0) {
      csound->InitError(csound, Str("ftconv: invalid impulse response "
                                    "partition length"));
      return NOTOK;
    }
    ftp = csound->FTFind(csound, p->iFTNum);
    if (ftp == NULL)
      return NOTOK; /* ftfind should already have printed the error message */
    /* calculate total length / number of partitions */
    n = (int) ftp->flen / p->nChannels;
    skipSamples = RNDINT(*(p->iSkipSamples));
    n -= skipSamples;
    if (RNDINT(*(p->iTotLen)) > 0 && n > RNDINT(*(p->iTotLen)))
      n = RNDINT(*(p->iTotLen));
    if (n <= 0) {
      csound->InitError(csound, Str("ftconv: invalid length, "
                                    "or insufficient IR data for convolution"));
      return NOTOK;
    }
    p->nPartitions = (n + (p->partSize - 1)) / p->partSize;
    /* calculate the amount of aux space to allocate (in bytes) */
    nBytes = buf_bytes_alloc(p->nChannels, p->partSize, p->nPartitions);
    if (nBytes != (int) p->auxData.size)
      csound->AuxAlloc(csound, (long) nBytes, &(p->auxData));
    else if (p->initDone > 0 && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    /* if skipping samples: check for possible truncation of IR */
    if (skipSamples > 0 && (csound->oparms_->msglevel & 4)) {
      n = skipSamples * p->nChannels;
      if (n > (int) ftp->flen)
        n = (int) ftp->flen;
      for (i = 0; i < n; i++) {
        if (ftp->ftable[i] != FL(0.0)) {
          csound->Message(csound,
                          Str("ftconv: warning: skipped non-zero samples, "
                              "impulse response may be truncated\n"));
          break;
        }
      }
    }
    /* initialise buffer pointers */
    set_buf_pointers(p, p->nChannels, p->partSize, p->nPartitions);
    /* clear ring buffer to zero */
    n = (p->partSize << 1) * p->nPartitions;
    for (i = 0; i < n; i++)
      p->ringBuf[i] = FL(0.0);
    /* initialise buffer index */
    p->cnt = 0;
    p->rbCnt = 0;
    /* calculate FFT of impulse response partitions, in reverse order */
    /* also apply FFT amplitude scale here */
    FFTscale = csound->GetInverseRealFFTScale(csound, (p->partSize << 1));
    for (j = 0; j < p->nChannels; j++) {
      i = (skipSamples * p->nChannels) + j;           /* table read position */
      n = (p->partSize << 1) * (p->nPartitions - 1);  /* IR write position */
      do {
        for (k = 0; k < p->partSize; k++) {
          if (i >= 0 && i < (int) ftp->flen)
            p->IR_Data[j][n + k] = ftp->ftable[i] * FFTscale;
          else
            p->IR_Data[j][n + k] = FL(0.0);
          i += p->nChannels;
        }
        /* pad second half of IR to zero */
        for (k = p->partSize; k < (p->partSize << 1); k++)
          p->IR_Data[j][n + k] = FL(0.0);
        /* calculate FFT */
        csound->RealFFT(csound, &(p->IR_Data[j][n]), (p->partSize << 1));
        n -= (p->partSize << 1);
      } while (n >= 0);
    }
    /* clear output buffers to zero */
    for (j = 0; j < p->nChannels; j++) {
      for (i = 0; i < (p->partSize << 1); i++)
        p->outBuffers[j][i] = FL(0.0);
    }
    p->initDone = 1;

    return OK;
}

static int ftconv_perf(ENVIRON *csound, FTCONV *p)
{
    MYFLT         *x, *rBuf;
    int           i, n, nn, nSamples, rBufPos;

    if (p->initDone <= 0) {
      csound->PerfError(csound, Str("ftconv: not initialised"));
      return NOTOK;
    }
    nSamples = p->partSize;
    rBuf = &(p->ringBuf[p->rbCnt * (nSamples << 1)]);
    for (nn = 0; nn < csound->ksmps_; nn++) {
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
      csound->RealFFT(csound, rBuf, (nSamples << 1));
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
        csound->InverseRealFFT(csound, p->tmpBuf, (nSamples << 1));
        /* copy to output buffer, overlap with "tail" of previous block */
        x = &(p->outBuffers[n][0]);
        for (i = 0; i < nSamples; i++) {
          x[i] = p->tmpBuf[i] + x[i + nSamples];
          x[i + nSamples] = p->tmpBuf[i + nSamples];
        }
      }
    }
    return OK;
}

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    return 0;
}

int csoundModuleInit(void *csound_)
{
    ENVIRON *csound;
    csound = (ENVIRON*) csound_;
    return (csound->AppendOpcode(csound, "ftconv",
                                 (int) sizeof(FTCONV), 5, "mmmmmmmm", "aiiooo",
                                 (int (*)(void*, void*)) ftconv_init,
                                 (int (*)(void*, void*)) NULL,
                                 (int (*)(void*, void*)) ftconv_perf,
                                 (int (*)(void*, void*)) NULL));
}

