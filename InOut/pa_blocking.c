#include "pa_blocking.h"

#if defined(WIN32) || defined(__MACH__)

int paBlockingReadOpen(ENVIRON *csound,
		       PA_BLOCKING_STREAM **pabs_, PaStreamParameters *paParameters)
{
  PaError paError = -1;
  unsigned long maxLag = 0;
  PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)
    mcalloc(sizeof(PA_BLOCKING_STREAM));
  pabs->csound = csound;
  pabs->actualBufferSampleCount = csound->GetKsmps(csound)
    * csound->GetNchnls(csound);
  pabs->actualBuffer = (float *)
    mcalloc(pabs->actualBufferSampleCount * sizeof(float));
  pabs->paLock = csoundCreateThreadLock(csound);
  pabs->clientLock = csoundCreateThreadLock(csound);
  maxLag = O.oMaxLag <= 0 ? IODACSAMPS : O.oMaxLag;
#ifdef __MACH__
  maxLag =  ((int)ekr)*((maxLag + (int)ekr-1)%krate); /* Round to multiple */
#elif WIN32
  maxLag = csound->GetKsmps(csound);
#endif
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
    mfree(pabs->actualBuffer);
    mfree(pabs);
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
  float *paInput = (float *)input;
  csoundNotifyThreadLock(pabs->csound, pabs->clientLock);
  csoundWaitThreadLock(pabs->csound, pabs->paLock, 100);
  memcpy(pabs->actualBuffer, paInput,
	 pabs->actualBufferSampleCount * sizeof(float));
  return paContinue;
}

void paBlockingRead(PA_BLOCKING_STREAM *pabs, MYFLT *buffer)
{
  size_t i;
  size_t n;
  csoundWaitThreadLock(pabs->csound, pabs->clientLock, 100);
  for (i = 0, n = pabs->actualBufferSampleCount; i < n; i++) {
    buffer[i] = (MYFLT)pabs->actualBuffer[i];
  }
  csoundNotifyThreadLock(pabs->csound, pabs->paLock);
}

int paBlockingWriteOpen(ENVIRON *csound,
                        PA_BLOCKING_STREAM **pabs_,
                        PaStreamParameters *paParameters)
{
  PaError paError = paNoError;
  unsigned long maxLag = 0;
  PA_BLOCKING_STREAM *pabs =
    (PA_BLOCKING_STREAM *) mcalloc(sizeof(PA_BLOCKING_STREAM));
  pabs->csound = csound;
  pabs->currentIndex = 0;
  pabs->paLock = csoundCreateThreadLock(csound);
  pabs->clientLock = csoundCreateThreadLock(csound);
  maxLag = O.oMaxLag <= 0 ? IODACSAMPS : O.oMaxLag;
#ifdef __MACH__
  maxLag =  ((int)ekr)*((maxLag + (int)ekr-1)%krate); /* Round to multiple */
#elif WIN32
  maxLag = csound->GetKsmps(csound);
#endif
  pabs->actualBufferSampleCount = csound->GetKsmps(csound)
    * csound->GetNchnls(csound);
  //pabs->actualBufferSampleCount = maxLag * csound->GetKsmps(csound);
  pabs->actualBuffer = (float *)
    mcalloc(pabs->actualBufferSampleCount * sizeof(float));
  memcpy(&pabs->paParameters, paParameters, sizeof(PaStreamParameters));
  csound->Message(csound,
		  "paBlockingWriteOpen: nchnls %d sr %f buffer frames %u device %d\n",
		  pabs->paParameters.channelCount,
		  csound->esr_,
		  maxLag,
		  pabs->actualBufferSampleCount,
		  pabs->paParameters.device);
  paError = Pa_OpenStream(&pabs->paStream,
			  0,
			  &pabs->paParameters,
			  (double) csound->esr_,
			  maxLag,
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
  PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *)userData;
  float *paOutput = (float *)output;
  csoundWaitThreadLock(pabs->csound, pabs->paLock, 100);
  memcpy(paOutput, pabs->actualBuffer,
	 pabs->actualBufferSampleCount * sizeof(float));
  csoundNotifyThreadLock(pabs->csound, pabs->clientLock);
  return paContinue;
}

void paBlockingWrite(PA_BLOCKING_STREAM *pabs, int samples, MYFLT *buffer)
{
  size_t i;
  for (i = 0; i < samples; i++, pabs->currentIndex++) {
    pabs->actualBuffer[pabs->currentIndex] = (MYFLT) buffer[i];
    if(pabs->currentIndex >= pabs->actualBufferSampleCount) {
      csoundNotifyThreadLock(pabs->csound, pabs->paLock);
      csoundWaitThreadLock(pabs->csound, pabs->clientLock, 100);
      pabs->currentIndex = 0;
    }
  }
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

#endif
