/*
    rtalsa.c:

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

#include <alsa/asoundlib.h>
/* no #ifdef, should always have these on Linux */
#include <unistd.h>
#include <stdint.h>
#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"

#ifdef Str
#undef Str
#endif
#define Str(x) (((ENVIRON*) csound)->LocalizeString(x))

typedef struct devparams_ {
    snd_pcm_t       *handle;        /* handle                           */
    void            *buf;           /* sample conversion buffer         */
    char            *device;        /* device name                      */
    int             format;         /* sample format                    */
    int             sampleSize;     /* MYFLT sample frame size in bytes */
    int             srate;          /* sample rate in Hz                */
    int             nchns;          /* number of channels               */
    int             buffer_smps;    /* buffer length in samples         */
    int             period_smps;    /* period time in samples           */
    /* playback sample conversion function */
    void            (*playconv)(int, void*, void*, int*);
    /* record sample conversion function */
    void            (*rec_conv)(int, void*, void*);
    int             seed;           /* random seed for dithering        */
} DEVPARAMS;

static  const   char    *indev_var = "::ALSA_IN";
static  const   char    *outdev_var = "::ALSAOUT";

/* sample conversion routines for playback */

static void float_to_short(int nSmps, float *inBuf, int16_t *outBuf,
                           int *seed);
static void double_to_short(int nSmps, double *inBuf, int16_t *outBuf,
                            int *seed);
static void float_to_long(int nSmps, float *inBuf, int32_t *outBuf,
                          int *seed);
static void double_to_long(int nSmps, double *inBuf, int32_t *outBuf,
                           int *seed);
static void float_to_float_p(int nSmps, float *inBuf, float *outBuf,
                             int *seed);
static void double_to_float(int nSmps, double *inBuf, float *outBuf,
                            int *seed);

/* sample conversion routines for recording */

static void short_to_float(int nSmps, int16_t *inBuf, float *outBuf);
static void short_to_double(int nSmps, int16_t *inBuf, double *outBuf);
static void long_to_float(int nSmps, int32_t *inBuf, float *outBuf);
static void long_to_double(int nSmps, int32_t *inBuf, double *outBuf);
static void float_to_float_r(int nSmps, float *inBuf, float *outBuf);
static void float_to_double(int nSmps, float *inBuf, double *outBuf);

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    /* nothing to do, report success */
    ((ENVIRON*) csound)->Message(csound, "ALSA real-time audio module "
                                         "for Csound by Istvan Varga\n");
    return 0;
}

static int playopen_(void*, csRtAudioParams*);
static int recopen_(void*, csRtAudioParams*);
static void rtplay_(void*, void*, int);
static int rtrecord_(void*, void*, int);
static void rtclose_(void*);

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "alsa") == 0 || strcmp(drv, "Alsa") == 0 ||
          strcmp(drv, "ALSA") == 0))
      return 0;
    p->Message(csound, "rtaudio: ALSA module enabled\n");
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

/* select sample format */

static snd_pcm_format_t set_format(void **convFunc, int csound_format, int play,
                                   int myflt_is_double)
{
    short   endian_test = 0x1234;

    (*convFunc) = NULL;
    /* select conversion routine */
    switch (csound_format) {
      case AE_SHORT:
        if (play)
          (*convFunc) = (myflt_is_double ? (void*) double_to_short
                                           : (void*) float_to_short);
        else
          (*convFunc) = (myflt_is_double ? (void*) short_to_double
                                           : (void*) short_to_float);
        break;
      case AE_LONG:
        if (play)
          (*convFunc) = (myflt_is_double ? (void*) double_to_long
                                           : (void*) float_to_long);
        else
          (*convFunc) = (myflt_is_double ? (void*) long_to_double
                                           : (void*) long_to_float);
        break;
      case AE_FLOAT:
        if (play)
          (*convFunc) = (myflt_is_double ? (void*) double_to_float
                                           : (void*) float_to_float_p);
        else
          (*convFunc) = (myflt_is_double ? (void*) float_to_double
                                           : (void*) float_to_float_r);
        break;
    }
    if (*((unsigned char*) (&endian_test)) == (unsigned char) 0x34) {
      /* little-endian */
      switch (csound_format) {
        case AE_SHORT:  return SND_PCM_FORMAT_S16_LE;
        case AE_LONG:   return SND_PCM_FORMAT_S32_LE;
        case AE_FLOAT:  return SND_PCM_FORMAT_FLOAT_LE;
      }
    }
    else {
      /* big-endian */
      switch (csound_format) {
        case AE_SHORT:  return SND_PCM_FORMAT_S16_BE;
        case AE_LONG:   return SND_PCM_FORMAT_S32_BE;
        case AE_FLOAT:  return SND_PCM_FORMAT_FLOAT_BE;
      }
    }
    return SND_PCM_FORMAT_UNKNOWN;
}

/* set up audio device */

static int set_device_params(void *csound, DEVPARAMS *dev, int play)
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_format_t    alsaFmt;
    int                 err, n, myflt_is_double, alloc_smps;
    ENVIRON             *p;
    char                *devName, msg[512];

    p = (ENVIRON*) csound;
    dev->buf = NULL;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);
    /* open the device ... */
    if (dev->device == NULL || dev->device[0] == '\0')
      devName = "default";
    else
      devName = dev->device;
    if (play) {
      /* ... for playback */
      err = snd_pcm_open(&(dev->handle), devName,
                         SND_PCM_STREAM_PLAYBACK, 0);
      if (err < 0) {
        p->Message(csound, " *** Cannot open device %s for audio output: %s\n",
                           devName, snd_strerror(err));
        return -1;
      }
    }
    else {
      /* ... for capture */
      err = snd_pcm_open(&(dev->handle), devName,
                         SND_PCM_STREAM_CAPTURE, 0);
      if (err < 0) {
        p->Message(csound, " *** Cannot open device %s for audio input: %s\n",
                           devName, snd_strerror(err));
        return -1;
      }
    }
    /* allocate hardware and software parameters */
    if (snd_pcm_hw_params_any(dev->handle, hw_params) < 0) {
      sprintf(msg, " *** No real-time audio configurations found\n");
      goto err_return_msg;
    }
    /* now set the various hardware parameters: */
    /* access method, */
    if (snd_pcm_hw_params_set_access(dev->handle, hw_params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      sprintf(msg, " *** Error setting access type for soundcard\n");
      goto err_return_msg;
    }
    /* sample format, */
    alsaFmt = SND_PCM_FORMAT_UNKNOWN;
    dev->sampleSize = (int) p->GetSizeOfMYFLT() * dev->nchns;
    myflt_is_double = (p->GetSizeOfMYFLT() == (int) sizeof(float) ? 0 : 1);
    if (play)
      alsaFmt = set_format((void**) &(dev->playconv), dev->format, 1,
                           myflt_is_double);
    else
      alsaFmt = set_format((void**) &(dev->rec_conv), dev->format, 0,
                           myflt_is_double);
    if (alsaFmt == SND_PCM_FORMAT_UNKNOWN) {
      sprintf(msg, " *** Unknown sample format. Only 16-bit and 32-bit "
                   "integers,\n *** and 32-bit floats are supported.\n");
      goto err_return_msg;
    }
    if (snd_pcm_hw_params_set_format(dev->handle, hw_params, alsaFmt) < 0) {
      sprintf(msg, " *** Unable to set requested sample format on soundcard\n");
      goto err_return_msg;
    }
    /* number of channels, */
    if (snd_pcm_hw_params_set_channels(dev->handle, hw_params,
                                       (unsigned int) dev->nchns) < 0) {
      sprintf(msg, " *** Unable to set number of channels on soundcard\n");
      goto err_return_msg;
    }
    /* sample rate, */
    if (snd_pcm_hw_params_set_rate(dev->handle, hw_params,
                                   (unsigned int) dev->srate, 0) < 0) {
      sprintf(msg, " *** Unable to set sample rate on soundcard\n");
      goto err_return_msg;
    }
    /* buffer size, */
    if (dev->buffer_smps == 0) dev->buffer_smps = 1024;
    if (dev->buffer_smps < 64) dev->buffer_smps = 64;
    if (snd_pcm_hw_params_set_buffer_size(dev->handle, hw_params,
                                          (snd_pcm_uframes_t) dev->buffer_smps)
        < 0) {
      sprintf(msg, " *** Failed while trying to set soundcard "
                   "DMA buffer size\n");
      goto err_return_msg;
    }
    /* and period size */
    alloc_smps = dev->period_smps;
    if (dev->period_smps == 0) dev->period_smps = 256;
    if (dev->period_smps < 16) dev->period_smps = 16;
    if (dev->period_smps > (dev->buffer_smps >> 1))
      dev->period_smps = (dev->buffer_smps >> 1);
    if (alloc_smps < dev->period_smps)  /* make sure that enough memory */
      alloc_smps = dev->period_smps;    /* is allocated for the buffer */
    if (snd_pcm_hw_params_set_period_size(dev->handle, hw_params,
                                          (snd_pcm_uframes_t) dev->period_smps,
                                          0) < 0) {
      sprintf(msg, " *** Error setting period time for real-time audio\n");
      goto err_return_msg;
    }
    /* set up device according to the above parameters */
    if (snd_pcm_hw_params(dev->handle, hw_params) < 0) {
      sprintf(msg, " *** Error setting hardware parameters "
                   "for real-time audio\n");
      goto err_return_msg;
    }
    /* print settings */
    if (p->GetMessageLevel(csound) != 0) {
      sprintf(msg, "ALSA %s: total buffer size: %d, fragment size: %d\n",
                   (play ? "output" : "input"),
                   dev->buffer_smps, dev->period_smps);
      p->Message(csound, msg);
    }
    /* now set software parameters */
    n = (play ? dev->buffer_smps : 1);
    if (snd_pcm_sw_params_current(dev->handle, sw_params) < 0
        || snd_pcm_sw_params_set_start_threshold(dev->handle, sw_params,
                                                 (snd_pcm_uframes_t) n) < 0
        || snd_pcm_sw_params_set_avail_min(dev->handle, sw_params,
                                           dev->period_smps) < 0
        || snd_pcm_sw_params_set_xfer_align(dev->handle, sw_params, 1) < 0
        || snd_pcm_sw_params(dev->handle, sw_params) < 0) {
      sprintf(msg, " *** Error setting software parameters "
                   "for real-time audio\n");
      goto err_return_msg;
    }
    /* allocate memory for sample conversion buffer */
    n = (dev->format == AE_SHORT ? 2 : 4) * dev->nchns * alloc_smps;
    dev->buf = (void*) malloc((size_t) n);
    if (dev->buf == NULL) {
      sprintf(msg, " *** Memory allocation failure\n");
      goto err_return_msg;
    }
    memset(dev->buf, 0, (size_t) n);
    /* device successfully opened */
    return 0;

 err_return_msg:
    p->Message(csound, msg);
    snd_pcm_close(dev->handle);
    return -1;
}

/* open for audio input */

static int recopen_(void *csound, csRtAudioParams *parm)
{
    ENVIRON   *p;
    DEVPARAMS *dev;
    int       retval;

    p = (ENVIRON*) csound;
    /* check if the device is already opened */
    if (p->QueryGlobalVariable(csound, indev_var) != NULL)
      return 0;
    /* allocate structure */
    if (p->CreateGlobalVariable(csound, indev_var, sizeof(DEVPARAMS))
        != CSOUND_SUCCESS) {
      p->Message(csound, " *** Internal error creating ALSA input device\n");
      return -1;
    }
    /* get pointer and set up parameters */
    dev = (DEVPARAMS*) (p->QueryGlobalVariable(csound, indev_var));
    dev->handle = (snd_pcm_t*) NULL;
    dev->buf = NULL;
    dev->device = parm->devName;
    dev->format = parm->sampleFormat;
    dev->sampleSize = 1;
    dev->srate = (int) (parm->sampleRate + 0.5f);
    dev->nchns = parm->nChannels;
    dev->buffer_smps = parm->bufSamp_HW;
    dev->period_smps = parm->bufSamp_SW;
    dev->playconv = (void (*)(int, void*, void*, int*)) NULL;
    dev->rec_conv = (void (*)(int, void*, void*)) NULL;
    dev->seed = 1;
    /* open device */
    retval = (set_device_params(csound, dev, 0));
    if (retval != 0)
      p->DestroyGlobalVariable(csound, indev_var);
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
    if (p->QueryGlobalVariable(csound, outdev_var) != NULL)
      return 0;
    /* allocate structure */
    if (p->CreateGlobalVariable(csound, outdev_var, sizeof(DEVPARAMS))
        != CSOUND_SUCCESS) {
      p->Message(csound, " *** Internal error creating ALSA output device\n");
      return -1;
    }
    /* get pointer and set up parameters */
    dev = (DEVPARAMS*) (p->QueryGlobalVariable(csound, outdev_var));
    dev->handle = (snd_pcm_t*) NULL;
    dev->buf = NULL;
    dev->device = parm->devName;
    dev->format = parm->sampleFormat;
    dev->sampleSize = 1;
    dev->srate = (int) (parm->sampleRate + 0.5f);
    dev->nchns = parm->nChannels;
    dev->buffer_smps = parm->bufSamp_HW;
    dev->period_smps = parm->bufSamp_SW;
    dev->playconv = (void (*)(int, void*, void*, int*)) NULL;
    dev->rec_conv = (void (*)(int, void*, void*)) NULL;
    dev->seed = 1;
    /* open device */
    retval = (set_device_params(csound, dev, 1));
    if (retval != 0)
      p->DestroyGlobalVariable(csound, outdev_var);
    return retval;
}

/* get samples from ADC */

#ifdef warning
#undef warning
#endif
#define warning(x) {                                \
    if (p->GetMessageLevel(csound) & 4)             \
      p->Message(csound, Str("WARNING: %s\n"), x);  \
}

static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int       n, m, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (p->QueryGlobalVariableNoCheck(csound, indev_var));
    if (dev->handle == NULL) {
      /* no device, return zero samples */
      memset(inbuf_, 0, (size_t) bytes_);
      return bytes_;
    }
    /* calculate the number of samples to record */
    n = bytes_ / dev->sampleSize;

    m = 0;
    while (n) {
      err = (int) snd_pcm_readi(dev->handle, dev->buf, (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; m += err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning(Str("Buffer underrun in real-time audio input")); /* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio input suspended"));
        while (snd_pcm_resume(dev->handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      /* could not recover from error */
      p->Message(csound, Str("Error reading data from audio input device\n"));
      snd_pcm_close(dev->handle);
      dev->handle = NULL;
      break;
    }
    /* convert samples to MYFLT */
    dev->rec_conv(m * dev->nchns, dev->buf, inbuf_);
    return (m * dev->sampleSize);
}

/* put samples to DAC */

static void rtplay_(void *csound, void *outbuf_, int bytes_)
{
    DEVPARAMS *dev;
    ENVIRON   *p;
    int     n, err;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (p->QueryGlobalVariableNoCheck(csound, outdev_var));
    if (dev->handle == NULL)
      return;
    /* calculate the number of samples to play */
    n = bytes_ / dev->sampleSize;

    /* convert samples from MYFLT */
    dev->playconv(n * dev->nchns, outbuf_, dev->buf, &(dev->seed));

    while (n) {
      err = (int) snd_pcm_writei(dev->handle, dev->buf, (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning(Str("Buffer underrun in real-time audio output"));/* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio output suspended"));
        while (snd_pcm_resume(dev->handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      /* could not recover from error */
      p->Message(csound, Str("Error writing data to audio output device\n"));
      snd_pcm_close(dev->handle);
      dev->handle = NULL;
      break;
    }
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(void *csound)
{
    DEVPARAMS *dev;
    ENVIRON   *p;

    p = (ENVIRON*) csound;
    dev = (DEVPARAMS*) (p->QueryGlobalVariable(csound, indev_var));
    if (dev != NULL) {
      if (dev->handle != NULL)
        snd_pcm_close(dev->handle);
      if (dev->buf != NULL)
        free(dev->buf);
      p->DestroyGlobalVariable(csound, indev_var);
    }
    dev = (DEVPARAMS*) (p->QueryGlobalVariable(csound, outdev_var));
    if (dev != NULL) {
      if (dev->handle != NULL)
        snd_pcm_close(dev->handle);
      if (dev->buf != NULL)
        free(dev->buf);
      p->DestroyGlobalVariable(csound, outdev_var);
    }
}

/* sample conversion routines for playback */

static void float_to_short(int nSmps, float *inBuf, int16_t *outBuf,
                           int *seed)
{
    float tmp_f;
    int   tmp_i;
    while (nSmps--) {
      (*seed) = (((*seed) * 15625) + 1) & 0xFFFF;
      tmp_f = (float) ((*seed) - 0x8000) * (1.0f / (float) 0x10000);
      tmp_f += *(inBuf++) * (float) 0x8000;
      tmp_i = (int) (tmp_f + (tmp_f < 0.0f ? -0.5f : 0.5f));
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      *(outBuf++) = (int16_t) tmp_i;
    }
}

static void double_to_short(int nSmps, double *inBuf, int16_t *outBuf,
                            int *seed)
{
    double  tmp_f;
    int     tmp_i;
    while (nSmps--) {
      (*seed) = (((*seed) * 15625) + 1) & 0xFFFF;
      tmp_f = (double) ((*seed) - 0x8000) * (1.0 / (double) 0x10000);
      tmp_f += *(inBuf++) * (double) 0x8000;
      tmp_i = (int) (tmp_f + (tmp_f < 0.0 ? -0.5 : 0.5));
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      *(outBuf++) = (int16_t) tmp_i;
    }
}

static void float_to_long(int nSmps, float *inBuf, int32_t *outBuf,
                          int *seed)
{
    float   tmp_f;
    int64_t tmp_i;
    seed = seed;
    while (nSmps--) {
      tmp_f = *(inBuf++) * (float) 0x80000000UL;
      tmp_i = (int64_t) (tmp_f + (tmp_f < 0.0f ? -0.5f : 0.5f));
      if (tmp_i < -((int64_t) 0x80000000UL))
        tmp_i = -((int64_t) 0x80000000UL);
      if (tmp_i > (int64_t) 0x7FFFFFFF) tmp_i = (int64_t) 0x7FFFFFFF;
      *(outBuf++) = (int32_t) tmp_i;
    }
}

static void double_to_long(int nSmps, double *inBuf, int32_t *outBuf,
                           int *seed)
{
    double  tmp_f;
    int64_t tmp_i;
    seed = seed;
    while (nSmps--) {
      tmp_f = *(inBuf++) * (double) 0x80000000UL;
      tmp_i = (int64_t) (tmp_f + (tmp_f < 0.0 ? -0.5 : 0.5));
      if (tmp_i < -((int64_t) 0x80000000UL))
        tmp_i = -((int64_t) 0x80000000UL);
      if (tmp_i > (int64_t) 0x7FFFFFFF) tmp_i = (int64_t) 0x7FFFFFFF;
      *(outBuf++) = (int32_t) tmp_i;
    }
}

static void float_to_float_p(int nSmps, float *inBuf, float *outBuf,
                             int *seed)
{
    seed = seed;
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

static void double_to_float(int nSmps, double *inBuf, float *outBuf,
                            int *seed)
{
    seed = seed;
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++);
}

/* sample conversion routines for recording */

static void short_to_float(int nSmps, int16_t *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++) * (1.0f / (float) 0x8000);
}

static void short_to_double(int nSmps, int16_t *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (double) *(inBuf++) * (1.0 / (double) 0x8000);
}

static void long_to_float(int nSmps, int32_t *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++) * (1.0f / (float) 0x80000000UL);
}

static void long_to_double(int nSmps, int32_t *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (double) *(inBuf++) * (1.0 / (double) 0x80000000UL);
}

static void float_to_float_r(int nSmps, float *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

static void float_to_double(int nSmps, float *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (double) *(inBuf++);
}

