/*
    rtpa_new.c:

    Copyright (C) 2005 Istvan Varga

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

#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"

#include <portaudio.h>

#ifdef Str
#undef Str
#endif
#define Str(x) (((ENVIRON*) csound)->LocalizeString(x))

typedef struct devparams_ {
    PaStream    *handle;        /* stream handle                    */
    void        *buf;           /* sample conversion buffer         */
    int         sampleSize;     /* MYFLT sample frame size in bytes */
    int         nchns;          /* number of channels               */
    /* sample conversion function */
    void        (*conv_func)(int, void*, void*);
} DEVPARAMS;

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

static void printPortAudioErrorMessage(ENVIRON *csound, int errCode)
{
    csound->Message(csound, Str(" *** PortAudio: error: %s\n"),
                            Pa_GetErrorText((PaError) errCode));
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

static int listPortAudioDevices(ENVIRON *csound, int print_list, int play)
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

    maxNum = listPortAudioDevices(csound, 0, play) - 1;
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
        listPortAudioDevices(csound, 1, play);
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
    int                 err, devNum, myflt_is_double;

    p = (ENVIRON*) csound;
    /* set parameters */
    memset(dev, 0, sizeof(DEVPARAMS));
    memset(&streamParams, 0, sizeof(PaStreamParameters));
    if (parm->devName != NULL) {
      listPortAudioDevices(csound, 1, play);
      p->Message(csound, Str(" *** PortAudio: must specify a device number, "
                             "not a name\n"));
      return -1;
    }
    devNum = selectPortAudioDevice(csound, parm->devNum, play);
    if (devNum < 0)
      return -1;
    streamParams.device = (PaDeviceIndex) devNum;
    streamParams.channelCount = parm->nChannels;
    streamParams.sampleFormat = paFloat32;
    streamParams.suggestedLatency = (PaTime) ((double) parm->bufSamp_HW
                                              / (double) parm->sampleRate);
    streamParams.hostApiSpecificStreamInfo = NULL;
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
    dev->buf = p->Calloc(csound, (size_t) (dev->sampleSize * parm->bufSamp_SW));
    dev->conv_func = (void (*)(int, void*, void*))
                       set_format(play, myflt_is_double);
    return 0;
}

/* open for audio input */

static int recopen_(void *csound, csRtAudioParams *parm)
{
    ENVIRON   *p;
    DEVPARAMS *dev;
    int       retval;

    p = (ENVIRON*) csound;
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

static int playopen_(void *csound, csRtAudioParams *parm)
{
    ENVIRON   *p;
    DEVPARAMS *dev;
    int       retval;

    p = (ENVIRON*) csound;
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

static int rtrecord_(void *csound, void *inbuf_, int bytes_)
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

static void rtplay_(void *csound, void *outbuf_, int bytes_)
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

static void rtclose_(void *csound)
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

int csoundModuleCreate(void *csound)
{
    ENVIRON *p = (ENVIRON*) csound;
    /* nothing to do, report success */
    p->Message(csound, Str("PortAudio real-time audio module for Csound\n"));
    return 0;
}

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;
    int     err;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portaudio") == 0 || strcmp(drv, "Portaudio") == 0 ||
          strcmp(drv, "portAudio") == 0 || strcmp(drv, "PortAudio") == 0 ||
          strcmp(drv, "pa") == 0 || strcmp(drv, "PA") == 0))
      return 0;
    p->Message(csound, "rtaudio: PortAudio module enabled\n");
    /* print PortAudio version */
    drv = (char*) Pa_GetVersionText();
    if (drv != NULL)
      p->Message(csound, "%s\n", drv);
    /* initialise PortAudio */
    err = (int) Pa_Initialize();
    if (err != (int) paNoError) {
      printPortAudioErrorMessage(p, err);
      return -1;
    }
    p->CreateGlobalVariable(csound, "::PortAudio::NeedsTerminate", 1);
    /* set function pointers */
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

int csoundModuleDestroy(void *csound)
{
    ENVIRON *p = (ENVIRON*) csound;

    if (p->QueryGlobalVariable(csound, "::PortAudio::NeedsTerminate")) {
      p->DestroyGlobalVariable(csound, "::PortAudio::NeedsTerminate");
      return ((int) Pa_Terminate() == (int) paNoError ? 0 : -1);
    }
    return 0;
}

