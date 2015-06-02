/*
    rtpa.c:

    Copyright (C) 2004, 2005 John ffitch, Istvan Varga,
                             Michael Gogins, Victor Lazzarini

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

/*                                              RTPA.C for PortAudio    */

#include "csdl.h"
#if !defined(WIN32)
#include "soundio.h"
#endif
#include <portaudio.h>

#define NO_FULLDUPLEX_PA_LOCK   0

typedef struct PaAlsaStreamInfo {
    unsigned long   size;
    PaHostApiTypeId hostApiType;
    unsigned long   version;
    const char      *deviceString;
} PaAlsaStreamInfo;

typedef struct devparams_ {
    PaStream    *handle;        /* stream handle                    */
    float       *buf;           /* sample conversion buffer         */
    int         nchns;          /* number of channels               */
} DEVPARAMS;

typedef struct PA_BLOCKING_STREAM_ {
    CSOUND      *csound;
    PaStream    *paStream;
    int         mode;                   /* 1: rec, 2: play, 3: full-duplex  */
    int         noPaLock;
    int         inBufSamples;
    int         outBufSamples;
    int         currentInputIndex;
    int         currentOutputIndex;
    float       *inputBuffer;
    float       *outputBuffer;
    void        *paLock;                /* thread lock for stream callback  */
    void        *clientLock;            /* thread lock for rtplay/rtrecord  */
    csRtAudioParams inParm;
    csRtAudioParams outParm;
    PaStreamParameters inputPaParameters;
    PaStreamParameters outputPaParameters;
#ifdef WIN32
    int         paused;                 /* VL: to allow for smooth pausing  */
#endif
    int         paLockTimeout;
    int         complete;
} PA_BLOCKING_STREAM;

static int pa_PrintErrMsg(CSOUND *csound, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    csound->ErrMsgV(csound, Str(" *** PortAudio: error: "), fmt, args);
    va_end(args);
    return -1;
}

static int initPortAudio(CSOUND *csound)
{
    char  *s;
    int   err;
    /* initialise PortAudio */
    if (!csound->QueryGlobalVariable(csound, "::PortAudio::NeedsTerminate")) {
      if (csound->CreateGlobalVariable(csound,
                                       "::PortAudio::NeedsTerminate", 1) != 0)
        return -1;
      err = (int) Pa_Initialize();
      if (err != (int) paNoError) {
        return pa_PrintErrMsg(csound, "%d: %s",
                                      err, Pa_GetErrorText((PaError) err));
      }
      /* print PortAudio version */
      if ((s = (char*) Pa_GetVersionText()) != NULL)
        csound->Message(csound, "%s\n", s);
    }
    return 0;
}

/* list available input or output devices; returns the number of devices */
int listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int isOutput){
     PaDeviceInfo  *dev_info;
    int           i, j, ndev;
    char          tmp[256], *s;

    if (initPortAudio(csound) != 0)
      return 0;

     if ((s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL)
      return 0;

    ndev = (int) Pa_GetDeviceCount();
    for (i = j = 0; i < ndev; i++) {
      dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) i);
      if ((isOutput && dev_info->maxOutputChannels > 0) ||
          (!isOutput && dev_info->maxInputChannels > 0))
        j++;
    }
    if (!j) return 0;
    if(list!=NULL) {
    for (i = j = 0; i < ndev; i++) {
      dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) i);
      if ((isOutput && dev_info->maxOutputChannels > 0) ||
          (!isOutput && dev_info->maxInputChannels > 0)) {
        strncpy(list[j].device_name, dev_info->name, 63);
        if (isOutput) {
          snprintf(tmp, 256, "dac%d", j);
        } else {
          snprintf(tmp, 256, "adc%d", j);
        }
        strncpy(list[j].device_id, tmp, 63);
        strncpy(list[j].rt_module, s, 63);
        list[j].max_nchnls =
          isOutput ?  dev_info->maxOutputChannels : dev_info->maxInputChannels;
        list[j].isOutput = isOutput;
        j++;
      }
    }
    }
    return j;
}


static int listPortAudioDevices_blocking(CSOUND *csound,
                                         int print_list, int play)
{
    int i,n = listDevices(csound, NULL, play);
    CS_AUDIODEVICE *devs =
      (CS_AUDIODEVICE *) csound->Malloc(csound, n*sizeof(CS_AUDIODEVICE));
    listDevices(csound, devs, play);
    for(i=0; i < n; i++)
      csound->Message(csound, " %3d: %s (%s)\n",
                      i, devs[i].device_id, devs[i].device_name);
    csound->Free(csound, devs);
    return n;
}




/* select PortAudio device; returns the actual device number */

static int selectPortAudioDevice(CSOUND *csound, int devNum, int play)
{
    PaDeviceInfo  *dev_info;
    int           i, j, maxNum;

    maxNum = listPortAudioDevices_blocking(csound, 1, play) - 1;
    if (maxNum < 0)
      return -1;
    if (devNum == 1024) {
      if (play)
        devNum = (int) Pa_GetDefaultOutputDevice();
      else
        devNum = (int) Pa_GetDefaultInputDevice();
    }
    else {
      if (devNum < 0 || devNum > maxNum) {
        /* listPortAudioDevices_blocking(csound, 1, play); */
        pa_PrintErrMsg(csound, Str("%s device number %d is out of range"),
                               (play ? Str("output") : Str("input")), devNum);
        return -1;
      }
      for (i = j = 0; j <= maxNum; i++) {
        dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) i);
        if ((play && dev_info->maxOutputChannels > 0) ||
            (!play && dev_info->maxInputChannels > 0)) {
          if (j == devNum)
            break;
          j++;
        }
      }
      devNum = i;
    }
    dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) devNum);
    if (dev_info)
      csound->Message(csound, Str("PortAudio: selected %s device '%s'\n"),
                      (play ? Str("output") : Str("input")),
                      dev_info->name);
    else
      csound->Message(csound, Str("PortAudio: failed to obtain device info.\n"));

    if(play) {
      csound->system_sr(csound, (MYFLT) dev_info->defaultSampleRate);
    }

    return devNum;
}

static int pa_SetStreamParameters(CSOUND *csound, PaStreamParameters *sp,
                                                  csRtAudioParams *parm,
                                                  int is_playback)
{
    int dev;

    memset(sp, 0, sizeof(PaStreamParameters));
    if (parm->devName != NULL && parm->devName[0] != '\0') {
      return pa_PrintErrMsg(csound,
                            Str("Must specify a device number, not a name"));
    }
    dev = selectPortAudioDevice(csound, parm->devNum, is_playback);
    if (dev < 0)
      return -1;
    sp->device = (PaDeviceIndex) dev;
    sp->channelCount = (parm->nChannels < 2 ? 2 : parm->nChannels);
    sp->sampleFormat = (PaSampleFormat) paFloat32;
    sp->suggestedLatency = (PaTime) ((double) parm->bufSamp_HW
                                     / (double) parm->sampleRate);
    sp->hostApiSpecificStreamInfo = NULL;
    return 0;
}

static void rtclose_(CSOUND *csound);
static int  paBlockingReadWriteStreamCallback(const void *, void *,
                                              unsigned long,
                                              const PaStreamCallbackTimeInfo *,
                                              PaStreamCallbackFlags,
                                              void *);

static int paBlockingReadWriteOpen(CSOUND *csound)
{
    PA_BLOCKING_STREAM  *pabs;
    PaStream            *stream = NULL;
    PaError             err;

    pabs = (PA_BLOCKING_STREAM*) csound->QueryGlobalVariable(csound,
                                                             "_rtpaGlobals");
    if (pabs == NULL)
      return -1;
   if (initPortAudio(csound) != 0)
      goto err_return;

    if ((int) Pa_GetDeviceCount() <= 0) {
      pa_PrintErrMsg(csound, Str("No sound device is available"));
      goto err_return;
    }

    if (pabs->mode & 1) {
      if (pa_SetStreamParameters(csound, &(pabs->inputPaParameters),
                                         &(pabs->inParm), 0) != 0)
        goto err_return;
      pabs->inBufSamples = pabs->inParm.bufSamp_SW
                           * (int) pabs->inputPaParameters.channelCount;
      pabs->inputBuffer = (float*) calloc((size_t) pabs->inBufSamples,
                                          sizeof(float));
      if (pabs->inputBuffer == NULL) {
        pa_PrintErrMsg(csound, Str("Memory allocation failure"));
        goto err_return;
      }
    }
    if (pabs->mode & 2) {
      if (pa_SetStreamParameters(csound, &(pabs->outputPaParameters),
                                         &(pabs->outParm), 1) != 0)
        goto err_return;
      pabs->outBufSamples = pabs->outParm.bufSamp_SW
                            * (int) pabs->outputPaParameters.channelCount;
      pabs->outputBuffer = (float*) calloc((size_t) pabs->outBufSamples,
                                           sizeof(float));
      if (pabs->outputBuffer == NULL) {
        pa_PrintErrMsg(csound, Str("Memory allocation failure"));
        goto err_return;
      }
    }
    if ((pabs->mode & 3) == 3) {
      if (pabs->inParm.bufSamp_SW != pabs->outParm.bufSamp_SW) {
        pa_PrintErrMsg(csound, Str("Inconsistent full-duplex buffer sizes"));
        goto err_return;
      }
      if (pabs->inParm.sampleRate != pabs->outParm.sampleRate) {
        pa_PrintErrMsg(csound, Str("Inconsistent full-duplex sample rates"));
        goto err_return;
      }
      if (UNLIKELY(((pabs->inParm.bufSamp_SW / csound->GetKsmps(csound)) *
                    csound->GetKsmps(csound)) != pabs->inParm.bufSamp_SW))
        csound->MessageS(csound,
                         CSOUNDMSG_WARNING,
                         Str("WARNING: buffer size should be an integer "
                             "multiple of ksmps in full-duplex mode\n"));
#if NO_FULLDUPLEX_PA_LOCK
      pabs->noPaLock = 1;
#endif
    }

    pabs->paLock = csound->CreateThreadLock();
    if (pabs->paLock == NULL)
      goto err_return;
    pabs->clientLock = csound->CreateThreadLock();
    if (pabs->clientLock == NULL)
      goto err_return;
#if NO_FULLDUPLEX_PA_LOCK
    if (!pabs->noPaLock)
#endif
      csound->WaitThreadLock(pabs->paLock, (size_t) 500);
    csound->WaitThreadLock(pabs->clientLock, (size_t) 500);

    pabs->paLockTimeout =
        (int) (1.33 * (pabs->outputPaParameters.suggestedLatency
                         >= pabs->inputPaParameters.suggestedLatency ?
                       pabs->outputPaParameters.suggestedLatency
                       : pabs->inputPaParameters.suggestedLatency));
    if (pabs->paLockTimeout < 25)
      pabs->paLockTimeout = 25;
    else if (pabs->paLockTimeout > 1000)
      pabs->paLockTimeout = 1000;


    err = Pa_OpenStream(&stream,
                        (pabs->mode & 1 ? &(pabs->inputPaParameters)
                                          : (PaStreamParameters*) NULL),
                        (pabs->mode & 2 ? &(pabs->outputPaParameters)
                                          : (PaStreamParameters*) NULL),
                        (double) (pabs->mode & 2 ? pabs->outParm.sampleRate
                                                   : pabs->inParm.sampleRate),
                        (unsigned long) (pabs->mode & 2 ?
                                         pabs->outParm.bufSamp_SW
                                         : pabs->inParm.bufSamp_SW),
                        (csound->GetDitherMode(csound) ?
                         (PaStreamFlags) paNoFlag : (PaStreamFlags) paDitherOff),
                        paBlockingReadWriteStreamCallback,
                        (void*) pabs);
    if (err != paNoError) {
      pa_PrintErrMsg(csound, "%d: %s", (int) err, Pa_GetErrorText(err));
      goto err_return;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
      Pa_CloseStream(stream);
      pa_PrintErrMsg(csound, "%d: %s", (int) err, Pa_GetErrorText(err));
      goto err_return;
    }

    pabs->paStream = stream;
    return 0;

    /* clean up and report error */
 err_return:
    rtclose_(csound);
    return -1;
}

static CS_NOINLINE void paClearOutputBuffer(PA_BLOCKING_STREAM *pabs,
                                            float *buf)
{
    int   nsmps = pabs->outBufSamples;
    memset(buf,0, nsmps*sizeof(float));
    /* int   i = 0; */
    /* do { */
    /*   buf[i] = 0.0f; */
    /* } while (++i < nsmps); */
}

static int paBlockingReadWriteStreamCallback(const void *input,
                                             void *output,
                                             unsigned long frameCount,
                                             const PaStreamCallbackTimeInfo
                                                 *timeInfo,
                                             PaStreamCallbackFlags statusFlags,
                                             void *userData)
{
    int     err;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM*) userData;
    CSOUND  *csound = pabs->csound;
    float   *paInput = (float*) input;
    float   *paOutput = (float*) output;


    if (pabs->complete == 1) {
        return paComplete;
    }

#ifdef WIN32
if (pabs->paStream == NULL
      || pabs->paused
) {
if (pabs->mode & 2)
        paClearOutputBuffer(pabs, paOutput);
return paContinue;
}
#endif

#if NO_FULLDUPLEX_PA_LOCK
    err = 0;
    if (!pabs->noPaLock)
#endif
      /*#ifndef __MACH__*/
      /*#  ifdef WIN32 */
      err = csound->WaitThreadLock(pabs->paLock, (size_t) pabs->paLockTimeout);
    /*#  else
    err = csound->WaitThreadLock(pabs->paLock, (size_t) 500);
#  endif
    #else
      csound->WaitThreadLock(pabs->paLock, (size_t) 500);
      err = 0;
     #endif*/

    if (pabs->mode & 1) {
      int n = pabs->inBufSamples;
      int i = 0;
      do {
        pabs->inputBuffer[i] = paInput[i];
      } while (++i < n);
    }
    if (pabs->mode & 2) {
      if (!err) {
        int n = pabs->outBufSamples;
        int i = 0;
        do {
          paOutput[i] = pabs->outputBuffer[i];
        } while (++i < n);
      }
      else {
#ifdef WIN32
        pabs->paused = err;
#endif
        paClearOutputBuffer(pabs, paOutput);
      }
    }

   paClearOutputBuffer(pabs, pabs->outputBuffer);
   csound->NotifyThreadLock(pabs->clientLock);

    return paContinue;
}

/* get samples from ADC */

static int rtrecord_(CSOUND *csound, MYFLT *buffer, int nbytes)
{
    PA_BLOCKING_STREAM  *pabs;
    int     i = 0, samples = nbytes / (int) sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM*) *(csound->GetRtRecordUserData(csound));
    if (pabs == NULL) {
      memset(buffer, 0, nbytes);
      return nbytes;
    }
    if (pabs->paStream == NULL) {
      if (paBlockingReadWriteOpen(csound) != 0)
        csound->Die(csound, Str("Failed to initialise real time audio input"));
    }

    do {
      buffer[i] = (MYFLT) pabs->inputBuffer[pabs->currentInputIndex++];
      if (pabs->inParm.nChannels == 1)
        pabs->currentInputIndex++;
      if (pabs->currentInputIndex >= pabs->inBufSamples) {
        if (pabs->mode == 1) {
#if NO_FULLDUPLEX_PA_LOCK
          if (!pabs->noPaLock)
#endif
            csound->NotifyThreadLock(pabs->paLock);
          csound->WaitThreadLock(pabs->clientLock, (size_t) 500);
        }
        pabs->currentInputIndex = 0;
      }
    } while (++i < samples);

    return nbytes;
}

/* put samples to DAC */

static void rtplay_(CSOUND *csound, const MYFLT *buffer, int nbytes)
{
    PA_BLOCKING_STREAM  *pabs;
    int     i = 0, samples = nbytes / (int) sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM*) *(csound->GetRtPlayUserData(csound));
    if (pabs == NULL)
      return;
#ifdef WIN32
    pabs->paused = 0;
#endif

    do {
      pabs->outputBuffer[pabs->currentOutputIndex++] = (float) buffer[i];
      if (pabs->outParm.nChannels == 1)
        pabs->outputBuffer[pabs->currentOutputIndex++] = (float) buffer[i];
      if (pabs->currentOutputIndex >= pabs->outBufSamples) {
#if NO_FULLDUPLEX_PA_LOCK
        if (!pabs->noPaLock)
#endif
          csound->NotifyThreadLock(pabs->paLock);
        csound->WaitThreadLock(pabs->clientLock, (size_t) 500);
        pabs->currentOutputIndex = 0;
      }
    } while (++i < samples);
}

/* open for audio input */

static int recopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    CSOUND *p = csound;
    PA_BLOCKING_STREAM *pabs;

    pabs = (PA_BLOCKING_STREAM*) p->QueryGlobalVariable(p, "_rtpaGlobals");
    if (pabs == NULL) {
      if (p->CreateGlobalVariable(p, "_rtpaGlobals", sizeof(PA_BLOCKING_STREAM))
          != 0)
        return -1;
      pabs = (PA_BLOCKING_STREAM*) p->QueryGlobalVariable(p, "_rtpaGlobals");
      pabs->csound = p;
    }
    pabs->mode |= 1;
    memcpy(&(pabs->inParm), parm, sizeof(csRtAudioParams));
    *(p->GetRtRecordUserData(p)) = (void*) pabs;

    pabs->complete = 0;

    return 0;
}

/* open for audio output */

static int playopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    CSOUND *p = csound;
    PA_BLOCKING_STREAM *pabs;

    pabs = (PA_BLOCKING_STREAM*) p->QueryGlobalVariable(p, "_rtpaGlobals");
    if (pabs == NULL) {
      if (p->CreateGlobalVariable(p, "_rtpaGlobals", sizeof(PA_BLOCKING_STREAM))
          != 0)
        return -1;
      pabs = (PA_BLOCKING_STREAM*) p->QueryGlobalVariable(p, "_rtpaGlobals");
      pabs->csound = p;
    }
    pabs->mode |= 2;
    memcpy(&(pabs->outParm), parm, sizeof(csRtAudioParams));
    *(p->GetRtPlayUserData(p)) = (void*) pabs;

    pabs->complete = 0;

    return (paBlockingReadWriteOpen(p));
}

/* close the I/O device entirely */

static void rtclose_(CSOUND *csound)
{
    PA_BLOCKING_STREAM *pabs;
    pabs = (PA_BLOCKING_STREAM*) csound->QueryGlobalVariable(csound,
                                                             "_rtpaGlobals");

    csound->Message(csound, Str("closing device\n"));
    if (pabs == NULL)
      return;

    pabs->complete = 1;

    if (pabs->paStream != NULL) {
      PaStream  *stream = pabs->paStream;
      unsigned int i;

      for (i = 0; i < 4u; i++) {
#if NO_FULLDUPLEX_PA_LOCK
        if (!pabs->noPaLock)
#endif
          csound->NotifyThreadLock(pabs->paLock);
        csound->NotifyThreadLock(pabs->clientLock);
        //Pa_Sleep(80);
      }

      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      //Pa_Sleep(80);
    }

    if (pabs->clientLock != NULL) {
      csound->NotifyThreadLock(pabs->clientLock);
      csound->DestroyThreadLock(pabs->clientLock);
      pabs->clientLock = NULL;
    }
    if (pabs->paLock != NULL) {
      csound->NotifyThreadLock(pabs->paLock);
      csound->DestroyThreadLock(pabs->paLock);
      pabs->paLock = NULL;
    }

    if (pabs->outputBuffer != NULL) {
      free(pabs->outputBuffer);
      pabs->outputBuffer = NULL;
    }
    if (pabs->inputBuffer != NULL) {
      free(pabs->inputBuffer);
      pabs->inputBuffer = NULL;
    }
    pabs->paStream = NULL;
    *(csound->GetRtRecordUserData(csound)) = NULL;
    *(csound->GetRtPlayUserData(csound)) = NULL;
    csound->DestroyGlobalVariable(csound, "_rtpaGlobals");
}

/* set up audio device */

static int set_device_params(CSOUND *csound, DEVPARAMS *dev,
                             const csRtAudioParams *parm, int play)
{
    PaStreamParameters  streamParams;
    CSOUND              *p = csound;
    int                 err;

    /* set parameters */
    memset(dev, 0, sizeof(DEVPARAMS));
    memset(&streamParams, 0, sizeof(PaStreamParameters));
    streamParams.hostApiSpecificStreamInfo = NULL;
    if (parm->devName != NULL && parm->devName[0] != '\0') {
#if !defined(LINUX)
      listPortAudioDevices_blocking(p, 1, play);
      pa_PrintErrMsg(p, Str("Must specify a device number, not a name"));
      return -1;
#else
      PaAlsaStreamInfo info;
      p->Message(p, Str("PortAudio: using ALSA device '%s'\n"), parm->devName);
      memset(&info, 0, sizeof(PaAlsaStreamInfo));
      info.deviceString = parm->devName;
      info.hostApiType = paALSA;
      info.version = 1;
      info.size = sizeof(info);
      streamParams.device = paUseHostApiSpecificDeviceSpecification;
      streamParams.hostApiSpecificStreamInfo = &info;
#endif
    }
    else {
      int devNum = selectPortAudioDevice(p, parm->devNum, play);
      if (devNum < 0)
        return -1;
      streamParams.device = (PaDeviceIndex) devNum;
    }
    streamParams.channelCount = parm->nChannels;
    streamParams.sampleFormat = paFloat32;
    streamParams.suggestedLatency = (PaTime) ((double) parm->bufSamp_HW
                                              / (double) parm->sampleRate);
    /* open stream */
    if (play) {
      err = (int) Pa_OpenStream(&(dev->handle), NULL, &streamParams,
                                (double) parm->sampleRate,
                                (unsigned long) parm->bufSamp_SW,
                                (csound->GetDitherMode(csound) ?
                                 paNoFlag:paDitherOff),
                                NULL, NULL);
    }
    else {
      err = (int) Pa_OpenStream(&(dev->handle), &streamParams, NULL,
                                (double) parm->sampleRate,
                                (unsigned long) parm->bufSamp_SW,
                                paNoFlag, NULL, NULL);
    }
    if (err != (int) paNoError) {
      pa_PrintErrMsg(p, "%d: %s", err, Pa_GetErrorText((PaError) err));
      return -1;
    }
    /* set up device parameters */
    dev->nchns = parm->nChannels;
    dev->buf = (float*) p->Calloc(p, (size_t) (parm->bufSamp_SW
                                               * parm->nChannels
                                               * (int) sizeof(float)));

    return 0;
}

/* open for audio input */

static int recopen_blocking(CSOUND *csound, const csRtAudioParams *parm)
{
    DEVPARAMS *dev;
    int       retval;

    if (initPortAudio(csound) != 0)
      return -1;
    /* check if the device is already opened */
    if (*(csound->GetRtRecordUserData(csound)) != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) csound->Calloc(csound, sizeof(DEVPARAMS));
    *(csound->GetRtRecordUserData(csound)) = (void*) dev;
    /* set up parameters and open stream */
    retval = set_device_params(csound, dev, parm, 0);
    if (retval != 0) {
      csound->Free(csound, dev);
      *(csound->GetRtRecordUserData(csound)) = NULL;
    }
    else
      Pa_StartStream(dev->handle);
    return retval;
}

/* open for audio output */

static int playopen_blocking(CSOUND *csound, const csRtAudioParams *parm)
{
    DEVPARAMS *dev;
    int       retval;

    if (initPortAudio(csound) != 0)
      return -1;
    /* check if the device is already opened */
    if (*(csound->GetRtPlayUserData(csound)) != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) csound->Calloc(csound, sizeof(DEVPARAMS));
    *(csound->GetRtPlayUserData(csound)) = (void*) dev;
    /* set up parameters and open stream */
    retval = set_device_params(csound, dev, parm, 1);
    if (retval != 0) {
      csound->Free(csound, dev);
      *(csound->GetRtPlayUserData(csound)) = NULL;
    }
    else
      Pa_StartStream(dev->handle);
    return retval;
}

/* get samples from ADC */

static int rtrecord_blocking(CSOUND *csound, MYFLT *inbuf, int nbytes)
{
    DEVPARAMS *dev;
    int       i, n, err;

    dev = (DEVPARAMS*) (*(csound->GetRtRecordUserData(csound)));
    /* calculate the number of samples to record */
    n = nbytes / (dev->nchns * (int) sizeof(MYFLT));
    err = (int) Pa_ReadStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (csound->GetMessageLevel(csound) & 4))
      csound->Warning(csound, Str("Buffer overrun in real-time audio input"));
    /* convert samples to MYFLT */
    for (i = 0; i < (n * dev->nchns); i++)
      inbuf[i] = (MYFLT) dev->buf[i];

    return nbytes;
}

/* put samples to DAC */

static void rtplay_blocking(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    DEVPARAMS *dev;
    int       i, n, err;

    dev = (DEVPARAMS*) (*(csound->GetRtPlayUserData(csound)));
    /* calculate the number of samples to play */
    n = nbytes / (dev->nchns * (int) sizeof(MYFLT));
    /* convert samples from MYFLT */
    for (i = 0; i < (n * dev->nchns); i++)
      dev->buf[i] = (float) outbuf[i];
    err = (int) Pa_WriteStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (csound->GetMessageLevel(csound) & 4))
      csound->Warning(csound, Str("Buffer underrun in real-time audio output"));
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_blocking(CSOUND *csound)
{
    DEVPARAMS *dev;
    csound->Message(csound, Str("closing device\n"));
    dev = (DEVPARAMS*) (*(csound->GetRtRecordUserData(csound)));
    if (dev != NULL) {
      *(csound->GetRtRecordUserData(csound)) = NULL;
      if (dev->handle != NULL) {
        Pa_StopStream(dev->handle);
        Pa_CloseStream(dev->handle);
      }
      if (dev->buf != NULL)
        csound->Free(csound, dev->buf);
      csound->Free(csound, dev);
    }
    dev = (DEVPARAMS*) (*(csound->GetRtPlayUserData(csound)));
    if (dev != NULL) {
      *(csound->GetRtPlayUserData(csound)) = NULL;
      if (dev->handle != NULL) {
        Pa_StopStream(dev->handle);
        Pa_CloseStream(dev->handle);
      }
      if (dev->buf != NULL)
        csound->Free(csound, dev->buf);
      csound->Free(csound, dev);
    }
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
  //csound->Message(csound,
  // Str("PortAudio real-time audio module for Csound\n"));
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *s, drv[12];
    int     i;
    csound->module_list_add(csound, "pa_bl", "audio");
    csound->module_list_add(csound, "pa_cb", "audio");
    if ((s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL)
      return 0;
    for (i = 0; s[i] != '\0' && i < 11; i++)
      drv[i] = s[i] & (char) 0xDF;
    drv[i] = '\0';
    if (!(strcmp(drv, "PORTAUDIO") == 0 || strcmp(drv, "PA") == 0 ||
          strcmp(drv, "PA_BL") == 0 || strcmp(drv, "PA_CB") == 0))
      return 0;
    csound->Message(csound, Str("rtaudio: PortAudio module enabled ... \n"));
    /* set function pointers */
#ifdef LINUX
    if (strcmp(drv, "PA_CB") != 0)
#else
    if (strcmp(drv, "PA_BL") == 0)
#endif
    {
      csound->Message(csound, Str("using blocking interface\n"));
      csound->SetPlayopenCallback(csound, playopen_blocking);
      csound->SetRecopenCallback(csound, recopen_blocking);
      csound->SetRtplayCallback(csound, rtplay_blocking);
      csound->SetRtrecordCallback(csound, rtrecord_blocking);
      csound->SetRtcloseCallback(csound, rtclose_blocking);
      csound->SetAudioDeviceListCallback(csound, listDevices);
    }
    else {
      csound->Message(csound, Str("using callback interface\n"));
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
      csound->SetAudioDeviceListCallback(csound, listDevices);
    }

    csound->module_list_add(csound, s, "audio");
    return 0;
}

PUBLIC int csoundModuleDestroy(CSOUND *csound)
{
    if (csound->QueryGlobalVariable(csound, "::PortAudio::NeedsTerminate")) {
      csound->DestroyGlobalVariable(csound, "::PortAudio::NeedsTerminate");
      return ((int) Pa_Terminate() == (int) paNoError ? 0 : -1);
    }
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
