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

TREE *csoundParseOrc_TS(CSOUND *csound, char *str) {
  TREE *ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundParseOrc(csound, str);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundCompileTree_TS(CSOUND *csound, TREE *root){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundCompileTree(csound, root);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundCompileOrc_TS(CSOUND *csound, char *str){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundCompileOrc(csound, char *str);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundReadScore_TS(CSOUND *csound, char *str){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundReadScore(csound, char *str);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

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

MYFLT csoundGetSpoutSample_TS(CSOUND *csound, 
			      int frame, int channel){
  MYFLT ret; 
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundGetSpoutSample(csound, frame, channel);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
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
  ret = csoundScoreEventAbsolute(csound, type, pFields, numFields, time_ofs);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

void csoundInputMessage_TS(CSOUND *csound, const char *message){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundInputMessage(csound, message);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

MYFLT csoundTableGet_TS(CSOUND *csound, int table, int index){
  MYFLT ret; 
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundTableGet(csound,table,index);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

void csoundTableSet_TS(CSOUND *csound, int table, int index, MYFLT value){
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  csoundTableSet(csound,tabke,index,value);
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundTableCopyOut_TS(CSOUND *csound, int table, MYFLT *ptable){
   int len;
   MYFLT *ftab;
   csoundWaitThreadLockNoTimeout(csound->API_lock);
   len = csoundGetTable(csound, &ftab, table); 
   memcpy(ptable, ftab, len*sizeof(MYFLT)); 
   csoundNotifyThreadLock(csound->API_lock);
}

void csoundTableCopyIn_TS(CSOUND *csound, int table, MYFLT *ptable){
   int len;
   MYFLT *ftab;
   csoundWaitThreadLockNoTimeout(csound->API_lock);
   len = csoundGetTable(csound, &ftab, table); 
   memcpy(ftab, ptable, len*sizeof(MYFLT)); 
   csoundNotifyThreadLock(csound->API_lock);
}

MYFLT csoundGetControlChannel_TS(CSOUND *csound, char *channel){
  MYFLT val=FL(0.0), *pval;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL))
    val = *pval;
  csoundNotifyThreadLock(csound->API_lock);
  return val;
}

void csoundSetControlChannel_TS(CSOUND *csound, char *channel, MYFLT val){
  MYFLT *pval;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL))
    *pval = val;
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundGetAudioChannel_TS(CSOUND *csound, char *channel, MYFLT *samples){
  MYFLT  *psamples;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &psamples, name, 
			 CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL))
  memcpy(samples, psamples, csoundGetKsmps(csound)*sizeof(MYFLT));
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundSetAudioChannel_TS(CSOUND *csound, char *channel, MYFLT *samples){
  MYFLT  *psamples;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL))
  memcpy(psamples, samples, csoundGetKsmps(csound)*sizeof(MYFLT));
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundSetStringChannel_TS(CSOUND *csound, char *channel, MYFLT *string){
  MYFLT  *pstring;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL))
  strcpy(pstring, string);
  csoundNotifyThreadLock(csound->API_lock);
}

void csoundSetStringChannel_TS(CSOUND *csound, char *channel, MYFLT *string){
  MYFLT  *pstring;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  if(csoundGetChannelPtr(csound, &pval, name, 
			 CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL))
  strcpy(string, pstring);
  csoundNotifyThreadLock(csound->API_lock);
}

int csoundPvsinSet_TS(CSOUND *, const PVSDATEXT *fin, int n){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundPvsinSet(csound, fin, n);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}

int csoundPvsoutGet_TS(CSOUND *csound, PVSDATEXT *fout, int n){
  int ret;
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  ret = csoundPvsoutGet(csound, fin, n);
  csoundNotifyThreadLock(csound->API_lock);
  return ret;
}



