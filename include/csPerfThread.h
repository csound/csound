/*
  csPerfThread.h: C API interface to Csound PerformanceThread

  Copyright François Pinot (C) 2024 

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

#ifndef CSOUND_CSPERFTHREAD_H
#define CSOUND_CSPERFTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif
  typedef void CS_PERF_THREAD;

  /**
     Runs Csound in a separate thread.
     The playback (which is paused by default) is stopped by calling
     stop(), or if an error occurs.
     The constructor takes a Csound instance pointer as argument; it assumes
     that ctcsound.compile_() was called successfully before creating
     the performance thread. Once the playback is stopped for one of the above
     mentioned reasons, the performance thread return
  */
  PUBLIC CS_PERF_THREAD* csoundCreatePerformanceThread(CSOUND *csound);

  /**
     Destroys a Csound perfomance thread object
  */
  PUBLIC void csoundDestroyPerformanceThread(CS_PERF_THREAD* pt);

  /**
     Returns true if the performance thread is running, false otherwise.
  */
  PUBLIC int32_t csoundPerformanceThreadIsRunning(CS_PERF_THREAD* pt);

  /**
     Returns the process callback.
  */
  PUBLIC void *csoundPerformanceThreadGetProcessCB(CS_PERF_THREAD*  pt);

  /**
     Sets the process callback.
  */ 
  PUBLIC void csoundPerformanceThreadSetProcessCB(CS_PERF_THREAD* pt,
    void (*callback)(void *),
    void *cbData);
  
  /**
     Returns the Csound instance pointer
  */
  PUBLIC CSOUND *csoundPerformanceThreadGetCsound(CS_PERF_THREAD* pt);

  /**
     Returns the current status.
     Zero if still playing, positive if the end of score was reached or
     performance was stopped, and negative if an error occured.
  */
  PUBLIC int32_t csoundPerformanceThreadGetStatus(CS_PERF_THREAD* pt);

  /**
     Starts/Continues performance if it was paused
  */
  PUBLIC void csoundPerformanceThreadPlay(CS_PERF_THREAD* pt);

  /**
     Pauses performance
  */
  PUBLIC void csoundPerformanceThreadPause(CS_PERF_THREAD* pt);

  /**
     Toggles performance depending on its state (playing, paused)
  */
  PUBLIC void csoundPerformanceThreadTogglePause(CS_PERF_THREAD* pt);

  /**
     Stops performance fully.
  */ 
  PUBLIC void csoundPerformanceThreadStop(CS_PERF_THREAD* pt);

  /**
     Starts recording the output from Csound.
     The sample rate and number of channels are taken directly from the
     running Csound instance.
  */
  PUBLIC void csoundPerformanceThreadRecord(CS_PERF_THREAD* pt,
    const char *filename,
    int32_t samplebits, int32_t numbufs);

  /**
     Stops recording and closes audio file.
  */
  PUBLIC void csoundPerformanceThreadStopRecord(CS_PERF_THREAD* pt);

  /** 
      Sends an event.
      The event has type opcod (e.g. 'i' for a note event).
      pFields is tuple, a list, or an ndarray of MYFLTs with all the pfields
      for this event, starting with the p1 value specified in pFields[0].
      If absp2mode is non-zero, the start time of the event is measured
      from the beginning of performance, instead of the default of relative
      to the current time.
  */
  PUBLIC void csoundPerformanceThreadScoreEvent(CS_PERF_THREAD* pt,
    int32_t absp2mode, char opcod,
    int32_t pcnt, MYFLT *p);

  /**
     Sends an event as a string
  */
  PUBLIC void csoundPerformanceThreadInputMessage(CS_PERF_THREAD* pt,
    const char *s);

  /**
     Sets the playback time pointer to the specified value (in seconds)
  */
  PUBLIC void csoundPerformanceThreadSetScoreOffsetSeconds(CS_PERF_THREAD* pt,
    double timeVal);
  
  /**
     Waits until the performance is finished or fails.
     Returns a positive value if the end of score was reached or
     stop() was called, and a negative value if an error occured.
     Also releases any resources associated with the performance thread
     object.

  */
  PUBLIC int32_t csoundPerformanceThreadJoin(CS_PERF_THREAD* pt);

  /**
     Waits until all pending messages are actually received.
     (pause, send score event, etc.)
  */
  PUBLIC void csoundPerformanceThreadFlushMessageQueue(CS_PERF_THREAD* pt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
