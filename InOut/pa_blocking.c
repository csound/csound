#include "pa_blocking.h"

#define INS 1
#define OUTS 0

/* VL: portaudio full-duplex implementation */
int paBlockingReadWriteOpen(ENVIRON * csound,
                              PA_BLOCKING_STREAM * pabs,
                              PaStreamParameters * paParameters,
                              csRtAudioParams * parm)
{
    PaError paError = -1;
    unsigned long buf_size = 0;

    buf_size = parm->bufSamp_SW;
    /* input & output streams */
    pabs[OUTS].csound = pabs[INS].csound = csound;
    pabs[INS].currentIndex = pabs[OUTS].currentIndex = 0;
    pabs[OUTS].actualBufferSampleCount =
        pabs[INS].actualBufferSampleCount =
        buf_size * paParameters->channelCount;

    /* allocate input */
    pabs[INS].actualBuffer = (MYFLT *)
        ((ENVIRON *) csound)->Calloc(csound, pabs[INS].actualBufferSampleCount
                                     * sizeof(MYFLT));
    /* allocate output */
    pabs[OUTS].actualBuffer = (MYFLT *)
        ((ENVIRON *) csound)->Calloc(csound, pabs[OUTS].actualBufferSampleCount
                                     * sizeof(MYFLT));
    /* locks */
    pabs[INS].paLock = csound->CreateThreadLock(csound);
    pabs[OUTS].paLock = csound->CreateThreadLock(csound);
    pabs[INS].clientLock = csound->CreateThreadLock(csound);
    pabs[OUTS].clientLock = csound->CreateThreadLock(csound);

    memcpy(&pabs[INS].paParameters, paParameters, sizeof(PaStreamParameters));
    memcpy(&pabs[OUTS].paParameters, paParameters, sizeof(PaStreamParameters));

    csound->Message(csound,
                    "paBlockingReadWriteOpen: nchnls %d sr %f buf_size %lu\n",
                    paParameters->channelCount,
                    csound->GetSr(csound), buf_size);

    paError = Pa_OpenStream(&pabs->paStream,
                            &pabs[INS].paParameters,
                            &pabs[OUTS].paParameters,
                            (double) csound->GetSr(csound),
                            buf_size,
                            paNoFlag, paBlockingReadWriteStreamCallback, pabs);
    if (paError == paNoError)
      paError = Pa_StartStream(pabs->paStream);

    if (paError != paNoError) {
      ((ENVIRON *) csound)->Free(csound, pabs[INS].actualBuffer);
      ((ENVIRON *) csound)->Free(csound, pabs[OUTS].actualBuffer);
      pabs[INS].actualBuffer = NULL;
      pabs[OUTS].actualBuffer = NULL;
    }
    return paError;
}

int paBlockingWriteOpen(ENVIRON * csound,
                          PA_BLOCKING_STREAM * pabs,
                          PaStreamParameters * paParameters,
                          csRtAudioParams * parm)
{
    PaError paError = paNoError;
    unsigned long buf_size = 0;

    pabs[OUTS].csound = csound;
    pabs[OUTS].currentIndex = 0;
    pabs[OUTS].paLock = csound->CreateThreadLock(csound);
    pabs[OUTS].clientLock = csound->CreateThreadLock(csound);
    buf_size = parm->bufSamp_SW;
    pabs[OUTS].actualBufferSampleCount = buf_size * paParameters->channelCount;
    pabs[OUTS].actualBuffer = (MYFLT *)
        ((ENVIRON *) csound)->Calloc(csound, pabs[OUTS].actualBufferSampleCount
                                     * sizeof(MYFLT));
    memcpy(&pabs[OUTS].paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound,
                    "paBlockingWriteOpen: nchnls %d sr %f buffer "
                    "frames %lu device %d\n",
                    pabs[OUTS].paParameters.channelCount,
                    csound->esr, buf_size, pabs[OUTS].paParameters.device);
    paError = Pa_OpenStream(&pabs[OUTS].paStream,
                            0,
                            &pabs[OUTS].paParameters,
                            (double) csound->esr,
                            buf_size,
                            paNoFlag, paBlockingWriteStreamCallback, pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs[OUTS].paStream);
    }
    if (paError != paNoError) {
      ((ENVIRON *) csound)->Free(csound, pabs[OUTS].actualBuffer);
      pabs[OUTS].actualBuffer = NULL;
    }
    return paError;
}

/* VL: portaudio full-duplex implementation */
int paBlockingReadWriteStreamCallback(const void *input,
                                        void *output,
                                        unsigned long frameCount,
                                        const PaStreamCallbackTimeInfo *
                                        timeInfo,
                                        PaStreamCallbackFlags statusFlags,
                                        void *userData)
{
    size_t  i;
    size_t  n;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *) userData;
    float  *paInput = (float *) input;
    float  *paOutput = (float *) output;

    ((ENVIRON *) pabs[OUTS].csound)->WaitThreadLock(pabs[OUTS].csound,
                                                    pabs[OUTS].paLock, 500);
/* VL: I am using only the output lock */
/* ((ENVIRON*) pabs[INS].csound)->WaitThreadLock(pabs[INS].csound,
                                                 pabs[INS].paLock, 500); */
    for (i = 0, n = pabs[INS].actualBufferSampleCount; i < n; i++) {
      pabs[INS].actualBuffer[i] = (MYFLT) paInput[i];
    }
/* ((ENVIRON*) pabs[INS].csound)->NotifyThreadLock(pabs[INS].csound,
                                                   pabs[INS].clientLock); */

    for (i = 0, n = pabs[OUTS].actualBufferSampleCount; i < n; i++) {
      paOutput[i] = (float) pabs[OUTS].actualBuffer[i];
    }
    ((ENVIRON *) pabs[OUTS].csound)->NotifyThreadLock(pabs[OUTS].csound,
                                                      pabs[OUTS].clientLock);
    return paContinue;
}

void paBlockingRead(PA_BLOCKING_STREAM * pabs, int samples, MYFLT * buffer)
{
    size_t  i;
    ENVIRON *csound = pabs->csound;
    int     chns = csound->nchnls;

    for (i = 0; i < (unsigned int) samples; i++, pabs->currentIndex++) {
      if (pabs->currentIndex >= pabs->actualBufferSampleCount) {
        /* csound->NotifyThreadLock(csound, pabs->paLock);
           csound->WaitThreadLock(csound, pabs->clientLock, 500); */
        pabs->currentIndex = 0;
      }
      buffer[i] = pabs->actualBuffer[pabs->currentIndex];
      if (chns == 1)
        pabs->currentIndex++;
    }
}

int paBlockingWriteStreamCallback(const void *input,
                                    void *output, unsigned long frameCount,
                                    const PaStreamCallbackTimeInfo * timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData)
{
    size_t  i;
    size_t  n;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *) userData;
    float  *paOutput = (float *) output;

    if (!pabs)
      return paContinue;
    if (!pabs->paStream)
      return paContinue;
    ((ENVIRON *) pabs->csound)->WaitThreadLock(pabs->csound, pabs->paLock, 500);
    for (i = 0, n = pabs[OUTS].actualBufferSampleCount; i < n; i++) {
      paOutput[i] = (float) pabs[OUTS].actualBuffer[i];
    }
    ((ENVIRON *) pabs->csound)->NotifyThreadLock(pabs->csound,
                                                 pabs->clientLock);
    return paContinue;
}

void paBlockingWrite(PA_BLOCKING_STREAM * pabs, int samples, MYFLT * buffer)
{
    size_t  i;
    ENVIRON *csound = pabs->csound;
    int     chns = csound->nchnls;

    if (!pabs || !pabs->actualBuffer)
      return;
    for (i = 0; i < (unsigned int) samples; i++, pabs->currentIndex++) {
      if (pabs->currentIndex >= pabs->actualBufferSampleCount) {
        csound->NotifyThreadLock(csound, pabs->paLock);
        csound->WaitThreadLock(csound, pabs->clientLock, 500);
        pabs->currentIndex = 0;
      }
      pabs->actualBuffer[pabs->currentIndex] = buffer[i];
      if (chns == 1)
        pabs->actualBuffer[++(pabs->currentIndex)] = buffer[i];
    }
}

/* VL: these next two functions are not called at the moment  */
int paBlockingReadOpen(ENVIRON * csound,
                         PA_BLOCKING_STREAM * pabs,
                         PaStreamParameters * paParameters,
                         csRtAudioParams * parm)
{
    PaError paError = -1;
    unsigned long buf_size = 0;

    pabs[INS].csound = csound;
    pabs[INS].actualBuffer = (MYFLT *)
        ((ENVIRON *) csound)->Calloc(csound, pabs[INS].actualBufferSampleCount
                                     * sizeof(MYFLT));
    pabs[INS].paLock = csound->CreateThreadLock(csound);
    pabs[INS].clientLock = csound->CreateThreadLock(csound);
    buf_size = parm->bufSamp_SW;
    pabs[INS].actualBufferSampleCount = buf_size * csound->GetNchnls(csound);
    memcpy(&pabs[INS].paParameters, paParameters, sizeof(PaStreamParameters));
    csound->Message(csound,
                    "paBlockingReadOpen: nchnls %d sr %f buf_size %lu\n",
                    pabs[INS].paParameters.channelCount, csound->GetSr(csound),
                    buf_size);
    paError =
        Pa_OpenStream(&pabs[INS].paStream, &pabs[INS].paParameters, 0,
                      (double) csound->GetSr(csound), buf_size, paNoFlag,
                      paBlockingReadStreamCallback, pabs);
    if (paError == paNoError) {
      paError = Pa_StartStream(pabs[INS].paStream);
    }
    if (paError != paNoError) {
      ((ENVIRON *) csound)->Free(csound, pabs[INS].actualBuffer);
      pabs[INS].actualBuffer = NULL;
    }
    return paError;
}

int paBlockingReadStreamCallback(const void *input,
                                   void *output,
                                   unsigned long frameCount,
                                   const PaStreamCallbackTimeInfo * timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void *userData)
{
    size_t  i, n;
    float  *paInput = (float *) input;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM *) userData;

    ((ENVIRON *) pabs->csound)->WaitThreadLock(pabs->csound, pabs->paLock, 500);
    for (i = 0, n = pabs[INS].actualBufferSampleCount; i < n; i++) {
      pabs[INS].actualBuffer[i] = (MYFLT) paInput[i];
    }
    ((ENVIRON *) pabs->csound)->NotifyThreadLock(pabs->csound,
                                                 pabs->clientLock);
    return paContinue;
}

void paBlockingClose(void *csound, PA_BLOCKING_STREAM * pabs)
{
    int    *openOnce;

    if (pabs) {
#ifndef __MACH__
      if (pabs->paStream) {
        /* OSX seems to get stuck here sometimes */
        Pa_CloseStream(pabs->paStream);
        Pa_Sleep(500);
      }
#endif
      ((ENVIRON *) csound)->Free(csound, pabs[OUTS].actualBuffer);
      pabs[OUTS].actualBuffer = NULL;
      openOnce = (int *) ((ENVIRON *) csound)->QueryGlobalVariable(csound,
                                                                   "openOnce");
      if (!*openOnce)
        ((ENVIRON *) csound)->Free(csound, pabs[INS].actualBuffer);
      pabs[INS].actualBuffer = NULL;
    }
}
