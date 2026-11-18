/*
  csound_rtmidi.h: realtime midi module functions

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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef _CSOUND_RTMIDI_
#define _CSOUND_RTMIDI_
#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus */
  
  /** @defgroup RTMIDI Realtime Midi I/O
   *
   *  @{ */

  /**
   * Calling this function after csoundCreate()
   * and before the start of performance to implement
   * MIDI via the callbacks below.
   */
  PUBLIC void csoundSetHostMIDIIO(CSOUND *csound);

  /**
   *  Sets the current MIDI IO module. This loads the plugins if they are
   *  not loaded yet, and starts them again if they are. If a plugin fails
   *  to load or start, this returns, and a later csoundStart() fails on it.
   */
  PUBLIC void csoundSetMIDIModule(CSOUND *csound, const char *module);


  /**
   * Sets callback for opening real time MIDI input.
   */
  PUBLIC void csoundSetExternalMidiInOpenCallback(CSOUND *,int32_t (*func)
                                                  (CSOUND *,void **userData,
                                                   const char *devName));

  /**
   * Sets callback for reading from real time MIDI input. Csound may call
   * it again in the same control cycle, at most until it returns 0 or has
   * given 16 times nBytes bytes in that cycle, so it returns 0 when it has
   * nothing more for now. What it still has is read in a later cycle.
   * The start fails if it is NULL when MIDI input opens. Set to NULL
   * during a performance, it reads nothing more.
   */
  PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *,int32_t (*func)
                                                (CSOUND *, void *userData,
                                                 unsigned char *buf,
                                                 int32_t nBytes));

  /**
   * Sets callback for closing real time MIDI input.
   */
  PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *,int32_t (*func)
                                                   (CSOUND *,void *userData));

  /**
   * Sets callback for opening real time MIDI output.
   */
  PUBLIC void csoundSetExternalMidiOutOpenCallback(CSOUND *,int32_t (*func)
                                                   (CSOUND *,void **userData,
                                                    const char *devName));

  /**
   * Sets callback for writing to real time MIDI output.
   * The start fails if it is NULL when MIDI output opens. Set to NULL
   * during a performance, it writes nothing more.
   */
  PUBLIC void csoundSetExternalMidiWriteCallback(CSOUND *, int32_t (*func)
                                                 (CSOUND *,void *userData,
                                                  const unsigned char *buf,
                                                  int32_t nBytes));

  /**
   * Sets callback for closing real time MIDI output.
   */
  PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *,
                                                    int32_t (*func)(CSOUND *,
                                                                void *userData));

  /**
   * Sets callback for converting MIDI error codes to strings.
   */
  PUBLIC void csoundSetExternalMidiErrorStringCallback(CSOUND *,
                                                       const char *(*func)(int32_t));


  /**
   * Sets a function that is called to obtain a list of MIDI devices.
   * This should be set by IO plugins, and should not be used by hosts.
   * With NULL, no device is listed. The function returns how many devices
   * there are when its list is NULL, and otherwise how many it filled, at
   * most that many, or a negative number on an error. (See csoundGetMIDIDevList())
   */
  PUBLIC void csoundSetMIDIDeviceListCallback(CSOUND *csound,
                                              int32_t (*mididevlist__)
                                              (CSOUND *,CS_MIDIDEVICE *list,
                                               int32_t isOutput));

  /** @}*/
#ifdef __cplusplus
}
#endif /*  __cplusplus */
#endif // _CSOUND_RTAUDIO_
