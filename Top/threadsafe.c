/*
 * threadsafe.c: threadsafe API functions
 *               Copyright (c) V Lazzarini, 2013
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 */

#include "csoundCore.h"
#include "csound_orc.h"
#include "namedins.h"
#include <stdlib.h>

#ifdef USE_DOUBLE
#  define CS_FLOAT_INT_TYPE int64_t
#else
#  define CS_FLOAT_INT_TYPE int32_t
#endif

int32_t csound_compile_tree(CSOUND *csound, TREE *root, int32_t async);
int32_t csound_compile_orc(CSOUND *csound, const char *str, int32_t async);
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE* typetable, OPDS *ids);
void xturnoff_instance(CSOUND *csound, cs_float instr, int32_t insno, INSDS *ip,
                  int32_t mode, int32_t allow_release);
void csoundInputMessage(CSOUND *csound, const char *message);
int32_t csoundReadScore(CSOUND *csound, const char *message);
int32_t csound_score_event(CSOUND *csound, char type,
                             const cs_float *pfields, long numFields);
int32_t csound_score_event_absolute(CSOUND *csound, char type,
                                     const cs_float *pfields, long numFields,
                                     cs_double time_ofs);
void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);
static void csound_table_copy_out(CSOUND *csound, int32_t table, cs_float *ptable);
static void csound_table_copy_in(CSOUND *csound, int32_t table, const cs_float *ptable);

enum {INPUT_MESSAGE=1, READ_SCORE, SCORE_EVENT, SCORE_EVENT_ABS,
      TABLE_COPY_OUT, TABLE_COPY_IN, TABLE_SET, MERGE_STATE, KILL_INSTANCE};

/* MAX QUEUE SIZE */
#define API_MAX_QUEUE 1024
/* ARG LIST ALIGNMENT */
#define ARG_ALIGN 8

/* Each slot has one producer and the single kperf consumer. Positions reserve
   capacity; ready publishes the complete payload. A delayed producer can defer
   the head, but the audio thread never waits for it. Concurrent async API
   calls require MSVC or HAVE_ATOMIC_BUILTIN. Without either, the ATOMIC_*
   fallback supports serialized API calls and dequeue only. */
typedef struct _message_queue {
  int32_t message;
  char *args;
  volatile long ready;
} message_queue_t;

/* Called by csoundCreate() and after reset by csoundStart(). The queue, slots
   and payloads use the tracked allocator: memreset() also frees pending entries
   on reset/destroy, so they do not need a separate teardown path. */
void allocate_message_queue(CSOUND *csound) {
  if (csound->msg_queue == NULL) {
    int32_t i;
    csound->msg_queue = (message_queue_t **)
      csound->Calloc(csound, sizeof(message_queue_t*) * API_MAX_QUEUE);
    for (i = 0; i < API_MAX_QUEUE; i++) {
      csound->msg_queue[i] = (message_queue_t *)
        csound->Calloc(csound, sizeof(message_queue_t));
    }
    ATOMIC_SET(csound->msg_queue_write, 0);
    ATOMIC_SET(csound->msg_queue_read, 0);
  }
}

static int32_t message_queue_full(CSOUND *csound) {
  /* Read the consumer first. It can pass an older write snapshot, making
     unsigned subtraction report a full queue when there is space. */
  unsigned long read = (unsigned long) ATOMIC_GET(csound->msg_queue_read);
  unsigned long write = (unsigned long) ATOMIC_GET(csound->msg_queue_write);
  return write - read >= API_MAX_QUEUE;
}

/* Takes ownership of args, including on rejection. Allocation and copying must
   finish before reservation: neither can leave an unpublished slot behind. */
static int32_t message_enqueue_owned(CSOUND *csound, int32_t message, char *args) {
  int32_t attempt;
  if (csound->msg_queue != NULL) {
    /* Bound contention as well as capacity. A producer must never wait for the
       audio thread (which can itself call an async API from a host callback). */
    for (attempt = 0; attempt < API_MAX_QUEUE; attempt++) {
      unsigned long read = (unsigned long) ATOMIC_GET(csound->msg_queue_read);
      long write = ATOMIC_GET(csound->msg_queue_write);
      long next = (long) ((unsigned long) write + 1UL);
      message_queue_t *msg;
      if ((unsigned long) write - read >= API_MAX_QUEUE)
        break;
      if (ATOMIC_CMP_XCH(&csound->msg_queue_write, next, write))
        continue;
      msg = csound->msg_queue[(unsigned long) write % API_MAX_QUEUE];
      /* The consumer released this slot before advancing read. Reclaim its old
         payload here to keep deallocation out of the queue drain. */
      if (msg->args != NULL)
        csound->Free(csound, msg->args);
      msg->args = args;
      msg->message = message;
      ATOMIC_SET(msg->ready, 1);
      return CSOUND_SUCCESS;
    }
  }
  csound->Free(csound, args);
  return CSOUND_ERROR;
}

static int32_t message_enqueue(CSOUND *csound, int32_t message, const void *args,
                               size_t argsiz) {
  char *copy;
  if (csound->msg_queue == NULL || message_queue_full(csound))
    return CSOUND_ERROR;
  copy = (char *) csound->Calloc(csound, argsiz);
  memcpy(copy, args, argsiz);
  return message_enqueue_owned(csound, message, copy);
}

/* dequeue should be called by kperf_*()
   NB: these calls are already in place
*/
void message_dequeue(CSOUND *csound) {
  if(csound->msg_queue != NULL) {
    unsigned long read = (unsigned long) ATOMIC_GET(csound->msg_queue_read);
    unsigned long end = (unsigned long) ATOMIC_GET(csound->msg_queue_write);

    /* Snapshot the end so nested API calls wait until the next audio block. */
    while (read != end) {
      message_queue_t *msg = csound->msg_queue[read % API_MAX_QUEUE];
      if (!ATOMIC_GET(msg->ready))
        break;
      switch(msg->message) {
      case INPUT_MESSAGE:
        {
          const char *str = msg->args;
          csoundInputMessage(csound, str);
        }

        break;
      case READ_SCORE:
        {
          const char *str = msg->args;
          csoundReadScore(csound, str);
        }
        break;
      case SCORE_EVENT:
        {
          char type;
          cs_float *fargs = (cs_float *) msg->args;
          type = (char) fargs[0];
          csound_score_event(csound, type, &fargs[2], (int32_t) fargs[1]);
        }
        break;
      case SCORE_EVENT_ABS:
        {
          char type;
          const cs_float *pfields;
          long numFields;
          cs_double ofs;
          type = msg->args[0];
          memcpy(&pfields, msg->args + ARG_ALIGN,
                 sizeof(cs_float *));
          memcpy(&numFields, msg->args + ARG_ALIGN*2,
                 sizeof(long));
          memcpy(&ofs, msg->args + ARG_ALIGN*3,
                 sizeof(cs_double));

          csound_score_event_absolute(csound, type, pfields, numFields,
                                             ofs);
        }
        break;
      case TABLE_COPY_OUT:
        {
          int32_t table;
          cs_float *ptable;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(cs_float *));
          csound_table_copy_out(csound, table, ptable);
        }
        break;
      case TABLE_COPY_IN:
        {
          int32_t table;
          cs_float *ptable;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(cs_float *));
          csound_table_copy_in(csound, table, ptable);
        }
        break;
      case TABLE_SET:
        {
          int32_t table, index;
          cs_float value;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&index, msg->args + ARG_ALIGN,
                 sizeof(int32_t));
          memcpy(&value, msg->args + 2*ARG_ALIGN,
                 sizeof(cs_float));
          csound->flist[table]->ftable[index] = value;
        }
        break;
      case MERGE_STATE:
        {
          ENGINE_STATE *e;
          TYPE_TABLE *t;
          OPDS *ids;
          memcpy(&e, msg->args, sizeof(ENGINE_STATE *));
          memcpy(&t, msg->args + ARG_ALIGN,
                 sizeof(TYPE_TABLE *));
          memcpy(&ids, msg->args + 2*ARG_ALIGN,
                 sizeof(OPDS *));
          named_instr_assign_numbers(csound, e);
          merge_state(csound, e, t, ids);
        }
        break;
      case KILL_INSTANCE:
        {
          cs_float instr;
          int32_t mode, insno, rls;
          INSDS *ip, *target;
          memcpy(&instr, msg->args, sizeof(cs_float));
          memcpy(&insno, msg->args + ARG_ALIGN,
                 sizeof(int32_t));
          memcpy(&target, msg->args + ARG_ALIGN*2, sizeof(INSDS *));
          memcpy(&mode, msg->args + ARG_ALIGN*3, sizeof(int32_t));
          memcpy(&rls, msg->args + ARG_ALIGN*4, sizeof(int32_t));
          /* The saved address may have been freed. Search live nodes before
             dereferencing it. Keep the legacy API's address-based selection:
             a fresh instrument lookup could stop a different note. This check
             does not distinguish a new note that reuses the same address. */
          ip = csound->actanchor.nxtact;
          while (ip != NULL && ip != target)
            ip = ip->nxtact;
          if (ip != NULL && (int32_t) ip->insno == insno)
            xturnoff_instance(csound, instr, insno, ip, mode, rls);
        }
        break;
      }
      msg->message = 0;
      ATOMIC_SET(msg->ready, 0);
      read++;
      ATOMIC_SET(csound->msg_queue_read, (long) read);
    }
  }
}

static void report_api_queue_rejection(CSOUND *csound) {
  long expected = 0, warned = 1;
  if (!(csound->oparms->msglevel & CS_WARNMSG))
    return;
  /* Set before calling the host: its callback may submit another request.
     Warn once per reset so repeated rejection adds no logging load. */
  if (ATOMIC_CMP_XCH(&csound->msg_queue_warned, warned, expected) == 0)
    csound->MessageS(csound, CSOUNDMSG_WARNING, "%s",
                     Str("async API request could not be queued; "
                         "further warnings suppressed until reset\n"));
}

/* these are the message enqueueing functions for each relevant API function */
static inline void input_message_enqueue(CSOUND *csound,
                                              const char *str){
  if (message_enqueue(csound, INPUT_MESSAGE, str, strlen(str)+1) != CSOUND_SUCCESS)
    report_api_queue_rejection(csound);
}

static inline int32_t read_score_enqueue(CSOUND *csound, const char *str){
  return message_enqueue(csound, READ_SCORE, str, strlen(str)+1);
}

static inline int32_t table_copy_out_enqueue(CSOUND *csound, int32_t table,
                                             cs_float *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2] = { 0 };
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(cs_float *));
  if (message_enqueue(csound, TABLE_COPY_OUT, args, argsize) != CSOUND_SUCCESS) {
    report_api_queue_rejection(csound);
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

static inline int32_t table_copy_in_enqueue(CSOUND *csound, int32_t table,
                                            const cs_float *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2] = { 0 };
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(cs_float *));
  if (message_enqueue(csound, TABLE_COPY_IN, args, argsize) != CSOUND_SUCCESS) {
    report_api_queue_rejection(csound);
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

static inline int32_t score_event_enqueue(CSOUND *csound, char type,
                                          const cs_float *pfields,
                                          long numFields) {
  size_t argsize;
  cs_float *args;
  if (numFields < 0 || (size_t) numFields > SIZE_MAX / sizeof(cs_float) - 2 ||
      csound->msg_queue == NULL || message_queue_full(csound))
    return CSOUND_ERROR;
  argsize = sizeof(cs_float) * ((size_t) numFields + 2);
  args = (cs_float *) csound->Calloc(csound, argsize);
  if (numFields > 0)
    memcpy(&args[2], pfields, sizeof(cs_float) * (size_t) numFields);
  args[0] = (cs_float) type;
  args[1] = (cs_float) numFields;
  return message_enqueue_owned(csound, SCORE_EVENT, (char *) args);
}

static int32_t kill_instance_enqueue(CSOUND *csound, cs_float instr, int32_t insno,
                                     INSDS *ip, int32_t mode,
                                     int32_t allow_release) {
  char args[ARG_ALIGN*5] = { 0 };
  memcpy(args, &instr, sizeof(cs_float));
  memcpy(args+ARG_ALIGN, &insno, sizeof(int32_t));
  memcpy(args+ARG_ALIGN*2, &ip, sizeof(INSDS *));
  memcpy(args+ARG_ALIGN*3, &mode, sizeof(int32_t));
  memcpy(args+ARG_ALIGN*4, &allow_release, sizeof(int32_t));
  return message_enqueue(csound, KILL_INSTANCE, args, sizeof(args));
}

/* this is to be called from
   csound_compile_tree() in csound_orc_compile.c
*/
int32_t merge_state_enqueue(CSOUND *csound, ENGINE_STATE *e,
                            TYPE_TABLE *t, OPDS *ids) {
  /* The queue keeps ownership through shutdown; its admission check must
     reject stopped work instead of sending it to the audio-side API queue. */
  if (csound->oparms->realtime && csound->alloc_queue != NULL) {
    ALLOC_DATA data = { 0 };
    data.engine_state = e;
    data.type_table = t;
    data.ids = ids;
    data.type = ALLOC_DATA_MERGE_STATE;
    return alloc_queue_enqueue(csound, &data);
  }

  const int32_t argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3] = { 0 };
  memcpy(args, &e, sizeof(ENGINE_STATE *));
  memcpy(args+ARG_ALIGN, &t, sizeof(TYPE_TABLE *));
  memcpy(args+2*ARG_ALIGN, &ids, sizeof(OPDS *));
  return message_enqueue(csound, MERGE_STATE, args, argsize);
}

/** Async versions of the functions above
    To be removed once everything is made async
*/
void input_message_async(CSOUND *csound, const char *message){
  input_message_enqueue(csound, message);
}

int32_t read_score_async(CSOUND *csound, const char *message){
  if (read_score_enqueue(csound, message) != CSOUND_SUCCESS) {
    report_api_queue_rejection(csound);
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

int32_t score_event_async(CSOUND *csound, char type,
                          const cs_float *pfields, long numFields){
  if (score_event_enqueue(csound, type, pfields, numFields) != CSOUND_SUCCESS) {
    report_api_queue_rejection(csound);
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

int32_t csoundTableCopyOut(CSOUND *csound, int32_t table, cs_float *ptable,
                           int32_t async){
  if(async)
    return table_copy_out_enqueue(csound, table, ptable);
  if (!csound->oparms->realtime)
    csoundLockMutex(csound->API_lock);
  csound_table_copy_out(csound, table, ptable);
  if (!csound->oparms->realtime)
    csoundUnlockMutex(csound->API_lock);
  return CSOUND_SUCCESS;
}

int32_t csoundTableCopyIn(CSOUND *csound, int32_t table, const
		       cs_float *ptable, int32_t async){
  if(async)
    return table_copy_in_enqueue(csound, table, ptable);
  if (!csound->oparms->realtime)
    csoundLockMutex(csound->API_lock);
  csound_table_copy_in(csound, table, ptable);
  if (!csound->oparms->realtime)
    csoundUnlockMutex(csound->API_lock);
  return CSOUND_SUCCESS;
}

/* Legacy internal entry point, no longer part of the public API. Callers must
   keep engine state and the active list stable during target lookup. Realtime
   synchronous turnoff also skips API_lock: serialize it against performance,
   instance init/turnoff and other host calls instead of locking the audio thread. */
int32_t csoundKillInstance(CSOUND *csound, cs_float instr, char *instrName,
                       int32_t mode, int32_t allow_release, int32_t async)
{
  INSDS *ip;
  int32_t   insno;

  if (instrName) {
    instr = named_instr_find(csound, instrName);
    insno = (int32_t) instr;
  } else insno = instr;

  if (UNLIKELY(insno < 1 || insno > (int32_t) csound->engineState.maxinsno ||
               csound->engineState.instrtxtp[insno] == NULL)) {
    return CSOUND_ERROR;
  }

  if (UNLIKELY(mode < 0 || mode > 15 || (mode & 3) == 3)) {
    return CSOUND_ERROR;
  }
  ip = &(csound->actanchor);

  while ((ip = ip->nxtact) != NULL && (int32_t) ip->insno != insno);
  if (UNLIKELY(ip == NULL)) {
    return CSOUND_ERROR;
  }

  if (!async) {
    if (!csound->oparms->realtime)
      csoundLockMutex(csound->API_lock);
    xturnoff_instance(csound, instr, insno, ip, mode, allow_release);
    if (!csound->oparms->realtime)
      csoundUnlockMutex(csound->API_lock);
  }
  else
    return kill_instance_enqueue(csound, instr, insno, ip, mode, allow_release);
  return CSOUND_SUCCESS;
}

/* These helpers also run in the audio queue drain. Bela cannot take the init
   mutex here, even with a try-lock. Callers must keep table storage valid and
   prevent concurrent table changes until the copy finishes (see csound.h). */
static void csound_table_copy_out(CSOUND *csound, int32_t table,
                                  cs_float *ptable) {
  cs_float *ftab;
  int32_t len = csoundGetTable(csound, &ftab, table);
  if (LIKELY(len > 0))
    memcpy(ptable, ftab, (size_t) len * sizeof(cs_float));
}

static void csound_table_copy_in(CSOUND *csound, int32_t table,
                                 const cs_float *ptable) {
  cs_float *ftab;
  int32_t len = csoundGetTable(csound, &ftab, table);
  if (LIKELY(len > 0))
    memcpy(ftab, ptable, ((size_t) len + 1) * sizeof(cs_float));
}
