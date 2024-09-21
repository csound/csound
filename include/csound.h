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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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

  /**
   *  csound configuration structure, mirrors part of
   *  OPARMS, uses more meaningful names
   */
  typedef struct {
    int     debug_mode;     /* debug mode, 0 or 1 */
    int     buffer_frames;  /* number of frames in in/out buffers */
    int     hardware_buffer_frames; /* ibid. hardware */
    int     displays;       /* graph displays, 0 or 1 */
    int     ascii_graphs;   /* use ASCII graphs, 0 or 1 */
    int     postscript_graphs; /* use postscript graphs, 0 or 1 */
    int     message_level;     /* message printout control */
    int     tempo;             /* tempo (sets Beatmode)  */
    int     ring_bell;         /* bell, 0 or 1 */
    int     use_cscore;        /* use cscore for processing */
    int     terminate_on_midi; /* terminate performance at the end
                                  of midifile, 0 or 1 */
    int     heartbeat;         /* print heart beat, 0 or 1 */
    int     defer_gen01_load ;  /* defer GEN01 load, 0 or 1 */
    int     midi_key;           /* pfield to map midi key no */
    int     midi_key_cps;       /* pfield to map midi key no as cps */
    int     midi_key_oct;       /* pfield to map midi key no as oct */
    int     midi_key_pch;       /* pfield to map midi key no as pch */
    int     midi_velocity;      /* pfield to map midi velocity */
    int     midi_velocity_amp;   /* pfield to map midi velocity as amplitude */
    int     no_default_paths;     /* disable relative paths from files, 0 or 1 */
    int     number_of_threads;   /* number of threads for multicore performance */
    int     syntax_check_only;   /* do not compile, only check syntax */
    int     csd_line_counts;     /* csd line error reporting */
    int     compute_weights;     /* deprecated, kept for backwards comp.  */
    int     realtime_mode;       /* use realtime priority mode, 0 or 1 */
    int     sample_accurate;     /* use sample-level score event accuracy */
    MYFLT   sample_rate_override; /* overriding sample rate */
    MYFLT   control_rate_override; /* overriding control rate */
    int     nchnls_override;  /* overriding number of out channels */
    int     nchnls_i_override;  /* overriding number of in channels */
    MYFLT   e0dbfs_override;   /* overriding 0dbfs */
    int     daemon;  /* daemon mode */
    int     ksmps_override; /* ksmps override */
    int     FFT_library;    /* fft_lib */
  } CSOUND_PARAMS;

  /**
   * Device information
   */
  typedef struct {
    char device_name[128];
    char device_id[128];
    char rt_module[128];
    int max_nchnls;
    int isOutput;
  } CS_AUDIODEVICE;

  typedef struct {
    char device_name[128];
    char interface_name[128];
    char device_id[128];
    char midi_module[128];
    int isOutput;
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
    int x;
    int y;
    int width;
    int height;
    /** This member must be set explicitly to NULL if not used */
    char *attributes;
  } controlChannelHints_t;

  typedef struct controlChannelInfo_s {
    char  *name;
    int     type;
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
    CS_END_EVENT
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
  PUBLIC int csoundInitialize(int flags);

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
  PUBLIC int csoundGetVersion(void);

  /**
   * Returns the API version number times 100 (1.00 = 100).
   */
  PUBLIC int csoundGetAPIVersion(void);

  /**
   * Returns the number of audio sample frames per second.
   */
  PUBLIC MYFLT csoundGetSr(CSOUND *) ;

  /**
   * Returns the number of control samples per second.
   */
  PUBLIC MYFLT csoundGetKr(CSOUND *);

  /**
   * Returns the number of audio sample frames per control sample.
   */
  PUBLIC uint32_t csoundGetKsmps(CSOUND *);

  /**
   * Returns the number of audio channels in the Csound instance.
   * If isInput = 0, the value of nchnls is returned,
   * otherwise nchnls_i. If this variable is
   * not set, the value is always taken from nchnls.
   */
  PUBLIC uint32_t csoundGetChannels(CSOUND *, int isInput);

  /**
   * Returns the 0dBFS level of the spin/spout buffers.
   */
  PUBLIC MYFLT csoundGet0dBFS(CSOUND *);

  /**
   * Returns the A4 frequency reference
   */
  PUBLIC MYFLT csoundGetA4(CSOUND *);

  /**
   * Return the current performance time in samples
   */
  PUBLIC int64_t csoundGetCurrentTimeSamples(CSOUND *csound);

  /**
   * Return the size of MYFLT in bytes.
   */
  PUBLIC int csoundGetSizeOfMYFLT(void);

  /**
   * Returns host data.
   */
  PUBLIC void *csoundGetHostData(CSOUND *);

  /**
   * Sets host data.
   */
  PUBLIC void csoundSetHostData(CSOUND *, void *hostData);

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
  PUBLIC int csoundSetGlobalEnv(const char *name, const char *value);

  /**
   * Set csound options (flag). Returns CSOUND_SUCCESS on success.
   * This needs to be called after csoundCreate() and before any code is
   * compiled. Multiple options are allowed in one string.
   * Returns zero on success.
   */
  PUBLIC int csoundSetOption(CSOUND *csound, const char *option);

  /**
   *  Configure Csound with a given set of parameters defined in
   *  the CSOUND_PARAMS structure. These parameters are the part of the
   *  OPARMS struct that are configurable through command line flags.
   *  The CSOUND_PARAMS structure can be obtained using csoundGetParams().
   *  These options should only be changed before performance has started.
   */
  PUBLIC void csoundSetParams(CSOUND *csound, CSOUND_PARAMS *p);

  /**
   *  Get the current set of parameters from a CSOUND instance in
   *  a CSOUND_PARAMS structure. See csoundSetParams().
   */
  PUBLIC void csoundGetParams(CSOUND *csound, CSOUND_PARAMS *p);

  /**
   * Returns whether Csound is set to print debug messages sent through the
   * DebugMsg() internal API function. Anything different to 0 means true.
   */
  PUBLIC int csoundGetDebug(CSOUND *);

  /**
   * Sets whether Csound prints debug messages from the DebugMsg() internal
   * API function. Anything different to 0 means true.
   */
  PUBLIC void csoundSetDebug(CSOUND *, int debug);

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
   *  int n = 0;
   *  while(!csoundGetModule(csound, n++, &name, &type))
   *       printf("Module %d:  %s (%s) \n", n, name, type);
   * \endcode
   */
  PUBLIC int csoundGetModule(CSOUND *csound, int number,
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
   *   int i,n = csoundGetAudioDevList(csound,NULL,1);
   *   CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *)
   *       malloc(n*sizeof(CS_AUDIODEVICE));
   *   csoundGetAudioDevList(csound,devs,1);
   *   for(i=0; i < n; i++)
   *       csound->Message(csound, " %d: %s (%s)\n",
   *             i, devs[i].device_id, devs[i].device_name);
   *   free(devs);
   * \endcode
   */
  PUBLIC int csoundGetAudioDevList(CSOUND *csound,
                                   CS_AUDIODEVICE *list, int isOutput);

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
  PUBLIC int csoundGetMIDIDevList(CSOUND *csound,
                                  CS_MIDIDEVICE *list, int isOutput);

  /**
   * Returns the Csound message level (from 0 to 231).
   */
  PUBLIC int csoundGetMessageLevel(CSOUND *);

  /**
   * Sets the Csound message level (from 0 to 231).
   */
  PUBLIC void csoundSetMessageLevel(CSOUND *, int messageLevel);

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
  PUBLIC int csoundCompile(CSOUND *, int argc, const char **argv);

  /**
   * Parse, and compile the given orchestra from an ASCII string,
   * also evaluating any global space code (i-time only)
   * in synchronous or asynchronous (async = 1) mode.
   * /code
   *       char *orc = "instr 1 \n a1 rand 0dbfs/4 \n out a1 \n";
   *       csoundCompileOrc(csound, orc, 0);
   * /endcode
   */
  PUBLIC int csoundCompileOrc(CSOUND *csound, const char *str, int async);

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
   */
  PUBLIC MYFLT csoundEvalCode(CSOUND *csound, const char *str);

  /**
   * Compiles a Csound input file (CSD, .csd file) or a tx string
   * containing the CSD code.
   * Returns a non-zero error code on failure.
   *
   * If csoundStart is called before csoundCompileCsd, the <CsOptions>
   * element is ignored (but csoundSetOption can be called any number of
   * times), the <CsScore> element is not pre-processed, but dispatched as
   * real-time events; and performance continues indefinitely, or until
   * ended by calling csoundStop or some other logic. In this "real-time"
   * mode, the sequence of calls should be:
   *
   * \code
   *
   * csoundSetOption("-an_option");
   * csoundSetOption("-another_option");
   * csoundStart(csound);
   * csoundCompileCSD(csound, csd_filename, 0);
   * while (1) {
   *    csoundPerformBuffer(csound);
   *    // Something to break out of the loop
   *    // when finished here...
   * }
   * csoundReset(csound);
   *
   * \endcode
   *
   * NB: this function can be called repeatedly during performance to
   * replace or add new instruments and events.
   *
   * But if csoundCompileCsd is called before csoundStart, the <CsOptions>
   * element is used, the <CsScore> section is pre-processed and dispatched
   * normally, and performance terminates when the score terminates, or
   * csoundStop is called. In this "non-real-time" mode (which can still
   * output real-time audio and handle real-time events), the sequence of
   * calls should be:
   *
   * \code
   *
   * csoundCompileCsd(csound, csd_filename, 0);
   * csoundStart(csound);
   * while (1) {
   *    int finished = csoundPerformBuffer(csound);
   *    if (finished) break;
   * }
   * csoundCleanup(csound);
   * csoundReset(csound);
   *
   * \endcode
   *
   * if mode = 1, csd contains a full CSD code (rather than a filename).
   * This is convenient when it is desirable to package the csd as part of
   * an application or a multi-language piece.
   *
   */
  PUBLIC int csoundCompileCSD(CSOUND *csound, const char *csd, int mode);

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
  PUBLIC int csoundStart(CSOUND *csound);

  /**
   * Senses input events, and performs one control sample worth (ksmps) of
   * audio output. csoundStart() must be called first.
   * Returns false during performance, and true when performance is finished.
   * If called until it returns true, will perform an entire score.
   * Enables external software to control the execution of Csound,
   * and to synchronize performance with audio input and output.
   */
  PUBLIC int  csoundPerformKsmps(CSOUND *);

  /**
   * Run utility with the specified name and command line arguments.
   * Should be called after loading utility plugins.
   * Use csoundReset() to clean up after calling this function.
   * Returns zero if the utility was run successfully.
   */
  PUBLIC int csoundRunUtility(CSOUND *, const char *name,
                              int argc, char **argv);

  /**
   * Resets all internal memory and state in preparation for a new performance.
   * Enables external software to run successive Csound performances
   * without reloading Csound.
   */
  PUBLIC void csoundReset(CSOUND *);
  /** @}*/

  /** @defgroup RTAUDIOIO Realtime Audio I/O
   *
   *  @{ */

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

  /**
   * retrieves a module name and type ("audio" or "midi") given a
   * number Modules are added to list as csound loads them returns
   * CSOUND_SUCCESS on success and CSOUND_ERROR if module number
   * was not found
   *
   * \code
   *  char *name, *type;
   *  int n = 0;
   *  while(!csoundGetModule(csound, n++, &name, &type))
   *       printf("Module %d:  %s (%s) \n", n, name, type);
   * \endcode
   */


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
   *  Sets the current MIDI IO module
   */
  PUBLIC void csoundSetMIDIModule(CSOUND *csound, const char *module);


  /**
   * Sets callback for opening real time MIDI input.
   */
  PUBLIC void csoundSetExternalMidiInOpenCallback(CSOUND *,int (*func)
                                                  (CSOUND *,void **userData,
                                                   const char *devName));

  /**
   * Sets callback for reading from real time MIDI input.
   */
  PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *,int (*func)
                                                (CSOUND *, void *userData,
                                                 unsigned char *buf,
                                                 int nBytes));

  /**
   * Sets callback for closing real time MIDI input.
   */
  PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *,int (*func)
                                                   (CSOUND *,void *userData));

  /**
   * Sets callback for opening real time MIDI output.
   */
  PUBLIC void csoundSetExternalMidiOutOpenCallback(CSOUND *,int (*func)
                                                   (CSOUND *,void **userData,
                                                    const char *devName));

  /**
   * Sets callback for writing to real time MIDI output.
   */
  PUBLIC void csoundSetExternalMidiWriteCallback(CSOUND *, int (*func)
                                                 (CSOUND *,void *userData,
                                                  const unsigned char *buf,
                                                  int nBytes));

  /**
   * Sets callback for closing real time MIDI output.
   */
  PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *,
                                                    int (*func)(CSOUND *,
                                                                void *userData));

  /**
   * Sets callback for converting MIDI error codes to strings.
   */
  PUBLIC void csoundSetExternalMidiErrorStringCallback(CSOUND *,
                                                       const char *(*func)(int));


  /**
   * Sets a function that is called to obtain a list of MIDI devices.
   * This should be set by IO plugins, and should not be used by hosts.
   * (See csoundGetMIDIDevList())
   */
  PUBLIC void csoundSetMIDIDeviceListCallback(CSOUND *csound,
                                              int (*mididevlist__)
                                              (CSOUND *,CS_MIDIDEVICE *list,
                                               int isOutput));

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
  PUBLIC CS_PRINTF3 void csoundMessageS(CSOUND *,int attr, const char *format,
                                        ...);

  PUBLIC void csoundMessageV(CSOUND *, int attr, const char *format,
                             va_list args);

  PUBLIC void csoundSetDefaultMessageCallback(void (*csoundMessageCallback_)
                                              (CSOUND *,int attr,
                                               const char *format,
                                               va_list valist));

  /**
   * Sets a function to be called by Csound to print an informational message.
   * This callback is never called on --realtime mode
   */
  PUBLIC void csoundSetMessageCallback(CSOUND *,
                                       void (*csoundMessageCallback_)
                                       (CSOUND *, int attr,const char *format,
                                        va_list valist));

  /**
   * Sets an alternative function to be called by Csound to print an
   * informational message, using a less granular signature.
   *  This callback can be set for --realtime mode.
   *  This callback is cleared after csoundReset
   */
  PUBLIC void csoundSetMessageStringCallback(CSOUND *csound,
                                             void (*csoundMessageStrCallback)
                                             (CSOUND *csound,int attr,
                                              const char *str));
  /**
   * Creates a buffer for storing messages printed by Csound.
   * Should be called after creating a Csound instance andthe buffer
   * can be freed by calling csoundDestroyMessageBuffer() before
   * deleting the Csound instance. You will generally want to call
   * csoundCleanup() to make sure the last messages are flushed to
   * the message buffer before destroying Csound.
   * If 'toStdOut' is non-zero, the messages are also printed to
   * stdout and stderr (depending on the type of the message),
   * in addition to being stored in the buffer.
   * Using the message buffer ties up the internal message callback, so
   * csoundSetMessageCallback should not be called after creating the
   * message buffer.
   */
  PUBLIC void csoundCreateMessageBuffer(CSOUND *csound, int toStdOut);

  /**
   * Returns the first message from the buffer.
   */
  PUBLIC const char*  csoundGetFirstMessage(CSOUND *csound);

  /**
   * Returns the attribute parameter (see msg_attr.h) of the first message
   * in the buffer.
   */
  PUBLIC int csoundGetFirstMessageAttr(CSOUND *csound);

  /**
   * Removes the first message from the buffer.
   */
  PUBLIC void csoundPopFirstMessage(CSOUND *csound);

  /**
   * Returns the number of pending messages in the buffer.
   */
  PUBLIC int csoundGetMessageCnt(CSOUND *csound);

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
  PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 void **p, const char *name, int type);

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
  PUBLIC int csoundListChannels(CSOUND *, controlChannelInfo_t **lst);

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
  PUBLIC int csoundSetControlChannelHints(CSOUND *, const char *name,
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
  PUBLIC int csoundGetControlChannelHints(CSOUND *, const char *name,
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
                                       int *err);

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
                                          const char *type, int dimensions,
                                          const int *sizes);


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
  PUBLIC int csoundArrayDataDimensions(const ARRAYDAT *adat);

  /**
   * Get the sizes of each dimension of the ARRAYDAT adat;
   **/
  PUBLIC const int* csoundArrayDataSizes(const ARRAYDAT *adat);


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
                                      int size, int overlap, int winsize,
                                      int wintype, int format);

  /**
   * Get the analysis FFT size used by the PVSDAT pvsdat
   */
  PUBLIC int csoundPvsDataFFTSize(const PVSDAT *pvsdat);

  /**
   * Get the analysis overlap size used by the PVSDAT pvsdat
   */
  PUBLIC int csoundPvsDataOverlap(const PVSDAT *pvsdat);

  /**
   * Get the analysis window size used by the PVSDAT pvsdat
   */
  PUBLIC int csoundPvsDataWindowSize(const PVSDAT *pvsdat);

  /**
   * Get the analysis data format used by the PVSDAT pvsdat
   */
  PUBLIC int csoundPvsDataFormat(const PVSDAT *pvsdat);

  /**
   * Get the current framecount from PVSDAT pvsdat
   */
  PUBLIC unsigned int csoundPvsDataFramecount(const PVSDAT *pvsdat);

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
  PUBLIC int csoundGetChannelDatasize(CSOUND *csound, const char *name);

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
   * Send a new event. 'type' is the event type
   * type 0 - instrument instance     CS_INSTR_EVENT
   * type 1 - function table instance CS_TABLE_EVENT
   * type 2 - end event               CS_END_EVENT
   * event parameters is nparams MYFLT array with the event parameters (p-fields)
   * optionally run asynchronously (async = 1)
   */
  PUBLIC void  csoundEvent(CSOUND *, int type, MYFLT *params, int nparams, int async);

  /**
   * Send a new event as a NULL-terminated string
   * Multiple events separated by newlines are possible
   * and score preprocessing (carry, etc) is applied.
   * optionally run asynchronously (async = 1)
   */
  PUBLIC void  csoundEventString(CSOUND *, const char *message, int async);

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
   *   unsigned int type
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
  PUBLIC int csoundRegisterKeyboardCallback(CSOUND *,
                                            int (*func)(void *userData, void *p,
                                                        unsigned int type),
                                            void *userData, unsigned int type);

  /**
   * Removes a callback previously set with csoundRegisterKeyboardCallback().
   */
  PUBLIC void csoundRemoveKeyboardCallback(CSOUND *csound,
                                           int (*func)(void *, void *, unsigned int));


  /** @}*/
  /** @defgroup TABLE Tables
   *
   *  @{ */

  /**
   * Returns the length of a function table (not including the guard point),
   * or -1 if the table does not exist.
   */
  PUBLIC int csoundTableLength(CSOUND *, int table);

  /**
   * Stores pointer to function table 'tableNum' in *tablePtr,
   * and returns the table length (not including the guard point).
   * If the table does not exist, *tablePtr is set to NULL and
   * -1 is returned.
   */
  PUBLIC int csoundGetTable(CSOUND *, MYFLT **tablePtr, int tableNum);

  /**
   * Stores pointer to the arguments used to generate
   * function table 'tableNum' in *argsPtr,
   * and returns the number of arguments used.
   * If the table does not exist, *argsPtr is set to NULL and
   * -1 is returned.
   * NB: the argument list starts with the GEN number and is followed by
   * its parameters. eg. f 1 0 1024 10 1 0.5  yields the list {10.0,1.0,0.5}
   */
  PUBLIC int csoundGetTableArgs(CSOUND *csound, MYFLT **argsPtr, int tableNum);

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
   * Sorts score file 'inFile' and writes the result to 'outFile'.
   * The Csound instance should be initialised
   * before calling this function, and csoundReset() should be called
   * after sorting the score to clean up. On success, zero is returned.
   */
  PUBLIC int csoundScoreSort(CSOUND *, FILE *inFile, FILE *outFile);

  /**
   * Extracts from 'inFile', controlled by 'extractFile', and writes
   * the result to 'outFile'. The Csound instance should be initialised
   * before calling this function, and csoundReset()
   * should be called after score extraction to clean up.
   * The return value is zero on success.
   */
  PUBLIC int csoundScoreExtract(CSOUND *,
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
  PUBLIC int csoundLoadPlugins(CSOUND *csound, const char *dir);


  /**
   * Appends an opcode implemented by external software
   * to Csound's internal opcode list.
   * The opcode list is extended by one slot,
   * and the parameters are copied into the new slot.
   * Returns zero on success.
   */
  PUBLIC int csoundAppendOpcode (CSOUND *, const char *opname,
                                 int dsblksiz, int flags,
                                 const char *outypes, const char *intypes,
                                 int(*init)(CSOUND *, void *),
                                 int(*perf)(CSOUND *, void *),
                                 int(*deinit)(CSOUND *, void *));

  /** @}*/

#endif  /* !CSOUND_CSDL_H */
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
