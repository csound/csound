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

#ifndef MAX_PATH
  static const int MAX_PATH = 0xff;
#endif

  /**
   * Csound symbols referenced in this file.
   */

#define csoundMaxExits 64

  static void* csoundExitFuncs_[csoundMaxExits];
  static long csoundNumExits_ = -1;
  static jmp_buf csoundJump_;
  extern OPARMS O;
  extern ENVIRON cenviron;
  extern int csoundMain(void *csound, int argc, char **argv);
  extern void csoundMainCleanup(void);
  extern void mainRESET(void);
  extern void dispkill(WINDAT *windat);
  extern int kperf(int kcnt);
  extern int kcnt;
  extern char *inbuf;
  extern void *outbuf;
  extern void SfReset(void);
  extern void csoundDefaultMidiOpen(void *csound);
  extern void create_opcodlst(void);
  extern int writeLine(const char *text, long size);
  extern void newevent(void *csound, char type, MYFLT *pfields, long numFields);
  extern opcodelist* new_opcode_list();
  extern int dispose_opcode_list(opcodelist *list);
  extern void csoundDefaultExternalMidiCloseCallback(void *csound);

  PUBLIC void *csoundCreate(void *hostdata)
  {
	cenviron.hostdata_ = hostdata;
    return &cenviron;
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
    //  Do nothing at this time.
    //  In the future, it will be necessary to destroy
    //  dynamically allocated instances of Csound.
  }

  int csoundGetVersion()
  {
    return VERSION * 100 + SUBVER;
  }

	int csoundGetAPIVersion(void)
	{
		return APIVERSION * 100 + APISUBVER;
	}

  void *csoundGetHostData(void *csound)
  {
    return ((ENVIRON *)csound)->hostdata_;
  }

  void csoundSetHostData(void *csound, void *hostData)
  {
    ((ENVIRON *)csound)->hostdata_ = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int frsturnon;
  extern int sensevents(void);
  extern int cleanup(void);
  extern int orcompact(void);

  int csoundPerform(void *csound, int argc, char **argv)
  {
    volatile int returnValue;
    /* setup jmp for return after an exit() */
    if (returnValue = setjmp(csoundJump_))
      {
        csoundMessage(csound, "Early return from csoundPerform().");
        return returnValue;
      }
    csoundReset(csound);
    frsturnon = 0;
    return csoundMain(csound, argc, argv);
  }

  int csoundCompile(void *csound, int argc, char **argv)
  {
    volatile int returnValue;
    /* setup jmp for return after an exit()
     */
    if (returnValue = setjmp(csoundJump_))
      {
        csoundMessage(csound, "Early return from csoundCompile().");
        return returnValue;
      }
    csoundReset(csound);
    frsturnon = 1;
    return csoundMain(csound, argc, argv);
  }

  int csoundPerformKsmps(void *csound)
  {
    int done = 0;
    volatile int returnValue;
    /* setup jmp for return after an exit()
     */
    if (returnValue = setjmp(csoundJump_))
      {
        csoundMessage(csound, "Early return from csoundPerformKsmps().");
        return returnValue;
      }
    done = sensevents();
    if (!done && kcnt)
      {
        /*
                Rather than overriding real-time event handling in kperf,
                turn it off before calling kperf, and back on afterwards.
        */
        int rtEvents = O.RTevents;
        O.RTevents = 0;
        kperf(1);
        kcnt -= 1;
        O.RTevents = rtEvents;
      }
    return done;
  }

  /* external host's outbuffer passed in csoundPerformBuffer()
   */
  static char *_rtCurOutBuf = 0;
  static long _rtCurOutBufSize = 0;
  static long _rtCurOutBufCount = 0;
  static char *_rtOutOverBuf = 0;
  static long _rtOutOverBufSize = 0;
  static long _rtOutOverBufCount = 0;
  static char *_rtInputBuf = 0;
  static long _rtInputBufSize = 0;
  static long _rtInputBufIndex = 0;

  int csoundPerformBuffer(void *csound)
  {
    volatile int returnValue;
    /* Number of samples still needed to create before returning.
     */
    static int sampsNeeded = 0;
    int sampsPerKperf = csoundGetKsmps(csound) * csoundGetNchnls(csound);
    int done = 0;
    /* Setup jmp for return after an exit().
     */
    if (returnValue = setjmp(csoundJump_))
      {
        csoundMessage(csound, "Early return from csoundPerformBuffer().");
        return returnValue;
      }
    _rtCurOutBufCount = 0;
       sampsNeeded += O.outbufsamps;
    while (!done && sampsNeeded > 0)
      {
        done = sensevents();
        if (done)
          {
            return done;
          }
        if (kcnt)
          {
            int rtEvents = O.RTevents;
            O.RTevents = 0;
            kperf(1);
            kcnt -= 1;
            sampsNeeded -= sampsPerKperf;
            O.RTevents = rtEvents;
          }
      }
    return done;
  }

  void csoundCleanup(void *csound)
  {
    orcompact();
    cleanup();
    // Call all the funcs registered with atexit().
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

  MYFLT csoundGetSr(void *csound)
  {
    return ((ENVIRON *)csound)->esr_;
  }

  MYFLT csoundGetKr(void *csound)
  {
    return ((ENVIRON *)csound)->ekr_;
  }

  int csoundGetKsmps(void *csound)
  {
	return ((ENVIRON *)csound)->ksmps_;
  }

  int csoundGetNchnls(void *csound)
  {
    return ((ENVIRON *)csound)->nchnls_;
  }

  int csoundGetSampleFormat(void *csound)
  {
    return O.outformat; /* should we assume input is same as output ? */
  }

  int csoundGetSampleSize(void *csound)
  {
    return O.outsampsiz; /* should we assume input is same as output ? */
  }

  long csoundGetInputBufferSize(void *csound)
  {
    return O.inbufsamps;
  }

  long csoundGetOutputBufferSize(void *csound)
  {
    return O.outbufsamps;
  }


  /** FIXME - SYY - 2003.11.29
   * Didn't know where to find inbuf in csound5
   */
  void *csoundGetInputBuffer(void *csound)
  {
    /* return inbuf; */
    return NULL;
  }

  void *csoundGetOutputBuffer(void *csound)
  {
    return outbuf;
  }

  MYFLT* csoundGetSpin(void *csound)
  {
    return ((ENVIRON *)csound)->spin_;
  }

  MYFLT* csoundGetSpout(void *csound)
  {
    return ((ENVIRON *)csound)->spout_;
  }

  MYFLT csoundGetScoreTime(void *csound)
  {
    return ((ENVIRON *)csound)->kcounter_ * ((ENVIRON *)csound)->onedkr_;
  }

  MYFLT csoundGetProgress(void *csound)
  {
    return -1;
  }

  MYFLT csoundGetProfile(void *csound)
  {
    return -1;
  }

  MYFLT csoundGetCpuUsage(void *csound)
  {
    return -1;
  }

  /*
   * SCORE HANDLING
   */

  static int csoundIsScorePending_ = 0;

  int csoundIsScorePending(void *csound)
  {
    return csoundIsScorePending_;
  }

  void csoundSetScorePending(void *csound, int pending)
  {
    csoundIsScorePending_ = pending;
  }

  static MYFLT csoundScoreOffsetSeconds_ = (MYFLT) 0.0;

  void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset)
  {
    csoundScoreOffsetSeconds_ = offset;
  }

  MYFLT csoundGetScoreOffsetSeconds(void *csound)
  {
    return csoundScoreOffsetSeconds_;
  }

  void csoundRewindScore(void *csound)
  {
    if(((ENVIRON *)csound)->scfp_)
      {
        fseek(((ENVIRON *)csound)->scfp_, 0, SEEK_SET);
      }
  }

  static void csoundDefaultMessageCallback(void *csound, const char *format, va_list args)
  {
		vfprintf(stdout, format, args);
  }

  static void (*csoundMessageCallback_)(void *csound, const char *format, va_list args) = csoundDefaultMessageCallback;

  void csoundSetMessageCallback(void *csound, void (*csoundMessageCallback)(void *csound, const char *format, va_list args))
  {
    csoundMessageCallback_ = csoundMessageCallback;
  }

  void csoundMessageV(void *csound, const char *format, va_list args)
  {
    csoundMessageCallback_(&cenviron, format, args);
  }

	void csoundMessageS(void *csound, const char *format, va_list args)
	{
		csoundMessageCallback_(&cenviron, format, args);
	}

  void csoundMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundMessageCallback_(&cenviron, format, args);
    va_end(args);
  }

  static void (*csoundThrowMessageCallback_)(void *csound, const char *format, va_list args) = csoundDefaultMessageCallback;

  void csoundSetThrowMessageCallback(void *csound, void (*csoundThrowMessageCallback)(void *csound, const char *format, va_list args))
  {
    csoundThrowMessageCallback_ = csoundThrowMessageCallback;
  }

  void csoundThrowMessageV(void *csound, const char *format, va_list args)
  {
    csoundThrowMessageCallback_(&cenviron, format, args);
  }

  void csoundThrowMessage(void *csound, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundThrowMessageCallback_(&cenviron, format, args);
    va_end(args);
  }

  int csoundGetMessageLevel(void *csound)
  {
    return ((ENVIRON *)csound)->oparms_->msglevel;
  }

  void csoundSetMessageLevel(void *csound, int messageLevel)
  {
    ((ENVIRON *)csound)->oparms_->msglevel = messageLevel;
  }

  void csoundInputMessage(void *csound, const char *message)
  {
    writeLine(message, strlen(message));
  }

  static char inChar_ = 0;

  void csoundKeyPress(void *csound, char c)
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

  static void (*csoundInputValueCallback_)(void *csound, char *channelName, MYFLT *value) = 0;

  void csoundSetInputValueCallback(void *csound, void (*inputValueCalback)(void *csound, char *channelName, MYFLT *value))
  {
    csoundInputValueCallback_ = inputValueCalback;
  }

  void InputValue(char *channelName, MYFLT *value)
  {
    if (csoundInputValueCallback_)
      {
        csoundInputValueCallback_(&cenviron, channelName, value);
      }
    else
      {
        *value = 0.0;
      }
  }

  static void (*csoundOutputValueCallback_)(void *csound, char *channelName, MYFLT value) = 0;

  void csoundSetOutputValueCallback(void *csound, void (*outputValueCalback)(void *csound, char *channelName, MYFLT value))
  {
    csoundOutputValueCallback_ = outputValueCalback;
  }

  void OutputValue(char *channelName, MYFLT value)
  {
    if (csoundOutputValueCallback_)
      {
        csoundOutputValueCallback_(&cenviron, channelName, value);
      }
  }

  void csoundScoreEvent(void *csound, char type, MYFLT *pfields, long numFields)
  {
    newevent(csound, type, pfields, numFields);
  }

  /*
   *    REAL-TIME AUDIO
   */

  extern void playopen_(int nchanls, int dsize, float sr, int scale);

  void (*playopen)(int nchanls, int dsize, float sr, int scale) = playopen_;

  void playopen_mi(int nchanls, int dsize, float sr, int scale) /* open for audio output */
  {
    _rtCurOutBufSize = O.outbufsamps*dsize;
    _rtCurOutBufCount = 0;
    _rtCurOutBuf = mmalloc(_rtCurOutBufSize);

    /* a special case we need to handle 'overlaps'
     */
    /* FIXME - SYY - 11.15. 2003
     * GLOBAL * should be passed in so that ksmps can be grabbed by that
     */
    if (csoundGetKsmps(&cenviron) * nchanls > O.outbufsamps) 
      {
        _rtOutOverBufSize = (csoundGetKsmps(&cenviron) * nchanls - O.outbufsamps)*dsize;
        _rtOutOverBuf = mmalloc(_rtOutOverBufSize);
        _rtOutOverBufCount = 0;
      }
    else
      {
        _rtOutOverBufSize = 0;
        _rtOutOverBuf = 0;
        _rtOutOverBufCount = 0;
      }
  }

  extern void rtplay_(char *outBuf, int nbytes);

  void (*rtplay)(char *outBuf, int nbytes) = rtplay_;

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

  extern void recopen_(int nchanls, int dsize, float sr, int scale);

  void (*recopen)(int nchanls, int dsize, float sr, int scale) = recopen_;

  void recopen_mi(int nchanls, int dsize, float sr, int scale)
  {
    if (O.inbufsamps*O.outsampsiz != O.outbufsamps*O.insampsiz)
      die("Input buffer must be the same size as Output buffer\n");
    _rtInputBuf = mmalloc(O.inbufsamps*O.insampsiz);
  }

  extern int rtrecord_(char *inBuf, int nbytes);

  int (*rtrecord)(char *inBuf, int nbytes) = rtrecord_;

  int rtrecord_mi(char *inBuf, int nbytes)
  {
    memcpy(inBuf, _rtInputBuf, nbytes);
    return nbytes;
  }

  extern void rtclose_(void);

  void (*rtclose)(void) = rtclose_;

  void rtclose_mi(void)
  {
    if (_rtCurOutBuf)
      mfree(_rtCurOutBuf);
    if (_rtOutOverBuf)
      mfree(_rtOutOverBuf);
    if (_rtInputBuf)
      mfree(_rtInputBuf);
  }

  void csoundSetPlayopenCallback(void *csound, void (*playopen__)(int nchanls, int dsize, float sr, int scale))
  {
    playopen = playopen__;
  }

  void csoundSetRtplayCallback(void *csound, void (*rtplay__)(char *outBuf, int nbytes))
  {
    rtplay = rtplay__;
  }

  void csoundSetRecopenCallback(void *csound, void (*recopen__)(int nchanls, int dsize, float sr, int scale))
  {
    recopen = recopen__;
  }

  void csoundSetRtrecordCallback(void *csound, int (*rtrecord__)(char *inBuf, int nbytes))
  {
    rtrecord = rtrecord__;
  }

  void csoundSetRtcloseCallback(void *csound, void (*rtclose__)(void))
  {
    rtclose = rtclose__;
  }

#if !defined(RTAUDIO)
  void playopen_(int nchanls, int dsize, float sr, int scale)
  {
  }

  void rtplay_(char *outBuf, int nbytes)
  {
  }

  void recopen_(int nchanls, int dsize, float sr, int scale)
  {
  }

  int rtrecord_(char *inBuf, int nbytes)
  {
    return 0;
  }

  void rtclose_(void)
  {
  }
#endif

  /*
   * MIDI -- this code replaces the MIDIDevice.c file
   */

  static void (*csoundExternalMidiOpenCallback_)(void *csound) = csoundDefaultMidiOpen;

  void csoundSetExternalMidiOpenCallback(void *csound, void (*csoundMidiOpen)(void *csound))
  {
    csoundExternalMidiOpenCallback_ = csoundMidiOpen;
  }

  void csoundExternalMidiOpen(void *csound)
  {
    csoundExternalMidiOpenCallback_(csound);
  }

  static int defaultCsoundMidiRead(void *csound, unsigned char *midiData, int size)
  {
    return 0;
  }

  static int (*csoundExternalMidiReadCallback_)(void *csound, unsigned char *midiData, int size) = defaultCsoundMidiRead;

  void csoundSetExternalMidiReadCallback(void *csound, int (*midiReadCallback)(void *csound, unsigned char *midiData, int size))
  {
    csoundExternalMidiReadCallback_ = midiReadCallback;
  }

  int csoundExternalMidiRead(void *csound, char *mbuf, int size)
  {
    return csoundExternalMidiReadCallback_(csound, mbuf, size);
  }

  static int csoundExternalMidiEnabled_ = 0;

  int csoundIsExternalMidiEnabled(void *csound)
  {
    return csoundExternalMidiEnabled_;
  }

  void csoundSetExternalMidiEnabled(void *csound, int enabled)
  {
    csoundExternalMidiEnabled_ = enabled;
  }

  static int defaultCsoundMidiWrite(void *csound, unsigned char *midiData)
  {
    return 0;
  }

  static int (*csoundExternalMidiWriteCallback_)(void *csound, unsigned char *midiData) = defaultCsoundMidiWrite;

  void csoundSetExternalMidiWriteCallback(void *csound, int (*midiWriteCallback)(void *csound, unsigned char *midiData))
  {
    csoundExternalMidiWriteCallback_ = midiWriteCallback;
  }

  int csoundExternalMidiWrite(unsigned char *midiData)
  {
    return csoundExternalMidiWriteCallback_(&cenviron, midiData);
  }

  void csoundDefaultMidiCloseCallback(void *csound)
  {
  }

  static void (*csoundExternalMidiCloseCallback_)(void *csound) = csoundDefaultMidiCloseCallback;

  void csoundSetExternalMidiCloseCallback(void *csound, void (*csoundExternalMidiCloseCallback)(void *csound))
  {
    csoundExternalMidiCloseCallback_ = csoundExternalMidiCloseCallback;
  }

  void csoundExternalMidiClose(void *csound)
  {
    csoundExternalMidiCloseCallback_(csound);
  }

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  static int isGraphable_ = 1;

  void csoundSetIsGraphable(void *csound, int isGraphable)
  {
    isGraphable_ = isGraphable;
  }

  int Graphable()
  {
    return isGraphable_;
  }

  static void defaultCsoundMakeGraph(void *csound, WINDAT *windat, char *name)
  {
    MakeAscii(windat, name);
  }

  static void (*csoundMakeGraphCallback_)(void *csound,  WINDAT *windat, char *name) = defaultCsoundMakeGraph;

  void csoundSetMakeGraphCallback(void *csound, void (*makeGraphCallback)(void *csound, WINDAT *windat, char *name))
  {
    csoundMakeGraphCallback_ = makeGraphCallback;
  }

  void MakeGraph(WINDAT *windat, char *name)
  {
    csoundMakeGraphCallback_(&cenviron, windat, name);
  }

  static void defaultCsoundDrawGraph(void *csound, WINDAT *windat)
  {
    DrawAscii(windat);
  }

  static void (*csoundDrawGraphCallback_)(void *csound,  WINDAT *windat) = defaultCsoundDrawGraph;

  void csoundSetDrawGraphCallback(void *csound, void (*drawGraphCallback)(void *csound, WINDAT *windat))
  {
    csoundDrawGraphCallback_ = drawGraphCallback;
  }

  void DrawGraph(WINDAT *windat)
  {
    csoundDrawGraphCallback_(&cenviron, windat);
  }

  static void defaultCsoundKillGraph(void *csound, WINDAT *windat)
  {
    KillAscii(windat);
  }

  static void (*csoundKillGraphCallback_)(void *csound,  WINDAT *windat) = defaultCsoundKillGraph;

  void csoundSetKillGraphCallback(void *csound, void (*killGraphCallback)(void *csound, WINDAT *windat))
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

  void csoundSetExitGraphCallback(void *csound, int (*exitGraphCallback)(void *csound))
  {
    csoundExitGraphCallback_ = exitGraphCallback;
  }

  int ExitGraph()
  {
    return csoundExitGraphCallback_(&cenviron);
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

  /** FIXME - SYY - 2003.11.29
   *  Does function need to exist anymore
   *  now that opcodelist is stored with ENVIRON?
   */
  opcodelist *csoundNewOpcodeList()
  {
    /* create_opcodlst();
    return new_opcode_list(); */
    
    return NULL;
  }

  /** FIXME - SYY - 2003.11.29
   *  Does function need to exist anymore
   *  now that opcodelist is stored with ENVIRON?
   */
  void csoundDisposeOpcodeList(opcodelist *list)
  {
    // dispose_opcode_list(list);
  }

  int csoundAppendOpcode(void *csound,
  						 char *opname,
                         int dsblksiz,
                         int thread,
                         char *outypes,
                         char *intypes,
                         SUBR iopadr,
                         SUBR kopadr,
                         SUBR aopadr,
                         SUBR dopadr)
  {
    int oldSize = (int)((char *)((ENVIRON *)csound)->oplstend_ - 
    					(char *)((ENVIRON *)csound)->opcodlst_);
    int newSize = oldSize + sizeof(OENTRY);
    int oldCount = oldSize / sizeof(OENTRY);
    int newCount = oldCount + 1;
    OENTRY *oldOpcodlst = ((ENVIRON *)csound)->opcodlst_;
    ((ENVIRON *)csound)->opcodlst_ = (OENTRY *) mrealloc(((ENVIRON *)csound)->opcodlst_, newSize);
    if(!((ENVIRON *)csound)->opcodlst_)
      {
        ((ENVIRON *)csound)->opcodlst_ = oldOpcodlst;
        err_printf("Failed to allocate new opcode entry.");
        return 0;
      }
    else
      {
        OENTRY *oentry = ((ENVIRON *)csound)->opcodlst_ + oldCount;
        ((ENVIRON *)csound)->oplstend_ = ((ENVIRON *)csound)->opcodlst_ + newCount;
        oentry->opname = opname;
        oentry->dsblksiz = dsblksiz;
        oentry->thread = thread;
        oentry->outypes = outypes;
        oentry->intypes = intypes;
        oentry->iopadr = iopadr;
        oentry->kopadr = kopadr;
        oentry->dopadr = dopadr;
        printf("Appended opcodlst[%d]: opcode = %-20s intypes = %-20s outypes = %-20s\n",
               oldCount,
               oentry->opname,
               oentry->intypes,
               oentry->outypes);
        return 1;
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
    while(ip = (INSDS *)ip->nxti)
      {
        pds = (OPDS *)ip;
        while(pds = pds->nxti)
          {
            if(pds->dopadr)
              {
                (*pds->dopadr)(pds);
              }
          }
      }
    ip = ip_;
    while(ip = (INSDS *)ip->nxtp)
      {
        pds = (OPDS *)ip;
        while(pds = pds->nxtp)
          {
            if(pds->dopadr)
              {
                (*pds->dopadr)(pds);
              }
          }
      }
  }

  /*
        * MISC FUNCTIONS
        */

        /**
        *       to enable csoundYield(), you must define 
	*       USE_CSOUND_YIELD in cs.h, so that the following happens:
        *       #define POLL_EVENTS() csoundYield(0)
        */

  static int defaultCsoundYield(void *csound)
  {
    return 1;
  }

  static int (*csoundYieldCallback_)(void *csound) = defaultCsoundYield;

  void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound))
  {
    csoundYieldCallback_ = yieldCallback;
  }

  int csoundYield(void *csound)
  {
    return csoundYieldCallback_(&cenviron);
  }

  const static int MAX_ENVIRONS = 10;

  typedef struct Environs
  {
    char *environmentVariableName;
    char *path;
  } Environs;

  static Environs *csoundEnv_ = 0;

  static int csoundNumEnvs_ = 0;

  void csoundSetEnv(void *csound, const char *environmentVariableName, const char *path)
  {
    int i = 0;
    if (!environmentVariableName || !path)
      return;

    if (csoundEnv_ == NULL)
      {
        csoundEnv_ = (Environs *) mcalloc(MAX_ENVIRONS * sizeof(Environs));
        if (!csoundEnv_)
          {
            return;
          }
      }
    for (i = 0; i < csoundNumEnvs_; i++)
      {
        if (strcmp(csoundEnv_[i].environmentVariableName, environmentVariableName) == 0)
          {
            mrealloc(csoundEnv_[i].path, strlen(path)+1);
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
    csoundEnv_[csoundNumEnvs_].environmentVariableName =  mmalloc(strlen(environmentVariableName)+1);
    strcpy(csoundEnv_[csoundNumEnvs_].environmentVariableName, environmentVariableName);
    csoundEnv_[csoundNumEnvs_].path = mmalloc(strlen(path) + 1);
    strcpy(csoundEnv_[csoundNumEnvs_].path, path);
  }

  char *csoundGetEnv(const char *environmentVariableName)
  {
    int i;
    for (i = 0; i < csoundNumEnvs_; i++)
      {
        if (strcmp(csoundEnv_[i].environmentVariableName, environmentVariableName) == 0)
          {
            return (csoundEnv_[i].path);
          }
      }
    return 0;
  }

  void csoundReset(void *csound)
  {
    mainRESET();
    /* FIXME - SYY - 11.16.2003
     * This call is needed by opcodes in Opcode plugin generated from 
     * Opcodes/sfont.c.  Needs to be reword in sfont.c with using opcode 
     * destructor functions or csoundAppendResetFunction needs to be created
     */
    /* SfReset();  */
    ((ENVIRON *)csound)->spoutactive_ = 0;
    csoundIsScorePending_ = 1;
    csoundScoreOffsetSeconds_ = (MYFLT) 0.0;
    O.Midiin = 0;
    ((ENVIRON *)csound)->nrecs_ = 0;
    ((ENVIRON *)csound)->orchname_ = NULL;
    ((ENVIRON *)csound)->scorename_ = NULL;
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


#ifdef __cplusplus
};
#endif

