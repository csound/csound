#ifndef CSOUND_H
#define CSOUND_H
/*
 * C S O U N D
 *
 * An auto-extensible system for making music on computers
 * by means of software alone.
 * Copyright (c) 2001 by Michael Gogins. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \file
 * \brief Declares the public Csound application programming interface (API).
 * \author John P. Fitch, Michael Gogins, Matt Ingalls, and John D. Ramsdell
 *
 * \b Purposes
 *
 * The purposes of the Csound API are as follows:
 *
 * \li Declare a stable public application programming interface (API)
 *     for Csound in csound.h. This is the only header file that needs
 *     to be #included by users of the Csound API.
 *
 * \li Hide the internal implementation details of Csound from users of
 *     the API, so that development of Csound can proceed without affecting
 *     code that uses the API.
 *
 * \b Users
 *
 * Users of the Csound API fall into two main categories: hosts, and plugins.
 *
 * \li Hosts are applications that use Csound as a software synthesis engine.
 *     Hosts can link with the Csound API either statically or dynamically.
 *
 * \li Plugins are shared libraries loaded by Csound at run time to implement
 *     external opcodes and/or drivers for audio or MIDI input and output.
 *
 * Hosts using the Csound API must #include <csound.h>, and link with the
 * Csound API library.
 *
 * Hosts must first create an instance of Csound using the \c csoundCreate
 * API function. When hosts are finished using Csound, they must destroy the
 * instance of csound using the \c csoundDestroy API function.
 * Most of the other Csound API functions take the Csound instance as their
 * first argument.
 * Hosts can call either the standalone API functions defined in csound.h,
 * e.g. \c csoundGetSr(csound), or the function pointers in the Csound instance
 * structure, e.g. csound->GetSr(csound). Each function in the Csound API has
 * a corresponding function pointer in the Csound instance structure.
 *
 * Here is the complete code for the simplest possible Csound API host,
 * a command-line Csound application:
 *
 * \code
 *
 * #include <csound.h>
 *
 * int main(int argc, char **argv)
 * {
 *     void *csound = csoundCreate(0);
 *     int result = csoundPerform(csound, argc, argv);
 *     csoundDestroy(csound);
 *     return result;
 * }
 *
 * \endcode
 *
 * All opcodes, including plugins, receive a pointer to their host
 * instance of Csound as the first argument. Therefore, plugins MUST NOT
 * create an instance of Csound, and MUST call the Csound API function
 * pointers off the Csound instance pointer.
 *
 * \code
 * MYFLT sr = csound->GetSr(csound);
 * \endcode
 *
 * In general, plugins should ONLY access Csound functionality through the
 * API function pointers.
 *
 * \b TODO
 *
 * The Csound API is not finished. At this time, the Csound API functions
 * do not all take a pointer to the Csound instance as their first argument.
 * This needs to be changed.
 *
 * In addition, some new functions need to be added to the API for
 * various purposes:
 *
 * \li Create and destroy function tables.
 *
 * \li Support for configurating and accessing realtime audio busses.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*
   * Platform-dependent definitions and declarations.
   */
#if defined HAVE_CONFIG_H && (defined _WIN32 || defined __CYGWIN__)
#  if !defined PIC
#    define PUBLIC
#  elif defined BUILDING_LIBCSOUND
#    define PUBLIC __declspec(dllexport)
#  else
#    define PUBLIC __declspec(dllimport)
#  endif
#  define LIBRARY_CALL WINAPI
#elif defined WIN32
#define PUBLIC __declspec(dllexport)
#define LIBRARY_CALL WINAPI
#else
#define PUBLIC
#define LIBRARY_CALL
#endif

  /* Enables Python interface. */
#ifdef SWIG
  %module csound
  %{
#include "sysdep.h"
#include "cwindow.h"
#include "opcode.h"
#include "text.h"
#include <stdarg.h>
    %}
#else
#include "sysdep.h"
#include "cwindow.h"
#include "opcode.h"
#include "text.h"
#include <stdarg.h>
#endif
  /**
   * ERROR DEFINITIONS
   */

  typedef enum
    {
      /*/ Completed successfully. */
      CSOUND_SUCCESS = 0,
      /*/ Unspecified failure. */
      CSOUND_ERROR = -1,
      /*/ Failed during initialization. */
      CSOUND_INITIALIZATION = -2,
      /*/ Failed during performance. */
      CSOUND_PERFORMANCE = -3,
      /*/ Failed to allocate requested memory. */
      CSOUND_MEMORY = -4,
    }
  CSOUND_STATUS;

  /*
   * INSTANTIATION
   */

  /**
   * Initialise Csound library.
   * Returns zero on success.
   */

  PUBLIC int csoundInitialize(int *argc, char ***argv);

  /**
   * Creates an instance of Csound.
   * Returns an opaque pointer that must be passed to most Csound API functions.
   * The hostData parameter can be null, or it can be a pointer to any sort of
   * data; this pointer can be accessed from the Csound instance that is passed
   * to callback routines.
   */
  PUBLIC void *csoundCreate(void *hostData);

  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  PUBLIC int csoundPreCompile(void *csound);

  /**
   * Returns a pointer to the requested interface, if available, in the
   * interface argument, and its version number, in the version argument.
   * Returns 0 for success and 1 for failure.
   */
  PUBLIC int csoundQueryInterface(const char *name, void **iface, int *version);

  /**
   * Destroys an instance of Csound.
   */
  PUBLIC void csoundDestroy(void *csound);

  /**
   * Returns the version number times 100 (4.20 = 420).
   */
  PUBLIC int csoundGetVersion(void);

  /**
   * Returns host data.
   */
  PUBLIC void *csoundGetHostData(void *csound);

  /**
   * Sets host data.
   */
  PUBLIC void csoundSetHostData(void *csound, void *hostData);

  /*
   * PERFORMANCE
   */

  /**
   * Compiles and renders a Csound performance,
   * as directed by the supplied command-line arguments,
   * in one pass. Returns 1 for success, 0 for failure.
   */
  PUBLIC int csoundPerform(void *csound, int argc, char **argv);

  /**
   * Compiles Csound input files (such as an orchestra and score)
   * as directed by the supplied command-line arguments,
   * but does not perform them. Returns a non-zero error code on failure.
   * In this (host-driven) mode, the sequence of calls should be as follows:
   * /code
   *
   *       csoundCompile(csound, argc, argv, thisObj);
   *       while(!csoundPerformBuffer(csound));
   *       csoundCleanup(csound);
   *       csoundReset(csound);
   * /endcode
   */
  PUBLIC int csoundCompile(void *csound, int argc, char **argv);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output.
   * Note that csoundCompile must be called first.
   * Returns false during performance, and true when performance is finished.
   * If called until it returns true, will perform an entire score.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int csoundPerformKsmps(void *csound);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output.
   * Note that csoundCompile must be called first.
   * Performs audio whether or not the Csound score has finished.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int csoundPerformKsmpsAbsolute(void *csound);

  /**
   * Performs Csound, sensing real-time and score events
   * and processing one buffer's worth (-b frames) of interleaved audio.
   * Returns a pointer to the new output audio in 'outputAudio'
   * Note that csoundCompile must be called first, then call
   * csoundGetOutputBuffer() and csoundGetInputBuffer() to get the pointer
   * to csound's I/O buffers.
   * Returns false during performance, and true when performance is finished.
   */
  PUBLIC int csoundPerformBuffer(void *csound);

  /**
   * Prints information about the end of a performance.
   * Must be called after the final call to csoundPerformKsmps.
   */
  PUBLIC int csoundCleanup(void *csound);

  /**
   * Resets all internal memory and state in preparation for a new performance.
   * Enables external software to run successive Csound performances
   * without reloading Csound.
   */
  PUBLIC void csoundReset(void *csound);

  /*
   * ATTRIBUTES
   */

  /**
   * Returns the number of audio sample frames per second.
   */
  PUBLIC MYFLT csoundGetSr(void *csound);

  /**
   * Returns the number of control samples per second.
   */
  PUBLIC MYFLT csoundGetKr(void *csound);

  /**
   * Returns the number of audio sample frames per control sample.
   */
  PUBLIC int csoundGetKsmps(void *csound);

  /**
   * Returns the number of audio output channels.
   */
  PUBLIC int csoundGetNchnls(void *csound);

  /**
   * Returns the sample format.
   */
  PUBLIC int csoundGetSampleFormat(void *csound);

  /**
   * Returns the size in bytes of a single sample.
   */
  PUBLIC int csoundGetSampleSize(void *csound);

  /**
   * Returns the number of samples in Csound's input buffer.
   */
  PUBLIC long csoundGetInputBufferSize(void *csound);

  /**
   * Returns the number of samples in Csound's output buffer.
   */
  PUBLIC long csoundGetOutputBufferSize(void *csound);

  /**
   * Returns the address of the Csound audio input buffer.
   * Enables external software to write audio into Csound before calling
   * csoundPerformBuffer
   */
  PUBLIC void *csoundGetInputBuffer(void *csound);

  /**
   * Returns the address of the Csound audio output buffer.
   * Enables external software to read audio from Csound after calling
   * csoundPerformBuffer.
   */
  PUBLIC void *csoundGetOutputBuffer(void *csound);

  /**
   * Returns the address of the Csound audio input working buffer (spin).
   * Enables external software to write audio into Csound before calling
   * csoundPerformKsmps.
   */
  PUBLIC MYFLT *csoundGetSpin(void *csound);

  /**
   * Returns the address of the Csound audio output working buffer (spout).
   * Enables external software to read audio from Csound after calling
   * csoundPerformKsmps.
   */
  PUBLIC MYFLT *csoundGetSpout(void *csound);

  /**
   * Returns the current score time.
   */
  PUBLIC MYFLT csoundGetScoreTime(void *csound);

  /**
   * Returns the % of score completed.
   */
  PUBLIC MYFLT csoundGetProgress(void *csound);

  /**
   * Returns the scoreTime vs. calculatedTime ratio.  For
   * real-time performance this value should be always == 1.
   */
  PUBLIC MYFLT csoundGetProfile(void *csound);

  /**
   * Returns the sampsTime vs. calculatedTime ratio.
   */
  PUBLIC MYFLT csoundGetCpuUsage(void *csound);

  /*
   * SCORE HANDLING
   */

  /**
   * Returns whether Csound's score is synchronized with external software.
   */
  PUBLIC int csoundIsScorePending(void *csound);

  /**
   * Sets whether Csound's score is synchronized with external software.
   */
  PUBLIC void csoundSetScorePending(void *csound, int pending);

  /**
   * Csound events prior to the offset are consumed and discarded
   * prior to beginning performance.  Can be used by external
   * software to begin performance midway through a Csound score.
   */
  PUBLIC MYFLT csoundGetScoreOffsetSeconds(void *csound);

  /**
   * Csound events prior to the offset are consumed and discarded
   * prior to beginning performance.  Can be used by external
   * software to begin performance midway through a Csound score.
   */
  PUBLIC void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset);

  /**
   * Rewinds a compiled Csound score to its beginning.
   */
  PUBLIC void csoundRewindScore(void *csound);

  /*
   * MESSAGES & TEXT
   */

  /**
   * Displays an informational message.
   */
  PUBLIC
#ifdef HAVE_GCC3
    __attribute__ ((__format__(__printf__, 2, 3)))
#endif
      void csoundMessage(void *csound, const char *format, ...);

  /**
   * Print message with special attributes (see msg_attr.h for the list of
   * available attributes). With attr=0, csoundMessageS() is identical to
   * csoundMessage().
   */
  PUBLIC
#ifdef HAVE_GCC3
    __attribute__ ((__format__(__printf__, 3, 4)))
#endif
      void csoundMessageS(void *csound, int attr, const char *format, ...);

  PUBLIC void csoundMessageV(void *csound, int attr, const char *format,
                                                     va_list args);

  /**
   * Throws an informational message as a C++ exception.
   */
  PUBLIC void csoundThrowMessage(void *csound, const char *format, ...);
  PUBLIC void csoundThrowMessageV(void *csound, const char *format,
                                                va_list args);

  /**
   * Sets a function to be called by Csound to print an informational message.
   */
  PUBLIC void csoundSetMessageCallback(void *csound,
                              void (*csoundMessageCallback)(void *csound,
                                                            int attr,
                                                            const char *format,
                                                            va_list valist));

  /**
   * Sets a function for Csound to stop execution with an error message or
   * exception.
   */
  PUBLIC void csoundSetThrowMessageCallback(void *csound,
                              void (*throwMessageCallback)(void *csound,
                                                           const char *format,
                                                           va_list valist));

  /**
   * Returns the Csound message level (from 0 to 7).
   */
  PUBLIC int csoundGetMessageLevel(void *csound);

  /**
   * Sets the Csound message level (from 0 to 7).
   */
  PUBLIC void csoundSetMessageLevel(void *csound, int messageLevel);

  /**
   * Input a NULL-terminated string (as if from a console)
   * usually used for lineevents
   */
  PUBLIC void csoundInputMessage(void *csound, const char *message);

  /**
   * Set the ASCII code of the most recent key pressed.
   * This value is used by the 'keypress' opcode.
   */
  PUBLIC void csoundKeyPress(void *csound, char c);

  /*
   * CONTROL AND EVENTS
   */

  /**
   * Control values are specified by a 'channelName' string.
   * Note that the 'invalue' & 'outvalue' channels can be specified by either
   * a string or a number.  If a number is specified, it will be converted
   * to a string before making the callbacks to the external software.
   * numerical channel names into text, so if the orchestra contains
   */

  /**
   * Called by external software to set a function for Csound to
   * fetch input control values.  The 'invalue' opcodes will
   * directly call this function.
   */
  PUBLIC void csoundSetInputValueCallback(void *csound,
                  void (*inputValueCalback)(void *csound,
                                            char *channelName, MYFLT *value));

  /**
   * Called by external software to set a function for Csound to
   * send output control values.  The 'outvalue' opcodes will
   * directly call this function.
   */
  PUBLIC void csoundSetOutputValueCallback(void *csound,
                  void (*outputValueCalback)(void *csound,
                                             char *channelName, MYFLT value));

  /**
   * Send a new score event. 'type' is the score event type ('i', 'f', or 'e')
   * 'numFields' is the size of the pFields array.  'pFields' is an array
   *  of floats with all the pfields for this event, starting with the p1 value
   *  specified in pFields[0].
   */
  PUBLIC int csoundScoreEvent(void *csound, char type,
                              MYFLT *pFields, long numFields);

  /*
   * MIDI
   */

  /**
   * Open MIDI input device 'devName', and store stream specific
   * data pointer in *userData. Return value is zero on success,
   * and a non-zero error code if an error occured.
   */
  int csoundExternalMidiInOpen(void *csound, void **userData,
                               const char *devName);

  /**
   * Read at most 'nbytes' bytes of MIDI data from input stream
   * 'userData', and store in 'buf'. Returns the actual number of
   * bytes read, which may be zero if there were no events, and
   * negative in case of an error. Note: incomplete messages (such
   * as a note on status without the data bytes) should not be
   * returned.
   */
  int csoundExternalMidiRead(void *csound, void *userData,
                             unsigned char *buf, int nbytes);

  /**
   * Close MIDI input device associated with 'userData'.
   * Return value is zero on success, and a non-zero error
   * code on failure.
   */
  int csoundExternalMidiInClose(void *csound, void *userData);

  /**
   * Open MIDI output device 'devName', and store stream specific
   * data pointer in *userData. Return value is zero on success,
   * and a non-zero error code if an error occured.
   */
  int csoundExternalMidiOutOpen(void *csound, void **userData,
                                const char *devName);

  /**
   * Write 'nbytes' bytes of MIDI data to output stream 'userData'
   * from 'buf' (the buffer will not contain incomplete messages).
   * Returns the actual number of bytes written, or a negative
   * error code.
   */
  int csoundExternalMidiWrite(void *csound, void *userData,
                              unsigned char *buf, int nbytes);

  /**
   * Close MIDI output device associated with '*userData'.
   * Return value is zero on success, and a non-zero error
   * code on failure.
   */
  int csoundExternalMidiOutClose(void *csound, void *userData);

  /**
   * Returns pointer to a string constant storing an error massage
   * for error code 'errcode'.
   */
  char *csoundExternalMidiErrorString(void *csound, int errcode);

  /* Set real time MIDI function pointers. */

  PUBLIC void csoundSetExternalMidiInOpenCallback(void *csound,
                                                  int (*func)(void*, void**,
                                                              const char*));

  PUBLIC void csoundSetExternalMidiReadCallback(void *csound,
                                                int (*func)(void*, void*,
                                                            unsigned char*,
                                                            int));

  PUBLIC void csoundSetExternalMidiInCloseCallback(void *csound,
                                                   int (*func)(void*, void*));

  PUBLIC void csoundSetExternalMidiOutOpenCallback(void *csound,
                                                   int (*func)(void*, void**,
                                                               const char*));

  PUBLIC void csoundSetExternalMidiWriteCallback(void *csound,
                                                 int (*func)(void*, void*,
                                                             unsigned char*,
                                                             int));

  PUBLIC void csoundSetExternalMidiOutCloseCallback(void *csound,
                                                    int (*func)(void*, void*));

  PUBLIC void csoundSetExternalMidiErrorStringCallback(void *csound,
                                                       char *(*func)(int));

  /*
   * FUNCTION TABLE DISPLAY
   */

  /**
   * Tells Csound supports external graphic table display.
   */
  PUBLIC void csoundSetIsGraphable(void *csound, int isGraphable);

  /**
   * Called by external software to set Csound's MakeGraph function.
   */
  PUBLIC void csoundSetMakeGraphCallback(void *csound,
                        void (*makeGraphCallback)(void *csound,
                                                  WINDAT *windat, char *name));

  /**
   * Called by external software to set Csound's DrawGraph function.
   */
  PUBLIC void csoundSetDrawGraphCallback(void *csound,
                        void (*drawGraphCallback)(void *csound,
                                                  WINDAT *windat));

  /**
   * Called by external software to set Csound's KillGraph function.
   */
  PUBLIC void csoundSetKillGraphCallback(void *csound,
                        void (*killGraphCallback)(void *csound,
                                                  WINDAT *windat));

  /**
   * Called by external software to set Csound's ExitGraph function.
   */
  PUBLIC void csoundSetExitGraphCallback(void *csound,
                        int (*exitGraphCallback)(void *csound));

  /*
   * OPCODES
   */

  /*
   * Gets a list of all opcodes.
   * Make sure to call csoundDisposeOpcodeList() when done with the list.
   */
  PUBLIC opcodelist *csoundNewOpcodeList(void);

  /*
   * Releases an opcode list
   */
  PUBLIC void csoundDisposeOpcodeList(opcodelist *opcodelist_);

  /**
   * Appends an opcode implemented by external software
   * to Csound's internal opcode list.
   * The opcode list is extended by one slot,
   * and the parameters are copied into the new slot.
   * Returns zero on success.
   */
  PUBLIC int csoundAppendOpcode(void *csound, char *opname, int dsblksiz,
                                int thread, char *outypes, char *intypes,
                                int (*iopadr)(void *, void *),
                                int (*kopadr)(void *, void *),
                                int (*aopadr)(void *, void *),
                                int (*dopadr)(void *, void *));

#ifndef SWIG
  /*
   * Signature for external registration function,
   * such as for plugin opcodes or scripting languages.
   * The external has complete access to the Csound API,
   * so a plugin opcode can just call csound->AppendOpcode()
   * for each of its opcodes.
   */
  typedef PUBLIC int (*CsoundRegisterExternalType)(void *csound);
#endif

  /*
   * Registers all opcodes in the library.
   */
  PUBLIC int csoundLoadExternal(void *csound, const char *libraryPath);

  /*
   * Registers all opcodes in all libraries in the opcodes directory.
   */
  PUBLIC int csoundLoadExternals(void *csound);

  /*
   * MISCELLANEOUS FUNCTIONS
   */

  /*
   * Platform-independent function to load a shared library.
   */
  PUBLIC void *csoundOpenLibrary(const char *libraryPath);

  /*
   * Platform-independent function to unload a shared library.
   */
  PUBLIC int csoundCloseLibrary(void *library);

  /*
   * Platform-independent function to get a symbol address in a shared library.
   */
  PUBLIC void *csoundGetLibrarySymbol(void *library, const char *symbolName);

  /*
   * Check system events, yielding cpu time for
   * coopertative multitasking, etc.
   */
  PUBLIC int csoundYield(void *);

  /**
   * Called by external software to set a function for
   * checking system events, yielding cpu time for
   * coopertative multitasking, etc..
   * This function is optional.  It is often used as a way
   * to 'turn off' Csound, allowing it to exit gracefully.
   * In addition, some operations like utility analysis
   * routines are not reentrant and you should use this
   * function to do any kind of updating during the operation.
   *
   * Returns an 'OK to continue' boolean
   */
  PUBLIC void csoundSetYieldCallback(void *csound,
                                     int (*yieldCallback)(void *csound));

  /**
   *       REAL-TIME AUDIO PLAY AND RECORD
   */

  /* IV - Feb 01 2005: real-time audio parameters structure */

  typedef struct {
      char    *devName;       /* device name (NULL/empty: default)           */
      int     devNum;         /* device number (0-1023), 1024: default       */
      int     bufSamp_SW;     /* buffer fragment size in samples             */
                              /*   (*not* multiplied by nchnls)              */
      int     bufSamp_HW;     /* total buffer size in samples ( = O.oMaxLag) */
      int     nChannels;      /* number of channels                          */
      int     sampleFormat;   /* sample format (AE_SHORT etc.)               */
      float   sampleRate;     /* sample rate in Hz                           */
  } csRtAudioParams;

  /**
   * Sets a function to be called by Csound for opening real-time
   * audio playback.
   */
  PUBLIC void csoundSetPlayopenCallback(void *csound,
                                    int (*playopen__)(void *csound,
                                                      csRtAudioParams *parm));

  /**
   * Sets a function to be called by Csound for performing real-time
   * audio playback.
   */
  PUBLIC void csoundSetRtplayCallback(void *csound,
                                      void (*rtplay__)(void *csound,
                                                       void *outBuf,
                                                       int nbytes));

  /**
   * Sets a function to be called by Csound for opening real-time
   * audio recording.
   */
  PUBLIC void csoundSetRecopenCallback(void *csound,
                                       int (*recopen_)(void *csound,
                                                       csRtAudioParams *parm));

  /**
   * Sets a function to be called by Csound for performing real-time
   * audio recording.
   */
  PUBLIC void csoundSetRtrecordCallback(void *csound,
                                        int (*rtrecord__)(void *csound,
                                                          void *inBuf,
                                                          int nbytes));

  /**
   * Sets a function to be called by Csound for closing real-time
   * audio playback and recording.
   */
  PUBLIC void csoundSetRtcloseCallback(void *csound,
                                       void (*rtclose__)(void *csound));

  /**
   * Returns whether Csound is in debug mode.
   */
  PUBLIC int csoundGetDebug(void *csound);

  /**
   * Sets whether Csound is in debug mode.
   */
  PUBLIC void csoundSetDebug(void *csound, int debug);

  /**
   * Returns the length of a function table, or -1 if the table does
   * not exist.
   */
  PUBLIC int csoundTableLength(void *csound, int table);

  /**
   * Returns the value of a slot in a function table.
   */
  PUBLIC MYFLT csoundTableGet(void *csound, int table, int index);

  /**
   * Sets the value of a slot in a function table.
   */
  PUBLIC void csoundTableSet(void *csound, int table, int index, MYFLT value);

  /**
   * Creates and starts a new thread of execution.
   * Returns an opaque pointer that represents the thread on success, or
   * null for failure.
   * The userdata pointer is passed to the thread routine.
   */
  PUBLIC void *csoundCreateThread(void *csound,
                                  int (*threadRoutine)(void *userdata),
                                  void *userdata);

  /**
   * Waits until the indicated thread's routine has finished.
   * Returns the value returned by the thread routine.
   */
  PUBLIC int csoundJoinThread(void *csound, void *thread);

  /**
   * Creates and returns a monitor object, or null if not successful.
   */
  PUBLIC void *csoundCreateThreadLock(void *csound);

  /**
   * Waits on the indicated monitor object for the indicated period.
   * The function returns either when the monitor object is notified,
   * or when the period has elapsed, whichever is sooner.
   * If the period is 0, the wait is infinite.
   */
  PUBLIC void csoundWaitThreadLock(void *csound, void *lock,
                                                 size_t milliseconds);

  /**
   * Notifies the indicated monitor object.
   */
  PUBLIC void csoundNotifyThreadLock(void *csound, void *lock);

  /**
   * Destroys the indicated monitor object.
   */
  PUBLIC void csoundDestroyThreadLock(void *csound, void *lock);

  /**
   * Sets whether or not the FLTK widget thread calls Fl::lock().
   */
  PUBLIC void csoundSetFLTKThreadLocking(void *csound, int isLocking);

  /**
   * Returns whether or not the FLTK widget thread calls Fl::lock().
   */
  PUBLIC int csoundGetFLTKThreadLocking(void *csound);

#ifndef RTCLOCK_S_DEFINED
#define RTCLOCK_S_DEFINED

typedef struct RTCLOCK_S {
    unsigned long   starttime_real_high;
    unsigned long   starttime_real_low;
    double          real_time_to_seconds_scale;
    unsigned long   starttime_CPU_high;
    unsigned long   starttime_CPU_low;
    double          CPU_time_to_seconds_scale;
} RTCLOCK;

#endif

  /**
   * initialise a timer structure
   */
  PUBLIC void timers_struct_init(RTCLOCK*);

  /**
   * Return the elapsed real time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double timers_get_real_time(RTCLOCK*);

  /**
   * Return the elapsed CPU time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double timers_get_CPU_time(RTCLOCK*);

  /**
   * Return a 32-bit unsigned integer to be used as seed from current time.
   */
  PUBLIC unsigned long timers_random_seed(void);

  /**
   * Set language to 'lang_code' (lang_code can be for example
   * CSLANGUAGE_ENGLISH_UK or CSLANGUAGE_FRENCH or many others,
   * see n_getstr.h for the list of languages). This affects all
   * Csound instances running in the address space of the current
   * process. The special language code CSLANGUAGE_DEFAULT can be
   * used to disable translation of messages and free all memory
   * allocated by a previous call to csoundSetLanguage().
   * csoundSetLanguage() loads all files for the selected language
   * from the directory specified by the CSSTRNGS environment
   * variable.
   */
  PUBLIC void csoundSetLanguage(cslanguage_t lang_code);

  /**
   * Translate string 's' to the current language, and return
   * pointer to the translated message. This may be the same as
   * 's' if language was set to CSLANGUAGE_DEFAULT.
   */
  PUBLIC char *csoundLocalizeString(const char *s);

  /**
   * Allocate nbytes bytes of memory that can be accessed later by calling
   * csoundQueryGlobalVariable() with the specified name; the space is
   * cleared to zero.
   * Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
   * parameters (zero nbytes, invalid or already used name), or
   * CSOUND_MEMORY if there is not enough memory.
   */
  PUBLIC int csoundCreateGlobalVariable(void *csound, const char *name,
                                        size_t nbytes);

  /**
   * Get pointer to space allocated with the name "name".
   * Returns NULL if the specified name is not defined.
   */
  PUBLIC void *csoundQueryGlobalVariable(void *csound, const char *name);

  /**
   * This function is the same as csoundQueryGlobalVariable(), except the
   * variable is assumed to exist and no error checking is done.
   * Faster, but may crash or return an invalid pointer if 'name' is
   * not defined.
   */
  PUBLIC void *csoundQueryGlobalVariableNoCheck(void *csound, const char *name);

  /**
   * Free memory allocated for "name" and remove "name" from the database.
   * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
   * not defined.
   */
  PUBLIC int csoundDestroyGlobalVariable(void *csound, const char *name);

  /**
   * Return the size of MYFLT in bytes.
   */
  PUBLIC int csoundGetSizeOfMYFLT(void);

  /**
   * Return pointer to user data pointer for real time audio input.
   */
  PUBLIC void **csoundGetRtRecordUserData(void *csound);

  /**
   * Return pointer to user data pointer for real time audio output.
   */
  PUBLIC void **csoundGetRtPlayUserData(void *csound);

  /**
   * Register a function to be called once in every control period
   * by sensevents(). Any number of functions may be registered.
   * The callback function takes two arguments of type void*, the first
   * is the Csound instance pointer, and the second is the userData pointer
   * as passed to this function.
   * Returns zero on success.
   */
  PUBLIC int csoundRegisterSenseEventCallback(void *csound_,
                                              void (*func)(void *, void *),
                                              void *userData);

/* type/macro definitions and interface functions for configuration variables */
#include "cfgvar.h"
/* interface functions for environment variables, and finding files */
#include "envvar.h"
/* interface functions for complex and real FFT */
#include "fftlib.h"
/* message attribute definitions (for csoundMessageS() and csoundMessageV()) */
#include "msg_attr.h"

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CSOUND_H */

