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
#include "fft.h"
#include <math.h>

#define FTCONV_MAXCHN   8
#define DEF_DCONV_LEN   64

typedef struct {
    int     nSamples;   /* IR length (not padded) in samples                */
    int     cnt;        /* buffer position, counts from 0 to nSamples       */
    int     nChannels;  /* number of channels (1 to 8)                      */
    MYFLT   *ex;        /* pointer returned by AssignBasis(nSamples*2)      */
    MYFLT   *inBuffer;  /* input/FFT buffer (size = nSamples*2 + 2)         */
    MYFLT   *tmpBuf;    /* temporary buffer for FFT (size = nSamples*2 + 2) */
    MYFLT   *outBuffers[FTCONV_MAXCHN];
                        /* output buffer (size = nSamples*2)                */
    MYFLT   *IR_Data[FTCONV_MAXCHN];
                        /* FFT of impulse response (size = nSamples*2 + 2)  */
    MYFLT   data[1];
} CONVOLVE_FFT;

typedef struct {
    int     nSamples;   /* impulse response length in samples               */
    int     cnt;        /* buffer position, counts from 0 to nSamples-1     */
    int     nChannels;  /* number of channels (1 to 8)                      */
    MYFLT   *inBuffer;  /* circular input buffer (size = nSamples)          */
    MYFLT   *IR_Data[FTCONV_MAXCHN];    /* impulse response in reverse      */
    MYFLT   data[1];                    /*   order (size = nSamples)        */
} DCONVOLVE;

typedef struct {
    OPDS          h;
    MYFLT         *aOut[FTCONV_MAXCHN];
    MYFLT         *aIn;
    MYFLT         *iFTNum;
    MYFLT         *iDConvLen;
    MYFLT         *iSkipInit;
 /* ------------------------- */
    int           initDone;
    int           nChannels;
    int           nFFTConv;
    DCONVOLVE     *dConvolve;
    CONVOLVE_FFT  *fftConvolve[32];
    AUXCH         auxData;
} FTCONV;

static int fft_convolve_bytes_alloc(int nChannels, int nSamples)
{
    int nBytes;

    nBytes = (nSamples << 1) + 2;                   /* inBuffer   */
    nBytes += (nSamples << 1) + 2;                  /* tmpBuf     */
    nBytes += ((nSamples << 1) * nChannels);        /* outBuffers */
    nBytes += (((nSamples << 1) + 2) * nChannels);  /* IR_Data    */
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
    fp->ex = (MYFLT*) csound->AssignBasis_((complex*) NULL,
                                           (long) (nSamples << 1));
    nsmps = 0;
    fp->inBuffer = &(fp->data[nsmps]); nsmps += ((nSamples << 1) + 2);
    fp->tmpBuf = &(fp->data[nsmps]); nsmps += ((nSamples << 1) + 2);
    for (n = 0; n < nChannels; n++) {
      fp->outBuffers[n] = &(fp->data[nsmps]); nsmps += (nSamples << 1);
      fp->IR_Data[n] = &(fp->data[nsmps]); nsmps += ((nSamples << 1) + 2);
      /* clear output buffer */
      for (i = 0; i < (nSamples << 1); i++)
        fp->outBuffers[n][i] = FL(0.0);
      /* calculate FFT of impulse response */
      j = (skipSamples * nChannels) + n;
      for (i = 0; i < nSamples; i++) {
        fp->IR_Data[n][i] = ftData[j];
        j += nChannels;
      }
      for (i = nSamples; i < ((nSamples << 1) + 2); i++)
        fp->IR_Data[n][i] = FL(0.0);    /* pad to double length */
      csound->FFT2realpacked_((complex*) &(fp->IR_Data[n][0]),
                              (long) (nSamples << 1), (complex*) fp->ex);
    }
}

static int dconvolve_bytes_alloc(int nChannels, int nSamples)
{
    int nBytes;

    nBytes = nSamples;                  /* inBuffer */
    nBytes += nChannels * nSamples;     /* IR_Data  */
    nBytes *= (int) sizeof(MYFLT);
    nBytes += (int) (sizeof(DCONVOLVE) - sizeof(MYFLT));
    nBytes = (nBytes + 15) & (~15);
    return nBytes;
}

static void dconvolve_init(ENVIRON *csound, DCONVOLVE *dp, MYFLT *ftData,
                           int nChannels, int nSamples, int skipSamples)
{
    int i, j, n, nsmps;

    dp->nSamples = nSamples;
    dp->cnt = 0;
    dp->nChannels = nChannels;
    nsmps = 0;
    dp->inBuffer = &(dp->data[nsmps]); nsmps += nSamples;
    for (i = 0; i < nSamples; i++)
      dp->inBuffer[i] = FL(0.0);
    /* for each channel: */
    for (n = 0; n < nChannels; n++) {
      dp->IR_Data[n] = &(dp->data[nsmps]); nsmps += nSamples;
      /* store impulse response in reverse order */
      i = 0; j = (nSamples + skipSamples) * nChannels + n;
      do {
        j -= nChannels;
        dp->IR_Data[n][i] = ftData[j];
      } while (++i < nSamples);
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
    /* minFFTlen: length of direct convolution and shortest FFT convolve */
    /* maxFFTlen: total length = length of longest FFT convolve unit * 2 */
    minFFTlen = (int) (*(p->iDConvLen) + FL(0.5));
    if (minFFTlen <= 0)
      minFFTlen = DEF_DCONV_LEN;
    else if (minFFTlen < 8 || (minFFTlen & (minFFTlen - 1)) != 0) {
      initerror(Str("ftconv: invalid direct convolution length"));
      return NOTOK;
    }
    ftp = csound->ftfind_(csound, p->iFTNum);
    if (ftp == NULL)
      return NOTOK; /* ftfind should already have printed the error message */
    maxFFTlen = (int) ftp->flen / p->nChannels;
    if (maxFFTlen <= 0) {
      initerror(Str("ftconv: insufficient data in table for convolution"));
      return NOTOK;
    }
    for (i = 1; i <= maxFFTlen; i <<= 1);
    maxFFTlen = i >> 1; /* round down to nearest power of two */
    if (maxFFTlen < minFFTlen)
      minFFTlen = maxFFTlen;
    p->nFFTConv = 0;
    for (i = minFFTlen; i < maxFFTlen; i <<= 1)
      p->nFFTConv++;    /* number of parallel FFT convolve units */
    /* calculate the amount of aux space to allocate (in bytes) */
    nBytes = dconvolve_bytes_alloc(p->nChannels, minFFTlen);
    n = minFFTlen;
    for (i = 0; i < p->nFFTConv; i++) {
      nBytes += fft_convolve_bytes_alloc(p->nChannels, n);
      n <<= 1;
    }
    if (nBytes != (int) p->auxData.size)
      csound->auxalloc_(csound, (long) nBytes, &(p->auxData));
    else if (p->initDone > 0 && *(p->iSkipInit) != FL(0.0))
      return OK;    /* skip initialisation if requested */
    /* set up direct convolve unit */
    p->dConvolve = (DCONVOLVE*) (p->auxData.auxp);
    nBytes = dconvolve_bytes_alloc(p->nChannels, minFFTlen);
    dconvolve_init(csound, p->dConvolve, &(ftp->ftable[0]),
                   p->nChannels, minFFTlen, 0);
    /* set up all FFT convolve units */
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
    DCONVOLVE     *dp;
    CONVOLVE_FFT  *fp;
    double        y;
    MYFLT         inSig, *x, *x1, *x2;
    int           i, j, n, nn, nSamples;

    if (p->initDone <= 0) {
      perferror(Str("ftconv: not initialised"));
      return NOTOK;
    }
    for (nn = 0; nn < csound->ksmps_; nn++) {
      /* get input signal */
      inSig = p->aIn[nn];
      /* -------- direct convolution -------- */
      dp = p->dConvolve;
      nSamples = dp->nSamples;
      /* store input signal in circular buffer */
      dp->inBuffer[dp->cnt++] = inSig;
      dp->cnt &= (nSamples - 1);
      /* convolve all channels */
      for (n = 0; n < p->nChannels; n++) {
        i = nSamples - dp->cnt;
        y = 0.0;
        x1 = &(dp->inBuffer[dp->cnt]);
        x2 = &(dp->IR_Data[n][0]);
        do {
          y += (double) (*(x1++) * *(x2++));
        } while (--i);
        i = dp->cnt;
        x1 = &(dp->inBuffer[0]);
        while (i--) {
          y += (double) (*(x1++) * *(x2++));
        }
        p->aOut[n][nn] = (MYFLT) y;
      }
      /* -------- FFT convolution -------- */
      for (j = 0; j < p->nFFTConv; j++) {
        fp = p->fftConvolve[j];
        nSamples = fp->nSamples;
        /* is input buffer full ? */
        if (fp->cnt >= nSamples) {
          x = &(fp->tmpBuf[0]);
          x1 = &(fp->inBuffer[0]);
          /* yes, calculate FFT */
          for (i = nSamples; i < ((nSamples << 1) + 2); i++)
            x1[i] = FL(0.0);        /* pad to double length */
          csound->FFT2realpacked_((complex*) x1, (long) (nSamples << 1),
                                  (complex*) fp->ex);
          /* for each channel: */
          for (n = 0; n < fp->nChannels; n++) {
            /* multiply complex arrays */
            x2 = &(fp->IR_Data[n][0]);
            for (i = 0; i <= (nSamples << 1); i += 2) {
              x[i] = x1[i] * x2[i] - x1[i + 1] * x2[i + 1];
              x[i + 1] = x1[i] * x2[i + 1] + x2[i] * x1[i + 1];
            }
            /* inverse FFT */
            csound->FFT2torlpacked_((complex*) x, (long) (nSamples << 1),
                                    FL(1.0) / (MYFLT) (nSamples << 1),
                                    (complex*) fp->ex);
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
                                 (int) sizeof(FTCONV), 5, "mmmmmmmm", "aijo",
                                 (int (*)(void*, void*)) ftconv_init,
                                 (int (*)(void*, void*)) NULL,
                                 (int (*)(void*, void*)) ftconv_perf,
                                 (int (*)(void*, void*)) NULL));
}

