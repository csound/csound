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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <windows.h>
#include "csdl.h"
#include "soundio.h"

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
    void      (*playconv)(int, MYFLT*, void*, int*);
    /* record sample conversion function */
    void      (*rec_conv)(int, void*, MYFLT*);
    int64_t   prv_time;
    float     timeConv, bufTime;
    WAVEHDR   buffers[MAXBUFFERS];
} rtWinMMDevice;

typedef struct rtWinMMGlobals_ {
    rtWinMMDevice *inDev;
    rtWinMMDevice *outDev;
    int           enable_buf_timer;
} rtWinMMGlobals;

#define MBUFSIZE    1024

typedef struct rtmidi_mme_globals_ {
    HMIDIIN             inDev;
    int                 inBufWritePos;
    int                 inBufReadPos;
    DWORD               inBuf[MBUFSIZE];
    CRITICAL_SECTION    threadLock;
} RTMIDI_MME_GLOBALS;

static const unsigned char msg_bytes[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 0, 1
};

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
                                            const csRtAudioParams *parm,
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
    if (UNLIKELY(dev->nBuffers > MAXBUFFERS)) {
      dev->nBuffers = 0;
      return err_msg(csound, Str("too many buffers"));
    }
    for (i = 0; i < dev->nBuffers; i++) {
      ptr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (SIZE_T) bufBytes);
      if (UNLIKELY(ptr == (HGLOBAL) NULL)) {
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
      if (UNLIKELY(err != MMSYSERR_NOERROR))
        return err_msg(csound, Str("failed to prepare buffers"));
      dev->buffers[i].dwFlags |= WHDR_DONE;
    }
    return 0;
}

static int set_format_params(CSOUND *csound, WAVEFORMATEX *wfx,
                                             const csRtAudioParams *parm)
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

static void MYFLT_to_short(int nSmps, MYFLT *inBuf, int16_t *outBuf, int *seed)
{
    MYFLT   tmp_f;
    int     tmp_i;
    int n;
    for (n=0; n<nSmps; n++){
      int rnd = (((*seed) * 15625) + 1) & 0xFFFF;
      *seed = (((rnd) * 15625) + 1) & 0xFFFF;
      rnd += *seed;           /* triangular distribution */
      tmp_f = (MYFLT) ((rnd>>1) - 0x8000) * (FL(1.0) / (MYFLT) 0x10000);
      tmp_f += inBuf[n] * (MYFLT) 0x8000;
      tmp_i = (int) MYFLT2LRND(tmp_f);
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      outBuf[n] = (int16_t) tmp_i;
    }
}

static void MYFLT_to_short_u(int nSmps, MYFLT *inBuf, int16_t *outBuf, int *seed)
{
    MYFLT   tmp_f;
    int     tmp_i;
    int n;
    for (n=0; n<nSmps; n++) {
      int rnd = (((*seed) * 15625) + 1) & 0xFFFF;
      *seed = rnd;
      tmp_f = (MYFLT) (rnd - 0x8000) * (FL(1.0) / (MYFLT) 0x10000);
      tmp_f += inBuf[n] * (MYFLT) 0x8000;
      tmp_i = (int) MYFLT2LRND(tmp_f);
      if (tmp_i < -0x8000) tmp_i = -0x8000;
      if (tmp_i > 0x7FFF) tmp_i = 0x7FFF;
      outBuf[n] = (int16_t) tmp_i;
    }
}

static void MYFLT_to_short_no_dither(int nSmps, MYFLT *inBuf,
                                     int16_t *outBuf, int *seed)
{
    MYFLT   tmp_f;
    int     tmp_i;
    int n;
    for (n=0; n<nSmps; n++){
      tmp_f = inBuf[n] * (MYFLT) 0x8000;
      tmp_i = (int) MYFLT2LRND(tmp_f);
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
      tmp_i = (int64_t) (tmp_f + (tmp_f < FL(0.0) ? FL(-0.5) : FL(0.5)));
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
    while (nSmps--)
      *(outBuf++) = (MYFLT) *(inBuf++) * (FL(1.0) / (MYFLT) 0x8000);
}

static void long_to_MYFLT(int nSmps, int32_t *inBuf, MYFLT *outBuf)
{
    int n;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (MYFLT) inBuf[n] * (FL(1.0) / (MYFLT) 0x80000000UL);
}

static void float_to_MYFLT(int nSmps, float *inBuf, MYFLT *outBuf)
{
    int n;
    for (n=0; n<nSmps; n++)
      outBuf[n] = (MYFLT) inBuf[n];
}

static int open_device(CSOUND *csound,
                       const csRtAudioParams *parm, int is_playback)
{
    rtWinMMGlobals  *p;
    rtWinMMDevice   *dev;
    WAVEFORMATEX    wfx;
    LARGE_INTEGER   pp;
    int             i, ndev, devNum, conv_idx;
    DWORD           openFlags = CALLBACK_NULL;

    if (UNLIKELY(parm->devName != NULL))
      return err_msg(csound, Str("Must specify a device number, not a name"));
    if (set_format_params(csound, &wfx, parm) != 0)
      return -1;
    devNum = (parm->devNum == 1024 ? 0 : parm->devNum);
    if (parm->sampleFormat != AE_FLOAT && ((int) GetVersion() & 0xFF) >= 0x05)
      openFlags |= WAVE_FORMAT_DIRECT;

    if (is_playback) {
      WAVEOUTCAPSA  caps;
      ndev = (int) waveOutGetNumDevs();
      csound->Message(csound, Str("The available output devices are:\n"));
      for (i = 0; i < ndev; i++) {
        waveOutGetDevCapsA((unsigned int) i, (LPWAVEOUTCAPSA) &caps,
                           sizeof(WAVEOUTCAPSA));
        csound->Message(csound, Str("%3d: %s\n"), i, (char*) caps.szPname);
      }
      if (UNLIKELY(ndev < 1))
        return err_msg(csound, Str("No output device is available"));
      if (UNLIKELY(devNum < 0 || devNum >= ndev)) {
        return err_msg(csound, Str("Device number is out of range"));
      }
      waveOutGetDevCapsA((unsigned int) devNum, (LPWAVEOUTCAPSA) &caps,
                         sizeof(WAVEOUTCAPSA));
      csound->Message(csound, Str("winmm: opening output device %d (%s)\n"),
                              devNum, (char*) caps.szPname);
    }
    else {
      WAVEINCAPSA  caps;
      ndev = (int) waveInGetNumDevs();
      csound->Message(csound, Str("The available input devices are:\n"));
      for (i = 0; i < ndev; i++) {
        waveInGetDevCapsA((unsigned int) i, (LPWAVEINCAPSA) &caps,
                          sizeof(WAVEINCAPSA));
        csound->Message(csound, Str("%3d: %s\n"), i, (char*) caps.szPname);
      }
      if (UNLIKELY(ndev < 1))
        return err_msg(csound, Str("no input device is available"));
      if (UNLIKELY(devNum < 0 || devNum >= ndev)) {
        return err_msg(csound, Str("device number is out of range"));
      }
      waveInGetDevCapsA((unsigned int) devNum, (LPWAVEINCAPSA) &caps,
                        sizeof(WAVEINCAPSA));
      csound->Message(csound, Str("winmm: opening input device %d (%s)\n"),
                      devNum, (char*) caps.szPname);
    }
    p = (rtWinMMGlobals*)
          csound->QueryGlobalVariable(csound, "_rtwinmm_globals");
    dev = (rtWinMMDevice*) csound->Malloc(csound, sizeof(rtWinMMDevice));
    if (UNLIKELY(dev == NULL))
      return err_msg(csound, Str("memory allocation failure"));
    memset(dev, 0, sizeof(rtWinMMDevice));
    conv_idx = (parm->sampleFormat == AE_SHORT ?
                0 : (parm->sampleFormat == AE_LONG ? 1 : 2));
    if (is_playback) {
      p->outDev = dev;
     *(csound->GetRtPlayUserData(csound)) = (void*) dev;
      dev->enable_buf_timer = p->enable_buf_timer;
      if (UNLIKELY(waveOutOpen((LPHWAVEOUT) &(dev->outDev), (unsigned int) devNum,
                               (LPWAVEFORMATEX) &wfx, 0, 0,
                               openFlags) != MMSYSERR_NOERROR)) {
        dev->outDev = (HWAVEOUT) 0;
        return err_msg(csound, Str("failed to open device"));
      }
      switch (conv_idx) {
        case 0:
          if (csound->GetDitherMode(csound)==1)
            dev->playconv =
                  (void (*)(int, MYFLT*, void*, int*)) MYFLT_to_short;
          else if (csound->GetDitherMode(csound)==2)
            dev->playconv =
                  (void (*)(int, MYFLT*, void*, int*)) MYFLT_to_short_u;
          else
            dev->playconv =
                  (void (*)(int, MYFLT*, void*, int*)) MYFLT_to_short_no_dither;
          break;
        case 1: dev->playconv =
                  (void (*)(int, MYFLT*, void*, int*)) MYFLT_to_long;   break;
        case 2: dev->playconv =
                  (void (*)(int, MYFLT*, void*, int*)) MYFLT_to_float;  break;
      }
    }
    else {
      p->inDev = dev;
      *(csound->GetRtRecordUserData(csound)) = (void*) dev;
      /* disable playback timer in full-duplex mode */
      dev->enable_buf_timer = p->enable_buf_timer = 0;
      if (UNLIKELY(waveInOpen((LPHWAVEIN) &(dev->inDev), (unsigned int) devNum,
                              (LPWAVEFORMATEX) &wfx, 0, 0,
                              openFlags) != MMSYSERR_NOERROR)) {
        dev->inDev = (HWAVEIN) 0;
        return err_msg(csound, Str("failed to open device"));
      }
      switch (conv_idx) {
        case 0: dev->rec_conv =
                  (void (*)(int, void*, MYFLT*)) short_to_MYFLT;  break;
        case 1: dev->rec_conv =
                  (void (*)(int, void*, MYFLT*)) long_to_MYFLT;   break;
        case 2: dev->rec_conv =
                  (void (*)(int, void*, MYFLT*)) float_to_MYFLT;  break;
      }
    }
    if (UNLIKELY(allocate_buffers(csound, dev, parm, is_playback) != 0))
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

static int rtrecord_(CSOUND *csound, MYFLT *inBuf, int nbytes)
{
    rtWinMMDevice   *dev = (rtWinMMDevice*) *(csound->GetRtRecordUserData(csound));
    WAVEHDR         *buf = &(dev->buffers[dev->cur_buf]);
    volatile DWORD  *dwFlags = &(buf->dwFlags);

    dev->rec_conv(nbytes / (int) sizeof(MYFLT), (void*) buf->lpData, inBuf);
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

static void rtplay_(CSOUND *csound, const MYFLT *outBuf, int nbytes)
{
    rtWinMMDevice   *dev = (rtWinMMDevice*) *(csound->GetRtPlayUserData(csound));
    WAVEHDR         *buf = &(dev->buffers[dev->cur_buf]);
    volatile DWORD  *dwFlags = &(buf->dwFlags);
    LARGE_INTEGER   pp;
    int64_t         curTime;
    float           timeDiff, timeWait;
    int             i, nbufs;

    while (!(*dwFlags & WHDR_DONE))
      Sleep(1);
    dev->playconv(nbytes / (int) sizeof(MYFLT),
                  (MYFLT*) outBuf, (void*) buf->lpData, &(dev->seed));
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
    i = MYFLT2LRND(timeWait);
    if (i > 0)
      Sleep((DWORD) i);
}

static void rtclose_(CSOUND *csound)
{
    rtWinMMGlobals  *pp;
    rtWinMMDevice   *inDev, *outDev;
    int             i;

    *(csound->GetRtPlayUserData(csound))  = NULL;
    *(csound->GetRtRecordUserData(csound))  = NULL;
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
      csound->Free(csound,inDev);
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
      csound->Free(csound,outDev);
    }
}

static void CALLBACK midi_in_handler(HMIDIIN hmin, UINT wMsg, DWORD_PTR dwInstance,
                                     DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) dwInstance;
    int                 new_pos;

    (void) hmin;
    (void) dwParam2;
    if (wMsg != MIM_DATA)       /* ignore non-data messages */
      return;
    EnterCriticalSection(&(p->threadLock));
    new_pos = (p->inBufWritePos + 1) & (MBUFSIZE - 1);
    if (new_pos != p->inBufReadPos) {
      p->inBuf[p->inBufWritePos] = dwParam1;
      p->inBufWritePos = new_pos;
    }
    LeaveCriticalSection(&(p->threadLock));
}

static int midi_in_open(CSOUND *csound, void **userData, const char *devName)
{
  int                 i, ndev, devnum = 0;
    RTMIDI_MME_GLOBALS  *p;
    MIDIINCAPS          caps;

    *userData = NULL;
    ndev = (int) midiInGetNumDevs();
    if (UNLIKELY(ndev < 1)) {
      csound->ErrorMsg(csound, Str("rtmidi: no input devices are available"));
      return -1;
    }
    if (devName != NULL && devName[0] != '\0' &&
        strcmp(devName, "default") != 0) {
      if (UNLIKELY(devName[0] < '0' || devName[0] > '9')) {
        csound->ErrorMsg(csound, Str("rtmidi: must specify a device number, "
                                     "not a name"));
        return -1;
      }
      devnum = (int) atoi(devName);
    }
    csound->Message(csound, Str("The available MIDI input devices are:\n"));
    for (i = 0; i < ndev; i++) {
      midiInGetDevCaps((unsigned int) i, &caps, sizeof(MIDIINCAPS));
      csound->Message(csound, "%3d: %s\n", i, &(caps.szPname[0]));
    }
    if (UNLIKELY(devnum < 0 || devnum >= ndev)) {
      csound->ErrorMsg(csound,
                       Str("rtmidi: input device number is out of range"));
      return -1;
    }
    p = (RTMIDI_MME_GLOBALS*) csound->Calloc(csound,
                                             (size_t) sizeof(RTMIDI_MME_GLOBALS));
    if (UNLIKELY(p == NULL)) {
      csound->ErrorMsg(csound, Str("rtmidi: memory allocation failure"));
      return -1;
    }
    InitializeCriticalSection(&(p->threadLock));
    *userData = (void*) p;
    midiInGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIINCAPS));
    csound->Message(csound, Str("Opening MIDI input device %d (%s)\n"),
                            devnum, &(caps.szPname[0]));
    if (midiInOpen(&(p->inDev), (unsigned int) devnum,
                   (DWORD_PTR) midi_in_handler, (DWORD_PTR) p, CALLBACK_FUNCTION)
        != MMSYSERR_NOERROR) {
      p->inDev = (HMIDIIN) 0;
      csound->ErrorMsg(csound, Str("rtmidi: could not open input device"));
      return -1;
    }
    midiInStart(p->inDev);

    return 0;
}

static int midi_in_read(CSOUND *csound,
                        void *userData, unsigned char *buf, int nbytes)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) userData;
    unsigned int        tmp;
    unsigned char       s, d1, d2, len;
    int                 nread = 0;

    (void) csound;
    EnterCriticalSection(&(p->threadLock));
    while (p->inBufReadPos != p->inBufWritePos) {
      tmp = (unsigned int) p->inBuf[p->inBufReadPos++];
      p->inBufReadPos &= (MBUFSIZE - 1);
      s = (unsigned char) tmp; tmp >>= 8;
      d1 = (unsigned char) tmp; tmp >>= 8;
      d2 = (unsigned char) tmp;
      len = msg_bytes[(int) s >> 3];
      if (nread + len > nbytes)
        break;
      if (len) {
        buf[nread++] = s;
        if (--len) {
          buf[nread++] = d1;
          if (--len)
            buf[nread++] = d2;
        }
      }
    }
    LeaveCriticalSection(&(p->threadLock));

    return nread;
}

static int midi_in_close(CSOUND *csound, void *userData)
{
    RTMIDI_MME_GLOBALS  *p = (RTMIDI_MME_GLOBALS*) userData;

    (void) csound;
    if (p == NULL)
      return 0;
    if (p->inDev != (HMIDIIN) 0) {
      midiInStop(p->inDev);
      midiInReset(p->inDev);
      midiInClose(p->inDev);
    }
    DeleteCriticalSection(&(p->threadLock));
    csound->Free(csound,p);

    return 0;
}

static int midi_out_open(CSOUND *csound, void **userData, const char *devName)
{
    int         i, ndev, devnum = 0;
    MIDIOUTCAPS caps;
    HMIDIOUT    outDev = (HMIDIOUT) 0;

    *userData = NULL;
    ndev = (int) midiOutGetNumDevs();
    if (UNLIKELY(ndev < 1)) {
      csound->ErrorMsg(csound, Str("rtmidi: no output devices are available"));
      return -1;
    }
    if (devName != NULL && devName[0] != '\0' &&
        strcmp(devName, "default") != 0) {
      if (UNLIKELY(devName[0] < '0' || devName[0] > '9')) {
        csound->ErrorMsg(csound, Str("rtmidi: must specify a device number, "
                                     "not a name"));
        return -1;
      }
      devnum = (int) atoi(devName);
    }
    csound->Message(csound, Str("The available MIDI output devices are:\n"));
    for (i = 0; i < ndev; i++) {
      midiOutGetDevCaps((unsigned int) i, &caps, sizeof(MIDIOUTCAPS));
      csound->Message(csound, "%3d: %s\n", i, &(caps.szPname[0]));
    }
    if (UNLIKELY(devnum < 0 || devnum >= ndev)) {
      csound->ErrorMsg(csound,
                       Str("rtmidi: output device number is out of range"));
      return -1;
    }
    midiOutGetDevCaps((unsigned int) devnum, &caps, sizeof(MIDIOUTCAPS));
    csound->Message(csound, Str("Opening MIDI output device %d (%s)\n"),
                            devnum, &(caps.szPname[0]));
    if (UNLIKELY(midiOutOpen(&outDev, (unsigned int) devnum,
                             (DWORD) 0, (DWORD) 0,
                             CALLBACK_NULL) != MMSYSERR_NOERROR)) {
      csound->ErrorMsg(csound, Str("rtmidi: could not open output device"));
      return -1;
    }
    *userData = (void*) outDev;

    return 0;
}

static int midi_out_write(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
    HMIDIOUT        outDev = (HMIDIOUT) userData;
    unsigned int    tmp;
    unsigned char   s, len;
    int             pos = 0;

    (void) csound;
    while (pos < nbytes) {
      s = buf[pos++];
      len = msg_bytes[(int) s >> 3];
      if (!len)
        continue;
      tmp = (unsigned int) s;
      if (--len) {
        if (pos >= nbytes)
          break;
        tmp |= ((unsigned int) buf[pos++] << 8);
        if (--len) {
          if (pos >= nbytes)
            break;
          tmp |= ((unsigned int) buf[pos++] << 16);
        }
      }
      midiOutShortMsg(outDev, (DWORD) tmp);
    }

    return pos;
}

static int midi_out_close(CSOUND *csound, void *userData)
{
    (void) csound;
    if (userData != NULL) {
      HMIDIOUT  outDev = (HMIDIOUT) userData;
      midiOutReset(outDev);
      midiOutClose(outDev);
    }

    return 0;
}

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    rtWinMMGlobals  *pp;
    OPARMS oparms;
     csound->GetOParms(csound, &oparms);

    if (UNLIKELY(oparms.msglevel & 0x400))
      csound->Message(csound, Str("Windows MME real time audio and MIDI module "
                                  "for Csound by Istvan Varga\n"));

    if (UNLIKELY(csound->CreateGlobalVariable(csound, "_rtwinmm_globals",
                                              sizeof(rtWinMMGlobals)) != 0))
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

static CS_NOINLINE int check_name(const char *s)
{
    if (s != NULL) {
      char  buf[8];
      int   i = 0;
      do {
        buf[i] = s[i] | (char) 0x20;
        i++;
      } while (i < 7 && s[i] != '\0');
      buf[i] = '\0';
      if (strcmp(buf, "winmm") == 0 || strcmp(buf, "mme") == 0)
        return 1;
    }
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    if (check_name((char*) csound->QueryGlobalVariable(csound, "_RTAUDIO"))) {
      csound->Message(csound, Str("rtaudio: WinMM module enabled\n"));
      csound->SetPlayopenCallback(csound, playopen_);
      csound->SetRecopenCallback(csound, recopen_);
      csound->SetRtplayCallback(csound, rtplay_);
      csound->SetRtrecordCallback(csound, rtrecord_);
      csound->SetRtcloseCallback(csound, rtclose_);
    }
    if (check_name((char*) csound->QueryGlobalVariable(csound, "_RTMIDI"))) {
      csound->Message(csound, Str("rtmidi: WinMM module enabled\n"));
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
