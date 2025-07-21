/*
   rtaudio.c
   AAudio module for Csound

   Copyright (C) 2011 Victor Lazzarini.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA

*/
#include <aaudio/AAudio.h>
#include "csoundCore.h"
#include <android/log.h>
#include <stdint.h>
#include <time.h>


typedef struct  {
  AAudioStream *stream;
  int32_t nchnls;
  MYFLT sr;
  void *cb;
  MYFLT *buffer;
  CSOUND *csound; 
} AAUDIO_PARAMS;


static aaudio_data_callback_result_t
 output_callback(AAudioStream *stream, void *userData, void *audioData,
        int32_t numFrames);

static int32_t open_out(CSOUND *csound, const csRtAudioParams *parm) {
   csound->Message(csound, "AAUDIO output opening\n");
   AAudioStreamBuilder *builder;
   aaudio_result_t result = AAudio_createStreamBuilder(&builder);
   int32 dev = parm->devName ? atoi(parm->devName) : AAUDIO_UNSPECIFIED;
   void **playdata = csound->GetRtPlayUserData(csound);
   AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS*)
      csound->Calloc(csound, sizeof(AAUDIO_PARAMS));
   *playdata = cdata;
   csound->Message(csound, "creating circular buffer\n");
   cdata->cb = csound->CreateCircularBuffer(csound,
                                   parm->bufSamp_HW*parm->nChannels,
                                             sizeof(MYFLT));

   csound->Message(csound, "created circular buffer\n");
   cdata->buffer =
      (MYFLT *) csound->Calloc(csound,
                               csound->GetOutputBufferSize(csound)*sizeof(MYFLT));
    memset(cdata->buffer, 0,
           csound->GetOutputBufferSize(csound)*sizeof(MYFLT));
   
   AAudioStreamBuilder_setDeviceId(builder, dev);  
   AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
   AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
   AAudioStreamBuilder_setSampleRate(builder, parm->sampleRate);
   AAudioStreamBuilder_setChannelCount(builder, parm->nChannels);
   AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
   AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
   AAudioStreamBuilder_setDataCallback(builder, output_callback, cdata);

   AAudioStream *stream;
   result = AAudioStreamBuilder_openStream(builder, &stream);
   cdata->nchnls = parm->nChannels;
   cdata->stream = stream;
   cdata->sr = parm->sampleRate;
   cdata->csound = csound;
   result = AAudioStream_requestStart(stream);
   AAudioStreamBuilder_delete(builder);
   csound->Message(csound, "AAUDIO output opened\n");
   return OK;
}

#define MICROS 1000000
static void audio_output(CSOUND *csound, const MYFLT *outbuff, int32_t nbytes){
    AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) *(csound->GetRtPlayUserData(csound));
    int32_t n = nbytes/sizeof(MYFLT);
    int32_t m = 0, l;
    MYFLT sr = cdata->sr;
    do {
      l = csound->WriteCircularBuffer(csound, cdata->cb,&outbuff[m],n);
      m += l;
      n -= l;
      if(n) usleep(MICROS/sr);
    } while(n);
}
  

static aaudio_data_callback_result_t
   output_callback(AAudioStream *stream,void *userData, void *audioData,
                    int32_t numFrames) {
  AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) userData;
  float *samples = (float *) audioData;
  CSOUND *csound = cdata->csound;
  int32_t chns = cdata->nchnls;
  int32_t n = numFrames*chns;
  MYFLT *buffer = cdata->buffer;
  n = csound->ReadCircularBuffer(csound,cdata->cb,buffer,n);
  for (int i = 0; i < numFrames*chns; i++) {
    if(i < n)
     samples[i] = buffer[i];
    else samples[i] = 0.f;
  }
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static aaudio_data_callback_result_t
 input_callback(AAudioStream *stream, void *userData, void *audioData,
        int32_t numFrames);

static int32_t open_in(CSOUND *csound, const csRtAudioParams *parm) {
   AAudioStreamBuilder *builder;
   aaudio_result_t result = AAudio_createStreamBuilder(&builder);
   int32 dev = parm->devName ? atoi(parm->devName) : AAUDIO_UNSPECIFIED;
   void **recdata = csound->GetRtRecordUserData(csound);
   AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS*)
      csound->Calloc(csound, sizeof(AAUDIO_PARAMS));
   *recdata = cdata;

   cdata->cb = csound->CreateCircularBuffer(csound,
                                   parm->bufSamp_HW*parm->nChannels,
                                             sizeof(MYFLT));
   cdata->buffer =
      (MYFLT *) csound->Calloc(csound,
                               csound->GetInputBufferSize(csound)*sizeof(MYFLT));
    memset(cdata->buffer, 0,
           csound->GetInputBufferSize(csound)*sizeof(MYFLT));
   
   AAudioStreamBuilder_setDeviceId(builder, dev);  
   AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
   AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
   AAudioStreamBuilder_setSampleRate(builder, parm->sampleRate);
   AAudioStreamBuilder_setChannelCount(builder, parm->nChannels);
   AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
   AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
   AAudioStreamBuilder_setDataCallback(builder, input_callback, cdata);

   AAudioStream *stream;
   result = AAudioStreamBuilder_openStream(builder, &stream);
   cdata->nchnls = parm->nChannels;
   cdata->stream = stream;
   cdata->sr = parm->sampleRate;
   cdata->csound = csound;
   result = AAudioStream_requestStart(stream);
   AAudioStreamBuilder_delete(builder);
   return OK;
}


static int32_t audio_input(CSOUND *csound, MYFLT *inbuff, int32_t nbytes)
{
    AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) *(csound->GetRtRecordUserData(csound));
    int32_t n = nbytes/sizeof(MYFLT);
    int32_t m = 0, l;
    MYFLT sr = cdata->sr;
    do{
      l = csound->ReadCircularBuffer(csound,cdata->cb,&inbuff[m],n);
      m += l;
      n -= l;
      if(n) usleep(MICROS/sr);
    } while(n);
    return nbytes;
}

static aaudio_data_callback_result_t
   input_callback(AAudioStream *stream,void *userData, void *audioData,
                    int32_t numFrames) {
  AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) userData;
  CSOUND *csound = cdata->csound;
  float *samples = (float *) audioData;
  int32_t chns = cdata->nchnls;
  int32_t n = numFrames*chns, k = csound->GetInputBufferSize(csound);
  MYFLT *buffer = cdata->buffer;
  for (int i = 0; i < n; i++) {
    if(i < k)
     buffer[i] = samples[i];
  }
  csound->WriteCircularBuffer(csound,cdata->cb,buffer,n);
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static void close_io(CSOUND *csound) {
  AAUDIO_PARAMS *cdata = (AAUDIO_PARAMS *) *(csound->GetRtRecordUserData(csound));
  if(cdata) {
    AAudioStream_requestStop(cdata->stream);
    AAudioStream_close(cdata->stream);
    csound->DestroyCircularBuffer(csound, cdata->cb);
    csound->Free(csound, cdata->buffer);
    csound->Free(csound, cdata);
  }

  cdata = (AAUDIO_PARAMS *) *(csound->GetRtPlayUserData(csound));
  if(cdata) {
    AAudioStream_requestStop(cdata->stream);
    AAudioStream_close(cdata->stream);
    csound->DestroyCircularBuffer(csound, cdata->cb);
    csound->Free(csound, cdata->buffer);
    csound->Free(csound, cdata);
  }  
}


void aaudio_setup(CSOUND *csound) {
 csoundSetPlayopenCallback(csound, open_out);
 csoundSetRecopenCallback(csound, open_in);
 csoundSetRtplayCallback(csound, audio_output);
 csoundSetRtrecordCallback(csound, audio_input);
 csoundSetRtcloseCallback(csound, close_io);
}
