#include "pa_blocking.h"

#define INS 0
#define OUTS 1
int openOnce = 0;

/* VL: CoreAUDIO full-duplex implementation */ 

int paBlockingReadWriteOpen(ENVIRON *csound,
    PA_BLOCKING_STREAM **pabs_in, PA_BLOCKING_STREAM **pabs_out, PaStreamParameters *paParameters,
    csRtAudioParams *parm)
{
    PaError paError = -1;
    unsigned long maxLag_ = 0;
    maxLag_ = (parm->bufSamp_HW <= 0 ? IODACSAMPS : parm->bufSamp_HW); 
    /* input & output streams */
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)
      ((ENVIRON*) csound)->Calloc(csound,sizeof(PA_BLOCKING_STREAM)*2);

    pabs[OUTS].csound = pabs[INS].csound = csound;
    pabs[INS].currentIndex = pabs[OUTS].currentIndex   = 0;
    pabs[OUTS].actualBufferSampleCount = pabs[INS].actualBufferSampleCount = maxLag_
      * csound->GetNchnls(csound);
    
    /* allocate input */  
    pabs[INS].actualBuffer = (float *)
     ((ENVIRON*) csound)->Calloc(csound, pabs->actualBufferSampleCount
                                          * sizeof(float));
    
    /* allocate output */
    pabs[OUTS].actualBuffer = (float *)
     ((ENVIRON*) csound)->Calloc(csound, pabs->actualBufferSampleCount
                                          * sizeof(float));
    
    /* locks */  
    pabs[INS].paLock = csound->CreateThreadLock(csound);
    pabs[OUTS].paLock = csound->CreateThreadLock(csound);
    pabs[INS].clientLock = csound->CreateThreadLock(csound);
    pabs[OUTS].clientLock = csound->CreateThreadLock(csound);    
    
   
    memcpy(&pabs[INS].paParameters, paParameters, sizeof(PaStreamParameters));
    memcpy(&pabs[OUTS].paParameters, paParameters, sizeof(PaStreamParameters)); 
    
    csound->Message(csound, "paBlockingReadWriteOpen: nchnls %d sr %f maxLag %lu\n",
                    paParameters->channelCount,
                    csound->GetSr(csound),
                    maxLag_);
                    
    paError = Pa_OpenStream(&pabs[INS].paStream,
                            &pabs[INS].paParameters,
                            &pabs[OUTS].paParameters,
                            (double) csound->GetSr(csound),
                            maxLag_,
                            paNoFlag,
                            paBlockingReadWriteStreamCallback,  
                            pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs[INS].paStream);
      pabs[OUTS].paStream = pabs[INS].paStream;
    }
    if (paError != paNoError) {
          ((ENVIRON*) csound)->Free(csound, pabs[INS].actualBuffer);
             ((ENVIRON*) csound)->Free(csound, pabs[OUTS].actualBuffer);
      ((ENVIRON*) csound)->Free(csound, pabs);
      *pabs_in = 0;
      *pabs_out = 0;
    } else {
      *pabs_in = &pabs[INS];
      *pabs_out = &pabs[OUTS];
      openOnce = 1;
    }
    return paError;
}

/* VL ASIO full-duplex implementation */
int paBlockingReadWriteStreamCallback(const void *input,
                                 void *output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    size_t i;
    size_t n;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)userData;
    float *paInput = (float *)input;
    float *paOutput = (float *) output;

((ENVIRON*) pabs[INS].csound)->WaitThreadLock(pabs[INS].csound, pabs[INS].paLock, 100);
    for (i = 0, n = pabs[INS].actualBufferSampleCount; i < n; i++) {
      pabs[INS].actualBuffer[i] = paInput[i];
    }
   
((ENVIRON*) pabs[INS].csound)->NotifyThreadLock(pabs[INS].csound, pabs[INS].clientLock);

                                                
((ENVIRON*) pabs[OUTS].csound)->WaitThreadLock(pabs[OUTS].csound, pabs[OUTS].paLock, 100);
    for (i = 0, n = pabs[OUTS].actualBufferSampleCount; i < n; i++) {
      paOutput[i] = pabs[OUTS].actualBuffer[i];
    }
    
((ENVIRON*) pabs[OUTS].csound)->NotifyThreadLock(pabs[OUTS].csound, pabs[OUTS].clientLock);  
    return paContinue;
}


int paBlockingReadOpen(ENVIRON *csound,
                       PA_BLOCKING_STREAM **pabs_,
                       PaStreamParameters *paParameters,
                       csRtAudioParams *parm)
{
    PaError paError = -1;
    unsigned long maxLag = 0;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)
      ((ENVIRON*) csound)->Calloc(csound, sizeof(PA_BLOCKING_STREAM));
    pabs->csound = csound;
    pabs->actualBuffer = (float *)
      ((ENVIRON*) csound)->Calloc(csound, pabs->actualBufferSampleCount
                                          * sizeof(float));
    pabs->paLock = csound->CreateThreadLock(csound);
    pabs->clientLock = csound->CreateThreadLock(csound);
    maxLag = (parm->bufSamp_HW <= 0 ? IODACSAMPS : parm->bufSamp_HW);
    pabs->actualBufferSampleCount = maxLag
      * csound->GetNchnls(csound); /* VL: was ksmps*channels , modified to HW buffer size */
    memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound, "paBlockingReadOpen: nchnls %d sr %f maxLag %lu\n",
                    pabs->paParameters.channelCount,
                    csound->GetSr(csound),
                    maxLag);
    paError = Pa_OpenStream(&pabs->paStream,
                            &pabs->paParameters,
                            0,
                            (double) csound->GetSr(csound),
                            maxLag,
                            paNoFlag,
                            paBlockingReadStreamCallback,  /* VL fixed: was paBlockingWriteStreamCallback */
                            pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs->paStream);
    }
    if (paError != paNoError) {
      ((ENVIRON*) csound)->Free(csound, pabs->actualBuffer);
      ((ENVIRON*) csound)->Free(csound, pabs);
      *pabs_ = 0;
    } else {
      *pabs_ = pabs;
    }
    return paError;
}

int paBlockingReadStreamCallback(const void *input,
                                 void *output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)userData;
    ((ENVIRON*) pabs->csound)->NotifyThreadLock(pabs->csound, pabs->clientLock);
    ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound, pabs->paLock, 100);
    memcpy(pabs->actualBuffer, input,
           pabs->actualBufferSampleCount * sizeof(float));
    return paContinue;
}

void paBlockingRead(PA_BLOCKING_STREAM *pabs, int samples, MYFLT *buffer)
{
    size_t i;
   /* VL: now this function takes the number of samples in *buffer into account */   
 for (i = 0; i < samples; i++, pabs->currentIndex++) {
      if (pabs->currentIndex >= pabs->actualBufferSampleCount) {
        ((ENVIRON*) pabs->csound)->NotifyThreadLock(pabs->csound, pabs->paLock);
        ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound,pabs->clientLock, 100);
        pabs->currentIndex = 0;
         
      }
      buffer[i] = (MYFLT)pabs->actualBuffer[pabs->currentIndex];
    }
    
    
}

int paBlockingWriteOpen(ENVIRON *csound,
                        PA_BLOCKING_STREAM **pabs_,
                        PaStreamParameters *paParameters,
                        csRtAudioParams *parm)
{
    PaError paError = paNoError;
    unsigned long maxLag = 0;
    PA_BLOCKING_STREAM *pabs =
      (PA_BLOCKING_STREAM *)
        ((ENVIRON*) csound)->Calloc(csound, sizeof(PA_BLOCKING_STREAM));
    pabs->csound = csound;
    pabs->currentIndex = 0;
    pabs->paLock = csound->CreateThreadLock(csound);
    pabs->clientLock = csound->CreateThreadLock(csound);
    maxLag = (parm->bufSamp_HW <= 0 ? IODACSAMPS : parm->bufSamp_HW);
    pabs->actualBufferSampleCount = maxLag * csound->GetNchnls(csound);
    pabs->actualBuffer = (float *)
      ((ENVIRON*) csound)->Calloc(csound, pabs->actualBufferSampleCount
                                          * sizeof(float));
    memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound,
                    "paBlockingWriteOpen: nchnls %d sr %f buffer "
                    "frames %lu device %d\n",
                    pabs->paParameters.channelCount,
                    csound->esr,
                    maxLag,
                    pabs->paParameters.device);
    paError = Pa_OpenStream(&pabs->paStream,
                            0,
                            &pabs->paParameters,
                            (double) csound->esr,
                            maxLag,
                            paNoFlag,
                            paBlockingWriteStreamCallback,
                            pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs->paStream);
    }
    csound->Message(csound, "paBlockingWriteOpen returned %d.\n", paError);
    if (paError != paNoError) {
      ((ENVIRON*) csound)->Free(csound, pabs->actualBuffer);
      ((ENVIRON*) csound)->Free(csound, pabs);
      *pabs_ = 0;
    } else {
      *pabs_ = pabs;
    }
    return paError;
}

int paBlockingWriteStreamCallback(const void *input,
                                  void *output, unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo* timeInfo,
                                  PaStreamCallbackFlags statusFlags,
                                  void *userData)
{
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)userData;
    if (!pabs) return paContinue;
    if (!pabs->paStream) return paContinue;
    ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound, pabs->paLock, 100);
    memcpy(output, pabs->actualBuffer,
           pabs->actualBufferSampleCount * sizeof(float));
    ((ENVIRON*) pabs->csound)->NotifyThreadLock(pabs->csound, pabs->clientLock);
    return paContinue;
}

void paBlockingWrite(PA_BLOCKING_STREAM *pabs, int samples, MYFLT *buffer)
{
    size_t i;
    if (!pabs) return;
    if (!pabs->actualBuffer) return;
    for (i = 0; i < samples; i++, pabs->currentIndex++) {
      if (pabs->currentIndex >= pabs->actualBufferSampleCount) {
        ((ENVIRON*) pabs->csound)->NotifyThreadLock(pabs->csound, pabs->paLock);
        ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound,pabs->clientLock, 100);
           
        pabs->currentIndex = 0;
      }
      pabs->actualBuffer[pabs->currentIndex] = (MYFLT) buffer[i];
    }
}

void paBlockingClose(void *csound, PA_BLOCKING_STREAM *pabs)
{
    if (pabs) {
      if (pabs->paStream) {
        Pa_CloseStream(pabs->paStream);
        Pa_Sleep(500);
       ((ENVIRON*) csound)->Free(csound, pabs[INS].actualBuffer);
       if(openOnce) ((ENVIRON*) csound)->Free(csound, pabs[OUTS].actualBuffer);
       ((ENVIRON*) csound)->Free(csound, pabs);
      }
    }
}

