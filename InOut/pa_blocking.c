#include "pa_blocking.h"

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
    pabs->actualBufferSampleCount = csound->GetKsmps(csound)
      * csound->GetNchnls(csound);
    pabs->actualBuffer = (float *)
      ((ENVIRON*) csound)->Calloc(csound, pabs->actualBufferSampleCount
                                          * sizeof(float));
    pabs->paLock = csound->CreateThreadLock(csound);
    pabs->clientLock = csound->CreateThreadLock(csound);
    maxLag = (parm->bufSamp_HW <= 0 ? IODACSAMPS : parm->bufSamp_HW);
    maxLag = csound->GetKsmps(csound);
    memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound, "paBlockingReadOpen: nchnls %d sr %f maxLag %u\n",
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

void paBlockingRead(PA_BLOCKING_STREAM *pabs, MYFLT *buffer)
{
    size_t i;
    size_t n;
    ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound,
                                              pabs->clientLock, 100);
    for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
      buffer[i] = (MYFLT)pabs->actualBuffer[i];
    }
    ((ENVIRON*) pabs->csound)->NotifyThreadLock(pabs->csound, pabs->paLock);
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
                    "frames %u device %d\n",
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
        ((ENVIRON*) pabs->csound)->WaitThreadLock(pabs->csound,
                                                  pabs->clientLock, 100);
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
        ((ENVIRON*) csound)->Free(csound, pabs->actualBuffer);
        ((ENVIRON*) csound)->Free(csound, pabs);
      }
    }
}

