#ifndef CSOUND_H
#define CSOUND_H
/**
* S I L E N C E
*
* An auto-extensible system for making music on computers by means of software alone.
* Copyright (c) 2001 by Michael Gogins. All rights reserved.
*
* L I C E N S E
*
* This software is in the public domain.
*
* P U R P O S E
*
* Declares the public C application programming interface to Csound.
*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*
   * Platform-dependent definitions and declarations.
   */
#ifdef WIN32
#define PUBLIC __declspec(dllexport)
#define LIBRARY_CALL WINAPI
#else
#define PUBLIC
#define LIBRARY_CALL
#endif

#include "sysdep.h"
#include "cwindow.h"
#include "opcode.h"
#include <stdarg.h>

  /**
   * ERROR DEFINITIONS
   */

  typedef enum {
      CSOUND_SUCCESS = 0,
      CSOUND_ERROR = -1,
      CSOUND_INITIALIZATION = -2,
      CSOUND_PERFORMANCE = -3,
      CSOUND_MEMORY = -4,
  } CSOUND_STATUS;

  /*
   * INSTANTIATION
   */

  /**
   * Creates an instance of Csound.
   * Returns an opaque pointer that must be passed to most Csound API functions.
   * The hostData parameter can be null, or it can be a pointer to any sort of
   * data; this pointer can be accessed from the Csound instance that is passed
   * to callback routines.
   */
  void *csoundCreate(void *hostData);

  /**
   * Returns a pointer to the requested interface, if available, in the interface
   * argument, and its version number, in the version argument.
   * Returns 0 for success and 1 for failure.
   */
  int csoundQueryInterface(const char *name, void **iface, int *version);

  /**
   * Destroys an instance of Csound.
   */
  void csoundDestroy(void *csound);

  /**
   * Returns the version number times 100 (4.20 = 420).
   */
  int csoundGetVersion(void);

  /**
   * Returns host data.
   */
  void *csoundGetHostData(void *csound);

  /**
   * Sets host data.
   */
  void csoundSetHostData(void *csound, void *hostData);

  /*
   * PERFORMANCE
   */

  /**
   * Compiles and renders a Csound performance,
   * as directed by the supplied command-line arguments,
   * in one pass. Returns 1 for success, 0 for failure.
   */
  int csoundPerform(void *csound, int argc, char **argv);

  /**
   * Compiles Csound input files (such as an orchestra and score)
   * as directed by the supplied command-line arguments,
   * but does not perform them. Returns a non-zero error code on failure.
   * In this (host-driven) mode, the sequence of calls should be as follows:
   *
   *       csoundCompile(argc, argv, thisObj);
   *       while(!csoundPerformBuffer());
   *       csoundCleanup();
   *       csoundReset();
   */
  int csoundCompile(void *csound, int argc, char **argv);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output.
   * Note that csoundCompile must be called first.
   * Returns false during performance, and true when performance is finished.
   * If called until it returns true, will perform an entire score.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  int csoundPerformKsmps(void *csound);

  /**
   * Performs Csound, sensing real-time and score events
   * and processing one buffer's worth (-b frames) of interleaved audio.
   * Returns a pointer to the new output audio in 'outputAudio'
   * Note that csoundCompile must be called first, then call
   * csoundGetOutputBuffer() and csoundGetInputBuffer() to get the pointer
   * to csound's i/o buffers.
   * Returns false during performance, and true when performance is finished.
   */
  int csoundPerformBuffer(void *csound);

  /**
   * Prints information about the end of a performance.
   * Must be called after the final call to csoundPerformKsmps.
   */
  void csoundCleanup(void *csound);

  /**
   * Resets all internal memory and state in preparation for a new performance.
   * Enables external software to run successive Csound performances
   * without reloading Csound.
   */
  void csoundReset(void *csound);

  /*
   * ATTRIBUTES
   */

  /**
   * Returns the number of audio sample frames per second.
   */
  MYFLT csoundGetSr(void *csound);

  /**
   * Returns the number of control samples per second.
   */
  MYFLT csoundGetKr(void *csound);

  /**
   * Returns the number of audio sample frames per control sample.
   */
  int csoundGetKsmps(void *csound);

  /**
   * Returns the number of audio output channels.
   */
  int csoundGetNchnls(void *csound);

  /**
   * Returns the sample format.
   */
  int csoundGetSampleFormat(void *csound);

  /**
   * Returns the size in bytes of a single sample.
   */
  int csoundGetSampleSize(void *csound);

  /**
   * Returns the number of samples in Csound's input buffer.
   */
  long csoundGetInputBufferSize(void *csound);

  /**
   * Returns the number of samples in Csound's output buffer.
   */
  long csoundGetOutputBufferSize(void *csound);

  /**
   * Returns the address of the Csound audio input buffer.
   * Enables external software to write audio into Csound before calling
   * csoundPerformBuffer
   */
  void *csoundGetInputBuffer(void *csound);

  /**
   * Returns the address of the Csound audio output buffer.
   * Enables external software to read audio from Csound after calling
   * csoundPerformBuffer.
   */
  void *csoundGetOutputBuffer(void *csound);

  /**
   * Returns the address of the Csound audio input working buffer (spin).
   * Enables external software to write audio into Csound before calling
   * csoundPerformKsmps.
   */
  MYFLT *csoundGetSpin(void *csound);

  /**
   * Returns the address of the Csound audio output working buffer (spout).
   * Enables external software to read audio from Csound after calling
   * csoundPerformKsmps.
   */
  MYFLT *csoundGetSpout(void *csound);

  /**
   * Returns the current score time.
   */
  MYFLT csoundGetScoreTime(void *csound);

  /**
   * Returns the % of score completed.
   */
  MYFLT csoundGetProgress(void *csound);

  /**
   * Returns the scoreTime vs. calculatedTime ratio.  For real-time performance
   * this value should be always == 1.
   */
  MYFLT csoundGetProfile(void *csound);

  /**
   * Returns the sampsTime vs. calculatedTime ratio.
   */
  MYFLT csoundGetCpuUsage(void *csound);

  /*
   * SCORE HANDLING
   */

  /**
   * Returns whether Csound's score is synchronized with external software.
   */
  int csoundIsScorePending(void *csound);

  /**
   * Sets whether Csound's score is synchronized with external software.
   */
  void csoundSetScorePending(void *csound, int pending);

  /**
   * Csound events prior to the offset are consumed and discarded prior to
   * beginning performance.
   * Can be used by external software to begin performance midway through a
   * Csound score.
   */
  MYFLT csoundGetScoreOffsetSeconds(void *csound);

  /**
   * Csound events prior to the offset are consumed and discarded prior to
   * beginning performance.
   * Can be used by external software to begin performance midway through a
   * Csound score.
   */
  void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset);

  /**
   * Rewinds a compiled Csound score to its beginning.
   */
  void csoundRewindScore(void *csound);

  /*
   * MESSAGES & TEXT
   */

  /**
   * Displays an informational message.
   */
  void csoundMessage(void *csound, const char *format, ...);
  void csoundMessageV(void *csound, const char *format, va_list args);
  void csoundMessageS(void *csound, const char *format, va_list args);

  /**
   * Throws an informational message as a C++ exception.
   */
  void csoundThrowMessage(void *csound, const char *format, ...);
  void csoundThrowMessageV(void *csound, const char *format, va_list args);

  /**
   * Sets a function to be called by Csound to print an informational message.
   */
  void csoundSetMessageCallback(void *csound,
                                void (*csoundMessageCallback)(void *csound,
                                                              const char *format,
                                                              va_list valist));

  /**
   * Sets a funtion for Csound to stop execution with an error message or
   * exception.
   */
  void
  csoundSetThrowMessageCallback(void *csound,
                                void (*throwMessageCallback)(void *csound,
                                                             const char *format,
                                                             va_list valist));

  /**
   * Returns the Csound message level (from 0 to 7).
   */
  int csoundGetMessageLevel(void *csound);

  /**
   * Sets the Csound message level (from 0 to 7).
   */
  void csoundSetMessageLevel(void *csound, int messageLevel);

  /**
   * Input a NULL-terminated string (as if from a console) usually used
   * for lineevents
   */
  void csoundInputMessage(void *csound, const char *message);

  /**
   * Set the ascii code of most recent key pressed.
   * This value is used by the 'keypress' opcode.
   */
  void csoundKeyPress(void *csound, char c);

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
   * Called by external software to set a function for Csound to fetch input
   * control values.
   * The 'invalue' opcodes will directly call this function.
   */
  void csoundSetInputValueCallback(void *csound,
                                   void (*inputValueCalback)(void *csound,
                                                             char *channelName,
                                                             MYFLT *value));

  /**
   * Called by external software to set a function for Csound to send output
   * control values.
   * The 'outvalue' opcodes will directly call this function.
   */
  void csoundSetOutputValueCallback(void *csound,
                                    void (*outputValueCalback)(void *csound,
                                                               char *channelName,
                                                               MYFLT value));

  /**
   * Send a new score event. 'type' is the score event type ('i', 'f', or 'e')
   * 'numFields' is the size of the pFields array.  'pFields' is an array
   *  of floats with all the pfields for this event, starting with the p1 value
   *  specified in pFields[0].
   */
  void csoundScoreEvent(void *csound, char type, MYFLT *pFields, long numFields);

  /*
   * MIDI
   */

  /**
   * Called by external software to set a function for Csound to call to
   * open MIDI input.
   */
  void csoundSetExternalMidiOpenCallback(void *csound,
                                         void (*midiOpenCallback)(void *csound));

  /**
   * Called by external software to set a function for Csound to call to
   * read MIDI messages.
   */
  void
  csoundSetExternalMidiReadCallback(void *csound,
                                    int(*readMidiCallback)(void *csound,
                                                           unsigned char *midiData,
                                                           int size));

  /**
   * Called by external software to set a function for Csound to call to write
   * a 4-byte MIDI message.
   */
  void
  csoundSetExternalMidiWriteCallback(void *csound,
                                     int (*writeMidiCallback)(
                                                        void *csound,
                                                        unsigned char *midiData));

  /**
   * Called by external software to set a function for Csound to call to
   * close MIDI input.
   */
  void csoundSetExternalMidiCloseCallback(void *csound,
                                          void (*closeMidiCallback)(void *csound));

  /**
   * Returns true if external MIDI is enabled, and false otherwise.
   */
  int csoundIsExternalMidiEnabled(void *csound);

  /**
   * Sets whether external MIDI is enabled.
   */
  void csoundSetExternalMidiEnabled(void *csound, int enabled);

  /*
   * FUNCTION TABLE DISPLAY
   */

  /**
   * Tells Csound supports external graphic table display.
   */
  void csoundSetIsGraphable(void *csound, int isGraphable);

  /**
   * Called by external software to set Csound's MakeGraph function.
   */
  void csoundSetMakeGraphCallback(void *csound,
                                  void (*makeGraphCallback)(void *csound,
                                                            WINDAT *windat,
                                                            char *name));

  /**
   * Called by external software to set Csound's DrawGraph function.
   */
  void csoundSetDrawGraphCallback(void *csound,
                                  void (*drawGraphCallback)(void *csound,
                                                            WINDAT *windat));

  /**
   * Called by external software to set Csound's KillGraph function.
   */
  void csoundSetKillGraphCallback(void *csound,
                                  void (*killGraphCallback)(void *csound,
                                                            WINDAT *windat));

  /**
   * Called by external software to set Csound's ExitGraph function.
   */
  void csoundSetExitGraphCallback(void *csound,
                                  int (*exitGraphCallback)(void *csound));

  /*
   * OPCODES
   */

  /*
   * Gets a list of all opcodes.
   * Make sure to call csoundDisposeOpcodeList() when done with the list.
   */
  opcodelist *csoundNewOpcodeList(void);

  /*
   * Releases an opcode list
   */
  void csoundDisposeOpcodeList(opcodelist *opcodelist_);

  /**
   * Appends an opcode implemented by external software
   * to Csound's internal opcode list.
   * The opcode list is extended by one slot,
   * and the parameters are copied into the new slot.
   */
  int csoundAppendOpcode(char *opname, int dsblksiz, int thread,
                         char *outypes, char *intypes, void (*iopadr)(void *),
                         void (*kopadr)(void *), void (*aopadr)(void *),
                         void (*dopadr)(void *));

  /*
   * Signature for external registration function,
   * such as for plugin opcodes or scripting languages.
   * The external has complete access to the Csound API,
   * so a plugin opcode can just call csound->AppendOpcode()
   * for each of its opcodes.
   */
  typedef PUBLIC int (*CsoundRegisterExternalType)(void *csound);

  /*
   * Registers all opcodes in the library.
   */
  int csoundLoadExternal(void *csound, const char *libraryPath);

  /*
   * Registers all opcodes in all libraries in the opcodes directory.
   */
  int csoundLoadExternals(void *csound);

  /*
   * MISCELLANEOUS FUNCTIONS
   */

  /*
   * Platform-independent function
   * to load a shared library.
   */
  void *csoundOpenLibrary(const char *libraryPath);

  /*
   * Platform-independent function
   * to unload a shared library.
   */
  void *csoundCloseLibrary(void *library);

  /*
   * Platform-independent function
   * to get a symbol address
   * in a shared library.
   */
  void *csoundGetLibrarySymbol(void *library, const char *symbolName);

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
  void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound));

  /**
   * Sets an evironment path for a getenv() call in Csound.
   * you can also use this method as a way to have different
   * csound instances have different default directories,
   * change the default dirs during performance, etc..
   *
   * Currently, Csound uses these 'envi' names only:
   *       "SSDIR", "SFDIR", "SADIR", "SFOUTYP", "INCDIR",
   *               "CSSTRNGS", "MIDIOUTDEV", and "HOME"
   */
  void csoundSetEnv(void *csound,
                    const char *environmentVariableName,
                    const char *path);

  /**
   *       REAL-TIME AUDIO PLAY AND RECORD
   */

  /**
   * Sets a function to be called by Csound for opening real-time audio playback.
   */
  void csoundSetPlayopenCallback(void *csound,
                                 void (*playopen__)(int nchanls,
                                                    int dsize,
                                                    float sr,
                                                    int scale));

  /**
   * Sets a function to be called by Csound for performing real-time audio
   * playback.
   */
  void csoundSetRtplayCallback(void *csound,
                               void (*rtplay__)(char *outBuf, int nbytes));

  /**
   * Sets a function to be called by Csound for opening real-time audio recording.
   */
  void csoundSetRecopenCallback(void *csound,
                                void (*recopen_)(int nchanls,
                                                 int dsize,
                                                 float sr,
                                                 int scale));

  /**
   * Sets a function to be called by Csound for performing real-time audio
   * recording.
   */
  void csoundSetRtrecordCallback(void *csound,
                                 int (*rtrecord__)(char *inBuf, int nbytes));

  /**
   * Sets a function to be called by Csound for closing real-time audio
   * playback and recording.
   */
  void csoundSetRtcloseCallback(void *csound, void (*rtclose__)(void));


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CSOUND_H */






