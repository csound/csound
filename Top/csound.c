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

  /* from oload.c, initial state of ENVIRON structure */
  extern const ENVIRON cenviron_;
  /* from threads.c */
  void csoundLock(void);
  void csoundUnLock(void);
  /* aops.c */
  void aops_init_tables(void);

  typedef struct csInstance_s {
    ENVIRON             *csound;
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
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
    usleep(250000);
#elif defined(WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif
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
    if (!(flags & CSOUNDINIT_NO_SIGNAL_HANDLER))
      install_signal_handler();
    atexit(destroy_all_instances);
    aops_init_tables();
    csoundLock(); init_done = 1; csoundUnLock();
    return 0;
  }

  PUBLIC ENVIRON *csoundCreate(void *hostdata)
  {
    ENVIRON       *csound;
    csInstance_t  *p;
    if (init_done != 1) {
      int argc = 1;
      char *argv_[2] = { "csound", NULL };
      char **argv = &(argv_[0]);
      if (csoundInitialize(&argc, &argv, 0) != 0)
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
    p->nxt = (csInstance_t*) instance_list;
    instance_list = p;
    csoundUnLock();
    csoundReset(csound);
    return csound;
  }

  /* dummy real time MIDI functions */
  static int DummyMidiInOpen(ENVIRON *csound, void **userData,
                             const char *devName);
  static int DummyMidiRead(ENVIRON *csound, void *userData,
                           unsigned char *buf, int nbytes);
  static int DummyMidiInClose(ENVIRON *csound, void *userData);
  static int DummyMidiOutOpen(ENVIRON *csound, void **userData,
                              const char *devName);
  static int DummyMidiWrite(ENVIRON *csound, void *userData,
                            unsigned char *buf, int nbytes);
  static int DummyMidiOutClose(ENVIRON *csound, void *userData);
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
  PUBLIC int csoundPreCompile(ENVIRON *p)
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
    if (i != CSOUND_SUCCESS)
      return i;
    /* allow selecting real time audio module */
    max_len = 21;
    csoundCreateGlobalVariable(p, "_RTAUDIO", (size_t) max_len);
    s = csoundQueryGlobalVariable(p, "_RTAUDIO");
    strcpy(s, "PortAudio");
    csoundCreateConfigurationVariable(p, "rtaudio", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
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
    csoundCreateConfigurationVariable(p, "rtmidi", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Real time MIDI module name", NULL);
    max_len = 256;  /* should be the same as in csoundCore.h */
    csoundCreateConfigurationVariable(p, "mute_tracks",
                                      &(p->midiGlobals->muteTrackList[0]),
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Ignore events (other than tempo "
                                      "changes) in tracks defined by pattern",
                                      NULL);
    csoundCreateConfigurationVariable(p, "raw_controller_mode",
                                      &(p->midiGlobals->rawControllerMode),
                                      CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                      "Do not handle special MIDI controllers "
                                      "(sustain pedal etc.)", NULL);
    /* sound file tag options */
    max_len = 201;
    i = -1;
    while (id_option_table[++i][0] != NULL) {
      csoundCreateGlobalVariable(p, id_option_table[i][0], (size_t) max_len);
      csoundCreateConfigurationVariable(
                    p, id_option_table[i][1],
                    csoundQueryGlobalVariable(p, id_option_table[i][0]),
                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                    id_option_table[i][2], NULL);
    }
    /* max. length of string variables */
    {
      int minVal = 10;
      int maxVal = 10000;
      csoundCreateConfigurationVariable(p, "max_str_len",
                                        &(p->strVarMaxLen),
                                        CSOUNDCFG_INTEGER, 0, &minVal, &maxVal,
                                        "Maximum length of "
                                        "string variables + 1", NULL);
    }
    csoundCreateConfigurationVariable(p, "msg_color", &(p->enableMsgAttr),
                                      CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                      "Enable message attributes (colors etc.)",
                                      NULL);
    {
      MYFLT minVal = FL(0.0);
      csoundCreateConfigurationVariable(p, "skip_seconds",
                                        &(p->csoundScoreOffsetSeconds_),
                                        CSOUNDCFG_MYFLT, 0, &minVal, NULL,
                                        "Start score playback at the specified "
                                        "time, skipping earlier events", NULL);
    }
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

  PUBLIC void csoundDestroy(ENVIRON *csound)
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

  PUBLIC void *csoundGetHostData(ENVIRON *csound)
  {
    return csound->hostdata;
  }

  PUBLIC void csoundSetHostData(ENVIRON *csound, void *hostData)
  {
    csound->hostdata = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int sensevents(ENVIRON *);

  /**
   * perform currently active instrs for one kperiod
   *      & send audio result to output buffer
   * returns non-zero if this kperiod was skipped
   */

  static inline int kperf(ENVIRON *csound)
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

  PUBLIC int csoundPerformKsmps(ENVIRON *csound)
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

  PUBLIC int csoundPerformKsmpsAbsolute(ENVIRON *csound)
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

  PUBLIC int csoundPerformBuffer(ENVIRON *csound)
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

  PUBLIC MYFLT csoundGetSr(ENVIRON *csound)
  {
    return csound->esr;
  }

  PUBLIC MYFLT csoundGetKr(ENVIRON *csound)
  {
    return csound->ekr;
  }

  PUBLIC int csoundGetKsmps(ENVIRON *csound)
  {
    return csound->ksmps;
  }

  PUBLIC int csoundGetNchnls(ENVIRON *csound)
  {
    return csound->nchnls;
  }

  PUBLIC MYFLT csoundGet0dBFS(ENVIRON *csound)
  {
    return csound->e0dbfs;
  }

  PUBLIC int csoundGetSampleFormat(ENVIRON *csound)
  {
    /* should we assume input is same as output ? */
    return csound->oparms->outformat;
  }

  PUBLIC int csoundGetSampleSize(ENVIRON *csound)
  {
    /* should we assume input is same as output ? */
    return csound->oparms->sfsampsize;
  }

  PUBLIC long csoundGetInputBufferSize(ENVIRON *csound)
  {
    return csound->oparms->inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(ENVIRON *csound)
  {
    return csound->oparms->outbufsamps;
  }

  PUBLIC MYFLT *csoundGetSpin(ENVIRON *csound)
  {
    return csound->spin;
  }

  PUBLIC MYFLT *csoundGetSpout(ENVIRON *csound)
  {
    return csound->spout;
  }

  PUBLIC const char *csoundGetOutputFileName(ENVIRON *csound)
  {
    return (const char*) csound->oparms->outfilename;
  }

  PUBLIC MYFLT csoundGetScoreTime(ENVIRON *csound)
  {
    return (MYFLT) csound->curTime;
  }

  PUBLIC MYFLT csoundGetProgress(ENVIRON *csound)
  {
    return -1;
  }

  PUBLIC MYFLT csoundGetProfile(ENVIRON *csound)
  {
    return -1;
  }

  PUBLIC MYFLT csoundGetCpuUsage(ENVIRON *csound)
  {
    return -1;
  }

  /*
   * SCORE HANDLING
   */

  PUBLIC int csoundIsScorePending(ENVIRON *csound)
  {
    return csound->csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(ENVIRON *csound, int pending)
  {
    csound->csoundIsScorePending_ = pending;
  }

  PUBLIC void csoundSetScoreOffsetSeconds(ENVIRON *csound, MYFLT offset)
  {
    double  aTime;
    MYFLT   prv = (MYFLT) csound->csoundScoreOffsetSeconds_;

    csound->csoundScoreOffsetSeconds_ = offset;
    if (offset < FL(0.0))
      return;
    /* if csoundCompile() was not called yet, just store the offset */
    if (csound->QueryGlobalVariable(csound, "csRtClock") == NULL)
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
      insert_score_event(csound, &evt, csound->curTime, 0);
    }
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(ENVIRON *csound)
  {
    return csound->csoundScoreOffsetSeconds_;
  }

  extern void musmon_rewind_score(ENVIRON *csound);     /* musmon.c */
  extern void midifile_rewind_score(ENVIRON *csound);   /* midifile.c */

  PUBLIC void csoundRewindScore(ENVIRON *csound)
  {
    musmon_rewind_score(csound);
    midifile_rewind_score(csound);
  }

  void csoundDefaultMessageCallback(ENVIRON *csound, int attr,
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

  void csoundDefaultThrowMessageCallback(ENVIRON *csound,
                                         const char *format, va_list args)
  {
    csoundDefaultMessageCallback(csound, CSOUNDMSG_ERROR, format, args);
  }

  PUBLIC void csoundSetMessageCallback(ENVIRON *csound,
                            void (*csoundMessageCallback)(ENVIRON *csound,
                                                          int attr,
                                                          const char *format,
                                                          va_list args))
  {
    csound->csoundMessageCallback_ = csoundMessageCallback;
  }

  PUBLIC void csoundMessageV(ENVIRON *csound,
                             int attr, const char *format, va_list args)
  {
    csound->csoundMessageCallback_(csound, attr, format, args);
  }

  PUBLIC void csoundMessage(ENVIRON *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundMessageCallback_(csound, 0, format, args);
    va_end(args);
  }

  PUBLIC void csoundMessageS(ENVIRON *csound, int attr, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundMessageCallback_(csound, attr, format, args);
    va_end(args);
  }

  void csoundDie(ENVIRON *csound, const char *msg, ...)
  {
    va_list args;
    va_start(args, msg);
    csound->ErrMsgV(csound, (char*) 0, msg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
  }

  void csoundWarning(ENVIRON *csound, const char *msg, ...)
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

  void csoundDebugMsg(ENVIRON *csound, const char *msg, ...)
  {
    va_list args;
    if (!(csound->oparms->odebug))
      return;
    va_start(args, msg);
    csound->csoundMessageCallback_(csound, 0, msg, args);
    va_end(args);
    csoundMessage(csound, "\n");
  }

  void csoundErrorMsg(ENVIRON *csound, const char *msg, ...)
  {
    va_list args;
    va_start(args, msg);
    csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
    va_end(args);
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundErrMsgV(ENVIRON *csound,
                     const char *hdr, const char *msg, va_list args)
  {
    if (hdr != NULL)
      csound->MessageS(csound, CSOUNDMSG_ERROR, "%s", hdr);
    csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundLongJmp(ENVIRON *csound, int retval)
  {
    int n = CSOUND_EXITJMP_SUCCESS;
    n = (retval < 0 ? n + retval : n - retval) & (CSOUND_EXITJMP_SUCCESS - 1);
    if (!n)
      n = CSOUND_EXITJMP_SUCCESS;
    longjmp(csound->exitjmp, n);
  }

  PUBLIC void csoundSetThrowMessageCallback(ENVIRON *csound,
                    void (*csoundThrowMessageCallback)(ENVIRON *csound,
                                                       const char *format,
                                                       va_list args))
  {
    csound->csoundThrowMessageCallback_ = csoundThrowMessageCallback;
  }

  PUBLIC void csoundThrowMessageV(ENVIRON *csound,
                                  const char *format, va_list args)
  {
    csound->csoundThrowMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundThrowMessage(ENVIRON *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csound->csoundThrowMessageCallback_(csound, format, args);
    va_end(args);
  }

  PUBLIC int csoundGetMessageLevel(ENVIRON *csound)
  {
    return csound->oparms->msglevel;
  }

  PUBLIC void csoundSetMessageLevel(ENVIRON *csound, int messageLevel)
  {
    csound->oparms->msglevel = messageLevel;
  }

  PUBLIC void csoundInputMessage(ENVIRON *csound, const char *message)
  {
    writeLine(csound, message, strlen(message));
  }

  PUBLIC void csoundKeyPress(ENVIRON *csound, char c)
  {
    csound->inChar_ = (int) ((unsigned char) c);
  }

  char getChar(ENVIRON *csound)
  {
    char  c = (char) csound->inChar_;
    csound->inChar_ = 0;
    return c;
  }

  /*
   * CONTROL AND EVENTS
   */

  PUBLIC void
    csoundSetInputValueCallback(ENVIRON *csound,
                                void (*inputValueCalback)(ENVIRON *csound,
                                                          char *channelName,
                                                          MYFLT *value))
  {
    csound->InputValueCallback_ = inputValueCalback;
  }

  PUBLIC void
    csoundSetOutputValueCallback(ENVIRON *csound,
                                 void (*outputValueCalback)(ENVIRON *csound,
                                                            char *channelName,
                                                            MYFLT value))
  {
    csound->OutputValueCallback_ = outputValueCalback;
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

  PUBLIC int csoundScoreEvent(ENVIRON *csound, char type,
                              MYFLT *pfields, long numFields)
  {
    EVTBLK        evt;
    int           i;

    evt.strarg = NULL;
    evt.opcod = type;
    evt.pcnt = (short) numFields;
    for (i = 0; i < (int) numFields; i++)
      evt.p[i + 1] = pfields[i];
    return insert_score_event(csound, &evt, csound->curTime, 0);
  }

  /*
   *    REAL-TIME AUDIO
   */

/* dummy functions for the case when no real-time audio module is available */

static double *get_dummy_rtaudio_globals(ENVIRON *csound)
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

static void dummy_rtaudio_timer(ENVIRON *csound, double *p)
{
    double  timeWait;
    RTCLOCK *rt;
    int     i;

    rt = (RTCLOCK*) csoundQueryGlobalVariableNoCheck(csound, "csRtClock");
    timeWait = p[0] - timers_get_real_time(rt);
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
    i = (int) (timeWait * 1000000.0 + 0.5);
    if (i > 0)
      usleep((unsigned long) i);
#elif defined(WIN32) || defined(_WIN32)
    i = (int) (timeWait * 1000.0 + 0.5);
    if (i > 0)
      Sleep((DWORD) i);
#endif
}

int playopen_dummy(ENVIRON *csound, csRtAudioParams *parm)
{
    double  *p;
    char    *s;
    RTCLOCK *rt;

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
    p = get_dummy_rtaudio_globals(csound);
    csound->rtPlay_userdata = (void*) p;
    rt = (RTCLOCK*) csound->QueryGlobalVariable(csound, "csRtClock");
    p[0] = csound->timers_get_real_time(rt);
    p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                  * (double) parm->sampleRate);
    return CSOUND_SUCCESS;
}

void rtplay_dummy(ENVIRON *csound, void *outBuf, int nbytes)
{
    double  *p = (double*) csound->rtPlay_userdata;
    outBuf = outBuf;
    p[0] += ((double) nbytes * p[1]);
    dummy_rtaudio_timer(csound, p);
}

int recopen_dummy(ENVIRON *csound, csRtAudioParams *parm)
{
    double  *p;
    char    *s;
    RTCLOCK *rt;

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
    p = (double*) get_dummy_rtaudio_globals(csound) + 2;
    csound->rtRecord_userdata = (void*) p;
    rt = (RTCLOCK*) csound->QueryGlobalVariable(csound, "csRtClock");
    p[0] = csound->timers_get_real_time(rt);
    p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                  * (double) parm->sampleRate);
    return CSOUND_SUCCESS;
}

int rtrecord_dummy(ENVIRON *csound, void *inBuf, int nbytes)
{
    double  *p = (double*) csound->rtRecord_userdata;
    int     i;

    for (i = 0; i < (nbytes / (int) sizeof(MYFLT)); i++)
      ((MYFLT*) inBuf)[i] = FL(0.0);

    p[0] += ((double) nbytes * p[1]);
    dummy_rtaudio_timer(csound, p);

    return nbytes;
}

void rtclose_dummy(ENVIRON *csound)
{
    csound->rtPlay_userdata = NULL;
    csound->rtRecord_userdata = NULL;
}

PUBLIC void csoundSetPlayopenCallback(ENVIRON *csound,
                                      int (*playopen__)(ENVIRON *csound,
                                                        csRtAudioParams *parm))
{
    csound->playopen_callback = playopen__;
}

PUBLIC void csoundSetRtplayCallback(ENVIRON *csound,
                                    void (*rtplay__)(ENVIRON *csound,
                                                     void *outBuf, int nbytes))
{
    csound->rtplay_callback = rtplay__;
}

PUBLIC void csoundSetRecopenCallback(ENVIRON *csound,
                                     int (*recopen__)(ENVIRON *csound,
                                                      csRtAudioParams *parm))
{
    csound->recopen_callback = recopen__;
}

PUBLIC void csoundSetRtrecordCallback(ENVIRON *csound,
                                      int (*rtrecord__)(ENVIRON *csound,
                                                        void *inBuf,
                                                        int nbytes))
{
    csound->rtrecord_callback = rtrecord__;
}

PUBLIC void csoundSetRtcloseCallback(ENVIRON *csound,
                                     void (*rtclose__)(ENVIRON *csound))
{
    csound->rtclose_callback = rtclose__;
}

/* dummy real time MIDI functions */

static int DummyMidiInOpen(ENVIRON *csound,
                           void **userData, const char *devName)
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

static int DummyMidiRead(ENVIRON *csound,
                         void *userData, unsigned char *buf, int nbytes)
{
    return 0;
}

static int DummyMidiInClose(ENVIRON *csound, void *userData)
{
    return 0;
}

static int DummyMidiOutOpen(ENVIRON *csound,
                            void **userData, const char *devName)
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

static int DummyMidiWrite(ENVIRON *csound,
                          void *userData, unsigned char *buf, int nbytes)
{
    return nbytes;
}

static int DummyMidiOutClose(ENVIRON *csound, void *userData)
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
int csoundExternalMidiInOpen(ENVIRON *csound, void **userData,
                             const char *devName)
{
    if (csound->midiGlobals->MidiInOpenCallback == NULL)
      return -1;
    return (csound->midiGlobals->MidiInOpenCallback(csound, userData, devName));
}

/**
 * Read at most 'nbytes' bytes of MIDI data from input stream
 * 'userData', and store in 'buf'. Returns the actual number of
 * bytes read, which may be zero if there were no events, and
 * negative in case of an error. Note: incomplete messages (such
 * as a note on status without the data bytes) should not be
 * returned.
 */
int csoundExternalMidiRead(ENVIRON *csound, void *userData,
                           unsigned char *buf, int nbytes)
{
    if (csound->midiGlobals->MidiReadCallback == NULL)
      return -1;
    return (csound->midiGlobals->MidiReadCallback(csound,
                                                  userData, buf, nbytes));
}

/**
 * Close MIDI input device associated with 'userData'.
 * Return value is zero on success, and a non-zero error
 * code on failure.
 */
int csoundExternalMidiInClose(ENVIRON *csound, void *userData)
{
    int retval;
    if (csound->midiGlobals->MidiInCloseCallback == NULL)
      return -1;
    retval = csound->midiGlobals->MidiInCloseCallback(csound, userData);
    csound->midiGlobals->midiInUserData = NULL;
    return retval;
}

/**
 * Open MIDI output device 'devName', and store stream specific
 * data pointer in *userData. Return value is zero on success,
 * and a non-zero error code if an error occured.
 */
int csoundExternalMidiOutOpen(ENVIRON *csound, void **userData,
                              const char *devName)
{
    if (csound->midiGlobals->MidiOutOpenCallback == NULL)
      return -1;
    return (csound->midiGlobals->MidiOutOpenCallback(csound,
                                                     userData, devName));
}

/**
 * Write 'nbytes' bytes of MIDI data to output stream 'userData'
 * from 'buf' (the buffer will not contain incomplete messages).
 * Returns the actual number of bytes written, or a negative
 * error code.
 */
int csoundExternalMidiWrite(ENVIRON *csound, void *userData,
                            unsigned char *buf, int nbytes)
{
    if (csound->midiGlobals->MidiWriteCallback == NULL)
      return -1;
    return (csound->midiGlobals->MidiWriteCallback(csound,
                                                   userData, buf, nbytes));
}

/**
 * Close MIDI output device associated with '*userData'.
 * Return value is zero on success, and a non-zero error
 * code on failure.
 */
int csoundExternalMidiOutClose(ENVIRON *csound, void *userData)
{
    int retval;
    if (csound->midiGlobals->MidiOutCloseCallback == NULL)
      return -1;
    retval = csound->midiGlobals->MidiOutCloseCallback(csound, userData);
    csound->midiGlobals->midiOutUserData = NULL;
    return retval;
}

/**
 * Returns pointer to a string constant storing an error massage
 * for error code 'errcode'.
 */
char *csoundExternalMidiErrorString(ENVIRON *csound, int errcode)
{
    if (csound->midiGlobals->MidiErrorStringCallback == NULL)
      return NULL;
    return (csound->midiGlobals->MidiErrorStringCallback(errcode));
}

/* Set real time MIDI function pointers. */

PUBLIC void csoundSetExternalMidiInOpenCallback(ENVIRON *csound,
                                                int (*func)(ENVIRON*, void**,
                                                            const char*))
{
    csound->midiGlobals->MidiInOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiReadCallback(ENVIRON *csound,
                                              int (*func)(ENVIRON*, void*,
                                                          unsigned char*, int))
{
    csound->midiGlobals->MidiReadCallback = func;
}

PUBLIC void csoundSetExternalMidiInCloseCallback(ENVIRON *csound,
                                                 int (*func)(ENVIRON*, void*))
{
    csound->midiGlobals->MidiInCloseCallback = func;
}

PUBLIC void csoundSetExternalMidiOutOpenCallback(ENVIRON *csound,
                                                 int (*func)(ENVIRON*, void**,
                                                             const char*))
{
    csound->midiGlobals->MidiOutOpenCallback = func;
}

PUBLIC void csoundSetExternalMidiWriteCallback(ENVIRON *csound,
                                               int (*func)(ENVIRON*, void*,
                                                           unsigned char*, int))
{
    csound->midiGlobals->MidiWriteCallback = func;
}

PUBLIC void csoundSetExternalMidiOutCloseCallback(ENVIRON *csound,
                                                  int (*func)(ENVIRON*, void*))
{
    csound->midiGlobals->MidiOutCloseCallback = func;
}

PUBLIC void csoundSetExternalMidiErrorStringCallback(ENVIRON *csound,
                                                     char *(*func)(int))
{
    csound->midiGlobals->MidiErrorStringCallback = func;
}

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  PUBLIC void csoundSetIsGraphable(ENVIRON *csound, int isGraphable)
  {
    csound->isGraphable_ = isGraphable;
  }

  int Graphable(ENVIRON *csound)
  {
    return csound->isGraphable_;
  }

  void defaultCsoundMakeGraph(ENVIRON *csound, WINDAT *windat, char *name)
  {
#if defined(USE_FLTK)
    extern void MakeGraph_(ENVIRON *, WINDAT *, char *);
    MakeGraph_(csound, windat, name);
#else
    extern void MakeAscii(ENVIRON *, WINDAT *, char *);
    MakeAscii(csound, windat, name);
#endif
  }

  PUBLIC void csoundSetMakeGraphCallback(ENVIRON *csound,
                                    void (*makeGraphCallback)(ENVIRON *csound,
                                                              WINDAT *windat,
                                                              char *name))
  {
    csound->csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(ENVIRON *csound, WINDAT *windat, char *name)
  {
    csound->csoundMakeGraphCallback_(csound, windat, name);
  }

  void defaultCsoundDrawGraph(ENVIRON *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void DrawGraph_(ENVIRON *, WINDAT *);
    DrawGraph_(csound, windat);
#else
    extern void DrawAscii(ENVIRON *, WINDAT *);
    DrawAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetDrawGraphCallback(ENVIRON *csound,
                                    void (*drawGraphCallback)(ENVIRON *csound,
                                                              WINDAT *windat))
  {
    csound->csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(ENVIRON *csound, WINDAT *windat)
  {
    csound->csoundDrawGraphCallback_(csound, windat);
  }

  void defaultCsoundKillGraph(ENVIRON *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void KillGraph_(ENVIRON *, WINDAT *);
    KillGraph_(csound, windat);
#else
    extern void KillAscii(ENVIRON *, WINDAT *);
    KillAscii(csound, windat);
#endif
  }

  PUBLIC void csoundSetKillGraphCallback(ENVIRON *csound,
                                    void (*killGraphCallback)(ENVIRON *csound,
                                                              WINDAT *windat))
  {
    csound->csoundKillGraphCallback_ = killGraphCallback;
  }

  void KillGraph(ENVIRON *csound, WINDAT *windat)
  {
    csound->csoundKillGraphCallback_(csound, windat);
  }

  int defaultCsoundExitGraph(ENVIRON *csound)
  {
    return CSOUND_SUCCESS;
  }

  PUBLIC void csoundSetExitGraphCallback(ENVIRON *csound,
                                         int (*exitGraphCallback)(ENVIRON *))
  {
    csound->csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph(ENVIRON *csound)
  {
    return csound->csoundExitGraphCallback_(csound);
  }

  void MakeXYin(ENVIRON *csound, XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
    csoundMessage(csound, "xyin not supported. use invalue opcode instead.\n");
  }

  void ReadXYin(ENVIRON *csound, XYINDAT *xyindat)
  {
    csoundMessage(csound, "xyin not supported. use invlaue opcodes instead.\n");
  }

  /*
   * OPCODES
   */

  PUBLIC opcodelist *csoundNewOpcodeList(void)
  {
    /* create_opcodlst();
       return new_opcode_list(); */

    return NULL;
  }

  PUBLIC void csoundDisposeOpcodeList(opcodelist *list)
  {
    /* dispose_opcode_list(list); */
  }

  static inline int opcode_list_new_oentry(ENVIRON *csound, OENTRY *ep)
  {
    int     oldCnt = 0;

    if (csound->opcodlst != NULL)
      oldCnt = (int) ((OENTRY*) csound->oplstend - (OENTRY*) csound->opcodlst);
    if (!(oldCnt & 0x7F)) {
      OENTRY  *newList;
      size_t  nBytes = (size_t) (oldCnt + 0x80) * sizeof(OENTRY);
      if (!oldCnt)
        newList = (OENTRY*) csound->Malloc(csound, nBytes);
      else
        newList = (OENTRY*) csound->ReAlloc(csound, csound->opcodlst, nBytes);
      if (newList == NULL)
        return -1;
      csound->opcodlst = newList;
      csound->oplstend = ((OENTRY*) newList + (int) oldCnt);
      memset(&(((OENTRY*) csound->opcodlst)[oldCnt]), 0, sizeof(OENTRY) * 0x80);
    }
    memcpy(&(((OENTRY*) csound->opcodlst)[oldCnt]), ep, sizeof(OENTRY));
    csound->oplstend = (OENTRY*) csound->oplstend + (int) 1;
    return 0;
  }

  PUBLIC int csoundAppendOpcode(ENVIRON *csound,
                                char *opname, int dsblksiz, int thread,
                                char *outypes, char *intypes,
                                int (*iopadr)(ENVIRON *, void *),
                                int (*kopadr)(ENVIRON *, void *),
                                int (*aopadr)(ENVIRON *, void *))
  {
    OENTRY  tmpEntry;

    tmpEntry.opname     = opname;
    tmpEntry.dsblksiz   = (unsigned short) dsblksiz;
    tmpEntry.thread     = (unsigned short) thread;
    tmpEntry.outypes    = outypes;
    tmpEntry.intypes    = intypes;
    tmpEntry.iopadr     = (SUBR) iopadr;
    tmpEntry.kopadr     = (SUBR) kopadr;
    tmpEntry.aopadr     = (SUBR) aopadr;
    tmpEntry.useropinfo = NULL;
    tmpEntry.prvnum     = 0;
    if (opcode_list_new_oentry(csound, &tmpEntry) != 0) {
      csoundErrorMsg(csound, Str("Failed to allocate new opcode entry."));
      return -1;
    }
    return 0;
  }

  /**
   * Appends a list of opcodes implemented by external software to Csound's
   * internal opcode list. The list should either be terminated with an entry
   * that has a NULL opname, or the number of entries (> 0) should be specified
   * in 'n'. Returns zero on success.
   */

  PUBLIC int csoundAppendOpcodes(ENVIRON *csound,
                                 const OENTRY *opcodeList, int n)
  {
    OENTRY  *ep = (OENTRY*) opcodeList;
    int     retval = 0;

    if (opcodeList == NULL)
      return -1;
    if (n <= 0)
      n = 0x7FFFFFFF;
    while (n && ep->opname != NULL) {
      if (opcode_list_new_oentry(csound, ep) != 0) {
        csoundErrorMsg(csound, Str("Failed to allocate opcode entry for %s."),
                               ep->opname);
        retval = -1;
      }
      else {
        ((OENTRY*) csound->oplstend - 1)->useropinfo = NULL;
        ((OENTRY*) csound->oplstend - 1)->prvnum = 0;
      }
      n--, ep++;
    }
    return retval;
  }

  int csoundOpcodeCompare(const void *v1, const void *v2)
  {
    return strcmp(((OENTRY*)v1)->opname, ((OENTRY*)v2)->opname);
  }

  /*
   * MISC FUNCTIONS
   */

#if !defined(USE_FLTK)
  int defaultCsoundYield(ENVIRON *csound)
  {
    return 1;
  }
#endif

  void csoundSetYieldCallback(ENVIRON *csound, int (*yieldCallback)(ENVIRON *))
  {
    csound->csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(ENVIRON *csound)
  {
    if (exitNow_)
      csound->LongJmp(csound, CSOUND_SIGNAL);
    return csound->csoundYieldCallback_(csound);
  }

  extern void csoundDeleteAllGlobalVariables(ENVIRON *csound);
  extern int  csoundUnloadExternals(ENVIRON *csound);

  typedef struct resetCallback_s {
    void    *userData;
    int     (*func)(ENVIRON *, void *);
    struct resetCallback_s  *nxt;
  } resetCallback_t;

  extern void adsynRESET(ENVIRON *);
  extern void cscoreRESET(ENVIRON *);
  extern void disprepRESET(ENVIRON *);
  extern void lpcRESET(ENVIRON *);
  extern void memRESET(ENVIRON *);
  extern void oloadRESET(ENVIRON *);
  extern void orchRESET(ENVIRON *);
  extern void tranRESET(ENVIRON *);

  PUBLIC void csoundReset(ENVIRON *csound)
  {
    csoundCleanup(csound);

    /* call registered reset callbacks */
    while (csound->reset_list != NULL) {
      resetCallback_t *p = (resetCallback_t*) csound->reset_list;
      resetCallback_t *nxt = (resetCallback_t*) p->nxt;
      p->func(csound, p->userData);
      free(p);
      csound->reset_list = (void*) nxt;
    }

    /* unload plugin opcodes */
    csoundUnloadExternals(csound);
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
    orchRESET(csound);
    adsynRESET(csound);
    lpcRESET(csound);
    oloadRESET(csound);     /* should be called last but one */
    memRESET(csound);       /* and this one should be the last */
  }

  PUBLIC int csoundGetDebug(ENVIRON *csound)
  {
    return csound->oparms->odebug;
  }

  PUBLIC void csoundSetDebug(ENVIRON *csound, int debug)
  {
    csound->oparms->odebug = debug;
  }

  PUBLIC int csoundTableLength(ENVIRON *csound, int table)
  {
    int tableLength;
    csound->GetTable(csound, table, &tableLength);
    return tableLength;
  }

  PUBLIC MYFLT csoundTableGet(ENVIRON *csound, int table, int index)
  {
    return ((csound->flist[table])->ftable[index]);
  }

  PUBLIC void csoundTableSet(ENVIRON *csound, int table, int index, MYFLT value)
  {
    (csound->flist[table])->ftable[index] = value;
  }

  PUBLIC void csoundSetFLTKThreadLocking(ENVIRON *csound, int isLocking)
  {
    csound->doFLTKThreadLocking = isLocking;
  }

  PUBLIC int csoundGetFLTKThreadLocking(ENVIRON *csound)
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
    /* other systems: use ANSI C time() - allows 1 second resolution */
    return ((int_least64_t) time(NULL));
#endif
}

/* function for getting CPU time */

static inline int_least64_t get_CPU_time(void)
{
    return ((int_least64_t) ((uint32_t) clock()));
}

/* initialise a timer structure */

void timers_struct_init(RTCLOCK *p)
{
    p->starttime_real = get_real_time();
    p->starttime_CPU = get_CPU_time();
}

/* return the elapsed real time (in seconds) since the specified timer */
/* structure was initialised */

double timers_get_real_time(RTCLOCK *p)
{
    return ((double) (get_real_time() - p->starttime_real)
            * (double) timeResolutionSeconds);
}

/* return the elapsed CPU time (in seconds) since the specified timer */
/* structure was initialised */

double timers_get_CPU_time(RTCLOCK *p)
{
    return ((double) ((uint32_t) get_CPU_time() - (uint32_t) p->starttime_CPU)
            * (1.0 / (double) CLOCKS_PER_SEC));
}

/* return a 32-bit unsigned integer to be used as seed from current time */

unsigned long timers_random_seed(void)
{
    return (unsigned long) ((uint32_t) get_real_time());
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
PUBLIC void **csoundGetRtRecordUserData(ENVIRON *csound)
{
    return &(csound->rtRecord_userdata);
}

/**
 * Return pointer to user data pointer for real time audio output.
 */
PUBLIC void **csoundGetRtPlayUserData(ENVIRON *csound)
{
    return &(csound->rtPlay_userdata);
}

typedef struct opcodeDeinit_s {
    void    *p;
    int     (*func)(ENVIRON *, void *);
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

int csoundRegisterDeinitCallback(ENVIRON *csound, void *p,
                                 int (*func)(ENVIRON *, void *))
{
    INSDS           *ip = ((OPDS*) p)->insdshead;
    opcodeDeinit_t  *dp = (opcodeDeinit_t*) malloc(sizeof(opcodeDeinit_t));

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

int csoundRegisterResetCallback(ENVIRON *csound, void *userData,
                                int (*func)(ENVIRON *, void *))
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

int csoundDeinitialiseOpcodes(ENVIRON *csound, INSDS *ip)
{
    int err = 0;

    while (ip->nxtd != NULL) {
      opcodeDeinit_t  *dp = (opcodeDeinit_t*) ip->nxtd;
      opcodeDeinit_t  *nxt = dp->nxt;
      err |= dp->func(csound, dp->p);
      free(ip->nxtd);
      ip->nxtd = (void*) nxt;
    }
    return err;
}

  /**
   * Returns the name of the opcode of which the data structure
   * is pointed to by 'p'.
   */
  char *csoundGetOpcodeName(void *p)
  {
    ENVIRON *csound = (ENVIRON*) ((OPDS*) p)->insdshead->csound;
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
};
#endif

