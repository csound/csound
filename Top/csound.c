/*
 * C S O U N D
 *
 * An auto-extensible system for making music on computers by means of software alone.
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

#include <stdio.h>
#include <stdarg.h>
#include "csound.h"
#include "csoundCore.h"
#include "prototyp.h"
#include "csmodule.h"

  int fltk_abort = 0;
#ifdef INGALLS
  static jmp_buf csoundJump_;
#endif
#define csoundMaxExits 64
  static void* csoundExitFuncs_[csoundMaxExits];
  static long csoundNumExits_ = -1;

  extern ENVIRON cenviron_;

  PUBLIC void *csoundCreate(void *hostdata)
  {
    /* FIXME: should use malloc() eventually */
    ENVIRON *csound = &cenviron;
    memcpy(csound, &cenviron_, sizeof(ENVIRON));
    csoundReset(csound);
    csound->hostdata_ = hostdata;
    return csound;
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
    char    *s, *s2;
    int     i, max_len;

    p = (ENVIRON*) csound;
    /* reset instance, but keep host data pointer */
    saved_hostdata = p->hostdata_;
    csoundReset(csound);
    p->hostdata_ = saved_hostdata;
    /* allow selecting real time audio module */
    max_len = 21;
    csoundCreateGlobalVariable(csound, "_RTAUDIO", (size_t) max_len);
    s = csoundQueryGlobalVariable(csound, "_RTAUDIO");
    s2 = getenv("CSRTAUDIO");
    if (s2 != NULL && s2[0] != '\0') {
      if ((int) strlen(s2) < max_len) {
        strcpy(s, s2);
      }
      else {
        p->Message(csound, Str("WARNING: CSRTAUDIO='%s' is too long, using "
                               "PortAudio as default instead\n"), s2);
        strcpy(s, "PortAudio");
      }
    }
    else {
      strcpy(s, "PortAudio");
    }
    csoundCreateConfigurationVariable(csound, "rtaudio", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Real time audio module name", NULL);
    /* initialise real time MIDI */
    p->midiGlobals = (MGLOBAL*) mcalloc(csound, sizeof(MGLOBAL));
    p->midiGlobals->Midevtblk = (MEVENT*) NULL;
    p->midiGlobals->FMidevtblk = (MEVENT*) NULL;
    for (i = 0; i < 128; i++)
      p->midiGlobals->pgm2ins[i] = (i + 1);
    csoundSetExternalMidiInOpenCallback(csound, DummyMidiInOpen);
    csoundSetExternalMidiReadCallback(csound, DummyMidiRead);
    csoundSetExternalMidiInCloseCallback(csound, DummyMidiInClose);
    csoundSetExternalMidiOutOpenCallback(csound, DummyMidiOutOpen);
    csoundSetExternalMidiWriteCallback(csound, DummyMidiWrite);
    csoundSetExternalMidiOutCloseCallback(csound, DummyMidiOutClose);
    csoundSetExternalMidiErrorStringCallback(csound, DummyMidiErrorString);
    p->midiGlobals->midiInUserData = NULL;
    p->midiGlobals->midiOutUserData = NULL;
    p->midiGlobals->mfp = (FILE*) NULL;
    p->midiGlobals->bufp = &(p->midiGlobals->mbuf[0]);
    p->midiGlobals->endatp = p->midiGlobals->bufp;
    csoundCreateGlobalVariable(csound, "_RTMIDI", (size_t) max_len);
    s = csoundQueryGlobalVariable(csound, "_RTMIDI");
    strcpy(s, "PortMIDI");
    csoundCreateConfigurationVariable(csound, "rtmidi", s,
                                      CSOUNDCFG_STRING, 0, NULL, &max_len,
                                      "Real time MIDI module name", NULL);
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
    /* now load and pre-initialise external modules for this instance */
    /* this function returns an error value that may be worth checking */
    return csoundLoadModules(csound);
  }

  PUBLIC int csoundQueryInterface(const char *name, void **interface, int *version)
  {
    if(strcmp(name, "CSOUND") == 0)
      {
        *interface = csoundCreate(0);
        *version = csoundGetVersion();
        return 0;
      }
    return 1;
  }

  PUBLIC void csoundDestroy(void *csound)
  {
    csoundReset(csound);
#ifdef some_fine_day
    free(csound);
#endif
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
    return ((ENVIRON *)csound)->hostdata_;
  }

  PUBLIC void csoundSetHostData(void *csound, void *hostData)
  {
    ((ENVIRON *)csound)->hostdata_ = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int frsturnon;
  extern int sensevents(ENVIRON *);
  extern int cleanup(void*);

  PUBLIC int csoundPerform(void *csound, int argc, char **argv)
  {
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if ((returnValue = setjmp(cenviron.exitjmp_)))
      {
        csoundMessage(csound, "Early return from csoundPerform().\n");
        return returnValue;
      }
    return csoundMain(csound, argc, argv);
  }

  PUBLIC int csoundPerformKsmps(void *csound)
  {
    int done = 0;
    volatile int returnValue;
    /* setup jmp for return after an exit()
     */
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp_)))
      {
        csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
        return returnValue;
      }
    done = sensevents(csound);
    if (!done)
      {
        /* IV - Feb 05 2005 */
        if (!((ENVIRON*) csound)->oparms_->initonly)
          kperf(csound);
      }
    if(done)
      {
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
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp_)))
      {
        csoundMessage(csound, "Early return from csoundPerformKsmps().\n");
        return returnValue;
      }
    done = sensevents(csound);

    /* IV - Feb 05 2005 */
    if (!((ENVIRON*) csound)->oparms_->initonly)
      kperf(csound);
    return done;
  }


  /* external host's outbuffer passed in csoundPerformBuffer()
   */

  static long _rtCurOutBufCount = 0;

#if 0
  static char *_rtCurOutBuf = 0;
  static long _rtCurOutBufSize = 0;
  static char *_rtOutOverBuf = 0;
  static long _rtOutOverBufSize = 0;
  static long _rtOutOverBufCount = 0;
  static char *_rtInputBuf = 0;
#endif

  PUBLIC int csoundPerformBuffer(void *csound)
  {
    volatile int returnValue;
    /* Number of samples still needed to create before returning.
     */
    static int sampsNeeded = 0;
    int sampsPerKperf = csoundGetKsmps(csound) * csoundGetNchnls(csound);
    int done = 0;
    /* Setup jmp for return after an exit().
     */
    if ((returnValue = setjmp(((ENVIRON*) csound)->exitjmp_)))
      {
        csoundMessage(csound, "Early return from csoundPerformBuffer().\n");
        return returnValue;
      }
    _rtCurOutBufCount = 0;
    sampsNeeded += O.outbufsamps;
    while (!done && sampsNeeded > 0)
      {
        done = sensevents(csound);
        if (done)
          {
            return done;
          }
        if (!((ENVIRON*) csound)->oparms_->initonly)
            kperf(csound);
        sampsNeeded -= sampsPerKperf;
      }
    return done;
  }

  PUBLIC void csoundCleanup(void *csound)
  {
    cleanup(csound);
    /* Call all the funcs registered with atexit(). */
    while (csoundNumExits_ >= 0)
      {
        void (*func)(void) = csoundExitFuncs_[csoundNumExits_];
        func();
        csoundNumExits_--;
      }
  }

  /*
   * ATTRIBUTES
   */

  PUBLIC MYFLT csoundGetSr(void *csound)
  {
    return ((ENVIRON *)csound)->esr_;
  }

  PUBLIC MYFLT csoundGetKr(void *csound)
  {
    return ((ENVIRON *)csound)->ekr_;
  }

  PUBLIC int csoundGetKsmps(void *csound)
  {
    return ((ENVIRON *)csound)->ksmps_;
  }

  PUBLIC int csoundGetNchnls(void *csound)
  {
    return ((ENVIRON *)csound)->nchnls_;
  }

  PUBLIC int csoundGetSampleFormat(void *csound)
  {
    return O.outformat; /* should we assume input is same as output ? */
  }

  PUBLIC int csoundGetSampleSize(void *csound)
  {
    return O.sfsampsize; /* should we assume input is same as output ? */
  }

  PUBLIC long csoundGetInputBufferSize(void *csound)
  {
    return O.inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(void *csound)
  {
    return O.outbufsamps;
  }

  PUBLIC void *csoundGetInputBuffer(void *csound)
  {
    return inbuf;
  }

  PUBLIC void *csoundGetOutputBuffer(void *csound)
  {
    return outbuf;
  }

  PUBLIC MYFLT* csoundGetSpin(void *csound)
  {
    return ((ENVIRON *)csound)->spin_;
  }

  PUBLIC MYFLT* csoundGetSpout(void *csound)
  {
    return ((ENVIRON *)csound)->spout_;
  }

  PUBLIC MYFLT csoundGetScoreTime(void *csound)
  {
    return ((ENVIRON *)csound)->kcounter_ * ((ENVIRON *)csound)->onedkr_;
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

  static int csoundIsScorePending_ = 0;

  PUBLIC int csoundIsScorePending(void *csound)
  {
    return csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(void *csound, int pending)
  {
    csoundIsScorePending_ = pending;
  }

  static MYFLT csoundScoreOffsetSeconds_ = (MYFLT) 0.0;

  PUBLIC void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset)
  {
    csoundScoreOffsetSeconds_ = offset;
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(void *csound)
  {
    return csoundScoreOffsetSeconds_;
  }

  PUBLIC void csoundRewindScore(void *csound)
  {
    if(((ENVIRON *)csound)->scfp_)
      {
        fseek(((ENVIRON *)csound)->scfp_, 0, SEEK_SET);
      }
  }

  static void csoundDefaultMessageCallback(void *csound,
                                           const char *format, va_list args)
  {
#if defined(WIN32) || defined(mills_macintosh)
    vfprintf(stdout, format, args);
#else
    vfprintf(stderr, format, args);
#endif
  }

  static void (*csoundMessageCallback_)(void *csound,
                                        const char *format, va_list args) =
  csoundDefaultMessageCallback;

  PUBLIC void csoundSetMessageCallback(void *csound,
                 void (*csoundMessageCallback)(void *csound,
                                               const char *format, va_list args))
  {
    csoundMessageCallback_ = csoundMessageCallback;
  }

  PUBLIC void csoundMessageV(void *csound, const char *format, va_list args)
  {
    csoundMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundMessageCallback_(csound, format, args);
    va_end(args);
  }

  PUBLIC void csoundPrintf(const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundMessageCallback_(0, format, args);
    va_end(args);
  }

  static void (*csoundThrowMessageCallback_)(void *csound,
                                             const char *format,
                                             va_list args) =
  csoundDefaultMessageCallback;

  PUBLIC void csoundSetThrowMessageCallback(void *csound,
                    void (*csoundThrowMessageCallback)(void *csound,
                                                       const char *format,
                                                       va_list args))
  {
    csoundThrowMessageCallback_ = csoundThrowMessageCallback;
  }

  PUBLIC void csoundThrowMessageV(void *csound, const char *format, va_list args)
  {
    csoundThrowMessageCallback_(csound, format, args);
  }

  PUBLIC void csoundThrowMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundThrowMessageCallback_(csound, format, args);
    va_end(args);
  }

  PUBLIC int csoundGetMessageLevel(void *csound)
  {
    return ((ENVIRON *)csound)->oparms_->msglevel;
  }

  PUBLIC void csoundSetMessageLevel(void *csound, int messageLevel)
  {
    ((ENVIRON *)csound)->oparms_->msglevel = messageLevel;
  }

  PUBLIC void csoundInputMessage(void *csound, const char *message)
  {
    writeLine(message, strlen(message));
  }

  static char inChar_ = 0;

  PUBLIC void csoundKeyPress(void *csound, char c)
  {
    inChar_ = c;
  }

  char getChar()
  {
    return inChar_;
  }

  /*
   * CONTROL AND EVENTS
   */

  static void (*csoundInputValueCallback_)(void *csound,
                                           char *channelName, MYFLT *value) = NULL;

  PUBLIC void csoundSetInputValueCallback(void *csound,
                    void (*inputValueCalback)(void *csound,
                                              char *channelName, MYFLT *value))
  {
    csoundInputValueCallback_ = inputValueCalback;
  }

  void InputValue(char *channelName, MYFLT *value)
  {
    if (csoundInputValueCallback_)
      {
        csoundInputValueCallback_(&cenviron_, channelName, value);
      }
    else
      {
        *value = 0.0;
      }
  }

  static void (*csoundOutputValueCallback_)(void *csound,
                                            char *channelName, MYFLT value) = NULL;

  PUBLIC void csoundSetOutputValueCallback(void *csound,
                    void (*outputValueCalback)(void *csound,
                                               char *channelName, MYFLT value))
  {
    csoundOutputValueCallback_ = outputValueCalback;
  }

  void OutputValue(char *channelName, MYFLT value)
  {
    if (csoundOutputValueCallback_)
      {
        csoundOutputValueCallback_(&cenviron_, channelName, value);
      }
  }

  PUBLIC void csoundScoreEvent(void *csound, char type,
                               MYFLT *pfields, long numFields)
  {
    newevent(csound, type, pfields, numFields);
  }

  /*
   *    REAL-TIME AUDIO
   */

#if 0
  void playopen_mi(int nchanls, int dsize,
                   float sr, int scale) /* open for audio output */
  {
    _rtCurOutBufSize = O.outbufsamps*dsize;
    _rtCurOutBufCount = 0;
    _rtCurOutBuf = mmalloc(csound, _rtCurOutBufSize);

    /* a special case we need to handle 'overlaps'
     */
    /* FIXME - SYY - 11.15. 2003
     * GLOBAL * should be passed in so that ksmps can be grabbed by that
     */
    if (csoundGetKsmps(&cenviron_) * nchanls > O.outbufsamps)
      {
        _rtOutOverBufSize = (csoundGetKsmps(&cenviron_) * nchanls -
                             O.outbufsamps)*dsize;
        _rtOutOverBuf = mmalloc(csound, _rtOutOverBufSize);
        _rtOutOverBufCount = 0;
      }
    else
      {
        _rtOutOverBufSize = 0;
        _rtOutOverBuf = 0;
        _rtOutOverBufCount = 0;
      }
  }

  void rtplay_mi(char *outBuf, int nbytes)
  {
    int bytes2copy = nbytes;
    /* copy any remaining samps from last buffer
     */
    if (_rtOutOverBufCount)
      {
        memcpy(_rtCurOutBuf, _rtOutOverBuf, _rtOutOverBufCount);
        _rtCurOutBufCount = _rtOutOverBufCount;
        _rtOutOverBufCount = 0;
      }
    /* handle any new 'overlaps'
     */
    if (bytes2copy + _rtCurOutBufCount > _rtCurOutBufSize)
      {
        _rtOutOverBufCount = _rtCurOutBufSize - (bytes2copy + _rtCurOutBufCount);
        bytes2copy = _rtCurOutBufSize - _rtCurOutBufCount;

        memcpy(_rtOutOverBuf, outBuf+bytes2copy, _rtOutOverBufCount);
      }
    /* finally copy the buffer
     */
    memcpy(_rtCurOutBuf+_rtOutOverBufCount, outBuf, bytes2copy);
    _rtCurOutBufCount += bytes2copy;
  }

  void recopen_mi(int nchanls, int dsize, float sr, int scale)
  {
    if (O.inbufsamps*O.sfsampsize != O.outbufsamps*O.insampsiz)
      die("Input buffer must be the same size as Output buffer\n");
    _rtInputBuf = mmalloc(csound, O.inbufsamps*O.insampsiz);
  }

  int rtrecord_mi(char *inBuf, int nbytes)
  {
    memcpy(inBuf, _rtInputBuf, nbytes);
    return nbytes;
  }

  void rtclose_mi(void)
  {
    if (_rtCurOutBuf)
      mfree(csound, _rtCurOutBuf);
    if (_rtOutOverBuf)
      mfree(csound, _rtOutOverBuf);
    if (_rtInputBuf)
      mfree(csound, _rtInputBuf);
  }
#endif

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
        err_printf(Str(" *** error: rtaudio module set to empty string\n"));
      else
        err_printf(Str(" *** error: unknown rtaudio module: '%s'\n"), s);
      return CSOUND_ERROR;
    }
    /* IV - Feb 08 2005: avoid locking up the system with --sched */
#ifdef LINUX
    if (sched_getscheduler(0) != SCHED_OTHER) {
      err_printf(" *** error: cannot use --sched with dummy audio output\n");
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
        err_printf(Str(" *** error: rtaudio module set to empty string\n"));
      else
        err_printf(Str(" *** error: unknown rtaudio module: '%s'\n"), s);
      return CSOUND_ERROR;
    }
    /* IV - Feb 08 2005: avoid locking up the system with --sched */
#ifdef LINUX
    if (sched_getscheduler(0) != SCHED_OTHER) {
      err_printf(" *** error: cannot use --sched with dummy audio input\n");
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

  static int isGraphable_ = 1;

  PUBLIC void csoundSetIsGraphable(void *csound, int isGraphable)
  {
    isGraphable_ = isGraphable;
  }

  int Graphable()
  {
    return isGraphable_;
  }

  static void defaultCsoundMakeGraph(void *csound, WINDAT *windat, char *name)
  {
#if defined(USE_FLTK)
    extern void MakeGraph_(WINDAT *,char*);
    MakeGraph_(windat, name);
#else
    extern void MakeAscii(WINDAT *,char*);
    MakeAscii(windat, name);
#endif
  }

  static void (*csoundMakeGraphCallback_)(void *csound,
                                          WINDAT *windat,
                                          char *name) = defaultCsoundMakeGraph;

  PUBLIC void csoundSetMakeGraphCallback(void *csound,
                                         void (*makeGraphCallback)(void *csound,
                                                                   WINDAT *windat,
                                                                   char *name))
  {
    csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(WINDAT *windat, char *name)
  {
    csoundMakeGraphCallback_(&cenviron, windat, name);
  }

  static void defaultCsoundDrawGraph(void *csound, WINDAT *windat)
  {
#if defined(USE_FLTK)
    extern void DrawGraph_(WINDAT *);
    DrawGraph_(windat);
#else
    extern void MakeAscii(WINDAT *, char*);
    MakeAscii(windat, "");
#endif
  }

  static void (*csoundDrawGraphCallback_)(void *csound,
                                          WINDAT *windat) = defaultCsoundDrawGraph;

  PUBLIC void csoundSetDrawGraphCallback(void *csound,
                                         void (*drawGraphCallback)(void *csound,
                                                                   WINDAT *windat))
  {
    csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(WINDAT *windat)
  {
    csoundDrawGraphCallback_(&cenviron, windat);
  }

  static void defaultCsoundKillGraph(void *csound, WINDAT *windat)
  {
    extern void KillAscii(WINDAT *wdptr);
    KillAscii(windat);
  }

  static void (*csoundKillGraphCallback_)(void *csound,
                                          WINDAT *windat) = defaultCsoundKillGraph;

  PUBLIC void csoundSetKillGraphCallback(void *csound,
                                         void (*killGraphCallback)(void *csound,
                                                                   WINDAT *windat))
  {
    csoundKillGraphCallback_ = killGraphCallback;
  }

  void KillGraph(WINDAT *windat)
  {
    csoundKillGraphCallback_(&cenviron, windat);
  }

  static int defaultCsoundExitGraph(void *csound)
  {
    return CSOUND_SUCCESS;
  }

  static int (*csoundExitGraphCallback_)(void *csound) = defaultCsoundExitGraph;

  PUBLIC void csoundSetExitGraphCallback(void *csound,
                                         int (*exitGraphCallback)(void *csound))
  {
    csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph()
  {
    return csoundExitGraphCallback_(0);
  }

  void MakeXYin(XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
    printf("xyin not supported. use invalue opcode instead.\n");
  }

  void ReadXYin(XYINDAT *xyindat)
  {
    printf("xyin not supported. use invlaue opcodes instead.\n");
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

  PUBLIC int csoundAppendOpcode(void *csound,
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
    int oldSize = (int)((char *)((ENVIRON *)csound)->oplstend_ -
                        (char *)((ENVIRON *)csound)->opcodlst_);
    int newSize = oldSize + sizeof(OENTRY);
    int oldCount = oldSize / sizeof(OENTRY);
    int newCount = oldCount + 1;
    OENTRY *oldOpcodlst = ((ENVIRON *)csound)->opcodlst_;
    ((ENVIRON *)csound)->opcodlst_ =
      (OENTRY *) mrealloc(csound, ((ENVIRON *)csound)->opcodlst_, newSize);
    if(!((ENVIRON *)csound)->opcodlst_) {
      ((ENVIRON *)csound)->opcodlst_ = oldOpcodlst;
      err_printf("Failed to allocate new opcode entry.");
      return -1;
    }
    else {
      OENTRY *oentry = ((ENVIRON *)csound)->opcodlst_ + oldCount;
      ((ENVIRON *)csound)->oplstend_ = ((ENVIRON *)csound)->opcodlst_ + newCount;
      oentry->opname = opname;
      oentry->dsblksiz = dsblksiz;
      oentry->thread = thread;
      oentry->outypes = outypes;
      oentry->intypes = intypes;
      oentry->iopadr = (SUBR) iopadr;
      oentry->kopadr = (SUBR) kopadr;
      oentry->aopadr = (SUBR) aopadr;
      oentry->dopadr = (SUBR) dopadr;
#if 0
      printf("Appended opcodlst[%d]: opcode = %-20s "
             "intypes = %-20s outypes = %-20s\n",
             oldCount,
             oentry->opname,
             oentry->intypes,
             oentry->outypes);
#endif
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
  int POLL_EVENTS(void)
  {
    return 1;
  }
#else
  extern int POLL_EVENTS(void);
#endif

  int defaultCsoundYield(void *csound)
  {
    return POLL_EVENTS();
  }

  static int (*csoundYieldCallback_)(void *csound) = defaultCsoundYield;

  void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound))
  {
    csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(void *csound)
  {
    return csoundYieldCallback_(csound);
  }

  const static int MAX_ENVIRONS = 10;

  typedef struct Environs
  {
    char *environmentVariableName;
    char *path;
  } Environs;

  static Environs *csoundEnv_ = 0;

  static int csoundNumEnvs_ = 0;

  PUBLIC void csoundSetEnv(void *csound,
                           const char *environmentVariableName, const char *path)
  {
    int i = 0;
    if (!environmentVariableName || !path)
      return;

    if (csoundEnv_ == NULL)
      {
        csoundEnv_ = (Environs *) mcalloc(csound, MAX_ENVIRONS * sizeof(Environs));
        if (!csoundEnv_)
          {
            return;
          }
      }
    for (i = 0; i < csoundNumEnvs_; i++) {
      if (strcmp(csoundEnv_[i].environmentVariableName,
                 environmentVariableName) == 0)
        {
          mrealloc(csound, csoundEnv_[i].path, strlen(path)+1);
          strcpy(csoundEnv_[i].path, path);
          return;
        }
    }
    if (csoundNumEnvs_ >= MAX_ENVIRONS)
      {
        /* warning("Exceeded maximum number of environment paths"); */
        csoundMessage(csound, "Exceeded maximum number of environment paths");
        return;
      }

    csoundNumEnvs_++;
    csoundEnv_[csoundNumEnvs_].environmentVariableName =
      mmalloc(csound, strlen(environmentVariableName)+1);
    strcpy(csoundEnv_[csoundNumEnvs_].environmentVariableName,
           environmentVariableName);
    csoundEnv_[csoundNumEnvs_].path = mmalloc(csound, strlen(path) + 1);
    strcpy(csoundEnv_[csoundNumEnvs_].path, path);
  }

  PUBLIC char *csoundGetEnv(const char *environmentVariableName)
  {
    int i;
    for (i = 0; i < csoundNumEnvs_; i++) {
      if (strcmp(csoundEnv_[i].environmentVariableName,
                 environmentVariableName) == 0)
        {
          return (csoundEnv_[i].path);
        }
    }
    return 0;
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
    csoundIsScorePending_ = 1;
    csoundScoreOffsetSeconds_ = (MYFLT) 0.0;
  }

  PUBLIC int csoundGetDebug(void *csound_)
  {
    ENVIRON *csound = (ENVIRON *)csound;
    return csound->oparms_->odebug;
  }

  PUBLIC void csoundSetDebug(void *csound_, int debug)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    csound->oparms_->odebug = debug;
  }

  PUBLIC int csoundTableLength(void *csound_, int table)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    if(ftp) {
      return ftp->flen;
    } else {
      return -1;
    }
  }

  MYFLT csoundTableGet(void *csound_, int table, int index)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    return ftp->ftable[index];
  }

  PUBLIC void csoundTableSet(void *csound_, int table, int index, MYFLT value)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    MYFLT table_ = table;
    FUNC *ftp = (FUNC *)csound->ftfind_(csound,&table_);
    ftp->ftable[index] = value;
  }

#ifdef INGALLS
  /*
   * Returns a non-zero to the host,
   * which may call csoundCleanup().
   */
  void exit(int status)
  {
    longjmp(csoundJump_, status +1);
  }

  int atexit(void (*func)(void))
  {
    if (++csoundNumExits_ < csoundMaxExits)
      {
        csoundExitFuncs_[csoundNumExits_] = func;
        return 0;
      }
    else
      {
        return -1;
      }
  }
#endif

  PUBLIC void csoundSetFLTKThreadLocking(void *csound_, int isLocking)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    csound->doFLTKThreadLocking = isLocking;
  }

  PUBLIC int csoundGetFLTKThreadLocking(void *csound_)
  {
    ENVIRON *csound = (ENVIRON *)csound_;
    return csound->doFLTKThreadLocking;
  }

/* -------- IV - Jan 27 2005: timer functions -------- */

#include <time.h>
#include <ctype.h>

#if defined(WIN32)
#undef u_char
#undef u_short
#undef u_int
#undef u_long
#include <windows.h>
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

#if defined(HAVE_RDTSC)

/* find out CPU frequency based on /proc/cpuinfo */

static void get_CPU_cycle_time(void)
{
    FILE    *f;
    char    buf[256];

    /* if frequency is not known yet */
    f = fopen("/proc/cpuinfo", "r");
    if (f == NULL) {
      die("Cannot open /proc/cpuinfo. Support for RDTSC is not available.");
      return;
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
      die("No valid CPU frequency entry was found in /proc/cpuinfo.");
      return;
    }
    /* MHz -> seconds */
    timeResolutionSeconds = 0.000001 / timeResolutionSeconds;
}

#endif          /* HAVE_RDTSC */

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
    if (timeResolutionSeconds < 0.0) {
      /* set time resolution if not known yet */
#if defined(HAVE_RDTSC)
      get_CPU_cycle_time();
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
      err_printf("time resolution is %.3f ns\n", 1.0e9 * timeResolutionSeconds);
    }
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

