/*
  csound.h: Csound Host API

  Copyright (C) Csound Developers 2024

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

#ifndef CSOUND_H
#define CSOUND_H

#if (defined(WIN32) || defined(_WIN32)) && !defined(SWIG)
#  if defined(__BUILDING_LIBCSOUND)
#    define PUBLIC          __declspec(dllexport)
#    define PUBLIC_DATA     __declspec(dllexport)
#  else
#    define PUBLIC          __declspec(dllexport)
#    define PUBLIC_DATA     __declspec(dllimport)
#  endif
#elif defined(__wasi__)
#  define PUBLIC            __attribute__((used))
#  if !defined(PUBLIC_DATA)
#  define PUBLIC_DATA
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 4) /* && !defined(__MACH__) */
#  define PUBLIC            __attribute__ ( (visibility("default")) )
#  define PUBLIC_DATA       __attribute__ ( (visibility("default")) )
#else
#  define PUBLIC
#  define PUBLIC_DATA
#endif

#if defined(MSVC)
#  include <intrin.h> /* for _InterlockedExchange */
#endif

#if defined(__MACH__)
// on OSX 10.6 i386 does not have all builtins
#if defined(MAC_OS_X_VERSION_10_6)
#ifdef HAVE_ATOMIC_BUILTIN
#ifndef __x86_64__
#undef HAVE_ATOMIC_BUILTIN
#endif
#endif
#endif
#endif

// FOR ANDROID
#ifdef SWIG
#define CS_PRINTF2
#define CS_PRINTF3
#include "float-version.h"
#ifndef __MYFLT_DEF
#define __MYFLT_DEF
#ifndef USE_DOUBLE
#define MYFLT float
#else
#define MYFLT double
#endif
#endif
#else
#  include <stdint.h>
#  include "sysdep.h"
#  include "text.h"
#  include <stdarg.h>
#  include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
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
    } CSOUND_STATUS;

   /**
    Verbose levels
   */
   typedef enum {
     DEBUG_NONE = 0,
     /* runtime messages. */
     DEBUG_RUNTIME = 0x01,
     /* compiler messages. */
     DEBUG_COMPILER = 0x02,
     /* semantic analysis messages. */
     DEBUG_SEMANTICS = 0x04,
     /* expressions messages. */
     DEBUG_EXPRESSIONS = 0x08,
     /* parser messages. */
     DEBUG_PARSER = 0x10,
     /* print AST. */
     DEBUG_TREE = 0x20,
     /* print compiled instrs. */
     DEBUG_INSTR = 0X40,
     /* opcode messages. */
     DEBUG_OPCODES = 0x80,
     /* parcs messages. */
     DEBUG_PARCS = 0x100,
     /* all messages. */
     DEBUG_FULL = 0x7FFFFFFF
   } DEBUG_STATUS;
  

  /* Compilation or performance aborted, but not as a result of an error
     (e.g. --help, or running an utility with -U). */
#define CSOUND_EXITJMP_SUCCESS  (256)

  /**
   * Flags for csoundInitialize().
   */

#define CSOUNDINIT_NO_SIGNAL_HANDLER  1
#define CSOUNDINIT_NO_ATEXIT          2

  /**
   * Types for keyboard callbacks set in csoundRegisterKeyboardCallback()
   */

#define CSOUND_CALLBACK_KBD_EVENT   (0x00000001U)
#define CSOUND_CALLBACK_KBD_TEXT    (0x00000002U)

  /**
   * Forward declarations.
   */
  typedef struct CSOUND_  CSOUND;
  typedef struct stringdat STRINGDAT;
  typedef struct arraydat ARRAYDAT;
  typedef struct pvsdat PVSDAT;


  typedef struct CSOUND_PARAMS {
    /* debug flag */
    int32_t     odebug;
    /* sound input read flag */
    int32_t     sfread;
    /* sound output write flag (-s) */
    int32_t     sfwrite;
    /* soundfile type code */
    int32_t     filetyp;
    /* input buffer size in samples */
    int32_t     inbufsamps;
    /* output buffer size in samples */
    int32_t     outbufsamps;
    /* input soundfile format */
    int32_t     informat;
    /* output soundfile format */
    int32_t outformat;
    /* sample size */
    int32_t     sndfileSampleSize;
    /* displays flag */
    int32_t     displays;
    /* graphs flag */
    int32_t     graphsoff;
    /* postscript graphs flag */
    int32_t     postscript;
    /* message level (-m) */
    int32_t     msglevel;
    /* beat mode */
    int32_t     Beatmode;
    /* hardware buffer size (samples) */
    int32_t     oMaxLag;
    /* linevents flag (-L)*/
    int32_t     Linein;
    /* realtime events flag (scoreless, -L, -F, -M) */
    int32_t     RTevents;
    /* midi input flag (-M) */
    int32_t     Midiin;
    /* midi file input flag (-F) */
    int32_t     FMidiin;
    /* remote events flag */
    int32_t     RMidiin;
    /* ringbell flag */
    int32_t     ringbell;
    /* terminate on MIDI file input flag (-T) */
    int32_t     termifend;
    /* rewrite header flag */
    int32_t     rewrt_hdr;
    /* heartbeat flag */
    int32_t     heartbeat;
    /* GEN01 defer allocation flag */
    int32_t     gen01defer;
    /* tempo value (-t)  */
    double      cmdTempo;
    /* sampling rate override (-r) */
    MYFLT       sr_override;
    /* control rate override (-k) */
    MYFLT       kr_override;
    /* nchnls override */
    int32_t     nchnls_override;
    /* nchnls_i override */
    int32_t     nchnls_i_override;
    /* input file name (-i) */
    char       *infilename;
    /* output file name (-o) */
    char       *outfilename;
    /* line events source (-L) */
    char       *Linename;
    /* MIDI input device name (-M) */
    char       *Midiname;
    /* MIDI input file name (-F) */
    char       *FMidiname;
    /* MIDI output device name (-Q) */
    char       *Midioutname;
    /* MIDI output file name */
    char       *FMidioutname;
    /* MIDI key pfield mapping */
    int32_t     midiKey;
    /* MIDI key-cps pfield mapping */
    int32_t     midiKeyCps;
    /* MIDI key-oct pfield mapping */
    int32_t     midiKeyOct;
    /* MIDI key-pch pfield mapping */
    int32_t     midiKeyPch;
    /* MIDI vel pfield mapping */
    int32_t     midiVelocity;
    /* MIDI vel-amp pfield mapping */
    int32_t     midiVelocityAmp;
    /* default paths flag */
    int32_t     noDefaultPaths;
    /* multicore number of threads (-j) */
    int32_t     numThreads;
    /* syntax check only flag */
    int32_t     syntaxCheckOnly;
    /* run unit tests flag */
    int32_t     runUnitTests;
    /* csd line nums option */
    int32_t     useCsdLineCounts;
    /* sample accurate flag */
    int32_t     sampleAccurate;
    /* realtime priority flag */
    int32_t     realtime;
    /* 0dbfs override */
    MYFLT       e0dbfs_override;
    /* daemon mode flag */
    int32_t     daemon;
    /* OGG encoding quality */
    double      quality;
    /* ksmps override */
    int32_t     ksmps_override;
    /* FFT library option */
    int32_t     fft_lib;
    /* UDP echo commands flag */
    int32_t     echo;
    /* audio output limiter option */
    MYFLT       limiter;
    /* default sampling rate */
    MYFLT       sr_default;
    /* default control rate */
    MYFLT       kr_default;
    /* MP3 encoding mode  */
    int32_t     mp3_mode;
    /* instr redefinition flag */
    int32_t     redef;
    /* error on deprecated opcodes */
    int32_t     error_deprecated;
  } OPARMS;

  /**
   * Device information
   */
  typedef struct {
    char device_name[128];
    char device_id[128];
    char rt_module[128];
    int32_t max_nchnls;
    int32_t isOutput;
  } CS_AUDIODEVICE;

  typedef struct {
    char device_name[128];
    char interface_name[128];
    char device_id[128];
    char midi_module[128];
    int32_t isOutput;
  } CS_MIDIDEVICE;

  /**
     PVSDAT window types
  */
  enum PVS_WINTYPE {
    PVS_WIN_HAMMING = 0,
    PVS_WIN_HANN,
    PVS_WIN_KAISER,
    PVS_WIN_CUSTOM,
    PVS_WIN_BLACKMAN,
    PVS_WIN_BLACKMAN_EXACT,
    PVS_WIN_NUTTALLC3,
    PVS_WIN_BHARRIS_3,
    PVS_WIN_BHARRIS_MIN,
    PVS_WIN_RECT
  };


  /*
   *  PVSDAT formats
   */
  enum PVS_ANALFORMAT {
    PVS_AMP_FREQ = 0, /* phase vocoder */
    PVS_AMP_PHASE,    /* polar DFT */
    PVS_COMPLEX,      /* rectangular DFT */
    PVS_TRACKS        /* amp, freq, phase, ID tracks */
  };

  /**
   * Constants used by the bus interface (csoundGetChannelPtr() etc.).
   */
  typedef enum {
    CSOUND_CONTROL_CHANNEL =     1,
    CSOUND_AUDIO_CHANNEL  =      2,
    CSOUND_STRING_CHANNEL =      3,
    CSOUND_PVS_CHANNEL =      4,
    CSOUND_VAR_CHANNEL =      5,
    CSOUND_ARRAY_CHANNEL =      6,
    CSOUND_CHANNEL_TYPE_MASK =    15,
    CSOUND_INPUT_CHANNEL =       16,
    CSOUND_OUTPUT_CHANNEL =       32
  } controlChannelType;

  typedef enum {
    CSOUND_CONTROL_CHANNEL_NO_HINTS  = 0,
    CSOUND_CONTROL_CHANNEL_INT  = 1,
    CSOUND_CONTROL_CHANNEL_LIN  = 2,
    CSOUND_CONTROL_CHANNEL_EXP  = 3
  } controlChannelBehavior;

  /**
   * This structure holds the parameter hints for control channels
   *
   */
  typedef struct controlChannelHints_s {
    controlChannelBehavior    behav;
    MYFLT   dflt;
    MYFLT   min;
    MYFLT   max;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    /** This member must be set explicitly to NULL if not used */
    char *attributes;
  } controlChannelHints_t;

  typedef struct controlChannelInfo_s {
    char  *name;
    int32_t     type;
    controlChannelHints_t    hints;
  } controlChannelInfo_t;

  typedef void (*channelCallback_t)(CSOUND *csound,
                                    const char *channelName,
                                    void *channelValuePtr,
                                    const void *channelType);
  /**
     Event Types
  */
  enum {
    CS_INSTR_EVENT = 0,
    CS_TABLE_EVENT,
    CS_END_EVENT,
    CS_ADV_EVENT
  };

#ifndef CSOUND_CSDL_H
  /** @defgroup INSTANTIATION Instantiation
   *
   *  @{ */

  /**
   * Initialise Csound library with specific flags. This function is called
   * internally by csoundCreate(), so there is generally no need to use it
   * explicitly unless you need to avoid default initilization that sets
   * signal handlers and atexit() callbacks.
   * Return value is zero on success, positive if initialisation was
   * done already, and negative on error.
   */
  PUBLIC int32_t csoundInitialize(int32_t flags);

  /**
   * Creates an instance of Csound.  Returns an opaque pointer that
   * must be passed to most Csound API functions.  The hostData
   * parameter can be NULL, or it can be a pointer to any sort of
   * data; this pointer can be accessed from the Csound instance
   * that is passed to callback routines.
   * If not NULL the opcodedir parameter sets an override for the
   * plugin module/opcode directory search
   */
  PUBLIC CSOUND *csoundCreate(void *hostData, const char *opcodedir);

  /**
   * Destroys an instance of Csound.
   */
  PUBLIC void csoundDestroy(CSOUND *);


  /** @}*/
  /** @defgroup ATTRIBUTES Attributes
   *
   *  @{ */
  /**
   * Returns the version number times 1000 (5.00.0 = 5000).
   */
  PUBLIC int32_t csoundGetVersion(void);

  /**
   * Returns the number of audio sample frames per second.
   */
  PUBLIC MYFLT csoundGetSr(CSOUND *) ;

  /**
   * Returns the number of control samples per second.
   */
  PUBLIC MYFLT csoundGetKr(CSOUND *);

  /**
   * Returns the audio vector size in frames (= sr/kr)
   */
  PUBLIC uint32_t csoundGetKsmps(CSOUND *);

  /**
   * Returns the number of audio channels in the Csound instance.
   * If isInput = 0, the value of nchnls is returned,
   * otherwise nchnls_i. If this variable is
   * not set, the value is always taken from nchnls.
   */
  PUBLIC uint32_t csoundGetChannels(CSOUND *, int32_t isInput);

  /**
   * Returns the 0dBFS level of the spin/spout buffers.
   */
  PUBLIC MYFLT csoundGet0dBFS(CSOUND *);

  /**
   * Returns the A4 frequency reference
   */
  PUBLIC MYFLT csoundGetA4(CSOUND *);

  /**
   * Return the current performance time in sample frames
   */
  PUBLIC int64_t csoundGetCurrentTimeSamples(CSOUND *csound);

  /**
   * Return the size of MYFLT in bytes.
   */
  PUBLIC int32_t csoundGetSizeOfMYFLT(void);

  /**
   * Returns host data.
   */
  PUBLIC void *csoundGetHostData(CSOUND *);

  /**
   * Sets host data.
   */
  PUBLIC void csoundSetHostData(CSOUND *, void *hostData);

  /**
   * Returns the total error count of the current performance.
   */
  PUBLIC int32_t csoundErrCnt(CSOUND *csound);

  /**
   * Get pointer to the value of environment variable 'name', searching
   * in this order: local environment of 'csound' (if not NULL), variables
   * set with csoundSetGlobalEnv(), and system environment variables.
   * If 'csound' is not NULL, should be called after csoundCompile().
   * Return value is NULL if the variable is not set.
   */
  PUBLIC const char *csoundGetEnv(CSOUND *csound, const char *name);

  /**
   * Set the global value of environment variable 'name' to 'value',
   * or delete variable if 'value' is NULL.
   * It is not safe to call this function while any Csound instances
   * are active.
   * Returns zero on success.
   */
  PUBLIC int32_t csoundSetGlobalEnv(const char *name, const char *value);

  /**
   * Set csound options (flag). Returns CSOUND_SUCCESS on success.
   * This needs to be called after csoundCreate() and before any code is
   * compiled. Multiple options are allowed in one string.
   * Returns zero on success.
   */
  PUBLIC int32_t csoundSetOption(CSOUND *csound, const char *option);

  /**
   *  Get the current set of parameters from a CSOUND instance in
   *  a struct CSOUND_PARAMS structure.
   */
  PUBLIC const OPARMS *csoundGetParams(CSOUND *csound);

  /**
   * Returns whether Csound is set to print debug messages sent through the
   * DebugMsg() internal API function. Anything different to 0 means true.
   */
  PUBLIC int32_t csoundGetDebug(CSOUND *);

  /**
   * Sets whether Csound prints debug messages from the DebugMsg() internal
   * API function. Anything different to 0 means true.
   */
  PUBLIC void csoundSetDebug(CSOUND *, int32_t debug);

  /**
   * If val > 0, sets the internal variable holding the system HW sr.
   * Returns the stored value containing the system HW sr.
   */
  PUBLIC MYFLT csoundSystemSr(CSOUND *csound, MYFLT val);


  /**
   * retrieves a module name and type ("audio" or "midi") given a
   * number Modules are added to list as csound loads them returns
   * CSOUND_SUCCESS on success and CSOUND_ERROR if module number
   * was not found
   *
   * \code
   *  char *name, *type;
   *  int32_t n = 0;
   *  while(!csoundGetModule(csound, n++, &name, &type))
   *       printf("Module %d:  %s (%s) \n", n, name, type);
   * \endcode
   */
  PUBLIC int32_t csoundGetModule(CSOUND *csound, int32_t number,
                             char **name, char **type);

  /**
   * This function can be called to obtain a list of available
   * input or output audio devices available (depending on the backend
   * module used *).
   * If list is NULL, the function returns the number of devices
   * (isOutput=1 for out devices, 0 for in devices).
   * If list is non-NULL, then it should contain enough memory for
   * one CS_AUDIODEVICE structure per device.
   * Hosts will typically call this function twice: first to obtain
   * a number of devices, then, after allocating space for each
   * device information structure, pass an array of CS_AUDIODEVICE
   * structs to be filled:
   *
   * \code
   *   int32_t i,n = csoundGetAudioDevList(csound,NULL,1);
   *   CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *)
   *       malloc(n*sizeof(CS_AUDIODEVICE));
   *   csoundGetAudioDevList(csound,devs,1);
   *   for(i=0; i < n; i++)
   *       csound->Message(csound, " %d: %s (%s)\n",
   *             i, devs[i].device_id, devs[i].device_name);
   *   free(devs);
   * \endcode
   */
  PUBLIC int32_t csoundGetAudioDevList(CSOUND *csound,
                                   CS_AUDIODEVICE *list, int32_t isOutput);

  /**
   * This function can be called to obtain a list of available
   * input or output midi devices. If list is NULL, the function
   * will only return the number of devices (isOutput=1 for out
   * devices, 0 for in devices).
   * If list is non-NULL, then it should contain enough memory for
   * one CS_MIDIDEVICE structure per device.
   * Hosts will typically call this function twice: first to obtain
   * a number of devices, then, after allocating space for each
   * device information structure, pass an array of CS_MIDIDEVICE
   * structs to be filled. (see also csoundGetAudioDevList())
   */
  PUBLIC int32_t csoundGetMIDIDevList(CSOUND *csound,
                                  CS_MIDIDEVICE *list, int32_t isOutput);

  /**
   * Returns the Csound message level (from 0 to 231).
   */
  PUBLIC int32_t csoundGetMessageLevel(CSOUND *);

  /**
   * Sets the Csound message level (from 0 to 231).
   */
  PUBLIC void csoundSetMessageLevel(CSOUND *, int32_t messageLevel);

  /** @}*/
  /** @defgroup PERFORMANCE Compilation and performance
   *
   *  @{ */

  /**
   * Compiles Csound input files (such as an orchestra and score, or CSD)
   * as directed by the supplied command-line arguments,
   * but does not perform them. Returns a non-zero error code on failure.
   * In this mode, the sequence of calls should be as follows:
   * /code
   *       csoundCompile(csound, argc, argv);
   *       csoundStart(csound);
   *       while (!csoundPerformKsmps(csound));
   *       csoundReset(csound);
   * /endcode
   */
  PUBLIC int32_t csoundCompile(CSOUND *, int32_t argc, const char **argv);

  /**
   * Parse, and compile the given orchestra from an ASCII string,
   * also evaluating any global space code (i-time only)
   * in synchronous or asynchronous (async = 1) mode.
   * /code
   *       char *orc = "instr 1 \n a1 rand 0dbfs/4 \n out a1 \n";
   *       csoundCompileOrc(csound, orc, 0);
   * /endcode
   */
  PUBLIC int32_t csoundCompileOrc(CSOUND *csound, const char *str, int32_t async);

  /**
   *   Parse and compile an orchestra given on a string, synchronously,
   *   evaluating any global space code (i-time only).
   *   On SUCCESS it returns a value passed to the
   *   'return' opcode in global space
   * /code
   *       char *code =
   *           "i1 = 2 + 2 \n"
   *           "return i1 \n";
   *       MYFLT retval = csoundEvalCode(csound, code);
   * /endcode
   *   If the code fails to evaluate, the return value is always 0.
   *
   */
  PUBLIC MYFLT csoundEvalCode(CSOUND *csound, const char *str);

  /**
   * Compiles a Csound input file (CSD, .csd file) or a tx string
   * containing the CSD code, in synchronous or asynchronous (async = 1) mode.
   * Returns a non-zero error code on failure.
   *
   * If csoundStart is called before csoundCompileCSD, the <CsOptions>
   * element is ignored (but csoundSetOption can be called any number of
   * times), the <CsScore> element is dispatched as score events (e.g.
   * as it is done by csoundEventString())
   * \code
   *
   * csoundSetOption("-an_option");
   * csoundSetOption("-another_option");
   * csoundStart(csound);
   * csoundCompileCSD(csound, csd_filename, 0);
   * while (1) {
   *    csoundPerformKsmps(csound);
   *    // Something to break out of the loop
   *    // when finished here...
   * }
   *
   * \endcode
   *
   * NB: this function can be called repeatedly during performance to
   * replace or add new instruments and events.
   *
   * But if csoundCompileCsd is called before csoundStart, the <CsOptions>
   * element is used, the <CsScore> section is pre-processed and dispatched
   * normally, and performance terminates when the score terminates.
   *
   * \code
   *
   * csoundCompileCSD(csound, csd_filename, 0);
   * csoundStart(csound);
   * while (1) {
   *    int32_t finished = csoundPerformKsmps(csound);
   *    if (finished) break;
   * }
   *
   * \endcode
   *
   * if mode = 1, csd contains a full CSD code (rather than a filename).
   * This is convenient when it is desirable to package the csd as part of
   * an application or a multi-language piece.
   *
   */
  PUBLIC int32_t csoundCompileCSD(CSOUND *csound, const char *csd, int32_t mode,
                                  int32_t async);

  /**
   * Prepares Csound for performance. Normally called after compiling
   * a csd file or an orc file, in which case score preprocessing is
   * performed and performance terminates when the score terminates.
   *
   * However, if called before compiling a csd file or an orc file,
   * score preprocessing is not performed and "i" statements are dispatched
   * as real-time events, the <CsOptions> tag is ignored, and performance
   * continues indefinitely or until ended using the API.
   */
  PUBLIC int32_t csoundStart(CSOUND *csound);

  /**
   * Senses input events, and performs one block of
   * audio output containing ksmps frames. csoundStart() must be called first.
   * Returns false during performance, and true when performance is finished.
   * If called until it returns true, will perform an entire score.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int32_t  csoundPerformKsmps(CSOUND *);

  /**
   * Run utility with the specified name and command line arguments.
   * Should be called after loading utility plugins.
   * Use csoundReset() to clean up after calling this function.
   * Returns zero if the utility was run successfully.
   */
  PUBLIC int32_t csoundRunUtility(CSOUND *, const char *name,
                              int32_t argc, char **argv);

  /**
   * Resets all internal memory and state in preparation for a new performance.
   * Enables external software to run successive Csound performances
   * without reloading Csound.
   */
  PUBLIC void csoundReset(CSOUND *);
  /** @}*/

  /** @defgroup AUDIOIO Audio I/O
   *
   *  @{ */

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
  PUBLIC const MYFLT *csoundGetSpout(CSOUND *csound);

  /** @}*/


  /** @defgroup MESSAGES Csound Messages and Text
   *
   *  @{ */
  /**
   * Displays an informational message.
   */
  PUBLIC CS_PRINTF2 void csoundMessage(CSOUND *, const char *format, ...);

  /**
   * Print message with special attributes (see msg_attr.h for the list of
   * available attributes). With attr=0, csoundMessageS() is identical to
   * csoundMessage().
   */
  PUBLIC CS_PRINTF3 void csoundMessageS(CSOUND *,int32_t attr, const char *format,
                                        ...);

  PUBLIC void csoundMessageV(CSOUND *, int32_t attr, const char *format,
                             va_list args);

  PUBLIC void csoundSetDefaultMessageCallback(void (*csoundMessageCallback_)
                                              (CSOUND *,int32_t attr,
                                               const char *format,
                                               va_list valist));

  /**
   * Sets a function to be called by Csound to print an informational message.
   * This callback is never called on --realtime mode
   */
  PUBLIC void csoundSetMessageCallback(CSOUND *,
                                       void (*csoundMessageCallback_)
                                       (CSOUND *, int32_t attr,const char *format,
                                        va_list valist));

  /**
   * Sets an alternative function to be called by Csound to print an
   * informational message, using a less granular signature.
   *  This callback can be set for --realtime mode.
   *  This callback is cleared after csoundReset
   */
  PUBLIC void csoundSetMessageStringCallback(CSOUND *csound,
                                             void (*csoundMessageStrCallback)
                                             (CSOUND *csound,int32_t attr,
                                              const char *str));
  /**
   * Creates a buffer for storing messages printed by Csound.
   * Should be called after creating a Csound instance and the buffer
   * can be freed by calling csoundDestroyMessageBuffer() before
   * deleting the Csound instance.
   * If 'toStdOut' is non-zero, the messages are also printed to
   * stdout and stderr (depending on the type of the message),
   * in addition to being stored in the buffer.
   * Using the message buffer ties up the internal message callback, so
   * csoundSetMessageCallback should not be called after creating the
   * message buffer.
   */
  PUBLIC void csoundCreateMessageBuffer(CSOUND *csound, int32_t toStdOut);

  /**
   * Returns the first message from the buffer.
   */
  PUBLIC const char*  csoundGetFirstMessage(CSOUND *csound);

  /**
   * Returns the attribute parameter (see msg_attr.h) of the first message
   * in the buffer.
   */
  PUBLIC int32_t csoundGetFirstMessageAttr(CSOUND *csound);

  /**
   * Removes the first message from the buffer.
   */
  PUBLIC void csoundPopFirstMessage(CSOUND *csound);

  /**
   * Returns the number of pending messages in the buffer.
   */
  PUBLIC int32_t csoundGetMessageCnt(CSOUND *csound);

  /**
   * Releases all memory used by the message buffer.
   */
  void PUBLIC csoundDestroyMessageBuffer(CSOUND *csound);

  /** @}*/
  /** @defgroup CONTROLEVENTS Channels, Control and Events
   *
   *  @{ */

  /**
   * Stores a pointer to the specified channel of the bus in *p,
   * creating the channel first if it does not exist yet.
   * 'type' must be the bitwise OR of exactly one of the following values,
   *   CSOUND_CONTROL_CHANNEL
   *     control data (one MYFLT value) - (MYFLYT **) pp
   *   CSOUND_AUDIO_CHANNEL
   *     audio data (csoundGetKsmps(csound) MYFLT values) -(MYFLYT **) pp
   *   CSOUND_STRING_CHANNEL
   *     string data as a STRINGDAT structure - (STRINGDAT **) pp
   *    (see csoundGetStringData() and csoundSetStringData())
   *   CSOUND_ARRAY_CHANNEL
   *     array data as an ARRAYDAT structure - (ARRAYDAT **) pp
   *    (see csoundArrayData***(), csoundSetArrayData(),
   *     csoundGetArrayData(), and csoundInitArrayData())
   *   CSOUND_PVS_CHANNEL
   *     pvs data as a PVSDATEXT structure - (PVSDAT **) pp
   *    (see csoundPvsData***(), csoundSetPvsData(),
   *     csoundGetPvsData(), and csoundInitPvsData())
   * and at least one of these:
   *   CSOUND_INPUT_CHANNEL
   *   CSOUND_OUTPUT_CHANNEL
   * If the channel already exists, it must match the data type
   * (control, string, audio, pvs or array), however, the input/output bits are
   * OR'd with the new value. Note that audio and string channels
   * can only be created after calling csoundCompile(), because the
   * storage size is not known until then.
   * Return value is zero on success, or a negative error code,
   *   CSOUND_MEMORY  there is not enough memory for allocating the channel
   *   CSOUND_ERROR   the specified name or type is invalid
   * or, if a channel with the same name but incompatible type
   * already exists, the type of the existing channel. In the case
   * of any non-zero return value, *p is set to NULL.
   * Note: to find out the type of a channel without actually
   * creating or changing it, set 'type' to zero, so that the return
   * value will be either the type of the channel, or CSOUND_ERROR
   * if it does not exist.
   *
   * Operations on **p are not thread-safe by default. The host is required
   * to take care of threadsafety by
   * 1) with control channels use __atomic_load() or
   *    __atomic_store() gcc atomic builtins to get or set a channel,
   *    if available.
   * 2) For string and audio channels (and controls if option 1 is not
   *    available), use csoundLockChannel() and csoundUnlockChannel()
   *    when accessing/modifying channel data at **p.
   * See Top/threadsafe.c in the Csound library sources for
   * examples.  Optionally, use the channel get/set functions
   * provided below, which are threadsafe by default.
   */
  PUBLIC int32_t csoundGetChannelPtr(CSOUND *,
                                 void **p, const char *name, int32_t type);

  /**
   *  Returns the var type for a channel name or NULL if the channel
   *  was not found.
   *  Currently supported channel var types are 'k' (control), 'a' (audio),
   *  'S' (string), 'f' (pvs), and '[' (array).
   */
  PUBLIC const char *csoundGetChannelVarTypeName(CSOUND *csound,
                                             const char *name);

  /**
   * Returns a list of allocated channels in *lst. A controlChannelInfo_t
   * structure contains the channel characteristics.
   * The return value is the number of channels, which may be zero if there
   * are none, or CSOUND_MEMORY if there is not enough memory for allocating
   * the list. In the case of no channels or an error, *lst is set to NULL.
   * Notes: the caller is responsible for freeing the list returned in *lst
   * with csoundDeleteChannelList(). The name pointers may become invalid
   * after calling csoundReset().
   */
  PUBLIC int32_t csoundListChannels(CSOUND *, controlChannelInfo_t **lst);

  /**
   * Releases a channel list previously returned by csoundListChannels().
   */
  PUBLIC void csoundDeleteChannelList(CSOUND *, controlChannelInfo_t *lst);

  /**
   * Set parameters hints for a control channel. These hints have no internal
   * function but can be used by front ends to construct GUIs or to constrain
   * values. See the controlChannelHints_t structure for details.
   * Returns zero on success, or a non-zero error code on failure:
   *   CSOUND_ERROR:  the channel does not exist, is not a control channel,
   *                  or the specified parameters are invalid
   *   CSOUND_MEMORY: could not allocate memory
   */
  PUBLIC int32_t csoundSetControlChannelHints(CSOUND *, const char *name,
                                          controlChannelHints_t hints);

  /**
   * Returns special parameters (assuming there are any) of a control channel,
   * previously set with csoundSetControlChannelHints() or the chnparams
   * opcode.
   * If the channel exists, is a control channel, the channel hints
   * are stored in the preallocated controlChannelHints_t structure. The
   * attributes member of the structure will be allocated inside this function
   * so it is necessary to free it explicitly in the host.
   *
   * The return value is zero if the channel exists and is a control
   * channel, otherwise, an error code is returned.
   */
  PUBLIC int32_t csoundGetControlChannelHints(CSOUND *, const char *name,
                                          controlChannelHints_t *hints);

  /**
   * locks access to the channel allowing access to data in
   * a threadsafe manner
   **/
  PUBLIC void csoundLockChannel(CSOUND *csound, const char *channel);

  /**
   * unlocks access to the channel, allowing access to data from
   * elsewhere.
   **/
  PUBLIC void csoundUnlockChannel(CSOUND *csound, const char *channel);

  /**
   * retrieves the value of control channel identified by *name.
   * If the err argument is not NULL, the error (or success) code
   * finding or accessing the channel is stored in it.
   */
  PUBLIC MYFLT csoundGetControlChannel(CSOUND *csound, const char *name,
                                       int32_t *err);

  /**
   * sets the value of control channel identified by *name
   */
  PUBLIC void csoundSetControlChannel(CSOUND *csound,
                                      const char *name, MYFLT val);

  /**
   * copies the audio channel identified by *name into array
   * *samples which should contain enough memory for ksmps MYFLTs
   */
  PUBLIC void csoundGetAudioChannel(CSOUND *csound,
                                    const char *name, MYFLT *samples);

  /**
   * sets the audio channel identified by *name with data from array
   * *samples which should contain at least ksmps MYFLTs
   */
  PUBLIC void csoundSetAudioChannel(CSOUND *csound, const char *name,
                                    const MYFLT *samples);

  /**
   * copies the string channel identified by *name into *string
   * which should contain enough memory for the string
   * (see csoundGetChannelDatasize() below)
   */
  PUBLIC void csoundGetStringChannel(CSOUND *csound,
                                     const char *name, char *string);

  /**
   * sets the string channel identified by *name with *string
   */
  PUBLIC  void csoundSetStringChannel(CSOUND *csound,
                                      const char *name, const char *string);

  /**
   * Create and initialise an array channel with a given array type
   * - "a" (audio sigs): each item is a ksmps-size MYFLT array
   * - "i" (init vars): each item is a MYFLT
   * - "S" (strings): each item is a STRINGDAT (see csoundGetStringData() and
   *   csoundSetStringData())
   * - "k" (control sigs): each item is a MYFLT
   *  dimensions - number of array dimensions
   *  sizes - sizes for each dimension
   * returns the ARRAYDAT for the requested channel or NULL on error
   * NB: if the channel exists and has already been initialised,
   * this function is a non-op.
   */
  PUBLIC ARRAYDAT *csoundInitArrayChannel(CSOUND *csound, const char *name,
                                          const char *type, int32_t dimensions,
                                          const int32_t *sizes);


  /**
   * Get the type of data the ARRAYDAT adat, returning
   * - "a" (audio sigs): each item is a ksmps-size MYFLT array
   * - "i" (init vars): each item is a MYFLT
   * - "S" (strings): each item is a STRINGDAT (see csoundGetStringData() and
   *   csoundSetStringData())
   * - "k" (control sigs): each item is a MYFLT
   */
  PUBLIC const char *csoundArrayDataType(const ARRAYDAT *adat);

  /**
   * Get the dimensions of the ARRAYDAT adat.
   **/
  PUBLIC int32_t csoundArrayDataDimensions(const ARRAYDAT *adat);

  /**
   * Get the sizes of each dimension of the ARRAYDAT adat;
   **/
  PUBLIC const int32_t* csoundArrayDataSizes(const ARRAYDAT *adat);


  /**
   * Set the data in the ARRAYDAT adat
   **/
  PUBLIC void csoundSetArrayData(ARRAYDAT *adat, const void* data);

  /**
   * Get the data from the ARRAYDAT adat
   **/
  PUBLIC const void *csoundGetArrayData(const ARRAYDAT *adat);


  /**
   * Get a null-terminated string from a STRINGDAT structure
   **/
  PUBLIC const char* csoundGetStringData(CSOUND *csound, STRINGDAT *sdata);

  /**
   * Set a STRINGDAT structure with a null-terminated string
   */
  PUBLIC void csoundSetStringData(CSOUND *csound, STRINGDAT *sdata,
                                  const char *str);

  /**
   * Create/initialise an Fsig channel with
   * size - FFT analysis size
   * overlap - analysis overlap size
   * winsize - analysis window size
   * wintype - analysis window type (see pvsdat types enumeration)
   * format - analysis data format (see pvsdat format enumeration)
   * returns the PVSDAT for the requested channel or NULL on error
   * NB: if the channel exists and has already been initialised,
   * this function is a non-op.
   */
  PUBLIC PVSDAT *csoundInitPvsChannel(CSOUND *csound, const char* name,
                                      int32_t size, int32_t overlap, int32_t winsize,
                                      int32_t wintype, int32_t format);

  /**
   * Get the analysis FFT size used by the PVSDAT pvsdat
   */
  PUBLIC int32_t csoundPvsDataFFTSize(const PVSDAT *pvsdat);

  /**
   * Get the analysis overlap size used by the PVSDAT pvsdat
   */
  PUBLIC int32_t csoundPvsDataOverlap(const PVSDAT *pvsdat);

  /**
   * Get the analysis window size used by the PVSDAT pvsdat
   */
  PUBLIC int32_t csoundPvsDataWindowSize(const PVSDAT *pvsdat);

  /**
   * Get the analysis data format used by the PVSDAT pvsdat
   */
  PUBLIC int32_t csoundPvsDataFormat(const PVSDAT *pvsdat);

  /**
   * Get the current framecount from PVSDAT pvsdat
   */
  PUBLIC uint32_t csoundPvsDataFramecount(const PVSDAT *pvsdat);

  /**
   * Get the analysis data frame from the PVSDAT pvsdat
   */
  PUBLIC const float *csoundGetPvsData(const PVSDAT *pvsdat);

  /**
   * Set the analysis data frame in the PVSDAT pvsdat
   */
  PUBLIC void csoundSetPvsData(PVSDAT *pvsdat, const float *frame);

  /**
   * returns the size of data stored in a channel; for string channels
   * this might change if the channel space gets reallocated
   * Since string variables use dynamic memory allocation
   * this function can be called to get the space required for
   * csoundGetStringChannel()
   */
  PUBLIC int32_t csoundGetChannelDatasize(CSOUND *csound, const char *name);

  /** Sets the function which will be called whenever the invalue opcode
   * is used. */
  PUBLIC void
  csoundSetInputChannelCallback(CSOUND *csound,
                                channelCallback_t inputChannelCalback);

  /** Sets the function which will be called whenever the outvalue opcode
   * is used. */
  PUBLIC void
  csoundSetOutputChannelCallback(CSOUND *csound,
                                 channelCallback_t outputChannelCalback);

  /**
   * Schedule a new realtime event. 'type' is the event type
   * type 0 - instrument instance     CS_INSTR_EVENT
   * type 1 - function table instance CS_TABLE_EVENT
   * type 2 - end event               CS_END_EVENT
   * event parameters is nparams MYFLT array with the event parameters (p-fields)
   * optionally run asynchronously (async = 1)
   * NB: This is non-op before csoundStart() is called.
   */
  PUBLIC void  csoundEvent(CSOUND *, int32_t type, const MYFLT *params,
                           int32_t nparams, int32_t async);

  /**
   * Schedule new score or realtime event(s) as a NULL-terminated string
   * Two operation modes are supported:
   * - Score events: any calls before csoundStart() add the string events to
   * the score (before pre-processing) (async should be set to 0).
   * - Realtime events: after the engine starts, string events are added to
   * the realtime event queue.
   *
   * Multiple events separated by newlines are possible
   * and score preprocessing (carry, etc) is applied.
   * optionally run asynchronously (async = 1)
   */
  PUBLIC void  csoundEventString(CSOUND *, const char *message, int32_t async);

  /**
   * Get the instrument number for a given instrument name string
   * for use in numeric parameters list (csoundEvent())
   * returns the instrument number or -1 if not found.
   */
  PUBLIC int32 csoundGetInstrNumber(CSOUND *, const char *name);

  /**
   * Set the ASCII code of the most recent key pressed.
   * This value is used by the 'sensekey' opcode if a callback
   * for returning keyboard events is not set (see
   * csoundRegisterKeyboardCallback()).
   */
  PUBLIC void csoundKeyPress(CSOUND *, char c);

  /**
   * Registers general purpose callback functions that will be called to query
   * keyboard events. These callbacks are called on every control period by
   * the sensekey opcode.
   * The callback is preserved on csoundReset(), and multiple
   * callbacks may be set and will be called in reverse order of
   * registration. If the same function is set again, it is only moved
   * in the list of callbacks so that it will be called first, and the
   * user data and type mask parameters are updated. 'typeMask' can be the
   * bitwise OR of callback types for which the function should be called,
   * or zero for all types.
   * Returns zero on success, CSOUND_ERROR if the specified function
   * pointer or type mask is invalid, and CSOUND_MEMORY if there is not
   * enough memory.
   *
   * The callback function takes the following arguments:
   *   void *userData
   *     the "user data" pointer, as specified when setting the callback
   *   void *p
   *     data pointer, depending on the callback type
   *   uint32_t type
   *     callback type, can be one of the following (more may be added in
   *     future versions of Csound):
   *       CSOUND_CALLBACK_KBD_EVENT
   *       CSOUND_CALLBACK_KBD_TEXT
   *         called by the sensekey opcode to fetch key codes. The data
   *         pointer is a pointer to a single value of type 'int', for
   *         returning the key code, which can be in the range 1 to 65535,
   *         or 0 if there is no keyboard event.
   *         For CSOUND_CALLBACK_KBD_EVENT, both key press and release
   *         events should be returned (with 65536 (0x10000) added to the
   *         key code in the latter case) as unshifted ASCII codes.
   *         CSOUND_CALLBACK_KBD_TEXT expects key press events only as the
   *         actual text that is typed.
   * The return value should be zero on success, negative on error, and
   * positive if the callback was ignored (for example because the type is
   * not known).
   */
  PUBLIC int32_t csoundRegisterKeyboardCallback(CSOUND *,
                                            int32_t (*func)(void *userData, void *p,
                                                        uint32_t type),
                                            void *userData, uint32_t type);

  /**
   * Removes a callback previously set with csoundRegisterKeyboardCallback().
   */
  PUBLIC void csoundRemoveKeyboardCallback(CSOUND *csound,
                                           int32_t (*func)(void *, void *, uint32_t));


  /** @}*/
  /** @defgroup TABLE Tables
   *
   *  @{ */

  /**
   * Returns the length of a function table (not including the guard point32_t),
   * or -1 if the table does not exist.
   */
  PUBLIC int32_t csoundTableLength(CSOUND *, int32_t table);

  /**
   * Stores pointer to function table 'tableNum' in *tablePtr,
   * and returns the table length (not including the guard point32_t).
   * If the table does not exist, *tablePtr is set to NULL and
   * -1 is returned.
   * NB: this function and the tablePtr returned are not threadsafe 
   */
  PUBLIC int32_t csoundGetTable(CSOUND *, MYFLT **tablePtr, int32_t tableNum);

  /**
   * Stores pointer to the arguments used to generate
   * function table 'tableNum' in *argsPtr,
   * and returns the number of arguments used.
   * If the table does not exist, *argsPtr is set to NULL and
   * -1 is returned.
   * NB: the argument list starts with the GEN number and is followed by
   * its parameters. eg. f 1 0 1024 10 1 0.5  yields the list {10.0,1.0,0.5}
   * This function and the argsPtr returned are not threadsafe
   */
  PUBLIC int32_t csoundGetTableArgs(CSOUND *csound, MYFLT **argsPtr,
				    int32_t tableNum);

  /** 
   * Copies an array stored in ptable to the function table
   * number given by table, which should exist in the engine.
   * The input array should be at least as long as the table
   * size plus one (guard point required).
   * This function is threadsafe and can also be run asynchronously
   */
  PUBLIC void csoundTableCopyIn(CSOUND *csound, int32_t table,
				 const MYFLT *ptable, int32_t async);


   /** 
   * Copies a function table number given by table, 
   * which should exist in the engine, into the array ptable,
   * and have enough space to accommodate the array size.
   * This function is threadsafe and can also be run asynchronously
   */
  PUBLIC void csoundTableCopyOut(CSOUND *csound, int32_t table,
				MYFLT *ptable, int32_t async);
  

  /** @}*/
  /** @defgroup SCOREHANDLING Score Handling
   *
   *  @{ */

  /**
   * Returns the current score time in seconds
   * since the beginning of performance.
   */
  PUBLIC double csoundGetScoreTime(CSOUND *);

  /**
   * Sets whether Csound score events are performed or not, independently
   * of real-time MIDI events (see csoundSetScorePending()).
   */
  PUBLIC int32_t csoundIsScorePending(CSOUND *);

  /**
   * Sets whether Csound score events are performed or not (real-time
   * events will continue to be performed). Can be used by external software,
   * such as a VST host, to turn off performance of score events (while
   * continuing to perform real-time events), for example to
   * mute a Csound score while working on other tracks of a piece, or
   * to play the Csound instruments live.
   */
  PUBLIC void csoundSetScorePending(CSOUND *, int32_t pending);

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
   * Sorts score file 'inFile' and writes the result to 'outFile'.
   * The Csound instance should be initialised
   * before calling this function, and csoundReset() should be called
   * after sorting the score to clean up. On success, zero is returned.
   */
  PUBLIC int32_t csoundScoreSort(CSOUND *, FILE *inFile, FILE *outFile);

  /**
   * Extracts from 'inFile', controlled by 'extractFile', and writes
   * the result to 'outFile'. The Csound instance should be initialised
   * before calling this function, and csoundReset()
   * should be called after score extraction to clean up.
   * The return value is zero on success.
   */
  PUBLIC int32_t csoundScoreExtract(CSOUND *,
                                FILE *inFile, FILE *outFile, FILE *extractFile);


  /**
   * Waits for at least the specified number of milliseconds,
   * yielding the CPU to other threads.
   */
  PUBLIC void csoundSleep(size_t milliseconds);


  /** @}*/
  /** @defgroup OPCODES Opcodes
   *
   *  @{ */

  /**
   *  Loads all plugins from a given directory. Generally called
   *   immediately after csoundCreate()
   *  to make new opcodes/modules available for compilation and performance.
   */
  PUBLIC int32_t csoundLoadPlugins(CSOUND *csound, const char *dir);


  /**
   * Appends an opcode implemented by external software
   * to Csound's internal opcode list.
   * The opcode list is extended by one slot,
   * and the parameters are copied into the new slot.
   * Returns zero on success.
   */
  PUBLIC int32_t csoundAppendOpcode (CSOUND *, const char *opname,
                                 size_t dsblksiz, int32_t flags,
                                 const char *outypes, const char *intypes,
                                 int32_t (*init)(CSOUND *, void *),
                                 int32_t (*perf)(CSOUND *, void *),
                                 int32_t (*deinit)(CSOUND *, void *));

  /** @}*/
#endif  /* !CSOUND_CSDL_H */

  /* realtime audio module functions */
#include "csound_rtaudio.h"
  /* realtime MIDI module functions */
#include "csound_rtmidi.h"
  /* typedefs, macros, and interface functions for configuration variables */
#include "cfgvar.h"
  /* message attribute definitions for csoundMessageS() and csoundMessageV() */
#include "msg_attr.h"
  /* macro definitions for Csound release, and API version */
#include "version.h"

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_H */
