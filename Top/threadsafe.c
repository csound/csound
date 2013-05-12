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

extern void csoundInputMessageInternal(CSOUND *csound, const char *message);

void csoundInputMessage(CSOUND *csound, const char *message){
  csoundLockMutex(csound->API_lock);
  csoundInputMessageInternal(csound, message);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableCopyOut(CSOUND *csound, int table, MYFLT *ptable){
  int len;
  MYFLT *ftab;

  csoundLockMutex(csound->API_lock);
  /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
  if(csound->oparms->realtime) csoundLockMutex(csound->init_pass_threadlock);
  len = csoundGetTable(csound, &ftab, table);
  memcpy(ptable, ftab, len*sizeof(MYFLT));
  if(csound->oparms->realtime) csoundUnlockMutex(csound->init_pass_threadlock);
  csoundUnlockMutex(csound->API_lock);
}

void csoundTableCopyIn(CSOUND *csound, int table, MYFLT *ptable){
  int len;
  MYFLT *ftab;
 csoundLockMutex(csound->API_lock);
  /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
 if(csound->oparms->realtime) csoundLockMutex(csound->init_pass_threadlock);
  len = csoundGetTable(csound, &ftab, table);
  memcpy(ftab, ptable, len*sizeof(MYFLT));
  if(csound->oparms->realtime) csoundUnlockMutex(csound->init_pass_threadlock);
  csoundUnlockMutex(csound->API_lock);
}

MYFLT csoundGetControlChannel(CSOUND *csound, const char *name, int *err)
{
    MYFLT *pval;
    int err_;
    union {
      MYFLT d;
      int64_t i;
    } x;
    x.d = FL(0.0);
    if ((err_ = csoundGetChannelPtr(csound, &pval, name,
                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL)
         == CSOUND_SUCCESS)) {
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
      int64_t i;
    } x;
    x.d = val;
    if(csoundGetChannelPtr(csound, &pval, name,
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
      int    size = csoundGetChannelDatasize(csound, name);
      int    *lock = csoundGetChannelLock(csound, (char*) name);
      csoundSpinLock(lock);
      //if(strlen(string) < size)
        strcpy((char *) pstring, string);
      //pstring[size-1] = '\0';
      csoundSpinUnLock(lock);
    }
}

void csoundGetStringChannel(CSOUND *csound, const char *name, char *string)
{
    MYFLT  *pstring;
    if (csoundGetChannelPtr(csound, &pstring, name,
                           CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)
            == CSOUND_SUCCESS){
      int *lock = csoundGetChannelLock(csound, (char*) name);
      csoundSpinLock(lock);
      strcpy(string, (char *) pstring);
      csoundSpinUnLock(lock);
    }
}

PUBLIC int csoundSetPvsChannel(CSOUND *csound, const PVSDATEXT *fin,
                               const char *name)
{
    MYFLT *f;
    if (csoundGetChannelPtr(csound, &f, name,
                           CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL)
            == CSOUND_SUCCESS){
        int    *lock =
                csoundGetChannelLock(csound, name);
        csoundSpinLock(lock);
        memcpy(f, fin, sizeof(PVSDATEXT));
        csoundSpinUnLock(lock);
    } else {
        return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;
}

PUBLIC int csoundGetPvsChannel(CSOUND *csound, PVSDATEXT *fout,
                               const char *name)
{
    MYFLT *f;
    if (csoundGetChannelPtr(csound, &f, name,
                           CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL)
            == CSOUND_SUCCESS){
      int    *lock =
      csoundGetChannelLock(csound, name);
      csoundSpinLock(lock);
      memcpy(fout, f, sizeof(PVSDATEXT));
      csoundSpinUnLock(lock);
    } else {
        return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;
}

