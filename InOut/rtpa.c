/*
    rtpa.c:

    Copyright (C) 2004, 2005 John ffitch, Istvan Varga, Victor Lazzarini

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

#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"
#include "pa_blocking.h"
#include <portaudio.h>

#ifdef Str
#undef Str
#endif
#define Str(x)  (((ENVIRON*) csound)->LocalizeString(x))

#define INS 1
#define OUTS 0

typedef struct PaAlsaStreamInfo {
    unsigned long size;
    int /*PaHostApiTypeId */ hostApiType;
    unsigned long version;
    const char *deviceString;
} PaAlsaStreamInfo;

typedef struct devparams_ {
    PaStream    *handle;        /* stream handle                    */
    void        *buf;           /* sample conversion buffer         */
    int         sampleSize;     /* MYFLT sample frame size in bytes */
    int         nchns;          /* number of channels               */
    /* sample conversion function */
    void        (*conv_func)(int, void*, void*);
} DEVPARAMS;

static void printPortAudioErrorMessage(ENVIRON *csound, int errCode)
{
    csound->Message(csound, Str(" *** PortAudio: error %d: %s\n"),
                            errCode, Pa_GetErrorText((PaError) errCode));
}

static int initPortAudio(ENVIRON *csound)
{
    char  *s;
    int   err;
    /* initialise PortAudio */
    if (!csound->QueryGlobalVariable(csound, "::PortAudio::NeedsTerminate")) {
      err = (int) Pa_Initialize();
      if (err != (int) paNoError) {
        printPortAudioErrorMessage(csound, err);
        return -1;
      }
      csound->CreateGlobalVariable(csound, "::PortAudio::NeedsTerminate", 1);
      /* print PortAudio version */
      if ((s = (char*) Pa_GetVersionText()) != NULL)
        csound->Message(csound, "%s\n", s);
    }
    return 0;
}

void listPortAudioDevices(void *csound)
{
    PaDeviceIndex deviceIndex = 0;
    PaDeviceIndex deviceCount = 0;
    const PaDeviceInfo *paDeviceInfo;

    deviceCount = Pa_GetDeviceCount();
    ((ENVIRON *) csound)->Message(csound, "Found %d PortAudio devices:\n",
                                  deviceCount);
    for (deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
      paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
      if (paDeviceInfo) {
        ((ENVIRON*) csound)->Message(csound, "Device%3d: %s\n",
                                     deviceIndex, paDeviceInfo->name);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Maximum channels in: %7d\n",
                                     paDeviceInfo->maxInputChannels);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Maximum channels out:%7d\n",
                                     paDeviceInfo->maxOutputChannels);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Default sample rate: %11.3f\n",
                                     paDeviceInfo->defaultSampleRate);
      }
    }
}

static int recopen_(void *csound_, csRtAudioParams * parm)
{                                               /* open for audio input */
    struct PaStreamParameters sp;
    ENVIRON *csound = (ENVIRON*) csound_;
    PA_BLOCKING_STREAM *pabs;
    int     oMaxLag;
    PaError paError;

    if (initPortAudio(csound) != 0)
      return -1;
    listPortAudioDevices(csound);

    oMaxLag = parm->bufSamp_HW; /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
    if (parm->devName != NULL && strlen(parm->devName) != 0) {
      csound->Message(csound, Str(" *** PortAudio: must specify "
                                  "a device number, not a name\n"));
      return -1;
    }
    if (parm->devNum == 1024) {
      parm->devNum = sp.device = Pa_GetDefaultInputDevice();
      csound->Message(csound, Str("No PortAudio input device given; "
                                  "defaulting to device %d\n"), parm->devNum);
    }
    else {
      sp.device = parm->devNum;
    }
    sp.hostApiSpecificStreamInfo = NULL;
    sp.suggestedLatency = (double) oMaxLag / (double) parm->sampleRate;
    sp.channelCount = (parm->nChannels < 2 ? 2 : parm->nChannels);
    sp.sampleFormat = paFloat32;
    csound->Message(csound, "Suggested PortAudio latency = %f seconds.\n",
                            sp.suggestedLatency);
    /* VL: we will open the device full-duplex if asked to open it for input */
    pabs = csound->QueryGlobalVariable(csound, "pabsReadWritep");
    paError = paBlockingReadWriteOpen(csound, pabs, &sp, parm);
    if (paError != paNoError) {
      printPortAudioErrorMessage(csound, (int) paError);
      return -1;
    }
    csound->Message(csound, Str("Opened PortAudio full-duplex device  %i.\n"),
                            sp.device);
    *((int*) csound->QueryGlobalVariable(csound, "openOnce")) = 1;
    return 0;
}

static int playopen_(void *csound_, csRtAudioParams * parm)
{                                               /* open for audio output */
    struct PaStreamParameters sp;
    ENVIRON *csound = (ENVIRON*) csound_;
    PA_BLOCKING_STREAM *pabs;
    int     oMaxLag;
    PaError paError;

    if (initPortAudio(csound) != 0)
      return -1;
    listPortAudioDevices(csound);

    oMaxLag = parm->bufSamp_HW; /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
    if (parm->devName != NULL && strlen(parm->devName) != 0) {
      csound->Message(csound, Str(" *** PortAudio: must specify "
                                  "a device number, not a name\n"));
      return -1;
    }
    if (parm->devNum == 1024) {
      parm->devNum = sp.device = Pa_GetDefaultOutputDevice();
      csound->Message(csound, Str("No PortAudio output device given; "
                                  "defaulting to device %d.\n"), sp.device);
    }
    else {
      sp.device = parm->devNum;
    }
    sp.hostApiSpecificStreamInfo = NULL;
    sp.suggestedLatency = (double) oMaxLag / (double) parm->sampleRate;
    sp.channelCount = (parm->nChannels < 2 ? 2 : parm->nChannels);
    sp.sampleFormat = paFloat32;
    if (!*((int*) csound->QueryGlobalVariable(csound, "openOnce"))) {
      csound->Message(csound, "Suggested PortAudio output latency = "
                              "%f seconds.\n", sp.suggestedLatency);

      pabs = (PA_BLOCKING_STREAM *)
          csound->QueryGlobalVariable(csound, "pabsReadWritep");
      paError = paBlockingWriteOpen(csound, pabs, &sp, parm);
      if (paError != paNoError) {
        printPortAudioErrorMessage(csound, (int) paError);
        return -1;
      }
      csound->Message(csound, Str("Opened PortAudio output device %i.\n"),
                              sp.device);
    }
    return 0;
}

/* get samples from ADC */
static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    PA_BLOCKING_STREAM *pabs;
    int     samples = bytes_ / sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM *)
        ((ENVIRON *) csound)->QueryGlobalVariableNoCheck(csound,
                                                         "pabsReadWritep");
    paBlockingRead(&pabs[INS], samples, (MYFLT *) inbuf_);
    return bytes_;
}

/* put samples to DAC  */
static void rtplay_(void *csound, void *outbuf_, int bytes_)
{
    PA_BLOCKING_STREAM *pabs;
    int     samples = bytes_ / sizeof(MYFLT);

    pabs = (PA_BLOCKING_STREAM *)
        ((ENVIRON *) csound)->QueryGlobalVariableNoCheck(csound,
                                                         "pabsReadWritep");
    paBlockingWrite(&pabs[OUTS], samples, (MYFLT *) outbuf_);
}

static void rtclose_(void *csound)
{                             /* close the I/O device entirely  */
    /* called only when both complete */
    paBlockingClose(csound,
                    ((ENVIRON*) csound)->QueryGlobalVariable(csound,
                                                             "pabsReadWritep"));
    /* VL: pabsReadWritep holds the whole memory block for pabsWrite &
       pabsRead on full-duplex */
    ((ENVIRON*) csound)->DestroyGlobalVariable(csound, "pabsReadWritep");
    ((ENVIRON*) csound)->DestroyGlobalVariable(csound, "openOnce");
}

/* sample conversion routines */

static void float_to_float(int nSmps, float *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

static void double_to_float(int nSmps, double *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++);
}

static void float_to_double(int nSmps, float *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (double) *(inBuf++);
}

/* select sample conversion routine; returns function pointer */

static void *set_format(int play, int myflt_is_double)
{
    if (play) {
      if (myflt_is_double)
        return (void*) double_to_float;
      else
        return (void*) float_to_float;
    }
    else {
      if (myflt_is_double)
        return (void*) float_to_double;
      else
        return (void*) float_to_float;
    }
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
      csound->Message(csound,
                      Str(" *** PortAudio: no %s devices are available\n"),
                      (play ? "output" : "input"));
      return 0;
    }
    if (!print_list)
      return j;
    csound->Message(csound, Str("PortAudio: available %s devices:\n"),
                            (play ? "output" : "input"));
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
        csound->Message(csound, Str(" *** PortAudio: %s device number %d is "
                                    "out of range\n"),
                                (play ? "output" : "input"), devNum);
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
                            (play ? "output" : "input"), dev_info->name);
    return devNum;
}

/* set up audio device */

static int set_device_params(void *csound, DEVPARAMS *dev,
                             csRtAudioParams *parm, int play)
{
    PaStreamParameters  streamParams;
    ENVIRON             *p;
    int                 err, myflt_is_double;

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
      printPortAudioErrorMessage(p, err);
      return -1;
    }
    /* set up device parameters */
    dev->sampleSize = p->GetSizeOfMYFLT() * parm->nChannels;
    dev->nchns = parm->nChannels;
    myflt_is_double = (p->GetSizeOfMYFLT() == (int) sizeof(double) ? 1 : 0);
    dev->buf = p->Calloc(p, (size_t) (dev->sampleSize * parm->bufSamp_SW));
    dev->conv_func = (void (*)(int, void*, void*))
                       set_format(play, myflt_is_double);
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
    int       n, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtRecordUserData(csound)));
    /* calculate the number of samples to record */
    n = bytes_ / dev->sampleSize;
    err = (int) Pa_ReadStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (p->GetMessageLevel(csound) & 4))
      p->Message(csound,
                 Str("WARNING: buffer overrun in real-time audio input\n"));
    /* convert samples to MYFLT */
    dev->conv_func(n * dev->nchns, dev->buf, inbuf_);
    return bytes_;
}

/* put samples to DAC */

static void rtplay_blocking(void *csound, void *outbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       n, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (*(p->GetRtPlayUserData(csound)));
    /* calculate the number of samples to play */
    n = bytes_ / dev->sampleSize;
    /* convert samples from MYFLT */
    dev->conv_func(n * dev->nchns, outbuf_, dev->buf);
    err = (int) Pa_WriteStream(dev->handle, dev->buf, (unsigned long) n);
    if (err != (int) paNoError && (p->GetMessageLevel(csound) & 4))
      p->Message(csound,
                 Str("WARNING: buffer underrun in real-time audio output\n"));
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
      /* memory for PA_BLOCKING_STREAM dataspace for both input and output */
      p->Message(csound, "using callback interface\n");
      p->CreateGlobalVariable(csound, "pabsReadWritep",
                                      sizeof(PA_BLOCKING_STREAM) * 2);
      p->CreateGlobalVariable(csound, "openOnce", sizeof(int));
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

