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
#include <stdlib.h>

#ifdef USE_DOUBLE
#  define MYFLT_INT_TYPE int64_t
#else
#  define MYFLT_INT_TYPE int32_t
#endif

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

enum {INPUT_MESSAGE, READ_SCORE, TABLE_COPY_OUT, TABLE_COPY_IN, TABLE_SET};

#define API_MAX_QUEUE 64
/* Message queue structure */
typedef struct _message_queue {
  int32_t message;  /* message id */
  char args[128];   /* space for 128 bytes of args */
} message_queue_t;


void message_enqueue(CSOUND *csound, int32_t message, char *args, int argsiz) {
  uint32_t wp = csound->msg_queue_wp;
  if(csound->msg_queue == NULL)
    csound->msg_queue = (message_queue_t *)
      csound->Calloc(csound, sizeof(message_queue_t)*API_MAX_QUEUE);
  csound->msg_queue[wp].message = message;
  memcpy(csound->msg_queue[wp].args, args, argsiz);
  // this might need to be atomic
  csound->msg_queue_wp = wp == API_MAX_QUEUE ? wp + 1 : 0;
}

void message_dequeue(CSOUND *csound) {
  if(csound->msg_queue != NULL) {
  uint32_t rp = csound->msg_queue_rp;
  uint32_t wp = csound->msg_queue_wp;
  int table, index;
  MYFLT *ptable, value; 
  while(rp != wp) {
    switch(csound->msg_queue[rp].message) {
    case INPUT_MESSAGE:
      csoundInputMessageInternal(csound, (char *) csound->msg_queue[rp].args);
      break;
    case READ_SCORE:
      csoundReadScoreInternal(csound, (char *) csound->msg_queue[rp].args);
      break;
    case TABLE_COPY_OUT:
      table = *((int *) csound->msg_queue[rp].args);
      ptable =  *((MYFLT **)(csound->msg_queue[rp].args + sizeof(int)));
      csoundTableCopyOutInternal(csound, table, ptable);
      break;
    case TABLE_COPY_IN:
      table = *((int *) (csound->msg_queue[rp].args));
      ptable =  *((MYFLT **)(csound->msg_queue[rp].args + sizeof(int)));
      csoundTableCopyInInternal(csound, table, ptable);
      break;
     case TABLE_SET:
      table = *((int *) (csound->msg_queue[rp].args));
      index = *((int *)(csound->msg_queue[rp].args + sizeof(int)));
      value = *((MYFLT *)(csound->msg_queue[rp].args + 2*sizeof(int)));
      csoundTableSetInternal(csound, table, index, value);
      break;
    }
    rp = rp == API_MAX_QUEUE ? rp + 1 : 0;
  }
   csound->msg_queue_wp = wp;
  }
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
    if (csound->oparms->realtime) csoundLockMutex(csound->init_pass_threadlock);
      csoundTableCopyInInternal(csound, table, ptable);
    if (csound->oparms->realtime) csoundUnlockMutex(csound->init_pass_threadlock);
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
  
}

int csoundScoreEventAbsolute(CSOUND *csound, char type,
                                    const MYFLT *pfields, long numFields,
                                    double time_ofs)
{
    
    csoundLockMutex(csound->API_lock);
    csoundScoreEventAbsoluteInternal(csound, type, pfields, numFields, time_ofs);
    csoundUnlockMutex(csound->API_lock);
  
}

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
