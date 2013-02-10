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

void csoundScoreline(CSOUND *csound, const char *message){
  csoundWaitThreadLockNoTimeout(csound->API_lock);
  csoundInputMessage(csound, message);
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





