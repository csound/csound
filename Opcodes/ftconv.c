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
    int     nSamples;   /* IR length (not padded) in samples                */
    int     cnt;        /* buffer position, counts from 0 to nSamples       */
    int     nChannels;  /* number of channels (1 to 8)                      */
    MYFLT   *inBuffer;  /* input/FFT buffer (size = nSamples*2)             */
    MYFLT   *tmpBuf;    /* temporary buffer for FFT (size = nSamples*2)     */
    MYFLT   *outBuffers[FTCONV_MAXCHN];
                        /* output buffer (size = nSamples*2)                */
    MYFLT   *IR_Data[FTCONV_MAXCHN];
                        /* FFT of impulse response (size = nSamples*2)      */
    MYFLT   data[1];
} CONVOLVE_FFT;

typedef struct {
    OPDS          h;
    MYFLT         *aOut[FTCONV_MAXCHN];
    MYFLT         *aIn;
    MYFLT         *iFTNum;
    MYFLT         *iMinLen;
    MYFLT         *iSkipInit;
 /* ------------------------- */
    int           initDone;
    int           nChannels;
    int           nFFTConv;
    CONVOLVE_FFT  *fftConvolve[32];
    AUXCH         auxData;
} FTCONV;

static int fft_convolve_bytes_alloc(int nChannels, int nSamples)
{
    int nBytes;

    nBytes = (nSamples << 1);                   /* inBuffer   */
    nBytes += (nSamples << 1);                  /* tmpBuf     */
    nBytes += ((nSamples << 1) * nChannels);    /* outBuffers */
    nBytes += ((nSamples << 1) * nChannels);    /* IR_Data    */
    nBytes *= (int) sizeof(MYFLT);
    nBytes += (int) (sizeof(CONVOLVE_FFT) - sizeof(MYFLT));
    nBytes = (nBytes + 15) & (~15);
    return nBytes;
}

static void fft_convolve_init(ENVIRON *csound, CONVOLVE_FFT *fp, MYFLT *ftData,
                              int nChannels, int nSamples, int skipSamples)
{
    int i, j, n, nsmps;

    fp->nSamples = nSamples;
    fp->cnt = 0;
    fp->nChannels = nChannels;
    nsmps = 0;
    fp->inBuffer = &(fp->data[nsmps]); nsmps += (nSamples << 1);
    fp->tmpBuf = &(fp->data[nsmps]); nsmps += (nSamples << 1);
    for (n = 0; n < nChannels; n++) {
      fp->outBuffers[n] = &(fp->data[nsmps]); nsmps += (nSamples << 1);
      fp->IR_Data[n] = &(fp->data[nsmps]); nsmps += (nSamples << 1);
      /* clear output buffer */
      for (i = 0; i < (nSamples << 1); i++)
        fp->outBuffers[n][i] = FL(0.0);
      /* calculate FFT of impulse response */
      j = (skipSamples * nChannels) + n;
      for (i = 0; i < nSamples; i++) {
        fp->IR_Data[n][i] = ftData[j];
        j += nChannels;
      }
      for (i = nSamples; i < (nSamples << 1); i++)
        fp->IR_Data[n][i] = FL(0.0);    /* pad to double length */
      csound->RealFFT(csound, &(fp->IR_Data[n][0]), (nSamples << 1));
    }
}

static int ftconv_init(ENVIRON *csound, FTCONV *p)
{
    FUNC    *ftp;
    int     i, n, nBytes, minFFTlen, maxFFTlen;

    /* check parameters */
    p->nChannels = (int) p->OUTOCOUNT;
    if (p->nChannels < 1 || p->nChannels > FTCONV_MAXCHN) {
      initerror(Str("ftconv: invalid number of channels"));
      return NOTOK;
    }
    /* minFFTlen: length of shortest FFT convolve unit */
    /* maxFFTlen: total length = length of longest FFT convolve unit * 2 */
    minFFTlen = (int) (*(p->iMinLen) + FL(0.5));
    if (minFFTlen < 1 || (minFFTlen & (minFFTlen - 1)) != 0) {
      initerror(Str("ftconv: invalid number of samples to skip"));
      return NOTOK;
    }
    ftp = csound->ftfind_(csound, p->iFTNum);
    if (ftp == NULL)
      return NOTOK; /* ftfind should already have printed the error message */
    maxFFTlen = (int) ftp->flen / p->nChannels;
    /* round down to the nearest power of two and divide by two to get */
    /* the length of the longest impulse response partition */
    for (i = 1; i <= maxFFTlen; i <<= 1);
    maxFFTlen = i >> 2;
    if (maxFFTlen < minFFTlen) {
      initerror(Str("ftconv: insufficient data in table for convolution"));
      return NOTOK;
    }
    p->nFFTConv = 0;
    for (i = minFFTlen; i <= maxFFTlen; i <<= 1)
      p->nFFTConv++;    /* number of parallel FFT convolve units */
    /* calculate the amount of aux space to allocate (in bytes) */
    nBytes = 0;
    n = minFFTlen;
    for (i = 0; i < p->nFFTConv; i++) {
      nBytes += fft_convolve_bytes_alloc(p->nChannels, n);
      n <<= 1;
    }
    if (nBytes != (int) p->auxData.size)
      csound->auxalloc_(csound, (long) nBytes, &(p->auxData));
    else if (p->initDone > 0 && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    if (csound->oparms_->msglevel & 4) {
      for (i = 0; i < (minFFTlen * p->nChannels); i++) {
        if (ftp->ftable[i] != FL(0.0)) {
          csound->Message(csound,
                          Str("ftconv: warning: skipped non-zero samples, "
                              "impulse response may be truncated\n"));
          break;
        }
      }
    }
    /* set up all FFT convolve units */
    nBytes = 0;
    n = minFFTlen;
    for (i = 0; i < p->nFFTConv; i++) {
      p->fftConvolve[i] = (CONVOLVE_FFT*) ((unsigned char*) (p->auxData.auxp)
                                           + (int) nBytes);
      nBytes += fft_convolve_bytes_alloc(p->nChannels, n);
      fft_convolve_init(csound, p->fftConvolve[i], &(ftp->ftable[0]),
                        p->nChannels, n, n);
      n <<= 1;
    }
    p->initDone = 1;

    return OK;
}

static int ftconv_perf(ENVIRON *csound, FTCONV *p)
{
    CONVOLVE_FFT  *fp;
    MYFLT         inSig, *x, *x1, *x2;
    int           i, j, n, nn, nSamples;

    if (p->initDone <= 0) {
      perferror(Str("ftconv: not initialised"));
      return NOTOK;
    }
    for (nn = 0; nn < csound->ksmps_; nn++) {
      /* get input signal */
      inSig = p->aIn[nn];
      /* clear output to zero */
      for (n = 0; n < p->nChannels; n++)
        p->aOut[n][nn] = FL(0.0);
      /* -------- FFT convolution -------- */
      for (j = 0; j < p->nFFTConv; j++) {
        fp = p->fftConvolve[j];
        nSamples = fp->nSamples;
        /* is input buffer full ? */
        if (fp->cnt >= nSamples) {
          x = &(fp->tmpBuf[0]);
          x1 = &(fp->inBuffer[0]);
          /* yes, calculate FFT */
          for (i = nSamples; i < (nSamples << 1); i++)
            x1[i] = FL(0.0);        /* pad to double length */
          csound->RealFFT(csound, x1, (nSamples << 1));
          /* for each channel: */
          for (n = 0; n < fp->nChannels; n++) {
            /* multiply complex arrays */
            x2 = &(fp->IR_Data[n][0]);
            csound->RealFFTMult(csound, x, x1, x2, (nSamples << 1),
                                csound->GetInverseRealFFTScale(csound,
                                                               nSamples << 1));
            /* inverse FFT */
            csound->InverseRealFFT(csound, x, (nSamples << 1));
            /* copy to output buffer, overlap with "tail" of previous block */
            x2 = &(fp->outBuffers[n][0]);
            for (i = 0; i < nSamples; i++) {
              x2[i] = x[i] + x2[i + nSamples];
              x2[i + nSamples] = x[i + nSamples];
            }
          }
          /* start again from beginning of buffer */
          fp->cnt = 0;
        }
        /* copy output signal on all channels */
        for (n = 0; n < fp->nChannels; n++)
          p->aOut[n][nn] += fp->outBuffers[n][fp->cnt];
        /* store input signal in buffer */
        fp->inBuffer[fp->cnt++] = inSig;
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
                                 (int) sizeof(FTCONV), 5, "mmmmmmmm", "aiio",
                                 (int (*)(void*, void*)) ftconv_init,
                                 (int (*)(void*, void*)) NULL,
                                 (int (*)(void*, void*)) ftconv_perf,
                                 (int (*)(void*, void*)) NULL));
}

