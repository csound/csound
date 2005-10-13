/**
 * C S O U N D
 *
 * An auto-extensible system for making music on computers
 * by means of software alone.
 *
 * Copyright (C) 2001-2005 Michael Gogins, Matt Ingalls, John D. Ramsdell,
 *                         John P. ffitch, Istvan Varga
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

#ifndef CSOUND_H
#define CSOUND_H

/** \file
 * \brief Declares the public Csound application programming interface (API).
 * \author John P. ffitch, Michael Gogins, Matt Ingalls, John D. Ramsdell,
 *         and Istvan Varga
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
 * Csound API library. Plugin libraries should #include <csdl.h> to get
 * access to the API function pointers in the CSOUND structure, and do not
 * need to link with the Csound API library.
 * Only one of csound.h and csdl.h may be included by a compilation unit.
 *
 * Hosts must first create an instance of Csound using the \c csoundCreate
 * API function. When hosts are finished using Csound, they must destroy the
 * instance of csound using the \c csoundDestroy API function.
 * Most of the other Csound API functions take the Csound instance as their
 * first argument.
 * Hosts can only call the standalone API functions declared in csound.h.
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
 *     CSOUND *csound = csoundCreate(NULL);
 *     int result = csoundPerform(csound, argc, argv);
 *     csoundDestroy(csound);
 *     return result;
 * }
 *
 * \endcode
 *
 * All opcodes, including plugins, receive a pointer to their host
 * instance of Csound as the first argument. Therefore, plugins MUST NOT
 * compile, perform, or destroy the host instance of Csound, and MUST call
 * the Csound API function pointers off the Csound instance pointer.
 *
 * \code
 * MYFLT sr = csound->GetSr(csound);
 * \endcode
 *
 * In general, plugins should ONLY access Csound functionality through the
 * API function pointers and public members of the CSOUND structure.
 */

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Platform-dependent definitions and declarations.
   */

#if (defined(WIN32) || defined(_WIN32)) && !defined(SWIG)
#  define PUBLIC        __declspec(dllexport)
#else
#  define PUBLIC
#endif

  /**
   * Forward declaration.
   */

  typedef struct CSOUND_ CSOUND;

  /**
   * Enables Python interface.
   */

#ifdef SWIG
#define CS_PRINTF2
#define CS_PRINTF3
#ifndef __MYFLT_DEF
#define __MYFLT_DEF
#ifndef USE_DOUBLE
#define MYFLT float
#else
#define MYFLT double
#endif
#endif
%module csound
%{
#  include "sysdep.h"
#  include "text.h"
#  include <stdarg.h>
%}
#else
#  include "sysdep.h"
#  include "text.h"
#  include <stdarg.h>
#endif

  /**
   * ERROR DEFINITIONS
   */

  typedef enum
    {
      /* Completed successfully. */
      CSOUND_SUCCESS = 0,
      /* Unspecified failure. */
      CSOUND_ERROR = -1,
      /* Failed during initialization. */
      CSOUND_INITIALIZATION = -2,
      /* Failed during performance. */
      CSOUND_PERFORMANCE = -3,
      /* Failed to allocate requested memory. */
      CSOUND_MEMORY = -4,
      /* Termination requested by SIGINT or SIGTERM. */
      CSOUND_SIGNAL = -5
    }
  CSOUND_STATUS;

  /* Compilation or performance aborted, but not as a result of an error
     (e.g. --help, or running an utility with -U). */
#define CSOUND_EXITJMP_SUCCESS  (256)

  /**
   * Flags for csoundInitialize().
   */
#define CSOUNDINIT_NO_SIGNAL_HANDLER  1
#define CSOUNDINIT_NO_ATEXIT          2

  /**
   * Constants used by the bus interface (csoundGetChannelPtr() etc.).
   */
#define CSOUND_CONTROL_CHANNEL      1
#define CSOUND_AUDIO_CHANNEL        2
#define CSOUND_STRING_CHANNEL       3

#define CSOUND_CHANNEL_TYPE_MASK    15

#define CSOUND_INPUT_CHANNEL        16
#define CSOUND_OUTPUT_CHANNEL       32

#define CSOUND_CONTROL_CHANNEL_INT  1
#define CSOUND_CONTROL_CHANNEL_LIN  2
#define CSOUND_CONTROL_CHANNEL_EXP  3

  /**
   * TYPE DEFINITIONS
   */

  /**
   * Real-time audio parameters structure
   */
  typedef struct {
    char    *devName;       /* device name (NULL/empty: default)          */
    int     devNum;         /* device number (0-1023), 1024: default      */
    int     bufSamp_SW;     /* buffer fragment size (-b) in sample frames */
    int     bufSamp_HW;     /* total buffer size (-B) in sample frames    */
    int     nChannels;      /* number of channels                         */
    int     sampleFormat;   /* sample format (AE_SHORT etc.)              */
    float   sampleRate;     /* sample rate in Hz                          */
  } csRtAudioParams;

  typedef struct RTCLOCK_S {
    int_least64_t   starttime_real;
    int_least64_t   starttime_CPU;
  } RTCLOCK;

  typedef struct {
    char    *opname;
    char    *outypes;
    char    *intypes;
  } opcodeListEntry;

  typedef struct CsoundRandMTState_ {
    int       mti;
    uint32_t  mt[624];
  } CsoundRandMTState;

  typedef struct windat_  WINDAT;
  typedef struct xyindat_ XYINDAT;

#ifndef CSOUND_CSDL_H

  /**
   * INSTANTIATION
   */

  /**
   * Initialise Csound library; should be called once before creating
   * any Csound instances.
   * Returns zero on success.
   */
  PUBLIC int csoundInitialize(int *argc, char ***argv, int flags);

  /**
   * Creates an instance of Csound.
   * Returns an opaque pointer that must be passed to most Csound API functions.
   * The hostData parameter can be NULL, or it can be a pointer to any sort of
   * data; this pointer can be accessed from the Csound instance that is passed
   * to callback routines.
   */
  PUBLIC CSOUND *csoundCreate(void *hostData);

  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  PUBLIC int csoundPreCompile(CSOUND *);

  /**
   * csoundInitializeCscore() prepares an instance of Csound for Cscore
   * processing outside of running an orchestra (i.e. "standalone Cscore").
   * It is an alternative to csoundPreCompile(), csoundCompile(), and
   * csoundPerform*() and should not be used with these functions.
   * You must call this function before using the interface in "cscore.h"
   * when you do not wish to compile an orchestra.
   * Pass it the already open FILE* pointers to the input and
   * output score files.
   * It returns CSOUND_SUCCESS on success and CSOUND_INITIALIZATION or other
   * error code if it fails.
   */
  PUBLIC int csoundInitializeCscore(CSOUND *, FILE* insco, FILE* outsco);

  /**
   * Returns a pointer to the requested interface, if available, in the
   * interface argument, and its version number, in the version argument.
   * Returns 0 for success.
   */
  PUBLIC int csoundQueryInterface(const char *name, void **iface, int *version);

  /**
   * Destroys an instance of Csound.
   */
  PUBLIC void csoundDestroy(CSOUND *);

  /**
   * Returns the version number times 1000 (5.00.0 = 5000).
   */
  PUBLIC int csoundGetVersion(void);

  /**
   * Returns the API version number times 100 (1.00 = 100).
   */
  PUBLIC int csoundGetAPIVersion(void);

  /**
   * Returns host data.
   */
  PUBLIC void *csoundGetHostData(CSOUND *);

  /**
   * Sets host data.
   */
  PUBLIC void csoundSetHostData(CSOUND *, void *hostData);

  /**
   * Get pointer to value of environment variable 'name'.
   * Should be called after csoundPreCompile() or csoundCompile().
   * Return value is NULL if the variable is not set.
   */
  PUBLIC const char *csoundGetEnv(CSOUND *csound, const char *name);

  /**
   * Set the default value of environment variable 'name' to 'value'.
   * It is not safe to call this function while any Csound instances
   * are active.
   * Returns zero on success.
   */
  PUBLIC int csoundSetDefaultEnv(const char *name, const char *value);

  /**
   * PERFORMANCE
   */

  /**
   * Compiles and renders a Csound performance,
   * as directed by the supplied command-line arguments,
   * in one pass. Returns 0 for success.
   */
  PUBLIC int csoundPerform(CSOUND *, int argc, char **argv);

  /**
   * Compiles Csound input files (such as an orchestra and score)
   * as directed by the supplied command-line arguments,
   * but does not perform them. Returns a non-zero error code on failure.
   * In this (host-driven) mode, the sequence of calls should be as follows:
   * /code
   *       csoundCompile(csound, argc, argv);
   *       while (!csoundPerformBuffer(csound));
   *       csoundCleanup(csound);
   *       csoundReset(csound);
   * /endcode
   */
  PUBLIC int csoundCompile(CSOUND *, int argc, char **argv);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output.
   * Note that csoundCompile must be called first.
   * Returns false during performance, and true when performance is finished.
   * If called until it returns true, will perform an entire score.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int csoundPerformKsmps(CSOUND *);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output.
   * Note that csoundCompile must be called first.
   * Performs audio whether or not the Csound score has finished.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int csoundPerformKsmpsAbsolute(CSOUND *);

  /**
   * Performs Csound, sensing real-time and score events
   * and processing one buffer's worth (-b frames) of interleaved audio.
   * Returns a pointer to the new output audio in 'outputAudio'
   * Note that csoundCompile must be called first, then call
   * csoundGetOutputBuffer() and csoundGetInputBuffer() to get the pointer
   * to csound's I/O buffers.
   * Returns false during performance, and true when performance is finished.
   */
  PUBLIC int csoundPerformBuffer(CSOUND *);

  /**
   * Prints information about the end of a performance.
   * After calling csoundCleanup(), the operation of the perform
   * functions is undefined.
   */
  PUBLIC int csoundCleanup(CSOUND *);

  /**
   * Resets all internal memory and state in preparation for a new performance.
   * Enables external software to run successive Csound performances
   * without reloading Csound. Implies csoundCleanup(), unless already called.
   */
  PUBLIC void csoundReset(CSOUND *);

  /**
   * ATTRIBUTES
   */

  /**
   * Returns the number of audio sample frames per second.
   */
  PUBLIC MYFLT csoundGetSr(CSOUND *);

  /**
   * Returns the number of control samples per second.
   */
  PUBLIC MYFLT csoundGetKr(CSOUND *);

  /**
   * Returns the number of audio sample frames per control sample.
   */
  PUBLIC int csoundGetKsmps(CSOUND *);

  /**
   * Returns the number of audio output channels.
   */
  PUBLIC int csoundGetNchnls(CSOUND *);

  /**
   * Returns the 0dBFS level of the spin/spout buffers.
   */
  PUBLIC MYFLT csoundGet0dBFS(CSOUND *);

  /**
   * Returns the number of bytes allocated for a string variable
   * (the actual length is one less because of the null character
   * at the end of the string). Should be called after csoundCompile().
   */
  PUBLIC int csoundGetStrVarMaxLen(CSOUND *);

  /**
   * Returns the sample format.
   */
  PUBLIC int csoundGetSampleFormat(CSOUND *);

  /**
   * Returns the size in bytes of a single sample.
   */
  PUBLIC int csoundGetSampleSize(CSOUND *);

  /**
   * Returns the number of samples in Csound's input buffer.
   */
  PUBLIC long csoundGetInputBufferSize(CSOUND *);

  /**
   * Returns the number of samples in Csound's output buffer.
   */
  PUBLIC long csoundGetOutputBufferSize(CSOUND *);

  /**
   * Returns the address of the Csound audio input buffer.
   * Enables external software to write audio into Csound before calling
   * csoundPerformBuffer.
   */
  PUBLIC MYFLT *csoundGetInputBuffer(CSOUND *);

  /**
   * Returns the address of the Csound audio output buffer.
   * Enables external software to read audio from Csound after calling
   * csoundPerformBuffer.
   */
  PUBLIC MYFLT *csoundGetOutputBuffer(CSOUND *);

  /**
   * Returns the address of the Csound audio input working buffer (spin).
   * Enables external software to write audio into Csound before calling
   * csoundPerformKsmps.
   */
  PUBLIC MYFLT *csoundGetSpin(CSOUND *);

  /**
   * Returns the address of the Csound audio output working buffer (spout).
   * Enables external software to read audio from Csound after calling
   * csoundPerformKsmps.
   */
  PUBLIC MYFLT *csoundGetSpout(CSOUND *);

  /**
   * Returns the output sound file name (-o).
   */
  PUBLIC const char *csoundGetOutputFileName(CSOUND *);

  /**
   * Calling this function with a non-zero 'state' value between
   * csoundPreCompile() and csoundCompile() will disable all default
   * handling of sound I/O by the Csound library, allowing the host
   * application to use the spin/spout/input/output buffers directly.
   * If 'bufSize' is greater than zero, the buffer size (-b) will be
   * set to the integer multiple of ksmps that is nearest to the value
   * specified.
   */
  PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *, int state, int bufSize);

  /**
   * Returns the current score time in seconds
   * since the beginning of performance.
   */
  PUBLIC MYFLT csoundGetScoreTime(CSOUND *);

  /**
   * SCORE HANDLING
   */

  /**
   * Sets whether Csound score events are performed or not, independently
   * of real-time MIDI events (see csoundSetScorePending()).
   */
  PUBLIC int csoundIsScorePending(CSOUND *);

  /**
   * Sets whether Csound score events are performed or not (real-time
   * events will continue to be performed). Can be used by external software,
   * such as a VST host, to turn off performance of score events (while
   * continuing to perform real-time events), for example to
   * mute a Csound score while working on other tracks of a piece, or
   * to play the Csound instruments live.
   */
  PUBLIC void csoundSetScorePending(CSOUND *, int pending);

  /**
   * Returns the score time beginning at which score events will
   * actually immediately be performed (see csoundSetScoreOffsetSeconds()).
   */
  PUBLIC MYFLT csoundGetScoreOffsetSeconds(CSOUND *);

  /**
   * Csound score events prior to the specified time are not performed, and
   * performance begins immediately at the specified time (real-time events
   * will continue to be performed as they are received).
   * Can be used by external software, such as a VST host,
   * to begin score performance midway through a Csound score,
   * for example to repeat a loop in a sequencer, or to synchronize
   * other events with the Csound score.
   */
  PUBLIC void csoundSetScoreOffsetSeconds(CSOUND *, MYFLT time);

  /**
   * Rewinds a compiled Csound score to the time specified with
   * csoundSetScoreOffsetSeconds().
   */
  PUBLIC void csoundRewindScore(CSOUND *);

  /**
   * Sets an external callback for Cscore processing.
   * Pass NULL to reset to the internal cscore() function (which does nothing).
   * This callback is retained after a csoundReset() call.
   */
  PUBLIC void csoundSetCscoreCallback(CSOUND *,
                                      void (*cscoreCallback)(CSOUND *));

  /**
   * MESSAGES & TEXT
   */

  /**
   * Displays an informational message.
   */
  PUBLIC CS_PRINTF2 void csoundMessage(CSOUND *, const char *format, ...);

  /**
   * Print message with special attributes (see msg_attr.h for the list of
   * available attributes). With attr=0, csoundMessageS() is identical to
   * csoundMessage().
   */
  PUBLIC CS_PRINTF3 void csoundMessageS(CSOUND *,
                                        int attr, const char *format, ...);

  PUBLIC void csoundMessageV(CSOUND *,
                             int attr, const char *format, va_list args);

  /**
   * Throws an informational message as a C++ exception.
   */
  PUBLIC void csoundThrowMessage(CSOUND *, const char *format, ...);

  PUBLIC void csoundThrowMessageV(CSOUND *, const char *format, va_list args);

  /**
   * Sets a function to be called by Csound to print an informational message.
   */
  PUBLIC void csoundSetMessageCallback(CSOUND *,
                            void (*csoundMessageCallback)(CSOUND *,
                                                          int attr,
                                                          const char *format,
                                                          va_list valist));

  /**
   * Sets a function for Csound to stop execution with an error message or
   * exception.
   */
  PUBLIC void csoundSetThrowMessageCallback(CSOUND *,
                            void (*throwMessageCallback)(CSOUND *,
                                                         const char *format,
                                                         va_list valist));

  /**
   * Returns the Csound message level (from 0 to 231).
   */
  PUBLIC int csoundGetMessageLevel(CSOUND *);

  /**
   * Sets the Csound message level (from 0 to 231).
   */
  PUBLIC void csoundSetMessageLevel(CSOUND *, int messageLevel);

  /**
   * Input a NULL-terminated string (as if from a console),
   * used for line events (requires -L command line option).
   */
  PUBLIC void csoundInputMessage(CSOUND *, const char *message);

  /**
   * Set the ASCII code of the most recent key pressed.
   * This value is used by the 'keypress' opcode.
   */
  PUBLIC void csoundKeyPress(CSOUND *, char c);

  /**
   * CONTROL AND EVENTS
   */

  /**
   * Control values are specified by a 'channelName' string.
   * Note that the 'invalue' & 'outvalue' channels can be specified by
   * either a string or a number.  If a number is specified, it will be
   * converted to a string before making the callbacks to the external
   * software.
   */

  /**
   * Called by external software to set a function for Csound to
   * fetch input control values.  The 'invalue' opcodes will
   * directly call this function.
   */
  PUBLIC void csoundSetInputValueCallback(CSOUND *,
                            void (*inputValueCalback)(CSOUND *,
                                                      const char *channelName,
                                                      MYFLT *value));

  /**
   * Called by external software to set a function for Csound to
   * send output control values.  The 'outvalue' opcodes will
   * directly call this function.
   */
  PUBLIC void csoundSetOutputValueCallback(CSOUND *,
                            void (*outputValueCalback)(CSOUND *,
                                                       const char *channelName,
                                                       MYFLT value));

  /**
   * Send a new score event. 'type' is the score event type ('a', 'i', 'q',
   * 'f', or 'e').
   * 'numFields' is the size of the pFields array.  'pFields' is an array of
   * floats with all the pfields for this event, starting with the p1 value
   * specified in pFields[0].
   */
  PUBLIC int csoundScoreEvent(CSOUND *,
                              char type, const MYFLT *pFields, long numFields);

  /**
   * MIDI
   */

  /* Set real time MIDI callback function pointers. */

  PUBLIC void csoundSetExternalMidiInOpenCallback(CSOUND *,
                    int (*func)(CSOUND *, void **, const char *));

  PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *,
                    int (*func)(CSOUND *, void *, unsigned char *, int));

  PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *,
                    int (*func)(CSOUND *, void *));

  PUBLIC void csoundSetExternalMidiOutOpenCallback(CSOUND *,
                    int (*func)(CSOUND *, void **, const char *));

  PUBLIC void csoundSetExternalMidiWriteCallback(CSOUND *,
                    int (*func)(CSOUND *, void *, const unsigned char *, int));

  PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *,
                    int (*func)(CSOUND *, void *));

  PUBLIC void csoundSetExternalMidiErrorStringCallback(CSOUND *,
                    const char *(*func)(int));

  /**
   * FUNCTION TABLE DISPLAY
   */

  /**
   * Tells Csound whether external graphic table display is supported.
   * Returns the previously set value (initially zero).
   */
  PUBLIC int csoundSetIsGraphable(CSOUND *, int isGraphable);

  /**
   * Called by external software to set Csound's MakeGraph function.
   */
  PUBLIC void csoundSetMakeGraphCallback(CSOUND *,
                            void (*makeGraphCallback)(CSOUND *,
                                                      WINDAT *windat,
                                                      const char *name));

  /**
   * Called by external software to set Csound's DrawGraph function.
   */
  PUBLIC void csoundSetDrawGraphCallback(CSOUND *,
                            void (*drawGraphCallback)(CSOUND *,
                                                      WINDAT *windat));

  /**
   * Called by external software to set Csound's KillGraph function.
   */
  PUBLIC void csoundSetKillGraphCallback(CSOUND *,
                            void (*killGraphCallback)(CSOUND *,
                                                      WINDAT *windat));

  /**
   * Called by external software to set Csound's MakeXYin function.
   */
  PUBLIC void csoundSetMakeXYinCallback(CSOUND *,
                            void (*makeXYinCallback)(CSOUND *, XYINDAT *,
                                                     MYFLT x, MYFLT y));

  /**
   * Called by external software to set Csound's ReadXYin function.
   */
  PUBLIC void csoundSetReadXYinCallback(CSOUND *,
                            void (*readXYinCallback)(CSOUND *, XYINDAT *));

  /**
   * Called by external software to set Csound's KillXYin function.
   */
  PUBLIC void csoundSetKillXYinCallback(CSOUND *,
                            void (*killXYinCallback)(CSOUND *, XYINDAT *));

  /**
   * Called by external software to set Csound's ExitGraph function.
   */
  PUBLIC void csoundSetExitGraphCallback(CSOUND *,
                            int (*exitGraphCallback)(CSOUND *));

  /**
   * OPCODES
   */

  /**
   * Gets an alphabetically sorted list of all opcodes.
   * Should be called after externals are loaded by csoundCompile().
   * Returns the number of opcodes, or a negative error code on failure.
   * Make sure to call csoundDisposeOpcodeList() when done with the list.
   */
  PUBLIC int csoundNewOpcodeList(CSOUND *, opcodeListEntry **opcodelist);

  /**
   * Releases an opcode list.
   */
  PUBLIC void csoundDisposeOpcodeList(CSOUND *, opcodeListEntry *opcodelist);

  /**
   * Appends an opcode implemented by external software
   * to Csound's internal opcode list.
   * The opcode list is extended by one slot,
   * and the parameters are copied into the new slot.
   * Returns zero on success.
   */
  PUBLIC int csoundAppendOpcode(CSOUND *, const char *opname,
                                int dsblksiz, int thread,
                                const char *outypes, const char *intypes,
                                int (*iopadr)(CSOUND *, void *),
                                int (*kopadr)(CSOUND *, void *),
                                int (*aopadr)(CSOUND *, void *));

  /**
   * MISCELLANEOUS FUNCTIONS
   */

  /**
   * Platform-independent function to load a shared library.
   */
  PUBLIC void *csoundOpenLibrary(const char *libraryPath);

  /**
   * Platform-independent function to unload a shared library.
   */
  PUBLIC int csoundCloseLibrary(void *library);

  /**
   * Platform-independent function to get a symbol address in a shared library.
   */
  PUBLIC void *csoundGetLibrarySymbol(void *library, const char *symbolName);

  /**
   * Called by external software to set a function for
   * checking system events, yielding cpu time for
   * coopertative multitasking, etc.
   * This function is optional.  It is often used as a way
   * to 'turn off' Csound, allowing it to exit gracefully.
   * In addition, some operations like utility analysis
   * routines are not reentrant and you should use this
   * function to do any kind of updating during the operation.
   *
   * Returns an 'OK to continue' boolean.
   */
  PUBLIC void csoundSetYieldCallback(CSOUND *, int (*yieldCallback)(CSOUND *));

  /**
   * REAL-TIME AUDIO PLAY AND RECORD
   */

  /**
   * Sets a function to be called by Csound for opening real-time
   * audio playback.
   */
  PUBLIC void csoundSetPlayopenCallback(CSOUND *,
                            int (*playopen__)(CSOUND *,
                                              const csRtAudioParams *parm));

  /**
   * Sets a function to be called by Csound for performing real-time
   * audio playback.
   */
  PUBLIC void csoundSetRtplayCallback(CSOUND *,
                            void (*rtplay__)(CSOUND *,
                                             const MYFLT *outBuf, int nbytes));

  /**
   * Sets a function to be called by Csound for opening real-time
   * audio recording.
   */
  PUBLIC void csoundSetRecopenCallback(CSOUND *,
                            int (*recopen_)(CSOUND *,
                                            const csRtAudioParams *parm));

  /**
   * Sets a function to be called by Csound for performing real-time
   * audio recording.
   */
  PUBLIC void csoundSetRtrecordCallback(CSOUND *,
                            int (*rtrecord__)(CSOUND *,
                                              MYFLT *inBuf, int nbytes));

  /**
   * Sets a function to be called by Csound for closing real-time
   * audio playback and recording.
   */
  PUBLIC void csoundSetRtcloseCallback(CSOUND *, void (*rtclose__)(CSOUND *));

  /**
   * Returns whether Csound is in debug mode.
   */
  PUBLIC int csoundGetDebug(CSOUND *);

  /**
   * Sets whether Csound is in debug mode.
   */
  PUBLIC void csoundSetDebug(CSOUND *, int debug);

  /**
   * Returns the length of a function table (not including the guard point),
   * or -1 if the table does not exist.
   */
  PUBLIC int csoundTableLength(CSOUND *, int table);

  /**
   * Returns the value of a slot in a function table.
   * The table number and index are assumed to be valid.
   */
  PUBLIC MYFLT csoundTableGet(CSOUND *, int table, int index);

  /**
   * Sets the value of a slot in a function table.
   * The table number and index are assumed to be valid.
   */
  PUBLIC void csoundTableSet(CSOUND *, int table, int index, MYFLT value);

  /**
   * Returns pointer to a function table, and stores table length (not
   * including the guard point) in *tableLength.
   * If the table does not exist, NULL is returned.
   */
  PUBLIC MYFLT *csoundGetTable(CSOUND *, int tableNum, int *tableLength);

  /**
   * Creates and starts a new thread of execution.
   * Returns an opaque pointer that represents the thread on success,
   * or NULL for failure.
   * The userdata pointer is passed to the thread routine.
   */
  PUBLIC void *csoundCreateThread(uintptr_t (*threadRoutine)(void *),
                                  void *userdata);

  /**
   * Waits until the indicated thread's routine has finished.
   * Returns the value returned by the thread routine.
   */
  PUBLIC uintptr_t csoundJoinThread(void *thread);

  /**
   * Creates and returns a monitor object, or NULL if not successful.
   * The object is initially in signaled (notified) state.
   */
  PUBLIC void *csoundCreateThreadLock(void);

  /**
   * Waits on the indicated monitor object for the indicated period
   * (the timeout is not implemented on some platforms, so any non-zero
   * value may possibly request infinite wait time).
   * The function returns either when the monitor object is notified,
   * or when the period has elapsed, whichever is sooner; in the first case,
   * zero is returned.
   * If 'milliseconds' is zero and the object is not notified, the function
   * will return immediately with a non-zero status.
   */
  PUBLIC int csoundWaitThreadLock(void *lock, size_t milliseconds);

  /**
   * Waits on the indicated monitor object until it is notified.
   * This function is similar to csoundWaitThreadLock() with an infinite
   * wait time, but may be more efficient.
   */
  PUBLIC void csoundWaitThreadLockNoTimeout(void *lock);

  /**
   * Notifies the indicated monitor object.
   */
  PUBLIC void csoundNotifyThreadLock(void *lock);

  /**
   * Destroys the indicated monitor object.
   */
  PUBLIC void csoundDestroyThreadLock(void *lock);

  /**
   * Waits for at least the specified number of milliseconds,
   * yielding the CPU to other threads.
   */
  PUBLIC void csoundSleep(size_t milliseconds);

  /**
   * Initialise a timer structure.
   */
  PUBLIC void csoundInitTimerStruct(RTCLOCK *);

  /**
   * Return the elapsed real time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double csoundGetRealTime(RTCLOCK *);

  /**
   * Return the elapsed CPU time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double csoundGetCPUTime(RTCLOCK *);

  /**
   * Return a 32-bit unsigned integer to be used as seed from current time.
   */
  PUBLIC uint32_t csoundGetRandomSeedFromTime(void);

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
  PUBLIC int csoundCreateGlobalVariable(CSOUND *,
                                        const char *name, size_t nbytes);

  /**
   * Get pointer to space allocated with the name "name".
   * Returns NULL if the specified name is not defined.
   */
  PUBLIC void *csoundQueryGlobalVariable(CSOUND *, const char *name);

  /**
   * This function is the same as csoundQueryGlobalVariable(), except the
   * variable is assumed to exist and no error checking is done.
   * Faster, but may crash or return an invalid pointer if 'name' is
   * not defined.
   */
  PUBLIC void *csoundQueryGlobalVariableNoCheck(CSOUND *, const char *name);

  /**
   * Free memory allocated for "name" and remove "name" from the database.
   * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
   * not defined.
   */
  PUBLIC int csoundDestroyGlobalVariable(CSOUND *, const char *name);

  /**
   * Return the size of MYFLT in bytes.
   */
  PUBLIC int csoundGetSizeOfMYFLT(void);

  /**
   * Return pointer to user data pointer for real time audio input.
   */
  PUBLIC void **csoundGetRtRecordUserData(CSOUND *);

  /**
   * Return pointer to user data pointer for real time audio output.
   */
  PUBLIC void **csoundGetRtPlayUserData(CSOUND *);

  /**
   * Register a function to be called once in every control period
   * by sensevents(). Any number of functions may be registered,
   * and will be called in the order of registration.
   * The callback function takes two arguments: the Csound instance
   * pointer, and the userData pointer as passed to this function.
   * Returns zero on success.
   */
  PUBLIC int csoundRegisterSenseEventCallback(CSOUND *,
                                              void (*func)(CSOUND *, void *),
                                              void *userData);

  /**
   * Run utility with the specified name and command line arguments.
   * Should be called after loading utility plugins with csoundPreCompile();
   * use csoundReset() to clean up after calling this function.
   * Returns zero if the utility was run successfully.
   */
  PUBLIC int csoundRunUtility(CSOUND *, const char *name,
                                        int argc, char **argv);

  /**
   * Returns a NULL terminated list of registered utility names.
   * The caller is responsible for freeing the returned array with free(),
   * however, the names should not be freed.
   * The return value may be NULL in case of an error.
   */
  PUBLIC char **csoundListUtilities(CSOUND *);

  /**
   * Get utility description.
   * Returns NULL if the utility was not found, or it has no description,
   * or an error occured.
   */
  PUBLIC const char *csoundGetUtilityDescription(CSOUND *,
                                                 const char *utilName);

  /**
   * Stores a pointer to the specified channel of the bus in *p,
   * creating the channel first if it does not exist yet.
   * 'type' must be the bitwise OR of exactly one of the following values,
   *   CSOUND_CONTROL_CHANNEL
   *     control data (one MYFLT value)
   *   CSOUND_AUDIO_CHANNEL
   *     audio data (csoundGetKsmps(csound) MYFLT values)
   *   CSOUND_STRING_CHANNEL
   *     string data (MYFLT values with enough space to store
   *     csoundGetStrVarMaxLen(csound) characters, including the
   *     NULL character at the end of the string)
   * and at least one of these:
   *   CSOUND_INPUT_CHANNEL
   *   CSOUND_OUTPUT_CHANNEL
   * If the channel already exists, it must match the data type (control,
   * audio, or string), however, the input/output bits are OR'd with the
   * new value. Note that audio and string channels can only be created
   * after calling csoundCompile(), because the storage size is not known
   * until then.
   * Return value is zero on success, or a negative error code,
   *   CSOUND_MEMORY  there is not enough memory for allocating the channel
   *   CSOUND_ERROR   the specified name or type is invalid
   * or, if a channel with the same name but incompatible type already exists,
   * the type of the existing channel. In the case of any non-zero return
   * value, *p is set to NULL.
   * Note: to find out the type of a channel without actually creating or
   * changing it, set 'type' to zero, so that the return value will be either
   * the type of the channel, or CSOUND_ERROR if it does not exist.
   */
  PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 MYFLT **p, const char *name, int type);

  /**
   * Returns a list of allocated channels, storing a pointer to an array
   * of channel names in *names, and a pointer to an array of channel types
   * in *types. (*types)[n] corresponds to (*names)[n], and has the same
   * format as the 'type' parameter of csoundGetChannelPtr().
   * The return value is the number of channels, which may be zero if there
   * are none, or CSOUND_MEMORY if there is not enough memory for allocating
   * the lists. In the case of no channels or an error, *names and *types are
   * set to NULL.
   * Note: the caller is responsible for freeing the lists returned in *names
   * and *types with free(), however, the actual channel names should not be
   * changed or freed. Also, the name pointers may become invalid after calling
   * csoundReset().
   */
  PUBLIC int csoundListChannels(CSOUND *, char ***names, int **types);

  /**
   * Sets special parameters for a control channel. The parameters are:
   *   type:  must be one of CSOUND_CONTROL_CHANNEL_INT,
   *          CSOUND_CONTROL_CHANNEL_LIN, or CSOUND_CONTROL_CHANNEL_EXP for
   *          integer, linear, or exponential channel data, respectively,
   *          or zero to delete any previously assigned parameter information
   *   dflt:  the control value that is assumed to be the default, should be
   *          greater than or equal to 'min', and less than or equal to 'max'
   *   min:   the minimum value expected; if the control type is exponential,
   *          it must be non-zero
   *   max:   the maximum value expected, should be greater than 'min';
   *          if the control type is exponential, it must be non-zero and
   *          match the sign of 'min'
   * Returns zero on success, or a non-zero error code on failure:
   *   CSOUND_ERROR:  the channel does not exist, is not a control channel,
   *                  or the specified parameters are invalid
   *   CSOUND_MEMORY: could not allocate memory
   */
  PUBLIC int csoundSetControlChannelParams(CSOUND *, const char *name,
                                           int type, MYFLT dflt,
                                           MYFLT min, MYFLT max);

  /**
   * Returns special parameters (assuming there are any) of a control channel,
   * previously set with csoundSetControlChannelParams().
   * If the channel exists, is a control channel, and has the special parameters
   * assigned, then the default, minimum, and maximum value is stored in *dflt,
   * *min, and *max, respectively, and a positive value that is one of
   * CSOUND_CONTROL_CHANNEL_INT, CSOUND_CONTROL_CHANNEL_LIN, and
   * CSOUND_CONTROL_CHANNEL_EXP is returned.
   * In any other case, *dflt, *min, and *max are not changed, and the return
   * value is zero if the channel exists, is a control channel, but has no
   * special parameters set; otherwise, a negative error code is returned.
   */
  PUBLIC int csoundGetControlChannelParams(CSOUND *, const char *name,
                                           MYFLT *dflt, MYFLT *min, MYFLT *max);

  /**
   * Simple linear congruential random number generator:
   *   (*seedVal) = (*seedVal) * 742938285 % 2147483647
   * the initial value of *seedVal must be in the range 1 to 2147483646.
   * Returns the next number from the pseudo-random sequence,
   * in the range 1 to 2147483646.
   */
  PUBLIC int csoundRand31(int *seedVal);

  /**
   * Initialise Mersenne Twister (MT19937) random number generator,
   * using 'keyLength' unsigned 32 bit values from 'initKey' as seed.
   * If the array is NULL, the length parameter is used for seeding.
   */
  PUBLIC void csoundSeedRandMT(CsoundRandMTState *p,
                               const uint32_t *initKey, uint32_t keyLength);

  /**
   * Returns next random number from MT19937 generator.
   * The PRNG must be initialised first by calling csoundSeedRandMT().
   */
  PUBLIC uint32_t csoundRandMT(CsoundRandMTState *p);

#endif  /* !CSOUND_CSDL_H */

  /* typedefs, macros, and interface functions for configuration variables */
#include "cfgvar.h"
  /* message attribute definitions for csoundMessageS() and csoundMessageV() */
#include "msg_attr.h"

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_H */

