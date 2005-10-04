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

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#include "csdl.h"
#include <unistd.h>
#include <alsa/asoundlib.h>
#include "soundio.h"

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
    void            (*playconv)(int, MYFLT *, void *, int *);
    /* record sample conversion function */
    void            (*rec_conv)(int, void *, MYFLT *);
    int             seed;           /* random seed for dithering        */
} DEVPARAMS;

#ifdef BUF_SIZE
#undef BUF_SIZE
#endif
#define BUF_SIZE  4096

typedef struct alsaMidiInputDevice_ {
    unsigned char  buf[BUF_SIZE];
    snd_rawmidi_t  *dev;
    int            bufpos, nbytes, datreq;
    unsigned char  prvStatus, dat1, dat2;
} alsaMidiInputDevice;

static const unsigned char dataBytes[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0
};

/* sample conversion routines for playback */

static void MYFLT_to_short(int nSmps, MYFLT *inBuf, int16_t *outBuf, int *seed)
{
    MYFLT tmp_f;
    int   tmp_i;
    while (nSmps--) {
      (*seed) = (((*seed) * 15625) + 1) & 0xFFFF;
      tmp_f = (MYFLT) ((*seed) - 0x8000) * (FL(1.0) / (MYFLT) 0x10000);
      tmp_f += *(inBuf++) * (MYFLT) 0x8000;
#ifndef USE_DOUBLE
      tmp_i = (int) lrintf(tmp_f);
#else
      tmp_i = (int) lrint(tmp_f);
#endif
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      *(outBuf++) = (int16_t) tmp_i;
    }
}

static void MYFLT_to_long(int nSmps, MYFLT *inBuf, int32_t *outBuf, int *seed)
{
    MYFLT   tmp_f;
    int64_t tmp_i;
    (void) seed;
    while (nSmps--) {
      tmp_f = *(inBuf++) * (MYFLT) 0x80000000UL;
#ifndef USE_DOUBLE
      tmp_i = (int64_t) llrintf(tmp_f);
#else
      tmp_i = (int64_t) llrint(tmp_f);
#endif
      if (tmp_i < -((int64_t) 0x80000000UL))
        tmp_i = -((int64_t) 0x80000000UL);
      if (tmp_i > (int64_t) 0x7FFFFFFF) tmp_i = (int64_t) 0x7FFFFFFF;
      *(outBuf++) = (int32_t) tmp_i;
    }
}

static void MYFLT_to_float(int nSmps, MYFLT *inBuf, float *outBuf, int *seed)
{
    (void) seed;
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++);
}

/* sample conversion routines for recording */

static void short_to_MYFLT(int nSmps, int16_t *inBuf, MYFLT *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (MYFLT) *(inBuf++) * (FL(1.0) / (MYFLT) 0x8000);
}

static void long_to_MYFLT(int nSmps, int32_t *inBuf, MYFLT *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (MYFLT) *(inBuf++) * (FL(1.0) / (MYFLT) 0x80000000UL);
}

static void float_to_MYFLT(int nSmps, float *inBuf, MYFLT *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (MYFLT) *(inBuf++);
}

/* select sample format */

static snd_pcm_format_t set_format(void (**convFunc)(void), int csound_format,
                                   int play)
{
    short   endian_test = 0x1234;

    (*convFunc) = NULL;
    /* select conversion routine */
    switch (csound_format) {
      case AE_SHORT:
        if (play)
          *convFunc = (void (*)(void)) MYFLT_to_short;
        else
          *convFunc = (void (*)(void)) short_to_MYFLT;
        break;
      case AE_LONG:
        if (play)
          *convFunc = (void (*)(void)) MYFLT_to_long;
        else
          *convFunc = (void (*)(void)) long_to_MYFLT;
        break;
      case AE_FLOAT:
        if (play)
          *convFunc = (void (*)(void)) MYFLT_to_float;
        else
          *convFunc = (void (*)(void)) float_to_MYFLT;
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

static int set_device_params(CSOUND *csound, DEVPARAMS *dev, int play)
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_format_t    alsaFmt;
    int                 err, n, alloc_smps;
    CSOUND              *p = csound;
    char                *devName, msg[512];

    dev->buf = NULL;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);
    /* open the device */
    if (dev->device == NULL || dev->device[0] == '\0')
      devName = "default";
    else
      devName = dev->device;
    err = snd_pcm_open(&(dev->handle), devName,
                       (play ? SND_PCM_STREAM_PLAYBACK
                               : SND_PCM_STREAM_CAPTURE), 0);
    if (err < 0) {
      if (play)
        p->ErrorMsg(p, Str(" *** Cannot open device '%s' for audio output: %s"),
                       devName, snd_strerror(err));
      else
        p->ErrorMsg(p, Str(" *** Cannot open device '%s' for audio input: %s"),
                       devName, snd_strerror(err));
      return -1;
    }
    /* allocate hardware and software parameters */
    if (snd_pcm_hw_params_any(dev->handle, hw_params) < 0) {
      sprintf(msg, "No real-time audio configurations found");
      goto err_return_msg;
    }
    /* now set the various hardware parameters: */
    /* access method, */
    if (snd_pcm_hw_params_set_access(dev->handle, hw_params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      sprintf(msg, "Error setting access type for soundcard");
      goto err_return_msg;
    }
    /* sample format, */
    alsaFmt = SND_PCM_FORMAT_UNKNOWN;
    dev->sampleSize = (int) sizeof(MYFLT) * dev->nchns;
    {
      void  (*fp)(void) = NULL;
      alsaFmt = set_format(&fp, dev->format, play);
      if (play) dev->playconv = (void (*)(int, MYFLT*, void*, int*)) fp;
      else      dev->rec_conv = (void (*)(int, void*, MYFLT*)) fp;
    }
    if (alsaFmt == SND_PCM_FORMAT_UNKNOWN) {
      sprintf(msg, "Unknown sample format.\n *** Only 16-bit and 32-bit "
                   "integers, and 32-bit floats are supported.");
      goto err_return_msg;
    }
    if (snd_pcm_hw_params_set_format(dev->handle, hw_params, alsaFmt) < 0) {
      sprintf(msg, "Unable to set requested sample format on soundcard");
      goto err_return_msg;
    }
    /* number of channels, */
    if (snd_pcm_hw_params_set_channels(dev->handle, hw_params,
                                       (unsigned int) dev->nchns) < 0) {
      sprintf(msg, "Unable to set number of channels on soundcard");
      goto err_return_msg;
    }
    /* sample rate, */
    if (snd_pcm_hw_params_set_rate(dev->handle, hw_params,
                                   (unsigned int) dev->srate, 0) < 0) {
      sprintf(msg, "Unable to set sample rate on soundcard");
      goto err_return_msg;
    }
    /* buffer size, */
    if (dev->buffer_smps == 0)
      dev->buffer_smps = 1024;
    else if (dev->buffer_smps < 64)
      dev->buffer_smps = 64;
    {
      snd_pcm_uframes_t nn = (snd_pcm_uframes_t) dev->buffer_smps;
      err = snd_pcm_hw_params_set_buffer_size_near(dev->handle, hw_params, &nn);
      if (err < 0 || (int) nn != dev->buffer_smps) {
        if (err >= 0)
          p->Message(p, Str("ALSA: -B %d not allowed on this device; "
                            "use %d instead\n"), dev->buffer_smps, (int) nn);
        sprintf(msg, "Failed while trying to set soundcard DMA buffer size");
        goto err_return_msg;
      }
    }
    /* and period size */
    alloc_smps = dev->period_smps;
    if (dev->period_smps == 0)
      dev->period_smps = 256;
    else if (dev->period_smps < 16)
      dev->period_smps = 16;
    else if (dev->period_smps > (dev->buffer_smps >> 1))
      dev->period_smps = (dev->buffer_smps >> 1);
    if (alloc_smps < dev->period_smps)  /* make sure that enough memory */
      alloc_smps = dev->period_smps;    /* is allocated for the buffer */
    {
      snd_pcm_uframes_t nn = (snd_pcm_uframes_t) dev->period_smps;
      int               dir = 0;
      err = snd_pcm_hw_params_set_period_size_near(dev->handle, hw_params, &nn,
                                                   &dir);
      if (err < 0 || (int) nn != dev->period_smps) {
        if (err >= 0)
          p->Message(p, Str("ALSA: -b %d not allowed on this device; "
                            "use %d instead\n"), dev->period_smps, (int) nn);
        sprintf(msg, "Error setting period time for real-time audio");
        goto err_return_msg;
      }
    }
    /* set up device according to the above parameters */
    if (snd_pcm_hw_params(dev->handle, hw_params) < 0) {
      sprintf(msg, "Error setting hardware parameters for real-time audio");
      goto err_return_msg;
    }
    /* print settings */
    if (p->GetMessageLevel(p) != 0)
      p->Message(p, Str("ALSA %s: total buffer size: %d, period size: %d\n"),
                    (play ? "output" : "input"),
                    dev->buffer_smps, dev->period_smps);
    /* now set software parameters */
    n = (play ? dev->buffer_smps : 1);
    if (snd_pcm_sw_params_current(dev->handle, sw_params) < 0
        || snd_pcm_sw_params_set_start_threshold(dev->handle, sw_params,
                                                 (snd_pcm_uframes_t) n) < 0
        || snd_pcm_sw_params_set_avail_min(dev->handle, sw_params,
                                           dev->period_smps) < 0
        || snd_pcm_sw_params_set_xfer_align(dev->handle, sw_params, 1) < 0
        || snd_pcm_sw_params(dev->handle, sw_params) < 0) {
      sprintf(msg, "Error setting software parameters for real-time audio");
      goto err_return_msg;
    }
    /* allocate memory for sample conversion buffer */
    n = (dev->format == AE_SHORT ? 2 : 4) * dev->nchns * alloc_smps;
    dev->buf = (void*) malloc((size_t) n);
    if (dev->buf == NULL) {
      sprintf(msg, "Memory allocation failure");
      goto err_return_msg;
    }
    memset(dev->buf, 0, (size_t) n);
    /* device successfully opened */
    return 0;

 err_return_msg:
    p->MessageS(p, CSOUNDMSG_ERROR, " *** %s\n", Str(msg));
    snd_pcm_close(dev->handle);
    return -1;
}

static int open_device(CSOUND *csound, const csRtAudioParams *parm, int play)
{
    DEVPARAMS *dev;
    void      **userDataPtr;
    int       retval;

    userDataPtr = (play ? (void**) &(csound->rtPlay_userdata)
                          : (void**) &(csound->rtRecord_userdata));
    /* check if the device is already opened */
    if (*userDataPtr != NULL)
      return 0;
    if (parm->devNum != 1024) {
      csound->ErrorMsg(csound, Str(" *** ALSA: must specify a device name, "
                                   "not a number"));
      return -1;
    }
    /* allocate structure */
    dev = (DEVPARAMS*) malloc(sizeof(DEVPARAMS));
    if (dev == NULL) {
      csound->ErrorMsg(csound, Str(" *** ALSA: %s: memory allocation failure"),
                               (play ? "playopen" : "recopen"));
      return -1;
    }
    *userDataPtr = (void*) dev;
    memset(dev, 0, sizeof(DEVPARAMS));
    /* set up parameters */
    dev->handle = (snd_pcm_t*) NULL;
    dev->buf = NULL;
    dev->device = parm->devName;
    dev->format = parm->sampleFormat;
    dev->sampleSize = 1;
    dev->srate = (int) (parm->sampleRate + 0.5f);
    dev->nchns = parm->nChannels;
    dev->buffer_smps = parm->bufSamp_HW;
    dev->period_smps = parm->bufSamp_SW;
    dev->playconv = (void (*)(int, MYFLT*, void*, int*)) NULL;
    dev->rec_conv = (void (*)(int, void*, MYFLT*)) NULL;
    dev->seed = 1;
    /* open device */
    retval = set_device_params(csound, dev, play);
    if (retval != 0) {
      free(dev);
      *userDataPtr = NULL;
    }
    return retval;
}

/* open for audio input */

static int recopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    return open_device(csound, parm, 0);
}

/* open for audio output */

static int playopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    return open_device(csound, parm, 1);
}

/* get samples from ADC */

#ifdef warning
#undef warning
#endif
#define warning(x) {                            \
    if (csound->GetMessageLevel(csound) & 4)    \
      csound->Warning(csound, Str(x));          \
}

static int rtrecord_(CSOUND *csound, MYFLT *inbuf, int nbytes)
{
    DEVPARAMS *dev;
    int       n, m, err;

    dev = (DEVPARAMS*) csound->rtRecord_userdata;
    if (dev->handle == NULL) {
      /* no device, return zero samples */
      memset(inbuf, 0, (size_t) nbytes);
      return nbytes;
    }
    /* calculate the number of samples to record */
    n = nbytes / dev->sampleSize;

    m = 0;
    while (n) {
      err = (int) snd_pcm_readi(dev->handle, dev->buf, (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; m += err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning("Buffer overrun in real-time audio input");     /* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning("Real-time audio input suspended");
        while (snd_pcm_resume(dev->handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      /* could not recover from error */
      csound->ErrorMsg(csound,
                       Str("Error reading data from audio input device"));
      snd_pcm_close(dev->handle);
      dev->handle = NULL;
      break;
    }
    /* convert samples to MYFLT */
    dev->rec_conv(m * dev->nchns, dev->buf, inbuf);
    return (m * dev->sampleSize);
}

/* put samples to DAC */

static void rtplay_(CSOUND *csound, const MYFLT *outbuf, int nbytes)
{
    DEVPARAMS *dev;
    int     n, err;

    dev = (DEVPARAMS*) csound->rtPlay_userdata;
    if (dev->handle == NULL)
      return;
    /* calculate the number of samples to play */
    n = nbytes / dev->sampleSize;

    /* convert samples from MYFLT */
    dev->playconv(n * dev->nchns, (MYFLT*) outbuf, dev->buf, &(dev->seed));

    while (n) {
      err = (int) snd_pcm_writei(dev->handle, dev->buf, (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning("Buffer underrun in real-time audio output");   /* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning("Real-time audio output suspended");
        while (snd_pcm_resume(dev->handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      /* could not recover from error */
      csound->ErrorMsg(csound,
                       Str("Error writing data to audio output device"));
      snd_pcm_close(dev->handle);
      dev->handle = NULL;
      break;
    }
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(CSOUND *csound)
{
    DEVPARAMS *dev;

    dev = (DEVPARAMS*) (*(csound->GetRtRecordUserData(csound)));
    if (dev != NULL) {
      *(csound->GetRtRecordUserData(csound)) = NULL;
      if (dev->handle != NULL)
        snd_pcm_close(dev->handle);
      if (dev->buf != NULL)
        free(dev->buf);
      free(dev);
    }
    dev = (DEVPARAMS*) (*(csound->GetRtPlayUserData(csound)));
    if (dev != NULL) {
      *(csound->GetRtPlayUserData(csound)) = NULL;
      if (dev->handle != NULL)
        snd_pcm_close(dev->handle);
      if (dev->buf != NULL)
        free(dev->buf);
      free(dev);
    }
}

static int midi_in_open(CSOUND *csound, void **userData, const char *devName)
{
    alsaMidiInputDevice *dev;
    const char  *s = "hw:0,0";
    int         err;

    (*userData) = NULL;
    dev = (alsaMidiInputDevice*) malloc(sizeof(alsaMidiInputDevice));
    if (dev == NULL) {
      csound->ErrorMsg(csound, Str("ALSA MIDI: memory allocation failure"));
      return -1;
    }
    memset(dev, 0, sizeof(alsaMidiInputDevice));
    if (devName != NULL && devName[0] != '\0')
      s = devName;
    err = snd_rawmidi_open(&(dev->dev), NULL, s, SND_RAWMIDI_NONBLOCK);
    if (err != 0) {
      csound->ErrorMsg(csound, Str("ALSA: error opening MIDI input device"));
      free((void*) dev);
      return -1;
    }
    csound->Message(csound, Str("ALSA: opened MIDI input device '%s'\n"), s);
    (*userData) = (void*) dev;
    return 0;
}

static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
    alsaMidiInputDevice *dev = (alsaMidiInputDevice*) userData;
    int             bufpos = 0;
    unsigned char   c;

    (void) csound;
    while ((nbytes - bufpos) >= 3) {
      if (dev->bufpos >= dev->nbytes) { /* read from device */
        int n = (int) snd_rawmidi_read(dev->dev, &(dev->buf[0]), BUF_SIZE);
        dev->bufpos = 0;
        if (n <= 0) {                   /* until there is no more data left */
          dev->nbytes = 0;
          break;
        }
        dev->nbytes = n;
      }
      c = dev->buf[dev->bufpos++];
      if (c >= (unsigned char) 0xF8) {          /* real time message */
        buf[bufpos++] = c;
        continue;
      }
      if (c == (unsigned char) 0xF7)            /* end of system exclusive */
        c = dev->prvStatus;
      if (c < (unsigned char) 0x80) {           /* data byte */
        if (dev->datreq <= 0)
          continue;
        if (dev->datreq == (int) dataBytes[(int) dev->prvStatus >> 4])
          dev->dat1 = c;
        else
          dev->dat2 = c;
        if (--(dev->datreq) != 0)
          continue;
        dev->datreq = dataBytes[(int) dev->prvStatus >> 4];
        buf[bufpos] = dev->prvStatus;
        buf[bufpos + 1] = dev->dat1;
        buf[bufpos + 2] = dev->dat2;
        bufpos += (dev->datreq + 1);
        continue;
      }
      else if (c < (unsigned char) 0xF0) {      /* channel message */
        dev->prvStatus = c;
        dev->datreq = dataBytes[(int) c >> 4];
        continue;
      }
      if (c < (unsigned char) 0xF4)             /* ignore system messages */
        dev->datreq = -1;
    }
    return bufpos;
}

static int midi_in_close(CSOUND *csound, void *userData)
{
    int retval;
    (void) csound;
    retval = snd_rawmidi_close(((alsaMidiInputDevice*) userData)->dev);
    free(userData);
    return retval;
}

static int midi_out_open(CSOUND *csound, void **userData, const char *devName)
{
    snd_rawmidi_t *dev = NULL;
    const char  *s = "hw:0,0";
    int         err;

    (*userData) = NULL;
    if (devName != NULL && devName[0] != '\0')
      s = devName;
    err = snd_rawmidi_open(NULL, &dev, s, SND_RAWMIDI_NONBLOCK);
    if (err != 0) {
      csound->ErrorMsg(csound, Str("ALSA: error opening MIDI output device"));
      return -1;
    }
    csound->Message(csound, Str("ALSA: opened MIDI output device '%s'\n"), s);
    (*userData) = (void*) dev;
    return 0;
}

static int midi_out_write(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
    (void) csound;
    snd_rawmidi_write((snd_rawmidi_t*) userData, buf, (size_t) nbytes);
 /* snd_rawmidi_drain((snd_rawmidi_t*) userData); */
    return nbytes;
}

static int midi_out_close(CSOUND *csound, void *userData)
{
    int retval;
    (void) csound;
    snd_rawmidi_drain((snd_rawmidi_t*) userData);
    retval = snd_rawmidi_close((snd_rawmidi_t*) userData);
    return retval;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    csound->Message(csound, "ALSA real-time audio and MIDI module "
                            "for Csound by Istvan Varga\n");
    return 0;
}

static CS_NOINLINE int check_name(const char *s)
{
    if (s != NULL &&
        (s[0] | (char) 0x20) == 'a' && (s[1] | (char) 0x20) == 'l' &&
        (s[2] | (char) 0x20) == 's' && (s[3] | (char) 0x20) == 'a' &&
        s[4] == '\0')
      return 1;
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    if (check_name((char*) csound->QueryGlobalVariable(csound, "_RTAUDIO"))) {
      csound->Message(csound, "rtaudio: ALSA module enabled\n");
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
    }
    if (check_name((char*) csound->QueryGlobalVariable(csound, "_RTMIDI"))) {
      csound->Message(csound, "rtmidi: ALSA module enabled\n");
      csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
      csound->SetExternalMidiReadCallback(csound, midi_in_read);
      csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
      csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
      csound->SetExternalMidiWriteCallback(csound, midi_out_write);
      csound->SetExternalMidiOutCloseCallback(csound, midi_out_close);
    }
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

