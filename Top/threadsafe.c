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
#  define MYFLT_INT_TYPE int64_t
#else
#  define MYFLT_INT_TYPE int32_t
#endif

int32_t csound_compile_tree(CSOUND *csound, TREE *root, int32_t async);
int32_t csound_compile_orc(CSOUND *csound, const char *str, int32_t async);
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE* typetable, OPDS *ids);
void xturnoff_instance(CSOUND *csound, MYFLT instr, int32_t insno, INSDS *ip,
                  int32_t mode, int32_t allow_release);
void csoundInputMessage(CSOUND *csound, const char *message);
int32_t csoundReadScore(CSOUND *csound, const char *message);
int32_t csound_score_event(CSOUND *csound, char type,
                             const MYFLT *pfields, long numFields);
int32_t csound_score_event_absolute(CSOUND *csound, char type,
                                     const MYFLT *pfields, long numFields,
                                     double time_ofs);
void set_channel_data_ptr(CSOUND *csound, const char *name,
                          void *ptr, int32_t newSize);
void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);
static void csound_table_copy_out(CSOUND *csound, int32_t table, MYFLT *ptable);
static void csound_table_copy_in(CSOUND *csound, int32_t table, const MYFLT *ptable);
static void csound_table_set(CSOUND *csound, int32_t table, int32_t index, MYFLT value);

enum {INPUT_MESSAGE=1, READ_SCORE, SCORE_EVENT, SCORE_EVENT_ABS,
      TABLE_COPY_OUT, TABLE_COPY_IN, TABLE_SET, MERGE_STATE, KILL_INSTANCE};

/* MAX QUEUE SIZE */
#define API_MAX_QUEUE 1024
/* ARG LIST ALIGNMENT */
#define ARG_ALIGN 8

/* Message queue structure */
typedef struct _message_queue {
  int64_t message;  /* message id */
  char *args;   /* args, arg pointers */
  int64_t rtn;  /* return value */
} message_queue_t;


/* atomicGetAndIncrementWithModulus */
static long atomicGet_Incr_Mod(volatile long* val, long mod) {
  long oldVal;
  long newVal;
  do {
    oldVal = *val;
    newVal = (oldVal + 1) % mod;
  } while (ATOMIC_CMP_XCH(val, newVal, oldVal));
  return oldVal;
}

/* called by csoundCreate() at the start
   and also by csoundStart() to cover de-allocation
   by reset
*/
void allocate_message_queue(CSOUND *csound) {
  if (csound->msg_queue == NULL) {
    int32_t i;
    csound->msg_queue = (message_queue_t **)
      csound->Calloc(csound, sizeof(message_queue_t*)*API_MAX_QUEUE);
    for (i = 0; i < API_MAX_QUEUE; i++) {
      csound->msg_queue[i] =
        (message_queue_t*)
        csound->Calloc(csound, sizeof(message_queue_t));
    }
  }
}


/* enqueue should be called by the relevant API function */
void *message_enqueue(CSOUND *csound, int32_t message, char *args,
                      int32_t argsiz) {
  if(csound->msg_queue != NULL) {
    int64_t *rtn;
    volatile long items;

    /* block if queue is full */
    do {
      items = ATOMIC_GET(csound->msg_queue_items);
    } while(items >= API_MAX_QUEUE);

    message_queue_t* msg =
      csound->msg_queue[atomicGet_Incr_Mod(&csound->msg_queue_wget,
                                           API_MAX_QUEUE)];
    msg->message = message;
    if(msg->args != NULL)
      csound->Free(csound, msg->args);
    msg->args = (char *)csound->Calloc(csound, argsiz);
    memcpy(msg->args, args, argsiz);
    rtn = &msg->rtn;
    csound->msg_queue[atomicGet_Incr_Mod(&csound->msg_queue_wput,
                                         API_MAX_QUEUE)] = msg;
    ATOMIC_INCR(csound->msg_queue_items);
    return (void *) rtn;
  }
  else return NULL;
}

/* dequeue should be called by kperf_*()
   NB: these calls are already in place
*/
void message_dequeue(CSOUND *csound) {
  if(csound->msg_queue != NULL) {
    long rp = csound->msg_queue_rstart;
    long items = csound->msg_queue_items;
    long rend = rp + items;

    while(rp < rend) {
      message_queue_t* msg = csound->msg_queue[rp % API_MAX_QUEUE];
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
          MYFLT *fargs = (MYFLT *) msg->args;
          type = (char) fargs[0];
          csound_score_event(csound, type, &fargs[2], (int32_t)
                                   (int32_t) fargs[1]);
        }
        break;
      case SCORE_EVENT_ABS:
        {
          char type;
          const MYFLT *pfields;
          long numFields;
          double ofs;
          type = msg->args[0];
          memcpy(&pfields, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          memcpy(&numFields, msg->args + ARG_ALIGN*2,
                 sizeof(long));
          memcpy(&ofs, msg->args + ARG_ALIGN*3,
                 sizeof(double));

          csound_score_event_absolute(csound, type, pfields, numFields,
                                             ofs);
        }
        break;
      case TABLE_COPY_OUT:
        {
          int32_t table;
          MYFLT *ptable;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          csound_table_copy_out(csound, table, ptable);
        }
        break;
      case TABLE_COPY_IN:
        {
          int32_t table;
          MYFLT *ptable;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          csound_table_copy_in(csound, table, ptable);
        }
        break;
      case TABLE_SET:
        {
          int32_t table, index;
          MYFLT value;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&index, msg->args + ARG_ALIGN,
                 sizeof(int32_t));
          memcpy(&value, msg->args + 2*ARG_ALIGN,
                 sizeof(MYFLT));
          csound_table_set(csound, table, index, value);
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
          MYFLT instr;
          int32_t mode, insno, rls;
          INSDS *ip;
          memcpy(&instr, msg->args, sizeof(MYFLT));
          memcpy(&insno, msg->args + ARG_ALIGN,
                 sizeof(int32_t));
          memcpy(&ip, msg->args + ARG_ALIGN*2,
                 sizeof(INSDS *));
          memcpy(&mode, msg->args + ARG_ALIGN*3,
                 sizeof(int32_t));
          memcpy(&rls, msg->args  + ARG_ALIGN*4,
                 sizeof(int32_t));
          xturnoff_instance(csound, instr, insno, ip, mode, rls);
        }
        break;
      }
      msg->message = 0;
      rp += 1;
    }
    ATOMIC_SUB(csound->msg_queue_items, items);
    csound->msg_queue_rstart = rp % API_MAX_QUEUE;
  }
}

/* these are the message enqueueing functions for each relevant API function */
static inline void input_message_enqueue(CSOUND *csound,
                                              const char *str){
  message_enqueue(csound,INPUT_MESSAGE, (char *) str, (int32_t) strlen(str)+1);
}

static inline int64_t *read_score_enqueue(CSOUND *csound, const char *str){
  return message_enqueue(csound, READ_SCORE, (char *) str, (int32_t) strlen(str)+1);
}

static inline void table_copy_out_enqueue(CSOUND *csound, int32_t table,
                                              MYFLT *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_OUT, args, argsize);
}

static inline void table_copy_in_enqueue(CSOUND *csound, int32_t table,
                                             const MYFLT *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_IN, args, argsize);
}

static inline int64_t *score_event_enqueue(CSOUND *csound, char type,
                                                const MYFLT *pfields,
                                                long numFields)
{
  const int32_t argsize = (int32_t) (sizeof(MYFLT)*(numFields+2));
  MYFLT *args = csoundCalloc(csound, argsize);
  memcpy(&args[2], pfields, argsize - sizeof(MYFLT)*2);
  args[0] = (MYFLT) type;
  args[1] = numFields;
  return message_enqueue(csound, SCORE_EVENT, (char *) args,
                         argsize);
}



void kill_instance_enqueue(CSOUND *csound, MYFLT instr, int32_t insno,
                          INSDS *ip, int32_t mode,
                          int32_t allow_release) {
  const int32_t argsize = ARG_ALIGN*5;
  char args[ARG_ALIGN*5];
  memcpy(args, &instr, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &insno, sizeof(int32_t));
  memcpy(args+ARG_ALIGN*2, &ip, sizeof(INSDS *));
  memcpy(args+ARG_ALIGN*3, &mode, sizeof(int32_t));
  memcpy(args+ARG_ALIGN*4, &allow_release, sizeof(int32_t));
  message_enqueue(csound,KILL_INSTANCE,args,argsize);
}

/* this is to be called from
   csound_compile_tree() in csound_orc_compile.c
*/
void merge_state_enqueue(CSOUND *csound, ENGINE_STATE *e, TYPE_TABLE* t, OPDS *ids) {
  const int32_t argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  memcpy(args, &e, sizeof(ENGINE_STATE *));
  memcpy(args+ARG_ALIGN, &t, sizeof(TYPE_TABLE *));
  memcpy(args+2*ARG_ALIGN, &ids, sizeof(OPDS *));
  message_enqueue(csound,MERGE_STATE, args, argsize);
}

int32_t init0(CSOUND *csound);

MYFLT csoundEvalCode(CSOUND *csound, const char *str)
{
  int32_t async = 0;
  if (str && csound_compile_orc(csound,str,async)
      == CSOUND_SUCCESS){
    if(!(csound->engineStatus & CS_STATE_COMP)) {
      init0(csound);
    }
      return csound->instr0->instance[0].retval;
    }
#ifdef NAN
  else return NAN;
#else
  else return 0;
#endif
}

/** Async versions of the functions above
    To be removed once everything is made async
*/
void input_message_async(CSOUND *csound, const char *message){
  input_message_enqueue(csound, message);
}

void read_score_async(CSOUND *csound, const char *message){
  read_score_enqueue(csound, message);
}

void score_event_async(CSOUND *csound, char type,
                           const MYFLT *pfields, long numFields){
  score_event_enqueue(csound, type, pfields, numFields);
}

void csoundTableCopyOut(CSOUND *csound, int32_t table, MYFLT *ptable, int32_t async){
  if(async) {
    table_copy_out_enqueue(csound, table, ptable);
    return;
  }
  csoundLockMutex(csound->API_lock);
  csound_table_copy_out(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableCopyIn(CSOUND *csound, int32_t table, const
		       MYFLT *ptable, int32_t async){
  if(async) {
    table_copy_in_enqueue(csound, table, ptable);
    return;
  }
  csoundLockMutex(csound->API_lock);
  csound_table_copy_in(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

int32_t csoundKillInstance(CSOUND *csound, MYFLT instr, char *instrName,
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
    csoundUnlockMutex(csound->API_lock);
    return CSOUND_ERROR;
  }
  ip = &(csound->actanchor);

  while ((ip = ip->nxtact) != NULL && (int32_t) ip->insno != insno);
  if (UNLIKELY(ip == NULL)) {
    return CSOUND_ERROR;
  }

  if (!async) {
    csoundLockMutex(csound->API_lock);
    xturnoff_instance(csound, instr, insno, ip, mode, allow_release);
    csoundUnlockMutex(csound->API_lock);
  }
  else
    kill_instance_enqueue(csound, instr, insno, ip, mode, allow_release);
  return CSOUND_SUCCESS;
}

void csound_table_set(CSOUND *csound, int32_t table, int32_t index,
                            MYFLT value) {
  if (csound->oparms->realtime)
    csoundLockMutex(csound->init_pass_threadlock);
  csound->flist[table]->ftable[index] = value;
  if (csound->oparms->realtime)
    csoundUnlockMutex(csound->init_pass_threadlock);
}

void csound_table_copy_out(CSOUND *csound, int32_t table, MYFLT *ptable) {
  int32_t len;
  MYFLT *ftab;
  /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
  if (csound->oparms->realtime)
    csoundLockMutex(csound->init_pass_threadlock);
  len = csoundGetTable(csound, &ftab, table);
  if (UNLIKELY(len > 0x00ffffff))
    len = 0x00ffffff; // As coverity is unhappy
  memcpy(ptable, ftab, (size_t)(len * sizeof(MYFLT)));
  if (csound->oparms->realtime)
    csoundUnlockMutex(csound->init_pass_threadlock);
}

void csound_table_copy_in(CSOUND *csound, int32_t table, const MYFLT *ptable) {
  int32_t len;
  MYFLT *ftab;
  /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
  if (csound->oparms->realtime)
    csoundLockMutex(csound->init_pass_threadlock);
  len = csoundGetTable(csound, &ftab, table);
  if (UNLIKELY(len > 0x00ffffff))
    len = 0x00ffffff; // As coverity is unhappy
  memcpy(ftab, ptable, (size_t)((len+1) * sizeof(MYFLT))); // + guard point
  if (csound->oparms->realtime)
    csoundUnlockMutex(csound->init_pass_threadlock);
}



