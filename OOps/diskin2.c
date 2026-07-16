/*
  diskin2.c:

  Copyright (C) 2005 Istvan Varga, (C) 2013 - 2026 V Lazzarini

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


#include "csoundCore.h"
#include "soundfile.h"
#include "soundio.h"
#include "diskin2.h"
#include <math.h>
#include <inttypes.h>


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
        csound->SndfileSeek(csound, p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        i = (int32_t) csound->SndfileReadSamples(csound, p->sf, p->buf, (sf_count_t) nsmps);
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
        bufPos *= chans;
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
  TYPE2SF(TYP_RAW)  | AE_SHORT,
  TYPE2SF(TYP_RAW)  | AE_CHAR,
  TYPE2SF(TYP_RAW)  | AE_ALAW,
  TYPE2SF(TYP_RAW)  | AE_ULAW,
  TYPE2SF(TYP_RAW)  | AE_SHORT,
  TYPE2SF(TYP_RAW)  | AE_LONG,
  TYPE2SF(TYP_RAW)  | AE_FLOAT,
  TYPE2SF(TYP_RAW)  | AE_UNCH,
  TYPE2SF(TYP_RAW)  | AE_24INT,
  TYPE2SF(TYP_RAW)  | AE_DOUBLE
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

static uintptr_t diskin_io_thread(void *p);
static uintptr_t diskin_io_thread_array(void *p);

typedef struct diskin2_async_entry {
  void *instance;
  void *mutex;
  spin_lock_t spinlock;
  volatile int32_t active;
  struct diskin2_async_entry *next;
} DISKIN2_ASYNC_ENTRY;

typedef struct {
  DISKIN2_ASYNC_ENTRY *entries;
  DISKIN2_ASYNC_ENTRY *entryTail;
  DISKIN2_ASYNC_ENTRY *arrayEntries;
  DISKIN2_ASYNC_ENTRY *arrayEntryTail;
  void *thread;
  void *arrayThread;
  void *mutex;
  volatile int32_t running;
  volatile int32_t arrayRunning;
} DISKIN2_ASYNC_STATE;

#ifndef __EMSCRIPTEN__

static inline DISKIN2_ASYNC_STATE *diskin2_async_state(CSOUND *csound)
{
  return (DISKIN2_ASYNC_STATE *) csound->diskin2_async_state;
}

static inline void diskin2_registry_lock(CSOUND *csound)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  if (state->mutex != NULL)
    csoundLockMutex(state->mutex);
  else
    csoundSpinLock(&csound->diskin2_async_lock);
}

static inline void diskin2_registry_unlock(CSOUND *csound)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  if (state->mutex != NULL)
    csoundUnlockMutex(state->mutex);
  else
    csoundSpinUnLock(&csound->diskin2_async_lock);
}

static inline void diskin2_entry_lock(CSOUND *csound,
                                      DISKIN2_ASYNC_ENTRY *entry)
{
  if (entry->mutex != NULL)
    csoundLockMutex(entry->mutex);
  else
    csoundSpinLock(&entry->spinlock);
}

static inline void diskin2_entry_unlock(CSOUND *csound,
                                        DISKIN2_ASYNC_ENTRY *entry)
{
  if (entry->mutex != NULL)
    csoundUnlockMutex(entry->mutex);
  else
    csoundSpinUnLock(&entry->spinlock);
}

static DISKIN2_ASYNC_ENTRY *diskin2_new_entry(CSOUND *csound, void *instance)
{
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *)
    csound->Calloc(csound, sizeof(DISKIN2_ASYNC_ENTRY));

  if (UNLIKELY(entry == NULL))
    return NULL;
#ifndef CSOUND_NO_RT_MUTEX
  entry->mutex = csoundCreateMutex(0);
#endif
  if (entry->mutex == NULL) {
#if CSOUND_SPINLOCK_AVAILABLE
    if (UNLIKELY(csoundSpinLockInit(&entry->spinlock) != CSOUND_SUCCESS)) {
      csound->Free(csound, entry);
      return NULL;
    }
#else
    csound->Free(csound, entry);
    return NULL;
#endif
  }
  entry->instance = instance;
  return entry;
}

static void diskin2_append_entry(DISKIN2_ASYNC_ENTRY **head,
                                 DISKIN2_ASYNC_ENTRY **tail,
                                 DISKIN2_ASYNC_ENTRY *entry)
{
  if (*tail == NULL)
    *head = entry;
  else
    (*tail)->next = entry;
  *tail = entry;
}

static int32_t diskin2_add_instance(CSOUND *csound, DISKIN2 *p)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *) p->asyncEntry;
  int32_t startThread;

  if (UNLIKELY(state == NULL))
    return NOTOK;

  if (entry == NULL) {
    entry = diskin2_new_entry(csound, p);
    if (UNLIKELY(entry == NULL))
      return NOTOK;
    diskin2_registry_lock(csound);
    diskin2_append_entry(&state->entries, &state->entryTail, entry);
    p->asyncEntry = entry;
    diskin2_registry_unlock(csound);
  }

  ATOMIC_SET(p->asyncStopRequested, 0);
  diskin2_entry_lock(csound, entry);
  ATOMIC_SET(entry->active, 1);
  diskin2_entry_unlock(csound, entry);

  diskin2_registry_lock(csound);
  startThread = !ATOMIC_GET(state->running);
  if (startThread)
    ATOMIC_SET(state->running, 1);
  diskin2_registry_unlock(csound);

  if (startThread) {
    void *thread = csound->CreateThread(diskin_io_thread, csound);
    if (UNLIKELY(thread == NULL)) {
      diskin2_entry_lock(csound, entry);
      ATOMIC_SET(entry->active, 0);
      diskin2_entry_unlock(csound, entry);
      ATOMIC_SET(state->running, 0);
      return NOTOK;
    }
    diskin2_registry_lock(csound);
    state->thread = thread;
    diskin2_registry_unlock(csound);
  }
  return OK;
}

static int32_t diskin2_add_array_instance(CSOUND *csound, DISKIN2_ARRAY *p)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *) p->asyncEntry;
  int32_t startThread;

  if (UNLIKELY(state == NULL))
    return NOTOK;

  if (entry == NULL) {
    entry = diskin2_new_entry(csound, p);
    if (UNLIKELY(entry == NULL))
      return NOTOK;
    diskin2_registry_lock(csound);
    diskin2_append_entry(&state->arrayEntries, &state->arrayEntryTail, entry);
    p->asyncEntry = entry;
    diskin2_registry_unlock(csound);
  }

  ATOMIC_SET(p->asyncStopRequested, 0);
  diskin2_entry_lock(csound, entry);
  ATOMIC_SET(entry->active, 1);
  diskin2_entry_unlock(csound, entry);

  diskin2_registry_lock(csound);
  startThread = !ATOMIC_GET(state->arrayRunning);
  if (startThread)
    ATOMIC_SET(state->arrayRunning, 1);
  diskin2_registry_unlock(csound);

  if (startThread) {
    void *thread = csound->CreateThread(diskin_io_thread_array, csound);
    if (UNLIKELY(thread == NULL)) {
      diskin2_entry_lock(csound, entry);
      ATOMIC_SET(entry->active, 0);
      diskin2_entry_unlock(csound, entry);
      ATOMIC_SET(state->arrayRunning, 0);
      return NOTOK;
    }
    diskin2_registry_lock(csound);
    state->arrayThread = thread;
    diskin2_registry_unlock(csound);
  }
  return OK;
}

static int32_t diskin2_remove_instance(CSOUND *csound, DISKIN2 *p)
{
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *) p->asyncEntry;

  if (UNLIKELY(entry == NULL))
    return NOTOK;

  /* Let a worker blocked on its circular buffer leave before taking the
     per-instance lock. Other diskin2 instances remain independent. */
  ATOMIC_SET(p->asyncStopRequested, 1);
  diskin2_entry_lock(csound, entry);
  if (UNLIKELY(!ATOMIC_GET(entry->active))) {
    diskin2_entry_unlock(csound, entry);
    return NOTOK;
  }
  ATOMIC_SET(entry->active, 0);
  p->async = 0;
  diskin2_entry_unlock(csound, entry);
  return OK;
}

static int32_t diskin2_remove_array_instance(CSOUND *csound,
                                              DISKIN2_ARRAY *p)
{
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *) p->asyncEntry;

  if (UNLIKELY(entry == NULL))
    return NOTOK;

  ATOMIC_SET(p->asyncStopRequested, 1);
  diskin2_entry_lock(csound, entry);
  if (UNLIKELY(!ATOMIC_GET(entry->active))) {
    diskin2_entry_unlock(csound, entry);
    return NOTOK;
  }
  ATOMIC_SET(entry->active, 0);
  p->async = 0;
  diskin2_entry_unlock(csound, entry);
  return OK;
}

static int32_t diskin2_instance_running(CSOUND *csound, DISKIN2 *p)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  return state != NULL && ATOMIC_GET(state->running) &&
         !ATOMIC_GET(p->asyncStopRequested);
}

static int32_t diskin2_array_instance_running(CSOUND *csound,
                                               DISKIN2_ARRAY *p)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  return state != NULL && ATOMIC_GET(state->arrayRunning) &&
         !ATOMIC_GET(p->asyncStopRequested);
}

static void diskin2_free_entries(CSOUND *csound, DISKIN2_ASYNC_ENTRY *entry)
{
  while (entry != NULL) {
    DISKIN2_ASYNC_ENTRY *next = entry->next;

    if (entry->mutex != NULL)
      csoundDestroyMutex(entry->mutex);
#if defined(__GNUC__) && defined(HAVE_PTHREAD_SPIN_LOCK)
    else
      pthread_spin_destroy(&entry->spinlock);
#endif
    csound->Free(csound, entry);
    entry = next;
  }
}

#endif

int32_t diskin2_async_setup(CSOUND *csound)
{
#ifndef __EMSCRIPTEN__
  DISKIN2_ASYNC_STATE *state;

  if (csound->diskin2_async_state != NULL)
    return CSOUND_SUCCESS;
  state = (DISKIN2_ASYNC_STATE *) csound->Calloc(
    csound, sizeof(DISKIN2_ASYNC_STATE));
  if (UNLIKELY(state == NULL))
    return CSOUND_MEMORY;
#ifndef CSOUND_NO_RT_MUTEX
  state->mutex = csoundCreateMutex(0);
#endif
  if (state->mutex == NULL) {
#if CSOUND_SPINLOCK_AVAILABLE
    if (UNLIKELY(csoundSpinLockInit(&csound->diskin2_async_lock) !=
                 CSOUND_SUCCESS)) {
      csound->Free(csound, state);
      return CSOUND_ERROR;
    }
#else
    csound->Free(csound, state);
    return CSOUND_ERROR;
#endif
  }
  csound->diskin2_async_state = state;
#else
  IGN(csound);
#endif
  return CSOUND_SUCCESS;
}

void diskin2_async_shutdown(CSOUND *csound)
{
#ifndef __EMSCRIPTEN__
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  void *thread, *arrayThread;

  if (state == NULL)
    return;
  ATOMIC_SET(state->running, 0);
  ATOMIC_SET(state->arrayRunning, 0);
  diskin2_registry_lock(csound);
  thread = state->thread;
  arrayThread = state->arrayThread;
  diskin2_registry_unlock(csound);

  if (thread != NULL)
    csound->JoinThread(thread);
  if (arrayThread != NULL)
    csound->JoinThread(arrayThread);
  diskin2_free_entries(csound, state->entries);
  diskin2_free_entries(csound, state->arrayEntries);
  csound->diskin2_async_state = NULL;
  if (state->mutex != NULL)
    csoundDestroyMutex(state->mutex);
#if defined(__GNUC__) && defined(HAVE_PTHREAD_SPIN_LOCK)
  else
    pthread_spin_destroy(&csound->diskin2_async_lock);
#endif
  csound->Free(csound, state);
#else
  IGN(csound);
#endif
}

static inline int32_t diskin2_async_available(CSOUND *csound)
{
#if defined(__EMSCRIPTEN__)
  IGN(csound);
  return 1;
#else
  return csound->diskin2_async_state != NULL;
#endif
}

static int32_t diskin2_init_(CSOUND *csound, DISKIN2 *p, int32_t stringname)
{
  double  pos;
  char    name[1024];
  void    *fd;
  SFLIB_INFO sfinfo;
  int32_t     n;

  /* check number of channels */
  p->oChannels = (int32_t)(p->OUTOCOUNT);
  if (UNLIKELY(p->oChannels < 1 || p->oChannels > DISKIN2_MAXCHN)) {
    return csound->InitError(csound,
                             Str("diskin2: invalid number of channels"));
  }
  /* if already open, close old file first */
  if (p->fdch.fd != NULL) {
    /* skip initialisation if requested */
    if (p->SkipInit != FL(0.0))
      return OK;
    if (p->async && UNLIKELY(diskin2_async_deinit(csound, p) != OK))
      return csound->InitError(csound, "%s",
                               Str("diskin2: could not stop async worker"));
    csoundFDClose(csound, &(p->fdch));
  }
  /* set default format parameters */
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  sfinfo.samplerate = MYFLT2LONG(CS_ESR);
  sfinfo.channels = p->oChannels;
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
    if (IsStringCode(*p->iFileCode))
      strNcpy(name,csoundGetArgString(csound, *p->iFileCode), 1023);
    else csound->StringArg2Name(csound, name, p->iFileCode, "soundin.",0);
  }
  else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

  fd = csound->FileOpen(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                        "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
  if (UNLIKELY(fd == NULL)) {
    return csound->InitError(csound,
                             Str("diskin2: %s: failed to open file (%s)"),
                             name, Str(csound->SndfileStrError(csound,NULL)));
  }
  /* record file handle so that it will be closed at note-off */
  memset(&(p->fdch), 0, sizeof(FDCH));
  p->fdch.fd = fd;
  csoundFDRecord(csound, &(p->fdch));

  /* set the number of channels from file */
  p->nChannels = sfinfo.channels;

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
  if (MYFLT2LONG(CS_ESR) != sfinfo.samplerate) {
    if (LIKELY(p->winSize != 1)) {
      /* will automatically convert sample rate if interpolation is enabled */
      p->warpScale = (double)sfinfo.samplerate / (double)CS_ESR;
    }
    else {
      csound->Warning(csound, Str("diskin2: warning: file sample rate (%d) "
                                  "!= orchestra sr (%d)\n"),
                      sfinfo.samplerate, MYFLT2LONG(CS_ESR));
    }
  }
  /* wrap mode */
  p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
  if (UNLIKELY(p->fileLength < 1L))
    p->wrapMode = 0;
  /* initialise read position */
  pos = (double)*(p->iSkipTime) * (double)CS_ESR * p->warpScale;
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
  
  if (csound->oparms->realtime == 1 && p->fforceSync == 0 &&
      diskin2_async_available(csound)) {
#ifdef __EMSCRIPTEN__
    DISKIN2 **top, *current;
#endif
    p->csound = csound;
    int32_t numelem =  p->bufSize*p->nChannels;
    
     /* circular buffer is allocated once per opcode
        instance and will be freed by csoundReset
        we also make sure size is compatible
      */
    if(p->cb == NULL ||
       csound->GetSizeCircularBuffer(csound,p->cb) < numelem) {
       p->cb = csound->CreateCircularBuffer(csound,
                                    numelem, sizeof(MYFLT));
    }

   if (UNLIKELY(p->cb == NULL)) {
     return csound->InitError(csound, "%s",
                              Str("diskin2: failed to allocate circular buffer"));
   }

    // allocate buffer
    p->aOut_bufsize =  ((unsigned int)p->bufSize) < CS_KSMPS ?
      ((MYFLT)CS_KSMPS) : ((MYFLT)p->bufSize);
    n = p->aOut_bufsize*sizeof(MYFLT)*p->nChannels;
    if (n != (int32_t)p->auxData2.size)
      csound->AuxAlloc(csound, (int32_t) n, &(p->auxData2));
    p->aOut_buf = (MYFLT *) (p->auxData2.auxp);
    memset(p->aOut_buf, 0, n);

    // allocate audio data buffer for asynchr processing
    // this is used to copy interleaved data before output
    n = CS_KSMPS*p->nChannels*sizeof(MYFLT);
    if (n != (int32_t)p->audioData.size)
       csound->AuxAlloc(csound, (int32_t) n, &(p->audioData));

#ifdef __EMSCRIPTEN__
    top = (DISKIN2 **)csound->QueryGlobalVariable(csound, "DISKIN_INST");
    if (top != NULL) {
      current = *top;
      if (current == NULL) {
        *top = p;
      }
      else {
        while (current->nxt != NULL)
          current = current->nxt;
        current->nxt = p;
      }
    }
#else
    if (UNLIKELY(diskin2_add_instance(csound, p) != OK))
      return csound->InitError(csound, "%s",
                               Str("diskin2: could not start async worker"));
#endif
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
    n = CS_KSMPS*p->nChannels*sizeof(MYFLT);

    // allocate audio data buffer for synchr processing
    // this is done to avoid using output variable memory
    if (n != (int32_t)p->audioData.size)
       csound->AuxAlloc(csound, (int32_t) n, &(p->audioData));
    for(n = 0; n < p->nChannels; n++)
      p->aOut[n] = ((MYFLT *)p->audioData.auxp + n*CS_KSMPS);

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

int32_t diskin2_async_deinit(CSOUND *csound, DISKIN2 *p){
#ifndef __EMSCRIPTEN__
  if (p->async)
    return diskin2_remove_instance(csound, p);
#elif defined(__EMSCRIPTEN__)
  if (p->async) {
    DISKIN2 **top, *current, *prv = NULL;
    if ((top = (DISKIN2 **)
         csound->QueryGlobalVariable(csound, "DISKIN_INST")) == NULL)
      return NOTOK;
    current = *top;
    while(current != NULL && current != p) {
      prv = current;
      current = current->nxt;
    }
    if (UNLIKELY(current == NULL))
      return NOTOK;
    if (prv == NULL) *top = current->nxt;
    else prv->nxt = current->nxt;
    p->async = 0;
  }
#else
  IGN(csound);
  IGN(p);
#endif
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
    int32_t      nsmps = CS_KSMPS;
    int32_t      chn, i, nn;
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
    /* clear audio data buffer to zero first */
    memset(p->audioData.auxp, 0, p->audioData.size);
    /* file read position */
    if (UNLIKELY(early)) nsmps -= early;
    ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
    case 1:    /* ---- no interpolation ---- */
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
    /* apply 0dBFS scale and copy to outputs */
    for (chn = 0; chn < p->oChannels; chn++) {
      if(chn < p->nChannels) {
      for (nn = offset; nn < nsmps; nn++)
        p->out[chn][nn] = p->aOut[chn][nn] * csound->e0dbfs;
      } else /* excess channels set to 0 */
        p->out[chn][nn] = FL(0.0);
    }
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;
}


void diskin_file_read(CSOUND *csound, DISKIN2 *p)
{
    /* nsmps is bufsize in frames */
    int32_t nsmps = csound->CheckCircularBuffer(csound, p->cb, 1)/p->nChannels;
    int32_t i, nn;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t ndx;
    int32_t wsized2, warp;
    MYFLT   *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */
    MYFLT transpose = p->transpose;

    if (UNLIKELY(p->fdch.fd == NULL) ) return;
    if (!p->initDone && !p->SkipInit) {
      csound->ErrorMsg(csound, Str("diskin2: not initialised"));
      return;
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
    memset(aOut, 0, p->auxData2.size);

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
#ifdef __EMSCRIPTEN__
      int32_t *start = csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START");
#endif
      do{
        lc =  csound->WriteCircularBuffer(csound, p->cb, &aOut[mc], nc);
        nc -= lc;
        mc += lc;
#ifdef __EMSCRIPTEN__
      } while(nc && *start);
#else
      } while(nc && diskin2_instance_running(csound, p));
#endif
    }
}


int32_t diskin2_perf_asynchronous(CSOUND *csound, DISKIN2 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, ni, nsmps = CS_KSMPS;
    MYFLT *samp = (MYFLT *) p->audioData.auxp;
    int32_t chn;
    void *cb = p->cb;

    int32_t chans = p->nChannels, ochans = p->oChannels;
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

    csound->ReadCircularBuffer(csound, cb, samp, nsmps*chans);
    for (ni = nn = offset; nn < nsmps; nn++, ni+=chans){
      for (chn = 0; chn < ochans; chn++) {
        if(chn < chans) {
         p->out[chn][nn] = csound->e0dbfs*samp[chn+ni];
        } else p->out[chn][nn] = FL(0.0);
      }
    }
    return OK;
}


static uintptr_t diskin_io_thread(void *p)
{
#ifdef __EMSCRIPTEN__
  DISKIN2 *current = (DISKIN2 *) p;
  CSOUND *csound = current->csound;
  int32_t wakeup = 1000 * current->h.insdshead->ksmps /
                   current->h.insdshead->esr;
  int32_t *start =
     csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START");
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  while (*start) {
    csoundSleep(wakeup > 0 ? wakeup : 1);
    current = *((DISKIN2 **)
                csound->QueryGlobalVariable(csound, "DISKIN_INST"));
    while (current) {
      diskin_file_read(csound, current);
      current = current->nxt;
    }
  }
#else
  CSOUND *csound = (CSOUND *) p;
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  DISKIN2_ASYNC_ENTRY *entry;
  int32_t wakeup = 1000 * csound->ksmps / csound->esr;

  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  for (;;) {
    csoundSleep(wakeup > 0 ? wakeup : 1);
    if (!ATOMIC_GET(state->running))
      break;
    diskin2_registry_lock(csound);
    entry = state->entries;
    diskin2_registry_unlock(csound);
    while (entry != NULL) {
      DISKIN2 *current = (DISKIN2 *) entry->instance;

      diskin2_entry_lock(csound, entry);
      if (ATOMIC_GET(entry->active) &&
          !ATOMIC_GET(current->asyncStopRequested))
        diskin_file_read(csound, current);
      diskin2_entry_unlock(csound, entry);
      diskin2_registry_lock(csound);
      entry = entry->next;
      diskin2_registry_unlock(csound);
    }
  }
#endif
  return 0;
}


int32_t diskin2_perf(CSOUND *csound, DISKIN2 *p) {
    if (!p->async) return diskin2_perf_synchronous(csound, p);
    else return diskin2_perf_asynchronous(csound, p);
}

static CS_NOINLINE void diskin2_read_buffer_array(CSOUND *csound,
                                                  DISKIN2_ARRAY *p,
                                                  int32_t bufReadPos) {
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
        csound->SndfileSeek(csound, p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        i = (int32_t) csound->SndfileReadSamples(csound, p->sf, p->buf, (sf_count_t) nsmps);
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
                                            int32_t n, MYFLT scl) {
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

int32_t diskin2_async_deinit_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
#ifndef __EMSCRIPTEN__
  if (p->async)
    return diskin2_remove_array_instance(csound, p);
#elif defined(__EMSCRIPTEN__)
  if (p->async) {
    DISKIN2_ARRAY  **top, *current, *prv;
    if ((top = (DISKIN2_ARRAY **)
         csound->QueryGlobalVariable(csound, "DISKIN_INST_ARRAY")) == NULL)
      return NOTOK;
    current = *top;
    prv = NULL;
    while(current != NULL && current != p) {
      prv = current;
      current = current->nxt;
    }
    if (UNLIKELY(current == NULL))
      return NOTOK;
    if (prv == NULL) *top = current->nxt;
    else prv->nxt = current->nxt;
    p->async = 0;
  }
#else
  IGN(csound);
  IGN(p);
#endif
  return OK;
}

void diskin_file_read_array(CSOUND *csound, DISKIN2_ARRAY *p) {
    /* nsmps is bufsize in frames */
    int32_t nsmps = csound->CheckCircularBuffer(csound, p->cb, 1)/p->nChannels;
    int32_t i, nn;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t   ndx;
    int32_t     wsized2, warp;
    MYFLT  *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */

    if (UNLIKELY(p->fdch.fd == NULL) ) return;
    if (!p->initDone && !p->SkipInit) {
      csound->ErrorMsg(csound, Str("diskin2: not initialised"));
      return;
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
    memset(aOut, 0, p->auxData2.size);
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
#ifdef __EMSCRIPTEN__
      int32_t *start = csound->QueryGlobalVariable(csound,"DISKIN_THREAD_START_ARRAY");
#endif
      do{
        lc = csound->WriteCircularBuffer(csound, p->cb, &aOut[mc], nc);
        nc -= lc;
        mc += lc;
#ifdef __EMSCRIPTEN__
      } while(nc && *start);
#else
      } while(nc && diskin2_array_instance_running(csound, p));
#endif
    }
}

static uintptr_t diskin_io_thread_array(void *p)
{
#ifdef __EMSCRIPTEN__
  DISKIN2_ARRAY *current = (DISKIN2_ARRAY *) p;
  CSOUND *csound = current->csound;
  int32_t wakeup = 1000 * current->h.insdshead->ksmps /
                   current->h.insdshead->esr;
  int32_t *start =
    csound->QueryGlobalVariable(csound, "DISKIN_THREAD_START_ARRAY");
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  while (*start) {
    current = *((DISKIN2_ARRAY **)
                csound->QueryGlobalVariable(csound, "DISKIN_INST_ARRAY"));
    csoundSleep(wakeup > 0 ? wakeup : 1);
    while (current != NULL) {
      diskin_file_read_array(csound, current);
      current = current->nxt;
    }
  }
#else
  CSOUND *csound = (CSOUND *) p;
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  DISKIN2_ASYNC_ENTRY *entry;
  int32_t wakeup = 1000 * csound->ksmps / csound->esr;

  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  for (;;) {
    csoundSleep(wakeup > 0 ? wakeup : 1);
    if (!ATOMIC_GET(state->arrayRunning))
      break;
    diskin2_registry_lock(csound);
    entry = state->arrayEntries;
    diskin2_registry_unlock(csound);
    while (entry != NULL) {
      DISKIN2_ARRAY *current = (DISKIN2_ARRAY *) entry->instance;

      diskin2_entry_lock(csound, entry);
      if (ATOMIC_GET(entry->active) &&
          !ATOMIC_GET(current->asyncStopRequested))
        diskin_file_read_array(csound, current);
      diskin2_entry_unlock(csound, entry);
      diskin2_registry_lock(csound);
      entry = entry->next;
      diskin2_registry_unlock(csound);
    }
  }
#endif
  return 0;
}


static int32_t diskin2_init_array(CSOUND *csound, DISKIN2_ARRAY *p,
                                  int32_t stringname){
    double  pos;
    char    name[1024];
    void    *fd;
    SFLIB_INFO sfinfo;
    int32_t     n;
    ARRAYDAT *t = p->aOut;

    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (p->SkipInit != FL(0.0))
        return OK;
      if (p->async &&
          UNLIKELY(diskin2_async_deinit_array(csound, p) != OK))
        return csound->InitError(csound, "%s",
                                 Str("diskin2: could not stop async worker"));
      csoundFDClose(csound, &(p->fdch));
    }
    // to handle raw files number of channels
    if (t->data) p->nChannels = t->sizes[0];
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    sfinfo.samplerate = MYFLT2LONG(CS_ESR);
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
      if (IsStringCode(*p->iFileCode))
        strNcpy(name,csoundGetArgString(csound, *p->iFileCode), 1023);
      else csound->StringArg2Name(csound, name, p->iFileCode, "soundin.",0);
    }
    else strNcpy(name, ((STRINGDAT *)p->iFileCode)->data, 1023);

    fd = csound->FileOpen(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound,
                               Str("diskin2: %s: failed to open file: %s"),
                               name, Str(csound->SndfileStrError(csound,NULL)));
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    csoundFDRecord(csound, &(p->fdch));

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
      var = csoundCreateVariableForType(csound, t->arrayType, NULL,
                                        p->h.insdshead);
      t->arrayMemberSize = var->memBlockSize;
      memSize = var->memBlockSize*(t->sizes[0]);
      t->data = csound->Calloc(csound, memSize);
      csound->Free(csound, var);
    }

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
    if (MYFLT2LONG(CS_ESR) != sfinfo.samplerate) {
      if (LIKELY(p->winSize != 1)) {
        /* will automatically convert sample rate if interpolation is enabled */
        p->warpScale = (double)sfinfo.samplerate / (double)CS_ESR;
      }
      else {
        csound->Warning(csound, Str("diskin2: warning: file sample rate (%d) "
                                    "!= orchestra sr (%d)\n"),
                        sfinfo.samplerate, MYFLT2LONG(CS_ESR));
      }
    }
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (UNLIKELY(p->fileLength < 1L))
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)CS_ESR * p->warpScale;
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


    if (csound->oparms->realtime == 1 && p->fforceSync == 0 &&
        diskin2_async_available(csound)) {
#ifdef __EMSCRIPTEN__
      DISKIN2_ARRAY **top, *current;
#endif
      p->csound = csound;
      int32_t numelem = p->bufSize*p->nChannels;
    
     /* circular buffer is allocated once per opcode
        instance and will be freed by csoundReset
        we also make sure size is compatible
      */
    if(p->cb == NULL ||
       csound->GetSizeCircularBuffer(csound,p->cb) < numelem) {
       p->cb = csound->CreateCircularBuffer(csound,
                                    numelem, sizeof(MYFLT));
    }
      if(p->cb == NULL) {
        return csound->InitError(csound, "could not allocate circular buffer\n");
      }

      // allocate buffer
      p->aOut_bufsize =
        ((unsigned int)p->bufSize) < CS_KSMPS ?
        ((MYFLT)CS_KSMPS) : ((MYFLT)p->bufSize);
      n = p->aOut_bufsize*sizeof(MYFLT)*p->nChannels;
      if (n != (int32_t)p->auxData2.size)
        csound->AuxAlloc(csound, (int32_t) n, &(p->auxData2));
      p->aOut_buf = (MYFLT *) (p->auxData2.auxp);
      memset(p->aOut_buf, 0, n);
      // allocate audio data buffer for asynchr processing
      // this is used to copy interleaved data before output
      n = CS_KSMPS*p->nChannels*sizeof(MYFLT);
      if (n != (int32_t)p->audioData.size)
       csound->AuxAlloc(csound, (int32_t) n, &(p->audioData));
#ifdef __EMSCRIPTEN__
      top =
        (DISKIN2_ARRAY **)csound->QueryGlobalVariable(csound, "DISKIN_INST_ARRAY");
      if (top != NULL) {
        current = *top;
        if (current == NULL) {
          *top = p;
        }
        else {
          while (current->nxt != NULL)
            current = current->nxt;
          current->nxt = p;
          current = current->nxt;
          }
      }
#else
      if (UNLIKELY(diskin2_add_array_instance(csound, p) != OK))
        return csound->InitError(csound, "%s",
                                 Str("diskin2: could not start async worker"));
#endif
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
    uint32_t nn, ni, nsmps = CS_KSMPS, ksmps = CS_KSMPS;
    MYFLT *samp = (MYFLT *) p->audioData.auxp;
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

    csound->ReadCircularBuffer(csound, cb, samp, nsmps*chans);
    for (ni = nn = offset; nn < nsmps; nn++, ni+=chans){
      for (chn = 0; chn < chans; chn++) {
        aOut[chn*ksmps+nn] = csound->e0dbfs*samp[chn+ni];
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

        csound->SndfileWriteSamples(csound, p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
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
        csound->SndfileWriteSamples(csound, p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
        p->c.outbufp = p->c.outbuf;
      }
      *(p->c.outbufp++) = p->asig1[nn];
      *(p->c.outbufp++) = p->asig2[nn];
    }

    return OK;
}


int32_t soundout_deinit(CSOUND *csound, void *pp)
{
    char    *opname = GetOpcodeName(pp);
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
        csound->SndfileWriteSamples(csound, q->sf, p0, (sf_count_t) ((MYFLT*) p1 - (MYFLT*) p0));
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
    SFLIB_INFO sfinfo;
    //SNDOUTS *p = (SNDOUTS*) pp;

    opname = GetOpcodeName(pp);
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

    if (stringname==0){
      if (IsStringCode(*ifilcod))
        strNcpy(name,csoundGetArgString(csound, *ifilcod), 1023);
      else csound->StringArg2Name(csound, name, ifilcod, "soundout.",0);
    }
    else strNcpy(name, ((STRINGDAT *)ifilcod)->data, 1023);

    sfname = name;
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    //sfinfo.frames = 0;
    sfinfo.samplerate = MYFLT2LONG(((SNDOUT*) pp)->h.insdshead->esr);
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
      csound->SndfileCommand(csound,q->sf, SFC_SET_CLIPPING, NULL, SFLIB_TRUE);
    else
      csound->SndfileCommand(csound,q->sf, SFC_SET_CLIPPING, NULL, SFLIB_FALSE);
#ifdef USE_DOUBLE
    csound->SndfileCommand(csound,q->sf, SFC_SET_NORM_DOUBLE, NULL, SFLIB_FALSE);
#else
    csound->SndfileCommand(csound,q->sf, SFC_SET_NORM_FLOAT, NULL, SFLIB_FALSE);
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
