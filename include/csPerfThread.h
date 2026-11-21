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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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
     csoundPerformanceThreadStop(), or if an error occurs.
     The constructor takes a Csound instance pointer as argument; it assumes
     that the instance was compiled and started (csoundStart()) before
     creating the performance thread. Once the playback is stopped for one
     of the above mentioned reasons, the performance thread returns.
     The calls that only queue a message (play, pause, toggle pause, stop,
     stop recording, score events, input messages, score offset, orchestra
     code to compile or evaluate, requested callbacks) and setting the
     process callback may come from any thread, the performance thread's
     callbacks included. Recording opens the file on the thread that asks
     for it: ask from a thread of the host's. Joining, flushing the
     message queue and destroying must not come from a callback the
     performance runs, and joining and destroying must come from one
     thread at a time, with no other call beside them: joining frees the
     message queue and its locks, which every call that queues a message
     and flushing use, and destroying frees the object. To end a performance
     thread: stop it, join it, destroy it, then destroy the Csound
     instance.
  */
  PUBLIC CS_PERF_THREAD* csoundCreatePerformanceThread(CSOUND *csound);

  /**
     Destroys a Csound perfomance thread object.
     Must not be called from a callback the performance runs. On the
     performance thread itself, i.e. from a message callback, from the
     process callback or from a Csound callback called there, it only stops
     the performance, with a warning the first time: the thread cannot wait
     for its own end, nor go on in a deleted object. The object is then
     still to be destroyed, from another thread, once the callback has
     returned. Csound calls some callbacks from other threads (its worker
     threads with -j, the recording thread, some audio and MIDI modules):
     there the call is not seen, and can deadlock.
     Not while another thread joins it: see csoundPerformanceThreadJoin().
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
     Once the status is no longer 0, it does nothing, and opens no file.
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
      pFields is tuple, a list, or an ndarray of cs_float values with all the pfields
      for this event, starting with the p1 value specified in pFields[0].
      If absp2mode is non-zero, the start time of the event is measured
      from the beginning of performance, instead of the default of relative
      to the current time.
  */
  PUBLIC void csoundPerformanceThreadScoreEvent(CS_PERF_THREAD* pt,
    int32_t absp2mode, char opcod,
    int32_t pcnt, cs_float *p);

  /**
     Sends an event as a string
  */
  PUBLIC void csoundPerformanceThreadInputMessage(CS_PERF_THREAD* pt,
    const char *s);

  /**
     Sets the playback time pointer to the specified value (in seconds)
  */
  PUBLIC void csoundPerformanceThreadSetScoreOffsetSeconds(CS_PERF_THREAD* pt,
    cs_double timeVal);
  
  /**
     Compiles the given orchestra code
  */  
  PUBLIC void csoundPerformanceThreadCompileOrc(CS_PERF_THREAD* pt, 
    const char* code);
    
  /**
     Evaluates the given code, calls the `returncb` callback with the 
     value passed to the `return` opcode in global space. 
  */
  PUBLIC void csoundPerformanceThreadEvalCode(CS_PERF_THREAD* pt, 
    const char *code, void (*returncb)(cs_float));
    
  /**
     Evaluates the given code, calls the `returncb` callback with the
     value passed to the `return` opcode in global space, and the
     `userdata` pointer.
  */
  PUBLIC void csoundPerformanceThreadEvalCodeWithData(CS_PERF_THREAD* pt,
    const char *code, void (*returncb)(cs_float, void *userdata),
    void *userdata);
    
  /**
     Calls the given callback within the context of the callback thread
  */
  PUBLIC void csoundPerformanceThreadRequestCallback(CS_PERF_THREAD *pt,
    void (*func)(void *));
    
  /**
     Waits until the performance is finished or fails.
     Returns a positive value if the end of score was reached or
     stop() was called, and a negative value if an error occured.
     Also releases the message queue and its locks, and stops a recording
     still running once the performance has ended.
     Must not be called from a callback the performance runs. On the
     performance thread itself, i.e. from a message callback, from the
     process callback or from a Csound callback called there, it cannot
     wait for itself: the call is ignored and returns the current status,
     with a warning the first time. Csound calls some callbacks from other
     threads (its worker threads with -j, the recording thread, some audio
     and MIDI modules): there the call is not seen, and can deadlock.
     Not thread-safe: two threads must not join the same object at once,
     nor one join it while another destroys it. Each would wait for the
     thread and free what the join frees, and the process crashes or hangs.
  */
  PUBLIC int32_t csoundPerformanceThreadJoin(CS_PERF_THREAD* pt);

  /**
      Waits until all pending messages (pause, send score event, etc.)
      and their callbacks have been processed by the performance thread.
      Must not be called from the performance thread itself, i.e. from a
      message callback or from the process callback: the performance thread
      cannot wait for itself. Such a call is ignored (with a one-time
      warning) instead of deadlocking. Csound calls some callbacks from
      other threads (its worker threads with -j, the recording thread, some
      audio and MIDI modules): there the call is not seen, and can deadlock.
   */
  PUBLIC void csoundPerformanceThreadFlushMessageQueue(CS_PERF_THREAD* pt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
