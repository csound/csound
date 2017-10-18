/*
 * threadsafe.c: threadsafe API functions
 *               (c) V Lazzarini, 2013
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "csoundCore.h"
#include "csound_orc.h"
#include <stdlib.h>

#ifdef USE_DOUBLE
#  define MYFLT_INT_TYPE int64_t
#else
#  define MYFLT_INT_TYPE int32_t
#endif

int csoundKillInstanceInternal(CSOUND *csound, MYFLT instr, char *instrName,
                               int mode, int allow_release, int async);
int csoundCompileTreeInternal(CSOUND *csound, TREE *root, int async);
int csoundCompileOrcInternal(CSOUND *csound, const char *str, int async);
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE* typetable, OPDS *ids);
void killInstance(CSOUND *csound, MYFLT instr, int insno, INSDS *ip,
                  int mode, int allow_release);
void csoundInputMessageInternal(CSOUND *csound, const char *message);
int csoundReadScoreInternal(CSOUND *csound, const char *message);
void csoundTableCopyOutInternal(CSOUND *csound, int table, MYFLT *ptable);
void csoundTableCopyInInternal(CSOUND *csound, int table, MYFLT *ptable);
void csoundTableSetInternal(CSOUND *csound, int table, int index, MYFLT value);
int csoundScoreEventInternal(CSOUND *csound, char type,
                             const MYFLT *pfields, long numFields);
int csoundScoreEventAbsoluteInternal(CSOUND *csound, char type,
                                     const MYFLT *pfields, long numFields,
                                     double time_ofs);
void set_channel_data_ptr(CSOUND *csound, const char *name,
                          void *ptr, int newSize);

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

/*
static inline int check_dequeued(CSOUND *csound) {
#ifdef HAVE_ATOMIC_BUILTIN
  return __atomic_load_n(&csound->msg_queue_flag, __ATOMIC_SEQ_CST);
#else
  return csound->msg_queue_flag;
#endif
}

static inline int get_rtn_code(int64_t *rtn) {
#ifdef HAVE_ATOMIC_BUILTIN
  return (int) __atomic_load_n(rtn, __ATOMIC_SEQ_CST);
#else
  return (int) *rtn;
#endif
}
*/

static long atomicGetAndIncrementWithModulus(volatile long* val, long mod) {

    volatile long oldVal, newVal; 
    do {
        oldVal = *val;
        newVal = (oldVal + 1) % mod;
#ifdef HAVE_ATOMIC_BUILTIN
    } while (!__atomic_compare_exchange(val, &oldVal, &newVal, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
#else
    } while (InterlockedCompareExchange(val, newVal, oldVal) != oldVal);
#endif

    return oldVal;
}

/* enqueue should be called by the relevant API function */
void *message_enqueue(CSOUND *csound, int32_t message, char *args, int argsiz) {
  int64_t *rtn;
  volatile long items;
  int i;

  // FIXME - this is not thread safe for multiple writer threads calling enqueue
  if (csound->msg_queue == NULL) {
      csound->msg_queue = (message_queue_t **) csound->Calloc(csound, sizeof(message_queue_t*)*API_MAX_QUEUE);
      for (i = 0; i < API_MAX_QUEUE; i++) {
          csound->msg_queue[i] = (message_queue_t*)csound->Calloc(csound, sizeof(message_queue_t));
      }
  }
  
  /* block if queue is full */
  do {
#ifdef HAVE_ATOMIC_BUILTIN
    items = __atomic_load_n (&csound->msg_queue_items, __ATOMIC_SEQ_CST);
#else
    items = csound->msg_queue_items;
#endif
  } while(items >= API_MAX_QUEUE);

  message_queue_t* msg = csound->msg_queue[atomicGetAndIncrementWithModulus(&csound->msg_queue_wget, API_MAX_QUEUE)];
  msg->message = message;
  if(msg->args != NULL)
    csound->Free(csound, msg->args);
  msg->args = (char *)csound->Calloc(csound, argsiz);
  memcpy(msg->args, args, argsiz);
  rtn = &msg->rtn;
  csound->msg_queue[atomicGetAndIncrementWithModulus(&csound->msg_queue_wput, API_MAX_QUEUE)] = msg;
  atomicGetAndIncrementWithModulus(&csound->msg_queue_rend, API_MAX_QUEUE);

#ifdef MSVC
  InterlockedIncrement(&csound->msg_queue_items);
#elif defined(HAVE_ATOMIC_BUILTIN)
  __atomic_add_fetch(&csound->msg_queue_items, 1, __ATOMIC_SEQ_CST);
#else
  csound->msg_queue_items++;
#endif
//  csound->msg_queue_wp = wp != API_MAX_QUEUE ? wp + 1 : 0;
  return (void *) rtn;
}

/* dequeue should be called by kperf_*() 
   NB: these calls are already in place
*/
void message_dequeue(CSOUND *csound) {
  if(csound->msg_queue != NULL) {
      long rp = csound->msg_queue_rstart;
      long rend = csound->msg_queue_rend;

      int64_t rtn = 0;
      long items;

      if (rend < rp) rend = rp + API_MAX_QUEUE;

#ifdef HAVE_ATOMIC_BUILTIN
    items = __atomic_load_n (&csound->msg_queue_items, __ATOMIC_SEQ_CST);
 #else
    items = csound->msg_queue_items;
#endif

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
	  rtn = csoundReadScoreInternal(csound, str);
	}
	break;
      case SCORE_EVENT:
        {
          char type;
          const MYFLT *pfields;
	  long numFields;
	  type = msg->args[0];
	  memcpy(&pfields, msg->args + ARG_ALIGN,
		 sizeof(MYFLT *));
	  memcpy(&numFields, msg->args + ARG_ALIGN*2,
		 sizeof(long));
	  rtn =
	    csoundScoreEventInternal(csound, type, pfields, numFields);
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
	  rtn =
	    csoundScoreEventAbsoluteInternal(csound, type, pfields, numFields,
					     ofs);
	}
	break;
      case TABLE_COPY_OUT:
        {
          int table;
          MYFLT *ptable;
          memcpy(&table, msg->args, sizeof(int));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          csoundTableCopyOutInternal(csound, table, ptable);
        }
        break;
      case TABLE_COPY_IN:
        {
          int table;
          MYFLT *ptable;
          memcpy(&table, msg->args, sizeof(int));
          memcpy(&ptable, msg->args + ARG_ALIGN,
                 sizeof(MYFLT *));
          csoundTableCopyInInternal(csound, table, ptable);
        }
        break;
      case TABLE_SET:
        {
          int table, index;
          MYFLT value;
          memcpy(&table, msg->args, sizeof(int));
          memcpy(&index, msg->args + ARG_ALIGN,
                 sizeof(int));
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
 	  merge_state(csound, e, t, ids);
	}
	break;
      case KILL_INSTANCE:
        {
          MYFLT instr;
          int mode, insno, rls;
          INSDS *ip;
          memcpy(&instr, msg->args, sizeof(MYFLT));
          memcpy(&insno, msg->args + ARG_ALIGN,
                 sizeof(int));
          memcpy(&ip, msg->args + ARG_ALIGN*2,
                 sizeof(INSDS *));
          memcpy(&mode, msg->args + ARG_ALIGN*3,
                 sizeof(int));
          memcpy(&rls, msg->args  + ARG_ALIGN*4,
                 sizeof(int));
          killInstance(csound, instr, insno, ip, mode, rls);
        }
        break;  
      }
      msg->message = 0;
      rp += 1;

#ifdef MSVC
  InterlockedDecrement(&csound->msg_queue_items);
#elif defined(HAVE_ATOMIC_BUILTIN)
      items = __atomic_sub_fetch(&csound->msg_queue_items, 1,
      __ATOMIC_SEQ_CST);
#else
      items = --csound->msg_queue_items;
#endif      
    }
    csound->msg_queue_rstart = rp % API_MAX_QUEUE;
  }
  

}

/* these are the message enqueueing functions for each relevant API function */
static inline void csoundInputMessage_enqueue(CSOUND *csound,
                                              const char *str){
  message_enqueue(csound,INPUT_MESSAGE, (char *) str, strlen(str)+1);
}

static inline int64_t *csoundReadScore_enqueue(CSOUND *csound, const char *str){
  return message_enqueue(csound, READ_SCORE, (char *) str, strlen(str)+1);
}

static inline void csoundTableCopyOut_enqueue(CSOUND *csound, int table,
                                              MYFLT *ptable){
  const int argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_OUT, args, argsize);
}

static inline void csoundTableCopyIn_enqueue(CSOUND *csound, int table,
                                             MYFLT *ptable){
  const int argsize = ARG_ALIGN*2;
  char args[ARG_ALIGN*2];
  memcpy(args, &table, sizeof(int));
  memcpy(args+ARG_ALIGN, &ptable, sizeof(MYFLT *));
  message_enqueue(csound,TABLE_COPY_IN, args, argsize);
}

static inline void csoundTableSet_enqueue(CSOUND *csound, int table, int index,
                                          MYFLT value)
{
  const int argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  memcpy(args, &table, sizeof(int));
  memcpy(args+ARG_ALIGN, &index, sizeof(int));
  memcpy(args+2*ARG_ALIGN, &value, sizeof(MYFLT));
  message_enqueue(csound,TABLE_SET, args, argsize);
}


static inline int64_t *csoundScoreEvent_enqueue(CSOUND *csound, char type,
						const MYFLT *pfields, long numFields)
{
  const int argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  args[0] = type;
  memcpy(args+ARG_ALIGN, &pfields, sizeof(MYFLT *));
  memcpy(args+2*ARG_ALIGN, &numFields, sizeof(long));
  return message_enqueue(csound,SCORE_EVENT, args, argsize);
}


static inline int64_t *csoundScoreEventAbsolute_enqueue(CSOUND *csound, char type,
							const MYFLT *pfields,
							long numFields,
							double time_ofs)
{
  const int argsize = ARG_ALIGN*4;
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
void killInstance_enqueue(CSOUND *csound, MYFLT instr, int insno,
                          INSDS *ip, int mode,
                          int allow_release) {
  const int argsize = ARG_ALIGN*5;
  char args[ARG_ALIGN*5];
  memcpy(args, &instr, sizeof(int));
  memcpy(args+ARG_ALIGN, &insno, sizeof(int));
  memcpy(args+ARG_ALIGN*2, &ip, sizeof(INSDS *));
  memcpy(args+ARG_ALIGN*3, &mode, sizeof(int));
  memcpy(args+ARG_ALIGN*4, &allow_release, sizeof(int));
  message_enqueue(csound,KILL_INSTANCE,args,argsize); 
}

/* this is to be called from 
   csoundCompileTreeInternal() in csound_orc_compile.c
*/
void mergeState_enqueue(CSOUND *csound, ENGINE_STATE *e, TYPE_TABLE* t, OPDS *ids) {
  const int argsize = ARG_ALIGN*3;
  char args[ARG_ALIGN*3];
  memcpy(args, &e, sizeof(ENGINE_STATE *));
  memcpy(args+ARG_ALIGN, &t, sizeof(TYPE_TABLE *));
  memcpy(args+2*ARG_ALIGN, &ids, sizeof(OPDS *));
  message_enqueue(csound,MERGE_STATE, args, argsize);
}

/*  VL: These functions are slated to
    be converted to message enqueueing
*/ 
void csoundInputMessage(CSOUND *csound, const char *message){
  csoundLockMutex(csound->API_lock);
  csoundInputMessageInternal(csound, message);
  csoundUnlockMutex(csound->API_lock);
}

int csoundReadScore(CSOUND *csound, const char *message){
  int res;
  csoundLockMutex(csound->API_lock);
  res = csoundReadScoreInternal(csound, message);
  csoundUnlockMutex(csound->API_lock);
  return res;
}

void csoundTableCopyOut(CSOUND *csound, int table, MYFLT *ptable){

  csoundLockMutex(csound->API_lock);
  csoundTableCopyOutInternal(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableCopyIn(CSOUND *csound, int table, MYFLT *ptable){
  csoundLockMutex(csound->API_lock);
  csoundTableCopyInInternal(csound, table, ptable);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableSet(CSOUND *csound, int table, int index, MYFLT value)
{
  csoundLockMutex(csound->API_lock);
  csoundTableSetInternal(csound, table, index, value);
  csoundUnlockMutex(csound->API_lock);
}

int csoundScoreEvent(CSOUND *csound, char type,
                     const MYFLT *pfields, long numFields)
{
    
  csoundLockMutex(csound->API_lock);
  csoundScoreEventInternal(csound, type, pfields, numFields);
  csoundUnlockMutex(csound->API_lock);
  return OK;
  
}

int csoundScoreEventAbsolute(CSOUND *csound, char type,
                             const MYFLT *pfields, long numFields,
                             double time_ofs)
{
  csoundLockMutex(csound->API_lock);
  csoundScoreEventAbsoluteInternal(csound, type, pfields, numFields, time_ofs);
  csoundUnlockMutex(csound->API_lock);
  return OK;
}

int csoundKillInstance(CSOUND *csound, MYFLT instr, char *instrName,
                       int mode, int allow_release){
  int async = 0;
  return csoundKillInstanceInternal(csound, instr, instrName, mode, allow_release, async);
}

int csoundCompileTree(CSOUND *csound, TREE *root) {
  int async = 0;
  return csoundCompileTreeInternal(csound, root, async);
}

int csoundCompileOrc(CSOUND *csound, const char *str) {
  int async = 0;
  return csoundCompileOrcInternal(csound, str, async);
}

MYFLT csoundEvalCode(CSOUND *csound, const char *str)
{
  int async = 0;
  if (str && csoundCompileOrcInternal(csound,str,async) == CSOUND_SUCCESS)
    return csound->instr0->instance[0].retval;
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
  /* 
   while(!check_dequeued(csound)) csoundSleep(1);
     return get_rtn_code(rtn);
  */
}

void csoundTableCopyOutAsync(CSOUND *csound, int table, MYFLT *ptable){
  csoundTableCopyOut_enqueue(csound, table, ptable);
}

void csoundTableCopyInAsync(CSOUND *csound, int table, MYFLT *ptable){
  csoundTableCopyIn_enqueue(csound, table, ptable);
}

void csoundTableSetAsync(CSOUND *csound, int table, int index, MYFLT value)
{
  csoundTableSet_enqueue(csound, table, index, value);
}

void csoundScoreEventAsync(CSOUND *csound, char type,
			   const MYFLT *pfields, long numFields)
{
  csoundScoreEvent_enqueue(csound, type, pfields, numFields);
  /*
  while(!check_dequeued(csound)) csoundSleep(1);
  return get_rtn_code(rtn);
  */
}

void csoundScoreEventAbsoluteAsync(CSOUND *csound, char type,
				   const MYFLT *pfields, long numFields,
				   double time_ofs)
{
  
  csoundScoreEventAbsolute_enqueue(csound, type, pfields, numFields, time_ofs);
  /*
  while(!check_dequeued(csound)) csoundSleep(1);
  return get_rtn_code(rtn);
  */
}

int csoundCompileTreeAsync(CSOUND *csound, TREE *root) {
  int async = 1;
  return csoundCompileTreeInternal(csound, root, async);
}

int csoundCompileOrcAsync(CSOUND *csound, const char *str) {
  int async = 1;
  return csoundCompileOrcInternal(csound, str, async);
}

int csoundKillInstanceAsync(CSOUND *csound, MYFLT instr, char *instrName,
                            int mode, int allow_release){
  int async = 1;
  return csoundKillInstanceInternal(csound, instr, instrName, mode, allow_release, async);
}

/*
MYFLT csoundEvalCodeAsync(CSOUND *csound, const char *str)
{
  int async = 1;
  if (str && csoundCompileOrcInternal(csound,str,async) == CSOUND_SUCCESS) {
    while(check_dequeued(csound))
      csoundSleep(1);
    return csound->instr0->instance[0].retval;
  }      
#ifdef NAN
  else return NAN;
#else
  else return 0;
#endif
}
*/

/* VL: the following do not depend on API_lock
   therefore do not need to be in the message queue
*/

MYFLT csoundGetControlChannel(CSOUND *csound, const char *name, int *err)
{
  MYFLT *pval;
  int err_;
  union {
    MYFLT d;
    MYFLT_INT_TYPE i;
  } x;
  x.d = FL(0.0);
  if (UNLIKELY(strlen(name) == 0)) return FL(.0);
  if ((err_ = csoundGetChannelPtr(csound, &pval, name,
                                  CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL))
      == CSOUND_SUCCESS) {
#ifdef HAVE_ATOMIC_BUILTIN
    x.i = __sync_fetch_and_add((MYFLT_INT_TYPE *)pval, 0);
#else
    x.d = *pval;
#endif
  }
  if (err) {
    *err = err_;
  }
  return x.d;
}

void csoundSetControlChannel(CSOUND *csound, const char *name, MYFLT val){
  MYFLT *pval;
  union {
    MYFLT d;
    MYFLT_INT_TYPE i;
  } x;
  x.d = val;
  if (csoundGetChannelPtr(csound, &pval, name,
                          CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS)
#ifdef HAVE_ATOMIC_BUILTIN
    __sync_lock_test_and_set((MYFLT_INT_TYPE *)pval,x.i);
#else
  {
    int    *lock =
      csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    *pval  = val;
    csoundSpinUnLock(lock);
  }
#endif
}

void csoundGetAudioChannel(CSOUND *csound, const char *name, MYFLT *samples)
{

  MYFLT  *psamples;
  if (strlen(name) == 0) return;
  if (csoundGetChannelPtr(csound, &psamples, name,
                          CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL)
      == CSOUND_SUCCESS) {
    int *lock = csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    memcpy(samples, psamples, csoundGetKsmps(csound)*sizeof(MYFLT));
    csoundSpinUnLock(lock);
  }
}

void csoundSetAudioChannel(CSOUND *csound, const char *name, MYFLT *samples)
{
  MYFLT  *psamples;
  if (csoundGetChannelPtr(csound, &psamples, name,
                          CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS){
    int *lock = csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    memcpy(psamples, samples, csoundGetKsmps(csound)*sizeof(MYFLT));
    csoundSpinUnLock(lock);
  }
}

void csoundSetStringChannel(CSOUND *csound, const char *name, char *string)
{
  MYFLT  *pstring;

  if (csoundGetChannelPtr(csound, &pstring, name,
                          CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS){

    STRINGDAT* stringdat = (STRINGDAT*) pstring;
    int    size = stringdat->size; //csoundGetChannelDatasize(csound, name);
    int    *lock = csoundGetChannelLock(csound, (char*) name);

    if (lock != NULL) {
      csoundSpinLock(lock);
    }

    if (strlen(string) + 1 > (unsigned int) size) {
      if (stringdat->data!=NULL) csound->Free(csound,stringdat->data);
      stringdat->data = cs_strdup(csound, string);
      stringdat->size = strlen(string) + 1;
      //set_channel_data_ptr(csound,name,(void*)pstring, strlen(string)+1);
    } else {
      strcpy((char *) stringdat->data, string);
    }

    if (lock != NULL) {
      csoundSpinUnLock(lock);
    }
  }
}

void csoundGetStringChannel(CSOUND *csound, const char *name, char *string)
{
  MYFLT  *pstring;
  char *chstring;
  int n2;
  if (strlen(name) == 0) return;
  if (csoundGetChannelPtr(csound, &pstring, name,
                          CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)
      == CSOUND_SUCCESS){
    int *lock = csoundGetChannelLock(csound, (char*) name);
    chstring = ((STRINGDAT *) pstring)->data;
    if (lock != NULL)
      csoundSpinLock(lock);
    if (string != NULL && chstring != NULL) {
      n2 = strlen(chstring);
      strncpy(string,chstring, n2);
      string[n2] = 0;
    }
    if (lock != NULL)
      csoundSpinUnLock(lock);
  }
}

PUBLIC int csoundSetPvsChannel(CSOUND *csound, const PVSDATEXT *fin,
                               const char *name)
{
  MYFLT *pp;
  PVSDATEXT *f;
  if (LIKELY(csoundGetChannelPtr(csound, &pp, name,
                                 CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL)
             == CSOUND_SUCCESS)){
    int    *lock =
      csoundGetChannelLock(csound, name);
    f = (PVSDATEXT *) pp;
    csoundSpinLock(lock);


    if (f->frame == NULL) {
      f->frame = csound->Calloc(csound, sizeof(float)*(fin->N+2));
    } else if (f->N < fin->N) {
      f->frame = csound->ReAlloc(csound, f->frame, sizeof(float)*(fin->N+2));
    }

    memcpy(f, fin, sizeof(PVSDATEXT)-sizeof(float *));
    if (fin->frame != NULL)
      memcpy(f->frame, fin->frame, (f->N+2)*sizeof(float));
    csoundSpinUnLock(lock);
  } else {
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

PUBLIC int csoundGetPvsChannel(CSOUND *csound, PVSDATEXT *fout,
                               const char *name)
{
  MYFLT *pp;
  PVSDATEXT *f;
  if (UNLIKELY(csoundGetChannelPtr(csound, &pp, name,
                                   CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL)
               == CSOUND_SUCCESS)){
    int    *lock =
      csoundGetChannelLock(csound, name);
    f = (PVSDATEXT *) pp;
    if (UNLIKELY(pp == NULL)) return CSOUND_ERROR;
    csoundSpinLock(lock);
    memcpy(fout, f, sizeof(PVSDATEXT)-sizeof(float *));
    if (fout->frame != NULL && f->frame != NULL)
      memcpy(fout->frame, f->frame, sizeof(float)*(fout->N));
    csoundSpinUnLock(lock);
  } else {
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}
