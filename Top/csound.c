/*
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
 *
 * 29 May 2002 - ma++ merge with CsoundLib.
 * 30 May 2002 - mkg add csound "this" pointer argument back into merge.
 * 27 Jun 2002 - mkg complete Linux dl code and Makefile
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_UNISTD_H) || defined (__unix) || defined(__unix__)
#include <unistd.h>
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

  /* from oload.c, initial state of CSOUND structure */
  extern const CSOUND cenviron_;
  /* from threads.c */
  void csoundLock(void);
  void csoundUnLock(void);
  /* aops.c */
  void aops_init_tables(void);

  typedef struct csInstance_s {
    CSOUND              *csound;
    struct csInstance_s *nxt;
  } csInstance_t;

  /* initialisation state: */
  /* 0: not done yet, 1: complete, 2: in progress, -1: failed */
  static  volatile  int init_done = 0;
  /* chain of allocated Csound instances */
  static  volatile  csInstance_t  *instance_list = NULL;
  /* non-zero if performance should be terminated now */
  static  volatile  int exitNow_ = 0;

  static void destroy_all_instances(void)
  {
    volatile csInstance_t *p;

    csoundLock();
    init_done = -1;     /* prevent the creation of any new instances */
    if (instance_list == NULL) {
      csoundUnLock();
      return;
    }
    csoundUnLock();
    csoundSleep(250);
    while (1) {
      csoundLock(); p = instance_list; csoundUnLock();
      if (p == NULL)
        break;
      csoundDestroy(p->csound);
    }
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

  static void psignal(int sig, char *str)
  {
    fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
  }
#elif defined(__BEOS__)
  static void psignal(int sig, char *str)
  {
    fprintf(stderr, "%s: %s\n", str, strsignal(sig));
  }
#endif

  static void signal_handler(int sig)
  {
    psignal(sig, "Csound tidy up");
    if ((sig == (int) SIGINT || sig == (int) SIGTERM) && !exitNow_) {
      exitNow_ = -1;
      return;
    }
    exit(1);
  }

  static void install_signal_handler(void)
  {
    int *x;
    int sigs[] = {
#if defined(LINUX) || defined(SGI) || defined(sol) || defined(__MACH__)
      SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT, SIGBUS,
      SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, SIGXCPU, SIGXFSZ,
#elif defined(WIN32)
      SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM,
#elif defined(__EMX__)
      SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE,
      SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGCHLD,
#endif
      -1
    };
    for (x = sigs; *x > 0; x++)
      signal(*x, signal_handler);
  }

  static int getTimeResolution(void);

  PUBLIC int csoundInitialize(int *argc, char ***argv, int flags)
  {
    int     n;

    do {
      csoundLock();
      n = init_done;
      switch (n) {
        case 2:
          csoundUnLock();
          csoundSleep(1);
        case 0:
          break;
        default:
          csoundUnLock();
          return (n >= 0 ? 0 : -1);
      }
    } while (n);
    init_done = 2;
    csoundUnLock();
    if (argc == NULL || argv == NULL || *argc <= 0 || *argv == NULL)
      init_getstring(0, (char**) NULL);
    else
      init_getstring(*argc, *argv);
    if (getTimeResolution() != 0) {
      csoundLock(); init_done = -1; csoundUnLock();
      return -1;
    }
    if (!(flags & CSOUNDINIT_NO_SIGNAL_HANDLER))
      install_signal_handler();
    if (!(flags & CSOUNDINIT_NO_ATEXIT))
      atexit(destroy_all_instances);
    aops_init_tables();
    csoundLock(); init_done = 1; csoundUnLock();
    return 0;
  }

  PUBLIC CSOUND *csoundCreate(void *hostdata)
  {
    CSOUND        *csound;
    csInstance_t  *p;

    if (init_done != 1) {
      if (csoundInitialize(NULL, NULL, 0) != 0)
        return NULL;
    }
    csound = (CSOUND*) malloc(sizeof(CSOUND));
    if (csound == NULL)
      return NULL;
    memcpy(csound, &cenviron_, sizeof(CSOUND));
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
    p->nxt = (csInstance_t*) instance_list;
    instance_list = p;
    csoundUnLock();
    csoundReset(csound);

    return csound;
  }

  /* dummy real time MIDI functions */
  static int DummyMidiInOpen(CSOUND *csound, void **userData,
                             const char *devName);
  static int DummyMidiRead(CSOUND *csound, void *userData,
                           unsigned char *buf, int nbytes);
  static int DummyMidiInClose(CSOUND *csound, void *userData);
  static int DummyMidiOutOpen(CSOUND *csound, void **userData,
                              const char *devName);
  static int DummyMidiWrite(CSOUND *csound, void *userData,
                            const unsigned char *buf, int nbytes);
  static int DummyMidiOutClose(CSOUND *csound, void *userData);
  static const char *DummyMidiErrorString(int errcode);

  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  PUBLIC int csoundPreCompile(CSOUND *p)
  {
    char    *s;
    int     i, max_len;
    volatile int  n;

    if ((n = setjmp(p->exitjmp)) != 0) {
      return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    /* reset instance */
    csoundReset(p);
    /* copy system environment variables */
    i = csoundInitEnv(p);
    if (i != CSOUND_SUCCESS) {
      p->engineState |= 16;
      return i;
    }
    /* allow selecting real time audio module */
    max_len = 21;
    csoundCreateGlobalVariable(p, "_RTAUDIO", (size_t) max_len);
    s = csoundQueryGlobalVariable(p, "_RTAUDIO");
    strcpy(s, "PortAudio");
    csoundCreateConfigurationVariable(
        p, "rtaudio", s, CSOUNDCFG_STRING, 0, NULL, &max_len,
        "Real time audio module name", NULL);
    /* initialise real time MIDI */
    p->midiGlobals = (MGLOBAL*) mcalloc(p, sizeof(MGLOBAL));
    p->midiGlobals->Midevtblk = (MEVENT*) NULL;
    csoundSetExternalMidiInOpenCallback(p, DummyMidiInOpen);
    csoundSetExternalMidiReadCallback(p, DummyMidiRead);
    csoundSetExternalMidiInCloseCallback(p, DummyMidiInClose);
    csoundSetExternalMidiOutOpenCallback(p, DummyMidiOutOpen);
    csoundSetExternalMidiWriteCallback(p, DummyMidiWrite);
    csoundSetExternalMidiOutCloseCallback(p, DummyMidiOutClose);
    csoundSetExternalMidiErrorStringCallback(p, DummyMidiErrorString);
    p->midiGlobals->midiInUserData = NULL;
    p->midiGlobals->midiOutUserData = NULL;
    p->midiGlobals->midiFileData = NULL;
    p->midiGlobals->bufp = &(p->midiGlobals->mbuf[0]);
    p->midiGlobals->endatp = p->midiGlobals->bufp;
    csoundCreateGlobalVariable(p, "_RTMIDI", (size_t) max_len);
    s = csoundQueryGlobalVariable(p, "_RTMIDI");
    strcpy(s, "PortMIDI");
    csoundCreateConfigurationVariable(
        p, "rtmidi", s, CSOUNDCFG_STRING, 0, NULL, &max_len,
        "Real time MIDI module name", NULL);
    max_len = 256;  /* should be the same as in csoundCore.h */
    csoundCreateConfigurationVariable(
        p, "mute_tracks", &(p->midiGlobals->muteTrackList[0]),
        CSOUNDCFG_STRING, 0, NULL, &max_len,
        "Ignore events (other than tempo changes) in tracks defined by pattern",
        NULL);
    csoundCreateConfigurationVariable(
        p, "raw_controller_mode", &(p->midiGlobals->rawControllerMode),
        CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
        "Do not handle special MIDI controllers (sustain pedal etc.)", NULL);
    /* sound file tag options */
    max_len = 201;
    i = (max_len + 7) & (~7);
    p->SF_id_title = (char*) mcalloc(p, (size_t) i * (size_t) 6);
    csoundCreateConfigurationVariable(
        p, "id_title", p->SF_id_title, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Title tag in output soundfile (no spaces)", NULL);
    p->SF_id_copyright = (char*) p->SF_id_title + (int) i;
    csoundCreateConfigurationVariable(
        p, "id_copyright", p->SF_id_copyright, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Copyright tag in output soundfile (no spaces)", NULL);
    p->SF_id_software = (char*) p->SF_id_copyright + (int) i;
    csoundCreateConfigurationVariable(
        p, "id_software", p->SF_id_software, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Software tag in output soundfile (no spaces)", NULL);
    p->SF_id_artist = (char*) p->SF_id_software + (int) i;
    csoundCreateConfigurationVariable(
        p, "id_artist", p->SF_id_artist, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Artist tag in output soundfile (no spaces)", NULL);
    p->SF_id_comment = (char*) p->SF_id_artist + (int) i;
    csoundCreateConfigurationVariable(
        p, "id_comment", p->SF_id_comment, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Comment tag in output soundfile (no spaces)", NULL);
    p->SF_id_date = (char*) p->SF_id_comment + (int) i;
    csoundCreateConfigurationVariable(
        p, "id_date", p->SF_id_date, CSOUNDCFG_STRING, 0,
        NULL, &max_len, "Date tag in output soundfile (no spaces)", NULL);
    {
      int   minVal = 10;
      int   maxVal = 10000;
      MYFLT minValF = FL(0.0);
      /* max. length of string variables */
      csoundCreateConfigurationVariable(
          p, "max_str_len", &(p->strVarMaxLen),
          CSOUNDCFG_INTEGER, 0, &minVal, &maxVal,
          "Maximum length of string variables + 1", NULL);
      csoundCreateConfigurationVariable(
          p, "msg_color", &(p->enableMsgAttr),
          CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
          "Enable message attributes (colors etc.)", NULL);
      csoundCreateConfigurationVariable(
          p, "skip_seconds", &(p->csoundScoreOffsetSeconds_),
          CSOUNDCFG_MYFLT, 0, &minValF, NULL,
          "Start score playback at the specified time, skipping earlier events",
          NULL);
    }
    p->engineState |= 1;
    /* now load and pre-initialise external modules for this instance */
    /* this function returns an error value that may be worth checking */
    return csoundLoadModules(p);
  }

  PUBLIC int csoundQueryInterface(const char *name, void **iface, int *version)
  {
    if (strcmp(name, "CSOUND") != 0)
      return 1;
    *iface = csoundCreate(NULL);
    *version = csoundGetVersion();
    return 0;
  }

  PUBLIC void csoundDestroy(CSOUND *csound)
  {
    csInstance_t  *p, *prv = NULL;
    csoundLock();
    p = (csInstance_t*) instance_list;
    while (p != NULL && p->csound != csound) {
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
    free(csound->oparms);
    free(csound);
  }

  PUBLIC int csoundGetVersion(void)
  {
    return (int) (CS_VERSION * 1000 + CS_SUBVER * 10 + CS_PATCHLEVEL);
  }

  int csoundGetAPIVersion(void)
  {
    return CS_APIVERSION * 100 + CS_APISUBVER;
  }

  PUBLIC void *csoundGetHostData(CSOUND *csound)
  {
    return csound->hostdata;
  }

  PUBLIC void csoundSetHostData(CSOUND *csound, void *hostData)
  {
    csound->hostdata = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int sensevents(CSOUND *);

  /**
   * perform currently active instrs for one kperiod
   *      & send audio result to output buffer
   * returns non-zero if this kperiod was skipped
   */

  static inline int kperf(CSOUND *csound)
  {
    INSDS   *ip;
    int     i;

    /* update orchestra time */
    csound->kcounter = ++(csound->global_kcounter);
    csound->curTime += csound->curTime_inc;
    csound->curBeat += csound->curBeat_inc;
    /* if skipping time on request by 'a' score statement: */
    if (csound->advanceCnt) {
      csound->advanceCnt--;
      return 1;
    }
    /* if i-time only, return now */
    if (csound->initonly)
      return 1;
    /* PC GUI needs attention, but avoid excessively frequent */
    /* calls of csoundYield() */
    if (--(csound->evt_poll_cnt) < 0) {
      csound->evt_poll_cnt = csound->evt_poll_maxcnt;
      if (!csoundYield(csound))
        csound->LongJmp(csound, 1);
    }
    /* for one kcnt: */
    if (csound->oparms->sfread)         /*   if audio_infile open  */
      csound->spinrecv(csound);         /*      fill the spin buf  */
    csound->spoutactive = 0;            /*   make spout inactive   */
    ip = csound->actanchor.nxtact;
    while (ip != NULL) {                /* for each instr active:  */
      INSDS *nxt = ip->nxtact;
      csound->pds = (OPDS*) ip;
      while ((csound->pds = csound->pds->nxtp) != NULL) {
        (*csound->pds->opadr)(csound, csound->pds); /* run each opcode */
      }
      ip = nxt;
    }
    if (!csound->spoutactive)           /*   results now in spout? */
      for (i = 0; i < (int) csound->nspout; i++)
        csound->spout[i] = FL(0.0);
    csound->spoutran(csound);           /*      send to audio_out  */
    return 0;
  }

  PUBLIC int csoundPerformKsmps(CSOUND *csound)
  {
    int done;
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if ((returnValue = setjmp(csound->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
      return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    do {
      if ((done = sensevents(csound))) {
        csoundMessage(csound, "Score finished in csoundPerformKsmps()\n");
        return done;
      }
    } while (kperf(csound));
    return 0;
  }

  PUBLIC int csoundPerformKsmpsAbsolute(CSOUND *csound)
  {
    int done = 0;
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if ((returnValue = setjmp(csound->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
      return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    do {
      done |= sensevents(csound);
    } while (kperf(csound));
    return done;
  }

  /* external host's outbuffer passed in csoundPerformBuffer() */

  PUBLIC int csoundPerformBuffer(CSOUND *csound)
  {
    volatile int returnValue;
    int     done;
    /* Setup jmp for return after an exit(). */
    if ((returnValue = setjmp(csound->exitjmp))) {
      csoundMessage(csound, "Early return from csoundPerformBuffer().\n");
      return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    csound->sampsNeeded += csound->oparms->outbufsamps;
    while (csound->sampsNeeded > 0) {
      do {
        if ((done = sensevents(csound)))
          return done;
      } while (kperf(csound));
      csound->sampsNeeded -= csound->nspout;
    }
    return 0;
  }

  /*
   * ATTRIBUTES
   */

  PUBLIC MYFLT csoundGetSr(CSOUND *csound)
  {
    return csound->esr;
  }

  PUBLIC MYFLT csoundGetKr(CSOUND *csound)
  {
    return csound->ekr;
  }

  PUBLIC int csoundGetKsmps(CSOUND *csound)
  {
    return csound->ksmps;
  }

  PUBLIC int csoundGetNchnls(CSOUND *csound)
  {
    return csound->nchnls;
  }

  PUBLIC MYFLT csoundGet0dBFS(CSOUND *csound)
  {
    return csound->e0dbfs;
  }

  PUBLIC int csoundGetSampleFormat(CSOUND *csound)
  {
    /* should we assume input is same as output ? */
    return csound->oparms->outformat;
  }

  PUBLIC int csoundGetSampleSize(CSOUND *csound)
  {
    /* should we assume input is same as output ? */
    return csound->oparms->sfsampsize;
  }

  PUBLIC long csoundGetInputBufferSize(CSOUND *csound)
  {
    return csound->oparms->inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(CSOUND *csound)
  {
    return csound->oparms->outbufsamps;
  }

  PUBLIC MYFLT *csoundGetSpin(CSOUND *csound)
  {
    return csound->spin;
  }

  PUBLIC MYFLT *csoundGetSpout(CSOUND *csound)
  {
    return csound->spout;
  }

  PUBLIC const char *csoundGetOutputFileName(CSOUND *csound)
  {
    return (const char*) csound->oparms->outfilename;
  }

  /**
   * Calling this function with a non-zero 'state' value between
   * csoundPreCompile() and csoundCompile() will disable all default
   * handling of sound I/O by the Csound library, allowing the host
   * application to use the spin/spout/input/output buffers directly.
   * If 'bufSize' is greater than zero, the buffer size (-b) will be
   * set to the integer multiple of ksmps that is nearest to the value
   * specified.
   */

  PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *csound,
                                              int state, int bufSize)
  {
    csound->enableHostImplementedAudioIO = state;
    csound->hostRequestedBufferSize = (bufSize > 0 ? bufSize : 0);
  }

  PUBLIC MYFLT csoundGetScoreTime(CSOUND *csound)
  {
    return (MYFLT) csound->curTime;
  }

  PUBLIC MYFLT csoundGetProgress(CSOUND *csound)
  {
    (void) csound;
    return -1;
  }

  PUBLIC MYFLT csoundGetProfile(CSOUND *csound)
  {
    (void) csound;
    return -1;
  }

  PUBLIC MYFLT csoundGetCpuUsage(CSOUND *csound)
  {
    (void) csound;
    return -1;
  }

  /*
   * SCORE HANDLING
   */

  PUBLIC int csoundIsScorePending(CSOUND *csound)
  {
    return csound->csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(CSOUND *csound, int pending)
  {
    csound->csoundIsScorePending_ = pending;
  }

  PUBLIC void csoundSetScoreOffsetSeconds(CSOUND *csound, MYFLT offset)
  {
    double  aTime;
    MYFLT   prv = (MYFLT) csound->csoundScoreOffsetSeconds_;

    csound->csoundScoreOffsetSeconds_ = offset;
    if (offset < FL(0.0))
      return;
    /* if csoundCompile() was not called yet, just store the offset */
    if (!(csound->engineState & 2))
      return;
    /* otherwise seek to the requested time now */
    aTime = (double) offset - csound->curTime;
    if (aTime < 0.0 || offset < prv) {
      csoundRewindScore(csound);    /* will call csoundSetScoreOffsetSeconds */
      return;
    }
    if (aTime > 0.0) {
      EVTBLK  evt;
      evt.strarg = NULL;
      evt.opcod = 'a';
      evt.pcnt = 3;
      evt.p[2] = evt.p[1] = FL(0.0);
      evt.p[3] = (MYFLT) aTime;
      insert_score_event(csound, &evt, csound->curTime);
    }
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(CSOUND *csound)
  {
    return csound->csoundScoreOffsetSeconds_;
  }

  extern void musmon_rewind_score(CSOUND *csound);      /* musmon.c */
  extern void midifile_rewind_score(CSOUND *csound);    /* midifile.c */

  PUBLIC void csoundRewindScore(CSOUND *csound)
  {
    musmon_rewind_score(csound);
    midifile_rewind_score(csound);
  }

  void csoundDefaultMessageCallback(CSOUND *csound, int attr,
                                    const char *format, va_list args)
  {
#if defined(WIN32) || defined(MAC)
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
    if (!attr || !csound->enableMsgAttr) {
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

  void csoundDefaultThrowMessageCallback(CSOUND *csound,
                                         const char *format, va_list args)
  {
    csoundDefaultMessageCallback(csound, CSOUNDMSG_ERROR, format, args);
  }

  PUBLIC void csoundSetMessageCallback(CSOUND *csound,
                            void (*csoundMessageCallback)(CSOUND *csound,
                                                          int attr,
                                                          const char *format,
                                                          va_list args))
  {
    csound->csoundMessageCallback_ = csoundMessageCallback;
  }

  PUBLIC void csoundMessageV(CSOUND *csound,
                             int attr, const char *format, va_list args)
  {
    csound->csoundMessageCallback_(csound, attr, format, args);
  }

  PUBLIC void csoundMessage(CSOUND *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundMessageCallback_(csound, 0, format, args);
    va_end(args);
  }

  PUBLIC void csoundMessageS(CSOUND *csound, int attr, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundMessageCallback_(csound, attr, format, args);
    va_end(args);
  }

  void csoundDie(CSOUND *csound, const char *msg, ...)
  {
    va_list args;
    va_start(args, msg);
    csound->ErrMsgV(csound, (char*) 0, msg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
  }

  void csoundWarning(CSOUND *csound, const char *msg, ...)
  {
    va_list args;
    if (!(csound->oparms->msglevel & WARNMSG))
      return;
    csoundMessageS(csound, CSOUNDMSG_WARNING, Str("WARNING: "));
    va_start(args, msg);
    csound->csoundMessageCallback_(csound, CSOUNDMSG_WARNING, msg, args);
    va_end(args);
    csoundMessageS(csound, CSOUNDMSG_WARNING, "\n");
  }

  void csoundDebugMsg(CSOUND *csound, const char *msg, ...)
  {
    va_list args;
    if (!(csound->oparms->odebug))
      return;
    va_start(args, msg);
    csound->csoundMessageCallback_(csound, 0, msg, args);
    va_end(args);
    csoundMessage(csound, "\n");
  }

  void csoundErrorMsg(CSOUND *csound, const char *msg, ...)
  {
    va_list args;
    va_start(args, msg);
    csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
    va_end(args);
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundErrMsgV(CSOUND *csound,
                     const char *hdr, const char *msg, va_list args)
  {
    if (hdr != NULL)
      csound->MessageS(csound, CSOUNDMSG_ERROR, "%s", hdr);
    csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundLongJmp(CSOUND *csound, int retval)
  {
    int n = CSOUND_EXITJMP_SUCCESS;
    n = (retval < 0 ? n + retval : n - retval) & (CSOUND_EXITJMP_SUCCESS - 1);
    if (!n)
      n = CSOUND_EXITJMP_SUCCESS;
    csound->engineState |= 16;
    longjmp(csound->exitjmp, n);
  }

  PUBLIC void csoundSetThrowMessageCallback(CSOUND *csound,
                    void (*csoundThrowMessageCallback)(CSOUND *csound,
                                                       const char *format,
                                                       va_list args))
  {
    csound->csoundThrowMessageCallback_ = csoundThrowMessageCallback;
  }

  PUBLIC void csoundThrowMessageV(CSOUND *csound,
                                  const char *format, va_list args)
  {
    csound->csoundThrowMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundThrowMessage(CSOUND *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundThrowMessageCallback_(csound, format, args);
    va_end(args);
  }

  PUBLIC int csoundGetMessageLevel(CSOUND *csound)
  {
    return csound->oparms->msglevel;
  }

  PUBLIC void csoundSetMessageLevel(CSOUND *csound, int messageLevel)
  {
    csound->oparms->msglevel = messageLevel;
  }

  PUBLIC void csoundInputMessage(CSOUND *csound, const char *message)
  {
    writeLine(csound, message, strlen(message));
  }

  PUBLIC void csoundKeyPress(CSOUND *csound, char c)
  {
    csound->inChar_ = (int) ((unsigned char) c);
  }

  char getChar(CSOUND *csound)
  {
    char  c = (char) csound->inChar_;
    csound->inChar_ = 0;
    return c;
  }

  /*
   * CONTROL AND EVENTS
   */

  PUBLIC void csoundSetInputValueCallback(CSOUND *csound,
                  void (*inputValueCalback)(CSOUND *csound,
                                            const char *channelName,
                                            MYFLT *value))
  {
    csound->InputValueCallback_ = inputValueCalback;
  }

  PUBLIC void csoundSetOutputValueCallback(CSOUND *csound,
                  void (*outputValueCalback)(CSOUND *csound,
                                             const char *channelName,
                                             MYFLT value))
  {
    csound->OutputValueCallback_ = outputValueCalback;
  }

  PUBLIC int csoundScoreEvent(CSOUND *csound, char type,
                              const MYFLT *pfields, long numFields)
  {
    EVTBLK        evt;
    int           i;

    evt.strarg = NULL;
    evt.opcod = type;
    evt.pcnt = (short) numFields;
    for (i = 0; i < (int) numFields; i++)
      evt.p[i + 1] = pfields[i];
    return insert_score_event(csound, &evt, csound->curTime);
  }

  /*
   *    REAL-TIME AUDIO
   */

/* dummy functions for the case when no real-time audio module is available */

static double *get_dummy_rtaudio_globals(CSOUND *csound)
{
    double  *p;

    p = (double*) csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
    if (p == NULL) {
      if (csound->CreateGlobalVariable(csound, "__rtaudio_null_state",
                                               sizeof(double) * 4) != 0)
        csound->Die(csound, Str("rtdummy: failed to allocate globals"));
      csound->Message(csound, Str("rtaudio: dummy module enabled\n"));
      p = (double*) csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
    }
    return p;
}

static void dummy_rtaudio_timer(CSOUND *csound, double *p)
{
    double  timeWait;
    int     i;

    timeWait = p[0] - csoundGetRealTime(csound->csRtClock);
    i = (int) (timeWait * 1000.0 + 0.5);
    if (i > 0)
      csoundSleep((size_t) i);
}

int playopen_dummy(CSOUND *csound, const csRtAudioParams *parm)
{
    double  *p;
    char    *s;

    /* find out if the use of dummy real-time audio functions was requested, */
    /* or an unknown plugin name was specified; the latter case is an error */
    s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
    if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                       strcmp(s, "NULL") == 0)) {
      if (s[0] == '\0')
        csoundErrorMsg(csound,
                       Str(" *** error: rtaudio module set to empty string"));
      else
        csoundErrorMsg(csound,
                       Str(" *** error: unknown rtaudio module: '%s'"), s);
      return CSOUND_ERROR;
    }
    p = get_dummy_rtaudio_globals(csound);
    csound->rtPlay_userdata = (void*) p;
    p[0] = csound->GetRealTime(csound->csRtClock);
    p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                  * (double) parm->sampleRate);
    return CSOUND_SUCCESS;
}

void rtplay_dummy(CSOUND *csound, const MYFLT *outBuf, int nbytes)
{
    double  *p = (double*) csound->rtPlay_userdata;
    (void) outBuf;
    p[0] += ((double) nbytes * p[1]);
    dummy_rtaudio_timer(csound, p);
}

int recopen_dummy(CSOUND *csound, const csRtAudioParams *parm)
{
    double  *p;
    char    *s;

    /* find out if the use of dummy real-time audio functions was requested, */
    /* or an unknown plugin name was specified; the latter case is an error */
    s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
    if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                       strcmp(s, "NULL") == 0)) {
      if (s[0] == '\0')
        csoundErrorMsg(csound,
                       Str(" *** error: rtaudio module set to empty string"));
      else
        csoundErrorMsg(csound,
                       Str(" *** error: unknown rtaudio module: '%s'"), s);
      return CSOUND_ERROR;
    }
    p = (double*) get_dummy_rtaudio_globals(csound) + 2;
    csound->rtRecord_userdata = (void*) p;
    p[0] = csound->GetRealTime(csound->csRtClock);
    p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                  * (double) parm->sampleRate);
    return CSOUND_SUCCESS;
}

int rtrecord_dummy(CSOUND *csound, MYFLT *inBuf, int nbytes)
{
    double  *p = (double*) csound->rtRecord_userdata;
    int     i;

    for (i = 0; i < (nbytes / (int) sizeof(MYFLT)); i++)
      ((MYFLT*) inBuf)[i] = FL(0.0);

    p[0] += ((double) nbytes * p[1]);
    dummy_rtaudio_timer(csound, p);

    return nbytes;
}

void rtclose_dummy(CSOUND *csound)
{
    csound->rtPlay_userdata = NULL;
    csound->rtRecord_userdata = NULL;
}

  PUBLIC void csoundSetPlayopenCallback(CSOUND *csound,
                  int (*playopen__)(CSOUND *, const csRtAudioParams *parm))
  {
    csound->playopen_callback = playopen__;
  }

  PUBLIC void csoundSetRtplayCallback(CSOUND *csound,
                  void (*rtplay__)(CSOUND *, const MYFLT *outBuf, int nbytes))
  {
    csound->rtplay_callback = rtplay__;
  }

  PUBLIC void csoundSetRecopenCallback(CSOUND *csound,
                  int (*recopen__)(CSOUND *, const csRtAudioParams *parm))
  {
    csound->recopen_callback = recopen__;
  }

  PUBLIC void csoundSetRtrecordCallback(CSOUND *csound,
                  int (*rtrecord__)(CSOUND *, MYFLT *inBuf, int nbytes))
  {
    csound->rtrecord_callback = rtrecord__;
  }

  PUBLIC void csoundSetRtcloseCallback(CSOUND *csound,
                  void (*rtclose__)(CSOUND *))
  {
    csound->rtclose_callback = rtclose__;
  }

/* dummy real time MIDI functions */

static int DummyMidiInOpen(CSOUND *csound, void **userData, const char *devName)
{
    char *s;

    (void) devName;
    *userData = NULL;
    s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (s == NULL ||
        (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
         strcmp(s, "NULL") == 0)) {
      csoundMessage(csound, Str("WARNING: real time midi input disabled, "
                                "using dummy functions\n"));
      return 0;
    }
    if (s[0] == '\0')
      csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
    else
      csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
    return -1;
}

static int DummyMidiRead(CSOUND *csound,
                         void *userData, unsigned char *buf, int nbytes)
{
    (void) csound; (void) userData; (void) buf; (void) nbytes;
    return 0;
}

static int DummyMidiInClose(CSOUND *csound, void *userData)
{
    (void) csound; (void) userData;
    return 0;
}

static int DummyMidiOutOpen(CSOUND *csound,
                            void **userData, const char *devName)
{
    char *s;

    (void) devName;
    *userData = NULL;
    s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (s == NULL ||
        (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
         strcmp(s, "NULL") == 0)) {
      csoundMessage(csound, Str("WARNING: real time midi output disabled, "
                                "using dummy functions\n"));
      return 0;
    }
    if (s[0] == '\0')
      csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
    else
      csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
    return -1;
}

static int DummyMidiWrite(CSOUND *csound,
                          void *userData, const unsigned char *buf, int nbytes)
{
    (void) csound; (void) userData; (void) buf;
    return nbytes;
}

static int DummyMidiOutClose(CSOUND *csound, void *userData)
{
    (void) csound; (void) userData;
    return 0;
}

static const char *midi_err_msg = "Unknown MIDI error";

static const char *DummyMidiErrorString(int errcode)
{
    (void) errcode;
    return midi_err_msg;
}

/**
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
const char *csoundExternalMidiErrorString(CSOUND *csound, int errcode)
{
    if (csound->midiGlobals->MidiErrorStringCallback == NULL)
      return midi_err_msg;
    return (csound->midiGlobals->MidiErrorStringCallback(errcode));
}

/* Set real time MIDI function pointers. */

  PUBLIC void csoundSetExternalMidiInOpenCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void **, const char *))
  {
    csound->midiGlobals->MidiInOpenCallback = func;
  }

  PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void *, unsigned char *, int))
  {
    csound->midiGlobals->MidiReadCallback = func;
  }

  PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void *))
  {
    csound->midiGlobals->MidiInCloseCallback = func;
  }

  PUBLIC void csoundSetExternalMidiOutOpenCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void **, const char *))
  {
    csound->midiGlobals->MidiOutOpenCallback = func;
  }

  PUBLIC void csoundSetExternalMidiWriteCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void *, const unsigned char *, int))
  {
    csound->midiGlobals->MidiWriteCallback = func;
  }

  PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *csound,
                  int (*func)(CSOUND *, void *))
  {
    csound->midiGlobals->MidiOutCloseCallback = func;
  }

  PUBLIC void csoundSetExternalMidiErrorStringCallback(CSOUND *csound,
                  const char *(*func)(int))
  {
    csound->midiGlobals->MidiErrorStringCallback = func;
  }

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  PUBLIC void csoundSetIsGraphable(CSOUND *csound, int isGraphable)
  {
    csound->isGraphable_ = isGraphable;
  }

  int Graphable(CSOUND *csound)
  {
    return csound->isGraphable_;
  }

  void defaultCsoundMakeGraph(CSOUND *csound, WINDAT *windat, const char *name)
  {
#if defined(USE_FLTK)
    extern void MakeGraph_(CSOUND *, WINDAT *, const char *);
    MakeGraph_(csound, windat, name);
#else
    extern void MakeAscii(CSOUND *, WINDAT *, const char *);
    MakeAscii(csound, windat, name);
#endif
  }

  PUBLIC void csoundSetMakeGraphCallback(CSOUND *csound,
                  void (*makeGraphCallback)(CSOUND *csound,
                                            WINDAT *windat, const char *name))
  {
    csound->csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(CSOUND *csound, WINDAT *windat, const char *name)
  {
    csound->csoundMakeGraphCallback_(csound, windat, name);
  }

  void defaultCsoundDrawGraph(CSOUND *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void DrawGraph_(CSOUND *, WINDAT *);
    DrawGraph_(csound, windat);
#else
    extern void DrawAscii(CSOUND *, WINDAT *);
    DrawAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetDrawGraphCallback(CSOUND *csound,
                  void (*drawGraphCallback)(CSOUND *csound, WINDAT *windat))
  {
    csound->csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(CSOUND *csound, WINDAT *windat)
  {
    csound->csoundDrawGraphCallback_(csound, windat);
  }

  void defaultCsoundKillGraph(CSOUND *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void KillGraph_(CSOUND *, WINDAT *);
    KillGraph_(csound, windat);
#else
    extern void KillAscii(CSOUND *, WINDAT *);
    KillAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetKillGraphCallback(CSOUND *csound,
                  void (*killGraphCallback)(CSOUND *csound, WINDAT *windat))
  {
    csound->csoundKillGraphCallback_ = killGraphCallback;
  }

  void KillGraph(CSOUND *csound, WINDAT *windat)
  {
    csound->csoundKillGraphCallback_(csound, windat);
  }

  int defaultCsoundExitGraph(CSOUND *csound)
  {
    (void) csound;
    return CSOUND_SUCCESS;
  }

  PUBLIC void csoundSetExitGraphCallback(CSOUND *csound,
                                         int (*exitGraphCallback)(CSOUND *))
  {
    csound->csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph(CSOUND *csound)
  {
    return csound->csoundExitGraphCallback_(csound);
  }

  void MakeXYin(CSOUND *csound, XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
    (void) xyindat; (void) x; (void) y;
    csoundMessage(csound, "xyin not supported. use invalue opcode instead.\n");
  }

  void ReadXYin(CSOUND *csound, XYINDAT *xyindat)
  {
    (void) xyindat;
    csoundMessage(csound, "xyin not supported. use invalue opcode instead.\n");
  }

  /*
   * OPCODES
   */

  static inline int opcode_list_new_oentry(CSOUND *csound, OENTRY *ep)
  {
    int     oldCnt = 0;

    if (csound->opcodlst != NULL)
      oldCnt = (int) ((OENTRY*) csound->oplstend - (OENTRY*) csound->opcodlst);
    if (!(oldCnt & 0x7F)) {
      OENTRY  *newList;
      size_t  nBytes = (size_t) (oldCnt + 0x80) * sizeof(OENTRY);
      if (!oldCnt)
        newList = (OENTRY*) malloc(nBytes);
      else
        newList = (OENTRY*) realloc(csound->opcodlst, nBytes);
      if (newList == NULL)
        return CSOUND_MEMORY;
      csound->opcodlst = newList;
      csound->oplstend = ((OENTRY*) newList + (int) oldCnt);
      memset(&(((OENTRY*) csound->opcodlst)[oldCnt]), 0, sizeof(OENTRY) * 0x80);
    }
    memcpy(&(((OENTRY*) csound->opcodlst)[oldCnt]), ep, sizeof(OENTRY));
    csound->oplstend = (OENTRY*) csound->oplstend + (int) 1;
    return 0;
  }

  PUBLIC int csoundAppendOpcode(CSOUND *csound,
                                const char *opname, int dsblksiz, int thread,
                                const char *outypes, const char *intypes,
                                int (*iopadr)(CSOUND *, void *),
                                int (*kopadr)(CSOUND *, void *),
                                int (*aopadr)(CSOUND *, void *))
  {
    OENTRY  tmpEntry;
    int     err;

    tmpEntry.opname     = (char*) opname;
    tmpEntry.dsblksiz   = (unsigned short) dsblksiz;
    tmpEntry.thread     = (unsigned short) thread;
    tmpEntry.outypes    = (char*) outypes;
    tmpEntry.intypes    = (char*) intypes;
    tmpEntry.iopadr     = (SUBR) iopadr;
    tmpEntry.kopadr     = (SUBR) kopadr;
    tmpEntry.aopadr     = (SUBR) aopadr;
    tmpEntry.useropinfo = NULL;
    tmpEntry.prvnum     = 0;
    err = opcode_list_new_oentry(csound, &tmpEntry);
    if (err)
      csoundErrorMsg(csound, Str("Failed to allocate new opcode entry."));

    return err;
  }

  /**
   * Appends a list of opcodes implemented by external software to Csound's
   * internal opcode list. The list should either be terminated with an entry
   * that has a NULL opname, or the number of entries (> 0) should be specified
   * in 'n'. Returns zero on success.
   */

  int csoundAppendOpcodes(CSOUND *csound, const OENTRY *opcodeList, int n)
  {
    OENTRY  *ep = (OENTRY*) opcodeList;
    int     err, retval = 0;

    if (opcodeList == NULL)
      return -1;
    if (n <= 0)
      n = 0x7FFFFFFF;
    while (n && ep->opname != NULL) {
      if ((err = opcode_list_new_oentry(csound, ep)) != 0) {
        csoundErrorMsg(csound, Str("Failed to allocate opcode entry for %s."),
                               ep->opname);
        retval = err;
      }
      else {
        ((OENTRY*) csound->oplstend - 1)->useropinfo = NULL;
        ((OENTRY*) csound->oplstend - 1)->prvnum = 0;
      }
      n--, ep++;
    }
    return retval;
  }

  /*
   * MISC FUNCTIONS
   */

#if !defined(USE_FLTK)
  int defaultCsoundYield(CSOUND *csound)
  {
    (void) csound;
    return 1;
  }
#endif

  PUBLIC void csoundSetYieldCallback(CSOUND *csound,
                                     int (*yieldCallback)(CSOUND *))
  {
    csound->csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(CSOUND *csound)
  {
    if (exitNow_)
      csound->LongJmp(csound, CSOUND_SIGNAL);
    return csound->csoundYieldCallback_(csound);
  }

  extern void csoundDeleteAllGlobalVariables(CSOUND *csound);

  typedef struct resetCallback_s {
    void    *userData;
    int     (*func)(CSOUND *, void *);
    struct resetCallback_s  *nxt;
  } resetCallback_t;

  extern void cscoreRESET(CSOUND *);
  extern void disprepRESET(CSOUND *);
  extern void tranRESET(CSOUND *);
  extern void oloadRESET(CSOUND *);
  extern void memRESET(CSOUND *);

  PUBLIC void csoundReset(CSOUND *csound)
  {
    csoundCleanup(csound);

    /* call registered reset callbacks */
    while (csound->reset_list != NULL) {
      resetCallback_t *p = (resetCallback_t*) csound->reset_list;
      p->func(csound, p->userData);
      csound->reset_list = (void*) p->nxt;
      free(p);
    }

    /* call local destructor routines of external modules */
    /* should check return value... */
    csoundDestroyModules(csound);
    /* IV - Feb 01 2005: clean up configuration variables and */
    /* named dynamic "global" variables of Csound instance */
    csoundDeleteAllConfigurationVariables(csound);
    csoundDeleteAllGlobalVariables(csound);

    cscoreRESET(csound);
    disprepRESET(csound);
    tranRESET(csound);
    oloadRESET(csound);     /* should be called last but one */
    memRESET(csound);       /* and this one should be the last */
  }

  PUBLIC int csoundGetDebug(CSOUND *csound)
  {
    return csound->oparms->odebug;
  }

  PUBLIC void csoundSetDebug(CSOUND *csound, int debug)
  {
    csound->oparms->odebug = debug;
  }

  PUBLIC int csoundTableLength(CSOUND *csound, int table)
  {
    int tableLength;
    csound->GetTable(csound, table, &tableLength);
    return tableLength;
  }

  PUBLIC MYFLT csoundTableGet(CSOUND *csound, int table, int index)
  {
    return csound->flist[table]->ftable[index];
  }

  PUBLIC void csoundTableSet(CSOUND *csound, int table, int index, MYFLT value)
  {
    csound->flist[table]->ftable[index] = value;
  }

  PUBLIC void csoundSetFLTKThreadLocking(CSOUND *csound, int isLocking)
  {
    csound->doFLTKThreadLocking = isLocking;
  }

  PUBLIC int csoundGetFLTKThreadLocking(CSOUND *csound)
  {
    return csound->doFLTKThreadLocking;
  }

/* -------- IV - Jan 27 2005: timer functions -------- */

#ifdef HAVE_GETTIMEOFDAY
#undef HAVE_GETTIMEOFDAY
#endif
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
#define HAVE_GETTIMEOFDAY 1
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
#elif defined(WIN32)
    LARGE_INTEGER tmp1;
    int_least64_t tmp2;
    QueryPerformanceFrequency(&tmp1);
    tmp2 = (int_least64_t) tmp1.LowPart + ((int_least64_t) tmp1.HighPart << 32);
    timeResolutionSeconds = 1.0 / (double) tmp2;
#elif defined(HAVE_GETTIMEOFDAY)
    timeResolutionSeconds = 0.000001;
#else
    timeResolutionSeconds = 1.0;
#endif
    fprintf(stderr, "time resolution is %.3f ns\n",
                    1.0e9 * timeResolutionSeconds);
    return 0;
  }

  /* function for getting real time */

  static inline int_least64_t get_real_time(void)
  {
#if defined(HAVE_RDTSC)
    /* optimised high resolution timer for Linux/i586/GCC only */
    uint32_t  l, h;
    asm volatile ("rdtsc" : "=a" (l), "=d" (h));
    return ((int_least64_t) l + ((int_least64_t) h << 32));
#elif defined(WIN32)
    /* Win32: use QueryPerformanceCounter - resolution depends on system, */
    /* but is expected to be better than 1 us. GetSystemTimeAsFileTime    */
    /* seems to have much worse resolution under Win95.                   */
    LARGE_INTEGER tmp;
    QueryPerformanceCounter(&tmp);
    return ((int_least64_t) tmp.LowPart + ((int_least64_t) tmp.HighPart << 32));
#elif defined(HAVE_GETTIMEOFDAY)
    /* UNIX: use gettimeofday() - allows 1 us resolution */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int_least64_t) tv.tv_usec
            + (int_least64_t) ((uint32_t) tv.tv_sec * (uint64_t) 1000000));
#else
    /* other systems: use time() - allows 1 second resolution */
    return ((int_least64_t) time(NULL));
#endif
  }

  /* function for getting CPU time */

  static inline int_least64_t get_CPU_time(void)
  {
    return ((int_least64_t) ((uint32_t) clock()));
  }

  /* initialise a timer structure */

  PUBLIC void csoundInitTimerStruct(RTCLOCK *p)
  {
    p->starttime_real = get_real_time();
    p->starttime_CPU = get_CPU_time();
  }

  /**
   * return the elapsed real time (in seconds) since the specified timer
   * structure was initialised
   */
  PUBLIC double csoundGetRealTime(RTCLOCK *p)
  {
    return ((double) (get_real_time() - p->starttime_real)
            * (double) timeResolutionSeconds);
  }

  /**
   * return the elapsed CPU time (in seconds) since the specified timer
   * structure was initialised
   */
  PUBLIC double csoundGetCPUTime(RTCLOCK *p)
  {
    return ((double) ((uint32_t) get_CPU_time() - (uint32_t) p->starttime_CPU)
            * (1.0 / (double) CLOCKS_PER_SEC));
  }

  /* return a 32-bit unsigned integer to be used as seed from current time */

  PUBLIC uint32_t csoundGetRandomSeedFromTime(void)
  {
    return (uint32_t) get_real_time();
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
  PUBLIC void **csoundGetRtRecordUserData(CSOUND *csound)
  {
    return &(csound->rtRecord_userdata);
  }

  /**
   * Return pointer to user data pointer for real time audio output.
   */
  PUBLIC void **csoundGetRtPlayUserData(CSOUND *csound)
  {
    return &(csound->rtPlay_userdata);
  }

  typedef struct opcodeDeinit_s {
    void    *p;
    int     (*func)(CSOUND *, void *);
    void    *nxt;
  } opcodeDeinit_t;

  /**
   * Register a function to be called at note deactivation.
   * Should be called from the initialisation routine of an opcode.
   * 'p' is a pointer to the OPDS structure of the opcode, and 'func'
   * is the function to be called, with the same arguments and return
   * value as in the case of opcode init/perf functions.
   * The functions are called in reverse order of registration.
   * Returns zero on success.
   */

  int csoundRegisterDeinitCallback(CSOUND *csound, void *p,
                                   int (*func)(CSOUND *, void *))
  {
    INSDS           *ip = ((OPDS*) p)->insdshead;
    opcodeDeinit_t  *dp = (opcodeDeinit_t*) malloc(sizeof(opcodeDeinit_t));

    (void) csound;
    if (dp == NULL)
      return CSOUND_MEMORY;
    dp->p = p;
    dp->func = func;
    dp->nxt = ip->nxtd;
    ip->nxtd = dp;
    return CSOUND_SUCCESS;
  }

  /**
   * Register a function to be called by csoundReset(), in reverse order
   * of registration, before unloading external modules. The function takes
   * the Csound instance pointer as the first argument, and the pointer
   * passed here as 'userData' as the second, and is expected to return zero
   * on success.
   * The return value of csoundRegisterResetCallback() is zero on success.
   */

  int csoundRegisterResetCallback(CSOUND *csound, void *userData,
                                  int (*func)(CSOUND *, void *))
  {
    resetCallback_t *dp = (resetCallback_t*) malloc(sizeof(resetCallback_t));

    if (dp == NULL)
      return CSOUND_MEMORY;
    dp->userData = userData;
    dp->func = func;
    dp->nxt = csound->reset_list;
    csound->reset_list = (void*) dp;
    return CSOUND_SUCCESS;
  }

  /* call the opcode deinitialisation routines of an instrument instance */
  /* called from deact() in insert.c */

  int csoundDeinitialiseOpcodes(CSOUND *csound, INSDS *ip)
  {
    int err = 0;

    while (ip->nxtd != NULL) {
      opcodeDeinit_t  *dp = (opcodeDeinit_t*) ip->nxtd;
      err |= dp->func(csound, dp->p);
      ip->nxtd = (void*) dp->nxt;
      free(dp);
    }
    return err;
  }

  /**
   * Returns the name of the opcode of which the data structure
   * is pointed to by 'p'.
   */
  char *csoundGetOpcodeName(void *p)
  {
    CSOUND *csound = (CSOUND*) ((OPDS*) p)->insdshead->csound;
    return (char*) csound->opcodlst[((OPDS*) p)->optext->t.opnum].opname;
  }

  /**
   * Returns the number of input arguments for opcode 'p'.
   */
  int csoundGetInputArgCnt(void *p)
  {
    return (int) ((OPDS*) p)->optext->t.inoffs->count;
  }

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a-rate, bit 1 is set if the second input argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgAMask(void *p)
  {
    return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xincod);
  }

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a string, bit 1 is set if the second input argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgSMask(void *p)
  {
    return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xincod_str);
  }

  /**
   * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetInputArgName(void *p, int n)
  {
    if ((unsigned int) n >= (unsigned int) ((OPDS*) p)->optext->t.inoffs->count)
      return (char*) NULL;
    return (char*) ((OPDS*) p)->optext->t.inlist->arg[n];
  }

  /**
   * Returns the number of output arguments for opcode 'p'.
   */
  int csoundGetOutputArgCnt(void *p)
  {
    return (int) ((OPDS*) p)->optext->t.outoffs->count;
  }

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a-rate, bit 1 is set if the second output argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgAMask(void *p)
  {
    return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xoutcod);
  }

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a string, bit 1 is set if the second output argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgSMask(void *p)
  {
    return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xoutcod_str);
  }

  /**
   * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetOutputArgName(void *p, int n)
  {
    if ((unsigned int) n
        >= (unsigned int) ((OPDS*) p)->optext->t.outoffs->count)
      return (char*) NULL;
    return (char*) ((OPDS*) p)->optext->t.outlist->arg[n];
  }

  /**
   * Set release time in control periods (1 / csound->ekr second units)
   * for opcode 'p' to 'n'. If the current release time is longer than
   * the specified value, it is not changed.
   * Returns the new release time.
   */
  int csoundSetReleaseLength(void *p, int n)
  {
    if (n > (int) ((OPDS*) p)->insdshead->xtratim)
      ((OPDS*) p)->insdshead->xtratim = n;
    return (int) ((OPDS*) p)->insdshead->xtratim;
  }

  /**
   * Set release time in seconds for opcode 'p' to 'n'.
   * If the current release time is longer than the specified value,
   * it is not changed.
   * Returns the new release time in seconds.
   */
  MYFLT csoundSetReleaseLengthSeconds(void *p, MYFLT n)
  {
    int kcnt = (int) (n * ((OPDS*) p)->insdshead->csound->ekr + FL(0.5));
    if (kcnt > (int) ((OPDS*) p)->insdshead->xtratim)
      ((OPDS*) p)->insdshead->xtratim = kcnt;
    return ((MYFLT) ((OPDS*) p)->insdshead->xtratim
            * ((OPDS*) p)->insdshead->csound->onedkr);
  }

  /**
   * Returns MIDI channel number (0 to 15) for the instrument instance
   * that called opcode 'p'.
   * In the case of score notes, -1 is returned.
   */
  int csoundGetMidiChannelNumber(void *p)
  {
    MCHNBLK *chn = ((OPDS*) p)->insdshead->m_chnbp;
    int     i;
    if (chn == NULL)
      return -1;
    for (i = 0; i < 16; i++) {
      if (chn == ((OPDS*) p)->insdshead->csound->m_chnbp[i])
        return i;
    }
    return -1;
  }

  /**
   * Returns a pointer to the MIDI channel structure for the instrument
   * instance that called opcode 'p'.
   * In the case of score notes, NULL is returned.
   */
  MCHNBLK *csoundGetMidiChannel(void *p)
  {
    return ((OPDS*) p)->insdshead->m_chnbp;
  }

  /**
   * Returns MIDI note number (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiNoteNumber(void *p)
  {
    return (int) ((OPDS*) p)->insdshead->m_pitch;
  }

  /**
   * Returns MIDI velocity (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiVelocity(void *p)
  {
    return (int) ((OPDS*) p)->insdshead->m_veloc;
  }

  /**
   * Returns non-zero if the current note (owning opcode 'p') is releasing.
   */
  int csoundGetReleaseFlag(void *p)
  {
    return (int) ((OPDS*) p)->insdshead->relesing;
  }

  /**
   * Returns the note-off time in seconds (measured from the beginning of
   * performance) of the current instrument instance, from which opcode 'p'
   * was called. The return value may be negative if the note has indefinite
   * duration.
   */
  double csoundGetOffTime(void *p)
  {
    return (double) ((OPDS*) p)->insdshead->offtim;
  }

  /**
   * Returns the array of p-fields passed to the instrument instance
   * that owns opcode 'p', starting from p0. Only p1, p2, and p3 are
   * guaranteed to be available. p2 is measured in seconds from the
   * beginning of the current section.
   */
  MYFLT *csoundGetPFields(void *p)
  {
    return (MYFLT*) &(((OPDS*) p)->insdshead->p0);
  }

  /**
   * Returns the instrument number (p1) for opcode 'p'.
   */
  int csoundGetInstrumentNumber(void *p)
  {
    return (int) ((OPDS*) p)->insdshead->p1;
  }

#ifdef __cplusplus
}
#endif

