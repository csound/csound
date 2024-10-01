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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "csoundCore.h"
#include "csound_orc.h"
#include <stdlib.h>

#ifdef USE_DOUBLE
#  define MYFLT_INT_TYPE int64_t
#else
#  define MYFLT_INT_TYPE int32_t
#endif

int32_t csoundKillInstanceInternal(CSOUND *csound, MYFLT instr, char *instrName,
                               int32_t mode, int32_t allow_release, int32_t async);
int32_t csoundCompileTreeInternal(CSOUND *csound, TREE *root, int32_t async);
int32_t csoundCompileOrcInternal(CSOUND *csound, const char *str, int32_t async);
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE* typetable, OPDS *ids);
void killInstance(CSOUND *csound, MYFLT instr, int32_t insno, INSDS *ip,
                  int32_t mode, int32_t allow_release);
void csoundInputMessageInternal(CSOUND *csound, const char *message);
int32_t csoundReadScoreInternal(CSOUND *csound, const char *message);
void csoundTableCopyOutInternal(CSOUND *csound, int32_t table, MYFLT *ptable);
void csoundTableCopyInInternal(CSOUND *csound, int32_t table, MYFLT *ptable);
void csoundTableSetInternal(CSOUND *csound, int32_t table, int32_t index, MYFLT value);
int32_t csoundScoreEventInternal(CSOUND *csound, char type,
                             const MYFLT *pfields, long numFields);
int32_t csoundScoreEventAbsoluteInternal(CSOUND *csound, char type,
                                     const MYFLT *pfields, long numFields,
                                     double time_ofs);
void set_channel_data_ptr(CSOUND *csound, const char *name,
                          void *ptr, int32_t newSize);

void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);

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
  volatile long oldVal, newVal;
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
          csoundInputMessageInternal(csound, str);
        }

        break;
      case READ_SCORE:
        {
          const char *str = msg->args;
          csoundReadScoreInternal(csound, str);
        }
        break;
      case SCORE_EVENT:
        {
          char type;
          MYFLT *fargs = (MYFLT *) msg->args;
          type = (char) fargs[0];
          csoundScoreEventInternal(csound, type, &fargs[2], (int32_t)
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

          csoundScoreEventAbsoluteInternal(csound, type, pfields, numFields,
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
          csoundTableCopyOutInternal(csound, table, ptable);
        }
        break;
      case TABLE_COPY_IN:
        {
          int32_t table;
          MYFLT *ptable;
          memcpy(&table, msg->args, sizeof(int32_t));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          csoundTableCopyInInternal(csound, table, ptable);
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
          csoundTableSetInternal(csound, table, index, value);
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
          killInstance(csound, instr, insno, ip, mode, rls);
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
static inline void csoundInputMessage_enqueue(CSOUND *csound,
                                              const char *str){
  message_enqueue(csound,INPUT_MESSAGE, (char *) str, (int32_t) strlen(str)+1);
}

static inline int64_t *csoundReadScore_enqueue(CSOUND *csound, const char *str){
  return message_enqueue(csound, READ_SCORE, (char *) str, (int32_t) strlen(str)+1);
}

static inline void csoundTableCopyOut_enqueue(CSOUND *csound, int32_t table,
                                              MYFLT *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_OUT, args, argsize);
}

static inline void csoundTableCopyIn_enqueue(CSOUND *csound, int32_t table,
                                             MYFLT *ptable){
  const int32_t argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_IN, args, argsize);
}

static inline void csoundTableSet_enqueue(CSOUND *csound, int32_t table, int32_t index,
                                          MYFLT value)
{
  const int32_t argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  memcpy(args, &table, sizeof(int32_t));
  memcpy(args+ARG_ALIGN, &index, sizeof(int32_t));
  memcpy(args+2*ARG_ALIGN, &value, sizeof(MYFLT));
  message_enqueue(csound,TABLE_SET, args, argsize);
}


static inline int64_t *csoundScoreEvent_enqueue(CSOUND *csound, char type,
                                                const MYFLT *pfields,
                                                long numFields)
{
  const int32_t argsize = (int32_t) (sizeof(MYFLT)*(numFields+2));
  MYFLT *args = mcalloc(csound, argsize);
  memcpy(&args[2], pfields, argsize - sizeof(MYFLT)*2);
  args[0] = (MYFLT) type;
  args[1] = numFields;
  return message_enqueue(csound, SCORE_EVENT, (char *) args,
                         argsize);
}


static inline int64_t *csoundScoreEventAbsolute_enqueue(CSOUND *csound, char type,
                                                        const MYFLT *pfields,
                                                        long numFields,
                                                        double time_ofs)
{
  const int32_t argsize = ARG_ALIGN*4;
  char args[ARG_ALIGN*4];
  args[0] = type;
  memcpy(args+ARG_ALIGN, &pfields, sizeof(MYFLT *));
  memcpy(args+2*ARG_ALIGN, &numFields, sizeof(long));
  memcpy(args+3*ARG_ALIGN, &time_ofs, sizeof(double));
  return message_enqueue(csound,SCORE_EVENT_ABS, args, argsize);
}

/* this is to be called from
   csoundKillInstanceInternal() in insert.c
*/
void killInstance_enqueue(CSOUND *csound, MYFLT instr, int32_t insno,
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
   csoundCompileTreeInternal() in csound_orc_compile.c
*/
void mergeState_enqueue(CSOUND *csound, ENGINE_STATE *e, TYPE_TABLE* t, OPDS *ids) {
  const int32_t argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  memcpy(args, &e, sizeof(ENGINE_STATE *));
  memcpy(args+ARG_ALIGN, &t, sizeof(TYPE_TABLE *));
  memcpy(args+2*ARG_ALIGN, &ids, sizeof(OPDS *));
  message_enqueue(csound,MERGE_STATE, args, argsize);
}

/*  VL: These functions are slated to
    be converted to message enqueueing
    in the next API revision.
*/
void csoundInputMessage(CSOUND *csound, const char *message){
  csoundLockMutex(csound->API_lock);
  csoundInputMessageInternal(csound, message);
  csoundUnlockMutex(csound->API_lock);
}

int32_t csoundReadScore(CSOUND *csound, const char *message){
  int32_t res;
  csoundLockMutex(csound->API_lock);
  res = csoundReadScoreInternal(csound, message);
  csoundUnlockMutex(csound->API_lock);
  return res;
}

void csoundTableSet(CSOUND *csound, int32_t table, int32_t index, MYFLT value)
{
  csoundLockMutex(csound->API_lock);
  csoundTableSetInternal(csound, table, index, value);
  csoundUnlockMutex(csound->API_lock);
}

int32_t csoundScoreEvent(CSOUND *csound, char type,
                     const MYFLT *pfields, long numFields)
{

  csoundLockMutex(csound->API_lock);
  csoundScoreEventInternal(csound, type, pfields, numFields);
  csoundUnlockMutex(csound->API_lock);
  return OK;

}

int32_t csoundScoreEventAbsolute(CSOUND *csound, char type,
                             const MYFLT *pfields, long numFields,
                             double time_ofs)
{
  csoundLockMutex(csound->API_lock);
  csoundScoreEventAbsoluteInternal(csound, type, pfields, numFields, time_ofs);
  csoundUnlockMutex(csound->API_lock);
  return OK;
}

int32_t csoundKillInstance(CSOUND *csound, MYFLT instr, char *instrName,
                       int32_t mode, int32_t allow_release){
  int32_t async = 0;
  return csoundKillInstanceInternal(csound, instr, instrName, mode,
                                    allow_release, async);
}

int32_t init0(CSOUND *csound);

MYFLT csoundEvalCode(CSOUND *csound, const char *str)
{
  int32_t async = 0;
  if (str && csoundCompileOrcInternal(csound,str,async)
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
void csoundInputMessageAsync(CSOUND *csound, const char *message){
  csoundInputMessage_enqueue(csound, message);
}

void csoundReadScoreAsync(CSOUND *csound, const char *message){
  csoundReadScore_enqueue(csound, message);
}

void csoundTableCopyOut(CSOUND *csound, int32_t table, MYFLT *ptable, int32_t async){
  if(async) {
    csoundTableCopyOut_enqueue(csound, table, ptable);
    return;
  }
  csoundLockMutex(csound->API_lock);
  csoundTableCopyOutInternal(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableCopyIn(CSOUND *csound, int32_t table, MYFLT *ptable, int32_t async){
  if(async) {
    csoundTableCopyIn_enqueue(csound, table, ptable);
    return;
  }
  csoundLockMutex(csound->API_lock);
  csoundTableCopyInInternal(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableSetAsync(CSOUND *csound, int32_t table, int32_t index, MYFLT value)
{
  csoundTableSet_enqueue(csound, table, index, value);
}

void csoundScoreEventAsync(CSOUND *csound, char type,
                           const MYFLT *pfields, long numFields)
{
  csoundScoreEvent_enqueue(csound, type, pfields, numFields);
}

void csoundScoreEventAbsoluteAsync(CSOUND *csound, char type,
                                   const MYFLT *pfields, long numFields,
                                   double time_ofs)
{

  csoundScoreEventAbsolute_enqueue(csound, type, pfields, numFields, time_ofs);
}

int32_t csoundCompileTreeAsync(CSOUND *csound, TREE *root) {
  int32_t async = 1;
  return csoundCompileTreeInternal(csound, root, async);
}

int32_t csoundCompileOrcAsync(CSOUND *csound, const char *str) {
  int32_t async = 1;
  return csoundCompileOrcInternal(csound, str, async);
}

int32_t csoundKillInstanceAsync(CSOUND *csound, MYFLT instr, char *instrName,
                            int32_t mode, int32_t allow_release){
  int32_t async = 1;
  return csoundKillInstanceInternal(csound, instr, instrName, mode,
                                    allow_release, async);
}
