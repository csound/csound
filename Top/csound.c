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
 *
 * 29 May 2002 - ma++ merge with CsoundLib.
 * 30 May 2002 - mkg add csound "this" pointer argument back into merge.
 * 27 Jun 2002 - mkg complete Linux dl code and Makefile
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "csoundCore.h"
#include "csmodule.h"

#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef WIN32
# include <windows.h>
#endif

  int fltk_abort = 0;

  /* from oload.c, initial state of ENVIRON structure */
  extern const ENVIRON cenviron_;
  /* from threads.c */
  void csoundLock(void);
  void csoundUnLock(void);

  typedef struct csInstance_s {
    ENVIRON             *csound;
    struct csInstance_s *nxt;
  } csInstance_t;

  /* initialisation state: */
  /* 0: not done yet, 1: complete, 2: in progress, -1: failed */
  static  volatile  int init_done = 0;
  /* chain of allocated Csound instances */
  static  csInstance_t  *instance_list = NULL;

  static void destroy_all_instances(void)
  {
    csInstance_t  *p;
    csoundLock(); p = instance_list; csoundUnLock();
    if (p == NULL)
      return;
    fltk_abort = 1;
    while (1) {
      csoundLock(); p = instance_list; csoundUnLock();
      if (p == NULL)
        break;
      csoundDestroy(p->csound);
    }
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
    usleep(250000);
#elif defined(WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif
  }

#if !defined(LINUX) && !defined(SGI) && !defined(__BEOS__) && !defined(__MACH__)
  static char *signal_to_string(int sig)
  {
    switch(sig) {
#ifdef SIGHUP
    case SIGHUP:        return "Hangup";
#endif
#ifdef SIGINT
    case SIGINT:        return "Interrupt";
#endif
#ifdef SIGQUIT
    case SIGQUIT:       return "Quit";
#endif
#ifdef SIGILL
    case SIGILL:        return "Illegal instruction";
#endif
#ifdef SIGTRAP
    case SIGTRAP:       return "Trace trap";
#endif
#ifdef SIGABRT
    case SIGABRT:       return "Abort";
#endif
#ifdef SIGBUS
    case SIGBUS:        return "BUS error";
#endif
#ifdef SIGFPE
    case SIGFPE:        return "Floating-point exception";
#endif
#ifdef SIGUSR1
    case SIGUSR1:       return "User-defined signal 1";
#endif
#ifdef SIGSEGV
    case SIGSEGV:       return "Segmentation violation";
#endif
#ifdef SIGUSR2
    case SIGUSR2:       return "User-defined signal 2";
#endif
#ifdef SIGPIPE
    case SIGPIPE:       return "Broken pipe";
#endif
#ifdef SIGALRM
    case SIGALRM:       return "Alarm clock";
#endif
#ifdef SIGTERM
    case SIGTERM:       return "Termination";
#endif
#ifdef SIGSTKFLT
    case SIGSTKFLT:     return "???";
#endif
#ifdef SIGCHLD
    case SIGCHLD:       return "Child status has changed";
#endif
#ifdef SIGCONT
    case SIGCONT:       return "Continue";
#endif
#ifdef SIGSTOP
    case SIGSTOP:       return "Stop, unblockable";
#endif
#ifdef SIGTSTP
    case SIGTSTP:       return "Keyboard stop";
#endif
#ifdef SIGTTIN
    case SIGTTIN:       return "Background read from tty";
#endif
#ifdef SIGTTOU
    case SIGTTOU:       return "Background write to tty";
#endif
#ifdef SIGURG
    case SIGURG:        return "Urgent condition on socket ";
#endif
#ifdef SIGXCPU
    case SIGXCPU:       return "CPU limit exceeded";
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:       return "File size limit exceeded ";
#endif
#ifdef SIGVTALRM
    case SIGVTALRM:     return "Virtual alarm clock ";
#endif
#ifdef SIGPROF
    case SIGPROF:       return "Profiling alarm clock";
#endif
#ifdef SIGWINCH
    case SIGWINCH:      return "Window size change ";
#endif
#ifdef SIGIO
    case SIGIO:         return "I/O now possible";
#endif
#ifdef SIGPWR
    case SIGPWR:        return "Power failure restart";
#endif
    default:
      return "???";
    }
  }

#ifndef __MACH__
  void psignal(int sig, char *str)
  {
    fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
  }
#endif
#endif

#if defined(__BEOS__)
  void psignal(int sig, char *str)
  {
    fprintf(stderr, "%s: %s\n", str, strsignal(sig));
  }
#endif

  static void signal_handler(int sig)
  {
#if defined(USE_FLTK)
#if defined(LINUX) || defined(NETBSD) || defined(__MACH__)
    if (sig == SIGALRM)
      return;
#endif
#endif
    psignal(sig, "Csound tidy up");
    fltk_abort = 1;
    exit(1);
  }

  static void install_signal_handler(void)
  {
    int *x;
    int sigs[] = {
#if defined(LINUX) || defined(SGI) || defined(sol) || defined(__MACH__)
      SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT, SIGBUS,
      SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGXCPU, SIGXFSZ,
#elif defined(CSWIN)
      SIGHUP, SIGINT, SIGQUIT,
#elif defined(WIN32)
      SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM,
#elif defined(__EMX__)
      SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE,
      SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD,
#endif
      -1
    };
    for (x = sigs; *x > 0; x++)
      signal(*x, signal_handler);
  }

  static int getTimeResolution(void);

  PUBLIC int csoundInitialize(int *argc, char ***argv)
  {
    int n;
    do {
      csoundLock();
      n = init_done;
      switch (n) {
        case 2:
          csoundUnLock();
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
          usleep(1000);
#elif defined(WIN32)
          Sleep(1);
#endif
        case 0:
          break;
        default:
          csoundUnLock();
          return (n >= 0 ? 0 : -1);
      }
    } while (n);
    init_done = 2;
    csoundUnLock();
    init_getstring(*argc, *argv);
    if (getTimeResolution() != 0) {
      csoundLock(); init_done = -1; csoundUnLock();
      return -1;
    }
    install_signal_handler();
    atexit(destroy_all_instances);
    csoundLock(); init_done = 1; csoundUnLock();
    return 0;
  }

  PUBLIC void *csoundCreate(void *hostdata)
  {
    ENVIRON       *csound;
    csInstance_t  *p;
    if (init_done != 1) {
      int argc = 1;
      char *argv_[2] = { "csound", NULL };
      char **argv = &(argv_[0]);
      if (csoundInitialize(&argc, &argv) != 0)
        return NULL;
    }
    csound = (ENVIRON*) malloc(sizeof(ENVIRON));
    if (csound == NULL)
      return NULL;
    memcpy(csound, &cenviron_, sizeof(ENVIRON));
    csound->oparms = (OPARMS*) malloc(sizeof(OPARMS));
    if (csound->oparms == NULL) {
      free(csound);
      return NULL;
    }
    memset(csound->oparms, 0, sizeof(OPARMS));
    csound->hostdata = hostdata;
    p = (csInstance_t*) malloc(sizeof(csInstance_t));
    if (p == NULL) {
      free(csound->oparms);
      free(csound);
      return NULL;
    }
    csoundLock();
    p->csound = csound;
    p->nxt = instance_list;
    instance_list = p;
    csoundUnLock();
    csoundReset(csound);
    return (void*) csound;
  }

  /* dummy real time MIDI functions */
  static int DummyMidiInOpen(void *csound, void **userData,
                             const char *devName);
  static int DummyMidiRead(void *csound, void *userData,
                           unsigned char *buf, int nbytes);
  static int DummyMidiInClose(void *csound, void *userData);
  static int DummyMidiOutOpen(void *csound, void **userData,
                              const char *devName);
  static int DummyMidiWrite(void *csound, void *userData,
                            unsigned char *buf, int nbytes);
  static int DummyMidiOutClose(void *csound, void *userData);
  static char *DummyMidiErrorString(int errcode);

  static  const   char    *id_option_table[][3] = {
      { "::SF::id_title", "id_title",
        "Title tag in output soundfile (no spaces)" },
      { "::SF::id_copyright", "id_copyright",
        "Copyright tag in output soundfile (no spaces)" },
      { "::SF::id_software", "id_software",
        "Software tag in output soundfile (no spaces)" },
      { "::SF::id_artist", "id_artist",
        "Artist tag in output soundfile (no spaces)" },
      { "::SF::id_comment", "id_comment",
        "Comment tag in output soundfile (no spaces)" },
      { "::SF::id_date", "id_date",
        "Date tag in output soundfile (no spaces)" },
      { NULL, NULL, NULL }
  };

  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  PUBLIC int csoundPreCompile(void *csound)
  {
    ENVIRON *p;
    void    *saved_hostdata;
    char    *s;
    int     i, max_len;

    p = (ENVIRON*) csound;
    /* reset instance, but keep host data pointer */
    saved_hostdata = p->hostdata;
    csoundReset(csound);
    p->hostdata = saved_hostdata;
    /* copy system environment variables */
    i = csoundInitEnv(csound);
    if (i != CSOUND_SUCCESS)
      return i;
    /* allow selecting real time audio module */
    max_len = 21;
    csoundCreateGlobalVariable(csound, "_RTAUDIO", (size_t) max_len);
    s = csoundQueryGlobalVariable(csound, "_RTAUDIO");
    strcpy(s, "PortAudio");
    csoundCreateConfigurationVariable(csound, "rtaudio", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Real time audio module name", NULL);
    /* initialise real time MIDI */
    p->midiGlobals = (MGLOBAL*) mcalloc(csound, sizeof(MGLOBAL));
    p->midiGlobals->Midevtblk = (MEVENT*) NULL;
    csoundSetExternalMidiInOpenCallback(csound, DummyMidiInOpen);
    csoundSetExternalMidiReadCallback(csound, DummyMidiRead);
    csoundSetExternalMidiInCloseCallback(csound, DummyMidiInClose);
    csoundSetExternalMidiOutOpenCallback(csound, DummyMidiOutOpen);
    csoundSetExternalMidiWriteCallback(csound, DummyMidiWrite);
    csoundSetExternalMidiOutCloseCallback(csound, DummyMidiOutClose);
    csoundSetExternalMidiErrorStringCallback(csound, DummyMidiErrorString);
    p->midiGlobals->midiInUserData = NULL;
    p->midiGlobals->midiOutUserData = NULL;
    p->midiGlobals->midiFileData = NULL;
    p->midiGlobals->bufp = &(p->midiGlobals->mbuf[0]);
    p->midiGlobals->endatp = p->midiGlobals->bufp;
    csoundCreateGlobalVariable(csound, "_RTMIDI", (size_t) max_len);
    s = csoundQueryGlobalVariable(csound, "_RTMIDI");
    strcpy(s, "PortMIDI");
    csoundCreateConfigurationVariable(csound, "rtmidi", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Real time MIDI module name", NULL);
    max_len = 256;  /* should be the same as in csoundCore.h */
    csoundCreateConfigurationVariable(csound, "mute_tracks",
                                      &(p->midiGlobals->muteTrackList[0]),
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Ignore events (other than tempo "
                                      "changes) in tracks defined by pattern",
                                      NULL);
    csoundCreateConfigurationVariable(csound, "raw_controller_mode",
                                      &(p->midiGlobals->rawControllerMode),
                                      CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                      "Do not handle special MIDI controllers "
                                      "(sustain pedal etc.)", NULL);
    /* sound file tag options */
    max_len = 201;
    i = -1;
    while (id_option_table[++i][0] != NULL) {
      csoundCreateGlobalVariable(csound, id_option_table[i][0],
                                         (size_t) max_len);
      csoundCreateConfigurationVariable(
                    csound, id_option_table[i][1],
                    csoundQueryGlobalVariable(csound, id_option_table[i][0]),
                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                    id_option_table[i][2], NULL);
    }
    /* max. length of string variables */
    {
      int minVal = 10;
      int maxVal = 10000;
      csoundCreateConfigurationVariable(csound, "max_str_len",
                                        &(p->strVarMaxLen),
                                        CSOUNDCFG_INTEGER, 0, &minVal, &maxVal,
                                        "Maximum length of "
                                        "string variables + 1", NULL);
    }
    csoundCreateConfigurationVariable(csound, "msg_color", &(p->enableMsgAttr),
                                      CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                      "Enable message attributes (colors etc.)",
                                      NULL);
    /* now load and pre-initialise external modules for this instance */
    /* this function returns an error value that may be worth checking */
    return csoundLoadModules(csound);
  }

  PUBLIC int csoundQueryInterface(const char *name, void **iface, int *version)
  {
    if(strcmp(name, "CSOUND") == 0)
      {
        *iface = csoundCreate(0);
        *version = csoundGetVersion();
        return 0;
      }
    return 1;
  }

  PUBLIC void csoundDestroy(void *csound)
  {
    csInstance_t  *p, *prv = NULL;
    csoundLock();
    p = instance_list;
    while (p != NULL && p->csound != (ENVIRON*) csound) {
      prv = p; p = p->nxt;
    }
    if (p == NULL) {
      csoundUnLock();
      return;
    }
    if (prv == NULL)
      instance_list = p->nxt;
    else
      prv->nxt = p->nxt;
    csoundUnLock();
    free(p);
    csoundReset(csound);
    free(((ENVIRON*) csound)->oparms);
    free(csound);
  }

  PUBLIC int csoundGetVersion()
  {
    return (int) (atof(PACKAGE_VERSION) * 100);
  }

  int csoundGetAPIVersion(void)
  {
    return APIVERSION * 100 + APISUBVER;
  }

  PUBLIC void *csoundGetHostData(void *csound)
  {
    return ((ENVIRON *)csound)->hostdata;
  }

  PUBLIC void csoundSetHostData(void *csound, void *hostData)
  {
    ((ENVIRON *)csound)->hostdata = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int sensevents(ENVIRON *);

  PUBLIC int csoundPerform(void *csound, int argc, char **argv)
  {
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerform().\n");
      return returnValue;
    }
    return csoundMain(csound, argc, argv);
  }

  PUBLIC int csoundPerformKsmps(void *csound)
  {
    int done = 0;
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
      return returnValue;
    }
    done = sensevents(csound);
    if (!done) {
      /* IV - Feb 05 2005 */
      if (!((ENVIRON*) csound)->oparms->initonly)
        kperf(csound);
    }
    else {
      csoundMessage(csound, "Score finished in csoundPerformKsmps()\n");
    }
    return done;
  }

  PUBLIC int csoundPerformKsmpsAbsolute(void *csound)
  {
    int done = 0;
    volatile int returnValue;
    /* setup jmp for return after an exit()
     */
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
      return returnValue;
    }
    done = sensevents(csound);

    /* IV - Feb 05 2005 */
    if (!((ENVIRON*) csound)->oparms->initonly)
      kperf(csound);
    return done;
  }

  /* external host's outbuffer passed in csoundPerformBuffer() */

  PUBLIC int csoundPerformBuffer(void *csound_)
  {
    ENVIRON *csound = (ENVIRON*) csound_;
    volatile int returnValue;
    int     done;
    /* Setup jmp for return after an exit(). */
    if ((returnValue = setjmp(csound->exitjmp))) {
      csoundMessage(csound_, "Early return from csoundPerformBuffer().\n");
      return returnValue;
    }
    csound->sampsNeeded += csound->oparms->outbufsamps;
    while (csound->sampsNeeded > 0) {
      if ((done = sensevents(csound)))
        return done;
      if (!csound->oparms->initonly)
        kperf(csound);
      csound->sampsNeeded -= csound->nspout;
    }
    return 0;
  }

  /*
   * ATTRIBUTES
   */

  PUBLIC MYFLT csoundGetSr(void *csound)
  {
    return ((ENVIRON *)csound)->esr;
  }

  PUBLIC MYFLT csoundGetKr(void *csound)
  {
    return ((ENVIRON *)csound)->ekr;
  }

  PUBLIC int csoundGetKsmps(void *csound)
  {
    return ((ENVIRON *)csound)->ksmps;
  }

  PUBLIC int csoundGetNchnls(void *csound)
  {
    return ((ENVIRON *)csound)->nchnls;
  }

  PUBLIC int csoundGetSampleFormat(void *csound)
  {
    /* should we assume input is same as output ? */
    return ((ENVIRON*) csound)->oparms->outformat;
  }

  PUBLIC int csoundGetSampleSize(void *csound)
  {
    /* should we assume input is same as output ? */
    return ((ENVIRON*) csound)->oparms->sfsampsize;
  }

  PUBLIC long csoundGetInputBufferSize(void *csound)
  {
    return ((ENVIRON*) csound)->oparms->inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(void *csound)
  {
    return ((ENVIRON*) csound)->oparms->outbufsamps;
  }

  PUBLIC MYFLT* csoundGetSpin(void *csound)
  {
    return ((ENVIRON *)csound)->spin;
  }

  PUBLIC MYFLT* csoundGetSpout(void *csound)
  {
    return ((ENVIRON *)csound)->spout;
  }

  PUBLIC MYFLT csoundGetScoreTime(void *csound)
  {
    return (MYFLT) ((ENVIRON*)csound)->sensEvents_state.curTime;
  }

  PUBLIC MYFLT csoundGetProgress(void *csound)
  {
    return -1;
  }

  PUBLIC MYFLT csoundGetProfile(void *csound)
  {
    return -1;
  }

  PUBLIC MYFLT csoundGetCpuUsage(void *csound)
  {
    return -1;
  }

  /*
   * SCORE HANDLING
   */

  PUBLIC int csoundIsScorePending(void *csound)
  {
    return ((ENVIRON*) csound)->csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(void *csound, int pending)
  {
    ((ENVIRON*) csound)->csoundIsScorePending_ = pending;
  }

  PUBLIC void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset)
  {
    ((ENVIRON*) csound)->csoundScoreOffsetSeconds_ = offset;
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(void *csound)
  {
    return ((ENVIRON*) csound)->csoundScoreOffsetSeconds_;
  }

  PUBLIC void csoundRewindScore(void *csound)
  {
    if(((ENVIRON *)csound)->scfp)
      {
        fseek(((ENVIRON *)csound)->scfp, 0, SEEK_SET);
      }
  }

  void csoundDefaultMessageCallback(void *csound, int attr,
                                    const char *format, va_list args)
  {
#if defined(WIN32) || defined(mills_macintosh)
    switch (attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_ERROR:
      case CSOUNDMSG_WARNING:
      case CSOUNDMSG_REALTIME:
        vfprintf(stderr, format, args);
        break;
      default:
        vfprintf(stdout, format, args);
    }
#else
    if (!attr || !((ENVIRON*) csound)->enableMsgAttr) {
      vfprintf(stderr, format, args);
      return;
    }
    if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ORCH)
      attr |= CSOUNDMSG_FG_BOLD;
    if (attr & CSOUNDMSG_BG_COLOR_MASK)
      fprintf(stderr, "\033[4%cm", ((attr & 0x70) >> 4) + '0');
    if (attr & CSOUNDMSG_FG_ATTR_MASK) {
      if (attr & CSOUNDMSG_FG_BOLD)
        fprintf(stderr, "\033[1m");
      if (attr & CSOUNDMSG_FG_UNDERLINE)
        fprintf(stderr, "\033[4m");
    }
    if (attr & CSOUNDMSG_FG_COLOR_MASK)
      fprintf(stderr, "\033[3%cm", (attr & 7) + '0');
    vfprintf(stderr, format, args);
    fprintf(stderr, "\033[m");
#endif
  }

  void csoundDefaultThrowMessageCallback(void *csound, const char *format,
                                                       va_list args)
  {
    csoundDefaultMessageCallback(csound, CSOUNDMSG_ERROR, format, args);
  }

  PUBLIC void csoundSetMessageCallback(void *csound,
                            void (*csoundMessageCallback)(void *csound,
                                                          int attr,
                                                          const char *format,
                                                          va_list args))
  {
    ((ENVIRON*) csound)->csoundMessageCallback_ = csoundMessageCallback;
  }

  PUBLIC void csoundMessageV(void *csound, int attr,
                             const char *format, va_list args)
  {
    ((ENVIRON*) csound)->csoundMessageCallback_(csound, attr, format, args);
  }

  PUBLIC void csoundMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    ((ENVIRON*) csound)->csoundMessageCallback_(csound, 0, format, args);
    va_end(args);
  }

  PUBLIC void csoundMessageS(void *csound, int attr, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    ((ENVIRON*) csound)->csoundMessageCallback_(csound, attr, format, args);
    va_end(args);
  }

  void csoundPrintf(const char *format, ...)
  {
    fprintf(stderr, " *** error: use csound->Message() instead of printf()\n");
    exit(-1);
  }

  void csoundDie(void *csound, const char *msg, ...)
  {
    ENVIRON *p = (ENVIRON*) csound;
    va_list args;
    va_start(args, msg);
    p->csoundMessageCallback_(p, CSOUNDMSG_ERROR, msg, args);
    va_end(args);
    csoundMessageS(p, CSOUNDMSG_ERROR, "\n");
    longjmp(p->exitjmp, 1);
  }

  void csoundWarning(void *csound, const char *msg, ...)
  {
    ENVIRON *p = (ENVIRON*) csound;
    va_list args;
    if (!(p->oparms->msglevel & WARNMSG))
      return;
    csoundMessageS(p, CSOUNDMSG_WARNING, Str("WARNING: "));
    va_start(args, msg);
    p->csoundMessageCallback_(p, CSOUNDMSG_WARNING, msg, args);
    va_end(args);
    csoundMessageS(p, CSOUNDMSG_WARNING, "\n");
  }

  void csoundDebugMsg(void *csound, const char *msg, ...)
  {
    va_list args;
    if (!(((ENVIRON*) csound)->oparms->odebug))
      return;
    va_start(args, msg);
    ((ENVIRON*) csound)->csoundMessageCallback_(csound, 0, msg, args);
    va_end(args);
    csoundMessage(csound, "\n");
  }

  PUBLIC void csoundSetThrowMessageCallback(void *csound,
                    void (*csoundThrowMessageCallback)(void *csound,
                                                       const char *format,
                                                       va_list args))
  {
    ((ENVIRON*) csound)->csoundThrowMessageCallback_ =
      csoundThrowMessageCallback;
  }

  PUBLIC void csoundThrowMessageV(void *csound, const char *format,
                                                va_list args)
  {
    ((ENVIRON*) csound)->csoundThrowMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundThrowMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    ((ENVIRON*) csound)->csoundThrowMessageCallback_(csound, format, args);
    va_end(args);
  }

  PUBLIC int csoundGetMessageLevel(void *csound)
  {
    return ((ENVIRON *)csound)->oparms->msglevel;
  }

  PUBLIC void csoundSetMessageLevel(void *csound, int messageLevel)
  {
    ((ENVIRON *)csound)->oparms->msglevel = messageLevel;
  }

  PUBLIC void csoundInputMessage(void *csound, const char *message)
  {
    writeLine((ENVIRON*) csound, message, strlen(message));
  }

  PUBLIC void csoundKeyPress(void *csound, char c)
  {
    ((ENVIRON*) csound)->inChar_ = (int) ((unsigned char) c);
  }

  char getChar(void *csound)
  {
    return (char) ((ENVIRON*)csound)->inChar_;
  }

  /*
   * CONTROL AND EVENTS
   */

  PUBLIC void
    csoundSetInputValueCallback(void *csound,
                                void (*inputValueCalback)(void *csound,
                                                          char *channelName,
                                                          MYFLT *value))
  {
    ((ENVIRON*) csound)->InputValueCallback_ = inputValueCalback;
  }

  PUBLIC void
    csoundSetOutputValueCallback(void *csound,
                                 void (*outputValueCalback)(void *csound,
                                                            char *channelName,
                                                            MYFLT value))
  {
    ((ENVIRON*) csound)->OutputValueCallback_ = outputValueCalback;
  }

  void InputValue(ENVIRON *csound, char *channelName, MYFLT *value)
  {
    if (csound->InputValueCallback_)
      csound->InputValueCallback_(csound, channelName, value);
    else
      *value = FL(0.0);
  }

  void OutputValue(ENVIRON *csound, char *channelName, MYFLT value)
  {
    if (csound->OutputValueCallback_) {
      csound->OutputValueCallback_(csound, channelName, value);
    }
  }

  PUBLIC int csoundScoreEvent(void *csound, char type,
                              MYFLT *pfields, long numFields)
  {
    EVTBLK        evt;
    int           i;

    evt.strarg = NULL;
    evt.opcod = type;
    evt.pcnt = (short) numFields;
    for (i = 0; i < (int) numFields; i++)
      evt.p[i + 1] = pfields[i];
    return
      insert_score_event((ENVIRON*) csound, &evt,
                         ((ENVIRON*) csound)->sensEvents_state.curTime, 0);
  }

  /*
   *    REAL-TIME AUDIO
   */

/* dummy functions for the case when no real-time audio module is available */

#ifdef LINUX
#include <sched.h>
#endif

int playopen_dummy(void *csound, csRtAudioParams *parm)
{
    char *s;

    parm = parm;
    /* find out if the use of dummy real-time audio functions was requested, */
    /* or an unknown plugin name was specified; the latter case is an error */
    s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
    if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                       strcmp(s, "NULL") == 0)) {
      if (s[0] == '\0')
        csoundMessage(csound,
                      Str(" *** error: rtaudio module set to empty string\n"));
      else
        csoundMessage(csound,
                      Str(" *** error: unknown rtaudio module: '%s'\n"), s);
      return CSOUND_ERROR;
    }
    /* IV - Feb 08 2005: avoid locking up the system with --sched */
#ifdef LINUX
    if (sched_getscheduler(0) != SCHED_OTHER) {
      csoundMessage(csound,
                    " *** error: cannot use --sched with dummy audio output\n");
      return CSOUND_ERROR;
    }
#endif
    return CSOUND_SUCCESS;
}

void rtplay_dummy(void *csound, void *outBuf, int nbytes)
{
    csound = csound; outBuf = outBuf; nbytes = nbytes;
}

int recopen_dummy(void *csound, csRtAudioParams *parm)
{
    char *s;

    parm = parm;
    /* find out if the use of dummy real-time audio functions was requested, */
    /* or an unknown plugin name was specified; the latter case is an error */
    s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
    if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                       strcmp(s, "NULL") == 0)) {
      if (s[0] == '\0')
        csoundMessage(csound,
                      Str(" *** error: rtaudio module set to empty string\n"));
      else
        csoundMessage(csound,
                      Str(" *** error: unknown rtaudio module: '%s'\n"), s);
      return CSOUND_ERROR;
    }
    /* IV - Feb 08 2005: avoid locking up the system with --sched */
#ifdef LINUX
    if (sched_getscheduler(0) != SCHED_OTHER) {
      csoundMessage(csound,
                    " *** error: cannot use --sched with dummy audio input\n");
      return CSOUND_ERROR;
    }
#endif
    return CSOUND_SUCCESS;
}

int rtrecord_dummy(void *csound, void *inBuf, int nbytes)
{
    int i;
    csound = csound;
    for (i = 0; i < (nbytes / (int) sizeof(MYFLT)); i++)
      ((MYFLT*) inBuf)[i] = FL(0.0);
    return nbytes;
}

void rtclose_dummy(void *csound)
{
    csound = csound;
}

PUBLIC void csoundSetPlayopenCallback(void *csound,
                                      int (*playopen__)(void *csound,
                                                        csRtAudioParams *parm))
{
    ((ENVIRON*) csound)->playopen_callback = playopen__;
}

PUBLIC void csoundSetRtplayCallback(void *csound,
                                    void (*rtplay__)(void *csound,
                                                     void *outBuf, int nbytes))
{
    ((ENVIRON*) csound)->rtplay_callback = rtplay__;
}

PUBLIC void csoundSetRecopenCallback(void *csound,
                                     int (*recopen__)(void *csound,
                                                      csRtAudioParams *parm))
{
    ((ENVIRON*) csound)->recopen_callback = recopen__;
}

PUBLIC void csoundSetRtrecordCallback(void *csound,
                                      int (*rtrecord__)(void *csound,
                                                        void *inBuf,
                                                        int nbytes))
{
    ((ENVIRON*) csound)->rtrecord_callback = rtrecord__;
}

PUBLIC void csoundSetRtcloseCallback(void *csound,
                                     void (*rtclose__)(void *csound))
{
    ((ENVIRON*) csound)->rtclose_callback = rtclose__;
}

/* dummy real time MIDI functions */

static int DummyMidiInOpen(void *csound, void **userData, const char *devName)
{
    char *s;

    *userData = NULL;
    s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (s == NULL ||
        (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
         strcmp(s, "NULL") == 0)) {
      csoundMessage(csound, "WARNING: real time midi input disabled, "
                            "using dummy functions\n");
      return 0;
    }
    if (s[0] == '\0')
      csoundMessage(csound, "error: -+rtmidi set to empty string\n");
    else
      csoundMessage(csound, "error: -+rtmidi='%s': unknown module\n", s);
    return -1;
}

static int DummyMidiRead(void *csound, void *userData,
                         unsigned char *buf, int nbytes)
{
    return 0;
}

static int DummyMidiInClose(void *csound, void *userData)
{
    return 0;
}

static int DummyMidiOutOpen(void *csound, void **userData, const char *devName)
{
    char *s;

    *userData = NULL;
    s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (s == NULL ||
        (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
         strcmp(s, "NULL") == 0)) {
      csoundMessage(csound, "WARNING: real time midi output disabled, "
                            "using dummy functions\n");
      return 0;
    }
    if (s[0] == '\0')
      csoundMessage(csound, "error: -+rtmidi set to empty string\n");
    else
      csoundMessage(csound, "error: -+rtmidi='%s': unknown module\n", s);
    return -1;
}

static int DummyMidiWrite(void *csound, void *userData,
                          unsigned char *buf, int nbytes)
{
    return nbytes;
}

static int DummyMidiOutClose(void *csound, void *userData)
{
    return 0;
}

static const char *midi_err_msg = "Unknown MIDI error";

static char *DummyMidiErrorString(int errcode)
{
    return (char*) midi_err_msg;
}

/**
 * Open MIDI input device 'devName', and store stream specific
 * data pointer in *userData. Return value is zero on success,
 * and a non-zero error code if an error occured.
 */
int csoundExternalMidiInOpen(void *csound, void **userData,
                             const char *devName)
{
    if (((ENVIRON*) csound)->midiGlobals->MidiInOpenCallback == NULL)
      return -1;
    return (((ENVIRON*) csound)->midiGlobals->MidiInOpenCallback(csound,
                                                                 userData,
                                                                 devName));
}

/**
 * Read at most 'nbytes' bytes of MIDI data from input stream
 * 'userData', and store in 'buf'. Returns the actual number of
 * bytes read, which may be zero if there were no events, and
 * negative in case of an error. Note: incomplete messages (such
 * as a note on status without the data bytes) should not be
 * returned.
 */
int csoundExternalMidiRead(void *csound, void *userData,
                           unsigned char *buf, int nbytes)
{
    if (((ENVIRON*) csound)->midiGlobals->MidiReadCallback == NULL)
      return -1;
    return (((ENVIRON*) csound)->midiGlobals->MidiReadCallback(csound,
                                                               userData,
                                                               buf, nbytes));
}

/**
 * Close MIDI input device associated with 'userData'.
 * Return value is zero on success, and a non-zero error
 * code on failure.
 */
int csoundExternalMidiInClose(void *csound, void *userData)
{
    int retval;
    if (((ENVIRON*) csound)->midiGlobals->MidiInCloseCallback == NULL)
      return -1;
    retval = ((ENVIRON*) csound)->midiGlobals->MidiInCloseCallback(csound,
                                                                   userData);
    ((ENVIRON*) csound)->midiGlobals->midiInUserData = NULL;
    return retval;
}

/**
 * Open MIDI output device 'devName', and store stream specific
 * data pointer in *userData. Return value is zero on success,
 * and a non-zero error code if an error occured.
 */
int csoundExternalMidiOutOpen(void *csound, void **userData,
                              const char *devName)
{
    if (((ENVIRON*) csound)->midiGlobals->MidiOutOpenCallback == NULL)
      return -1;
    return (((ENVIRON*) csound)->midiGlobals->MidiOutOpenCallback(csound,
                                                                  userData,
                                                                  devName));
}

/**
 * Write 'nbytes' bytes of MIDI data to output stream 'userData'
 * from 'buf' (the buffer will not contain incomplete messages).
 * Returns the actual number of bytes written, or a negative
 * error code.
 */
int csoundExternalMidiWrite(void *csound, void *userData,
                            unsigned char *buf, int nbytes)
{
    if (((ENVIRON*) csound)->midiGlobals->MidiWriteCallback == NULL)
      return -1;
    return (((ENVIRON*) csound)->midiGlobals->MidiWriteCallback(csound,
                                                                userData,
                                                                buf, nbytes));
}

/**
 * Close MIDI output device associated with '*userData'.
 * Return value is zero on success, and a non-zero error
 * code on failure.
 */
int csoundExternalMidiOutClose(void *csound, void *userData)
{
    int retval;
    if (((ENVIRON*) csound)->midiGlobals->MidiOutCloseCallback == NULL)
      return -1;
    retval = ((ENVIRON*) csound)->midiGlobals->MidiOutCloseCallback(csound,
                                                                    userData);
    ((ENVIRON*) csound)->midiGlobals->midiOutUserData = NULL;
    return retval;
}

/**
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
char *csoundExternalMidiErrorString(void *csound, int errcode)
{
    if (((ENVIRON*) csound)->midiGlobals->MidiErrorStringCallback == NULL)
      return NULL;
    return (((ENVIRON*) csound)->midiGlobals->MidiErrorStringCallback(errcode));
}

/* Set real time MIDI function pointers. */

PUBLIC void csoundSetExternalMidiInOpenCallback(void *csound,
                                                int (*func)(void*, void**,
                                                            const char*))
{
    ((ENVIRON*) csound)->midiGlobals->MidiInOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiReadCallback(void *csound,
                                              int (*func)(void*, void*,
                                                          unsigned char*, int))
{
    ((ENVIRON*) csound)->midiGlobals->MidiReadCallback = func;
}

PUBLIC void csoundSetExternalMidiInCloseCallback(void *csound,
                                                 int (*func)(void*, void*))
{
    ((ENVIRON*) csound)->midiGlobals->MidiInCloseCallback = func;
}

PUBLIC void csoundSetExternalMidiOutOpenCallback(void *csound,
                                                 int (*func)(void*, void**,
                                                             const char*))
{
    ((ENVIRON*) csound)->midiGlobals->MidiOutOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiWriteCallback(void *csound,
                                               int (*func)(void*, void*,
                                                           unsigned char*, int))
{
    ((ENVIRON*) csound)->midiGlobals->MidiWriteCallback = func;
}

PUBLIC void csoundSetExternalMidiOutCloseCallback(void *csound,
                                                  int (*func)(void*, void*))
{
    ((ENVIRON*) csound)->midiGlobals->MidiOutCloseCallback = func;
}

PUBLIC void csoundSetExternalMidiErrorStringCallback(void *csound,
                                                     char *(*func)(int))
{
    ((ENVIRON*) csound)->midiGlobals->MidiErrorStringCallback = func;
}

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  PUBLIC void csoundSetIsGraphable(void *csound, int isGraphable)
  {
    ((ENVIRON*) csound)->isGraphable_ = isGraphable;
  }

  int Graphable(void *csound)
  {
    return ((ENVIRON*) csound)->isGraphable_;
  }

  void defaultCsoundMakeGraph(void *csound, WINDAT *windat, char *name)
  {
#if defined(USE_FLTK)
    extern void MakeGraph_(void *, WINDAT *, char *);
    MakeGraph_(csound, windat, name);
#else
    extern void MakeAscii(void *, WINDAT *, char *);
    MakeAscii(csound, windat, name);
#endif
  }

  PUBLIC void csoundSetMakeGraphCallback(void *csound,
                                    void (*makeGraphCallback)(void *csound,
                                                              WINDAT *windat,
                                                              char *name))
  {
    ((ENVIRON*) csound)->csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(void *csound, WINDAT *windat, char *name)
  {
    ((ENVIRON*) csound)->csoundMakeGraphCallback_(csound, windat, name);
  }

  void defaultCsoundDrawGraph(void *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void DrawGraph_(void *, WINDAT *);
    DrawGraph_(csound, windat);
#else
    extern void DrawAscii(void *, WINDAT *);
    DrawAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetDrawGraphCallback(void *csound,
                                    void (*drawGraphCallback)(void *csound,
                                                              WINDAT *windat))
  {
    ((ENVIRON*) csound)->csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(void *csound, WINDAT *windat)
  {
    ((ENVIRON*) csound)->csoundDrawGraphCallback_(csound, windat);
  }

  void defaultCsoundKillGraph(void *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void KillGraph_(void *, WINDAT *);
    KillGraph_(csound, windat);
#else
    extern void KillAscii(void *, WINDAT *);
    KillAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetKillGraphCallback(void *csound,
                                    void (*killGraphCallback)(void *csound,
                                                              WINDAT *windat))
  {
    ((ENVIRON*) csound)->csoundKillGraphCallback_ = killGraphCallback;
  }

  void KillGraph(void *csound, WINDAT *windat)
  {
    ((ENVIRON*) csound)->csoundKillGraphCallback_(csound, windat);
  }

  int defaultCsoundExitGraph(void *csound)
  {
    return CSOUND_SUCCESS;
  }

  PUBLIC void csoundSetExitGraphCallback(void *csound,
                                         int (*exitGraphCallback)(void *csound))
  {
    ((ENVIRON*) csound)->csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph(void *csound)
  {
    return ((ENVIRON*) csound)->csoundExitGraphCallback_(csound);
  }

  void MakeXYin(void *csound, XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
    csoundMessage(csound, "xyin not supported. use invalue opcode instead.\n");
  }

  void ReadXYin(void *csound, XYINDAT *xyindat)
  {
    csoundMessage(csound, "xyin not supported. use invlaue opcodes instead.\n");
  }

  /*
   * OPCODES
   */

  PUBLIC opcodelist *csoundNewOpcodeList()
  {
    /* create_opcodlst();
       return new_opcode_list(); */

    return NULL;
  }

  PUBLIC void csoundDisposeOpcodeList(opcodelist *list)
  {
    /* dispose_opcode_list(list); */
  }

  PUBLIC int csoundAppendOpcode(void *csound_,
                                char *opname,
                                int dsblksiz,
                                int thread,
                                char *outypes,
                                char *intypes,
                                int (*iopadr)(void *, void *),
                                int (*kopadr)(void *, void *),
                                int (*aopadr)(void *, void *),
                                int (*dopadr)(void *, void *))
  {
    ENVIRON *csound = (ENVIRON*) csound_;
    int oldSize = (int) ((char*) csound->oplstend - (char*) csound->opcodlst);
    int newSize = oldSize + sizeof(OENTRY);
    int oldCount = oldSize / sizeof(OENTRY);
    int newCount = oldCount + 1;
    OENTRY *oldOpcodlst = csound->opcodlst;
    csound->opcodlst = (OENTRY*) mrealloc(csound, csound->opcodlst, newSize);
    if (!csound->opcodlst) {
      csound->opcodlst = oldOpcodlst;
      csoundMessage(csound, "Failed to allocate new opcode entry.\n");
      return -1;
    }
    else {
      OENTRY *oentry = csound->opcodlst + oldCount;
      csound->oplstend = csound->opcodlst + newCount;
      oentry->opname = opname;
      oentry->dsblksiz = dsblksiz;
      oentry->thread = thread;
      oentry->outypes = outypes;
      oentry->intypes = intypes;
      oentry->iopadr = (SUBR) iopadr;
      oentry->kopadr = (SUBR) kopadr;
      oentry->aopadr = (SUBR) aopadr;
      oentry->dopadr = (SUBR) dopadr;
      return 0;
    }
  }

  int csoundOpcodeCompare(const void *v1, const void *v2)
  {
    return strcmp(((OENTRY*)v1)->opname, ((OENTRY*)v2)->opname);
  }

  void csoundOpcodeDeinitialize(void *csound, INSDS *ip_)
  {
    INSDS *ip = ip_;
    OPDS *pds;
    while((ip = (INSDS *)ip->nxti))
      {
        pds = (OPDS *)ip;
        while((pds = pds->nxti))
          {
            if(pds->dopadr)
              {
                (*pds->dopadr)(csound,pds);
              }
          }
      }
    ip = ip_;
    while((ip = (INSDS *)ip->nxtp))
      {
        pds = (OPDS *)ip;
        while((pds = pds->nxtp))
          {
            if(pds->dopadr)
              {
                (*pds->dopadr)(csound,pds);
              }
          }
      }
  }

  /*
   * MISC FUNCTIONS
   */

#if !defined(USE_FLTK)
  int POLL_EVENTS(ENVIRON *csound)
  {
    return 1;
  }
#else
  extern int POLL_EVENTS(ENVIRON *csound);
#endif

  int defaultCsoundYield(void *csound)
  {
    return POLL_EVENTS((ENVIRON*) csound);
  }

  void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound))
  {
    ((ENVIRON*) csound)->csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(void *csound)
  {
    return ((ENVIRON*) csound)->csoundYieldCallback_(csound);
  }

  extern void csoundDeleteAllGlobalVariables(void *csound);

  PUBLIC void csoundReset(void *csound)
  {
    csoundCleanup(csound);
    /* call local destructor routines of external modules */
    /* should check return value... */
    csoundDestroyModules(csound);
    /* IV - Feb 01 2005: clean up configuration variables and */
    /* named dynamic "global" variables of Csound instance */
    csoundDeleteAllConfigurationVariables(csound);
    csoundDeleteAllGlobalVariables(csound);
    mainRESET(csound);
  }

  PUBLIC int csoundGetDebug(void *csound)
  {
    return ((ENVIRON*) csound)->oparms->odebug;
  }

  PUBLIC void csoundSetDebug(void *csound, int debug)
  {
    ((ENVIRON*) csound)->oparms->odebug = debug;
  }

  PUBLIC int csoundTableLength(void *csound, int table)
  {
    int tableLength;
    ((ENVIRON*) csound)->GetTable((ENVIRON*) csound, table, &tableLength);
    return tableLength;
  }

  MYFLT csoundTableGet(void *csound, int table, int index)
  {
    return ((ENVIRON*) csound)->GetTable((ENVIRON*) csound, table, NULL)[index];
  }

  PUBLIC void csoundTableSet(void *csound, int table, int index, MYFLT value)
  {
    ((ENVIRON*) csound)->GetTable((ENVIRON*)csound, table, NULL)[index] = value;
  }

  PUBLIC void csoundSetFLTKThreadLocking(void *csound, int isLocking)
  {
    ((ENVIRON*) csound)->doFLTKThreadLocking = isLocking;
  }

  PUBLIC int csoundGetFLTKThreadLocking(void *csound)
  {
    return ((ENVIRON*) csound)->doFLTKThreadLocking;
  }

/* -------- IV - Jan 27 2005: timer functions -------- */

#if defined(WIN32)
/* do not use UNIX code under Win32 */
#ifdef __unix
#undef __unix
#endif
#elif defined(__unix) || defined(SGI) || defined(LINUX)
#include <sys/time.h>
#endif

/* enable use of high resolution timer (Linux/i586/GCC only) */
/* could in fact work under any x86/GCC system, but do not   */
/* know how to query the actual CPU frequency ...            */

#define HAVE_RDTSC  1

/* ------------------------------------ */

#if defined(HAVE_RDTSC)
#if !(defined(LINUX) && defined(__GNUC__) && defined(__i386__))
#undef HAVE_RDTSC
#endif
#endif

/* hopefully cannot change during performance */
static double timeResolutionSeconds = -1.0;

/* find out CPU frequency based on /proc/cpuinfo */

static int getTimeResolution(void)
{
#if defined(HAVE_RDTSC)
    FILE    *f;
    char    buf[256];

    /* if frequency is not known yet */
    f = fopen("/proc/cpuinfo", "r");
    if (f == NULL) {
      fprintf(stderr, "Cannot open /proc/cpuinfo. "
                      "Support for RDTSC is not available.\n");
      return -1;
    }
    /* find CPU frequency */
    while (fgets(buf, 256, f) != NULL) {
      int     i;
      char    *s = (char*) buf - 1;

      buf[255] = '\0';          /* safety */
      if (strlen(buf) < 9)
        continue;                       /* too short, skip */
      while (*++s != '\0')
        if (isupper(*s))
          *s = tolower(*s);             /* convert to lower case */
      if (strncmp(buf, "cpu mhz", 7) != 0)
        continue;                       /* check key name */
      s = strchr(buf, ':');             /* find frequency value */
      if (s == NULL) continue;              /* invalid entry */
      do {
        s++;
      } while (*s == ' ' || *s == '\t');    /* skip white space */
      i = sscanf(s, "%lf", &timeResolutionSeconds);
      if (i < 1 || timeResolutionSeconds < 1.0) {
        timeResolutionSeconds = -1.0;       /* invalid entry */
        continue;
      }
    }
    fclose(f);
    if (timeResolutionSeconds <= 0.0) {
      fprintf(stderr, "No valid CPU frequency entry "
                      "was found in /proc/cpuinfo.\n");
      return -1;
    }
    /* MHz -> seconds */
    timeResolutionSeconds = 0.000001 / timeResolutionSeconds;
#elif defined(__unix) || defined(SGI) || defined(LINUX)
    timeResolutionSeconds = 0.000001;
#elif defined(WIN32)
    {
      LARGE_INTEGER tmp;
      QueryPerformanceFrequency(&tmp);
      timeResolutionSeconds = (double) ((unsigned long) tmp.LowPart);
      timeResolutionSeconds += (double) ((long) tmp.HighPart)
                               * 4294967296.0;
      timeResolutionSeconds = 1.0 / timeResolutionSeconds;
    }
#else
    timeResolutionSeconds = 1.0;
#endif
    fprintf(stderr, "time resolution is %.3f ns\n",
                    1.0e9 * timeResolutionSeconds);
    return 0;
}

/* macro for getting real time */

#if defined(HAVE_RDTSC)
/* optimised high resolution timer for Linux/i586/GCC only */
#define get_real_time(h,l)                                              \
{                                                                       \
    asm volatile ("rdtsc" : "=a" (l), "=d" (h));                        \
}
#elif defined(__unix) || defined(SGI) || defined(LINUX)
/* UNIX: use gettimeofday() - allows 1 us resolution */
#define get_real_time(h,l)                                              \
{                                                                       \
    struct timeval tv;                                                  \
    gettimeofday(&tv, NULL);                                            \
    h = (unsigned long) tv.tv_sec;                                      \
    l = (unsigned long) tv.tv_usec;                                     \
}
#elif defined(WIN32)
/* Win32: use QueryPerformanceCounter - resolution depends on system, */
/* but is expected to be better than 1 us. GetSystemTimeAsFileTime    */
/* seems to have much worse resolution under Win95.                   */
#define get_real_time(h,l)                                              \
{                                                                       \
    LARGE_INTEGER tmp;                                                  \
    QueryPerformanceCounter(&tmp);                                      \
    h = (unsigned long) tmp.HighPart;                                   \
    l = (unsigned long) tmp.LowPart;                                    \
}
#else
/* other systems: use ANSI C time() - allows 1 second resolution */
#define get_real_time(h,l)                                              \
{                                                                       \
    h = 0UL;                                                            \
    l = (unsigned long) time(NULL);                                     \
}
#endif

/* macro for getting CPU time */

#define get_CPU_time(h,l)                                               \
{                                                                       \
    h = 0UL;                                                            \
    l = (unsigned long) clock();                                        \
}

/* initialise a timer structure */

void timers_struct_init(RTCLOCK *p)
{
    p->real_time_to_seconds_scale = timeResolutionSeconds;
    p->CPU_time_to_seconds_scale = 1.0 / (double) CLOCKS_PER_SEC;
    get_real_time(p->starttime_real_high, p->starttime_real_low)
    get_CPU_time(p->starttime_CPU_high, p->starttime_CPU_low)
}

/* return the elapsed real time (in seconds) since the specified timer */
/* structure was initialised */

double timers_get_real_time(RTCLOCK *p)
{
    unsigned long h, l;
    get_real_time(h, l)
#if defined(HAVE_RDTSC) || defined(WIN32)
    h -= p->starttime_real_high;
    if (l < p->starttime_real_low) h--;
    l -= p->starttime_real_low;
    return (((double) ((long) h) * 4294967296.0 + (double) l)
            * p->real_time_to_seconds_scale);
#elif defined(__unix) || defined(SGI) || defined(LINUX)
    return ((double) ((long) h - (long) p->starttime_real_high)
            + ((double) ((long) l - (long) p->starttime_real_low) * 0.000001));
#else
    return ((double) ((long) (l - p->starttime_real_low)));
#endif
}

/* return the elapsed CPU time (in seconds) since the specified timer */
/* structure was initialised */

double timers_get_CPU_time(RTCLOCK *p)
{
    unsigned long h, l;
    get_CPU_time(h, l)
    l -= p->starttime_CPU_low;
    return ((double) l * p->CPU_time_to_seconds_scale);
}

/* return a 32-bit unsigned integer to be used as seed from current time */

unsigned long timers_random_seed(void)
{
    unsigned long h, l;
    get_real_time(h, l)
#if !defined(HAVE_RDTSC) && (defined(__unix) || defined(SGI) || defined(LINUX))
    /* make use of all bits */
    l += (h & 0x00000FFFUL) * 1000000UL;
#endif
    return l;
}

/**
 * Return the size of MYFLT in bytes.
 */
PUBLIC int csoundGetSizeOfMYFLT(void)
{
    return (int) sizeof(MYFLT);
}

/**
 * Return pointer to user data pointer for real time audio input.
 */
PUBLIC void **csoundGetRtRecordUserData(void *csound)
{
    return &(((ENVIRON*) csound)->rtRecord_userdata);
}

/**
 * Return pointer to user data pointer for real time audio output.
 */
PUBLIC void **csoundGetRtPlayUserData(void *csound)
{
    return &(((ENVIRON*) csound)->rtPlay_userdata);
}

#ifdef __cplusplus
};
#endif

