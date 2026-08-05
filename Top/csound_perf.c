/*
    csound_perf.c: engine performance

    Copyright (C) 2025 The Csound Developers

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
#include "cs_par_base.h"

int32_t dag_get_task(CSOUND *csound, int32_t index, int32_t numThreads,
                     int32_t next_task);
int32_t dag_end_task(CSOUND *csound, int32_t task);
void dag_build(CSOUND *csound, INSDS *chain);
void dag_reinit(CSOUND *csound);
void message_dequeue(CSOUND *csound);
int32_t sense_events(CSOUND *);


inline static void mix_out(MYFLT *out, MYFLT *in, uint32_t smps) {
  uint32_t i;
  for (i = 0; i < smps; i++)
    out[i] += in[i];
}

#ifdef PARCS
static int32_t get_thread_index(CSOUND *csound, void *threadId) {
  int32_t index = 0;
  THREADINFO *current = csound->multiThreadedThreadInfo;

  if (current == NULL) {
    return -1;
  }

  while (current != NULL) {
#ifdef HAVE_PTHREAD
    if (pthread_equal(*(pthread_t *)threadId,
                      *(pthread_t *)current->threadId))
#elif defined(WIN32)
    DWORD *d = (DWORD *)threadId;
    if (*d == GetThreadId((HANDLE)current->threadId))
#else
    // FIXME - need to verify this works...
    if (threadId == current->threadId)
#endif
      return index;
    index++;
    current = current->next;
  }
  return -1;
}

#define INVALID (-1)
#define WAIT (-2)
/**
   Perform one partition of multicore execution
   essentially containing similar code as
   in kperf() single thread, with the extra
   PARCS dispatching
*/
inline static int32_t node_perf(CSOUND *csound, int32_t index,
                               int32_t numThreads) {
  INSDS *insds = NULL;
  OPDS *opstart = NULL;
  int32_t played_count = 0;
  int32_t which_task;
  INSDS **task_map = (INSDS **)csound->dag_task_map;
  double time_end;
  int32_t next_task = INVALID;

  while (1) {
    int32_t done;
    which_task = dag_get_task(csound, index, numThreads, next_task);
    if(csoundGetDebug(csound) & DEBUG_PARCS)
      csound->Message(csound, "Select task %d %d\n", which_task, index);
    if (which_task == WAIT)
      continue;
    if (which_task == INVALID)
      return played_count;
    /* VL: the validity of icurTime needs to be checked */
    time_end = (csound->ksmps + csound->icurTimeSamples) / csound->esr;
    insds = task_map[which_task];
    if (insds->offtim > 0 && time_end > insds->offtim) {
      /* this is the last cycle of performance */
      insds->ksmps_no_end = insds->no_end;
    }
#if defined(MSVC)
    done = InterlockedExchangeAdd(&insds->init_done, 0);
#elif defined(HAVE_ATOMIC_BUILTIN)
    done = __atomic_load_n((int32_t *)&insds->init_done, __ATOMIC_SEQ_CST);
#else
    done = insds->init_done;
#endif
    if (done) {
      opstart = (OPDS *)task_map[which_task];
      if (insds->ksmps == csound->ksmps) {
        insds->spin = csound->spin;
        insds->spout = csound->spout_tmp + index * csound->nspout;
        insds->kcounter = csound->kcounter;
        csound->mode = 2;
        while ((opstart = opstart->nxtp) != NULL) {
          /* In case of jumping need this repeat of opstart */
          opstart->insdshead->pds = opstart;
          csound->op = opstart->optext->t.opcod;
          (*opstart->perf)(csound, opstart); /* run each opcode */
          opstart = opstart->insdshead->pds;
        }
        csound->mode = 0;
      } else {
        int32_t i, n = csound->nspout, start = 0;
        int32_t lksmps = insds->ksmps;
        int32_t incr = csound->nchnls * lksmps;
        int32_t offset = insds->ksmps_offset;
        int32_t early = insds->ksmps_no_end;
        OPDS *opstart;
        insds->spin = csound->spin;
        insds->spout = csound->spout_tmp + index * csound->nspout;
        insds->kcounter = csound->kcounter * csound->ksmps;
        /* we have to deal with sample-accurate code
           whole CS_KSMPS blocks are offset here, the
           remainder is left to each opcode to deal with.
        */
        while (offset >= lksmps) {
          offset -= lksmps;
          start += csound->nchnls;
        }
        insds->ksmps_offset = offset;
        if (UNLIKELY(early)) {
          n -= (early * csound->nchnls);
          insds->ksmps_no_end = early % lksmps;
        }
        for (i = start; i < n;
             i += incr, insds->spin += lksmps, insds->spout += lksmps) {
          opstart = (OPDS *)insds;
          csound->mode = 2;
          while ((opstart = opstart->nxtp) != NULL) {
            opstart->insdshead->pds = opstart;
            csound->op = opstart->optext->t.opcod;
            (*opstart->perf)(csound, opstart); /* run each opcode */
            opstart = opstart->insdshead->pds;
          }
          csound->mode = 0;
          insds->kcounter++;
        }
      }
      insds->ksmps_offset = 0; /* reset sample-accuracy offset */
      insds->ksmps_no_end = 0; /* reset end of loop samples */
      played_count++;
    }
    if(csoundGetDebug(csound) & DEBUG_PARCS)
      csound->Message(csound, "Finished task %d\n", which_task);
    next_task = dag_end_task(csound, which_task);
  }
  return played_count;
}

int32_t csound_node_perf(CSOUND *csound, int32_t index,
                         int32_t numThreads) {
  return node_perf(csound, index, numThreads);
}

/**
    Thread function for multicore performance
    for N-1 threads in parallel with the
    master thread.
*/
unsigned long kperf_thread(void *cs) {
  CSOUND *csound = (CSOUND *)cs;
  void *threadId;
  int32_t index;
  int32_t numThreads;
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  csound->WaitBarrier(csound->barrier2);
  threadId = csound->GetCurrentThreadID();
  index = get_thread_index(csound, threadId);
  numThreads = csound->oparms->numThreads;
  if(csoundGetDebug(csound) & DEBUG_PARCS)
    csound->Message(csound,
                  Str("Multithread performance:thread %d of "
                      "%d starting.\n"),
                  index + 1, numThreads);
  if (UNLIKELY(index < 0)) {
    csound->Die(csound, Str("Bad ThreadId"));
    return ULONG_MAX;
  }
  index++;
  int32_t parflag, taskflag = 0;
  while (1) {
#ifdef PARCS_USE_LOCK_BARRIER
    csound->WaitBarrier(csound->barrier1);
#else
    do parflag = ATOMIC_GET(csound->parflag);
    while(parflag == taskflag);
    taskflag = parflag;
#endif
    if (ATOMIC_GET(csound->multiThreadedComplete) == 1) {
      // exit thread on performance end
      free(threadId);
      return 0UL;
    }
    csound->taskflag[index] = 0;
    node_perf(csound, index, numThreads);
    csound->taskflag[index] = 1;
#ifdef PARCS_USE_LOCK_BARRIER
    csound->WaitBarrier(csound->barrier2);
#endif
  }
}
#endif // PARCS

/**
   Perform one k-cycle
   either in a single thread
   or as the master thread for multicore
*/
int32_t kperf(CSOUND *csound) {
  INSDS *ip;
  int32_t lksmps = csound->ksmps;
  /* update orchestra time */
  csound->kcounter = ++(csound->global_kcounter);
  csound->icurTimeSamples += csound->ksmps;
  csound->curBeat += csound->curBeat_inc;

  /* call message_dequeue to run API calls */
  message_dequeue(csound);

  /* if skipping time on request by 'a' score statement: */
  if (UNLIKELY(UNLIKELY(csound->advanceCnt))) {
    csound->advanceCnt--;
    return 1;
  }
  /* if i-time only, return now */
  if (UNLIKELY(csound->initonly))
    return 1;

  /* for one kcnt: */
  if (csound->oparms_.sfread) /*   if audio_infile open  */
    csound->spinrecv(csound); /*      fill the spin buf  */
  /* clear spout */
  memset(csound->spout, 0, csound->nspout * sizeof(MYFLT));
  memset(csound->spout_tmp, 0,
         sizeof(MYFLT) * csound->nspout * csound->oparms->numThreads);
  ip = csound->actanchor.nxtact;

  if (ip != NULL) {
    // multicore performance
    if (csound->multiThreadedThreadInfo != NULL) {
    /* There are 2 partitions of work: 1st by inso,
       2nd by inso count / thread count. */
#ifdef PARCS
      int32_t k;
      int32_t n = csound->oparms->numThreads;
      if (csound->dag_changed)
        dag_build(csound, ip);
      else
        dag_reinit(csound); /* set to initial state */

      /* process this partition */
#ifdef PARCS_USE_LOCK_BARRIER
      csound->WaitBarrier(csound->barrier1)
#else
      ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
      node_perf(csound, 0, n);
      /* wait until partition is complete */
#ifdef PARCS_USE_LOCK_BARRIER
      csound->WaitBarrier(csound->barrier2);
#else
      {
        int32_t i, sum;
        do {
          for(i = 1, sum = 1; i < n; i++)
            sum += csound->taskflag[i];
        } while(sum < n);
      }
#endif
      /* do the mixing of thread buffers */
      for (k = 1; k < csound->oparms->numThreads; k++)
          mix_out(csound->spout_tmp, csound->spout_tmp +
                  k * csound->nspout, csound->nspout);
#endif /* PARCS */
      csound->multiThreadedDag = NULL;
    }
    // single-thread performance
    else {
      int32_t done;
      double time_end = (csound->ksmps + csound->icurTimeSamples) / csound->esr;

      while (ip != NULL) { /* for each instr active:  */
        INSDS *nxt = ip->nxtact;
        if (UNLIKELY(csound->oparms->sampleAccurate && ip->offtim > 0 &&
                     time_end > ip->offtim)) {
          /* this is the last cycle of performance */
          if(csoundGetDebug(csound) & DEBUG_RUNTIME)
             csound->Message(csound, "last cycle %d: %f %f %d\n",
                 ip->insno, csound->icurTimeSamples/csound->esr,
                 ip->offtim, ip->no_end);
          ip->ksmps_no_end = ip->no_end;
        }
        done = ATOMIC_GET(ip->init_done);
        if (done == 1) { /* if init-pass has been done */
          int32_t error = 0;
          OPDS *opstart = (OPDS *)ip;
          ip->spin = csound->spin;
          ip->spout = csound->spout_tmp;
          ip->kcounter = csound->kcounter;
          if (ip->ksmps == csound->ksmps) {
            csound->mode = 2;
            while (error == 0 && opstart != NULL &&
                   (opstart = opstart->nxtp) != NULL && ip->actflg) {
              opstart->insdshead->pds = opstart;
              csound->op = opstart->optext->t.opcod;
              error = (*opstart->perf)(csound, opstart); /* run each opcode */
              opstart = opstart->insdshead->pds;
            }
            csound->mode = 0;
          } else {
            int32_t error = 0;
            int32_t i, n = csound->nspout, start = 0;
            lksmps = ip->ksmps;
            int32_t incr = csound->nchnls * lksmps;
            int32_t offset = ip->ksmps_offset;
            int32_t early = ip->ksmps_no_end;
            OPDS *opstart;
            ip->kcounter = (csound->kcounter - 1) * csound->ksmps / lksmps;

            /* we have to deal with sample-accurate code
               whole CS_KSMPS blocks are offset here, the
               remainder is left to each opcode to deal with.
            */
            while (offset >= lksmps) {
              offset -= lksmps;
              start += csound->nchnls;
            }
            ip->ksmps_offset = offset;
            if (UNLIKELY(early)) {
              n -= (early * csound->nchnls);
              ip->ksmps_no_end = early % lksmps;
            }
            for (i = start; i < n;
                 i += incr, ip->spin += lksmps, ip->spout += lksmps) {
              ip->kcounter++;
              opstart = (OPDS *)ip;
              csound->mode = 2;
              while (error == 0 && (opstart = opstart->nxtp) != NULL &&
                     ip->actflg) {
                opstart->insdshead->pds = opstart;
                csound->op = opstart->optext->t.opcod;
                error = (*opstart->perf)(csound, opstart); /* run each opcode */
                opstart = opstart->insdshead->pds;
              }
              csound->mode = 0;
            }
          }
        }
        ip->ksmps_offset = 0; /* reset sample-accuracy offset */
        ip->ksmps_no_end = 0; /* reset end of loop samples */
        if(nxt == NULL) {
          ip = ip->nxtact;
        /* VL 13.04.21 this allows for deletions to operate
           correctly on the active list at perf time.
           This allows for turnoff2 to work correctly
        */
        }
        else {
          ip = nxt;
          /* now check again if there is nothing nxt
             in the chain making sure turnoff also works  */
        }
      }
    }
  }
  csound->spoutran(csound); /* send to audio_out */
  return 0;
}


 int32_t csoundPerformKsmps(CSOUND *csound) {
  int32_t done;
  /* VL: 1.1.13 if not compiled (csoundStart() not called)  */
  if (UNLIKELY(!(csound->engineStatus & CS_STATE_COMP))) {
    csound->Warning(csound,
                    Str("Csound not ready for performance: csoundStart() "
                        "has not been called\n"));
    return CSOUND_ERROR;
  }
  if (csound->jumpset == 0) {
    int32_t returnValue;
    csound->jumpset = 1;
    /* setup jmp for return after an exit() */
    if (UNLIKELY((returnValue = setjmp(csound->exitjmp))))
      return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
  }
  if (!csound->oparms->realtime) // no API lock in realtime mode
    csoundLockMutex(csound->API_lock);
  do {
    done = sense_events(csound);
    if (UNLIKELY(done)) {
      if (!csound->oparms->realtime) // no API lock in realtime mode
        csoundUnlockMutex(csound->API_lock);
      if (csound->oparms->numThreads > 1) {
        ATOMIC_SET(csound->multiThreadedComplete, 1);
#ifdef PARCS_USE_LOCK_BARRIER
        csound->WaitBarrier(csound->barrier1);
#else
        ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
      }
      csoundMessage(csound, Str("end of Performance\n"));
      return done;
    }
  } while (csound->kperf(csound));
  if (!csound->oparms->realtime) // no API lock in realtime mode
    csoundUnlockMutex(csound->API_lock);
  return 0;
}

/** Perform a full buffer 
    used solely in audio backends, internally (not plugins) 
    may be fully removed in the future.
 */
int32_t perform_buffer(CSOUND *csound) {
  int32_t returnValue;
  int32_t done;
  /* VL: 1.1.13 if not compiled (csoundStart() not called)  */
  if (UNLIKELY(!(csound->engineStatus & CS_STATE_COMP))) {
    csound->Warning(csound,
                    Str("Csound not ready for performance: csoundStart() "
                        "has not been called\n"));
    return CSOUND_ERROR;
  }
  /* Setup jmp for return after an exit(). */
  if (UNLIKELY((returnValue = setjmp(csound->exitjmp)))) {
#ifndef MACOSX
    csoundMessage(csound, Str("Early return from csoundPerformBuffer().\n"));
#endif
    return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
  }
  csound->sampsNeeded += csound->oparms_.outbufsamps;
  while (csound->sampsNeeded > 0) {
    if (!csound->oparms->realtime) { // no API lock in realtime mode
      csoundLockMutex(csound->API_lock);
    }
    do {
      if (UNLIKELY((done = sense_events(csound)))) {
        if (!csound->oparms->realtime) // no API lock in realtime mode
          csoundUnlockMutex(csound->API_lock);
        if (csound->oparms->numThreads > 1) {
        ATOMIC_SET(csound->multiThreadedComplete, 1);
#ifdef PARCS_USE_LOCK_BARRIER
        csound->WaitBarrier(csound->barrier1);
#else
        ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
        }
        return done;
      }
    } while (csound->kperf(csound));
    if (!csound->oparms->realtime) { // no API lock in realtime mode
      csoundUnlockMutex(csound->API_lock);
    }
    csound->sampsNeeded -= csound->nspout;
  }
  return 0;
}


