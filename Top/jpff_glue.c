/*  
    jpff_glue.c:

    Copyright (C) 2002 John ffitch, Michael Gogins

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "version.h"
#include "cs.h"

/* This file is glue code to make csound actually run again */

extern long    kcnt;
void csoundMessage(void *, const char *, ...);
int csoundMain(void*, int, char**);

#if !defined(CWIN) && !defined(MACOSX)
extern OPARMS  O_;
extern ENVIRON cenviron_;
void csoundMessage(void *, const char *, ...);

extern void remove_tmpfiles(void);      /* IV - Oct 31 2002 */
int run_status = 0;

#if !defined(FLTK_GUI) && !defined(mills_macintosh)
int main(int argc, char **argv)
{
    extern int csoundMain(void*, int, char**);
    run_status = 0;
    O = O_;
    cenviron = cenviron_;
    atexit(remove_tmpfiles);            /* IV - Oct 31 2002 */
    return csoundMain(NULL, argc, argv);
}
#endif
#endif

void *csoundCreate(void *hostData)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

int csoundQueryInterface(const char *name, void **interface, int *version)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return 0;
}

void csoundDestroy(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundGetVersion(void)
{
#if BETA
    return -(100*VERSION+SUBVER);
#else
    return 100*VERSION+SUBVER;
#endif
}

int csoundGetAPIVersion(void)
{
    return APIVERSION * 100 + APISUBVER;
}

void *csoundGetHostData(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

void csoundSetHostData(void *csound, void *hostData)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundPerform(void *csound, int argc, char **argv)
{
    int val;
    jmp_buf lj;

    if ((val = setjmp(lj))) {
        printf("Error return");
        return val;
    }
    return csoundMain(csound, argc, argv);
}


extern int sensevents(void);
extern long kperf(long);
int csoundPerformKsmps(void *csound)
{
    int done = 0;
    int returnValue;
    jmp_buf lj;
    /* setup jmp for return after an exit()
     */
    if ((returnValue = setjmp(lj))) {
      csoundMessage(csound, "Early return from csoundPerformKsmps().");
      return returnValue;
    }
    done = sensevents();
    if (!done && kcnt) {
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

int csoundPerformBuffer(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return -1;
}

void csoundCleanup(void *csound)
{
    extern void cleanup(void);
    cleanup();
}

MYFLT csoundGetSr(void *csound)
{
    return esr;
}

MYFLT csoundGetKr(void *csound)
{
    return ekr;
}

int csoundGetKsmps(void *csound)
{
    return ksmps;
}

int csoundGetNchnls(void *csound)
{
    return nchnls;
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

void *csoundGetInputBuffer(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

void *csoundGetOutputBuffer(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

MYFLT* csoundGetSpin(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

MYFLT* csoundGetSpout(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

MYFLT csoundGetScoreTime(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return FL(0.0);
}

MYFLT csoundGetProgress(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return FL(0.0);
}

MYFLT csoundGetProfile(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return FL(0.0);
}

MYFLT csoundGetCpuUsage(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return FL(0.0);
}

int csoundIsScorePending(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return 0;
}

void csoundSetScorePending(void *csound, int pending)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetScoreOffsetSeconds(void *csound, MYFLT offset)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

MYFLT csoundGetScoreOffsetSeconds(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return FL(0.0);
}

void csoundRewindScore(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

static void csoundMessageCallback(void *csound, const char *format, va_list args)
{
    vfprintf(stderr, format, args);
}


void csoundSetMessageCallback(void *csound,
                              void (*csoundMessageCallback)(void *csound,
                                                            const char *format,
                                                            va_list args))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundMessageS(void *csound, const char *format, va_list args)
{
    vprintf(format, args);
}

#ifndef FLTK_GUI
void csoundMessageV(void *csound, const char *format, va_list args)
{
    vfprintf(stderr, format, args);
}

void csoundMessage(void *csound, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
#endif

void
csoundSetThrowMessageCallback(void *csound,
                              void (*csoundThrowMessageCallback)(void *csound,
                                                                 const char *format,
                                                                 va_list args))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundThrowMessageV(void *csound, const char *format, va_list args)
{
    vfprintf(stderr, format, args);
}

void csoundThrowMessage(void *csound, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

int csoundGetMessageLevel(void *csound)
{
    ENVIRON *cenviron = (ENVIRON*)csound;
    return cenviron->oparms_->msglevel;
}

void csoundSetMessageLevel(void *csound, int messageLevel)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundInputMessage(void *csound, const char *message)
{
    printf("DO NOT USE THIS INTERFACE\n");
}


void csoundKeyPress(void *csound, char c)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

char getChar(void)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return '\0';
}

void csoundSetInputValueCallback(void *csound,
                                 void (*inputValueCalback)(void *csound,
                                                           char *channelName,
                                                           MYFLT *value))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void InputValue(char *channelName, MYFLT *value)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetOutputValueCallback(void *csound,
                                  void (*outputValueCalback)(void *csound,
                                                             char *channelName,
                                                             MYFLT value))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void OutputValue(char *channelName, MYFLT value)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundScoreEvent(void *csound, char type, MYFLT *pfields, long numFields)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

#ifdef RTAUDIO
extern void playopen_(int nchanls, int dsize, float sr, int scale);

void (*playopen)(int nchanls, int dsize, float sr, int scale) = playopen_;

extern void rtplay_(char *outBuf, int nbytes);

void (*rtplay)(char *outBuf, int nbytes) = rtplay_;

extern void recopen_(int nchanls, int dsize, float sr, int scale);

void (*recopen)(int nchanls, int dsize, float sr, int scale) = recopen_;

extern int rtrecord_(char *inBuf, int nbytes);

int (*rtrecord)(char *inBuf, int nbytes) = rtrecord_;

extern void rtclose_(void);

void (*rtclose)(void) = rtclose_;
#endif

void csoundSetPlayopenCallback(void *csound,
                               void (*playopen__)(int nchanls, int dsize,
                                                  float sr, int scale))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetRtplayCallback(void *csound,
                             void (*rtplay__)(char *outBuf, int nbytes))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetRecopenCallback(void *csound,
                              void (*recopen__)(int nchanls, int dsize,
                                                float sr, int scale))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetRtrecordCallback(void *csound,
                               int (*rtrecord__)(char *inBuf, int nbytes))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetRtcloseCallback(void *csound, void (*rtclose__)(void))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetExternalMidiOpenCallback(void *csound,
                                       void (*csoundMidiOpen)(void *csound))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundExternalMidiOpen(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetExternalMidiReadCallback(void *csound,
                                       int (*midiReadCallback)(void *csound,
                                                               unsigned char *midiData,
                                                               int size))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundExternalMidiRead(void *csound, char *mbuf, int size)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return 0;
}

int csoundIsExternalMidiEnabled(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return 0;
}

void csoundSetExternalMidiEnabled(void *csound, int enabled)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetExternalMidiWriteCallback(void *csound,
                                        int (*midiWriteCallback)(void *csound,
                                                                 unsigned char *midiData))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundExternalMidiWrite(unsigned char *midiData)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return 0;
}

void csoundDefaultMidiCloseCallback(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetExternalMidiCloseCallback(void *csound,
                                        void (*csoundExternalMidiCloseCallback)(void *csound))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundExternalMidiClose(void *csound)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetIsGraphable(void *csound, int isGraphable)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetMakeGraphCallback(void *csound,
                                void (*makeGraphCallback)(void *csound, WINDAT *windat, char *name))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetDrawGraphCallback(void *csound,
                                void (*drawGraphCallback)(void *csound, WINDAT *windat))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetKillGraphCallback(void *csound,
                                void (*killGraphCallback)(void *csound, WINDAT *windat))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

void csoundSetExitGraphCallback(void *csound, int (*exitGraphCallback)(void *csound))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

#ifdef WINDOWS
extern int Graphable_(void);
extern void MakeGraph_(WINDAT *, char *);
extern void DrawGraph_(WINDAT *);
extern void KillGraph_(WINDAT *);
extern int ExitGraph_(void);
extern void MakeXYin_(XYINDAT *, MYFLT, MYFLT);
extern void ReadXYin_(XYINDAT *);

int Graphable(void)
{
    return Graphable_();
}

void MakeGraph(WINDAT *windat, char *name)
{
    MakeGraph_(windat, name);
}

void DrawGraph(WINDAT *windat)
{
    DrawGraph_(windat);
}

void KillGraph(WINDAT *windat)
{
    KillGraph_(windat);
}

int ExitGraph(void)
{
    return ExitGraph_();
}

void MakeXYin(XYINDAT *xyindat, MYFLT x, MYFLT y)       /* IV - Sep 8 2002 */
{
    MakeXYin_(xyindat, x, y);
}

void ReadXYin(XYINDAT *xyindat)
{
    ReadXYin_(xyindat);
}
#endif

opcodelist *csoundNewOpcodeList(void)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

void csoundDisposeOpcodeList(opcodelist *list)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundAppendOpcode(char *opname,
                       int dsblksiz,
                       int thread,
                       char *outypes,
                       char *intypes,
                       SUBR iopadr,
                       SUBR kopadr,
                       SUBR aopadr,
                       SUBR dopadr)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return -1;
}

void csoundSetYieldCallback(void *csound, int (*yieldCallback)(void *csound))
{
    printf("DO NOT USE THIS INTERFACE\n");
}

int csoundYield(void *csound)
{
    return 0;
}

void csoundSetEnv(void *csound,
                  const char *environmentVariableName,
                  const char *path)
{
    printf("DO NOT USE THIS INTERFACE\n");
}

char *csoundGetEnv(const char *environmentVariableName)
{
    return getenv(environmentVariableName);
}

void csoundReset(void *csound)
{
    extern void mainRESET(void *csound);
    mainRESET(csound);
}

void *csoundOpenLibrary(const char *libraryPath)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

void *csoundCloseLibrary(void *library)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    printf("DO NOT USE THIS INTERFACE\n");
    return NULL;
}

