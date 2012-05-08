
/* 
    rtopensl.c
    OpenSl ES Audio Module for Csound                        

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
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"
#include "csdl.h"
#include <android/log.h> 

typedef struct OPEN_SL_PARAMS_ {
  CSOUND      *csound;
  // engine interfaces
  SLObjectItf engineObject;
  SLEngineItf engineEngine;

// output mix interfaces
 SLObjectItf outputMixObject;

// buffer queue player interfaces
 SLObjectItf bqPlayerObject;
 SLPlayItf bqPlayerPlay;
 SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
 SLEffectSendItf bqPlayerEffectSend;

// recorder interfaces
 SLObjectItf recorderObject;
 SLRecordItf recorderRecord;
 SLAndroidSimpleBufferQueueItf recorderBufferQueue;

  int currentInputIndex;
  int currentOutputIndex;

  int currentOutputBuffer;
  int currentInputBuffer;
  
  short *outputBuffer[2];
  short *inputBuffer[2];

  int outBufSamples;
  int inBufSamples;

  // locks
  void *clientLockIn;
  void *clientLockOut;

  csRtAudioParams outParm;
  csRtAudioParams inParm;

} open_sl_params;

#define CONV16BIT 32768
#define CONVMYFLT FL(1./32768.)

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
   open_sl_params *params = (open_sl_params *) context;
   params->csound->NotifyThreadLock(params->clientLockOut);

}

/* put samples to DAC */
void androidrtplay_(CSOUND *csound, const MYFLT *buffer, int nbytes)
{
    open_sl_params *params; 
    int     i = 0, samples = nbytes / (int) sizeof(MYFLT);
    short* openslBuffer;

    params = (open_sl_params *) *(csound->GetRtPlayUserData(csound));
    if (params == NULL)
      return;
    openslBuffer = params->outputBuffer[params->currentOutputBuffer];
    do {     
      openslBuffer[params->currentOutputIndex++] = (short) (buffer[i]*CONV16BIT);
      if (params->currentOutputIndex >= params->outBufSamples) {
	 csound->WaitThreadLock(params->clientLockOut, (size_t) 1000);
         (*params->bqPlayerBufferQueue)->Enqueue(params->bqPlayerBufferQueue, 
	   openslBuffer,params->outBufSamples*sizeof(short));
         params->currentOutputBuffer = (params->currentOutputBuffer ?  0 : 1);
         params->currentOutputIndex = 0;
         openslBuffer = params->outputBuffer[params->currentOutputBuffer];
      }
    } while (++i < samples);
}

SLresult openSLCreateEngine(open_sl_params *params)
{
    SLresult result;

    // create engine
    result = slCreateEngine(&(params->engineObject), 0, NULL, 0, NULL, NULL);
    if(result != SL_RESULT_SUCCESS) goto engine_end;

    // realize the engine 
    result = (*params->engineObject)->Realize(params->engineObject, SL_BOOLEAN_FALSE);
     if(result != SL_RESULT_SUCCESS) goto engine_end;

    // get the engine interface, which is needed in order to create other objects
    result = (*params->engineObject)->GetInterface(params->engineObject, SL_IID_ENGINE, &(params->engineEngine));
     if(result != SL_RESULT_SUCCESS) goto engine_end;

 engine_end:
    return result;
}

int openSLPlayOpen(open_sl_params *params)
{
    SLresult result;
    SLuint32 sr = params->outParm.sampleRate;

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};

    switch(sr){

    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
   
    const SLInterfaceID ids[] = {SL_IID_VOLUME};
    const SLboolean req[] = {SL_BOOLEAN_FALSE};
    result = (*params->engineEngine)->CreateOutputMix(params->engineEngine, &(params->outputMixObject), 1, ids, req);
     if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // realize the output mix
    result = (*params->outputMixObject)->Realize(params->outputMixObject, SL_BOOLEAN_FALSE);
    int chnls = params->csound->GetNchnls(params->csound);
    int speakers;
      if(chnls == 1) speakers =  SL_SPEAKER_FRONT_CENTER;
      else speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;

    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, chnls , sr,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
       speakers, SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, params->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids1[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req1[] = {SL_BOOLEAN_TRUE};
    result = (*params->engineEngine)->CreateAudioPlayer(params->engineEngine, &(params->bqPlayerObject), &audioSrc, &audioSnk,
            1, ids1, req1);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // realize the player
    result = (*params->bqPlayerObject)->Realize(params->bqPlayerObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // get the play interface
    result = (*params->bqPlayerObject)->GetInterface(params->bqPlayerObject, SL_IID_PLAY, &(params->bqPlayerPlay));
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // get the buffer queue interface
    result = (*params->bqPlayerObject)->GetInterface(params->bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
						    &(params->bqPlayerBufferQueue));
   if(result != SL_RESULT_SUCCESS) goto end_openaudio;

    // register callback on the buffer queue
    result = (*params->bqPlayerBufferQueue)->RegisterCallback(params->bqPlayerBufferQueue, bqPlayerCallback, params);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;

     // set the player's state to playing
    result = (*params->bqPlayerPlay)->SetPlayState(params->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
 
 end_openaudio:
    return result;
}

int openSLInitOutParams(open_sl_params *params){

   params->clientLockOut = params->csound->CreateThreadLock();
   if (params->clientLockOut == NULL)
      goto err_return;
   params->outBufSamples  =  params->csound->GetOutputBufferSize(params->csound);

   if((params->outputBuffer[0] = calloc(params->outBufSamples, sizeof(short))) == NULL ||
      (params->outputBuffer[1] = calloc(params->outBufSamples, sizeof(short))) == NULL ){
       params->csound->Message(params->csound, "memory allocation failure in opensl module \n");
        goto err_return;
     }
    params->currentInputIndex = 0;
    params->currentOutputBuffer  = 0;
   return OK;

 err_return:
    return -1;  
}

/* open for audio output */
int androidplayopen_(CSOUND *csound, const csRtAudioParams *parm)
{

    CSOUND *p = csound;
    open_sl_params *params;

    params = (open_sl_params*) p->QueryGlobalVariable(p, "_openslGlobals");
    if (params == NULL) {
      if (p->CreateGlobalVariable(p, "_openslGlobals", sizeof(open_sl_params))
          != 0)
        return -1;
      params = (open_sl_params*) p->QueryGlobalVariable(p, "_openslGlobals");
      params->csound = p;
      
      if(openSLCreateEngine(params) != SL_RESULT_SUCCESS) {
           csound->Message(csound, Str("OpenSL: engine create error \n")); 
           return -1;
      }
      
    }
    memcpy(&(params->outParm), parm, sizeof(csRtAudioParams));
    *(p->GetRtPlayUserData(p)) = (void*) params;
    
    if(openSLPlayOpen(params) !=  SL_RESULT_SUCCESS) return -1;
   
    int returnVal = openSLInitOutParams(params);
    params->csound->NotifyThreadLock(params->clientLockOut);
    return returnVal;
}
// ===============================

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  open_sl_params *params = (open_sl_params *) context;
  params->csound->NotifyThreadLock(params->clientLockIn);
}

/* get samples from ADC */
int androidrtrecord_(CSOUND *csound, MYFLT *buffer, int nbytes)
{

    open_sl_params  *params;
    int     i = 0, j = 0, samples = nbytes / (int) sizeof(MYFLT);
    short *openslBuffer; int nchnls = csound->GetNchnls(csound);
    params = (open_sl_params*) *(csound->GetRtRecordUserData(csound));

    if (params == NULL) {
      memset(buffer, 0, nbytes);
      return nbytes;
    } 
    openslBuffer = params->inputBuffer[params->currentInputBuffer];
    do {
      if (params->currentInputIndex >= params->inBufSamples) {
	  csound->WaitThreadLock(params->clientLockIn, (size_t) 1000);
          (*params->recorderBufferQueue)->Enqueue(params->recorderBufferQueue, 
	 						  openslBuffer,params->inBufSamples*sizeof(short));
          params->currentInputBuffer = (params->currentInputBuffer ? 0 : 1);
          params->currentInputIndex = 0;
          openslBuffer = params->inputBuffer[params->currentInputBuffer];
      }
       for(j = 0; j < nchnls; j++) {
       buffer[i + j] = (MYFLT) openslBuffer[params->currentInputIndex]*CONVMYFLT;
      }
      params->currentInputIndex++;
      i += nchnls;
    } while (i < samples);
    

    return nbytes;
}

int openSLRecOpen(open_sl_params *params){

    SLresult result;
    SLuint32 sr = params->inParm.sampleRate;
    int nchnls;
    switch(sr){

    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
    nchnls = 1; /* always mono for the moment // params->csound->GetNchnls(params->csound); */
   // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,nchnls, sr,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*params->engineEngine)->CreateAudioRecorder(params->engineEngine, &(params->recorderObject), &audioSrc,
            &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

    // realize the audio recorder
    result = (*params->recorderObject)->Realize(params->recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

 
    // get the record interface
    result = (*params->recorderObject)->GetInterface(params->recorderObject, SL_IID_RECORD, &(params->recorderRecord));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
 
    // get the buffer queue interface
    result = (*params->recorderObject)->GetInterface(params->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
						     &(params->recorderBufferQueue));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;

    // register callback on the buffer queue
    result = (*params->recorderBufferQueue)->RegisterCallback(params->recorderBufferQueue, bqRecorderCallback,
            params);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;


    result = (*params->recorderRecord)->SetRecordState(params->recorderRecord, SL_RECORDSTATE_RECORDING);

   
 end_recopen: 
    return result;
}

int openSLInitInParams(open_sl_params *params){
   params->clientLockIn = params->csound->CreateThreadLock();
   if (params->clientLockIn == NULL)
      return -1; 
   /* input is always mono, so buffer size needs to be adjusted accordingly */
  params->inBufSamples  = params->csound->GetInputBufferSize(params->csound) /
                          params->csound->GetNchnls(params->csound);
  if((params->inputBuffer[0] = (short *)calloc(params->inBufSamples, sizeof(short))) == NULL ||
     (params->inputBuffer[1] = (short *)calloc(params->inBufSamples, sizeof(short))) == NULL){
       params->csound->Message(params->csound, "memory allocation failure in opensl module \n");
       return -1;
     }
   params->currentInputIndex = params->inBufSamples;
   params->currentInputBuffer = 0;    
   return OK;

}
/* open for audio input */
int androidrecopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    CSOUND *p = csound; int res;
    open_sl_params *params;
    params = (open_sl_params*) p->QueryGlobalVariable(p, "_openslGlobals");
    if (params == NULL) {
      if (p->CreateGlobalVariable(p, "_openslGlobals", sizeof(open_sl_params))
          != 0)
        return -1;
      params = (open_sl_params*) p->QueryGlobalVariable(p, "_openslGlobals");
      params->csound = p;
      
      if(openSLCreateEngine(params) != SL_RESULT_SUCCESS) {
           csound->Message(csound, Str("OpenSL: engine create error \n")); 
           return -1;
      }
      
    }
    memcpy(&(params->inParm), parm, sizeof(csRtAudioParams));
    *(p->GetRtRecordUserData(p)) = (void*) params;
    if(openSLRecOpen(params) !=  SL_RESULT_SUCCESS) {
      csound->Message(csound, Str("OpenSL: input open error \n")); 
     return -1;
    }
    
    res = openSLInitInParams(params);
    if(res == OK)  params->csound->NotifyThreadLock(params->clientLockIn);
    return res;
}

/* close the I/O device entirely */
void androidrtclose_(CSOUND *csound)
{
 
    open_sl_params *params;
    params = (open_sl_params *) csound->QueryGlobalVariable(csound,
                                                             "_openslGlobals");
    if (params == NULL)
      return;

    if (params->clientLockIn != NULL) {
      csound->NotifyThreadLock(params->clientLockIn);
      csound->DestroyThreadLock(params->clientLockIn);
      params->clientLockIn = NULL;
    }
    
    if (params->clientLockOut != NULL) {
      csound->NotifyThreadLock(params->clientLockOut);
      csound->DestroyThreadLock(params->clientLockOut);
      params->clientLockOut = NULL;
    }
    
    if (params->outputBuffer[0] != NULL) {
      free(params->outputBuffer[0]);
      params->outputBuffer[0] = NULL;
    }

    if (params->outputBuffer[1] != NULL) {
      free(params->outputBuffer[1]);
      params->outputBuffer[1] = NULL;
    }

    if (params->inputBuffer[0] != NULL) {
      free(params->inputBuffer[0]);
      params->inputBuffer[0] = NULL;
    }

     if (params->inputBuffer[1] != NULL) {
      free(params->inputBuffer[1]);
      params->inputBuffer[1] = NULL;
    }

    *(csound->GetRtRecordUserData(csound)) = NULL;
    *(csound->GetRtPlayUserData(csound)) = NULL;
    csound->DestroyGlobalVariable(csound, "_openslGlobals");


// destroy buffer queue audio player object, and invalidate all associated interfaces
    if (params->bqPlayerObject != NULL) {
        (*params->bqPlayerObject)->Destroy(params->bqPlayerObject);
        params->bqPlayerObject = NULL;
        params->bqPlayerPlay = NULL;
        params->bqPlayerBufferQueue = NULL;
        params->bqPlayerEffectSend = NULL;
    }

    // destroy audio recorder object, and invalidate all associated interfaces
    if (params->recorderObject != NULL) {
        (*params->recorderObject)->Destroy(params->recorderObject);
        params->recorderObject = NULL;
        params->recorderRecord = NULL;
        params->recorderBufferQueue = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (params->outputMixObject != NULL) {
        (*params->outputMixObject)->Destroy(params->outputMixObject);
        params->outputMixObject = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (params->engineObject != NULL) {
        (*params->engineObject)->Destroy(params->engineObject);
        params->engineObject = NULL;
        params->engineEngine = NULL;
    }

}

//PUBLIC int csoundModuleCreate(CSOUND *csound)
//{
//    /* nothing to do, report success */
//   csound->Message(csound,
//     Str("OpenSL ES real-time audio module for Csound\n"));
//    return 0;
//}
//
//PUBLIC int csoundModuleInit(CSOUND *csound)
//{
//    char    *s, drv[12];
//    int     i;
//
//    if ((s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL)
//      return 0;
//    for (i = 0; s[i] != '\0' && i < 11; i++)
//      drv[i] = s[i] & (char) 0xDF;
//    drv[i] = '\0';
//    if (!(strcmp(drv, "OPENSL") == 0 || strcmp(drv, "OSL") == 0 )
//      return 0;
//    csound->Message(csound, Str("rtaudio: OenSL ES module enabled ... "));
//    /* set function pointers */
//      csound->SetPlayopenCallback(csound, playopen_);
//      csound->SetRecopenCallback(csound, recopen_);
//      csound->SetRtplayCallback(csound, rtplay_);
//      csound->SetRtrecordCallback(csound, rtrecord_);
//      csound->SetRtcloseCallback(csound, rtclose_);
//    
//    return 0;
//}
//
//PUBLIC int csoundModuleDestroy(CSOUND *csound)
//{
//    return 0;
//}
//
//PUBLIC int csoundModuleInfo(void)
//{
//    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
//}
//
