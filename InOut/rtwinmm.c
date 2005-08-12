/*
    rtwinmm.c:

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

#ifdef MSVC
#include <windows.h>
#endif
#include "csdl.h"
#include "soundio.h"
#ifndef MSVC
#include <windows.h>
#endif

#ifdef MAXBUFFERS
#undef MAXBUFFERS
#endif
#define MAXBUFFERS  256

typedef struct rtWinMMDevice_ {
    HWAVEIN   inDev;
    HWAVEOUT  outDev;
    int       cur_buf;
    int       nBuffers;
    int       seed;             /* random seed for dithering */
    int       enable_buf_timer;
    /* playback sample conversion function */
    void      (*playconv)(int, void*, void*, int*);
    /* record sample conversion function */
    void      (*rec_conv)(int, void*, void*);
    int64_t   prv_time;
    float     timeConv, bufTime;
    WAVEHDR   buffers[MAXBUFFERS];
} rtWinMMDevice;

typedef struct rtWinMMGlobals_ {
    rtWinMMDevice *inDev;
    rtWinMMDevice *outDev;
    int           enable_buf_timer;
} rtWinMMGlobals;

static inline int64_t large_integer_to_int64(LARGE_INTEGER *p)
{
    return ((int64_t) p->LowPart + ((int64_t) p->HighPart << 32));
}

static int err_msg(CSOUND *csound, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    csound->ErrMsgV(csound, Str("winmm: error: "), fmt, args);
    va_end(args);
    return -1;
}

static int allocate_buffers(CSOUND *csound, rtWinMMDevice *dev,
                                             csRtAudioParams *parm,
                                             int is_playback)
{
    HGLOBAL ptr;
    int     i, err = 0, bufFrames, bufSamples, bufBytes;

    bufFrames = parm->bufSamp_SW;
    bufSamples = bufFrames * parm->nChannels;
    bufBytes = bufSamples * (parm->sampleFormat == AE_SHORT ? 2 : 4);
    dev->nBuffers = parm->bufSamp_HW / parm->bufSamp_SW;
    if (dev->nBuffers < 2)
      dev->nBuffers = 2;
    if (dev->enable_buf_timer)
      dev->nBuffers *= 2;
    if (dev->nBuffers > MAXBUFFERS) {
      dev->nBuffers = 0;
      return err_msg(csound, Str("too many buffers"));
    }
    for (i = 0; i < dev->nBuffers; i++) {
      ptr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (SIZE_T) bufBytes);
      if (ptr == (HGLOBAL) NULL) {
        dev->nBuffers = i;
        return err_msg(csound, Str("memory allocation failure"));
      }
      dev->buffers[i].lpData = (LPSTR) GlobalLock(ptr);
      memset((void*) dev->buffers[i].lpData, 0, (size_t) bufBytes);
      dev->buffers[i].dwBufferLength = (DWORD) bufBytes;
      if (is_playback)
        err = (int) waveOutPrepareHeader(dev->outDev,
                                         (LPWAVEHDR) &(dev->buffers[i]),
                                         sizeof(WAVEHDR));
      else
        err = (int) waveInPrepareHeader(dev->inDev,
                                        (LPWAVEHDR) &(dev->buffers[i]),
                                        sizeof(WAVEHDR));
      if (err != MMSYSERR_NOERROR)
        return err_msg(csound, Str("failed to prepare buffers"));
      dev->buffers[i].dwFlags |= WHDR_DONE;
    }
    return 0;
}

static int set_format_params(CSOUND *csound, WAVEFORMATEX *wfx,
                                              csRtAudioParams *parm)
{
    int sampsize = 4, framsize;
    memset(wfx, 0, sizeof(WAVEFORMATEX));
    switch (parm->sampleFormat) {
      case AE_SHORT:
        sampsize = 2;
        break;
      case AE_LONG:
      case AE_FLOAT:
        break;
      default:
        return err_msg(csound, Str("invalid sample format: "
                                   "must be -s, -l, or -f"));
    }
    framsize = sampsize * parm->nChannels;
    wfx->wFormatTag = (WORD) (parm->sampleFormat == AE_FLOAT ? 3 : 1);
    wfx->nChannels = (WORD) parm->nChannels;
    wfx->nSamplesPerSec = (DWORD) ((double) parm->sampleRate + 0.5);
    wfx->nAvgBytesPerSec = (DWORD) ((int) wfx->nSamplesPerSec * framsize);
    wfx->nBlockAlign = (DWORD) framsize;
    wfx->wBitsPerSample = (DWORD) (sampsize << 3);
    return 0;
}

/* sample conversion routines for playback */

static void float_to_short(int nSmps, float *inBuf, int16_t *outBuf, int *seed)
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

static void float_to_long(int nSmps, float *inBuf, int32_t *outBuf, int *seed)
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

static void double_to_long(int nSmps, double *inBuf, int32_t *outBuf, int *seed)
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

static void float_to_float_p(int nSmps, float *inBuf, float *outBuf, int *seed)
{
    seed = seed;
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

static void double_to_float(int nSmps, double *inBuf, float *outBuf, int *seed)
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

static int open_device(CSOUND *csound, csRtAudioParams *parm, int is_playback)
{
    rtWinMMGlobals  *p;
    rtWinMMDevice   *dev;
    WAVEFORMATEX    wfx;
    LARGE_INTEGER   pp;
    int             i, ndev, devNum, conv_idx;
    DWORD           openFlags = CALLBACK_NULL;

    if (parm->devName != NULL)
      return err_msg(csound, Str("must specify a device number, not a name"));
    if (set_format_params(csound, &wfx, parm) != 0)
      return -1;
    devNum = (parm->devNum == 1024 ? 0 : parm->devNum);
    if (parm->sampleFormat != AE_FLOAT && ((int) GetVersion() & 0xFF) >= 0x05)
      openFlags |= WAVE_FORMAT_DIRECT;

    if (is_playback) {
      WAVEOUTCAPSA  caps;
      ndev = (int) waveOutGetNumDevs();
      if (ndev < 1)
        return err_msg(csound, Str("no output device is available"));
      if (devNum < 0 || devNum >= ndev) {
        csound->Message(csound, Str("The available output devices are:\n"));
        for (i = 0; i < ndev; i++) {
          waveOutGetDevCapsA((unsigned int) i, (LPWAVEOUTCAPSA) &caps,
                             sizeof(WAVEOUTCAPSA));
          csound->Message(csound, Str("%3d: %s\n"), i, (char*) caps.szPname);
        }
        return err_msg(csound, Str("device number is out of range"));
      }
      waveOutGetDevCapsA((unsigned int) devNum, (LPWAVEOUTCAPSA) &caps,
                         sizeof(WAVEOUTCAPSA));
      csound->Message(csound, Str("winmm: opening output device %d (%s)\n"),
                              devNum, (char*) caps.szPname);
    }
    else {
      WAVEINCAPSA  caps;
      ndev = (int) waveInGetNumDevs();
      if (ndev < 1)
        return err_msg(csound, Str("no input device is available"));
      if (devNum < 0 || devNum >= ndev) {
        csound->Message(csound, Str("The available input devices are:\n"));
        for (i = 0; i < ndev; i++) {
          waveInGetDevCapsA((unsigned int) i, (LPWAVEINCAPSA) &caps,
                            sizeof(WAVEINCAPSA));
          csound->Message(csound, Str("%3d: %s\n"), i, (char*) caps.szPname);
        }
        return err_msg(csound, Str("device number is out of range"));
      }
      waveInGetDevCapsA((unsigned int) devNum, (LPWAVEINCAPSA) &caps,
                        sizeof(WAVEINCAPSA));
      csound->Message(csound, Str("winmm: opening input device %d (%s)\n"),
                              devNum, (char*) caps.szPname);
    }
    p = (rtWinMMGlobals*)
          csound->QueryGlobalVariable(csound, "_rtwinmm_globals");
    dev = (rtWinMMDevice*) malloc(sizeof(rtWinMMDevice));
    if (dev == NULL)
      return err_msg(csound, Str("memory allocation failure"));
    memset(dev, 0, sizeof(rtWinMMDevice));
    conv_idx = (parm->sampleFormat == AE_SHORT ?
                0 : (parm->sampleFormat == AE_LONG ? 1 : 2));
    if (csound->GetSizeOfMYFLT() != (int) sizeof(float))
      conv_idx += 3;
    if (is_playback) {
      p->outDev = dev;
      *(csound->GetRtPlayUserData(csound)) = (void*) dev;
      dev->enable_buf_timer = p->enable_buf_timer;
      if (waveOutOpen((LPHWAVEOUT) &(dev->outDev), (unsigned int) devNum,
                      (LPWAVEFORMATEX) &wfx, 0, 0,
                      openFlags) != MMSYSERR_NOERROR) {
        dev->outDev = (HWAVEOUT) 0;
        return err_msg(csound, Str("failed to open device"));
      }
      switch (conv_idx) {
        case 0: dev->playconv =
                  (void (*)(int, void*, void*, int*)) float_to_short;   break;
        case 1: dev->playconv =
                  (void (*)(int, void*, void*, int*)) float_to_long;    break;
        case 2: dev->playconv =
                  (void (*)(int, void*, void*, int*)) float_to_float_p; break;
        case 3: dev->playconv =
                  (void (*)(int, void*, void*, int*)) double_to_short;  break;
        case 4: dev->playconv =
                  (void (*)(int, void*, void*, int*)) double_to_long;   break;
        case 5: dev->playconv =
                  (void (*)(int, void*, void*, int*)) double_to_float;  break;
      }
    }
    else {
      p->inDev = dev;
      *(csound->GetRtRecordUserData(csound)) = (void*) dev;
      /* disable playback timer in full-duplex mode */
      dev->enable_buf_timer = p->enable_buf_timer = 0;
      if (waveInOpen((LPHWAVEIN) &(dev->inDev), (unsigned int) devNum,
                     (LPWAVEFORMATEX) &wfx, 0, 0,
                     openFlags) != MMSYSERR_NOERROR) {
        dev->inDev = (HWAVEIN) 0;
        return err_msg(csound, Str("failed to open device"));
      }
      switch (conv_idx) {
        case 0: dev->rec_conv =
                  (void (*)(int, void*, void*)) short_to_float;   break;
        case 1: dev->rec_conv =
                  (void (*)(int, void*, void*)) long_to_float;    break;
        case 2: dev->rec_conv =
                  (void (*)(int, void*, void*)) float_to_float_r; break;
        case 3: dev->rec_conv =
                  (void (*)(int, void*, void*)) short_to_double;  break;
        case 4: dev->rec_conv =
                  (void (*)(int, void*, void*)) long_to_double;   break;
        case 5: dev->rec_conv =
                  (void (*)(int, void*, void*)) float_to_double;  break;
      }
    }
    if (allocate_buffers(csound, dev, parm, is_playback) != 0)
      return -1;
    if (!is_playback)
      waveInStart(dev->inDev);
    QueryPerformanceFrequency(&pp);
    dev->timeConv = 1000.0f / (float) large_integer_to_int64(&pp);
    dev->bufTime = 1000.0f * (float) parm->bufSamp_SW / parm->sampleRate;
    QueryPerformanceCounter(&pp);
    dev->prv_time = large_integer_to_int64(&pp);
    return 0;
}

/* open for audio input */

static int recopen_(CSOUND *csound, csRtAudioParams *parm)
{
    return open_device(csound, parm, 0);
}

/* open for audio output */

static int playopen_(CSOUND *csound, csRtAudioParams *parm)
{
    return open_device(csound, parm, 1);
}

/* get samples from ADC */

static int rtrecord_(CSOUND *csound, void *inBuf, int nbytes)
{
    rtWinMMDevice   *dev = (rtWinMMDevice*)
                               *(csound->GetRtRecordUserData(csound));
    WAVEHDR         *buf = &(dev->buffers[dev->cur_buf]);
    volatile DWORD  *dwFlags = &(buf->dwFlags);

    dev->rec_conv(nbytes / csound->GetSizeOfMYFLT(),
                  (void*) buf->lpData, inBuf);
    while (!(*dwFlags & WHDR_DONE))
      Sleep(1);
    waveInAddBuffer(dev->inDev, (LPWAVEHDR) buf, sizeof(WAVEHDR));
    if (++(dev->cur_buf) >= dev->nBuffers)
      dev->cur_buf = 0;
    return nbytes;
}

/* put samples to DAC */

/* N.B. This routine serves as a THROTTLE in Csound Realtime Performance, */
/* delaying the actual writes and return until the hardware output buffer */
/* passes a sample-specific THRESHOLD.  If the I/O BLOCKING functionality */
/* is implemented ACCURATELY by the vendor-supplied audio-library write,  */
/* that is sufficient.  Otherwise, requires some kind of IOCTL from here. */
/* This functionality is IMPORTANT when other realtime I/O is occurring,  */
/* such as when external MIDI data is being collected from a serial port. */
/* Since Csound polls for MIDI input at the software synthesis K-rate     */
/* (the resolution of all software-synthesized events), the user can      */
/* eliminate MIDI jitter by requesting that both be made synchronous with */
/* the above audio I/O blocks, i.e. by setting -b to some 1 or 2 K-prds.  */

static void rtplay_(CSOUND *csound, void *outBuf, int nbytes)
{
    rtWinMMDevice   *dev = (rtWinMMDevice*)
                               *(csound->GetRtPlayUserData(csound));
    WAVEHDR         *buf = &(dev->buffers[dev->cur_buf]);
    volatile DWORD  *dwFlags = &(buf->dwFlags);
    LARGE_INTEGER   pp;
    int64_t         curTime;
    float           timeDiff, timeWait;
    int             i, nbufs;

    while (!(*dwFlags & WHDR_DONE))
      Sleep(1);
    dev->playconv(nbytes / csound->GetSizeOfMYFLT(),
                  outBuf, (void*) buf->lpData, &(dev->seed));
    waveOutWrite(dev->outDev, (LPWAVEHDR) buf, sizeof(WAVEHDR));
    if (++(dev->cur_buf) >= dev->nBuffers)
      dev->cur_buf = 0;

    if (!dev->enable_buf_timer)
      return;

    QueryPerformanceCounter(&pp);
    curTime = large_integer_to_int64(&pp);
    timeDiff = (float) (curTime - dev->prv_time) * dev->timeConv;
    dev->prv_time = curTime;
    for (i = nbufs = 0; i < dev->nBuffers; i++) {
      if (!(dev->buffers[i].dwFlags & WHDR_DONE))
        nbufs++;
    }
    if (nbufs <= 1)
      return;
    timeWait = dev->bufTime;
    timeWait *= (((float) nbufs / (float) dev->nBuffers) * 0.25f + 0.875f);
    timeWait -= timeDiff;
    i = (int) MYFLT2LRND((MYFLT) timeWait);
    if (i > 0)
      Sleep((DWORD) i);
}

static void rtclose_(CSOUND *csound)
{
    rtWinMMGlobals  *pp;
    rtWinMMDevice   *inDev, *outDev;
    int             i;

    *(csound->GetRtPlayUserData(csound)) = NULL;
    *(csound->GetRtRecordUserData(csound)) = NULL;
    pp = (rtWinMMGlobals*) csound->QueryGlobalVariable(csound,
                                                       "_rtwinmm_globals");
    if (pp == NULL)
      return;
    inDev = pp->inDev;
    outDev = pp->outDev;
    csound->DestroyGlobalVariable(csound, "_rtwinmm_globals");
    if (inDev != NULL) {
      waveInStop(inDev->inDev);
      waveInReset(inDev->inDev);
      for (i = 0; i < inDev->nBuffers; i++) {
        waveInUnprepareHeader(inDev->inDev, (LPWAVEHDR) &(inDev->buffers[i]),
                              sizeof(WAVEHDR));
        GlobalUnlock((HGLOBAL) inDev->buffers[i].lpData);
        GlobalFree((HGLOBAL) inDev->buffers[i].lpData);
      }
      waveInClose(inDev->inDev);
      free(inDev);
    }
    if (outDev != NULL) {
      volatile DWORD  *dwFlags = &(outDev->buffers[outDev->cur_buf].dwFlags);
      while (!(*dwFlags & WHDR_DONE))
        Sleep(1);
      waveOutReset(outDev->outDev);
      for (i = 0; i < outDev->nBuffers; i++) {
        waveOutUnprepareHeader(outDev->outDev,
                               (LPWAVEHDR) &(outDev->buffers[i]),
                               sizeof(WAVEHDR));
        GlobalUnlock((HGLOBAL) outDev->buffers[i].lpData);
        GlobalFree((HGLOBAL) outDev->buffers[i].lpData);
      }
      waveOutClose(outDev->outDev);
      free(outDev);
    }
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    rtWinMMGlobals  *pp;

    csound->Message(csound, Str("Windows MME real time audio module for Csound "
                                "by Istvan Varga\n"));

    if (csound->CreateGlobalVariable(csound, "_rtwinmm_globals",
                                   sizeof(rtWinMMGlobals)) != 0)
      return err_msg(csound, Str("could not allocate global structure"));
    pp = (rtWinMMGlobals*) csound->QueryGlobalVariable(csound,
                                                       "_rtwinmm_globals");
    pp->inDev = NULL;
    pp->outDev = NULL;
    pp->enable_buf_timer = 1;
    return (csound->CreateConfigurationVariable(
                csound, "mme_playback_timer", &(pp->enable_buf_timer),
                CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                "Attempt to reduce timing jitter "
                "in MME sound output (default: on)", NULL));
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");

    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "winmm") == 0 || strcmp(drv, "WinMM") == 0 ||
          strcmp(drv, "mme") == 0 || strcmp(drv, "MME") == 0))
      return 0;
    csound->Message(csound, "rtaudio: WinMM module enabled\n");
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

