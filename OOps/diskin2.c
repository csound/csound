/*
  diskin2.c:

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

#include "csoundCore.h"
#include "soundio.h"
#include "diskin2.h"
#include <math.h>
#include <inttypes.h>

typedef struct DISKIN_INST_ {
  CSOUND *csound;
  DISKIN2 *diskin;
  struct DISKIN_INST_ *nxt;
} DISKIN_INST;


static CS_NOINLINE void diskin2_read_buffer(CSOUND *csound,
                                            DISKIN2 *p, int32_t bufReadPos)
{
    MYFLT *tmp;
    int32_t nsmps;
    int32_t i;
    IGN(csound);
    /* swap buffer pointers */
    tmp = p->buf;
    p->buf = p->prvBuf;
    p->prvBuf = tmp;
    /* check if requested data can be found in previously used buffer */
    i = (int32_t)((int32_t) bufReadPos + (p->bufStartPos - p->prvBufStartPos));
    if ((uint32_t) i < (uint32_t) p->bufSize) {
      int32_t  tmp2;
      /* yes, only need to swap buffers and return */
      tmp2 = p->bufStartPos;
      p->bufStartPos = p->prvBufStartPos;
      p->prvBufStartPos = tmp2;
      return;
    }
    /* save buffer position */
    p->prvBufStartPos = p->bufStartPos;
    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int32_t) bufReadPos;
    p->bufStartPos &= (~((int32_t) (p->bufSize - 1)));
    i = 0;
    if (p->bufStartPos >= 0L) {
      /* number of sample frames to read */
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0L) {         /* if there is anything to read: */
        if (nsmps > (int32_t) p->bufSize)
          nsmps = (int32_t) p->bufSize;
        nsmps *= (int32_t) p->nChannels;
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        i = (int32_t)sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    memset(&p->buf[i], 0, sizeof(MYFLT)*(p->bufSize * p->nChannels-i));
    /* while (i < (p->bufSize * p->nChannels)) */
    /*   p->buf[i++] = FL(0.0); */
}

/* Mix one sample frame from input file at location 'pos' to outputs    */
/* of opcode 'p', at sample index 'n' (0 <= n < ksmps), with amplitude  */
/* scale 'scl'.                                                         */

static inline void diskin2_get_sample(CSOUND *csound,
                                      DISKIN2 *p, int32_t fPos, int32_t n,
                                      MYFLT scl)
{
    int32_t  bufPos, i;

    if (p->wrapMode) {
      if (UNLIKELY(fPos >= p->fileLength)){
        fPos -= p->fileLength;
      }
      else if (UNLIKELY(fPos < 0L)){
        fPos += p->fileLength;
      }
    }
    bufPos = (int32_t)(fPos - p->bufStartPos);
    if (UNLIKELY((uint32_t) bufPos >= (uint32_t) p->bufSize)) {
      /* not in current buffer frame, need to read file */
      diskin2_read_buffer(csound, p, bufPos);
      /* recalculate buffer position */
      bufPos = (int32_t)(fPos - p->bufStartPos);
    }

    if (p->aOut_buf == NULL){
      MYFLT **aOut = p->aOut;
      /* copy all channels from buffer */
      if (p->nChannels == 1) {
        aOut[0][n] +=  scl * p->buf[bufPos];
      }
      else if (p->nChannels == 2) {
        bufPos += bufPos;
        aOut[0][n] += scl * p->buf[bufPos];
        aOut[1][n] += scl * p->buf[bufPos + 1];
      }
      else {
        bufPos *= p->nChannels;
        i = 0;
        /* p->aOut[i++][n] += scl * p->buf[bufPos++]; */
        /* p->aOut[i++][n] += scl * p->buf[bufPos++]; */
        do {
          aOut[i++][n] += scl * p->buf[bufPos++];
        } while (i < p->nChannels);
      }
    } else{
      MYFLT *aOut = p->aOut_buf;
      int32_t chans = p->nChannels;
      /* copy all channels from buffer */
      if (chans == 1) {
        aOut[n] += scl * p->buf[bufPos];
      }
      else if (chans == 2) {
        bufPos += bufPos;
        aOut[n*2] +=  scl * p->buf[bufPos];
        aOut[n*2+1] += scl * p->buf[bufPos+1];
      }
      else {
        bufPos *= chans;//p->nChannels;
        i = 0;
        do {
          aOut[n*chans+i] += scl * p->buf[bufPos++];
        } while (++i < chans);
      }

    }
}

/* ------------- set up fast sine generator ------------- */
/* Input args:                                            */
/*   a: amplitude                                         */
/*   f: frequency (-PI - PI)                              */
/*   p: initial phase (0 - PI/2)                          */
/*   c: 2.0 * cos(f) - 2.0                                */
/* Output args:                                           */
/*  *x: first output sample                               */
/*  *v: coefficients for calculating next sample as       */
/*      shown below:                                      */
/*            v = v + c * x                               */
/*            x = x + v                                   */
/*          These values are calculated as follows:       */
/*            x = y[0]                                    */
/*            v = y[1] - (c + 1.0) * y[0]                 */
/*          where y[0], and y[1] are the first, and       */
/*          second sample of the sine wave to be          */
/*          generated, respectively.                      */
/* -------- written by Istvan Varga, Jan 28 2002 -------- */

static inline void init_sine_gen(double a, double f, double p, double c,
                                 double *x, double *v)
{
    double  y0, y1;             /* these should be doubles */

    y0 = sin(p);
    y1 = sin(p + f);
    *x = y0;
    *v = y1 - (c * y0) - y0;
    /* amp. scale */
    *x *= a; *v *= a;
}

/* calculate buffer size in sample frames */

static int32_t diskin2_calc_buffer_size(DISKIN2 *p, int32_t n_monoSamps)
{
    int32_t i, nFrames;

    /* default to 4096 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 4096;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* limit to sane range */
    if (i < p->winSize)
      i = p->winSize;
    else if (i > 1048576)
      i = 1048576;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 64;       /* will be at least 128 sample frames */
    do {
      nFrames <<= 1;
    } while (nFrames < i);

    return nFrames;
}

static const int32_t diskin2_format_table[11] = {
  0,
  SF_FORMAT_RAW | SF_FORMAT_PCM_16,
  SF_FORMAT_RAW | SF_FORMAT_PCM_S8,
  SF_FORMAT_RAW | SF_FORMAT_ALAW,
  SF_FORMAT_RAW | SF_FORMAT_ULAW,
  SF_FORMAT_RAW | SF_FORMAT_PCM_16,
  SF_FORMAT_RAW | SF_FORMAT_PCM_32,
  SF_FORMAT_RAW | SF_FORMAT_FLOAT,
  SF_FORMAT_RAW | SF_FORMAT_PCM_U8,
  SF_FORMAT_RAW | SF_FORMAT_PCM_24,
  SF_FORMAT_RAW | SF_FORMAT_DOUBLE
};

static int32_t diskin2_init_(CSOUND *csound, DISKIN2 *p, int32_t stringname);

int32_t diskin2_init(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    return diskin2_init_(csound,p,0);
}

int32_t diskin2_init_S(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    return diskin2_init_(csound,p,1);
}

/* VL 11-01-13  diskin_init - calls diskin2_init  */

int32_t diskin_init(CSOUND *csound, DISKIN2 *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    return diskin2_init_(csound,p,0);
}

int32_t diskin_init_S(CSOUND *csound, DISKIN2 *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    return diskin2_init_(csound,p,1);
}

/*
 * soundin now uses diskin2 VL 24-12-16
 */
int32_t sndinset(CSOUND *csound, DISKIN2 *p) {
    int32_t ret;
    p->SkipInit = *p->iWrapMode;
    p->iSampleFormat = p->iSkipTime;
    p->iSkipTime = p->kTranspose;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    ret = diskin2_init_(csound,p,0);
    return ret;
}

int32_t sndinset_S(CSOUND *csound, DISKIN2 *p){
    int32_t ret;
    p->SkipInit = *p->iWrapMode;
    p->iSampleFormat = p->iSkipTime;
    p->iSkipTime = p->kTranspose;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    ret = diskin2_init_(csound,p,1);
    return ret;
}

int32_t soundin(CSOUND *csound, DISKIN2 *p){
    MYFLT tmp = *p->kTranspose;
    int32_t ret;
    *p->kTranspose = 1.;
    ret = diskin2_perf(csound, p);
    *p->kTranspose = tmp;
    return ret;
}

int32_t diskin2_async_deinit(CSOUND *csound, void *p);

static int32_t diskin2_init_(CSOUND *csound, DISKIN2 *p, int32_t stringname)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int32_t     n;

    /* check number of channels */
    p->nChannels = (int32_t)(p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN)) {
      return csound->InitError(csound,
                               Str("diskin2: invalid number of channels"));
    }
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (p->SkipInit != FL(0.0))
        return OK;
      csound_fd_close(csound, &(p->fdch));
    }
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = MYFLT2LONG(csound->esr);
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = MYFLT2LONG(*p->iSampleFormat);
    if (n<0) {
      n = -n;
      if (UNLIKELY(n < 0 || n > 10))
        return csound->InitError(csound, Str("diskin2: unknown sample format"));
      sfinfo.format = diskin2_format_table[n];
    }
    /* open file */
    /* FIXME: name can overflow with very long string */
    if (stringname==0){
      if (csound->ISSTRCOD(*p->iFileCode))
        strNcpy(name,get_arg_string(csound, *p->iFileCode), 1023);
      else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

    fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound,
                               Str("diskin2: %s: failed to open file (%s)"),
                               name, Str(sf_strerror(NULL)));
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));

    /* check number of channels in file (must equal the number of outargs) */
    if (UNLIKELY(sfinfo.channels != p->nChannels)) {
      return csound->InitError(csound,
                               Str("diskin2: number of output args "
                                   "inconsistent with number of file channels"));
    }
    /* skip initialisation if requested */
    if (p->initDone && p->SkipInit != FL(0.0))
      return OK;


    /* interpolation window size: valid settings are 1 (no interpolation), */
    /* 2 (linear interpolation), 4 (cubic interpolation), and integer */
    /* multiples of 4 in the range 8 to 1024 (sinc interpolation) */
    p->winSize = MYFLT2LONG(p->WinSize);
    if (p->winSize < 1)
      p->winSize = 4;               /* use cubic interpolation by default */
    else if (p->winSize > 2) {
      /* cubic/sinc: round to nearest integer multiple of 4 */
      p->winSize = (p->winSize + 2) & (~3L);
      if ((uint32) p->winSize > 1024UL)
        p->winSize = 1024;
      /* constant for window calculation */
      p->winFact = (FL(1.0) - POWER(p->winSize * FL(0.85172), -FL(0.89624)))
        / ((MYFLT)((p->winSize * p->winSize) >> 2));
    }
    /* set file parameters from header info */
    p->fileLength = (int32_t) sfinfo.frames;
    p->warpScale = 1.0;
    if (MYFLT2LONG(csound->esr) != sfinfo.samplerate) {
      if (LIKELY(p->winSize != 1)) {
        /* will automatically convert sample rate if interpolation is enabled */
        p->warpScale = (double)sfinfo.samplerate / (double)csound->esr;
      }
      else {
        csound->Warning(csound, Str("diskin2: warning: file sample rate (%d) "
                                    "!= orchestra sr (%d)\n"),
                        sfinfo.samplerate, MYFLT2LONG(csound->esr));
      }
    }
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (UNLIKELY(p->fileLength < 1L))
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)csound->esr * p->warpScale;
    pos *= (double)POS_FRAC_SCALE;
    p->pos_frac = (int64_t)(pos >= 0.0 ? (pos + 0.5) : (pos - 0.5));
    if (p->wrapMode) {
      p->pos_frac %= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      if (UNLIKELY(p->pos_frac < (int64_t)0))
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
    }
    p->pos_frac_inc = (int64_t)0;
    p->prv_kTranspose = FL(0.0);
    p->transpose = FL(1.0);
    /* allocate and initialise buffers */
    p->bufSize = diskin2_calc_buffer_size(p, MYFLT2LONG(p->BufSize));
    n = 2 * p->bufSize * p->nChannels * (int32_t)sizeof(MYFLT);
    if (n != (int32_t)p->auxData.size)
      csound->AuxAlloc(csound, (int32_t) n, &(p->auxData));
    p->bufStartPos = p->prvBufStartPos = -((int32_t)p->bufSize);
    n = p->bufSize * p->nChannels;
    p->buf = (MYFLT*) (p->auxData.auxp);
    p->prvBuf = (MYFLT*) p->buf + (int32_t)n;

    memset(p->buf, 0, n*sizeof(MYFLT));

    // create circular buffer, on fail set mode to synchronous
    if (csound->oparms->realtime==1 && p->fforceSync==0 &&
        (p->cb = csound->CreateCircularBuffer(csound,
                                              p->bufSize*p->nChannels*2,
                                              sizeof(MYFLT))) != NULL){
      DISKIN_INST **top, *current;
#ifndef __EMSCRIPTEN__
      int32_t *start;
#endif
      // allocate buffer
      p->aOut_bufsize =  ((unsigned int)p->bufSize) < CS_KSMPS ?
        ((MYFLT)CS_KSMPS) : ((MYFLT)p->bufSize);
      n = p->aOut_bufsize*sizeof(MYFLT)*p->nChannels;
      if (n != (int32_t)p->auxData2.size)
        csound->AuxAlloc(csound, (int32_t) n, &(p->auxData2));
      p->aOut_buf = (MYFLT *) (p->auxData2.auxp);
      memset(p->aOut_buf, 0, n);
      top = (DISKIN_INST **)csound->QueryGlobalVariable(csound, "DISKIN_INST");
#ifndef __EMSCRIPTEN__
      if (top == NULL){
        csound->CreateGlobalVariable(csound, "DISKIN_INST", sizeof(DISKIN_INST *));
        top = (DISKIN_INST **) csound->QueryGlobalVariable(csound, "DISKIN_INST");
        *top = (DISKIN_INST *) csound->Calloc(csound, sizeof(DISKIN_INST));
        csound->CreateGlobalVariable(csound, "DISKIN_PTHREAD", sizeof(void**));
        csound->CreateGlobalVariable(csound,
                                     "DISKIN_THREAD_START", sizeof(int32_t));
        current = *top;
      }
      else
#endif
        {
          current = *top;
          while(current->nxt != NULL) { /* find next empty slot in chain */
            current = current->nxt;
          }
          current->nxt = (DISKIN_INST *) csound->Calloc(csound,
                                                        sizeof(DISKIN_INST));
          current = current->nxt;
        }
      current->csound = csound;
      current->diskin = p;
      current->nxt = NULL;

#ifndef __EMSCRIPTEN__
      if ( *(start = csound->QueryGlobalVariable(csound,
                                                 "DISKIN_THREAD_START")) == 0) {
        uintptr_t diskin_io_thread(void *p);
        void **thread = csound->QueryGlobalVariable(csound, "DISKIN_PTHREAD");
        *thread = csound->CreateThread(diskin_io_thread, *top);
        *start = 1;
      }
#endif
      csound->RegisterDeinitCallback(csound, p, diskin2_async_deinit);
      p->async = 1;

      /* print file information */
      if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
        csound->Message(csound, "%s '%s'\n"
                        "         %d Hz, %d %s, %"  PRId64 " %s",
                        Str("diskin2: opened (asynchronously)"),
                        csound->GetFileName(fd),
                        sfinfo.samplerate, sfinfo.channels,
                        Str("channel(s)"),
                        (int64_t)sfinfo.frames,
                        Str("sample frames\n"));
      }
    }
    else {
      p->aOut_buf = NULL;
      p->aOut_bufsize = 0;
      p->async = 0;
      /* print file information */
      if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
        csound->Message(csound, "%s '%s':\n"
                        "         %d Hz, %d %s, %" PRId64 " %s\n",
                        Str("diskin2: opened"),
                        csound->GetFileName(fd),
                        sfinfo.samplerate, sfinfo.channels,
                        Str("channel(s)"),
                        (int64_t)sfinfo.frames,
                        Str("sample frames\n"));
      }
    }

    /* done initialisation */
    p->initDone = 1;
    return OK;
}

int32_t diskin2_async_deinit(CSOUND *csound,  void *p){

    DISKIN_INST **top, *current, *prv;

    if ((top = (DISKIN_INST **)
         csound->QueryGlobalVariable(csound, "DISKIN_INST")) == NULL) return NOTOK;
    current = *top;
    prv = NULL;
    while(current->diskin != (DISKIN2 *)p) {
      prv = current;
      current = current->nxt;
    }
    if (prv == NULL) *top = current->nxt;
    else prv->nxt = current->nxt;

#ifndef __EMSCRIPTEN__
    if (*top == NULL) {
      int32_t *start; void **pt;

      start = (int32_t *) csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START");
      *start = 0;
      pt = csound->QueryGlobalVariable(csound,"DISKIN_PTHREAD");
      //csound->Message(csound, "dealloc %p %d\n", start, *start);
      csound->JoinThread(*pt);
      csound->DestroyGlobalVariable(csound, "DISKIN_PTHREAD");
      csound->DestroyGlobalVariable(csound, "DISKIN_THREAD_START");
      csound->DestroyGlobalVariable(csound, "DISKIN_INST");
    }
#endif
    csound->Free(csound, current);
    csound->DestroyCircularBuffer(csound, ((DISKIN2 *)p)->cb);

    return OK;
}

static inline void diskin2_file_pos_inc(DISKIN2 *p, int32_t *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    if (p->wrapMode) {
      if (*ndx >= p->fileLength) {
        *ndx -= p->fileLength;
        p->pos_frac -= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
      else if (*ndx < 0L) {
        *ndx += p->fileLength;
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
    }
}


int32_t diskin2_perf_synchronous(CSOUND *csound, DISKIN2 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int      nsmps = CS_KSMPS;
    int      chn, i, nn;
    double   d, frac_d, x, c, v, pidwarp_d;
    MYFLT    frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t  ndx;
    int32_t  wsized2, warp;


    if (UNLIKELY(p->fdch.fd == NULL) ) goto file_error;
    if (!p->initDone && !p->SkipInit){
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < nsmps; nn++)
        p->aOut[chn][nn] = FL(0.0);
    /* file read position */
    if (UNLIKELY(early)) nsmps -= early;
    ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
    case 1:                   /* ---- no interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        if (p->pos_frac & ((int64_t)POS_FRAC_SCALE >> 1))
          ndx++;                      /* round to nearest sample */
        diskin2_get_sample(csound, p, ndx, nn, FL(1.0));
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    case 2:                   /* ---- linear interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        a1 = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a0 = FL(1.0) - a1;
        diskin2_get_sample(csound, p, ndx, nn, a0);
        ndx++;
        diskin2_get_sample(csound, p, ndx, nn, a1);
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    case 4:                   /* ---- cubic interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        frac = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
        a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
        a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
        a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
        ndx--;                                /* sample -1 */
        diskin2_get_sample(csound, p, ndx, nn, a0);
        ndx++;                                /* sample 0 */
        diskin2_get_sample(csound, p, ndx, nn, a1);
        ndx++;                                /* sample +1 */
        diskin2_get_sample(csound, p, ndx, nn, a2);
        ndx++;                                /* sample +2 */
        diskin2_get_sample(csound, p, ndx, nn, a3);
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    default:                  /* ---- sinc interpolation ---- */
      wsized2 = p->winSize >> 1;
      nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
      if (p->pos_frac_inc > (int64_t) nn ||
          p->pos_frac_inc < (int64_t) (-nn)) {
        warp = 1;                     /* enable warp */
        onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                    ((MYFLT)nn / (MYFLT)p->pos_frac_inc)
                    : ((MYFLT)(-nn) / (MYFLT)p->pos_frac_inc));
        pidwarp_d = PI * (double)onedwarp;
        c = 2.0 * cos(pidwarp_d) - 2.0;
        /* correct window for kwarp */
        x = v = (double)wsized2; x *= x; x = 1.0 / x;
        v *= (double)onedwarp; v -= (double)((int32_t)v) + 0.5; v *= 4.0 * v;
        winFact = (MYFLT)(((double)p->winFact - x) * v + x);
      }
      else {
        warp = 0;
        onedwarp = FL(0.0);
        pidwarp_d = c = 0.0;
        winFact = p->winFact;
      }
      for (nn = offset; nn < nsmps; nn++) {
        frac_d = (double)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (1.0 / (double)POS_FRAC_SCALE);
        ndx += (int32_t)(1 - wsized2);
        d = (double)(1 - wsized2) - frac_d;
        if (warp) {                           /* ---- warp enabled ---- */
          init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
          /* samples -(window size / 2 - 1) to -1 */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
          /* sample 0 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d < 0.00003)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* sample 1 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d > 0.99997)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* samples 2 to (window size / 2) */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
        }
        else {                                /* ---- warp disabled ---- */
          /* avoid division by zero */
          if (frac_d < 0.00001 || frac_d > 0.99999) {
            ndx += (int32_t) (wsized2 - (frac_d < 0.5 ? 1 : 0));
            diskin2_get_sample(csound, p, ndx, nn, FL(1.0));
          }
          else {
            a0 = (MYFLT)(sin(PI * frac_d) / PI);
            i = wsized2;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = a1 * a1 / (MYFLT)d;
              diskin2_get_sample(csound, p, ndx, nn, a1*a0);
              d += 1.0;
              ndx++;
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = -(a1 * a1 / (MYFLT)d);
              diskin2_get_sample(csound, p, ndx, nn, a1*a0);
              d += 1.0;
              ndx++;
            } while (--i);
          }
        }
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
    }
    /* apply 0dBFS scale */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = offset; nn < nsmps; nn++)
        p->aOut[chn][nn] *= csound->e0dbfs;
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;
}

int checkspace(void *p, int writeCheck);

int32_t diskin_file_read(CSOUND *csound, DISKIN2 *p)
{
    /* nsmps is bufsize in frames */
  int32_t nsmps = checkspace(p->cb,1); // suggest in issue #1535 (no patch supplied though, but seems to work)
   //p->aOut_bufsize;// - p->h.insdshead->ksmps_offset;
    int32_t i, nn;
    int32_t chn, chans = p->nChannels;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t ndx;
    int32_t wsized2, warp;
    MYFLT   *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */
    MYFLT transpose = p->transpose;

    if (UNLIKELY(p->fdch.fd == NULL) ) goto file_error;
    if (!p->initDone && !p->SkipInit) {
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    if (transpose != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = transpose;
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < chans; chn++)
      for (nn = 0; nn < nsmps; nn++)
        aOut[chn + nn*chans] = FL(0.0);
    /* file read position */
    ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
    case 1:                   /* ---- no interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        if (p->pos_frac & ((int64_t)POS_FRAC_SCALE >> 1))
          ndx++;                      /* round to nearest sample */
        diskin2_get_sample(csound, p, ndx, nn, FL(1.0));
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    case 2:                   /* ---- linear interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        a1 = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a0 = FL(1.0) - a1;
        diskin2_get_sample(csound, p, ndx, nn, a0);
        ndx++;
        diskin2_get_sample(csound, p, ndx, nn, a1);
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    case 4:                   /* ---- cubic interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        frac = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
        a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
        a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
        a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
        ndx--;                                /* sample -1 */
        diskin2_get_sample(csound, p, ndx, nn, a0);
        ndx++;                                /* sample 0 */
        diskin2_get_sample(csound, p, ndx, nn, a1);
        ndx++;                                /* sample +1 */
        diskin2_get_sample(csound, p, ndx, nn, a2);
        ndx++;                                /* sample +2 */
        diskin2_get_sample(csound, p, ndx, nn, a3);
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
      break;
    default:                  /* ---- sinc interpolation ---- */
      wsized2 = p->winSize >> 1;
      nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
      if (p->pos_frac_inc > (int64_t) nn ||
          p->pos_frac_inc < (int64_t) (-nn)) {
        warp = 1;                     /* enable warp */
        onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                    ((MYFLT)nn / (MYFLT)p->pos_frac_inc)
                    : ((MYFLT)(-nn) / (MYFLT)p->pos_frac_inc));
        pidwarp_d = PI * (double)onedwarp;
        c = 2.0 * cos(pidwarp_d) - 2.0;
        /* correct window for kwarp */
        x = v = (double)wsized2; x *= x; x = 1.0 / x;
        v *= (double)onedwarp; v -= (double)((int32_t)v) + 0.5; v *= 4.0 * v;
        winFact = (MYFLT)(((double)p->winFact - x) * v + x);
      }
      else {
        warp = 0;
        onedwarp = FL(0.0);
        pidwarp_d = c = 0.0;
        winFact = p->winFact;
      }
      for (nn = 0; nn < nsmps; nn++) {
        frac_d = (double)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (1.0 / (double)POS_FRAC_SCALE);
        ndx += (int32_t)(1 - wsized2);
        d = (double)(1 - wsized2) - frac_d;
        if (warp) {                           /* ---- warp enabled ---- */
          init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
          /* samples -(window size / 2 - 1) to -1 */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
          /* sample 0 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d < 0.00003)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* sample 1 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d > 0.99997)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* samples 2 to (window size / 2) */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
        }
        else {                                /* ---- warp disabled ---- */
          /* avoid division by zero */
          if (frac_d < 0.00001 || frac_d > 0.99999) {
            ndx += (int32_t) (wsized2 - (frac_d < 0.5 ? 1 : 0));
            diskin2_get_sample(csound, p, ndx, nn, FL(1.0));
          }
          else {
            a0 = (MYFLT)(sin(PI * frac_d) / PI);
            i = wsized2;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = a0 * a1 * a1 / (MYFLT)d;
              diskin2_get_sample(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = -(a0 * a1 * a1 / (MYFLT)d);
              diskin2_get_sample(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
            } while (--i);
          }
        }
        /* update file position */
        diskin2_file_pos_inc(p, &ndx);
      }
    }
    {
      /* write to circular buffer */
      int32_t lc, mc=0, nc=nsmps*p->nChannels;
      int32_t *start = csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START");
      do{
        lc =  csound->WriteCircularBuffer(csound, p->cb, &aOut[mc], nc);
        nc -= lc;
        mc += lc;
      } while(nc && *start);
    }
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;
}


int32_t diskin2_perf_asynchronous(CSOUND *csound, DISKIN2 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    MYFLT samp;
    int32_t chn;
    void *cb = p->cb;
    int32_t chans = p->nChannels;
    p->transpose =  *p->kTranspose;

    if (offset || early) {
      for (chn = 0; chn < chans; chn++)
        for (nn = 0; nn < nsmps; nn++)
          p->aOut[chn][nn] = FL(0.0);
      if (UNLIKELY(early)) nsmps -= early;
    }

    if (UNLIKELY(p->fdch.fd == NULL)) return NOTOK;
    if (!p->initDone && !p->SkipInit){
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    for (nn = offset; nn < nsmps; nn++){

      for (chn = 0; chn < chans; chn++) {
        //int32_t i =0;
        //do {
        // i =
        csound->ReadCircularBuffer(csound, cb, &samp, 1);
        //} while(i==0);
        p->aOut[chn][nn] = csound->e0dbfs*samp;
      }
    }
    return OK;
}


uintptr_t diskin_io_thread(void *p){
    DISKIN_INST *current = (DISKIN_INST *) p;
    int32_t wakeup = 1000*current->csound->ksmps/current->csound->esr;
    int32_t *start =
      current->csound->QueryGlobalVariable(current->csound,"DISKIN_THREAD_START");
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    while(*start){
      current = (DISKIN_INST *) p;
      csoundSleep(wakeup > 0 ? wakeup : 1);
      while(current != NULL){
        diskin_file_read(current->csound, current->diskin);
        current = current->nxt;
      }
    }
    return 0;
}


int32_t diskin2_perf(CSOUND *csound, DISKIN2 *p) {
    if (!p->async) return diskin2_perf_synchronous(csound, p);
    else return diskin2_perf_asynchronous(csound, p);
}



static int32_t soundout_deinit(CSOUND *csound, void *pp)
{
    char    *opname = csound->GetOpcodeName(pp);
    SNDCOM  *q;

    if (strcmp(opname, "soundouts") == 0)
      q = &(((SNDOUTS*) pp)->c);
    else
      q = &(((SNDOUT*) pp)->c);

    if (q->fd != NULL) {
      /* flush buffer */
      MYFLT *p0 = (MYFLT*) &(q->outbuf[0]);
      MYFLT *p1 = (MYFLT*) q->outbufp;
      if (p1 > p0) {
        sf_write_MYFLT(q->sf, p0, (sf_count_t) ((MYFLT*) p1 - (MYFLT*) p0));
        q->outbufp = (MYFLT*) &(q->outbuf[0]);
      }
      /* close file */
      csound->FileClose(csound, q->fd);
      q->sf = (SNDFILE*) NULL;
      q->fd = NULL;
    }

    return OK;
}

/* RWD:DBFS: NB: thse funcs all supposed to write to a 'raw' file, so
   what will people want for 0dbfs handling? really need to update
   opcode with more options. */

/* init routine for instr soundout  */

static int32_t sndo1set_(CSOUND *csound, void *pp, int32_t stringname)
{
    char    *sfname, *opname, name[1024];
    SNDCOM  *q;
    MYFLT   *ifilcod, *iformat;
    int32_t filetyp = TYP_RAW, format = csound->oparms_.outformat, nchns = 1;
    SF_INFO sfinfo;
    //SNDOUTS *p = (SNDOUTS*) pp;

    opname = csound->GetOpcodeName(pp);
    csound->Warning(csound, Str("%s is deprecated; use fout instead\n"),
                    opname);
    if (strcmp(opname, "soundouts") == 0 || strcmp(opname, "soundouts.i") == 0) {
      q = &(((SNDOUTS*) pp)->c);
      ifilcod = ((SNDOUTS*) pp)->ifilcod;
      iformat = ((SNDOUTS*) pp)->iformat;
      nchns++;
    }
    else {
      q = &(((SNDOUT*) pp)->c);
      ifilcod = ((SNDOUT*) pp)->ifilcod;
      iformat = ((SNDOUT*) pp)->iformat;
    }

    if (q->fd != NULL)                  /* if file already open, */
      return OK;                        /* return now            */

    csound->RegisterDeinitCallback(csound, pp, soundout_deinit);

    if (stringname==0){
      if (csound->ISSTRCOD(*ifilcod))
        strNcpy(name,get_arg_string(csound, *ifilcod), 1023);
      else csound->strarg2name(csound, name, ifilcod, "soundout.",0);
    }
    else strNcpy(name, ((STRINGDAT *)ifilcod)->data, 1023);

    sfname = name;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    //sfinfo.frames = 0;
    sfinfo.samplerate = MYFLT2LONG(csound->esr);
    sfinfo.channels = nchns;
    switch (MYFLT2LONG(*iformat)) {
    case 1: format = AE_CHAR; break;
    case 4: format = AE_SHORT; break;
    case 5: format = AE_LONG; break;
    case 6: format = AE_FLOAT;
    case 0: break;
    default:
      return csound->InitError(csound, Str("%s: invalid sample format: %d"),
                               opname, MYFLT2LONG(*iformat));
    }
    sfinfo.format = TYPE2SF(filetyp) | FORMAT2SF(format);
    if (q->fd == NULL) {
      return csound->InitError(csound, Str("%s cannot open %s"), opname, sfname);
    }
    sfname = csound->GetFileName(q->fd);
    if (format != AE_FLOAT)
      sf_command(q->sf, SFC_SET_CLIPPING, NULL, SF_TRUE);
    else
      sf_command(q->sf, SFC_SET_CLIPPING, NULL, SF_FALSE);
#ifdef USE_DOUBLE
    sf_command(q->sf, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
    sf_command(q->sf, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    csound->Warning(csound, Str("%s: opening RAW outfile %s\n"),
                    opname, sfname);
    q->outbufp = q->outbuf;                 /* fix - isro 20-11-96 */
    q->bufend = q->outbuf + SNDOUTSMPS;     /* fix - isro 20-11-96 */

    return OK;
}

int32_t sndoutset(CSOUND *csound, SNDOUT *p){
    return sndo1set_(csound,p,0);
}

int32_t sndoutset_S(CSOUND *csound, SNDOUT *p){
    return sndo1set_(csound,p,1);
}


int32_t soundout(CSOUND *csound, SNDOUT *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;

    if (UNLIKELY(p->c.sf == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("soundout: not initialised"));
    if (UNLIKELY(early)) nsmps -= early;
    for (nn = offset; nn < nsmps; nn++) {
      if (UNLIKELY(p->c.outbufp >= p->c.bufend)) {

        sf_write_MYFLT(p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
        p->c.outbufp = p->c.outbuf;
      }
      *(p->c.outbufp++) = p->asig[nn];
    }

    return OK;
}

int32_t soundouts(CSOUND *csound, SNDOUTS *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;

    if (UNLIKELY(p->c.sf == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("soundouts: not initialised"));
    if (UNLIKELY(early)) nsmps -= early;
    for (nn = offset; nn < nsmps; nn++) {
      if (UNLIKELY(p->c.outbufp >= p->c.bufend)) {
        sf_write_MYFLT(p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
        p->c.outbufp = p->c.outbuf;
      }
      *(p->c.outbufp++) = p->asig1[nn];
      *(p->c.outbufp++) = p->asig2[nn];
    }

    return OK;
}

static CS_NOINLINE void diskin2_read_buffer_array(CSOUND *csound,
                                                  DISKIN2_ARRAY *p,
                                                  int32_t bufReadPos)
{
    MYFLT   *tmp;
    int32_t nsmps;
    int32_t i;
    IGN(csound);
    /* swap buffer pointers */
    tmp = p->buf;
    p->buf = p->prvBuf;
    p->prvBuf = tmp;
    /* check if requested data can be found in previously used buffer */
    i = (int32_t)((int32_t) bufReadPos + (p->bufStartPos - p->prvBufStartPos));
    if ((uint32_t) i < (uint32_t) p->bufSize) {
      int32_t  tmp2;
      /* yes, only need to swap buffers and return */
      tmp2 = p->bufStartPos;
      p->bufStartPos = p->prvBufStartPos;
      p->prvBufStartPos = tmp2;
      return;
    }
    /* save buffer position */
    p->prvBufStartPos = p->bufStartPos;
    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int32_t) bufReadPos;
    p->bufStartPos &= (~((int32_t) (p->bufSize - 1)));
    i = 0;
    if (p->bufStartPos >= 0L) {
      /* number of sample frames to read */
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0L) {         /* if there is anything to read: */
        if (nsmps > (int32_t) p->bufSize)
          nsmps = (int32_t) p->bufSize;
        nsmps *= (int32_t) p->nChannels;
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        i = (int32_t)sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    memset(&p->buf[i], 0, sizeof(MYFLT)*(p->bufSize * p->nChannels-i));
    /* while (i < (p->bufSize * p->nChannels)) */
    /*   p->buf[i++] = FL(0.0); */
}


static int32_t diskin2_calc_buffer_size_array(DISKIN2_ARRAY *p, int32_t n_monoSamps)
{
    int32_t i, nFrames;

    /* default to 4096 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 4096;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* limit to sane range */
    if (i < p->winSize)
      i = p->winSize;
    else if (i > 1048576)
      i = 1048576;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 64;       /* will be at least 128 sample frames */
    do {
      nFrames <<= 1;
    } while (nFrames < i);

    return nFrames;
}

static inline void diskin2_file_pos_inc_array(DISKIN2_ARRAY *p, int32_t *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    if (p->wrapMode) {
      if (*ndx >= p->fileLength) {
        *ndx -= p->fileLength;
        p->pos_frac -= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
      else if (*ndx < 0L) {
        *ndx += p->fileLength;
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
    }
}

static inline void diskin2_get_sample_array(CSOUND *csound,
                                            DISKIN2_ARRAY *p, int32_t fPos,
                                            int32_t n, MYFLT scl)
{
    int32_t  bufPos, i;
    int32_t ksmps = CS_KSMPS;
    MYFLT *aOut = (MYFLT *) p->aOut->data;

    if (p->wrapMode) {
      if (UNLIKELY(fPos >= p->fileLength)){
        fPos -= p->fileLength;
      }
      else if (UNLIKELY(fPos < 0L)){
        fPos += p->fileLength;
      }
    }
    bufPos = (int32_t)(fPos - p->bufStartPos);
    if (UNLIKELY((uint32_t) bufPos >= (uint32_t) p->bufSize)) {
      /* not in current buffer frame, need to read file */
      diskin2_read_buffer_array(csound, p, bufPos);
      /* recalculate buffer position */
      bufPos = (int32_t)(fPos - p->bufStartPos);
    }

    /* copy all channels from buffer */
    if (p->aOut_buf == NULL){
      if (p->nChannels == 1) {
        aOut[n] +=  scl * p->buf[bufPos];
      }
      else if (p->nChannels == 2) {
        bufPos += bufPos;
        aOut[n] += scl * p->buf[bufPos];
        aOut[n+ksmps] += scl * p->buf[bufPos + 1];
      }
      else {
        bufPos *= p->nChannels;
        i = 0;
        do {
          aOut[i*ksmps+n] += scl * p->buf[bufPos++];
        } while (++i < p->nChannels);
      }
    } else{
      MYFLT *aOut = p->aOut_buf;
      int32_t chans = p->nChannels;
      /* copy all channels from buffer */
      if (chans == 1) {
        aOut[n] += scl * p->buf[bufPos];
      }
      else if (chans == 2) {
        bufPos += bufPos;
        aOut[n*2] +=  scl * p->buf[bufPos];
        aOut[n*2+1] += scl * p->buf[bufPos+1];
      }
      else {
        bufPos *= chans;//p->nChannels;
        i = 0;
        do {
          aOut[n*chans+i] += scl * p->buf[bufPos++];
        } while (++i < chans);
      }

    }
}

int32_t diskin2_async_deinit_array(CSOUND *csound,  void *p){

    DISKIN_INST **top, *current, *prv;

    if ((top = (DISKIN_INST **)
         csound->QueryGlobalVariable(csound, "DISKIN_INST_ARRAY")) == NULL)
      return NOTOK;
    current = *top;
    prv = NULL;
    while(current->diskin != (DISKIN2 *)p) {
      prv = current;
      current = current->nxt;
    }
    if (prv == NULL) *top = current->nxt;
    else prv->nxt = current->nxt;

#ifndef __EMSCRIPTEN__
    if (*top == NULL) {
      int32_t *start; void **pt;
      start = (int32_t *) csound->QueryGlobalVariable(csound,
                                                  "DISKIN_THREAD_START_ARRAY");
      *start = 0;
      pt = csound->QueryGlobalVariable(csound,"DISKIN_PTHREAD_ARRAY");
      //csound->Message(csound, "dealloc %p %d\n", start, *start);
      csound->JoinThread(*pt);
      csound->DestroyGlobalVariable(csound, "DISKIN_PTHREAD_ARRAY");
      csound->DestroyGlobalVariable(csound, "DISKIN_THREAD_START_ARRAY");
      csound->DestroyGlobalVariable(csound, "DISKIN_INST_ARRAY");
    }
#endif

    csound->Free(csound, current);
    csound->DestroyCircularBuffer(csound, ((DISKIN2_ARRAY *)p)->cb);

    return OK;
}


int32_t diskin_file_read_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    /* nsmps is bufsize in frames */
  int32_t nsmps = p->aOut_bufsize;// - p->h.insdshead->ksmps_offset;
    int32_t i, nn;
    int32_t chn, chans = p->nChannels;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t   ndx;
    int32_t     wsized2, warp;
    MYFLT  *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */

    if (UNLIKELY(p->fdch.fd == NULL) ) goto file_error;
    if (!p->initDone && !p->SkipInit) {
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < chans; chn++)
      for (nn = 0; nn < nsmps; nn++)
        aOut[chn + nn*chans] = FL(0.0);
    /* file read position */
    ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
    case 1:                   /* ---- no interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        if (p->pos_frac & ((int64_t)POS_FRAC_SCALE >> 1))
          ndx++;                      /* round to nearest sample */
        diskin2_get_sample_array(csound, p, ndx, nn, FL(1.0));
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    case 2:                   /* ---- linear interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        a1 = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a0 = FL(1.0) - a1;
        diskin2_get_sample_array(csound, p, ndx, nn, a0);
        ndx++;
        diskin2_get_sample_array(csound, p, ndx, nn, a1);
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    case 4:                   /* ---- cubic interpolation ---- */
      for (nn = 0; nn < nsmps; nn++) {
        frac = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
        a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
        a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
        a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
        ndx--;                                /* sample -1 */
        diskin2_get_sample_array(csound, p, ndx, nn, a0);
        ndx++;                                /* sample 0 */
        diskin2_get_sample_array(csound, p, ndx, nn, a1);
        ndx++;                                /* sample +1 */
        diskin2_get_sample_array(csound, p, ndx, nn, a2);
        ndx++;                                /* sample +2 */
        diskin2_get_sample_array(csound, p, ndx, nn, a3);
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    default:                  /* ---- sinc interpolation ---- */
      wsized2 = p->winSize >> 1;
      nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
      if (p->pos_frac_inc > (int64_t) nn ||
          p->pos_frac_inc < (int64_t) (-nn)) {
        warp = 1;                     /* enable warp */
        onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                    ((MYFLT)nn / (MYFLT)p->pos_frac_inc)
                    : ((MYFLT)(-nn) / (MYFLT)p->pos_frac_inc));
        pidwarp_d = PI * (double)onedwarp;
        c = 2.0 * cos(pidwarp_d) - 2.0;
        /* correct window for kwarp */
        x = v = (double)wsized2; x *= x; x = 1.0 / x;
        v *= (double)onedwarp; v -= (double)((int32_t)v) + 0.5; v *= 4.0 * v;
        winFact = (MYFLT)(((double)p->winFact - x) * v + x);
      }
      else {
        warp = 0;
        onedwarp = FL(0.0);
        pidwarp_d = c = 0.0;
        winFact = p->winFact;
      }
      for (nn = 0; nn < nsmps; nn++) {
        frac_d = (double)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (1.0 / (double)POS_FRAC_SCALE);
        ndx += (int32_t)(1 - wsized2);
        d = (double)(1 - wsized2) - frac_d;
        if (warp) {                           /* ---- warp enabled ---- */
          init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
          /* samples -(window size / 2 - 1) to -1 */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample_array(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
          /* sample 0 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d < 0.00003)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample_array(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* sample 1 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d > 0.99997)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample_array(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* samples 2 to (window size / 2) */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample_array(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
        }
        else {                                /* ---- warp disabled ---- */
          /* avoid division by zero */
          if (frac_d < 0.00001 || frac_d > 0.99999) {
            ndx += (int32_t) (wsized2 - (frac_d < 0.5 ? 1 : 0));
            diskin2_get_sample_array(csound, p, ndx, nn, FL(1.0));
          }
          else {
            a0 = (MYFLT)(sin(PI * frac_d) / PI);
            i = wsized2;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = a0 * a1 * a1 / (MYFLT)d;
              diskin2_get_sample_array(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = -(a0 * a1 * a1 / (MYFLT)d);
              diskin2_get_sample_array(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
            } while (--i);
          }
        }
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
    }
    {
      /* write to circular buffer */
      int32_t lc, mc=0, nc=nsmps*p->nChannels;
      int32_t *start = csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START");
      do{
        lc = csound->WriteCircularBuffer(csound, p->cb, &aOut[mc], nc);
        nc -= lc;
        mc += lc;
      } while(nc && *start);
    }
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;
}

uintptr_t diskin_io_thread_array(void *p){
    DISKIN_INST *current = (DISKIN_INST *) p;
    int32_t wakeup = 1000*current->csound->ksmps/current->csound->esr;
    int32_t *start =
      current->csound->QueryGlobalVariable(current->csound,
                                           "DISKIN_THREAD_START_ARRAY");
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    while(*start){
      current = (DISKIN_INST *) p;
      csoundSleep(wakeup > 0 ? wakeup : 1);
      while(current != NULL){
        diskin_file_read_array(current->csound, (DISKIN2_ARRAY *)current->diskin);
        current = current->nxt;
      }
    }
    return 0;
}


static int32_t diskin2_init_array(CSOUND *csound, DISKIN2_ARRAY *p,
                                  int32_t stringname)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int32_t     n;
    ARRAYDAT *t = p->aOut;

    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (p->SkipInit != FL(0.0))
        return OK;
      csound_fd_close(csound, &(p->fdch));
    }
    // to handle raw files number of channels
    if (t->data) p->nChannels = t->sizes[0];
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = MYFLT2LONG(csound->esr);
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = MYFLT2LONG(*p->iSampleFormat);
    if (n<0) {
      n = -n;
      if (UNLIKELY(n > 10))
        return csound->InitError(csound, Str("diskin2: unknown sample format"));
      sfinfo.format = diskin2_format_table[n];
    }
    /* open file */
    /* FIXME: name can overflow with very long string */
    if (stringname==0){
      if (csound->ISSTRCOD(*p->iFileCode))
        strNcpy(name,get_arg_string(csound, *p->iFileCode), 1023);
      else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

    fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound,
                               Str("diskin2: %s: failed to open file: %s"),
                               name, Str(sf_strerror(NULL)));
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));

    /* get number of channels in file */
    p->nChannels = sfinfo.channels;

    if (UNLIKELY(t->data == NULL) || t->sizes[0] < p->nChannels ) {
      /* create array */
      CS_VARIABLE* var;
      int32_t memSize;
      if (t->data) {
        csound->Free(csound, t->data);
        csound->Free(csound, t->sizes);
      }
      t->dimensions = 1;
      t->sizes = csound->Calloc(csound, sizeof(int32_t));
      t->sizes[0] = p->nChannels;
      var  = t->arrayType->createVariable(csound, NULL);
      t->arrayMemberSize = var->memBlockSize;
      memSize = var->memBlockSize*(t->sizes[0]);
      t->data = csound->Calloc(csound, memSize);
    }
    /* else { */
    /*   /\* check dim 1 to see if it matches  channels*\/ */
    /*   if (t->sizes[0] < p->nChannels) */
    /*      return csound->InitError(csound, */
    /*                               Str("diskin2: output array too small")); */
    /* } */

    /* skip initialisation if requested */
    if (p->initDone && (p->SkipInit) != FL(0.0))
      return OK;

    /* interpolation window size: valid settings are 1 (no interpolation), */
    /* 2 (linear interpolation), 4 (cubic interpolation), and integer */
    /* multiples of 4 in the range 8 to 1024 (sinc interpolation) */
    p->winSize = MYFLT2LONG(p->WinSize);
    if (p->winSize < 1)
      p->winSize = 4;               /* use cubic interpolation by default */
    else if (p->winSize > 2) {
      /* cubic/sinc: round to nearest integer multiple of 4 */
      p->winSize = (p->winSize + 2) & (~3L);
      if ((uint32) p->winSize > 1024UL)
        p->winSize = 1024;
      /* constant for window calculation */
      p->winFact = (FL(1.0) - POWER(p->winSize * FL(0.85172), -FL(0.89624)))
        / ((MYFLT)((p->winSize * p->winSize) >> 2));
    }
    /* set file parameters from header info */
    p->fileLength = (int32_t) sfinfo.frames;
    p->warpScale = 1.0;
    if (MYFLT2LONG(csound->esr) != sfinfo.samplerate) {
      if (LIKELY(p->winSize != 1)) {
        /* will automatically convert sample rate if interpolation is enabled */
        p->warpScale = (double)sfinfo.samplerate / (double)csound->esr;
      }
      else {
        csound->Warning(csound, Str("diskin2: warning: file sample rate (%d) "
                                    "!= orchestra sr (%d)\n"),
                        sfinfo.samplerate, MYFLT2LONG(csound->esr));
      }
    }
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (UNLIKELY(p->fileLength < 1L))
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)csound->esr * p->warpScale;
    pos *= (double)POS_FRAC_SCALE;
    p->pos_frac = (int64_t)(pos >= 0.0 ? (pos + 0.5) : (pos - 0.5));
    if (p->wrapMode) {
      p->pos_frac %= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      if (UNLIKELY(p->pos_frac < (int64_t)0))
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
    }
    p->pos_frac_inc = (int64_t)0;
    p->prv_kTranspose = FL(0.0);
    /* allocate and initialise buffers */
    p->bufSize = diskin2_calc_buffer_size_array(p, MYFLT2LONG(p->BufSize));
    n = 2 * p->bufSize * p->nChannels * (int32_t)sizeof(MYFLT);
    if (n != (int32_t)p->auxData.size)
      csound->AuxAlloc(csound, (int32_t) n, &(p->auxData));
    p->bufStartPos = p->prvBufStartPos = -((int32_t)p->bufSize);
    n = p->bufSize * p->nChannels;
    p->buf = (MYFLT*) (p->auxData.auxp);
    p->prvBuf = (MYFLT*) p->buf + (int32_t)n;

    memset(p->buf, 0, n*sizeof(MYFLT));

    // create circular buffer, on fail set mode to synchronous
    if (csound->oparms->realtime==1 && p->fforceSync==0 &&
        (p->cb = csound->CreateCircularBuffer(csound,
                                              p->bufSize*p->nChannels*2,
                                              sizeof(MYFLT))) != NULL){
      DISKIN_INST **top, *current;
#ifndef __EMSCRIPTEN__
      int32_t *start;
#endif
      // allocate buffer
      p->aOut_bufsize =
        ((unsigned int)p->bufSize) < CS_KSMPS ?
        ((MYFLT)CS_KSMPS) : ((MYFLT)p->bufSize);
      n = p->aOut_bufsize*sizeof(MYFLT)*p->nChannels;
      if (n != (int32_t)p->auxData2.size)
        csound->AuxAlloc(csound, (int32_t) n, &(p->auxData2));
      p->aOut_buf = (MYFLT *) (p->auxData2.auxp);
      memset(p->aOut_buf, 0, n);
      top =
        (DISKIN_INST **)csound->QueryGlobalVariable(csound, "DISKIN_INST_ARRAY");
#ifndef __EMSCRIPTEN__
      if (top == NULL){
        csound->CreateGlobalVariable(csound,
                                     "DISKIN_INST_ARRAY", sizeof(DISKIN_INST *));
        top = (DISKIN_INST **) csound->QueryGlobalVariable(csound,
                                                           "DISKIN_INST_ARRAY");
        *top = (DISKIN_INST *) csound->Calloc(csound, sizeof(DISKIN_INST));
        csound->CreateGlobalVariable(csound,
                                     "DISKIN_PTHREAD_ARRAY", sizeof(void**));
        csound->CreateGlobalVariable(csound,
                                     "DISKIN_THREAD_START_ARRAY", sizeof(int32_t));
        current = *top;
      }
      else
#endif
        {
          current = *top;
          while(current->nxt != NULL) { /* find next empty slot in chain */
            current = current->nxt;
          }
          current->nxt =
            (DISKIN_INST *) csound->Calloc(csound, sizeof(DISKIN_INST));
          current = current->nxt;
        }
      current->csound = csound;
      current->diskin =  (DISKIN2 *) p;
      current->nxt = NULL;

#ifndef __EMSCRIPTEN__
      if (*(start =
            csound->QueryGlobalVariable(csound,
                                        "DISKIN_THREAD_START_ARRAY")) == 0) {
        uintptr_t diskin_io_thread_array(void *p);
        // TOFIX: this variable (thread) is not referenced
        #if 0
        void **thread = csound->QueryGlobalVariable(csound,
                                                       "DISKIN_PTHREAD_ARRAY");
        #endif
        *start = 1;
        csound->CreateThread(diskin_io_thread_array, *top);
      }
#endif
      csound->RegisterDeinitCallback(csound, (DISKIN2 *) p,
                                     diskin2_async_deinit_array);
      p->async = 1;

      /* print file information */
      if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
        csound->Message(csound, "%s '%s':\n"
                                "         %d Hz, %d %schannel(s), %" PRId64 " %s",
                        Str("diskin2: opened (asynchronously)"),
                        csound->GetFileName(fd),
                        sfinfo.samplerate, sfinfo.channels,
                        Str("channel(s)"),
                        (int64_t)sfinfo.frames,
                        Str("sample frames\n"));
      }
    }
    else {
      p->aOut_buf = NULL;
      p->aOut_bufsize = 0;
      p->async = 0;
      /* print file information */
      if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
        csound->Message(csound, "%s '%s':\n"
                        "         %d Hz, %d %s, %"  PRId64 " %s",
                        Str("diskin2: opened"),
                        csound->GetFileName(fd),
                        sfinfo.samplerate, sfinfo.channels,
                        Str("channel(s)"),
                        (int64_t)sfinfo.frames,
                        Str("sample frames\n"));
      }
    }

    /* done initialisation */
    p->initDone = 1;
    return OK;
}



int32_t diskin2_perf_synchronous_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t nsmps = CS_KSMPS, ksmps = CS_KSMPS;
    int32_t chn, i, nn;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t   ndx;
    int32_t     wsized2, warp;
    MYFLT *aOut = (MYFLT *) p->aOut->data;


    if (UNLIKELY(p->fdch.fd == NULL) ) goto file_error;
    if (!p->initDone && !p->SkipInit){
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < nsmps; nn++)
        aOut[chn*ksmps+nn] = FL(0.0);
    /* file read position */
    if (UNLIKELY(early)) nsmps -= early;
    ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
    case 1:                   /* ---- no interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        if (p->pos_frac & ((int64_t)POS_FRAC_SCALE >> 1))
          ndx++;                      /* round to nearest sample */
        diskin2_get_sample_array(csound, p, ndx, nn, FL(1.0));
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    case 2:                   /* ---- linear interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        a1 = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a0 = FL(1.0) - a1;
        diskin2_get_sample_array(csound, p, ndx, nn, a0);
        ndx++;
        diskin2_get_sample_array(csound, p, ndx, nn, a1);
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    case 4:                   /* ---- cubic interpolation ---- */
      for (nn = offset; nn < nsmps; nn++) {
        frac = (MYFLT)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
        a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
        a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
        a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
        a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
        ndx--;                                /* sample -1 */
        diskin2_get_sample_array(csound, p, ndx, nn, a0);
        ndx++;                                /* sample 0 */
        diskin2_get_sample_array(csound, p, ndx, nn, a1);
        ndx++;                                /* sample +1 */
        diskin2_get_sample_array(csound, p, ndx, nn, a2);
        ndx++;                                /* sample +2 */
        diskin2_get_sample_array(csound, p, ndx, nn, a3);
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
      break;
    default:                  /* ---- sinc interpolation ---- */
      wsized2 = p->winSize >> 1;
      nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
      if (p->pos_frac_inc > (int64_t) nn ||
          p->pos_frac_inc < (int64_t) (-nn)) {
        warp = 1;                     /* enable warp */
        onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                    ((MYFLT)nn / (MYFLT)p->pos_frac_inc)
                    : ((MYFLT)(-nn) / (MYFLT)p->pos_frac_inc));
        pidwarp_d = PI * (double)onedwarp;
        c = 2.0 * cos(pidwarp_d) - 2.0;
        /* correct window for kwarp */
        x = v = (double)wsized2; x *= x; x = 1.0 / x;
        v *= (double)onedwarp; v -= (double)((int32_t)v) + 0.5; v *= 4.0 * v;
        winFact = (MYFLT)(((double)p->winFact - x) * v + x);
      }
      else {
        warp = 0;
        onedwarp = FL(0.0);
        pidwarp_d = c = 0.0;
        winFact = p->winFact;
      }
      for (nn = offset; nn < nsmps; nn++) {
        frac_d = (double)((int32_t)(p->pos_frac & (int64_t)POS_FRAC_MASK))
          * (1.0 / (double)POS_FRAC_SCALE);
        ndx += (int32_t)(1 - wsized2);
        d = (double)(1 - wsized2) - frac_d;
        if (warp) {                           /* ---- warp enabled ---- */
          init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
          /* samples -(window size / 2 - 1) to -1 */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample_array(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
          /* sample 0 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d < 0.00003)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample_array(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* sample 1 */
          /* avoid division by zero */
          if (UNLIKELY(frac_d > 0.99997)) {
            a1 = onedwarp;
          }
          else {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
          }
          diskin2_get_sample_array(csound, p, ndx, nn, a1);
          ndx++;
          d += 1.0; v += c * x; x += v;
          /* samples 2 to (window size / 2) */
          i = wsized2 - 1;
          do {
            a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
            a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            diskin2_get_sample_array(csound, p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
          } while (--i);
        }
        else {                                /* ---- warp disabled ---- */
          /* avoid division by zero */
          if (frac_d < 0.00001 || frac_d > 0.99999) {
            ndx += (int32_t) (wsized2 - (frac_d < 0.5 ? 1 : 0));
            diskin2_get_sample_array(csound, p, ndx, nn, FL(1.0));
          }
          else {
            a0 = (MYFLT)(sin(PI * frac_d) / PI);
            i = wsized2;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = a0 * a1 * a1 / (MYFLT)d;
              diskin2_get_sample_array(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = -(a0 * a1 * a1 / (MYFLT)d);
              diskin2_get_sample_array(csound, p, ndx, nn, a1);
              d += 1.0;
              ndx++;
            } while (--i);
          }
        }
        /* update file position */
        diskin2_file_pos_inc_array(p, &ndx);
      }
    }
    /* apply 0dBFS scale */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = offset; nn < nsmps; nn++)
        aOut[chn*ksmps+nn] *= csound->e0dbfs;
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;
}



int32_t diskin2_perf_asynchronous_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS, ksmps = CS_KSMPS;
    MYFLT samp;
    int32_t chn;
    void *cb = p->cb;
    int32_t chans = p->nChannels;
    MYFLT *aOut = (MYFLT *) p->aOut->data;

    if (offset || early) {
      for (chn = 0; chn < chans; chn++)
        for (nn = 0; nn < nsmps; nn++)
          aOut[chn*ksmps+nn] = FL(0.0);
      if (UNLIKELY(early)) nsmps -= early;
    }

    if (UNLIKELY(p->fdch.fd == NULL)) return NOTOK;
    if (!p->initDone && !p->SkipInit){
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    for (nn = offset; nn < nsmps; nn++){

      for (chn = 0; chn < chans; chn++) {
        //int32_t i =0;
        //do {
        // i =
        csound->ReadCircularBuffer(csound, cb, &samp, 1);
        //} while(i==0);
        aOut[chn*ksmps+nn] = csound->e0dbfs*samp;
      }
    }
    return OK;
}

int32_t diskin2_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    return diskin2_init_array(csound,p,0);
}

int32_t diskin2_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    return diskin2_init_array(csound,p,1);
}

/* diskin_init_array - calls diskin2_init_array  */

int32_t diskin_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    return diskin2_init_array(csound,p,0);
}

int32_t diskin_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    return diskin2_init_array(csound,p,1);
}

int32_t diskin2_perf_array(CSOUND *csound, DISKIN2_ARRAY *p) {
    if (!p->async) return diskin2_perf_synchronous_array(csound, p);
    else return diskin2_perf_asynchronous_array(csound, p);
}

#if 0 // OLD SOUNDIN code VL 24-12-2016
/* -------- soundin opcode: simplified version of diskin2 -------- */

static void soundin_read_buffer(CSOUND *csound, SOUNDIN_ *p, int32_t bufReadPos)
{
    int32_t i = 0;

    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int_least64_t) bufReadPos;
    p->bufStartPos &= (~((int_least64_t) (p->bufSize - 1)));
    if (p->bufStartPos >= (int_least64_t) 0) {
      int_least64_t lsmps;
      int32_t           nsmps;
      /* number of sample frames to read */
      lsmps = p->fileLength - p->bufStartPos;
      if (lsmps > (int_least64_t) 0) {  /* if there is anything to read: */
        nsmps = (lsmps < (int_least64_t) p->bufSize ? (int) lsmps : p->bufSize);
        /* convert sample count to mono samples and read file */
        nsmps *= (int32_t) p->nChannels;
        if (csound->oparms->realtime==0){
          sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
          i = (int32_t) sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        }
        else
          i = (int32_t) csound->ReadAsync(csound, p->fdch.fd, p->buf,
                                      (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    for ( ; i < (p->bufSize * p->nChannels); i++)
      p->buf[i] = FL(0.0);
}

/* calculate buffer size in sample frames */

static int32_t soundin_calc_buffer_size(SOUNDIN_ *p, int32_t n_monoSamps)
{
    int32_t i, nFrames;

    /* default to 2048 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 2048;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* limit to sane range */
    if (i > 1048576)
      i = 1048576;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 32;       /* will be at least 64 sample frames */
    do {
      nFrames <<= 1;
    } while (nFrames < i);

    return nFrames;
}

static int32_t sndinset_(CSOUND *csound, SOUNDIN_ *p, int32_t stringname)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int32_t     n, fmt, typ;

    /* check number of channels */
    p->nChannels = (int32_t) (p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN)) {
      return csound->InitError(csound,
                               Str("soundin: invalid number of channels"));
    }
    p->bufSize = soundin_calc_buffer_size(p, MYFLT2LONG(*p->iBufSize));
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (*(p->iSkipInit) != FL(0.0))
        return OK;
      csound_fd_close(csound, &(p->fdch));
    }
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = MYFLT2LONG(csound->esr);
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = MYFLT2LONG(*p->iSampleFormat);
    if (n == 1) {
      sfinfo.format = SF_FORMAT_RAW
        | (int32_t) FORMAT2SF(csound->oparms_.outformat);
    }
    else {
      if (n<0) {
        n = -n;
        if (UNLIKELY(n > 10))
          return csound->InitError(csound, Str("soundin: unknown sample format"));
        sfinfo.format = diskin2_format_table[n];
      }
    }
    /* open file */
    /* FIXME: name can overflow with very long string */
    if (stringname==0){
      if (csound->ISSTRCOD(*p->iFileCode))
        strNcpy(name,get_arg_string(csound, *p->iFileCode), 1023);
      else csound->strarg2name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

    if (csound->oparms->realtime==0)
      fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                             "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    else
      fd = csound->FileOpenAsync(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                                 "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO,
                                 p->bufSize*p->nChannels, 0);
    if (UNLIKELY(fd == NULL)) {
      if (csound->oparms->realtime==0)
        return csound->InitError(csound,
                               Str("soundin: %s: failed to open file: %s"),
                                 name, Str(sf_strerror(NULL)));
      else
        return csound->InitError(csound,
                                 Str("soundin: %s: failed to open file"), name);
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
    /* print file information */
    if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
      csound->Message(csound, Str("soundin: opened '%s':\n"
                                  "         %d Hz, %d channel(s), "
                                  "%ld sample frames\n"),
                      csound->GetFileName(fd),
                      (int32_t) sfinfo.samplerate, (int32_t) sfinfo.channels,
                      (int32_t) sfinfo.frames);
    }
    /* check number of channels in file (must equal the number of outargs) */
    if (UNLIKELY(sfinfo.channels != p->nChannels)) {
      return csound->InitError(csound,
                               Str("soundin: number of output args "
                                   "inconsistent with number of file channels"));
    }
    /* skip initialisation if requested */
    if (p->auxData.auxp != NULL && *(p->iSkipInit) != FL(0.0))
      return OK;
    /* set file parameters from header info */
    p->fileLength = (int_least64_t) sfinfo.frames;
    if (MYFLT2LONG(csound->esr) != sfinfo.samplerate)
      csound->Warning(csound, Str("soundin: file sample rate (%d) "
                                  "!= orchestra sr (%d)\n"),
                      sfinfo.samplerate, MYFLT2LONG(csound->esr));
    fmt = sfinfo.format & SF_FORMAT_SUBMASK;
    typ = sfinfo.format & SF_FORMAT_TYPEMASK;
    if ((fmt != SF_FORMAT_FLOAT && fmt != SF_FORMAT_DOUBLE) ||
        (typ == SF_FORMAT_WAV || typ == SF_FORMAT_W64 || typ == SF_FORMAT_AIFF))
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);    /* do not scale "raw" float files */
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)sfinfo.samplerate;
    p->read_pos = (int_least64_t)(pos + (pos >= 0.0 ? 0.5 : -0.5));
    /* allocate and initialise buffer */
    n = p->bufSize * p->nChannels;
    if (n != (int32_t) p->auxData.size)
      csound->AuxAlloc(csound, (int32_t)(n * (int32_t)sizeof(MYFLT)),
                       &(p->auxData));
    p->buf = (MYFLT*) (p->auxData.auxp);
    /* make sure that read position is not in buffer, to force read */
    if (p->read_pos < (int_least64_t) 0)
      p->bufStartPos = (int_least64_t) p->bufSize;
    else
      p->bufStartPos = -((int_least64_t) p->bufSize);
    /* done initialisation */
    if (csound->oparms->realtime) {
      csound->FSeekAsync(csound,p->fdch.fd, p->read_pos, SEEK_SET);
      // csound->Message(csound, "using async code \n");
    }
    return OK;
}


int32_t sndinset(CSOUND *csound, SOUNDIN_ *p){
    return sndinset_(csound,p,0);
}

int32_t sndinset_S(CSOUND *csound, SOUNDIN_ *p){
    return sndinset_(csound,p,1);
}


int32_t soundin(CSOUND *csound, SOUNDIN_ *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps=CS_KSMPS, bufPos;
    int32_t i;

    if (UNLIKELY(p->fdch.fd == NULL)) {
      return csound->PerfError(csound, &(p->h),
                               Str("soundin: not initialised"));
    }
    if (UNLIKELY(offset)) for (i=0; i<p->nChannels; i++)
                            memset(p->aOut[i], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (i=0; i<p->nChannels; i++)
        memset(&(p->aOut[i][nsmps]), '\0', early*sizeof(MYFLT));
    }
    for (nn = offset; nn < nsmps; nn++) {
      bufPos = (int32_t) (p->read_pos - p->bufStartPos);
      if ((uint32_t) bufPos >= (uint32_t) p->bufSize) {
        /* not in current buffer frame, need to read file */
        soundin_read_buffer(csound, p, bufPos);
        /* recalculate buffer position */
        bufPos = (int32_t) (p->read_pos - p->bufStartPos);
      }
      /* copy all channels from buffer */
      if (p->nChannels == 1) {
        p->aOut[0][nn] = p->scaleFac * (MYFLT) p->buf[bufPos];
      }
      else if (p->nChannels == 2) {
        bufPos += bufPos;
        p->aOut[0][nn] = p->scaleFac * p->buf[bufPos];
        p->aOut[1][nn] = p->scaleFac * p->buf[bufPos + 1];
      }
      else {
        bufPos *= p->nChannels;
        i = 0;
        do {
          p->aOut[i++][nn] = p->scaleFac * p->buf[bufPos++];
        } while (i < p->nChannels);
      }
      p->read_pos++;
    }
    return OK;
}
#endif
