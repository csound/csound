/*
    rtalsa.c:

    Copyright (C) 2005 Istvan Varga
              (C) 2009 Andrés Cabrera, Clemens Ladisch
              (C) 2012 Tito Latini

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
/* _BSD_SOURCE definition can be dropped once support for glibc < 2.19 is dropped */
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#include "csdl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>


#include "soundio.h"

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}


#define MSG(csound, fmt, ...) {                                        \
    if(csound->GetMessageLevel(csound) || csound->GetDebug(csound)) {  \
      csound->Message(csound, fmt, __VA_ARGS__);                       \
    }                                                                  \
 }



typedef struct devparams_ {
    snd_pcm_t       *handle;        /* handle                           */
    void            *buf;           /* sample conversion buffer         */
    char            *device;        /* device name                      */
    int             format;         /* sample format                    */
    int             sampleSize;     /* MYFLT sample frame size in bytes */
    uint32_t        srate;          /* sample rate in Hz                */
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
    struct alsaMidiInputDevice_ *next;
} alsaMidiInputDevice;


typedef struct midiDevFile_ {
    unsigned char  buf[BUF_SIZE];
    int            fd;
    int            bufpos, nbytes, datreq;
    unsigned char  prvStatus, dat1, dat2;
} midiDevFile;

typedef struct alsaseqMidi_ {
    snd_seq_t             *seq;
    snd_midi_event_t      *mev;
    snd_seq_event_t       sev;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t   *pinfo;
} alsaseqMidi;

static const unsigned char dataBytes[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0
};

int set_scheduler_priority(CSOUND *csound, int priority)
{
    struct sched_param p;

    memset(&p, 0, sizeof(struct sched_param));
    if (UNLIKELY(priority < -20 || priority > sched_get_priority_max(SCHED_RR))) {
      csound->Message(csound,
                      Str("--scheduler: invalid priority value; "
                          "the allowed range is:"));
      csound->Message(csound,Str("  -20 to -1: set nice level"));
      csound->Message(csound,Str("          0: normal scheduling, "
                                 "but lock memory"));
      csound->Message(csound,Str("    1 to %d: SCHED_RR with the specified "
                                 "priority (DANGEROUS)"),
                      sched_get_priority_max(SCHED_RR));
      return -1;
    }
    /* set scheduling policy and priority */
    if (priority > 0) {
      p.sched_priority = priority;
      if (UNLIKELY(sched_setscheduler(0, SCHED_RR, &p) != 0)) {
        csound->Message(csound,
                        Str("csound: cannot set scheduling policy to SCHED_RR"));
      }
      else   csound->Message(csound,
                        Str("csound: setting scheduling policy to SCHED_RR\n"));
    }
    else {
      /* nice requested */
      if (UNLIKELY(setpriority(PRIO_PROCESS, 0, priority) != 0)) {
        csound->Message(csound, Str("csound: cannot set nice level to %d"),
                        priority);
      }
    }
    return 0;
}


/* sample conversion routines for playback */

static void MYFLT_to_short(int nSmps, MYFLT *inBuf, int16_t *outBuf, int *seed)
{
    MYFLT tmp_f;
    int   tmp_i;
    int n;
    for (n=0; n<nSmps; n++) {
      int rnd = (((*seed) * 15625) + 1) & 0xFFFF;
      *seed = (((rnd) * 15625) + 1) & 0xFFFF;
      rnd += *seed;           /* triangular distribution */
      tmp_f = (MYFLT) ((rnd>>1) - 0x8000) * (FL(1.0) / (MYFLT) 0x10000);
      tmp_f += inBuf[n] * (MYFLT) 0x8000;
#ifndef USE_DOUBLE
      tmp_i = (int) lrintf(tmp_f);
#else
      tmp_i = (int) lrint(tmp_f);
#endif
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      outBuf[n] = (int16_t) tmp_i;
    }
}

static void MYFLT_to_short_u(int nSmps, MYFLT *inBuf, int16_t *outBuf, int *seed)
{
    MYFLT tmp_f;
    int   tmp_i;
    int n;
    for (n=0; n<nSmps; n++) {
      int rnd = (((*seed) * 15625) + 1) & 0xFFFF;
      *seed = rnd;
      tmp_f = (MYFLT) (rnd - 0x8000) * (FL(1.0) / (MYFLT) 0x10000);
      tmp_f += inBuf[n] * (MYFLT) 0x8000;
#ifndef USE_DOUBLE
      tmp_i = (int) lrintf(tmp_f);
#else
      tmp_i = (int) lrint(tmp_f);
#endif
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      outBuf[n] = (int16_t) tmp_i;
    }
}

static void MYFLT_to_short_no_dither(int nSmps, MYFLT *inBuf,
                                     int16_t *outBuf, int *seed)
{
  IGN(seed);
    MYFLT tmp_f;
    int   tmp_i;
    int n;
    for (n=0; n<nSmps; n++) {
      tmp_f = inBuf[n] * (MYFLT) 0x8000;
#ifndef USE_DOUBLE
      tmp_i = (int) lrintf(tmp_f);
#else
      tmp_i = (int) lrint(tmp_f);
#endif
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      outBuf[n] = (int16_t) tmp_i;
    }
}

static void MYFLT_to_long(int nSmps, MYFLT *inBuf, int32_t *outBuf, int *seed)
{
    MYFLT   tmp_f;
    int64_t tmp_i;
    (void) seed;
    int n;
    for (n=0; n<nSmps; n++) {
      tmp_f = inBuf[n] * (MYFLT) 0x80000000UL;
#ifndef USE_DOUBLE
      tmp_i = (int64_t) llrintf(tmp_f);
#else
      tmp_i = (int64_t) llrint(tmp_f);
#endif
      if (tmp_i < -((int64_t) 0x80000000UL))
        tmp_i = -((int64_t) 0x80000000UL);
      if (tmp_i > (int64_t) 0x7FFFFFFF) tmp_i = (int64_t) 0x7FFFFFFF;
      outBuf[n] = (int32_t) tmp_i;
    }
}

static void MYFLT_to_float(int nSmps, MYFLT *inBuf, float *outBuf, int *seed)
{
    (void) seed;
    int n;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (float) inBuf[n];
}

/* sample conversion routines for recording */

static void short_to_MYFLT(int nSmps, int16_t *inBuf, MYFLT *outBuf)
{
    int n;
    MYFLT adjust = FL(1.0) / (MYFLT) 0x8000;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (MYFLT) inBuf[n] * adjust;
}

static void long_to_MYFLT(int nSmps, int32_t *inBuf, MYFLT *outBuf)
{
    int n;
    MYFLT adjust = FL(1.0) / (MYFLT) 0x80000000UL;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (MYFLT) inBuf[n] * adjust;
}

static void float_to_MYFLT(int nSmps, float *inBuf, MYFLT *outBuf)
{
    int n;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (MYFLT) inBuf[n];
}

/* select sample format */

static snd_pcm_format_t set_format(void (**convFunc)(void), int csound_format,
                                   int play, int csound_dither)
{
    int16   endian_test = 0x1234;

    (*convFunc) = NULL;
    /* select conversion routine */
    switch (csound_format) {
    case AE_SHORT:
      if (play) {
        if (csound_dither==1)
          *convFunc = (void (*)(void)) MYFLT_to_short;
        else if (csound_dither==2)
          *convFunc = (void (*)(void)) MYFLT_to_short_u;
        else
          *convFunc = (void (*)(void)) MYFLT_to_short_no_dither;
      }
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

static void DAC_channels(CSOUND *csound, int chans){
    int *dachans = (int *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_DAC_CHANNELS_",
                                       sizeof(int)) != 0)
        return;
      dachans = (int *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
      *dachans = chans;
    }
}

static void ADC_channels(CSOUND *csound, int chans){
    int *dachans = (int *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_ADC_CHANNELS_",
                                       sizeof(int)) != 0)
        return;
      dachans = (int *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
      *dachans = chans;
    }
}

/* set up audio device */
#define MSGLEN (512)
static int set_device_params(CSOUND *csound, DEVPARAMS *dev, int play)
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_format_t    alsaFmt;
    int                 err, n, alloc_smps;
    CSOUND              *p = csound;
    char                *devName, msg[MSGLEN];

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
    if (UNLIKELY(err < 0)) {
      if (play)
        p->ErrorMsg(p, Str(" *** Cannot open device '%s' for audio output: %s"),
                    devName, snd_strerror(err));
      else
        p->ErrorMsg(p, Str(" *** Cannot open device '%s' for audio input: %s"),
                    devName, snd_strerror(err));
      return -1;
    }
    /* allocate hardware and software parameters */
    if (UNLIKELY(snd_pcm_hw_params_any(dev->handle, hw_params) < 0)) {
      strNcpy(msg, Str("No real-time audio configurations found"), MSGLEN);
      goto err_return_msg;
    }
    /*=======================*/
    unsigned int hwchns;
    if (UNLIKELY(snd_pcm_hw_params_get_channels_max(hw_params, &hwchns) < 0)) {
      strNcpy(msg, Str("Could not retrieve max number of channels"), MSGLEN);
      goto err_return_msg;
    }
    if(play) {
      DAC_channels(csound,hwchns);
    }
    else ADC_channels(csound,hwchns);
    /*=========================*/

    /* now set the various hardware parameters: */
    /* access method, */
    if (UNLIKELY(snd_pcm_hw_params_set_access(dev->handle, hw_params,
                                              SND_PCM_ACCESS_RW_INTERLEAVED) < 0)) {
      strNcpy(msg, Str("Error setting access type for soundcard"), MSGLEN);
      goto err_return_msg;
    }
    /* sample format, */
    alsaFmt = SND_PCM_FORMAT_UNKNOWN;
    if(dev->srate  == 0) dev->format = AE_FLOAT;
    dev->sampleSize = (int) sizeof(MYFLT) * dev->nchns;
    {
      void  (*fp)(void) = NULL;
      alsaFmt = set_format(&fp, dev->format, play, csound->GetDitherMode(csound));
      if (play) dev->playconv = (void (*)(int, MYFLT*, void*, int*)) fp;
      else      dev->rec_conv = (void (*)(int, void*, MYFLT*)) fp;
    }

    if (UNLIKELY(alsaFmt == SND_PCM_FORMAT_UNKNOWN)) {
      strNcpy(msg, Str("Unknown sample format.\n *** Only 16-bit and 32-bit "
                     "integers, and 32-bit floats are supported."), MSGLEN);
      goto err_return_msg;
    }

    if (UNLIKELY(snd_pcm_hw_params_set_format(dev->handle, hw_params, alsaFmt)<0)) {
      strNcpy(msg,
              Str("Unable to set requested sample format on soundcard"),MSGLEN);
      goto err_return_msg;
    }
    /* number of channels, */
    if (UNLIKELY(snd_pcm_hw_params_set_channels(dev->handle, hw_params,
                                                (unsigned int) dev->nchns) < 0)) {
      strNcpy(msg, Str("Unable to set number of channels on soundcard"), MSGLEN);
      goto err_return_msg;
    }
    /* sample rate, (patched for sound cards that object to fixed rate) */
    {
      unsigned int target;
      if(dev->srate == 0) {
        // VL 2-4-2019 this code gets HW sr for use in Csound.
        unsigned int hwsr;
        snd_pcm_hw_params_t *pms;
        snd_pcm_hw_params_alloca(&pms);
        snd_pcm_hw_params_any(dev->handle, pms);
        snd_pcm_hw_params_get_rate(pms, &hwsr, 0);
        if(hwsr == 0) hwsr = 44100;
        csound->system_sr(csound, hwsr);
        target = dev->srate = hwsr;
        MSG(p, "alsa hw sampling rate: %d\n", hwsr);
      }
      else target = (unsigned int) dev->srate;

      if (UNLIKELY(snd_pcm_hw_params_set_rate_near(dev->handle,
                                                   hw_params,
                                                   (unsigned int *) &dev->srate, 0)
                   < 0)) {
        strNcpy(msg, Str("Unable to set sample rate on soundcard"), MSGLEN);
        goto err_return_msg;
      }
      if (dev->srate!=target)
        p->MessageS(p, CSOUNDMSG_WARNING, Str(" *** rate set to %d\n"), dev->srate);
      csound->system_sr(csound, dev->srate);
    }

    /* buffer size, */
    if (dev->buffer_smps == 0)
      dev->buffer_smps = 1024;
    else if (dev->buffer_smps < 16)
      dev->buffer_smps = 16;
    {
      snd_pcm_uframes_t nn = (snd_pcm_uframes_t) dev->buffer_smps;
      err = snd_pcm_hw_params_set_buffer_size_near(dev->handle, hw_params, &nn);
      if (err < 0 || (int) nn != dev->buffer_smps) {
        if (UNLIKELY(err >= 0))  {
          p->Message(p, Str("ALSA: -B %d not allowed on this device; "
                            "using %d instead\n"), dev->buffer_smps, (int) nn);
          dev->buffer_smps=nn;
        }
      }
    }
    /* and period size */
    alloc_smps = dev->period_smps;
    if (dev->period_smps == 0)
      dev->period_smps = 256;
    else if (dev->period_smps < 8)
      dev->period_smps = 8;
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
        if (UNLIKELY(err >= 0)) {
          p->Message(p, Str("ALSA: -b %d not allowed on this device; "
                            "using %d instead\n"), dev->period_smps, (int) nn);
          dev->period_smps=nn;
        }
      }
    }
    /* set up device according to the above parameters */
    if (UNLIKELY(snd_pcm_hw_params(dev->handle, hw_params) < 0)) {
      strNcpy(msg,
              Str("Error setting hardware parameters for real-time audio"),
              MSGLEN);
      goto err_return_msg;
    }
    /* print settings */

    if (p->GetMessageLevel(p) != 0)
      p->Message(p, Str("ALSA %s: total buffer size: %d, period size: %d\n"),
                 (play ? "output" : "input"),
                 dev->buffer_smps, dev->period_smps /*, dev->srate*/);
    /* now set software parameters */
    n = (play ? dev->buffer_smps : 1);
    if (UNLIKELY(snd_pcm_sw_params_current(dev->handle, sw_params) < 0 ||
                 snd_pcm_sw_params_set_start_threshold(dev->handle, sw_params,
                                                 (snd_pcm_uframes_t) n) < 0 ||
                 snd_pcm_sw_params_set_avail_min(dev->handle, sw_params,
                                           dev->period_smps) < 0 ||
        /* snd_pcm_sw_params_set_xfer_align(dev->handle, sw_params, 1) < 0 || */
                 snd_pcm_sw_params(dev->handle, sw_params) < 0)) {
      strNcpy(msg,
              Str("Error setting software parameters for real-time audio"),MSGLEN);
      goto err_return_msg;
    }
    /* allocate memory for sample conversion buffer */
    n = (dev->format == AE_SHORT ? 2 : 4) * dev->nchns * alloc_smps;
    dev->buf = (void*) csound->Malloc(csound, (size_t) n);
    if (UNLIKELY(dev->buf == NULL)) {
      strNcpy(msg, Str("Memory allocation failure"),MSGLEN);
      goto err_return_msg;
    }
    memset(dev->buf, 0, (size_t) n);
    /* device successfully opened */
    return 0;

 err_return_msg:
    p->MessageS(p, CSOUNDMSG_ERROR, " *** %s\n", msg);
    snd_pcm_close(dev->handle);
    return -1;
}

static void list_devices(CSOUND *csound)
{
    FILE * f = fopen("/proc/asound/pcm", "r");
    /*file presents this format:
      02-00: Analog PCM : Mona : playback 6 : capture 4*/
    char *line, *line_;
    line = (char *) csound->Calloc (csound, 128* sizeof(char));
    line_ = (char *) csound->Calloc (csound, 128* sizeof(char));
    char card_[] = "  ";
    char num_[] = "  ";
    char *temp;
    if (f)  {
      char *th;
      while (fgets(line, 128, f))  {   /* Read one line*/
        strcpy(line_, line);
        temp = strtok_r (line, "-", &th);
        strncpy (card_, temp, 2);
        temp = strtok_r (NULL, ":", &th);
        strncpy (num_, temp, 2);
        int card = atoi (card_);
        int num = atoi (num_);
        temp = strchr (line_, ':');
        if (temp)
          temp = temp + 2;
        /* name contains spaces at the beginning and the end.
           And line return at the end*/
        csound->Message(csound, " \"hw:%i,%i\" - %s",card, num, temp );
      }
      fclose(f);
    }
    csound->Free(csound, line);
    csound->Free(csound, line_);
}

static void trim_trailing_whitespace(char *s) {
    int i = 0, index = -1;
    while(s[i] != '\0') {
        if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
            index = i;
        i++;
    }
    s[index+1] = '\0';
}

int listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int isOutput){

  IGN(csound);
    FILE * f = fopen("/proc/asound/pcm", "r");
    /*file presents this format:
      02-00: Analog PCM : Mona : playback 6 : capture 4*/
    char line[128], line_[128];
    char card_[] = "  ";
    char num_[] = "  ";
    char *temp;
    char tmp[64];
    int n =0;
    memset(line, '\0', 128); memset(line_, '\0', 128);
    if (f)  {
      char *th;
      while (fgets(line, 128, f))  {   /* Read one line*/
        strcpy(line_, line);
        temp = strtok_r (line, "-", &th);
        if (temp==NULL) {
          fclose(f);
          return 0;
        }
        strncpy (card_, temp, 2);
        temp = strtok_r (NULL, ":", &th);
        if (temp==NULL) {
          fclose(f);
          return 0;
        }
        strncpy (num_, temp, 2);
        int card = atoi (card_);
        int num = atoi (num_);
        temp = strchr (line_, ':');
        if (temp) {
          temp = temp + 2;
        } else {
          fclose(f);
          return 0;
        }
        if (list != NULL) {
          /* for some reason, there appears to be a memory
             problem if we try to copy more than 10 chars,
             even though list[n].device_name is 64 chars long */

          strNcpy(list[n].device_name, temp, 63);
          trim_trailing_whitespace(list[n].device_name);
          //list[n].device_name[10] = '\0';
          snprintf(tmp, 64, "%shw:%i,%i", isOutput ? "dac:" : "adc:", card, num);
          strNcpy(list[n].device_id, tmp, 16);
          list[n].max_nchnls = -1;
          list[n].isOutput = isOutput;
        }
        n++;
      }
      fclose(f);
    }
    return n;
}

static int open_device(CSOUND *csound, const csRtAudioParams *parm, int play)
{    DEVPARAMS *dev;
    void      **userDataPtr;
    int       retval;

    userDataPtr = (play ? (void**) csound->GetRtPlayUserData(csound)
                   : (void**) csound->GetRtRecordUserData(csound));
    /* check if the device is already opened */
    if (*userDataPtr != NULL)
      return 0;
    if (UNLIKELY(parm->devNum != 1024)) {
      csound->ErrorMsg(csound, Str(" *** ALSA: must specify a device name, "
                                   "not a number (e.g. -odac:hw:0,0)"));
      list_devices(csound);
      return -1;
    }
    /* allocate structure */
    dev = (DEVPARAMS*) csound->Malloc(csound, sizeof(DEVPARAMS));
    if (UNLIKELY(dev == NULL)) {
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
    dev->srate = (int) (parm->sampleRate > 0 ? parm->sampleRate  + 0.5f : 0);
    dev->nchns = parm->nChannels;

    dev->period_smps = parm->bufSamp_SW;
    dev->playconv = (void (*)(int, MYFLT*, void*, int*)) NULL;
    dev->rec_conv = (void (*)(int, void*, MYFLT*)) NULL;
    dev->seed = 1;
    /* open device */
    retval = set_device_params(csound, dev, play);
    if (retval != 0) {
      csound->Free(csound,dev);
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
#define warning(x) {                                      \
      if (UNLIKELY(csound->GetMessageLevel(csound) & 4))  \
        csound->Warning(csound, Str(x));                  \
  }

static int rtrecord_(CSOUND *csound, MYFLT *inbuf, int nbytes)
{
    DEVPARAMS *dev;
    int       n, m, err;

    dev = (DEVPARAMS*) *(csound->GetRtRecordUserData(csound));
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
      if (UNLIKELY(err == -EPIPE)) {
        /* buffer underrun */
        warning(Str("Buffer overrun in real-time audio input"));     /* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio input suspended"));
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

    dev = (DEVPARAMS*) *(csound->GetRtPlayUserData(csound));
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
        warning(Str("Buffer underrun in real-time audio output"));   /* complain */
        if (snd_pcm_prepare(dev->handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio output suspended"));
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
        csound->Free(csound, dev->buf);
      csound->Free(csound,dev);
    }
    dev = (DEVPARAMS*) (*(csound->GetRtPlayUserData(csound)));
    if (dev != NULL) {
      *(csound->GetRtPlayUserData(csound)) = NULL;
      if (dev->handle != NULL)
        snd_pcm_close(dev->handle);
      if (dev->buf != NULL)
        csound->Free(csound, dev->buf);
      csound->Free(csound,dev);
    }
}

static alsaMidiInputDevice* open_midi_device(CSOUND *csound, const char  *s)
{
    int         err;
    alsaMidiInputDevice *dev;

    dev = (alsaMidiInputDevice*) csound->Malloc(csound,
                                                sizeof(alsaMidiInputDevice));
    if (UNLIKELY(dev == NULL)) {
      csound->ErrorMsg(csound, Str("ALSA MIDI: memory allocation failure"));
      return dev;
    }
    memset(dev, 0, sizeof(alsaMidiInputDevice));
    err = snd_rawmidi_open(&(dev->dev), NULL, s, SND_RAWMIDI_NONBLOCK);
    if (UNLIKELY(err != 0)) {
      csound->ErrorMsg(csound,
                       Str("ALSA: error opening MIDI input device: '%s'"), s);
      csound->Free(csound,dev);
      return NULL;
    }
    MSG(csound, Str("ALSA: opened MIDI input device '%s'\n"), s);
    return dev;
}

// This is the function which contains code from amidi
static int midi_in_open(CSOUND *csound, void **userData, const char *devName)
{
    alsaMidiInputDevice *dev = NULL, *newdev, *olddev;
    //const char  *s = "hw:0,0";
    int card;
    int device;
    snd_ctl_t *ctl;
    char* name;
    int numdevs = 0;
    name = (char *) csound->Calloc(csound, 32* sizeof(char));

    (*userData) = NULL;
    olddev = NULL;
    if (UNLIKELY(devName==NULL)) {
      csound->Message(csound, Str("ALSA midi: no string\n"));
      exit(1);                  /* what should happen here???????? */
    }
    else if (devName[0] == 'a') {
      if(csound->GetMessageLevel(csound) || csound->GetDebug(csound))
        csound->Message(csound, Str("ALSA midi: Using all devices.\n"));
      card = -1;
      if (snd_card_next(&card) >= 0 && card >= 0) {
        do {
          snprintf(name, 32, "hw:%d", card);
          if (snd_ctl_open(&ctl, name, 0) >= 0) {
            device = -1;
            for (;;) {
              if (snd_ctl_rawmidi_next_device(ctl, &device) < 0) {
                break;
              }
              if (device < 0) {
                break;
              }
              snprintf(name, 32, "hw:%d,%d", card, device);
              newdev = open_midi_device(csound, name);
              if (LIKELY(newdev != NULL)) {   /* Device opened successfully */
                numdevs++;
                if (olddev != NULL) {
                  olddev->next = newdev;
                }
                else { /* First Device */
                  dev = newdev;
                }
                olddev = newdev;
                newdev = NULL;
              }
              else { /* Device couldn't be opened */
                csound->Message(csound,
                                Str("ALSA midi: Error opening device: %s\n"),
                                name);
              }
            }
          }
          if (snd_card_next(&card) < 0)
            break;
        } while (card >= 0);

        snd_ctl_close(ctl);
      }
    }
    else if (devName[0] != '\0') {
      dev = open_midi_device(csound, devName);
      if (dev == NULL) {
        csound->Free(csound, name);
        return -1;
      }
      numdevs = 1;
    }
    csound->Free(csound, name);
    if (UNLIKELY(numdevs == 0)) {
      csound->ErrorMsg(csound, Str("ALSA midi: No devices found.\n"));
      *userData = NULL;
    }
    else {
      *userData = (void*) dev;
    }
    return 0;
}

static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
    alsaMidiInputDevice *dev = (alsaMidiInputDevice*) userData;
    int             bufpos = 0;
    unsigned char   c;
    IGN(csound);

    if (!dev) { /* No devices */
      /*  fprintf(stderr, "No devices!"); */
      return 0;
    }
    /* (void) csound; */
    dev->bufpos = 0;
    while (dev && dev->dev) {
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
      dev = dev->next;
    }
    return bufpos;
}

static int midi_in_close(CSOUND *csound, void *userData)
{
    int ret = 0, retval = 0;
    alsaMidiInputDevice *olddev, *dev = NULL;
    (void) csound;
    dev = (alsaMidiInputDevice*) userData;
    while (dev != NULL) {
      if (dev->dev) {
        ret = snd_rawmidi_close(dev->dev);
      }
      olddev = dev;
      dev = dev->next;
      csound->Free(csound,olddev);
      if (retval != -1)
        retval = ret;
    }
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
      csound->ErrorMsg(csound,
                       Str("ALSA: error opening MIDI output device '%s'"),s);
      return 0;
    }
    MSG(csound, Str("ALSA: opened MIDI output device '%s'\n"), s);
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
    int retval = 0;
    (void) csound;
    if (userData != NULL) {
      snd_rawmidi_drain((snd_rawmidi_t*) userData);
      retval = snd_rawmidi_close((snd_rawmidi_t*) userData);
    }
    return retval;
}

/* The following functions include code from Csound 4.23 (mididevice.c), */
/* written by John ffitch, David Ratajczak, and others. */

static int midi_in_open_file(CSOUND *csound, void **userData,
                             const char *devName)
{
    midiDevFile *dev;
    const char  *s = "stdin";

    (*userData) = NULL;
    dev = (midiDevFile*) csound->Calloc(csound, sizeof(midiDevFile));
    if (devName != NULL && devName[0] != '\0')
      s = devName;
    if (strcmp(s, "stdin") == 0) {
      if (fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NDELAY) < 0) {
        csound->ErrorMsg(csound, Str("-M stdin fcntl failed"));
        return -1;
      }
      dev->fd = 0;
    }
    else {
      /* open MIDI device, & set nodelay on reads */
      if ((dev->fd = open(s, O_RDONLY | O_NDELAY, 0)) < 0) {
        csound->ErrorMsg(csound, Str("cannot open %s"), s);
        return -1;
      }
    }
    if (isatty(dev->fd)) {
      struct termios  tty;
      memset(&tty, 0, sizeof(struct termios));
      if (tcgetattr(dev->fd, &tty) < 0) {
        if (dev->fd > 2)
          close(dev->fd);
        csound->ErrorMsg(csound,
                         Str("MIDI receive: cannot get termios info."));
        return -1;
      }
      cfmakeraw(&tty);
      if (cfsetispeed(&tty, EXTB) < 0) {
        if (dev->fd > 2)
          close(dev->fd);
        csound->ErrorMsg(csound,
                         Str("MIDI receive: cannot set input baud rate."));
        return -1;
      }
      if (tcsetattr(dev->fd, TCSANOW, &tty) < 0) {
        if (dev->fd > 2)
          close(dev->fd);
        csound->ErrorMsg(csound, Str("MIDI receive: cannot set termios."));
        return -1;
      }
    }
    MSG(csound, Str("Opened MIDI input device file '%s'\n"), s);
    (*userData) = (void*) dev;

    return 0;
}

static int midi_in_read_file(CSOUND *csound, void *userData,
                             unsigned char *buf, int nbytes)
{
    midiDevFile   *dev = (midiDevFile*) userData;
    int           bufpos = 0;
    unsigned char c;

    while ((nbytes - bufpos) >= 3) {
      if (dev->bufpos >= dev->nbytes) { /* read from device */
        /* For select() call, from David Ratajczak */
        fd_set    rfds;
        struct timeval tv;
        int       n;

        dev->bufpos = 0;
        dev->nbytes = 0;

        /********  NEW STUFF **********/    /* from David Ratajczak */
        /* Use select() to make truly */
        /* non-blocking call to midi  */
        /******************************/

        /* Watch rtfd to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(dev->fd, &rfds);
        /* return immediately */
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        n = select(dev->fd + 1, &rfds, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */

        if (n) {
          if (n < 0)
            csound->ErrorMsg(csound, Str("sensMIDI: retval errno %d"), errno);
          else
            n = read(dev->fd, &(dev->buf[0]), BUF_SIZE);
        }
        if (n > 0)
          dev->nbytes = n;
        else
          break;                        /* until there is no more data left */
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

static int midi_in_close_file(CSOUND *csound, void *userData)
{
    int     retval = 0;

    if (userData != NULL) {
      int   fd = ((midiDevFile*) userData)->fd;
      if (fd > 2)
        retval = close(fd);
      csound->Free(csound, userData);
    }
    return retval;
}

static int midi_out_open_file(CSOUND *csound, void **userData,
                              const char *devName)
{
    int     fd = 1;     /* stdout */

    (*userData) = NULL;
    if (devName != NULL && devName[0] != '\0' &&
        strcmp(devName, "stdout") != 0) {
      fd = open(devName, O_WRONLY);
      if (fd < 0) {
        csound->ErrorMsg(csound,
                         Str("Error opening MIDI output device file '%s'"),
                         devName);
        return -1;
      }
      MSG(csound, Str("Opened MIDI output device file '%s'\n"), devName);
    }
    (*userData) = (void*) ((uintptr_t) fd);

    return 0;
}

static int midi_out_write_file(CSOUND *csound, void *userData,
                               const unsigned char *buf, int nbytes)
{
    int     retval;

    (void) csound;
    retval = (int) write((int) ((uintptr_t) userData), buf, (size_t) nbytes);
    return retval;
}

static int midi_out_close_file(CSOUND *csound, void *userData)
{
    int     retval = 0;

    (void) csound;
    if (userData != NULL) {
      int   fd = (int) ((uintptr_t) userData);
      if (fd > 2)
        retval = close(fd);
    }
    return retval;
}

/* ALSA MIDI Sequencer added by Tito Latini (2012) */

#define ALSASEQ_SYSEX_BUFFER_SIZE  (1024)

static int alsaseq_get_client_id(CSOUND *csound, alsaseqMidi *amidi,
                                 unsigned int capability, const char *name)
{
  IGN(csound);
    snd_seq_client_info_t *client_info = amidi->cinfo;
    snd_seq_port_info_t   *port_info = amidi->pinfo;

    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(amidi->seq, client_info) >= 0) {
      int client_id;
      if ((client_id = snd_seq_client_info_get_client(client_info)) < 0)
        break;
      snd_seq_port_info_set_client(port_info, client_id);
      snd_seq_port_info_set_port(port_info, -1);
      if (snd_seq_query_next_port(amidi->seq, port_info) < 0)
        break;
      if (snd_seq_port_info_get_capability(port_info) & capability) {
        char *client_name;
        client_name = (char*) snd_seq_client_info_get_name(client_info);
        if (strcmp(name, client_name) == 0)
          return client_id;
      }
    }
    return -1;
}

/*
 * my_strchr is a destructive version of strchr that removes the
 * escape characters '\' from the string s. If escape_all is zero,
 * '\' is removed only when it is the escape char for c.
 */
static char *my_strchr(const char *s, int c, int escape_all)
{
    int    refill, success = 0, escape = 0, changed = 0;
    char   *old = (char*)s;

    for (refill = 1; *s != '\0'; s++) {
      if (*s == c) {
        if (escape) {
          escape = 0;
          refill = 1;
        }
        else {
          success = 1;
          break;
        }
      }
      else if (*s == '\\' || *s == 0x18) {
        /*
         * CAN char used to mark the escape character during the parsing
         * of CsOptions. It is useful only in parse_option_as_cfgvar.
         */
        escape ^= 1;
        if (escape_all || *(s+1) == c) {
          refill = !escape;
          changed = 1;
        }
      }
      else if (escape) {
        escape = 0;
        refill = 1;
      }
      if (refill) {
        /* ETX char used to mark the limits of a string */
        if (*s != 3 && *s != '\n')
          *old++ = (*s == 0x18 ? '\\' : *s);
        else
          changed = 1;
      }
    }
    if (changed)
      *old = '\0';
    return (success ? (char*)s : NULL);
}

/* Searching for port number after ':' at the end of the string */
static int get_port_from_string(CSOUND *csound, char *str)
{
  IGN(csound);
  int port = 0;
    char *end, *tmp, *c = str;

    while (1) {
      c = my_strchr(c, ':', 1);
      if (c == NULL)
        break;
      tmp = c+1;
      port = strtol(tmp, &end, 10);
      if (*end == '\0') {
        *c = '\0';
        break;
      }
      else { /* Not found, continue the search */
        port = 0;
        c = tmp;
      }
    }
    return port;
}

static int alsaseq_connect(CSOUND *csound, alsaseqMidi *amidi,
                           unsigned int capability, const char *addr_str)
{
    snd_seq_addr_t  addr;
    char            *s, *client_spec, direction_str[5];
    int             (*amidi_connect)(snd_seq_t*, int, int, int);

    if (capability == SND_SEQ_PORT_CAP_READ) {
      strcpy(direction_str, "from");
      amidi_connect = snd_seq_connect_from;
    }
    else {
      strcpy(direction_str, "to");
      amidi_connect = snd_seq_connect_to;
    }
    snd_seq_client_info_alloca(&amidi->cinfo);
    snd_seq_port_info_alloca(&amidi->pinfo);
    client_spec = s = (char*) addr_str;
    while (s != NULL) {
      int err;
      if ((s = my_strchr(client_spec, ',', 0)) != NULL)
        *s = '\0';
      if (*client_spec <= '9' && *client_spec >= '0') { /* client_id[:port] */
        err = snd_seq_parse_address(amidi->seq, &addr, client_spec);
        if (err >= 0) {
          err = amidi_connect(amidi->seq, 0, addr.client, addr.port);
          if (err < 0) {
            csound->ErrorMsg(csound, Str("ALSASEQ: connection failed %s %s (%s)"),
                             direction_str, client_spec, snd_strerror(err));
          }
          else {
            csound->Message(csound, Str("ALSASEQ: connected %s %d:%d\n"),
                            direction_str, addr.client, addr.port);
          }
        }
      }
      else { /* client_name[:port] */
        int client, port;
        port = get_port_from_string(csound, client_spec);
        client = alsaseq_get_client_id(csound, amidi, capability, client_spec);
        if (client >= 0) {
          err = amidi_connect(amidi->seq, 0, client, port);
          if (err < 0) {
            csound->ErrorMsg(csound,
                             Str("ALSASEQ: connection failed %s %s, port %d (%s)"),
                             direction_str, client_spec, port, snd_strerror(err));
          }
          else {
            csound->Message(csound, Str("ALSASEQ: connected %s %d:%d\n"),
                            direction_str, client, port);
          }
        }
        else {
          csound->ErrorMsg(csound,
                           Str("ALSASEQ: connection failed %s %s, port %d (%s)"),
                           direction_str, client_spec, port, snd_strerror(client));
        }
      }
      if (s != NULL)
        client_spec = s+1;
    }
    return OK;
}

static int alsaseq_in_open(CSOUND *csound, void **userData, const char *devName)
{
    int              err, client_id, port_id;
    alsaseqMidi      *amidi;
    csCfgVariable_t  *cfg;
    char             *client_name;

    *userData = NULL;
    amidi = (alsaseqMidi*) csound->Malloc(csound, sizeof(alsaseqMidi));
    if (UNLIKELY(amidi == NULL)) {
      csound->ErrorMsg(csound, Str("ALSASEQ input: memory allocation failure"));
      return -1;
    }
    memset(amidi, 0, sizeof(alsaseqMidi));
    err = snd_seq_open(&(amidi->seq), "default",
                       SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: error opening sequencer (%s)"),
                       snd_strerror(err));
      csound->Free(csound,amidi);
      return -1;
    }
    csound->Message(csound, Str("ALSASEQ: opened MIDI input sequencer\n"));
    cfg = csound->QueryConfigurationVariable(csound, "alsaseq_client");
    client_name = cfg->s.p;
    err = snd_seq_set_client_name(amidi->seq, client_name);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot set client name '%s' (%s)"),
                       client_name, snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
      return -1;
    }
    err = snd_seq_create_simple_port(amidi->seq, client_name,
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE,
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot create input port (%s)"),
                       snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
      return -1;
    }
    client_id = snd_seq_client_id(amidi->seq);
    port_id = err;
    csound->Message(csound, Str("ALSASEQ: created input port '%s' %d:%d\n"),
                    client_name, client_id, port_id);
    err = snd_midi_event_new(ALSASEQ_SYSEX_BUFFER_SIZE, &amidi->mev);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot create midi event (%s)"),
                       snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
      return -1;
    }
    snd_midi_event_init(amidi->mev);
    alsaseq_connect(csound, amidi, SND_SEQ_PORT_CAP_READ, devName);
    *userData = (void*) amidi;
    return OK;
}

static int alsaseq_in_read(CSOUND *csound,
                           void *userData, unsigned char *buf, int nbytes)
{
    int               err;
    alsaseqMidi       *amidi = (alsaseqMidi*) userData;
    snd_seq_event_t   *ev;
    IGN(csound);

    err = snd_seq_event_input(amidi->seq, &ev);
    if (err <= 0)
      return 0;
    else
      err = snd_midi_event_decode(amidi->mev, buf, nbytes, ev);
    return (err==-ENOENT) ? 0 : err;
}

static int alsaseq_in_close(CSOUND *csound, void *userData)
{
    alsaseqMidi *amidi = (alsaseqMidi*) userData;
    IGN(csound);

    if (amidi != NULL) {
      snd_midi_event_free(amidi->mev);
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
    }
    return OK;
}

static int alsaseq_out_open(CSOUND *csound, void **userData, const char *devName)
{
    int              err, client_id, port_id;
    alsaseqMidi      *amidi;
    csCfgVariable_t  *cfg;
    char             *client_name;

    *userData = NULL;
    amidi = (alsaseqMidi*) csound->Malloc(csound, sizeof(alsaseqMidi));
    if (UNLIKELY(amidi == NULL)) {
      csound->ErrorMsg(csound, Str("ALSASEQ output: memory allocation failure"));
      return -1;
    }
    memset(amidi, 0, sizeof(alsaseqMidi));
    err = snd_seq_open(&(amidi->seq), "default",
                       SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: error opening sequencer (%s)"),
                       snd_strerror(err));
      csound->Free(csound, amidi);
      return -1;
    }
    csound->Message(csound, Str("ALSASEQ: opened MIDI output sequencer\n"));
    cfg = csound->QueryConfigurationVariable(csound, "alsaseq_client");
    client_name = cfg->s.p;
    err = snd_seq_set_client_name(amidi->seq, client_name);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot set client name '%s' (%s)"),
                       client_name, snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound, amidi);
      return -1;
    }
    err = snd_seq_create_simple_port(amidi->seq, client_name,
                                     SND_SEQ_PORT_CAP_READ |
                                     SND_SEQ_PORT_CAP_SUBS_READ,
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot create output port (%s)"),
                       snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
      return -1;
    }
    client_id = snd_seq_client_id(amidi->seq);
    port_id = err;
    csound->Message(csound, Str("ALSASEQ: created output port '%s' %d:%d\n"),
                    client_name, client_id, port_id);
    err = snd_midi_event_new(ALSASEQ_SYSEX_BUFFER_SIZE, &amidi->mev);
    if (UNLIKELY(err < 0)) {
      csound->ErrorMsg(csound, Str("ALSASEQ: cannot create midi event (%s)"),
                       snd_strerror(err));
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
      return -1;
    }
    snd_midi_event_init(amidi->mev);
    snd_seq_ev_clear(&amidi->sev);
    snd_seq_ev_set_source(&amidi->sev, port_id);
    snd_seq_ev_set_subs(&amidi->sev);
    snd_seq_ev_set_direct(&amidi->sev);
    alsaseq_connect(csound, amidi, SND_SEQ_PORT_CAP_WRITE, devName);
    *userData = (void*) amidi;
    return OK;
}

static int alsaseq_out_write(CSOUND *csound,
                             void *userData, const unsigned char *buf, int nbytes)
{
    alsaseqMidi  *amidi = (alsaseqMidi*) userData;
    IGN(csound);

    if (nbytes == 0)
      return 0;
    snd_midi_event_reset_encode(amidi->mev);
    nbytes = snd_midi_event_encode(amidi->mev, buf, nbytes, &amidi->sev);
    snd_seq_event_output(amidi->seq, &amidi->sev);
    snd_seq_drain_output(amidi->seq);
    return nbytes;
}

static int alsaseq_out_close(CSOUND *csound, void *userData)
{
    alsaseqMidi  *amidi = (alsaseqMidi*) userData;
    IGN(csound);

    if (amidi != NULL) {
      snd_seq_drain_output(amidi->seq);
      snd_midi_event_free(amidi->mev);
      snd_seq_close(amidi->seq);
      csound->Free(csound,amidi);
    }
    return OK;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    int minsched, maxsched, *priority, maxlen;
    char *alsaseq_client;
    csound->CreateGlobalVariable(csound, "::priority", sizeof(int));
    priority = (int *) (csound->QueryGlobalVariable(csound, "::priority"));
    if (priority == NULL)
      csound->Message(csound, Str("warning... could not create global var\n"));
    minsched = -20;
    maxsched = (int) sched_get_priority_max(SCHED_RR);
    csound->CreateConfigurationVariable(csound, "rtscheduler", priority,
                                        CSOUNDCFG_INTEGER, 0, &minsched, &maxsched,
                                        Str("RT scheduler priority, alsa module"),
                                        NULL);
    maxlen = 64;
    alsaseq_client = (char*) csound->Calloc(csound, maxlen*sizeof(char));
    strcpy(alsaseq_client, "Csound");
    csound->CreateConfigurationVariable(csound, "alsaseq_client",
                                    (void*) alsaseq_client, CSOUNDCFG_STRING,
                                    0, NULL, &maxlen,
                                    Str("ALSASEQ client name (default: Csound)"),
                                    NULL);
    /* nothing to do, report success */
    {
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 0x400)
        csound->Message(csound, Str("ALSA real-time audio and MIDI module "
                                    "for Csound by Istvan Varga\n"));
    }
    return 0;
}

int listRawMidi(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput) {
    int count = 0;
    int card, err;

    card = -1;
    if ((err = snd_card_next(&card)) < 0) {
      csound->ErrorMsg(csound,
                       Str("cannot determine card number: %s"), snd_strerror(err));
      return 0;
    }
    if (card < 0) {
      csound->ErrorMsg(csound,Str("no sound card found"));
      return 0;
    }
    do {
      snd_ctl_t *ctl;
      char name[32];
      int device;
      int err;

      snprintf(name, 32, "hw:%d", card);
      if ((err = snd_ctl_open(&ctl, name, 0)) < 0) {
        csound->ErrorMsg(csound, Str("cannot open control for card %d: %s"),
                         card, snd_strerror(err));
        return 0;
      }
      device = -1;
      for (;;) {
        if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
          csound->ErrorMsg(csound, Str("cannot determine device number: %s"),
                           snd_strerror(err));
          break;
        }
        if (device < 0)
          break;
        snd_rawmidi_info_t *info;
        const char *name;
        const char *sub_name;
        int subs, subs_in, subs_out;
        int sub;
        int err;

        snd_rawmidi_info_alloca(&info);
        snd_rawmidi_info_set_device(info, device);

        snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
        err = snd_ctl_rawmidi_info(ctl, info);
        if (err >= 0)
          subs_in = snd_rawmidi_info_get_subdevices_count(info);
        else
          subs_in = 0;

        snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
        err = snd_ctl_rawmidi_info(ctl, info);
        if (err >= 0)
          subs_out = snd_rawmidi_info_get_subdevices_count(info);
        else
          subs_out = 0;

        subs = subs_in > subs_out ? subs_in : subs_out;
        if (!subs)
          return 0;

        for (sub = 0; sub < subs; ++sub) {
          snd_rawmidi_info_set_stream(info, sub < subs_in ?
                                      SND_RAWMIDI_STREAM_INPUT :
                                      SND_RAWMIDI_STREAM_OUTPUT);
          snd_rawmidi_info_set_subdevice(info, sub);
          err = snd_ctl_rawmidi_info(ctl, info);
          if (err < 0) {
            csound->Warning(csound,
                            Str("cannot get rawmidi information %d:%d:%d: %s\n"),
                            card, device, sub, snd_strerror(err));
            return 0;
          }
          name = snd_rawmidi_info_get_name(info);
          sub_name = snd_rawmidi_info_get_subdevice_name(info);
          if (sub == 0 && sub_name[0] == '\0') {
            if (sub < subs_in && !isOutput)  {
              if (list) {
                char devid[32];
                strNcpy(list[count].device_name, name, 32);
                snprintf(devid, 32, "hw:%d,%d", card, device);
                strNcpy(list[count].device_id, devid, 64);
                strNcpy(list[count].interface_name, devid, 32);
                memcpy(list[count].midi_module, "alsaraw", 8);
                list[count].isOutput = isOutput;
              }
              count++;
            }
            if (sub < subs_out && isOutput)  {
              if (list) {
                char devid[64];
                strNcpy(list[count].device_name, name, 64);
                snprintf(devid, 64, "hw:%d,%d", card, device);
                strNcpy(list[count].device_id, devid, 64);
                strNcpy(list[count].interface_name, devid, 32);
                memcpy(list[count].midi_module, "alsaraw", 8);
                list[count].isOutput = isOutput;
              }
              count++;
            }
            break;
          } else {
            if (sub < subs_in && !isOutput)  {
              if (list) {
                char devid[64];
                strNcpy(list[count].device_name, sub_name, 64);
                snprintf(devid, 64, "hw:%d,%d,%d", card, device,sub);
                strNcpy(list[count].device_id, devid, 64);
                memcpy(list[count].midi_module, "alsaraw", 8);
                list[count].isOutput = isOutput;
              }
              count++;
            }
            if (sub < subs_out && isOutput)  {
              if (list) {
                char devid[64];
                strNcpy(list[count].device_name, sub_name, 64);
                snprintf(devid, 64, "hw:%d,%d,%d", card, device,sub);
                strNcpy(list[count].device_id, devid, 64);
                memcpy(list[count].midi_module, "alsaraw", 8);
                list[count].isOutput = isOutput;
              }
              count++;
            }
          }
        }

      }
      snd_ctl_close(ctl);
      if ((err = snd_card_next(&card)) < 0) {
        csound->Warning(csound,
                        Str("cannot determine card number: %s"), snd_strerror(err));
        break;
      }
    } while (card >= 0);
    return count;
}


#define LIST_INPUT      1
#define LIST_OUTPUT     2

#define perm_ok(pinfo,bits)                                             \
  ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))

static int check_permission(snd_seq_port_info_t *pinfo, int perm)
{
    if (perm) {
      if (perm & LIST_INPUT) {
        if (perm_ok(pinfo, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ))
          goto __ok;
      }
      if (perm & LIST_OUTPUT) {
        if (perm_ok(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE))
          goto __ok;
      }
      return 0;
    }
 __ok:
    if (snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT)
      return 0;
    return 1;
}

int listAlsaSeq(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput) {
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int numdevs = 0, count = 0;
    snd_seq_t *seq;

    IGN(csound);

    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
      fprintf(stderr, Str("cannot open sequencer\n"));
      return 1;
    }

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
      /* reset query info */
      snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
      snd_seq_port_info_set_port(pinfo, -1);
      count = 0;
      while (snd_seq_query_next_port(seq, pinfo) >= 0) {
        if (check_permission(pinfo, isOutput? LIST_OUTPUT : LIST_INPUT)) {
          if (list) {
            strNcpy(list[numdevs].midi_module, "alsaseq", 15);
            strNcpy(list[numdevs].device_name,
                    snd_seq_port_info_get_name(pinfo), 63);
            strNcpy(list[numdevs].interface_name,
                    snd_seq_client_info_get_name(cinfo), 63);
            snprintf(list[numdevs].device_id, 64, "hw:%d,%d",
                    snd_seq_client_info_get_client(cinfo),
                    snd_seq_port_info_get_port(pinfo));
          }
          numdevs++;
          count++;
        }
      }
    }
    snd_seq_close(seq);
    return numdevs;
}

static int listDevicesM(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput){
    int count = 0;
    char *s;
    s = (char*) csound->QueryGlobalVariable(csound, "_RTMIDI");
    if (strncmp(s, "alsaraw", 8) == 0) { /* ALSA Raw MIDI */
      count = listRawMidi(csound, list, isOutput);
    } else if (strncmp(s, "alsaseq", 8) == 0) {/* ALSA Sequencer */
      count = listAlsaSeq(csound, list, isOutput);
    } else if (strncmp(s, "devfile", 8) == 0) {

    } else {
      csound->ErrorMsg(csound, Str("rtalsa: Wrong callback."));
    }
    return count;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    int     i;
    char    buf[9];
    char    *s = NULL;
    memset(buf, '\0', 9);
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);

    csound->module_list_add(csound, "alsa", "audio");
    csound->module_list_add(csound, "alsaraw", "midi");
    csound->module_list_add(csound, "alsaseq", "midi");
    csound->module_list_add(csound, "devfile", "midi");

    csCfgVariable_t *cfg;
    int priority;
    if ((cfg=csound->QueryConfigurationVariable(csound, "rtscheduler")) != NULL) {
      priority = *(cfg->i.p);
      if (priority != 0) set_scheduler_priority(csound, priority);
      csound->DeleteConfigurationVariable(csound, "rtscheduler");
      csound->DestroyGlobalVariable(csound, "::priority");
    }

    s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
    i = 0;
    if (s != NULL) {
      while (*s != (char) 0 && i < 8)
        buf[i++] = *(s++) | (char) 0x20;
    }
    buf[i] = (char) 0;
    if (strcmp(&(buf[0]), "alsa") == 0) {
      if (oparms.msglevel & 0x400 || oparms.odebug)
        csound->Message(csound, Str("rtaudio: ALSA module enabled\n"));
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
      csound->SetAudioDeviceListCallback(csound, listDevices);

    }
    s = (char*) csound->QueryGlobalVariable(csound, "_RTMIDI");
    i = 0;
    if (s != NULL) {
      while (*s != (char) 0 && i < 8)
        buf[i++] = *(s++) | (char) 0x20;
    }
    buf[i] = (char) 0;
    if (strcmp(&(buf[0]), "alsaraw") == 0 || strcmp(&(buf[0]), "alsa") == 0) {
      if (oparms.msglevel & 0x400 || oparms.odebug)
        csound->Message(csound, Str("rtmidi: ALSA Raw MIDI module enabled\n"));
      csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
      csound->SetExternalMidiReadCallback(csound, midi_in_read);
      csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
      csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
      csound->SetExternalMidiWriteCallback(csound, midi_out_write);
      csound->SetExternalMidiOutCloseCallback(csound, midi_out_close);
      csound->SetMIDIDeviceListCallback(csound,listDevicesM);

    }
    else if (strcmp(&(buf[0]), "alsaseq") == 0) {
      if (oparms.msglevel & 0x400 || oparms.odebug)
        csound->Message(csound, Str("rtmidi: ALSASEQ module enabled\n"));
      csound->SetExternalMidiInOpenCallback(csound, alsaseq_in_open);
      csound->SetExternalMidiReadCallback(csound, alsaseq_in_read);
      csound->SetExternalMidiInCloseCallback(csound, alsaseq_in_close);
      csound->SetExternalMidiOutOpenCallback(csound, alsaseq_out_open);
      csound->SetExternalMidiWriteCallback(csound, alsaseq_out_write);
      csound->SetExternalMidiOutCloseCallback(csound, alsaseq_out_close);
      csound->SetMIDIDeviceListCallback(csound,listDevicesM);
    }
    else if (strcmp(&(buf[0]), "devfile") == 0) {
      if (oparms.msglevel & 0x400)
        csound->Message(csound, Str("rtmidi: devfile module enabled\n"));
      csound->SetExternalMidiInOpenCallback(csound, midi_in_open_file);
      csound->SetExternalMidiReadCallback(csound, midi_in_read_file);
      csound->SetExternalMidiInCloseCallback(csound, midi_in_close_file);
      csound->SetExternalMidiOutOpenCallback(csound, midi_out_open_file);
      csound->SetExternalMidiWriteCallback(csound, midi_out_write_file);
      csound->SetExternalMidiOutCloseCallback(csound, midi_out_close_file);
      csound->SetMIDIDeviceListCallback(csound,listDevicesM);
    }

    return 0;
}

PUBLIC int csoundModuleDestroy(CSOUND *csound)
{
    csCfgVariable_t *cfg;

    cfg = csound->QueryConfigurationVariable(csound, "alsaseq_client");
    if (cfg != NULL && cfg->s.p != NULL)
      csound->Free(csound, cfg->s.p);
    return OK;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
