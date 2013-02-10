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


int csoundPerformKsmps_TS(CSOUND *csound) {
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundPerformKsmps(csound);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundPerformBuffer_TS(CSOUND *csound) {
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundPerformBuffer(csound);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

void csoundAddSpinSample_TS(CSOUND *csound,
			    int frame, int channel, MYFLT sample){
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  csoundAddSpinSample(csound, frame, channel, sample);
  csoundNotifyThreadLock(csound->API_lock);
}

int csoundScoreEvent_TS(CSOUND *csound,
			    char type, const MYFLT *pFields, long numFields) {
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundScoreEvent(csound, type, pFields, numFields);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundScoreEventAbsolute_TS(CSOUND *csound,
			     char type, const MYFLT *pfields, long numFields, double time_ofs){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundScoreEventAbsolute(csound, type, pfields, numFields, time_ofs);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

void csoundInputMessage_TS(CSOUND *csound, const char *message){
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  csoundInputMessage(csound, message);
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundTableSet_TS(CSOUND *csound, int table, int index, MYFLT value){
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
  if(csound->oparms->realtime) csound->WaitThreadLockNoTimeout(csound->init_pass_threadlock);
  csoundTableSet(csound,table,index,value);
   if(csound->oparms->realtime) csound->NotifyThreadLock(csound->init_pass_threadlock);
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundTableCopyOut(CSOUND *csound, int table, MYFLT *ptable){
   int len;
   MYFLT *ftab;
   len = csoundGetTable(csound, &ftab, table); 
   memcpy(ptable, ftab, len*sizeof(MYFLT)); 
}

void csoundTableCopyIn(CSOUND *csound, int table, MYFLT *ptable){
   int len;
   MYFLT *ftab;
   csoundWaitThreadLockNoTimeout(csound->API_lock);
   /* in realtime mode init pass is executed in a separate thread, so
     we need to protect it */
  if(csound->oparms->realtime) csound->WaitThreadLockNoTimeout(csound->init_pass_threadlock);
   len = csoundGetTable(csound, &ftab, table); 
   memcpy(ftab, ptable, len*sizeof(MYFLT)); 
  if(csound->oparms->realtime) csound->NotifyThreadLock(csound->init_pass_threadlock);
   csoundNotifyThreadLock(csound->API_lock);
}

MYFLT csoundGetControlChannel(CSOUND *csound, char *name){
  MYFLT val=FL(0.0), *pval;
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL))
    val = *pval;
  return val;
}

void csoundSetControlChannel(CSOUND *csound, char *name, MYFLT val){
  MYFLT *pval;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL))
    *pval = val;
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundGetAudioChannel(CSOUND *csound, char *name, MYFLT *samples){
  MYFLT  *psamples;
  if(csoundGetChannelPtr(csound, &psamples, name, 
			 CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL))
  memcpy(samples, psamples, csoundGetKsmps(csound)*sizeof(MYFLT));
}

void csoundSetAudioChannel(CSOUND *csound, char *name, MYFLT *samples){
  MYFLT  *psamples;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &psamples, name, 
			 CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL))
  memcpy(psamples, samples, csoundGetKsmps(csound)*sizeof(MYFLT));
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundSetStringChannel(CSOUND *csound, char *name, char *string){
  MYFLT  *pstring;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pstring, name, 
			 CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL))
    strcpy((char *) pstring, string);
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundGetStringChannel(CSOUND *csound, char *name, char *string){
  MYFLT  *pstring;
  if(csoundGetChannelPtr(csound, &pstring, name, 
			 CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL))
    strcpy(string, (char *) pstring);
}

int csoundPvsinSet_TS(CSOUND *csound, const PVSDATEXT *fin, int n){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundPvsinSet(csound, fin, n);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}




