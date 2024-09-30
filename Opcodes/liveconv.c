/*
  liveconv.c:

  Copyright (C) 2017 Sigurd Saue, Oeyvind Brandtsegg

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

/* The implementation is indebted to the ftconv opcode by Istvan Varga 2005 */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <math.h>

/*
** Data structures holding the load/unload information
*/

typedef struct {
  enum { NO_LOAD, LOADING, UNLOADING } status;
  int32_t pos;
} load_t;

typedef struct {
  load_t  *begin;
  load_t  *end;
  load_t  *head;
  int32_t                     available;
} rbload_t;

static inline
void init_load(rbload_t *buffer, int32_t size)
{
    load_t* iter = buffer->begin;

    buffer->head = buffer->begin;
    buffer->end = buffer->begin + size;
    buffer->available = 1;

    for (iter = buffer->begin; iter != buffer->end; iter++) {
      iter->status = NO_LOAD;
      iter->pos = 0;
    }
}

static inline
load_t* next_load(const rbload_t *buffer, load_t* const now)
{
    load_t *temp = now + 1;
    if (UNLIKELY(temp == buffer->end)) {
      temp = buffer->begin;
    }
    return temp;
}

static inline
load_t* previous_load(const rbload_t *buffer, load_t* const now)
{
  return (now == buffer->begin) ? (buffer->end - 1) : (now - 1);
}

/*
** liveconv - data structure holding the internal state
*/

typedef struct {

  /*
  **  Input parameters given by user
  */
  OPDS    h;
  MYFLT   *aOut;          // output buffer
  MYFLT   *aIn;           // input buffer

  MYFLT   *iFTNum;        // impulse respons table
  MYFLT   *iPartLen;      // length of impulse response partitions
                          // (latency <-> CPU usage)

  MYFLT     *kUpdate;     // Control variable for updating the IR buffer
                          // (+1 is start load, -1 is start unload)
  MYFLT     *kClear;      // Clear output buffers

  /*
  ** Internal state of opcode maintained outside
  */
  int32_t     initDone;       /* flag to indicate initialization */
  int32_t     cnt;            /* buffer position, 0 to partSize - 1       */
  int32_t     nPartitions;    /* number of convolve partitions            */
  int32_t     partSize;       /* partition length in sample frames
                             (= iPartLen as integer) */
  int32_t     rbCnt;          /* ring buffer index, 0 to nPartitions - 1  */

  /* The following pointer point into the auxData buffer */
  MYFLT   *tmpBuf;        /* temporary buffer for accumulating FFTs   */
  MYFLT   *ringBuf;       /* ring buffer of FFTs of input partitions -
                             these buffers are now computed during init */
  MYFLT   *IR_Data;       /* impulse responses (scaled)       */
  MYFLT   *outBuf;        /* output buffer (size=partSize*2)  */

  rbload_t        loader; /* Bookkeeping of load/unload operations */

  void    *fwdsetup, *invsetup;
  AUXCH   auxData;        /* Aux data buffer allocated in init pass */
} liveconv_t;

/*
**  Function to multiply the FFT buffers
**    outBuf - the output of the operation (called with tmpBuf), single channel only
**    ringBuf - the partitions of the single input signal
**    IR_data - the impulse response of a particular channel
**    partSize - size of partition
**    nPartitions - number of partitions
**    ringBuf_startPos - the starting position of the ring buffer
**                       (corresponds to the start of the partition after the
**                        last filled partition)
*/
static void multiply_fft_buffers(MYFLT *outBuf, MYFLT *ringBuf, MYFLT *IR_Data,
                                 int32_t partSize, int32_t nPartitions,
                                 int32_t ringBuf_startPos)
{
    MYFLT   re, im, re1, re2, im1, im2;
    MYFLT   *rbPtr, *irPtr, *outBufPtr, *outBufEndPm2, *rbEndP;

    /* note: partSize must be at least 2 samples */
    partSize <<= 1; /* locale partsize is twice the size of the partition size */
             /* Finding the index of the last sample pair in the output buffer */
    outBufEndPm2 = (MYFLT*) outBuf + (int32_t) (partSize - 2);
                                                 /* The end of the ring buffer */
    rbEndP = (MYFLT*) ringBuf + (int32_t) (partSize * nPartitions);
    rbPtr = &(ringBuf[ringBuf_startPos]);    /* Initialize ring buffer pointer */
    irPtr = IR_Data;                        /* Initialize impulse data pointer */
    outBufPtr = outBuf;                    /* Initialize output buffer pointer */

    /* clear output buffer to zero */
    memset(outBuf, 0, sizeof(MYFLT)*partSize);

    /*
    ** Multiply FFTs for each partition and mix to output buffer
    ** Note: IRs are stored in reverse partition order
    */
    do {
      /* wrap ring buffer position */
      if (rbPtr >= rbEndP)
        rbPtr = ringBuf;
      outBufPtr = outBuf;
      *(outBufPtr++) +=
        *(rbPtr++) * *(irPtr++); /* convolve DC - real part only */
      *(outBufPtr++) +=
        *(rbPtr++) * *(irPtr++); /* convolve Nyquist - real part only */
      re1 = *(rbPtr++);
      im1 = *(rbPtr++);
      re2 = *(irPtr++);
      im2 = *(irPtr++);

      /*
      ** Status:
      ** outBuf + 2, ringBuf + 4, irBuf + 4
      ** re = buf + 2, im = buf + 3
      */

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
        /*
        ** Status:
        ** outBuf + 2 + 4n, ringBuf + 4 + 4n, irBuf + 4 + 4n
        ** re = buf + 2 + 4n, im = buf + 3 + 4n
        */
      }
      outBufPtr[0] += re;
      outBufPtr[1] += im;

    } while (--nPartitions);
}
static inline int32_t buf_bytes_alloc(int32_t partSize, int32_t nPartitions)
{
    int32_t nSmps;

    nSmps = (partSize << 1);                            /* tmpBuf     */
    nSmps += ((partSize << 1) * nPartitions);           /* ringBuf    */
    nSmps += ((partSize << 1) * nPartitions);           /* IR_Data    */
    nSmps += ((partSize << 1));                         /* outBuf */
    nSmps *= (int32_t) sizeof(MYFLT);                   /* Buffer type MYFLT */

    nSmps += (nPartitions+1) * (int32_t) sizeof(load_t);/* Load/unload structure */
    /* One load/unload pr. partitions and an extra for buffering is sufficient */

    return nSmps;
}

static void set_buf_pointers(liveconv_t *p, int32_t partSize, int32_t nPartitions)
{
    MYFLT *ptr;

    ptr = (MYFLT*) (p->auxData.auxp);
    p->tmpBuf = ptr;
    ptr += (partSize << 1);
    p->ringBuf = ptr;
    ptr += ((partSize << 1) * nPartitions);
    p->IR_Data = ptr;
    ptr += ((partSize << 1) * nPartitions);
    p->outBuf = ptr;
    ptr += (partSize << 1);

    p->loader.begin = (load_t*) ptr;
}

static int32_t liveconv_init(CSOUND *csound, liveconv_t *p)
{
    FUNC    *ftp;       // function table
    int32_t     n, nBytes;

    /* set p->partSize to the initial partition length, iPartLen */
    p->partSize = MYFLT2LRND(*(p->iPartLen));
    if (UNLIKELY(p->partSize < 4 || (p->partSize & (p->partSize - 1)) != 0)) {
      // Must be a power of 2 at least as large as 4
      return csound->InitError(csound, "%s",
                               Str("liveconv: invalid impulse response "
                                   "partition length"));
    }

    /* Find and assign the function table numbered iFTNum */
    ftp = csound->FTFind(csound, p->iFTNum);
    if (UNLIKELY(ftp == NULL))
      return NOTOK; /* ftfind should already have printed the error message */

    /* Calculate the total length  */
    n = (int32_t) ftp->flen;
    if (UNLIKELY(n <= 0)) {
      return csound->InitError(csound, "%s",
                               Str("liveconv: invalid length, or insufficient"
                                   " IR data for convolution"));
    }

    // Compute the number of partitions (total length / partition size)
    p->nPartitions = (n + (p->partSize - 1)) / p->partSize;

    /*
    ** Calculate the amount of aux space to allocate (in bytes) and
    ** allocate if necessary
    ** Function of partition size and number of partitions
    */

    nBytes = buf_bytes_alloc(p->partSize, p->nPartitions);
    if (nBytes != (int32_t) p->auxData.size)
      csound->AuxAlloc(csound, (int32) nBytes, &(p->auxData));

    /*
    ** From here on is initialization of data
    */

    /* initialize buffer pointers */
    set_buf_pointers(p, p->partSize, p->nPartitions);

    /* Initialize load bookkeeping */
    init_load(&p->loader, (p->nPartitions + 1));

    /* clear ring buffer to zero */
    n = (p->partSize << 1) * p->nPartitions;
    memset(p->ringBuf, 0, n*sizeof(MYFLT));

    /* initialize buffer indices */
    p->cnt = 0;
    p->rbCnt = 0;

    p->fwdsetup = csound->RealFFTSetup(csound, (p->partSize << 1), FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, (p->partSize << 1), FFT_INV);

    /* clear IR buffer to zero */
    memset(p->IR_Data, 0, n*sizeof(MYFLT));

    /* clear output buffers to zero */
    memset(p->outBuf, 0, (p->partSize << 1)*sizeof(MYFLT));

    /*
    ** After initialization:
    **    Buffer indexes are zero
    **    tmpBuf is filled with rubish
    **    ringBuf and outBuf are filled with zero
    **    IR_Data buffers are filled with zero
    */

    p->initDone = 1;
    return OK;
}

static int32_t liveconv_perf(CSOUND *csound, liveconv_t *p)
{
    MYFLT       *x, *rBuf;
    FUNC        *ftp;       // function table
    int32_t         i, k, n, nSamples, rBufPos, updateIR, clearBuf, nPart, cnt;

    load_t      *load_ptr;
    // uint32_t                numLoad = p->nPartitions + 1;

    uint32_t      offset = p->h.insdshead->ksmps_offset;
    uint32_t      early  = p->h.insdshead->ksmps_no_end;
    uint32_t      nn, nsmps = CS_KSMPS;

    /* Only continue if initialized */
    if (UNLIKELY(p->initDone <= 0)) goto err1;

    ftp = csound->FTFind(csound, p->iFTNum);
    nSamples = p->partSize;   /* Length of partition */
                              /* Pointer to a partition of the ring buffer */
    rBuf = &(p->ringBuf[p->rbCnt * (nSamples << 1)]);

    if (UNLIKELY(offset))
      memset(p->aOut, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aOut[nsmps], '\0', early*sizeof(MYFLT));
    }

    /* If clear flag is set: empty buffers and reset indexes */
    clearBuf = MYFLT2LRND(*(p->kClear));
    if (clearBuf) {

      /* clear ring buffer to zero */
      n = (nSamples << 1) * p->nPartitions;
      memset(p->ringBuf, 0, n*sizeof(MYFLT));

      /* initialize buffer index */
      p->cnt = 0;
      p->rbCnt = 0;

      /* clear output buffers to zero */
      memset(p->outBuf, 0, (nSamples << 1)*sizeof(MYFLT));
    }

    /*
    ** How to handle the kUpdate input:
    ** -1: Gradually clear the IR buffer
    **      0: Do nothing
    **  1: Gradually load the IR buffer
    */

    if (p->loader.available) {

      // The buffer before the head position is the temporary buffer
      load_ptr = previous_load(&p->loader, p->loader.head);
      updateIR = MYFLT2LRND(*(p->kUpdate));
      if (updateIR == 1) {
        load_ptr->status = LOADING;
        load_ptr->pos = 0;
      }
      else if (updateIR == -1) {
        load_ptr->status = UNLOADING;
        load_ptr->pos = 0;
      }
      if (load_ptr->status != NO_LOAD) {
        p->loader.available = 0;

        /* Special case: At a partition border: Make the temporary buffer
           head position */
        if (p->cnt == 0)
          p->loader.head = load_ptr;
      }
    }

    /* For each sample in the audio input buffer (length = ksmps) */
    for (nn = offset; nn < nsmps; nn++) {

      /* store input signal in buffer */
      rBuf[p->cnt] = p->aIn[nn];

      /* copy output signals from buffer (contains data from previous
         convolution pass) */
      p->aOut[nn] = p->outBuf[p->cnt];

      /* is input buffer full ? */
      if (++p->cnt < nSamples)
        continue;                   /* no, continue with next sample */

      /* Check if there are any IR partitions to load/unload */
      load_ptr = p->loader.head;
      while (load_ptr->status != NO_LOAD) {

        cnt = load_ptr->pos;
        if (load_ptr->status == LOADING) {

          nPart = cnt / nSamples + 1;
          /* IR write position, starting with the last! */
          n = (nSamples << 1) * (p->nPartitions - nPart);

          /* Iterate over IR partitions in reverse order */
          for (k = 0; k < nSamples; k++) {
            /* Fill IR_Data with scaled IR data, or zero if outside the IR buffer */
            p->IR_Data[n + k] =
              (cnt < (int32_t)ftp->flen) ? ftp->ftable[cnt] : FL(0.0);
            cnt++;
          }

          /* pad second half of IR to zero */
          for (k = nSamples; k < (nSamples << 1); k++)
            p->IR_Data[n + k] = FL(0.0);

          /* calculate FFT (replace in the same buffer) */
          csound->RealFFT(csound, p->fwdsetup, &(p->IR_Data[n]));

        }
        else if (load_ptr->status == UNLOADING) {

          nPart = cnt / nSamples + 1;
          /* IR write position, starting with the last! */
          n = (nSamples << 1) * (p->nPartitions - nPart);
          memset(p->IR_Data + n, 0, (nSamples << 1)*sizeof(MYFLT));
        }

        // Update load buffer and move to the next buffer
        load_ptr->pos += nSamples;
        if (load_ptr->pos >= p->nPartitions * nSamples) {
          load_ptr->status = NO_LOAD;
        }

        load_ptr = next_load(&p->loader, load_ptr);
      }
      p->loader.available = 1;

      // Check if there is a temporary buffer ready to get loaded with the
      // next partition
      load_ptr = previous_load(&p->loader, p->loader.head);
      if (load_ptr->status != NO_LOAD)
        p->loader.head = load_ptr;

      /* Now the partition is filled with input --> start calculate the
         convolution */
      p->cnt = 0; /* reset buffer position */

      /* pad input in ring buffer with zeros to double length */
      for (i = nSamples; i < (nSamples << 1); i++)
        rBuf[i] = FL(0.0);

      /* calculate FFT of input */
      csound->RealFFT(csound, p->fwdsetup, rBuf);

      /* update ring buffer position */
      p->rbCnt++;
      if (p->rbCnt >= p->nPartitions)
        p->rbCnt = 0;
      rBufPos = p->rbCnt * (nSamples << 1);

      /* Move to next partition in ring buffer (used in next iteration to
         store the next input sample) */
      rBuf = &(p->ringBuf[rBufPos]);

      /* multiply complex arrays --> multiplication in the frequency domain */
      multiply_fft_buffers(p->tmpBuf, p->ringBuf, p->IR_Data,
                           nSamples, p->nPartitions, rBufPos);

      /* inverse FFT */
      csound->RealFFT(csound, p->invsetup, p->tmpBuf);

      /*
      ** Copy IFFT result to output buffer
      ** The second half is left as "tail" for next iteration
      ** The first half is overlapped with "tail" of previous block
      */
      x = &(p->outBuf[0]);
      for (i = 0; i < nSamples; i++) {
        x[i] = p->tmpBuf[i] + x[i + nSamples];
        x[i + nSamples] = p->tmpBuf[i + nSamples];
      }

    }
    return OK;

 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("liveconv: not initialised"));
}

/* module interface functions */

static OENTRY liveconv_localops[] = {
  {
    "liveconv",             // name of opcode
    sizeof(liveconv_t),     // data size of state block
    TR,                   
    "a",                    // output arguments
    "aiikk",                // input arguments
    (SUBR) liveconv_init,   // init function
    (SUBR) liveconv_perf    // a-rate function
  }
};

LINKAGE_BUILTIN(liveconv_localops)
