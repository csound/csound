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
#include "sysdep.h"
#include "diskin2.h"
#include <math.h>
#include <inttypes.h>

/* iwrap semantics:
     0  : no looping (play once)
     1  : hard wrap (original behaviour)
     n>1: loop, crossfade the loop boundary over n sample frames to
          remove clicking. The loop is shortened by n frames, following
          flooper2 implementation */
#define DISKIN2_XFADE_MAX         4096

/* Force inlining so the compile-time xf selector folds
   away and the two variants keep a single implementation. For debugging,
   define an empty CS_ALWAYS_INLINE (e.g. -DCS_ALWAYS_INLINE=) to override
   the and make these ordinary inline functions. This mirrors the same patterns
   used in fm4op.c and modal4.c. Also pffft uses a similar approach*/
#ifndef CS_ALWAYS_INLINE
#  if defined(_MSC_VER)
#    define CS_ALWAYS_INLINE  __forceinline
#  elif defined(HAVE_GCC3) && !defined(SWIG)
#    define CS_ALWAYS_INLINE  inline __attribute__ ((__always_inline__))
#  else
#    define CS_ALWAYS_INLINE  inline
#  endif
#endif

/* Wrap a sample frame position into the loop range [loopStart, loopEnd).
   A single conditional adjustment is not enough when the position is more
   than one loop length out of range, which happens with very short loops
   combined with fast playback or large sync windows. */
static inline int32_t diskin2_wrap_frame(int32_t fPos, int32_t loopStart,
                                         int32_t loopLength)
{
    int64_t  rel = (int64_t)fPos - (int64_t)loopStart;
    if (UNLIKELY(rel < 0 || rel >= (int64_t)loopLength)) {
      rel %= (int64_t)loopLength;
      if (rel < 0)
        rel += (int64_t)loopLength;
      fPos = (int32_t)((int64_t)loopStart + rel);
    }
    return fPos;
}

/* Fixed point equivalent of diskin2_wrap_frame, for pos_frac. */
static inline int64_t diskin2_wrap_pos(int64_t pos, int32_t loopStart,
                                       int32_t loopLength)
{
    int64_t  start = (int64_t)loopStart << POS_FRAC_SHIFT;
    int64_t  len = (int64_t)loopLength << POS_FRAC_SHIFT;
    int64_t  rel = pos - start;
    if (UNLIKELY(rel < 0 || rel >= len)) {
      rel %= len;
      if (rel < 0)
        rel += len;
      pos = start + rel;
    }
    return pos;
}

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
        if (p->memfile != NULL) {
          memcpy(p->buf, p->memfile->data + (size_t)p->bufStartPos * p->nChannels,
                 (size_t)nsmps * sizeof(MYFLT));
          i = nsmps;
        }
        else {
          csound->SndfileSeek(csound, p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
          /* convert sample count to mono samples and read file */
          i = (int32_t) csound->SndfileReadSamples(csound, p->sf, p->buf, (sf_count_t) nsmps);
        }
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

static CS_ALWAYS_INLINE void diskin2_get_sample(CSOUND *csound,
                                      DISKIN2 *p, int32_t fPos, int32_t n,
                                      MYFLT scl)
{
    int32_t  bufPos, i;

    if (p->hasEnd && !p->wrapMode && fPos >= p->loopEnd)
      return;
    if (p->wrapMode)
      fPos = diskin2_wrap_frame(fPos, p->loopStart, p->loopLength);
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

/* iwrap > 1 requests a loop crossfade of that many frames, capped at
   DISKIN2_XFADE_MAX and at half the loop length so the shortened loop keeps at
   least half its frames; the same reader routine serves the synchronous and
   asynchronous paths. */
static void diskin2_xf_setup(CSOUND *csound, DISKIN2_XF *xf,
                             int32_t wrapMode, MYFLT iWrapMode, int32_t loopLength,
                             int32_t nChannels, MYFLT transpose)
{
    xf->len = 0;
    xf->ready = 0;
    xf->count = 0;
    xf->dir = 0;
    xf->changing = 0;
    for (int32_t i = 0; i < 3; i++) {
      xf->control[i].transpose = transpose;
      xf->control[i].steps = 0;
    }
    xf->writeControl = 0;
    xf->readControl = 1;
    xf->sharedControl = 2;
    xf->perfTranspose = transpose;
    xf->steps = xf->stepsSeen = 0;
    xf->headEnd = (int64_t)0;
    xf->buf = NULL;
    if (wrapMode && iWrapMode > FL(1.0)) {
      int32_t n, len, maxLen;
      MYFLT w = iWrapMode > (MYFLT)DISKIN2_XFADE_MAX ? (MYFLT)DISKIN2_XFADE_MAX
                                                     : iWrapMode;
      len = (int32_t)w;
      if (len < 2) len = 2;
      /* The loop is shortened by the crossfade, so cap it at half the loop to
         keep at least half the frames and never run past the loop start. */
      maxLen = loopLength / 2;
      if (len > maxLen) len = maxLen;
      if (len >= 2) {
        xf->len = len;
        n = len * nChannels * (int32_t)sizeof(MYFLT);
        if (n != (int32_t)xf->aux.size)
          csound->AuxAlloc(csound, (int32_t) n, &(xf->aux));
        xf->buf = (MYFLT*) (xf->aux.auxp);
      }
    }
}

static int32_t diskin2_init_(CSOUND *csound, DISKIN2 *p, int32_t stringname, int32_t memory);

int32_t diskin2_init(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_(csound,p,0,0);
}

int32_t memplay_init(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = FL(1.0);
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_(csound,p,0,1);
}

int32_t diskin2_init_S(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_(csound,p,1,0);
}

int32_t memplay_init_S(CSOUND *csound, DISKIN2 *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = FL(1.0);
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_(csound,p,1,1);
}

/* VL 11-01-13  diskin_init - calls diskin2_init  */

int32_t diskin_init(CSOUND *csound, DISKIN2 *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    return diskin2_init_(csound,p,0,0);
}

int32_t diskin_init_S(CSOUND *csound, DISKIN2 *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    return diskin2_init_(csound,p,1,0);
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
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    ret = diskin2_init_(csound,p,0,0);
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
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    ret = diskin2_init_(csound,p,1,0);
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
#ifndef __EMSCRIPTEN__
static uintptr_t diskin2_io_loop(CSOUND *csound, int32_t array);
#endif

typedef struct diskin2_async_entry {
  void *instance;
  void **entrySlot;
  INSDS *owner;
  volatile int32_t *stopRequested;
  volatile int32_t *instanceReaders;
  spin_lock_t spinlock;
  int32_t active;
  int32_t borrowed;
  int32_t closeOnRelease;
  int32_t array;
  struct diskin2_async_entry *next;
  struct diskin2_async_entry *activeNext;
  struct diskin2_async_entry *activePrevious;
  struct diskin2_async_entry *freeNext;
  struct diskin2_async_entry *closeNext;
} DISKIN2_ASYNC_ENTRY;

typedef struct {
  /* All entries are retained until shutdown; inactive entries move to the
     corresponding free list for reuse. */
  DISKIN2_ASYNC_ENTRY *entries;
  DISKIN2_ASYNC_ENTRY *entryTail;
  DISKIN2_ASYNC_ENTRY *freeEntries;
  /* The worker scans only active scalar entries. */
  DISKIN2_ASYNC_ENTRY *activeEntries;
  DISKIN2_ASYNC_ENTRY *activeEntryTail;
  /* Array readers have an independent allocation and active registry. */
  DISKIN2_ASYNC_ENTRY *arrayEntries;
  DISKIN2_ASYNC_ENTRY *arrayEntryTail;
  DISKIN2_ASYNC_ENTRY *arrayFreeEntries;
  DISKIN2_ASYNC_ENTRY *activeArrayEntries;
  DISKIN2_ASYNC_ENTRY *activeArrayEntryTail;
  /* File closes that must wait for the current worker borrow. */
  DISKIN2_ASYNC_ENTRY *deferredCloses;
  void *thread;
  void *arrayThread;
  volatile int32_t running;
  volatile int32_t arrayRunning;
  volatile int32_t starting;
  volatile int32_t arrayStarting;
  volatile int32_t shuttingDown;
} DISKIN2_ASYNC_STATE;

/* Registry and entry locks belong to init, cleanup, and disk workers. The
   audio thread only posts stop flags. Registration holds an owner reference
   until cleanup finishes, including gaps between worker polls. This keeps
   turnoff from freeing a reader before the event thread removes it. */

enum {
  DISKIN2_ASYNC_IDLE = 0,
  DISKIN2_ASYNC_STARTING,
  DISKIN2_ASYNC_ACTIVE,
  DISKIN2_ASYNC_STOPPED
};

#define DISKIN2_ASYNC_CANCELLED 1

#ifndef __EMSCRIPTEN__

static void diskin2_wait_for_readers(CSOUND *csound,
                                     volatile int32_t *readers)
{
  while (ATOMIC_GET(*readers) != 0) {
    /* A terminal deinit may have transferred this final reference to the
       event-thread close list just before a queued reinit started. */
    diskin2_async_drain_deferred(csound);
    csoundSleep(1);
  }
}

static inline DISKIN2_ASYNC_STATE *diskin2_async_state(CSOUND *csound)
{
  return (DISKIN2_ASYNC_STATE *) csound->diskin2_async_state;
}

static inline void diskin2_registry_lock(CSOUND *csound)
{
  csoundSpinLock(&csound->diskin2_async_lock);
}

static inline void diskin2_registry_unlock(CSOUND *csound)
{
  csoundSpinUnLock(&csound->diskin2_async_lock);
}

static inline void diskin2_entry_lock(DISKIN2_ASYNC_ENTRY *entry)
{
  csoundSpinLock(&entry->spinlock);
}

static inline void diskin2_entry_unlock(DISKIN2_ASYNC_ENTRY *entry)
{
  csoundSpinUnLock(&entry->spinlock);
}

static DISKIN2_ASYNC_ENTRY *diskin2_new_entry(CSOUND *csound)
{
  DISKIN2_ASYNC_ENTRY *entry = (DISKIN2_ASYNC_ENTRY *)
    csound->Calloc(csound, sizeof(DISKIN2_ASYNC_ENTRY));

  if (UNLIKELY(entry == NULL))
    return NULL;
  if (UNLIKELY(realtime_spin_lock_init(&entry->spinlock) != OK)) {
    csound->Free(csound, entry);
    return NULL;
  }
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

static void diskin2_activate_entry_locked(DISKIN2_ASYNC_STATE *state,
                                          DISKIN2_ASYNC_ENTRY *entry,
                                          int32_t array)
{
  DISKIN2_ASYNC_ENTRY **head = array ? &state->activeArrayEntries :
                                      &state->activeEntries;
  DISKIN2_ASYNC_ENTRY **tail = array ? &state->activeArrayEntryTail :
                                      &state->activeEntryTail;

  entry->activePrevious = *tail;
  entry->activeNext = NULL;
  if (*tail == NULL)
    *head = entry;
  else
    (*tail)->activeNext = entry;
  *tail = entry;
}

static void diskin2_deactivate_entry_locked(DISKIN2_ASYNC_STATE *state,
                                            DISKIN2_ASYNC_ENTRY *entry,
                                            int32_t array)
{
  DISKIN2_ASYNC_ENTRY **head = array ? &state->activeArrayEntries :
                                      &state->activeEntries;
  DISKIN2_ASYNC_ENTRY **tail = array ? &state->activeArrayEntryTail :
                                      &state->activeEntryTail;

  if (entry->activePrevious != NULL)
    entry->activePrevious->activeNext = entry->activeNext;
  else if (*head == entry)
    *head = entry->activeNext;
  if (entry->activeNext != NULL)
    entry->activeNext->activePrevious = entry->activePrevious;
  else if (*tail == entry)
    *tail = entry->activePrevious;
  entry->activeNext = entry->activePrevious = NULL;
}

static void diskin2_detach_entry_locked(DISKIN2_ASYNC_ENTRY *entry)
{
  entry->instance = NULL;
  entry->entrySlot = NULL;
  entry->owner = NULL;
  entry->stopRequested = NULL;
  entry->instanceReaders = NULL;
  entry->closeOnRelease = 0;
}

static DISKIN2_ASYNC_ENTRY *diskin2_take_entry(
  CSOUND *csound, DISKIN2_ASYNC_ENTRY **head,
  DISKIN2_ASYNC_ENTRY **tail, DISKIN2_ASYNC_ENTRY **freeEntries,
  void *instance, INSDS *owner, volatile int32_t *stopRequested,
  volatile int32_t *instanceReaders, int32_t array)
{
  DISKIN2_ASYNC_ENTRY *entry;

  diskin2_registry_lock(csound);
  entry = *freeEntries;
  if (entry != NULL) {
    *freeEntries = entry->freeNext;
    entry->freeNext = NULL;
  }
  diskin2_registry_unlock(csound);

  if (entry == NULL) {
    entry = diskin2_new_entry(csound);
    if (entry != NULL) {
      diskin2_registry_lock(csound);
      diskin2_append_entry(head, tail, entry);
      diskin2_registry_unlock(csound);
    }
  }

  if (entry != NULL) {
    diskin2_entry_lock(entry);
    entry->instance = instance;
    entry->owner = owner;
    entry->stopRequested = stopRequested;
    entry->instanceReaders = instanceReaders;
    entry->borrowed = 0;
    entry->closeOnRelease = 0;
    entry->array = array;
    entry->closeNext = NULL;
    ATOMIC_SET(*instanceReaders, 0);
    entry->active = 0;
    diskin2_entry_unlock(entry);
  }
  return entry;
}

static int32_t diskin2_start_thread(CSOUND *csound, int32_t array)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  volatile int32_t *running = array ? &state->arrayRunning : &state->running;
  volatile int32_t *starting = array ? &state->arrayStarting :
                                       &state->starting;
  void **thread = array ? &state->arrayThread : &state->thread;
  void *newThread;

 retry:
  diskin2_registry_lock(csound);
  if (ATOMIC_GET(state->shuttingDown)) {
    diskin2_registry_unlock(csound);
    return NOTOK;
  }
  if (ATOMIC_GET(*running)) {
    diskin2_registry_unlock(csound);
    return OK;
  }
  if (ATOMIC_GET(*starting)) {
    diskin2_registry_unlock(csound);
    csoundSleep(1);
    goto retry;
  }
  ATOMIC_SET(*starting, 1);
  diskin2_registry_unlock(csound);

  /* The worker waits for starting to clear before inspecting running, so it
     cannot exit before this thread handle and lifecycle state are published. */
  newThread = csound->CreateThread(
    array ? diskin_io_thread_array : diskin_io_thread, csound);
  diskin2_registry_lock(csound);
  *thread = newThread;
  ATOMIC_SET(*running, newThread != NULL);
  ATOMIC_SET(*starting, 0);
  diskin2_registry_unlock(csound);
  return newThread != NULL ? OK : NOTOK;
}

static void diskin2_recycle_entry(CSOUND *csound,
                                  DISKIN2_ASYNC_STATE *state,
                                  DISKIN2_ASYNC_ENTRY *entry,
                                  int32_t array)
{
  DISKIN2_ASYNC_ENTRY **freeEntries =
    array ? &state->arrayFreeEntries : &state->freeEntries;

  diskin2_registry_lock(csound);
  entry->freeNext = *freeEntries;
  *freeEntries = entry;
  diskin2_registry_unlock(csound);
}

/* Called with the registry lock. Retirement keeps its registration reference
   until the event thread observes the final borrow has ended. */
static void diskin2_retire_entry_locked(DISKIN2_ASYNC_STATE *state,
                                        DISKIN2_ASYNC_ENTRY *entry,
                                        int32_t closeFiles)
{
  diskin2_entry_lock(entry);
  entry->active = 0;
  diskin2_deactivate_entry_locked(state, entry, entry->array);
  if (entry->entrySlot != NULL)
    *entry->entrySlot = NULL;
  entry->closeOnRelease = closeFiles;
  entry->closeNext = state->deferredCloses;
  state->deferredCloses = entry;
  diskin2_entry_unlock(entry);
}

static void diskin2_drain_deferred_closes(CSOUND *csound,
                                          DISKIN2_ASYNC_STATE *state,
                                          int32_t shutdown)
{
  DISKIN2_ASYNC_ENTRY *ready = NULL, **pending;

  diskin2_registry_lock(csound);
  for (int32_t array = 0; array < 2; array++) {
    DISKIN2_ASYNC_ENTRY *entry = array ? state->activeArrayEntries :
                                        state->activeEntries;
    while (entry != NULL) {
      DISKIN2_ASYNC_ENTRY *next = entry->activeNext;
      if (ATOMIC_GET(*entry->stopRequested) || shutdown)
        diskin2_retire_entry_locked(state, entry, 1);
      entry = next;
    }
  }
  pending = &state->deferredCloses;
  while (*pending != NULL) {
    DISKIN2_ASYNC_ENTRY *entry = *pending;
    diskin2_entry_lock(entry);
    if (!entry->borrowed) {
      *pending = entry->closeNext;
      entry->closeNext = ready;
      ready = entry;
    }
    else
      pending = &entry->closeNext;
    diskin2_entry_unlock(entry);
  }
  diskin2_registry_unlock(csound);

  while (ready != NULL) {
    DISKIN2_ASYNC_ENTRY *entry = ready;
    INSDS *owner = entry->owner;
    volatile int32_t *instanceReaders = entry->instanceReaders;
    int32_t array = entry->array;
    int32_t closeFiles = entry->closeOnRelease;
    ready = entry->closeNext;

    diskin2_entry_lock(entry);
    entry->closeNext = NULL;
    diskin2_detach_entry_locked(entry);
    diskin2_entry_unlock(entry);
    diskin2_recycle_entry(csound, state, entry, array);

    /* Match the engine's turnoff decision. If another reader still owns a
       reference, its cleanup will close the files. If turnoff has not yet
       marked the owner inactive, the engine will see the final decrement.
       Retain the last reference across the close so the owner cannot be reused. */
    async_instance_lock(csound);
    closeFiles = closeFiles && ATOMIC_GET(owner->async_ref_count) == 1 &&
                 ATOMIC_GET8(owner->actflg) == 0 && owner->fdchp != NULL;
    if (!closeFiles) {
      ATOMIC_DECR(*instanceReaders);
      ATOMIC_DECR(owner->async_ref_count);
      async_instance_unlock(csound);
    }
    else {
      async_instance_unlock(csound);
      fdchclose(csound, owner);
      async_instance_lock(csound);
      ATOMIC_DECR(*instanceReaders);
      ATOMIC_DECR(owner->async_ref_count);
      async_instance_unlock(csound);
    }
  }
}

static void *diskin2_acquire_async_instance(DISKIN2_ASYNC_ENTRY *entry)
{
  void *instance = NULL;

  diskin2_entry_lock(entry);
  if (entry->active && entry->instance != NULL &&
      !ATOMIC_GET(*entry->stopRequested)) {
    instance = entry->instance;
    entry->borrowed = 1;
  }
  diskin2_entry_unlock(entry);
  return instance;
}

static void diskin2_release_async_instance(DISKIN2_ASYNC_ENTRY *entry)
{
  diskin2_entry_lock(entry);
  entry->borrowed = 0;
  diskin2_entry_unlock(entry);
}

static int32_t diskin2_remove_async_instance(
  CSOUND *csound, void **entrySlot, volatile int32_t *stopRequested,
  volatile int32_t *asyncState, int32_t *async, int32_t array,
  int32_t terminalStop);

static int32_t diskin2_add_async_instance(
  CSOUND *csound, void *instance, void **entrySlot,
  INSDS *owner, volatile int32_t *stopRequested,
  volatile int32_t *instanceReaders, volatile int32_t *asyncState,
  int32_t *async, int32_t array)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  DISKIN2_ASYNC_ENTRY **head, **tail, **freeEntries;
  DISKIN2_ASYNC_ENTRY *entry;
  int32_t cancelled;

  if (UNLIKELY(state == NULL))
    return NOTOK;
  diskin2_registry_lock(csound);
  cancelled = ATOMIC_GET(state->shuttingDown) ||
              ATOMIC_GET(*asyncState) == DISKIN2_ASYNC_STOPPED ||
              ATOMIC_GET(*stopRequested);
  diskin2_registry_unlock(csound);
  if (cancelled)
    return DISKIN2_ASYNC_CANCELLED;
  if (UNLIKELY(diskin2_start_thread(csound, array) != OK))
    return NOTOK;
  head = array ? &state->arrayEntries : &state->entries;
  tail = array ? &state->arrayEntryTail : &state->entryTail;
  freeEntries = array ? &state->arrayFreeEntries : &state->freeEntries;
  entry = diskin2_take_entry(csound, head, tail, freeEntries, instance,
                             owner, stopRequested, instanceReaders, array);
  if (UNLIKELY(entry == NULL))
    return NOTOK;

  diskin2_registry_lock(csound);
  if (!ATOMIC_GET(state->shuttingDown) &&
      ATOMIC_GET(*asyncState) != DISKIN2_ASYNC_STOPPED &&
      !ATOMIC_GET(*stopRequested)) {
    /* Init owns the instance here. Retain it before publishing to a worker. */
    async_instance_lock(csound);
    ATOMIC_INCR(owner->async_ref_count);
    ATOMIC_SET(*instanceReaders, 1);
    async_instance_unlock(csound);
    diskin2_entry_lock(entry);
    entry->entrySlot = entrySlot;
    entry->active = 1;
    diskin2_entry_unlock(entry);
    diskin2_activate_entry_locked(state, entry, array);
    *entrySlot = entry;
    *async = 1;
    ATOMIC_SET(*asyncState, DISKIN2_ASYNC_ACTIVE);
    entry = NULL;
  }
  diskin2_registry_unlock(csound);
  if (entry != NULL) {
    diskin2_entry_lock(entry);
    diskin2_detach_entry_locked(entry);
    diskin2_entry_unlock(entry);
    diskin2_recycle_entry(csound, state, entry, array);
    return DISKIN2_ASYNC_CANCELLED;
  }
  return OK;
}

static int32_t diskin2_add_instance(CSOUND *csound, DISKIN2 *p)
{
  return diskin2_add_async_instance(csound, p, &p->asyncEntry,
                                    p->h.insdshead,
                                    &p->asyncStopRequested,
                                    &p->asyncReaders, &p->asyncState,
                                    &p->async, 0);
}

static int32_t diskin2_add_array_instance(CSOUND *csound, DISKIN2_ARRAY *p)
{
  return diskin2_add_async_instance(csound, p, &p->asyncEntry,
                                    p->h.insdshead,
                                    &p->asyncStopRequested,
                                    &p->asyncReaders, &p->asyncState,
                                    &p->async, 1);
}

static int32_t diskin2_remove_async_instance(
  CSOUND *csound, void **entrySlot, volatile int32_t *stopRequested,
  volatile int32_t *asyncState, int32_t *async, int32_t array,
  int32_t terminalStop)
{
  DISKIN2_ASYNC_STATE *state;
  DISKIN2_ASYNC_ENTRY *entry;
  IGN(array);

  *async = 0;
  if (terminalStop) {
    ATOMIC_SET(*asyncState, DISKIN2_ASYNC_STOPPED);
    /* Last access to the instance: cleanup may release its reference as soon
       as it observes this flag. No registry access on the audio thread. */
    ATOMIC_SET(*stopRequested, 1);
    return OK;
  }

  /* Reinit runs on the init thread and waits for the old registration. */
  state = diskin2_async_state(csound);
  if (state == NULL) {
    ATOMIC_SET(*stopRequested, 1);
    return OK;
  }
  diskin2_registry_lock(csound);
  /* Publish the stop and its reinit close policy together. Cleanup must not
     mistake this for a terminal stop while reinit has made the owner inactive. */
  ATOMIC_SET(*stopRequested, 1);
  entry = (DISKIN2_ASYNC_ENTRY *) *entrySlot;
  if (entry != NULL && entry->active)
    diskin2_retire_entry_locked(state, entry, 0);
  diskin2_registry_unlock(csound);
  return OK;
}

static int32_t diskin2_remove_instance(CSOUND *csound, DISKIN2 *p,
                                       int32_t terminalStop)
{
  return diskin2_remove_async_instance(
    csound, &p->asyncEntry, &p->asyncStopRequested, &p->asyncState,
    &p->async, 0, terminalStop);
}

static int32_t diskin2_remove_array_instance(CSOUND *csound,
                                              DISKIN2_ARRAY *p,
                                              int32_t terminalStop)
{
  return diskin2_remove_async_instance(
    csound, &p->asyncEntry, &p->asyncStopRequested, &p->asyncState,
    &p->async, 1, terminalStop);
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

    realtime_spin_lock_destroy(&entry->spinlock);
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
  if (UNLIKELY(realtime_spin_lock_init(&csound->diskin2_async_lock) != OK)) {
    csound->Free(csound, state);
    return CSOUND_ERROR;
  }
  csound->diskin2_async_state = state;
#else
  IGN(csound);
#endif
  return CSOUND_SUCCESS;
}

void diskin2_async_drain_deferred(CSOUND *csound)
{
#ifndef __EMSCRIPTEN__
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  if (state != NULL)
    diskin2_drain_deferred_closes(csound, state, 0);
#else
  IGN(csound);
#endif
}

void diskin2_async_shutdown(CSOUND *csound)
{
#ifndef __EMSCRIPTEN__
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  void *thread, *arrayThread;

  if (state == NULL)
    return;
  ATOMIC_SET(state->shuttingDown, 1);

 wait_for_start:
  diskin2_registry_lock(csound);
  if (ATOMIC_GET(state->starting) || ATOMIC_GET(state->arrayStarting)) {
    diskin2_registry_unlock(csound);
    csoundSleep(1);
    goto wait_for_start;
  }
  ATOMIC_SET(state->running, 0);
  ATOMIC_SET(state->arrayRunning, 0);
  thread = state->thread;
  arrayThread = state->arrayThread;
  diskin2_registry_unlock(csound);

  if (thread != NULL)
    csound->JoinThread(thread);
  if (arrayThread != NULL)
    csound->JoinThread(arrayThread);
  diskin2_drain_deferred_closes(csound, state, 1);
  diskin2_free_entries(csound, state->entries);
  diskin2_free_entries(csound, state->arrayEntries);
  csound->diskin2_async_state = NULL;
  realtime_spin_lock_destroy(&csound->diskin2_async_lock);
  csound->Free(csound, state);
#else
  IGN(csound);
#endif
}

void diskin2_async_prepare_shutdown(CSOUND *csound)
{
#ifndef __EMSCRIPTEN__
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);

  if (state != NULL)
    ATOMIC_SET(state->shuttingDown, 1);
#else
  IGN(csound);
#endif
}

static inline int32_t diskin2_async_available(CSOUND *csound, int32_t array)
{
  /* Never substitute a library lock for the audio thread's slot exchange. */
#if defined(MSVC)
  /* InterlockedExchange operates on an aligned 32-bit value. */
#elif defined(HAVE_ATOMIC_BUILTIN)
  if (!__atomic_always_lock_free(sizeof(int32_t), 0))
    return 0;
#else
  return 0;
#endif
#if defined(__EMSCRIPTEN__)
  /* The browser host owns the worker and publishes its list and run flag as
     Csound globals. Fall back to synchronous reads if that host contract is
     unavailable. */
  const char *instances = array ? "DISKIN_INST_ARRAY" : "DISKIN_INST";
  const char *running = array ? "DISKIN_THREAD_START_ARRAY" :
                                "DISKIN_THREAD_START";

  return csound->QueryGlobalVariable(csound, instances) != NULL &&
         csound->QueryGlobalVariable(csound, running) != NULL;
#else
  IGN(array);
  return csound->diskin2_async_state != NULL;
#endif
}

static int32_t diskin2_begin_async_init(CSOUND *csound, int32_t reinit,
                                        INSDS *owner,
                                        volatile int32_t *asyncState,
                                        volatile int32_t *stopRequested)
{
  int32_t cancelled;
#ifndef __EMSCRIPTEN__
  int32_t locked = diskin2_async_state(csound) != NULL;

  if (locked)
    diskin2_registry_lock(csound);
#endif
  /* Fresh instances may carry STOPPED from an earlier reuse. Clear it only
     during a registered init pass. Engine/insert.c defers terminal turnoff
     while init_running is set, and this check preserves cancellation if that
     lifecycle contract is ever violated by another caller. */
  if ((!reinit && owner != NULL && ATOMIC_GET(owner->init_running)) ||
      ATOMIC_GET(*asyncState) != DISKIN2_ASYNC_STOPPED) {
    ATOMIC_SET(*asyncState, DISKIN2_ASYNC_STARTING);
    ATOMIC_SET(*stopRequested, 0);
  }
  cancelled = ATOMIC_GET(*asyncState) == DISKIN2_ASYNC_STOPPED;
#ifndef __EMSCRIPTEN__
  if (locked)
    diskin2_registry_unlock(csound);
#else
  IGN(csound);
#endif
  return cancelled;
}

static int32_t diskin2_init_(CSOUND *csound, DISKIN2 *p, int32_t stringname, int32_t memory)
{
  double  pos;
  char    name[1024];
  void    *fd;
  SFLIB_INFO sfinfo;
  int32_t     n, asyncMode;

  /* check number of channels */
  p->oChannels = (int32_t)(p->OUTOCOUNT);
  if (UNLIKELY(p->oChannels < 1 || p->oChannels > DISKIN2_MAXCHN)) {
    return csound->InitError(csound,
                             Str("diskin2: invalid number of channels"));
  }
  /* The engine owns cached samples; reinit only resets this reader. */
  if (memory && p->memfile != NULL && p->initDone && p->SkipInit != FL(0.0))
    return OK;
  /* if already open, close old file first */
  if (p->fdch.fd != NULL) {
    /* skip initialisation if requested */
    if (p->SkipInit != FL(0.0))
      return OK;
#ifdef __EMSCRIPTEN__
    if (UNLIKELY(diskin2_async_deinit(csound, p) != OK))
#else
    if (UNLIKELY(diskin2_remove_instance(csound, p, 0) != OK))
#endif
      return csound->InitError(csound, "%s",
                               Str("diskin2: could not stop async worker"));
#ifndef __EMSCRIPTEN__
    diskin2_wait_for_readers(csound, &p->asyncReaders);
#endif
    if (p->fdch.fd != NULL)
      csoundFDClose(csound, &p->fdch);
  }
  p->memfile = NULL;
  p->initDone = 0;
  p->async = 0;
  if (!memory && diskin2_begin_async_init(csound, p->h.insdshead->reinitflag,
                              p->h.insdshead, &p->asyncState,
                              &p->asyncStopRequested))
    return OK;
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

  fd = NULL;
  if (memory) {
    p->memfile = csound->LoadSoundFile(csound, name, &sfinfo);
    if (UNLIKELY(p->memfile == NULL))
      return csound->InitError(csound, Str("memplay: could not load '%s'"), name);
    if (UNLIKELY(p->memfile->nFrames > INT32_MAX || sfinfo.channels < 1 ||
                 sfinfo.channels > DISKIN2_MAXCHN)) {
      p->memfile = NULL;
      return csound->InitError(csound, "%s",
                               Str("memplay: file has too many frames or channels"));
    }
  }
  else {
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
  }

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
  p->hasEnd = 0;
  p->loopStart = 0;
  p->loopEnd = p->fileLength;
  p->loopLength = p->fileLength;
  if (p->useEnd && p->fileLength > 0) {
    double  endd = (double)p->EndTime * (double)CS_ESR * p->warpScale;
    int32_t endFrame;
    if (UNLIKELY(endd < 0.0))
      endd = 0.0;
    endFrame = (int32_t)(endd + 0.5);
    if (endFrame > p->fileLength)
      endFrame = p->fileLength;
    p->loopEnd = endFrame;
    p->hasEnd = 1;
    if (p->wrapMode) {
      int32_t startFrame = (int32_t)(p->pos_frac >> POS_FRAC_SHIFT);
      if (UNLIKELY(startFrame >= endFrame)) {
        csound->Warning(csound, Str("diskin2: iend is not after iskiptime, "
                                    "looping the whole file\n"));
        p->hasEnd = 0;
        p->loopEnd = p->fileLength;
      }
      else {
        p->loopStart = startFrame;
        p->loopLength = endFrame - startFrame;
      }
    }
  }
  p->pos_frac_inc = (int64_t)0;
  p->prv_kTranspose = FL(0.0);
  /* Set up the crossfade state before anything can read it: the synchronous
     perf path uses it directly, and the asynchronous reader must not observe a
     half-initialised xf after the instance is published. */
  asyncMode = (!memory && csound->oparms->realtime == 1 && p->fforceSync == 0 &&
               diskin2_async_available(csound, 0));
  diskin2_xf_setup(csound, &p->xf, p->wrapMode, *(p->iWrapMode),
                   p->loopLength, p->nChannels, *p->kTranspose);
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

  if (asyncMode) {
#ifdef __EMSCRIPTEN__
    DISKIN2 **top, *current;
#endif
    p->csound = csound;
    int32_t numelem =  p->bufSize*p->nChannels;

     /* circular buffer is allocated once per opcode
        instance and will be freed by csoundReset
        we also make sure size is compatible
      */
    if (p->cb == NULL ||
        csound->GetSizeCircularBuffer(csound, p->cb) < numelem) {
      void *newCb =
        csound->CreateCircularBuffer(csound, numelem, sizeof(MYFLT));

      if (UNLIKELY(newCb == NULL))
        return csound->InitError(
          csound, "%s", Str("diskin2: failed to allocate circular buffer"));
      if (p->cb != NULL)
        csound->DestroyCircularBuffer(csound, p->cb);
      p->cb = newCb;
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

    /* Complete all reader state before publishing it to the worker. */
    p->initDone = 1;
#ifdef __EMSCRIPTEN__
    top = (DISKIN2 **)csound->QueryGlobalVariable(csound, "DISKIN_INST");
    p->nxt = NULL;
    current = *top;
    if (current == NULL) {
      *top = p;
    }
    else {
      while (current->nxt != NULL)
        current = current->nxt;
      current->nxt = p;
    }
#else
    n = diskin2_add_instance(csound, p);
    if (UNLIKELY(n == NOTOK)) {
      p->initDone = 0;
      csoundFDClose(csound, &p->fdch);
      return csound->InitError(csound, "%s",
                               Str("diskin2: could not start async worker"));
    }
    if (n == DISKIN2_ASYNC_CANCELLED) {
      p->initDone = 0;
      /* Terminal deinit is deferred while this init pass is registered, so
         this path has sole ownership of the newly recorded descriptor. */
      csoundFDClose(csound, &p->fdch);
      return OK;
    }
#endif
#ifdef __EMSCRIPTEN__
    p->async = 1;
    ATOMIC_SET(p->asyncState, DISKIN2_ASYNC_ACTIVE);
#endif
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
                      (memory ? Str("memplay: using memory file") : Str("diskin2: opened")),
                      (memory ? p->memfile->fullName : csound->GetFileName(fd)),
                      sfinfo.samplerate, sfinfo.channels,
                      Str("channel(s)"),
                      (int64_t)sfinfo.frames,
                      Str("sample frames\n"));
    }
  }

  /* done initialisation */
  if (!asyncMode)
    p->initDone = 1;
  return OK;
}

int32_t diskin2_async_deinit(CSOUND *csound, DISKIN2 *p)
{
#ifdef __EMSCRIPTEN__
  if (p->async) {
    DISKIN2 **top, *current, *prv = NULL;

    top = (DISKIN2 **) csound->QueryGlobalVariable(csound, "DISKIN_INST");
    if (top == NULL) {
      p->async = 0;
      p->nxt = NULL;
      return OK;
    }
    current = *top;
    while (current != NULL && current != p) {
      prv = current;
      current = current->nxt;
    }
    if (current != NULL) {
      if (prv == NULL)
        *top = current->nxt;
      else
        prv->nxt = current->nxt;
    }
    p->nxt = NULL;
    p->async = 0;
  }
#else
  return diskin2_remove_instance(csound, p, 1);
#endif
  return OK;
}

/* Drop a cached crossfade head so it is captured again at the current
   playback speed. Used whenever pos_frac_inc changes. */
static inline void diskin2_xf_reset(DISKIN2_XF *x)
{
    x->ready = 0;
    x->count = 0;
}

/* React to a change of kTranspose (pos_frac_inc).

   A discrete step invalidates the cached head, which was captured at the
   previous speed. A continuous pitch ramp, however, changes kTranspose every
   control period; resetting on each of those changes would restart the capture
   before it can run to completion and leave the reader on the plain hard wrap,
   bringing the boundary clicks back. Keep the cached head while the speed is
   moving and refresh it only on the first change after a period of constant
   speed.

   This is the synchronous form, called once per control period from the reader
   itself. The asynchronous readers cannot use it directly, because the worker
   thread may poll them several times within one period (including when no
   frames need reading) and would mistake each extra poll for a period of
   constant speed; they use diskin2_publish_control() instead. */
static inline void diskin2_xf_speed_change(DISKIN2_XF *x)
{
    if (!x->changing)
      diskin2_xf_reset(x);
    x->changing = 1;
}

/* SPSC triple buffer: perf and worker each own a slot, with the third in the
   exchange. Acquire/release transfers ownership before either side can reuse
   a slot. Neither side waits for the other or reads a slot the other owns.
   Only the 32-bit index is atomic; pitch and the 64-bit step count travel in
   the same owned slot, including on targets without lock-free 64-bit atomics. */
#define DISKIN2_CONTROL_NEW 4
#define DISKIN2_CONTROL_INDEX 3

static inline int32_t diskin2_exchange_control(DISKIN2_XF *x, int32_t value)
{
#if defined(MSVC)
    return InterlockedExchange((volatile LONG *)&x->sharedControl, value);
#elif defined(HAVE_ATOMIC_BUILTIN)
    return __atomic_exchange_n(&x->sharedControl, value, __ATOMIC_ACQ_REL);
#else
    /* Async playback is disabled by diskin2_async_available on this target. */
    int32_t previous = x->sharedControl;
    x->sharedControl = value;
    return previous;
#endif
}

/* Classify steps once per perf period. Keep their cumulative count so a step
   survives even when the worker skips intermediate pitch updates. */
static inline void diskin2_publish_control(DISKIN2_XF *x, MYFLT transpose)
{
    if (transpose != x->perfTranspose) {
      if (!x->changing)
        x->steps++;
      x->changing = 1;
    }
    else
      x->changing = 0;
    x->perfTranspose = transpose;
    x->control[x->writeControl].transpose = transpose;
    x->control[x->writeControl].steps = x->steps;
    x->writeControl = diskin2_exchange_control(
      x, x->writeControl | DISKIN2_CONTROL_NEW) & DISKIN2_CONTROL_INDEX;
}

static inline MYFLT diskin2_read_control(DISKIN2_XF *x, int32_t *reset)
{
    DISKIN2_CONTROL control;
    if (ATOMIC_GET(x->sharedControl) & DISKIN2_CONTROL_NEW)
      x->readControl = diskin2_exchange_control(x, x->readControl) &
                       DISKIN2_CONTROL_INDEX;
    control = x->control[x->readControl];
    *reset = control.steps != x->stepsSeen;
    x->stepsSeen = control.steps;
    return control.transpose;
}

/* Loop crossfade (async, enabled by iwrap > 1).

   The loophead (the first `len` output frames after `loopStart`, or
   before `loopEnd` when playing backwards) is recorded into `buf`. At the
   opposite loop end the audio is linearly xfaded into the rec head,
   NB: we follow flooper2 for the implementation, the loop is thus shortened
   by the crossfade size.

   */
static inline void diskin2_xfade(DISKIN2_XF *x, int32_t nch,
                                 int64_t pos, int64_t inc,
                                 int32_t loopStart, int32_t loopEnd,
                                 MYFLT *frame, int32_t stride)
{
    int32_t chn, F, dir = (inc > 0 ? 1 : -1);
    int64_t origin, span, dist, ainc, maxSpan;
    MYFLT   t, idxf, fr;
    int32_t idx0, idx1;

    if (UNLIKELY(x->len <= 0 || inc == 0))
      return;

    /* span is the crossfade length in position units, positive in both
       directions; dist below is the distance travelled from the relevant
       loop edge, also positive in both directions. The crossfade may not
       cover more than half the loop measured in source frames: otherwise a
       playback increment above one frame would let the captured head reach
       loopEnd, collapsing the shortened loop (and, without the speed-aware
       cap, making the restart position land exactly on loopEnd). */
    ainc = (inc > 0 ? inc : -inc);
    maxSpan = (int64_t)((loopEnd - loopStart) >> 1) << POS_FRAC_SHIFT;
    span = (int64_t)x->len * ainc;
    if (UNLIKELY(span > maxSpan))
      span = maxSpan;
    F = (int32_t)(span / ainc);
    if (UNLIKELY(F < 2)) {
        /* too fast to crossfade this loop: fall back to a plain wrap */
        x->ready = 0;
        x->count = 0;
        return;
    }
    span = (int64_t)F * ainc;

    if (x->dir != dir) {
        /* playback direction changed: any stored or partial head is invalid */
        x->ready = 0;
        x->count = 0;
        x->dir = dir;
    }

    if (!x->ready) {
        /* Capture the reference head while it is being played: the first F
           frames after loopStart (forward), or before loopEnd (backward). */
        origin = (int64_t)(dir > 0 ? loopStart : loopEnd) << POS_FRAC_SHIFT;
        dist = (pos - origin) * dir;
        if (dist < 0 || dist > span) {
            if (x->count > 0 && x->count < F)
              x->count = 0;             /* incomplete, retry next cycle */
            return;
        }
        for (chn = 0; chn < nch; chn++)
          x->buf[x->count * nch + chn] = frame[chn * stride];
        if (++x->count >= F) {
          x->headEnd = pos + inc;
          x->ready = 1;
        }
        return;
    }

    /* Blend the outgoing audio into the captured head near the boundary:
       dist is the distance from pos to the opposite loop edge. Derive the
       captured span from headEnd so the fade stays consistent even if the
       playback increment changed after the head was recorded. */
    F = x->count;
    if (dir > 0)
      span = x->headEnd - ((int64_t)loopStart << POS_FRAC_SHIFT);
    else
      span = ((int64_t)loopEnd << POS_FRAC_SHIFT) - x->headEnd;
    if (UNLIKELY(F < 2 || span <= 0))
      return;
    origin = (int64_t)(dir > 0 ? loopEnd : loopStart) << POS_FRAC_SHIFT;
    dist = (origin - pos) * dir;
    if (dist < 0 || dist > span)
      return;
    t = FL(1.0) - (MYFLT)((double)dist / (double)span);
    if (t <= FL(0.0))
      return;
    if (t > FL(1.0))
      t = FL(1.0);
    /* 0 < t <= 1 and F >= 2, so idx0 is always in [0, F-1] */
    idxf = t * (MYFLT)(F - 1);
    idx0 = (int32_t)idxf;
    idx1 = (idx0 + 1 < F ? idx0 + 1 : idx0);
    fr = idxf - (MYFLT)idx0;
    for (chn = 0; chn < nch; chn++) {
        MYFLT live = frame[chn * stride];
        MYFLT h0 = x->buf[idx0 * nch + chn];
        MYFLT h1 = x->buf[idx1 * nch + chn];
        MYFLT head = h0 + fr * (h1 - h0);
        frame[chn * stride] = live + t * (head - live);
    }
}

#define DISKIN2_XFADE(p, frame, stride)                                     \
    diskin2_xfade(&(p)->xf, (p)->nChannels, (p)->pos_frac, (p)->pos_frac_inc, \
                  (p)->loopStart, (p)->loopEnd, (frame), (stride))

static inline void diskin2_file_pos_inc(DISKIN2 *p, int32_t *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    if (p->wrapMode)
      p->pos_frac = diskin2_wrap_pos(p->pos_frac, p->loopStart, p->loopLength);
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
}

static inline void diskin2_file_pos_inc_xf(DISKIN2 *p, int32_t *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    /* Only reached by the crossfade readers (xf.len > 0 implies wrapMode), and
       loopLength == loopEnd - loopStart, so one unsigned compare covers both
       past-the-end and before-the-start */
    if (UNLIKELY((uint32_t) (*ndx - p->loopStart) >= (uint32_t) p->loopLength)) {
      if (p->xf.ready)
        p->pos_frac = diskin2_wrap_pos(p->xf.headEnd, p->loopStart,
                                       p->loopLength);
      else {
        p->xf.count = 0;
        p->pos_frac = diskin2_wrap_pos(p->pos_frac, p->loopStart,
                                       p->loopLength);
      }
      *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    }
}


static CS_ALWAYS_INLINE int32_t
diskin2_perf_synchronous_(CSOUND *csound, DISKIN2 *p, const int32_t xf)
{

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t      nsmps = CS_KSMPS;
    int32_t      chn, i, nn;
    double   d, frac_d, x, c, v, pidwarp_d;
    MYFLT    frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t  ndx;
    int32_t  wsized2, warp;


    if (UNLIKELY(p->fdch.fd == NULL && p->memfile == NULL) ) goto file_error;
    if (!p->initDone && !p->SkipInit){
      return csound->PerfError(csound, &(p->h),
                               Str("diskin2: not initialised"));
    }
    if (UNLIKELY(*(p->kTranspose) != p->prv_kTranspose)) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
      /* a step invalidates the captured head; a ramp keeps it */
      diskin2_xf_speed_change(&p->xf);
    }
    else
      p->xf.changing = 0;
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut[0] + nn, CS_KSMPS);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut[0] + nn, CS_KSMPS);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut[0] + nn, CS_KSMPS);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut[0] + nn, CS_KSMPS);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
          diskin2_file_pos_inc(p, &ndx);
      }
    }
    /* apply 0dBFS scale and copy to outputs */
    for (chn = 0; chn < p->oChannels; chn++) {
      if(chn < p->nChannels) {
      for (nn = offset; nn < nsmps; nn++)
        p->out[chn][nn] = p->aOut[chn][nn] * csound->e0dbfs;
      } else /* excess channels set to 0 */
        memset(p->out[chn], 0, CS_KSMPS * sizeof(MYFLT));
    }
    return OK;
 file_error:
    csound->ErrorMsg(csound, Str("diskin2: file descriptor closed or invalid\n"));
    return NOTOK;

}

int32_t diskin2_perf_synchronous(CSOUND *csound, DISKIN2 *p)
{
    return diskin2_perf_synchronous_(csound, p, 0);
}

static int32_t diskin2_perf_synchronous_xfade(CSOUND *csound, DISKIN2 *p)
{
    return diskin2_perf_synchronous_(csound, p, 1);
}




static CS_ALWAYS_INLINE void
diskin_file_read_(CSOUND *csound, DISKIN2 *p, const int32_t xf)
{

    /* nsmps is bufsize in frames */
    int32_t nsmps = csound->CheckCircularBuffer(csound, p->cb, 1)/p->nChannels;
    int32_t i, nn;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t ndx;
    int32_t wsized2, warp;
    MYFLT   *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */
    int32_t reset;
    MYFLT transpose = diskin2_read_control(&p->xf, &reset);

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
    if (reset)
      diskin2_xf_reset(&p->xf);
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
        /* xf is a constant known at compile time, so the compiler can eliminate
           the branch when inlining, for this and the other cases below */
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_xf(p, &ndx);
        }
        else
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

void diskin_file_read(CSOUND *csound, DISKIN2 *p)
{
    diskin_file_read_(csound, p, 0);
}

static void diskin_file_read_xfade(CSOUND *csound, DISKIN2 *p)
{
    diskin_file_read_(csound, p, 1);
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
    diskin2_publish_control(&p->xf, *p->kTranspose);

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
      if (current->xf.len) diskin_file_read_xfade(csound, current);
      else diskin_file_read(csound, current);
      current = current->nxt;
    }
  }
#else
  return diskin2_io_loop((CSOUND *) p, 0);
#endif
  return 0;
}


int32_t diskin2_perf(CSOUND *csound, DISKIN2 *p) {
    if (!p->async)
      return p->xf.len ? diskin2_perf_synchronous_xfade(csound, p)
                       : diskin2_perf_synchronous(csound, p);
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
        if (p->memfile != NULL) {
          memcpy(p->buf, p->memfile->data + (size_t)p->bufStartPos * p->nChannels,
                 (size_t)nsmps * sizeof(MYFLT));
          i = nsmps;
        }
        else {
          csound->SndfileSeek(csound, p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
          /* convert sample count to mono samples and read file */
          i = (int32_t) csound->SndfileReadSamples(csound, p->sf, p->buf, (sf_count_t) nsmps);
        }
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
    if (p->wrapMode)
      p->pos_frac = diskin2_wrap_pos(p->pos_frac, p->loopStart, p->loopLength);
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
}

static inline void diskin2_file_pos_inc_array_xf(DISKIN2_ARRAY *p, int32_t *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    /* Only reached by the crossfade readers (xf.len > 0 implies wrapMode), and
       loopLength == loopEnd - loopStart, so one unsigned compare covers both
       past-the-end and before-the-start */
    if (UNLIKELY((uint32_t) (*ndx - p->loopStart) >= (uint32_t) p->loopLength)) {
      if (p->xf.ready)
        p->pos_frac = diskin2_wrap_pos(p->xf.headEnd, p->loopStart,
                                       p->loopLength);
      else {
        p->xf.count = 0;
        p->pos_frac = diskin2_wrap_pos(p->pos_frac, p->loopStart,
                                       p->loopLength);
      }
      *ndx = (int32_t) (p->pos_frac >> POS_FRAC_SHIFT);
    }
}

static CS_ALWAYS_INLINE void diskin2_get_sample_array(CSOUND *csound,
                                            DISKIN2_ARRAY *p, int32_t fPos,
                                            int32_t n, MYFLT scl) {
    int32_t  bufPos, i;
    int32_t ksmps = CS_KSMPS;
    MYFLT *aOut = (MYFLT *) p->aOut->data;

    if (p->hasEnd && !p->wrapMode && fPos >= p->loopEnd)
      return;
    if (p->wrapMode)
      fPos = diskin2_wrap_frame(fPos, p->loopStart, p->loopLength);
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
#ifdef __EMSCRIPTEN__
  if (p->async) {
    DISKIN2_ARRAY **top, *current, *prv = NULL;

    top = (DISKIN2_ARRAY **) csound->QueryGlobalVariable(
      csound, "DISKIN_INST_ARRAY");
    if (top == NULL) {
      p->async = 0;
      p->nxt = NULL;
      return OK;
    }
    current = *top;
    while (current != NULL && current != p) {
      prv = current;
      current = current->nxt;
    }
    if (current != NULL) {
      if (prv == NULL)
        *top = current->nxt;
      else
        prv->nxt = current->nxt;
    }
    p->nxt = NULL;
    p->async = 0;
  }
#else
  return diskin2_remove_array_instance(csound, p, 1);
#endif
  return OK;
}

static CS_ALWAYS_INLINE void
diskin_file_read_array_(CSOUND *csound, DISKIN2_ARRAY *p, const int32_t xf)
{

    /* nsmps is bufsize in frames */
    int32_t nsmps = csound->CheckCircularBuffer(csound, p->cb, 1)/p->nChannels;
    int32_t i, nn;
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32_t   ndx;
    int32_t     wsized2, warp;
    MYFLT  *aOut = (MYFLT *)p->aOut_buf; /* needs to be allocated */
    int32_t reset;
    MYFLT transpose = diskin2_read_control(&p->xf, &reset);

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
    if (reset)
      diskin2_xf_reset(&p->xf);
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut_buf + nn * p->nChannels, 1);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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

void diskin_file_read_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    diskin_file_read_array_(csound, p, 0);
}

static void diskin_file_read_array_xfade(CSOUND *csound, DISKIN2_ARRAY *p)
{
    diskin_file_read_array_(csound, p, 1);
}


#ifndef __EMSCRIPTEN__
static uintptr_t diskin2_io_loop(CSOUND *csound, int32_t array)
{
  DISKIN2_ASYNC_STATE *state = diskin2_async_state(csound);
  volatile int32_t *running = array ? &state->arrayRunning : &state->running;
  volatile int32_t *starting = array ? &state->arrayStarting :
                                       &state->starting;
  int32_t wakeup = 1000 * csound->ksmps / csound->esr;

  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  for (;;) {
    DISKIN2_ASYNC_ENTRY *entry;

    csoundSleep(wakeup > 0 ? wakeup : 1);
    while (ATOMIC_GET(*starting))
      csoundSleep(1);
    if (!ATOMIC_GET(*running))
      break;
    diskin2_registry_lock(csound);
    entry = array ? state->activeArrayEntries : state->activeEntries;
    diskin2_registry_unlock(csound);
    while (entry != NULL) {
      DISKIN2_ASYNC_ENTRY *next;
      void *current;

      diskin2_registry_lock(csound);
      next = entry->activeNext;
      diskin2_registry_unlock(csound);
      current = diskin2_acquire_async_instance(entry);
      if (current != NULL) {
        if (array) {
          DISKIN2_ARRAY *item = (DISKIN2_ARRAY *) current;
          if (item->xf.len) diskin_file_read_array_xfade(csound, item);
          else diskin_file_read_array(csound, item);
        }
        else {
          DISKIN2 *item = (DISKIN2 *) current;
          if (item->xf.len) diskin_file_read_xfade(csound, item);
          else diskin_file_read(csound, item);
        }
        diskin2_release_async_instance(entry);
      }
      entry = next;
    }
  }
  return 0;
}
#endif

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
      if (current->xf.len) diskin_file_read_array_xfade(csound, current);
      else diskin_file_read_array(csound, current);
      current = current->nxt;
    }
  }
#else
  return diskin2_io_loop((CSOUND *) p, 1);
#endif
  return 0;
}


static int32_t diskin2_init_array(CSOUND *csound, DISKIN2_ARRAY *p,
                                  int32_t stringname, int32_t memory){
    double  pos;
    char    name[1024];
    void    *fd;
    SFLIB_INFO sfinfo;
    int32_t     n, asyncMode;
    ARRAYDAT *t = p->aOut;

    /* The engine owns cached samples; reinit only resets this reader. */
    if (memory && p->memfile != NULL && p->initDone && p->SkipInit != FL(0.0))
      return OK;
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (p->SkipInit != FL(0.0))
        return OK;
#ifdef __EMSCRIPTEN__
      if (UNLIKELY(diskin2_async_deinit_array(csound, p) != OK))
#else
      if (UNLIKELY(diskin2_remove_array_instance(csound, p, 0) != OK))
#endif
        return csound->InitError(csound, "%s",
                                 Str("diskin2: could not stop async worker"));
#ifndef __EMSCRIPTEN__
      diskin2_wait_for_readers(csound, &p->asyncReaders);
#endif
      if (p->fdch.fd != NULL)
        csoundFDClose(csound, &p->fdch);
    }
    p->memfile = NULL;
    p->initDone = 0;
    p->async = 0;
    if (!memory && diskin2_begin_async_init(csound, p->h.insdshead->reinitflag,
                                p->h.insdshead, &p->asyncState,
                                &p->asyncStopRequested))
      return OK;
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

    fd = NULL;
    if (memory) {
      p->memfile = csound->LoadSoundFile(csound, name, &sfinfo);
      if (UNLIKELY(p->memfile == NULL))
        return csound->InitError(csound, Str("memplay: could not load '%s'"), name);
      if (UNLIKELY(p->memfile->nFrames > INT32_MAX || sfinfo.channels < 1)) {
        p->memfile = NULL;
        return csound->InitError(csound, "%s",
                                 Str("memplay: invalid file length or channel count"));
      }
    }
    else {
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
    }

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
    p->hasEnd = 0;
    p->loopStart = 0;
    p->loopEnd = p->fileLength;
    p->loopLength = p->fileLength;
    if (p->useEnd && p->fileLength > 0) {
      double  endd = (double)p->EndTime * (double)CS_ESR * p->warpScale;
      int32_t endFrame;
      if (UNLIKELY(endd < 0.0))
        endd = 0.0;
      endFrame = (int32_t)(endd + 0.5);
      if (endFrame > p->fileLength)
        endFrame = p->fileLength;
      p->loopEnd = endFrame;
      p->hasEnd = 1;
      if (p->wrapMode) {
        int32_t startFrame = (int32_t)(p->pos_frac >> POS_FRAC_SHIFT);
        if (UNLIKELY(startFrame >= endFrame)) {
          csound->Warning(csound, Str("diskin2: iend is not after iskiptime, "
                                      "looping the whole file\n"));
          p->hasEnd = 0;
          p->loopEnd = p->fileLength;
        }
        else {
          p->loopStart = startFrame;
          p->loopLength = endFrame - startFrame;
        }
      }
    }
    p->pos_frac_inc = (int64_t)0;
    p->prv_kTranspose = FL(0.0);
    /* See diskin2_init_: set up the crossfade state before anything can read
       it (synchronous perf path or published asynchronous reader). */
    asyncMode = (!memory && csound->oparms->realtime == 1 && p->fforceSync == 0 &&
                 diskin2_async_available(csound, 1));
    diskin2_xf_setup(csound, &p->xf, p->wrapMode, *(p->iWrapMode),
                     p->loopLength, p->nChannels, *p->kTranspose);
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

    if (asyncMode) {
#ifdef __EMSCRIPTEN__
      DISKIN2_ARRAY **top, *current;
#endif
      p->csound = csound;
      int32_t numelem = p->bufSize*p->nChannels;

      /* The circular buffer is allocated once per opcode instance and freed
         by csoundReset. Reallocate it if the required size has grown. */
      if (p->cb == NULL ||
          csound->GetSizeCircularBuffer(csound, p->cb) < numelem) {
        void *newCb =
          csound->CreateCircularBuffer(csound, numelem, sizeof(MYFLT));

        if (UNLIKELY(newCb == NULL))
          return csound->InitError(csound,
                                   "could not allocate circular buffer\n");
        if (p->cb != NULL)
          csound->DestroyCircularBuffer(csound, p->cb);
        p->cb = newCb;
      }

      p->aOut_bufsize =
        ((unsigned int)p->bufSize) < CS_KSMPS ?
        ((MYFLT)CS_KSMPS) : ((MYFLT)p->bufSize);
      n = p->aOut_bufsize*sizeof(MYFLT)*p->nChannels;
      if (n != (int32_t)p->auxData2.size)
        csound->AuxAlloc(csound, (int32_t) n, &(p->auxData2));
      p->aOut_buf = (MYFLT *) (p->auxData2.auxp);
      memset(p->aOut_buf, 0, n);

      /* Copy interleaved data here before writing opcode outputs. */
      n = CS_KSMPS*p->nChannels*sizeof(MYFLT);
      if (n != (int32_t)p->audioData.size)
        csound->AuxAlloc(csound, (int32_t) n, &(p->audioData));
      p->initDone = 1;
#ifdef __EMSCRIPTEN__
      top = (DISKIN2_ARRAY **) csound->QueryGlobalVariable(
        csound, "DISKIN_INST_ARRAY");
      p->nxt = NULL;
      current = *top;
      if (current == NULL) {
        *top = p;
      }
      else {
        while (current->nxt != NULL)
          current = current->nxt;
        current->nxt = p;
      }
#else
      n = diskin2_add_array_instance(csound, p);
      if (UNLIKELY(n == NOTOK)) {
        p->initDone = 0;
        csoundFDClose(csound, &p->fdch);
        return csound->InitError(csound, "%s",
                                 Str("diskin2: could not start async worker"));
      }
      if (n == DISKIN2_ASYNC_CANCELLED) {
        p->initDone = 0;
        /* See the scalar path above: deinit cannot close this descriptor
           concurrently while the owning init pass is still running. */
        csoundFDClose(csound, &p->fdch);
        return OK;
      }
#endif
#ifdef __EMSCRIPTEN__
      p->async = 1;
      ATOMIC_SET(p->asyncState, DISKIN2_ASYNC_ACTIVE);
#endif

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
                        (memory ? Str("memplay: using memory file") : Str("diskin2: opened")),
                        (memory ? p->memfile->fullName : csound->GetFileName(fd)),
                        sfinfo.samplerate, sfinfo.channels,
                        Str("channel(s)"),
                        (int64_t)sfinfo.frames,
                        Str("sample frames\n"));
      }
    }

    /* done initialisation */
    if (!asyncMode)
      p->initDone = 1;
    return OK;
}


static CS_ALWAYS_INLINE int32_t
diskin2_perf_synchronous_array_(CSOUND *csound, DISKIN2_ARRAY *p, const int32_t xf)
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


    if (UNLIKELY(p->fdch.fd == NULL && p->memfile == NULL) ) goto file_error;
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
      /* a step invalidates the captured head; a ramp keeps it */
      diskin2_xf_speed_change(&p->xf);
    }
    else
      p->xf.changing = 0;
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut->data + nn, CS_KSMPS);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut->data + nn, CS_KSMPS);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut->data + nn, CS_KSMPS);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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
        if (xf) {
          DISKIN2_XFADE(p, p->aOut->data + nn, CS_KSMPS);
          diskin2_file_pos_inc_array_xf(p, &ndx);
        }
        else
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

int32_t diskin2_perf_synchronous_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    return diskin2_perf_synchronous_array_(csound, p, 0);
}

static int32_t diskin2_perf_synchronous_array_xfade(CSOUND *csound, DISKIN2_ARRAY *p)
{
    return diskin2_perf_synchronous_array_(csound, p, 1);
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
    diskin2_publish_control(&p->xf, *p->kTranspose);

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
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_array(csound,p,0,0);
}

int32_t memplay_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = FL(1.0);
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_array(csound,p,0,1);
}

int32_t diskin2_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = *p->forceSync;
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_array(csound,p,1,0);
}

int32_t memplay_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p) {
    p->SkipInit = *p->iSkipInit;
    p->WinSize = *p->iWinSize;
    p->BufSize =  *p->iBufSize;
    p->fforceSync = FL(1.0);
    p->EndTime = *p->iEnd;
    p->useEnd = (p->INOCOUNT > 9);
    return diskin2_init_array(csound,p,1,1);
}

/* diskin_init_array - calls diskin2_init_array  */

int32_t diskin_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    return diskin2_init_array(csound,p,0,0);
}

int32_t diskin_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p){
    p->SkipInit = *p->iWinSize;
    p->WinSize = 2;
    p->BufSize = 0;
    p->fforceSync = 0;
    p->EndTime = FL(0.0);
    p->useEnd = 0;
    return diskin2_init_array(csound,p,1,0);
}

int32_t diskin2_perf_array(CSOUND *csound, DISKIN2_ARRAY *p) {
    if (!p->async)
      return p->xf.len ? diskin2_perf_synchronous_array_xfade(csound, p)
                       : diskin2_perf_synchronous_array(csound, p);
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
      csound->FileClose(csound, q->fd, CSFILE_CLOSE_SYNC);
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

/* Cached files live until engine reset; note-off only invalidates the reader. */
int32_t memplay_deinit(CSOUND *csound, DISKIN2 *p)
{
    IGN(csound);
    p->memfile = NULL;
    p->initDone = 0;
    return OK;
}

int32_t memplay_deinit_array(CSOUND *csound, DISKIN2_ARRAY *p)
{
    IGN(csound);
    p->memfile = NULL;
    p->initDone = 0;
    return OK;
}
