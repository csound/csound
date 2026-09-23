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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "stdopcod.h"
#include <math.h>

#include "convolution.h"

typedef struct {
    CONV_OUTPUT out;
    int32_t     initDone;
    int32_t     nChannels;
    int32_t     cnt;            /* buffer position, 0 to partSize - 1       */
    int32_t     nPartitions;    /* number of convolve partitions            */
    int32_t     partSize;       /* partition length in sample frames        */
    int32_t     rbCnt;          /* ring buffer index, 0 to nPartitions - 1  */
    cs_float   *tmpBuf;            /* temporary buffer for accumulating FFTs   */
    cs_float   *ringBuf;           /* ring buffer of FFTs of input partitions  */
    cs_float   *IR_Data;           /* channel-major impulse spectra */
    cs_float   *outBuffers;        /* channel-major output and overlap */
    void  *fwdsetup, *invsetup;
    AUXCH   auxData;
} FTCONV_STATE;

typedef struct {
    OPDS h;
    cs_float *aOut[CONV_MAX_OUTPUTS];
    cs_float *aIn, *iFTNum, *iPartLen, *iSkipSamples, *iTotLen, *iSkipInit;
    FTCONV_STATE state;
} FTCONV;

typedef struct {
    OPDS h;
    ARRAYDAT *aOut;
    cs_float *aIn, *iFTNum, *iPartLen, *iChannels;
    cs_float *iSkipSamples, *iTotLen, *iSkipInit;
    FTCONV_STATE state;
} FTCONV_ARRAY;

static void multiply_fft_buffers(cs_float *outBuf, cs_float *ringBuf,
                                 cs_float *IR_Data, int32_t partSize,
                                 int32_t nPartitions,
                                 int32_t ringBuf_startPos)
{
    cs_float   re, im, re1, re2, im1, im2;
    cs_float   *rbPtr, *irPtr, *outBufPtr, *outBufEndPm2, *rbEndP;

    /* note: partSize must be at least 2 samples */
    partSize <<= 1;
    outBufEndPm2 = (cs_float*) outBuf + (int32_t) (partSize - 2);
    rbEndP = (cs_float*) ringBuf + (int32_t) (partSize * nPartitions);
    rbPtr = &(ringBuf[ringBuf_startPos]);
    irPtr = IR_Data;
    //outBufPtr = outBuf;
    /* clear output buffer to zero */
    memset(outBuf, 0, sizeof(cs_float)*(partSize));
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

static void set_buf_pointers(FTCONV_STATE *p, int32_t nChannels,
                             int32_t partSize, int32_t nPartitions)
{
    size_t fftSize = (size_t)partSize * 2;
    p->tmpBuf = (cs_float *)p->auxData.auxp;
    p->ringBuf = p->tmpBuf + fftSize;
    p->IR_Data = p->ringBuf + fftSize * nPartitions;
    p->outBuffers = p->IR_Data + fftSize * nPartitions * nChannels;
}

static int32_t ftconv_init_common(CSOUND *csound, OPDS *h, FTCONV_STATE *p,
                                  cs_float *ftnum, cs_float partLen, cs_float skipFrames,
                                  cs_float totalFrames, cs_float skipInit)
{
    FUNC *ftp;
    int32_t i, j, k, n;
    int32_t nChannels = p->out.channels;
    cs_double part = nearbyint((cs_double) partLen);
    cs_double skip = nearbyint((cs_double) skipFrames);
    cs_double length = nearbyint((cs_double) totalFrames);

    if (UNLIKELY(nChannels < 1))
      return csound->InitError(csound, "%s", Str("ftconv: invalid number of channels"));
    if (UNLIKELY(!(part >= 4 && part <= (INT32_MAX + 0.0) / 2)))
      return csound->InitError(csound, "%s",
                              Str("ftconv: invalid impulse response partition length"));
    int32_t partSize = (int32_t) part;
    if (UNLIKELY((partSize & (partSize - 1)) != 0))
      return csound->InitError(csound, "%s",
                              Str("ftconv: invalid impulse response partition length"));
    if (UNLIKELY(!(skip >= INT32_MIN && skip <= (INT32_MAX + 0.0) &&
                   length >= INT32_MIN && length <= (INT32_MAX + 0.0))))
      return csound->InitError(csound, "%s", Str("ftconv: invalid impulse response range"));
    ftp = csound->FTFind(csound, ftnum);
    if (UNLIKELY(ftp == NULL))
      return NOTOK;

    int64_t tableFrames = ftp->flen / nChannels;
    int64_t skipSamples = (int64_t) skip;
    int64_t irLength = tableFrames - skipSamples;
    if (length > 0 && irLength > (int64_t) length)
      irLength = (int64_t) length;
    if (UNLIKELY(irLength <= 0 || irLength > INT32_MAX))
      return csound->InitError(csound, "%s",
                              Str("ftconv: invalid length, or insufficient IR data for convolution"));
    int32_t nPartitions = (int32_t) ((irLength - 1) / partSize + 1);
    /* FFT and ring-buffer indices use signed 32-bit sample counts. */
    if (UNLIKELY(nPartitions > INT32_MAX / (partSize << 1)))
      return csound->InitError(csound, "%s", Str("ftconv: impulse response too large"));
    uint64_t nSamples = (uint64_t) (partSize << 1) *
                       ((uint64_t)nChannels + 1) * ((uint64_t) nPartitions + 1);
    if (UNLIKELY(nSamples > SIZE_MAX / sizeof(cs_float)))
      return csound->InitError(csound, "%s", Str("ftconv: impulse response too large"));
    size_t nBytes = (size_t) nSamples * sizeof(cs_float);

    if (conv_output_init(csound, h, &p->out) != OK)
      return NOTOK;

    /* Equal allocation sizes do not imply equal partition layouts. */
    if (p->initDone > 0 && skipInit != FL(0) &&
        p->nChannels == nChannels && p->partSize == partSize &&
        p->nPartitions == nPartitions)
      return OK;
    p->initDone = 0;
    p->nChannels = nChannels;
    p->partSize = partSize;
    p->nPartitions = nPartitions;
    if (p->auxData.auxp == NULL || nBytes != p->auxData.size)
      csound->AuxAlloc(csound, nBytes, &p->auxData);
    set_buf_pointers(p, nChannels, partSize, nPartitions);
    n = (partSize << 1) * nPartitions;
    memset(p->ringBuf, 0, (size_t) n * sizeof(cs_float));
    p->cnt = 0;
    p->rbCnt = 0;
    p->fwdsetup = csound->RealFFTSetup(csound, (partSize << 1), FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, (partSize << 1), FFT_INV);
    /* Store IR partitions in reverse order, padding beyond the requested end. */
    for (j = 0; j < nChannels; j++) {
      cs_float *ir = p->IR_Data + (size_t)j * (partSize << 1) * nPartitions;
      int64_t frame = skipSamples;
      int64_t endFrame = skipSamples + irLength;
      n = (partSize << 1) * (nPartitions - 1);
      do {
        for (k = 0; k < partSize; k++, frame++) {
          if (frame >= 0 && frame < tableFrames && frame < endFrame)
            ir[n + k] = ftp->ftable[frame * nChannels + j];
          else
            ir[n + k] = FL(0.0);
        }
        for (k = partSize; k < (partSize << 1); k++)
          ir[n + k] = FL(0.0);
        csound->RealFFT(csound, p->fwdsetup, &ir[n]);
        n -= (partSize << 1);
      } while (n >= 0);
    }
    for (j = 0; j < nChannels; j++)
      for (i = 0; i < (partSize << 1); i++)
        p->outBuffers[(size_t)j * (partSize << 1) + i] = FL(0.0);
    p->initDone = 1;
    return OK;
}

static int32_t ftconv_perf_common(CSOUND *csound, OPDS *h, FTCONV_STATE *p,
                                  cs_float *aIn)
{
    cs_float         *x, *rBuf;
    int32_t           i, n, nSamples, rBufPos;
    uint32_t offset = h->insdshead->ksmps_offset;
    uint32_t early  = h->insdshead->ksmps_no_end;
    uint32_t nn, nsmps = h->insdshead->ksmps;

    if (p->initDone <= 0) goto err1;
    if (conv_output_ready(csound, h, &p->out) != OK)
      return NOTOK;
    nSamples = p->partSize;
    rBuf = &(p->ringBuf[p->rbCnt * (nSamples << 1)]);
    if (UNLIKELY(offset))
      for (n = 0; n < p->nChannels; n++)
        memset(conv_output_channel(&p->out, n), '\0', offset*sizeof(cs_float));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (n = 0; n < p->nChannels; n++)
        memset(&conv_output_channel(&p->out, n)[nsmps], '\0', early*sizeof(cs_float));
    }
    for (nn = offset; nn < nsmps; nn++) {
      /* store input signal in buffer */
      rBuf[p->cnt] = aIn[nn];
      /* copy output signals from buffer */
      for (n = 0; n < p->nChannels; n++)
        conv_output_channel(&p->out, n)[nn] =
            p->outBuffers[(size_t)n * (nSamples << 1) + p->cnt];
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
        cs_float *ir = p->IR_Data + (size_t)n * (nSamples << 1) * p->nPartitions;
        multiply_fft_buffers(p->tmpBuf, p->ringBuf, ir,
                             nSamples, p->nPartitions, rBufPos);
        /* inverse FFT */
        csound->RealFFT(csound, p->invsetup, p->tmpBuf);
        /* copy to output buffer, overlap with "tail" of previous block */
        x = p->outBuffers + (size_t)n * (nSamples << 1);
        for (i = 0; i < nSamples; i++) {
          x[i] = p->tmpBuf[i] + x[i + nSamples];
          x[i + nSamples] = p->tmpBuf[i + nSamples];
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, h,
                             "%s", Str("ftconv: not initialised"));
}

static int32_t ftconv_init(CSOUND *csound, FTCONV *p)
{
    p->state.out = (CONV_OUTPUT){p->aOut, NULL, (int32_t)p->OUTOCOUNT};
    return ftconv_init_common(csound, &p->h, &p->state, p->iFTNum,
                             *p->iPartLen, *p->iSkipSamples, *p->iTotLen,
                             *p->iSkipInit);
}

static int32_t ftconv_array_init(CSOUND *csound, FTCONV_ARRAY *p)
{
    p->state.out = (CONV_OUTPUT){NULL, p->aOut, conv_array_channels(*p->iChannels)};
    return ftconv_init_common(csound, &p->h, &p->state, p->iFTNum,
                             *p->iPartLen, *p->iSkipSamples, *p->iTotLen,
                             *p->iSkipInit);
}

static int32_t ftconv_perf(CSOUND *csound, FTCONV *p)
{
    return ftconv_perf_common(csound, &p->h, &p->state, p->aIn);
}

static int32_t ftconv_array_perf(CSOUND *csound, FTCONV_ARRAY *p)
{
    return ftconv_perf_common(csound, &p->h, &p->state, p->aIn);
}

/* module interface functions */

int32_t ftconv_init_(CSOUND *csound)
{
    int32_t result = csound->AppendOpcode(csound, "ftconv",
                                (int32_t) sizeof(FTCONV), TR,
                                CONV_OUTPUT_TYPES, "aiiooo",
                                (int32_t (*)(CSOUND *, void *)) ftconv_init,
                                (int32_t (*)(CSOUND *, void *)) ftconv_perf,
                                NULL);
    if (result != OK) return result;
    return csound->AppendOpcode(csound, "ftconv",
                                (int32_t) sizeof(FTCONV_ARRAY), TR,
                                "a[]", "aiiiooo",
                                (int32_t (*)(CSOUND *, void *)) ftconv_array_init,
                                (int32_t (*)(CSOUND *, void *)) ftconv_array_perf,
                                NULL);
}
