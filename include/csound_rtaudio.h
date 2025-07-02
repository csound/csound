/*
  csound_rtaudio.h: realtime audio module functions

  Copyright (C) 1991-2024 

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

#ifndef _CSOUND_RTAUDIO_
#define _CSOUND_RTAUDIO_
#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus */

  /** @defgroup AUDIOIO Audio I/O
   *
   *  @{ */
  
  /**
   * Real-time audio parameters structure
   */
  typedef struct {
    /** device name (NULL/empty: default) */
    char    *devName;
    /** device number (0-1023), 1024: default */
    int32_t     devNum;
    /** buffer fragment size (-b) in sample frames */
    uint32_t     bufSamp_SW;
    /** total buffer size (-B) in sample frames */
    int32_t     bufSamp_HW;
    /** number of channels */
    int32_t     nChannels;
    /** sample format (AE_SHORT etc.) */
    int32_t     sampleFormat;
    /** sample rate in Hz */
    float   sampleRate;
    /** ksmps */
    int32_t ksmps;
  } csRtAudioParams;

  typedef int32_t (*devOpenFunc)(CSOUND *, const csRtAudioParams *);
  typedef void (*audioOutFunc)(CSOUND *, const MYFLT *, int32_t) ;
  typedef int32_t (*audioInFunc)(CSOUND *, MYFLT *, int32_t);
  typedef int32_t (*audioDevListFunc)(CSOUND *,
                                      CS_AUDIODEVICE *, int32_t);

  PUBLIC void **csoundGetRtRecordUserData(CSOUND *);
  PUBLIC void **csoundGetRtPlayUserData(CSOUND *);
  PUBLIC void csoundSetPlayopenCallback(CSOUND *, devOpenFunc);
  PUBLIC void csoundSetRtplayCallback(CSOUND *, audioOutFunc);
  PUBLIC void csoundSetRecopenCallback(CSOUND *, devOpenFunc);
  PUBLIC void csoundSetRtrecordCallback(CSOUND *, audioInFunc);
  PUBLIC void csoundSetRtcloseCallback(CSOUND *, void (*)(CSOUND *));
  PUBLIC void csoundSetAudioDeviceListCallback(CSOUND *csound, audioDevListFunc);

  /**
   * Calling this function after csoundCreate()
   * and before the start of performance will disable all default
   * handling of sound I/O by the Csound library via its audio backend module.
   * Host application should in this case use the spin/spout buffers directly.
   */
  PUBLIC void csoundSetHostAudioIO(CSOUND *);

  /**
   *  Sets the current RT audio module
   */
  PUBLIC void csoundSetRTAudioModule(CSOUND *csound, const char *module);

  /** @}*/

  
#ifdef __cplusplus
}
#endif /*  __cplusplus */
#endif // _CSOUND_RTAUDIO_
