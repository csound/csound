#include "pa_blocking.h"

int paBlockingReadOpen(ENVIRON *csound,
    PA_BLOCKING_STREAM **pabs_, PaStreamParameters *paParameters)
{
    PaError paError = -1;
    unsigned long maxLag_ = 0;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)
      mcalloc(sizeof(PA_BLOCKING_STREAM));
    pabs->csound = csound;
    pabs->actualBufferSampleCount = csound->GetKsmps(csound)
      * csound->GetNchnls(csound);
    pabs->actualBuffer = (float *)
      mcalloc(pabs->actualBufferSampleCount * sizeof(float));
    pabs->paLock = csoundCreateThreadLock(csound);
    pabs->clientLock = csoundCreateThreadLock(csound);
    maxLag_ = O.oMaxLag <= 0 ? IODACSAMPS : O.oMaxLag;
    memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound, "paBlockingReadOpen: nchnls %d sr %f maxLag %u\n",
                    pabs->paParameters.channelCount,
                    csound->GetSr(csound),
                    maxLag_);
    paError = Pa_OpenStream(&pabs->paStream,
                            &pabs->paParameters,
                            0,
                            (double) csound->GetSr(csound),
                            maxLag_,
                            paNoFlag,
                            paBlockingReadStreamCallback,  /* VL fixed: was paBlockingWriteStreamCallback */
                            pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs->paStream);
    }
    if (paError != paNoError) {
      mfree(pabs->actualBuffer);
      mfree(pabs);
      *pabs_ = 0;
    } else {
      *pabs_ = pabs;
    }
    return paError;
}

/* VL fixed was:PaBlockingReadStreamCallback */
int paBlockingReadStreamCallback(const void *input,
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
    csoundNotifyThreadLock(pabs->csound, pabs->clientLock);
    csoundWaitThreadLock(pabs->csound, pabs->paLock, 100);
    for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
      pabs->actualBuffer[i] = paInput[i];
    }
    return paContinue;
}

void paBlockingRead(PA_BLOCKING_STREAM *pabs, MYFLT *buffer)
{
    size_t i;
    size_t n;
    csoundWaitThreadLock(pabs->csound, pabs->clientLock, 100);
    for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
      buffer[i] = pabs->actualBuffer[i];
    }
    csoundNotifyThreadLock(pabs->csound, pabs->paLock);
}

int paBlockingWriteOpen(ENVIRON *csound,
                        PA_BLOCKING_STREAM **pabs_,
                        PaStreamParameters *paParameters)
{
    PaError paError = -1;
    unsigned long maxLag_ = 0;
    PA_BLOCKING_STREAM *pabs =
      (PA_BLOCKING_STREAM *) mcalloc(sizeof(PA_BLOCKING_STREAM));
    pabs->csound = csound;
    pabs->actualBufferSampleCount = csound->GetKsmps(csound)
      * csound->GetNchnls(csound);
    pabs->actualBuffer = (float *)
      mcalloc(pabs->actualBufferSampleCount * sizeof(float));
    pabs->paLock = csoundCreateThreadLock(csound);
    pabs->clientLock = csoundCreateThreadLock(csound);
    maxLag_ = O.oMaxLag <= 0 ? IODACSAMPS : O.oMaxLag;
    memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound,
                    "paBlockingWriteOpen: nchnls %d sr %f maxLag %u device %d\n",
                    pabs->paParameters.channelCount,
                    csound->GetSr(csound),
                    maxLag_,
                    pabs->paParameters.device);
    paError = Pa_OpenStream(&pabs->paStream,
                            0,
                            &pabs->paParameters,
                            (double) csound->GetSr(csound),
                            maxLag_,
                            paNoFlag,
                            paBlockingWriteStreamCallback,
                            pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs->paStream);
    }
    csound->Message(csound, "paBlockingWriteOpen returned %d.\n", paError);
    if (paError != paNoError) {
      mfree(pabs->actualBuffer);
      mfree(pabs);
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
    size_t i;
    size_t n;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)userData;
    float *paOutput = (float *)output;
    csoundNotifyThreadLock(pabs->csound, pabs->clientLock);
    csoundWaitThreadLock(pabs->csound, pabs->paLock, 100);
    for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
      paOutput[i] = pabs->actualBuffer[i];
    }
    return paContinue;
}

void paBlockingWrite(PA_BLOCKING_STREAM *pabs, MYFLT *buffer)
{
    size_t i;
    size_t n;
    for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
      pabs->actualBuffer[i] = buffer[i];
    }
    csoundNotifyThreadLock(pabs->csound, pabs->paLock);
    csoundWaitThreadLock(pabs->csound, pabs->clientLock, 100);
}

void paBlockingClose(PA_BLOCKING_STREAM *pabs)
{
    if (pabs) {
      if (pabs->paStream) {
        Pa_AbortStream(pabs->paStream);
        mfree(pabs->actualBuffer);
        mfree(pabs);
        pabs->paStream = 0;
      }
    }
}

