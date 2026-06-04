/*
   rtaudio.c
   AAudio module for Csound

   Copyright (C) 2025 Victor Lazzarini.

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
#include <aaudio/AAudio.h>
#include "csoundCore.h"
#include <android/log.h>
#include <stdint.h>
#include <time.h>

#ifdef USE_DOUBLE
#error "REQUIRES FLOATS BUILD"
#endif

typedef struct  {
  AAudioStream *stream;
  int32_t nchnls;
  CSOUND *csound;
  void *incb;
  int32_t cnt;
} AAUDIO_PARAMS;

static void audio_output(CSOUND *csound,
                          const MYFLT *outbuff, int32_t nbytes) {
  // nothing to do
}

int32_t csoundPerformKsmps(CSOUND *csound);

static aaudio_data_callback_result_t
   input_callback(AAudioStream *stream,void *userData, void *audioData,
                  int32_t numFrames) {
   AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) userData;
   CSOUND *csound = cdata->csound;
   float *samples = (float *) audioData;
   int32_t chns = cdata->nchnls;
   // check for pause flag
   if(*(int *)csoundQueryGlobalVariable(csound,"::paused::"))
     return AAUDIO_CALLBACK_RESULT_CONTINUE;
     
   csound->WriteCircularBuffer(csound,cdata->incb,samples,numFrames*chns);
   return AAUDIO_CALLBACK_RESULT_CONTINUE;
}


static aaudio_data_callback_result_t
   output_callback(AAudioStream *stream,void *userData, void *audioData,
                    int32_t numFrames) {

  AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) userData;
  float *samples = (float *) audioData;
  CSOUND *csound = cdata->csound;
  int32_t ksmps = csoundGetKsmps(csound), res;
  int32_t chns = cdata->nchnls, n = cdata->cnt;
  const MYFLT *bufo = csoundGetSpout(csound);
  MYFLT *bufi = csoundGetSpin(csound);
  memset(samples, 0, numFrames*chns*sizeof(float));
  // check for pause flag
  if(*(int *)csoundQueryGlobalVariable(csound,"::paused::"))
     return AAUDIO_CALLBACK_RESULT_CONTINUE;
  for(int i = 0;  i < numFrames; i++, n++){
    if(n == ksmps){
      int32_t nsmps = n*csound->GetNchnls_i(csound);
      memset(bufi, 0, nsmps*sizeof(MYFLT));
      if(cdata->incb != NULL) {
       csound->ReadCircularBuffer(csound,cdata->incb,
                                  bufi,nsmps);
      }
      res = csoundPerformKsmps(csound);
      if(res != 0) return AAUDIO_CALLBACK_RESULT_STOP;
      n = 0;
    }
    memcpy(samples+i*chns, bufo+n*chns,
           sizeof(float)*chns);
  }  
  cdata->cnt = n;
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static int32_t open_out(CSOUND *csound, const csRtAudioParams *parm) {
  AAudioStreamBuilder *builder;
  aaudio_result_t result = AAudio_createStreamBuilder(&builder);
  if(result == AAUDIO_OK) {
    int32 dev = parm->devName ? atoi(parm->devName) : AAUDIO_UNSPECIFIED;
    void **data = csound->GetRtPlayUserData(csound);
    AAUDIO_PARAMS *cdata, *cdata_in;
    cdata = (AAUDIO_PARAMS*)
      csound->Calloc(csound, sizeof(AAUDIO_PARAMS));
    *data = cdata;

    // check to see if input audio is open
    // NB: this is always opened before output
    cdata_in = *csound->GetRtRecordUserData(csound);
    // retrieve the circular buffer
    if(cdata_in) 
       cdata->incb = cdata_in->incb;
   
    AAudioStreamBuilder_setDeviceId(builder, dev);  
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setSampleRate(builder, parm->sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, parm->nChannels);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, parm->bufSamp_HW);
    AAudioStreamBuilder_setDataCallback(builder, output_callback, cdata);

    AAudioStream *stream;
    result = AAudioStreamBuilder_openStream(builder, &stream);
    AAudioStreamBuilder_delete(builder);
    if(result == AAUDIO_OK) {
      cdata->nchnls = parm->nChannels;
      cdata->stream = stream;
      cdata->csound = csound;
      cdata->cnt = 0;
      aaudio_stream_state_t state = AAudioStream_getState(cdata->stream);
      if (state == AAUDIO_STREAM_STATE_OPEN) {
        AAudioStream_requestStart(cdata->stream);
        csound->Message(csound, "AAUDIO output opened\n");
        return OK;
      }
     }
    csound->Free(csound, cdata);
    *data = NULL;
  }
  csound->Message(csound, "AAUDIO output open failed\n");
  return CSOUND_ERROR;
}

static int32_t open_in(CSOUND *csound, const csRtAudioParams *parm) {
   AAudioStreamBuilder *builder;
   aaudio_result_t result = AAudio_createStreamBuilder(&builder);
   if(result == AAUDIO_OK) {
     int32 dev = parm->devName ? atoi(parm->devName) : AAUDIO_UNSPECIFIED;
     void **data = csound->GetRtRecordUserData(csound);
     AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS*)
       csound->Calloc(csound, sizeof(AAUDIO_PARAMS));
     *data = cdata;
     
     AAudioStreamBuilder_setDeviceId(builder, dev);  
     AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
     AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
     AAudioStreamBuilder_setSampleRate(builder, parm->sampleRate);
     AAudioStreamBuilder_setChannelCount(builder, parm->nChannels);
     AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
     AAudioStreamBuilder_setPerformanceMode(builder,
                                            AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
     AAudioStreamBuilder_setBufferCapacityInFrames(builder, parm->bufSamp_HW);
     AAudioStreamBuilder_setDataCallback(builder, input_callback, cdata);
   
     AAudioStream *stream;
     result = AAudioStreamBuilder_openStream(builder, &stream);
     AAudioStreamBuilder_delete(builder);
     if(result == AAUDIO_OK) {
       cdata->nchnls = parm->nChannels;
       cdata->stream = stream;
       cdata->csound = csound;
       aaudio_stream_state_t state = AAudioStream_getState(cdata->stream);
       if (state == AAUDIO_STREAM_STATE_OPEN) {
         cdata->incb =
           csound->CreateCircularBuffer(csound,
                                    parm->bufSamp_HW*parm->nChannels,
                                    sizeof(float));
         AAudioStream_requestStart(cdata->stream);
         csound->Message(csound, "AAUDIO input opened\n");
         return OK;
       }
     }
     csound->Free(csound, cdata);
     *data = NULL;  
   }
   csound->Message(csound, "AAUDIO input open failed\n");
   return CSOUND_ERROR;
}


static int32_t audio_input(CSOUND *csound, MYFLT *inbuff, int32_t nbytes){
    // nothing to do but signal the caller not to fill spin
    return -1;
}


static void close_io(CSOUND *csound) {
  const int64_t timeout = 50 * 1000 * 1000;
  aaudio_stream_state_t inputState;
  aaudio_stream_state_t nextState;
  AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *)
    *(csound->GetRtRecordUserData(csound));
  if(cdata && cdata->stream) {
    inputState = AAudioStream_getState(cdata->stream);
    nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    AAudioStream_requestStop(cdata->stream);
    csound->Message(csound, "requested AAudio input stop\n");
    AAudioStream_waitForStateChange(cdata->stream, 
                                inputState, 
                                &nextState, 
                                timeout);
    AAudioStream_close(cdata->stream);
    csound->Message(csound, "closed AAudio input\n");
    csound->Free(csound, cdata);
    *(csound->GetRtRecordUserData(csound)) = NULL;
  }
  cdata = (AAUDIO_PARAMS *)
    *(csound->GetRtPlayUserData(csound));
  if(cdata && cdata->stream) {
    inputState = AAudioStream_getState(cdata->stream);
    nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    AAudioStream_requestStop(cdata->stream);
    csound->Message(csound, "requested AAudio output stop\n");   
    AAudioStream_waitForStateChange(cdata->stream, 
                                    inputState, 
                                    &nextState, 
                                    timeout);    
    AAudioStream_close(cdata->stream);
    csound->Message(csound, "closed AAudio output\n");
    if(cdata->incb)
      csound->DestroyCircularBuffer(csound, cdata->incb);
    csound->Free(csound, cdata);
    *(csound->GetRtPlayUserData(csound)) = NULL;
  }
}

void aaudio_setup(CSOUND *csound) {
 csoundSetPlayopenCallback(csound, open_out);
 csoundSetRecopenCallback(csound, open_in);
 csoundSetRtplayCallback(csound, audio_output);
 csoundSetRtrecordCallback(csound, audio_input);
 csoundSetRtcloseCallback(csound, close_io);
}
