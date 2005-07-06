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

/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
    Uses PortAudio library without callbacks -- JPff

    We will open the device full-duplex if asked to open it for
    input, regardless whether we are asked to open it for output.
    In that case we only open ONCE not twice(for input and output,
    separately) - VL
*/

#include "csdl.h"
#include "soundio.h"
#include <portaudio.h>

typedef struct PaAlsaStreamInfo {
    unsigned long size;
    int /*PaHostApiTypeId */ hostApiType;
    unsigned long version;
    const char *deviceString;
} PaAlsaStreamInfo;

typedef struct devparams_ {
    PaStream    *handle;        /* stream handle                    */
    void        *buf;           /* sample conversion buffer         */
    int         nchns;          /* number of channels               */
} DEVPARAMS;

typedef struct PA_BLOCKING_STREAM_ {
    ENVIRON     *csound;
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
} PA_BLOCKING_STREAM;

static int pa_PrintErrMsg(ENVIRON *csound, const char *fmt, ...)
{
    va_list args;
    csound->MessageS(csound, CSOUNDMSG_ERROR, Str(" *** PortAudio: error: "));
    va_start(args, fmt);
    csound->MessageV(csound, CSOUNDMSG_ERROR, fmt, args);
    va_end(args);
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
    return -1;
}

static int initPortAudio(ENVIRON *csound)
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

static int listPortAudioDevices_blocking(ENVIRON *csound,
                                         int print_list, int play)
{
    PaDeviceInfo  *dev_info;
    int           i, j, ndev;

    ndev = (int) Pa_GetDeviceCount();
    for (i = j = 0; i < ndev; i++) {
      dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) i);
      if ((play && dev_info->maxOutputChannels > 0) ||
          (!play && dev_info->maxInputChannels > 0))
        j++;
    }
    if (!j) {
      pa_PrintErrMsg(csound, Str("no %s devices are available\n"),
                             (play ? Str("output") : Str("input")));
      return 0;
    }
    if (!print_list)
      return j;
    csound->Message(csound, Str("PortAudio: available %s devices:\n"),
                            (play ? Str("output") : Str("input")));
    for (i = j = 0; i < ndev; i++) {
      dev_info = (PaDeviceInfo*) Pa_GetDeviceInfo((PaDeviceIndex) i);
      if ((play && dev_info->maxOutputChannels > 0) ||
          (!play && dev_info->maxInputChannels > 0)) {
        csound->Message(csound, " %3d: %s\n", j, dev_info->name);
        j++;
      }
    }
    return j;
}

/* select PortAudio device; returns the actual device number */

static int selectPortAudioDevice(ENVIRON *csound, int devNum, int play)
{
    PaDeviceInfo  *dev_info;
    int           i, j, maxNum;

    maxNum = listPortAudioDevices_blocking(csound, 0, play) - 1;
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
        listPortAudioDevices_blocking(csound, 1, play);
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
    csound->Message(csound, Str("PortAudio: selected %s device '%s'\n"),
                            (play ? Str("output") : Str("input")),
                            dev_info->name);
    return devNum;
}

static int pa_SetStreamParameters(ENVIRON *csound, PaStreamParameters *sp,
                                                   csRtAudioParams *parm,
                                                   int is_playback)
{
    int dev;

    memset(sp, 0, sizeof(PaStreamParameters));
    if (parm->devName != NULL && parm->devName[0] != '\0') {
      return pa_PrintErrMsg(csound,
                            Str("must specify a device number, not a name"));
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

static void rtclose_(void *csound);
static int  paBlockingReadWriteStreamCallback(const void *, void *,
                                              unsigned long,
                                              const PaStreamCallbackTimeInfo *,
                                              PaStreamCallbackFlags,
                                              void *);

static int paBlockingReadWriteOpen(ENVIRON *csound)
{
    PA_BLOCKING_STREAM  *pabs;
    PaError             err;

    pabs = (PA_BLOCKING_STREAM*) csound->QueryGlobalVariable(csound,
                                                             "_rtpaGlobals");
    if (pabs == NULL)
      return -1;
    if (initPortAudio(csound) != 0)
      goto err_return;

    if ((int) Pa_GetDeviceCount() <= 0) {
      pa_PrintErrMsg(csound, Str("no sound device is available"));
      goto err_return;
    }

    if (pabs->mode & 1) {
      if (pa_SetStreamParameters(csound, &(pabs->inputPaParameters),
                                         &(pabs->inParm), 0) != 0)
        goto err_return;
      pabs->inBufSamples = pabs->inParm.bufSamp_SW * pabs->inParm.nChannels;
      pabs->inputBuffer = (float*) calloc((size_t) pabs->inBufSamples,
                                          sizeof(float));
      if (pabs->inputBuffer == NULL) {
        pa_PrintErrMsg(csound, Str("memory allocation failure"));
        goto err_return;
      }
    }
    if (pabs->mode & 2) {
      if (pa_SetStreamParameters(csound, &(pabs->outputPaParameters),
                                         &(pabs->outParm), 1) != 0)
        goto err_return;
      pabs->outBufSamples = pabs->outParm.bufSamp_SW * pabs->outParm.nChannels;
      pabs->outputBuffer = (float*) calloc((size_t) pabs->outBufSamples,
                                           sizeof(float));
      if (pabs->outputBuffer == NULL) {
        pa_PrintErrMsg(csound, Str("memory allocation failure"));
        goto err_return;
      }
    }
    if ((pabs->mode & 3) == 3) {
      if (pabs->inParm.bufSamp_SW != pabs->outParm.bufSamp_SW) {
        pa_PrintErrMsg(csound, Str("inconsistent full-duplex buffer sizes"));
        goto err_return;
      }
      if (pabs->inParm.sampleRate != pabs->outParm.sampleRate) {
        pa_PrintErrMsg(csound, Str("inconsistent full-duplex sample rates"));
        goto err_return;
      }
    }
    pabs->paLock = csound->CreateThreadLock(csound);
    if (pabs->paLock == NULL)
      goto err_return;
    pabs->clientLock = csound->CreateThreadLock(csound);
    if (pabs->clientLock == NULL)
      goto err_return;

    err = Pa_OpenStream(&(pabs->paStream),
                        (pabs->mode & 1 ? &(pabs->inputPaParameters)
                                          : (PaStreamParameters*) NULL),
                        (pabs->mode & 2 ? &(pabs->outputPaParameters)
                                          : (PaStreamParameters*) NULL),
                        (double) (pabs->mode & 2 ? pabs->outParm.sampleRate
                                                   : pabs->inParm.sampleRate),
                        (unsigned long) (pabs->mode & 2 ?
                                         pabs->outParm.bufSamp_SW
                                         : pabs->inParm.bufSamp_SW),
                        (PaStreamFlags) paNoFlag,
                        paBlockingReadWriteStreamCallback,
                        (void*) pabs);
    if (err != paNoError) {
      pabs->paStream = NULL;
      pa_PrintErrMsg(csound, "%d: %s", (int) err, Pa_GetErrorText(err));
      goto err_return;
    }

    err = Pa_StartStream(pabs->paStream);
    if (err != paNoError) {
      pa_PrintErrMsg(csound, "%d: %s", (int) err, Pa_GetErrorText(err));
      goto err_return;
    }

    return 0;

    /* clean up and report error */
 err_return:
    rtclose_(csound);
    return -1;
}

static int paBlockingReadWriteStreamCallback(const void *input,
                                             void *output,
                                             unsigned long frameCount,
                                             const PaStreamCallbackTimeInfo
                                                 *timeInfo,
                                             PaStreamCallbackFlags statusFlags,
                                             void *userData)
{
    int     i, n;
    PA_BLOCKING_STREAM *pabs = (PA_BLOCKING_STREAM*) userData;
    ENVIRON *csound = pabs->csound;
    float   *paInput = (float*) input;
    float   *paOutput = (float*) output;

    if (!pabs)
      return paContinue;

    if (pabs->paStream == NULL) {
      if (pabs->mode & 2) {
        for (i = 0, n = pabs->outBufSamples; i < n; i++)
          paOutput[i] = 0.0f;
      }
      return paContinue;
    }

    if (!pabs->noPaLock)
      csound->WaitThreadLock(csound, pabs->paLock, (size_t) 500);

    if (pabs->mode & 1) {
      for (i = 0, n = pabs->inBufSamples; i < n; i++)
        pabs->inputBuffer[i] = paInput[i];
    }
    if (pabs->mode & 2) {
      for (i = 0, n = pabs->outBufSamples; i < n; i++)
        paOutput[i] = pabs->outputBuffer[i];
    }

    csound->NotifyThreadLock(csound, pabs->clientLock);

    return paContinue;
}

/* get samples from ADC */

static int rtrecord_(void *csound_, void *inbuf_, int bytes_)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    PA_BLOCKING_STREAM  *pabs;
    MYFLT   *buffer = (MYFLT*) inbuf_;
    int     i, chns, samples = bytes_ / (int) sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM*) *(csound->GetRtRecordUserData(csound));
    if (pabs == NULL) {
      memset(inbuf_, 0, bytes_);
      return bytes_;
    }
    if (pabs->paStream == NULL) {
      if (paBlockingReadWriteOpen(csound) != 0)
        csound->Die(csound, Str("Failed to initialise real time audio input"));
    }
    chns = pabs->inParm.nChannels;

    for (i = 0; i < samples; i++, pabs->currentInputIndex++) {
      if (pabs->currentInputIndex >= pabs->inBufSamples) {
        if (pabs->mode == 1) {
          if (!pabs->noPaLock)
            csound->NotifyThreadLock(csound, pabs->paLock);
          csound->WaitThreadLock(csound, pabs->clientLock, 500);
        }
        pabs->currentInputIndex = 0;
      }
      buffer[i] = (MYFLT) pabs->inputBuffer[pabs->currentInputIndex];
      if (chns == 1)
        pabs->currentInputIndex++;
    }

    return bytes_;
}

/* put samples to DAC */

static void rtplay_(void *csound_, void *outbuf_, int bytes_)
{
    ENVIRON *csound = (ENVIRON*) csound_;
    PA_BLOCKING_STREAM  *pabs;
    MYFLT   *buffer = (MYFLT*) outbuf_;
    int     i, chns, samples = bytes_ / (int) sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM*) *(csound->GetRtPlayUserData(csound));
    if (pabs == NULL)
      return;
    if (pabs->paStream == NULL) {
      if (paBlockingReadWriteOpen(csound) != 0)
        csound->Die(csound, Str("Failed to initialise real time audio output"));
    }
    chns = pabs->outParm.nChannels;

    for (i = 0; i < samples; i++, pabs->currentOutputIndex++) {
      if (pabs->currentOutputIndex >= pabs->outBufSamples) {
        if (!pabs->noPaLock)
          csound->NotifyThreadLock(csound, pabs->paLock);
        csound->WaitThreadLock(csound, pabs->clientLock, 500);
        pabs->currentOutputIndex = 0;
      }
      pabs->outputBuffer[pabs->currentOutputIndex] = (float) buffer[i];
      if (chns == 1)
        pabs->outputBuffer[++(pabs->currentOutputIndex)] = (float) buffer[i];
    }
}

static int recopen_(void *csound, csRtAudioParams *parm)
{                                               /* open for audio input */
    ENVIRON *p = (ENVIRON*) csound;
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

    return 0;
}

static int playopen_(void *csound, csRtAudioParams *parm)
{                                               /* open for audio output */
    ENVIRON *p = (ENVIRON*) csound;
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

    return 0;
}

static void rtclose_(void *csound_)     /* close the I/O device entirely */
{
    ENVIRON *csound = (ENVIRON*) csound_;
    PA_BLOCKING_STREAM *pabs;

    pabs = (PA_BLOCKING_STREAM*) csound->QueryGlobalVariable(csound,
                                                             "_rtpaGlobals");
    if (pabs == NULL)
      return;

    if (pabs->paStream != NULL) {
      PaStream  *stream = pabs->paStream;
      pabs->paStream = NULL;
      Pa_Sleep(300);
      if (!pabs->noPaLock)
        csound->NotifyThreadLock(csound, pabs->paLock);
      csound->NotifyThreadLock(csound, pabs->clientLock);
      Pa_Sleep(100);
      Pa_AbortStream(stream);
      if (!pabs->noPaLock)
        csound->NotifyThreadLock(csound, pabs->paLock);
      csound->NotifyThreadLock(csound, pabs->clientLock);
      Pa_CloseStream(stream);
      Pa_Sleep(100);
    }

    if (pabs->clientLock != NULL) {
      csound->DestroyThreadLock(csound, pabs->clientLock);
      pabs->clientLock = NULL;
    }
    if (pabs->paLock != NULL) {
      csound->DestroyThreadLock(csound, pabs->paLock);
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

    *(csound->GetRtRecordUserData(csound)) = NULL;
    *(csound->GetRtPlayUserData(csound)) = NULL;
    csound->DestroyGlobalVariable(csound, "_rtpaGlobals");
}

/* set up audio device */

static int set_device_params(void *csound, DEVPARAMS *dev,
                             csRtAudioParams *parm, int play)
{
    PaStreamParameters  streamParams;
    ENVIRON             *p;
    int                 err;

    p = (ENVIRON*) csound;
    /* set parameters */
    memset(dev, 0, sizeof(DEVPARAMS));
    memset(&streamParams, 0, sizeof(PaStreamParameters));
    streamParams.hostApiSpecificStreamInfo = NULL;
    if (parm->devName != NULL && parm->devName[0] != '\0') {
#if !defined(LINUX)
      listPortAudioDevices_blocking(p, 1, play);
      p->Message(p, Str(" *** PortAudio: "
                        "must specify a device number, not a name\n"));
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
                                paNoFlag, NULL, NULL);
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
    dev->buf = p->Calloc(p, (size_t) (parm->bufSamp_SW * parm->nChannels
                                      * (int) sizeof(float)));

    return 0;
}

/* open for audio input */

static int recopen_blocking(void *csound, csRtAudioParams *parm)
{
    ENVIRON   *p = (ENVIRON*) csound;
    DEVPARAMS *dev;
    int       retval;

    if (initPortAudio(p) != 0)
      return -1;
    /* check if the device is already opened */
    if (*(p->GetRtRecordUserData(csound)) != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) p->Calloc(csound, sizeof(DEVPARAMS));
    *(p->GetRtRecordUserData(csound)) = (void*) dev;
    /* set up parameters and open stream */
    retval = set_device_params(csound, dev, parm, 0);
    if (retval != 0) {
      p->Free(csound, dev);
      *(p->GetRtRecordUserData(csound)) = NULL;
    }
    else
      Pa_StartStream(dev->handle);
    return retval;
}

/* open for audio output */

static int playopen_blocking(void *csound, csRtAudioParams *parm)
{
    ENVIRON   *p = (ENVIRON*) csound;
    DEVPARAMS *dev;
    int       retval;

    if (initPortAudio(p) != 0)
      return -1;
    /* check if the device is already opened */
    if (*(p->GetRtPlayUserData(csound)) != NULL)
      return 0;
    /* allocate structure */
    dev = (DEVPARAMS*) p->Calloc(csound, sizeof(DEVPARAMS));
    *(p->GetRtPlayUserData(csound)) = (void*) dev;
    /* set up parameters and open stream */
    retval = set_device_params(csound, dev, parm, 1);
    if (retval != 0) {
      p->Free(csound, dev);
      *(p->GetRtPlayUserData(csound)) = NULL;
    }
    else
      Pa_StartStream(dev->handle);
    return retval;
}

/* get samples from ADC */

static int rtrecord_blocking(void *csound, void *inbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       i, n, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));
    /* calculate the number of samples to record */
    n = bytes_ / (dev->nchns * (int) sizeof(MYFLT));
    err = (int) Pa_ReadStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (p->GetMessageLevel(csound) & 4))
      p->Warning(csound, Str("buffer overrun in real-time audio input"));
    /* convert samples to MYFLT */
    for (i = 0; i < (n * dev->nchns); i++)
      ((MYFLT*) inbuf_)[i] = (MYFLT) ((float*) dev->buf)[i];

    return bytes_;
}

/* put samples to DAC */

static void rtplay_blocking(void *csound, void *outbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       i, n, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtPlayUserData(csound)));
    /* calculate the number of samples to play */
    n = bytes_ / (dev->nchns * (int) sizeof(MYFLT));
    /* convert samples from MYFLT */
    for (i = 0; i < (n * dev->nchns); i++)
      ((float*) dev->buf)[i] = (float) ((MYFLT*) outbuf_)[i];
    err = (int) Pa_WriteStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (p->GetMessageLevel(csound) & 4))
      p->Warning(csound, Str("buffer underrun in real-time audio output"));
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_blocking(void *csound)
{
    DEVPARAMS *dev;
    ENVIRON   *p;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));
    if (dev != NULL) {
      *(p->GetRtRecordUserData(csound)) = NULL;
      if (dev->handle != NULL) {
        Pa_StopStream(dev->handle);
        Pa_CloseStream(dev->handle);
      }
      if (dev->buf != NULL)
        p->Free(csound, dev->buf);
      p->Free(csound, dev);
    }
    dev = (DEVPARAMS*) (*(p->GetRtPlayUserData(csound)));
    if (dev != NULL) {
      *(p->GetRtPlayUserData(csound)) = NULL;
      if (dev->handle != NULL) {
        Pa_StopStream(dev->handle);
        Pa_CloseStream(dev->handle);
      }
      if (dev->buf != NULL)
        p->Free(csound, dev->buf);
      p->Free(csound, dev);
    }
}

/* module interface functions */

PUBLIC int csoundModuleCreate(void *csound)
{
    ENVIRON *p = (ENVIRON*) csound;
    /* nothing to do, report success */
    p->Message(csound, Str("PortAudio real-time audio module for Csound\n"));
    return 0;
}

PUBLIC int csoundModuleInit(void *csound)
{
    ENVIRON *p = (ENVIRON*) csound;
    char    *s, drv[12];
    int     i;

    if ((s = (char*) p->QueryGlobalVariable(p, "_RTAUDIO")) == NULL)
      return 0;
    for (i = 0; s[i] != '\0' && i < 11; i++)
      drv[i] = s[i] & (char) 0xDF;
    drv[i] = '\0';
    if (!(strcmp(drv, "PORTAUDIO") == 0 || strcmp(drv, "PA") == 0 ||
          strcmp(drv, "PA_BL") == 0 || strcmp(drv, "PA_CB") == 0))
      return 0;
    p->Message(csound, "rtaudio: PortAudio module enabled ... ");
    /* set function pointers */
#ifdef LINUX
    if (strcmp(drv, "PA_CB") != 0)
#else
    if (strcmp(drv, "PA_BL") == 0)
#endif
    {
      p->Message(csound, "using blocking interface\n");
      p->SetPlayopenCallback(csound, playopen_blocking);
      p->SetRecopenCallback(csound, recopen_blocking);
      p->SetRtplayCallback(csound, rtplay_blocking);
      p->SetRtrecordCallback(csound, rtrecord_blocking);
      p->SetRtcloseCallback(csound, rtclose_blocking);
    }
    else {
      p->Message(csound, "using callback interface\n");
      p->SetPlayopenCallback(csound, playopen_);
      p->SetRecopenCallback(csound, recopen_);
      p->SetRtplayCallback(csound, rtplay_);
      p->SetRtrecordCallback(csound, rtrecord_);
      p->SetRtcloseCallback(csound, rtclose_);
    }
    return 0;
}

PUBLIC int csoundModuleDestroy(void *csound)
{
    ENVIRON *p = (ENVIRON*) csound;

    if (p->QueryGlobalVariable(csound, "::PortAudio::NeedsTerminate")) {
      p->DestroyGlobalVariable(csound, "::PortAudio::NeedsTerminate");
      return ((int) Pa_Terminate() == (int) paNoError ? 0 : -1);
    }
    return 0;
}

