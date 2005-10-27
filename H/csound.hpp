
#ifndef __CSOUND_HPP__
#define __CSOUND_HPP__

#include "csound.h"
#include "cfgvar.h"

// main Csound class

class Csound {
  protected:
    CSOUND  *csound;
  public:
    // csound.h interface
    int PreCompile()
    {
      return csoundPreCompile(csound);
    }
    int InitializeCscore(FILE *insco, FILE *outsco)
    {
      return csoundInitializeCscore(csound, insco, outsco);
    }
    void *GetHostData()
    {
      return csoundGetHostData(csound);
    }
    void SetHostData(void *hostData)
    {
      csoundSetHostData(csound, hostData);
    }
    const char *GetEnv(const char *name)
    {
      return csoundGetEnv(csound, name);
    }
    int Perform(int argc, char **argv)
    {
      return csoundPerform(csound, argc, argv);
    }
    int Perform(char *csdName)
    {
      char  *argv[3];
      argv[0] = "csound";
      argv[1] = csdName;
      argv[2] = (char*) 0;
      return csoundPerform(csound, 2, &(argv[0]));
    }
    int Perform(char *orcName, char *scoName)
    {
      char  *argv[4];
      argv[0] = "csound";
      argv[1] = orcName;
      argv[2] = scoName;
      argv[3] = (char*) 0;
      return csoundPerform(csound, 3, &(argv[0]));
    }
    int Perform(char *arg1, char *arg2, char *arg3)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = (char*) 0;
      return csoundPerform(csound, 4, &(argv[0]));
    }
    int Perform(char *arg1, char *arg2, char *arg3, char *arg4)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = arg4;
      argv[5] = (char*) 0;
      return csoundPerform(csound, 5, &(argv[0]));
    }
    int Perform(char *arg1, char *arg2, char *arg3, char *arg4, char *arg5)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = arg4;
      argv[5] = arg5;
      argv[6] = (char*) 0;
      return csoundPerform(csound, 6, &(argv[0]));
    }
    int Compile(int argc, char **argv)
    {
      return csoundCompile(csound, argc, argv);
    }
    int Compile(char *csdName)
    {
      char  *argv[3];
      argv[0] = "csound";
      argv[1] = csdName;
      argv[2] = NULL;
      return csoundCompile(csound, 2, &(argv[0]));
    }
    int Compile(char *orcName, char *scoName)
    {
      char  *argv[4];
      argv[0] = "csound";
      argv[1] = orcName;
      argv[2] = scoName;
      argv[3] = NULL;
      return csoundCompile(csound, 3, &(argv[0]));
    }
    int Compile(char *arg1, char *arg2, char *arg3)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = (char*) 0;
      return csoundCompile(csound, 4, &(argv[0]));
    }
    int Compile(char *arg1, char *arg2, char *arg3, char *arg4)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = arg4;
      argv[5] = (char*) 0;
      return csoundCompile(csound, 5, &(argv[0]));
    }
    int Compile(char *arg1, char *arg2, char *arg3, char *arg4, char *arg5)
    {
      char  *argv[7];
      argv[0] = "csound";
      argv[1] = arg1;
      argv[2] = arg2;
      argv[3] = arg3;
      argv[4] = arg4;
      argv[5] = arg5;
      argv[6] = (char*) 0;
      return csoundCompile(csound, 6, &(argv[0]));
    }
    int PerformKsmps()
    {
      return csoundPerformKsmps(csound);
    }
    int PerformKsmpsAbsolute()
    {
      return csoundPerformKsmpsAbsolute(csound);
    }
    int PerformBuffer()
    {
      return csoundPerformBuffer(csound);
    }
    int Cleanup()
    {
      return csoundCleanup(csound);
    }
    void Reset()
    {
      csoundReset(csound);
    }
    // attributes
    MYFLT GetSr()
    {
      return csoundGetSr(csound);
    }
    MYFLT GetKr()
    {
      return csoundGetKr(csound);
    }
    int GetKsmps()
    {
      return csoundGetKsmps(csound);
    }
    int GetNchnls()
    {
      return csoundGetNchnls(csound);
    }
    MYFLT Get0dBFS()
    {
      return csoundGet0dBFS(csound);
    }
    int GetStrVarMaxLen()
    {
      return csoundGetStrVarMaxLen(csound);
    }
    int GetSampleFormat()
    {
      return csoundGetSampleFormat(csound);
    }
    int GetSampleSize()
    {
      return csoundGetSampleSize(csound);
    }
    long GetInputBufferSize()
    {
      return csoundGetInputBufferSize(csound);
    }
    long GetOutputBufferSize()
    {
      return csoundGetOutputBufferSize(csound);
    }
    MYFLT *GetInputBuffer()
    {
      return csoundGetInputBuffer(csound);
    }
    MYFLT *GetOutputBuffer()
    {
      return csoundGetOutputBuffer(csound);
    }
    MYFLT *GetSpin()
    {
      return csoundGetSpin(csound);
    }
    MYFLT *GetSpout()
    {
      return csoundGetSpout(csound);
    }
    const char *GetOutputFileName()
    {
      return csoundGetOutputFileName(csound);
    }
    void SetHostImplementedAudioIO(int state, int bufSize)
    {
      csoundSetHostImplementedAudioIO(csound, state, bufSize);
    }
    MYFLT GetScoreTime()
    {
      return csoundGetScoreTime(csound);
    }
    // score handling
    int IsScorePending()
    {
      return csoundIsScorePending(csound);
    }
    void SetScorePending(int pending)
    {
      csoundSetScorePending(csound, pending);
    }
    MYFLT GetScoreOffsetSeconds()
    {
      return csoundGetScoreOffsetSeconds(csound);
    }
    void SetScoreOffsetSeconds(MYFLT time)
    {
      csoundSetScoreOffsetSeconds(csound, time);
    }
    void RewindScore()
    {
      csoundRewindScore(csound);
    }
    void SetCscoreCallback(void (*cscoreCallback)(CSOUND *))
    {
      csoundSetCscoreCallback(csound, cscoreCallback);
    }
    // messages & text
    void Message(const char *format, ...)
    {
      va_list args;
      va_start(args, format);
      csoundMessageV(csound, 0, format, args);
      va_end(args);
    }
    void MessageS(int attr, const char *format, ...)
    {
      va_list args;
      va_start(args, format);
      csoundMessageV(csound, attr, format, args);
      va_end(args);
    }
    void MessageV(int attr, const char *format, va_list args)
    {
      csoundMessageV(csound, attr, format, args);
    }
    void ThrowMessage(const char *format, ...)
    {
      va_list args;
      va_start(args, format);
      csoundThrowMessage(csound, format, args);
      va_end(args);
    }
    void ThrowMessageV(const char *format, va_list args)
    {
      csoundThrowMessageV(csound, format, args);
    }
    void SetMessageCallback(void (*csoundMessageCallback)(CSOUND *, int attr,
                                                          const char *format,
                                                          va_list valist))
    {
      csoundSetMessageCallback(csound, csoundMessageCallback);
    }
    void SetThrowMessageCallback(void (*throwMessageCallback)(CSOUND *,
                                                              const char *fmt,
                                                              va_list valist))
    {
      csoundSetThrowMessageCallback(csound, throwMessageCallback);
    }
    int GetMessageLevel()
    {
      return csoundGetMessageLevel(csound);
    }
    void SetMessageLevel(int messageLevel)
    {
      csoundSetMessageLevel(csound, messageLevel);
    }
    void InputMessage(const char *message)
    {
      csoundInputMessage(csound, message);
    }
    void KeyPress(char c)
    {
      csoundKeyPress(csound, c);
    }
    // control and events
    void SetInputValueCallback(void (*inputValueCalback)(CSOUND *,
                                                         const char *, MYFLT *))
    {
      csoundSetInputValueCallback(csound, inputValueCalback);
    }
    void SetOutputValueCallback(void (*outputValueCalback)(CSOUND *,
                                                           const char *, MYFLT))
    {
      csoundSetOutputValueCallback(csound, outputValueCalback);
    }
    int ScoreEvent(char type, const MYFLT *pFields, long numFields)
    {
      return csoundScoreEvent(csound, type, pFields, numFields);
    }
    // MIDI
    void SetExternalMidiInOpenCallback(int (*func)(CSOUND *,
                                                   void **, const char *))
    {
      csoundSetExternalMidiInOpenCallback(csound, func);
    }
    void SetExternalMidiReadCallback(int (*func)(CSOUND *, void *,
                                                 unsigned char *, int))
    {
      csoundSetExternalMidiReadCallback(csound, func);
    }
    void SetExternalMidiInCloseCallback(int (*func)(CSOUND *, void *))
    {
      csoundSetExternalMidiInCloseCallback(csound, func);
    }
    void SetExternalMidiOutOpenCallback(int (*func)(CSOUND *,
                                                    void **, const char *))
    {
      csoundSetExternalMidiOutOpenCallback(csound, func);
    }
    void SetExternalMidiWriteCallback(int (*func)(CSOUND *, void *,
                                                  const unsigned char *, int))
    {
      csoundSetExternalMidiWriteCallback(csound, func);
    }
    void SetExternalMidiOutCloseCallback(int (*func)(CSOUND *, void *))
    {
      csoundSetExternalMidiOutCloseCallback(csound, func);
    }
    void SetExternalMidiErrorStringCallback(const char *(*func)(int))
    {
      csoundSetExternalMidiErrorStringCallback(csound, func);
    }
    // function table display
    int SetIsGraphable(int isGraphable)
    {
      return csoundSetIsGraphable(csound, isGraphable);
    }
    void SetMakeGraphCallback(void (*makeGraphCallback)(CSOUND *,
                                                        WINDAT *windat,
                                                        const char *name))
    {
      csoundSetMakeGraphCallback(csound, makeGraphCallback);
    }
    void SetDrawGraphCallback(void (*drawGraphCallback)(CSOUND *,
                                                        WINDAT *windat))
    {
      csoundSetDrawGraphCallback(csound, drawGraphCallback);
    }
    void SetKillGraphCallback(void (*killGraphCallback)(CSOUND *,
                                                        WINDAT *windat))
    {
      csoundSetKillGraphCallback(csound, killGraphCallback);
    }
    void SetMakeXYinCallback(void (*makeXYinCallback)(CSOUND *, XYINDAT *,
                                                      MYFLT x, MYFLT y))
    {
      csoundSetMakeXYinCallback(csound, makeXYinCallback);
    }
    void SetReadXYinCallback(void (*readXYinCallback)(CSOUND *, XYINDAT *))
    {
      csoundSetReadXYinCallback(csound, readXYinCallback);
    }
    void SetKillXYinCallback(void (*killXYinCallback)(CSOUND *, XYINDAT *))
    {
      csoundSetKillXYinCallback(csound, killXYinCallback);
    }
    void SetExitGraphCallback(int (*exitGraphCallback)(CSOUND *))
    {
      csoundSetExitGraphCallback(csound, exitGraphCallback);
    }
    // opcodes
    int NewOpcodeList(opcodeListEntry* &opcodelist)
    {
      opcodeListEntry *tmp = (opcodeListEntry*) 0;
      int retval;
      retval = csoundNewOpcodeList(csound, &tmp);
      opcodelist = tmp;
      return retval;
    }
    void DisposeOpcodeList(opcodeListEntry *opcodelist)
    {
      csoundDisposeOpcodeList(csound, opcodelist);
    }
    int AppendOpcode(const char *opname, int dsblksiz, int thread,
                     const char *outypes, const char *intypes,
                     int (*iopadr)(CSOUND *, void *),
                     int (*kopadr)(CSOUND *, void *),
                     int (*aopadr)(CSOUND *, void *))
    {
      return csoundAppendOpcode(csound, opname, dsblksiz, thread,
                                outypes, intypes, iopadr, kopadr, aopadr);
    }
    // miscellaneous functions
    void SetYieldCallback(int (*yieldCallback)(CSOUND *))
    {
      csoundSetYieldCallback(csound, yieldCallback);
    }
    // real-time audio play and record
    void SetPlayopenCallback(int (*playopen__)(CSOUND *,
                                               const csRtAudioParams *parm))
    {
      csoundSetPlayopenCallback(csound, playopen__);
    }
    void SetRtplayCallback(void (*rtplay__)(CSOUND *,
                                            const MYFLT *outBuf, int nbytes))
    {
      csoundSetRtplayCallback(csound, rtplay__);
    }
    void SetRecopenCallback(int (*recopen_)(CSOUND *,
                                            const csRtAudioParams *parm))
    {
      csoundSetRecopenCallback(csound, recopen_);
    }
    void SetRtrecordCallback(int (*rtrecord__)(CSOUND *,
                                               MYFLT *inBuf, int nbytes))
    {
      csoundSetRtrecordCallback(csound, rtrecord__);
    }
    void SetRtcloseCallback(void (*rtclose__)(CSOUND *))
    {
      csoundSetRtcloseCallback(csound, rtclose__);
    }
    // --------
    int GetDebug()
    {
      return csoundGetDebug(csound);
    }
    void SetDebug(int debug)
    {
      csoundSetDebug(csound, debug);
    }
    int TableLength(int table)
    {
      return csoundTableLength(csound, table);
    }
    MYFLT TableGet(int table, int index)
    {
      return csoundTableGet(csound, table, index);
    }
    void TableSet(int table, int index, MYFLT value)
    {
      return csoundTableSet(csound, table, index, value);
    }
    MYFLT *GetTable(int tableNum, int &tableLength)
    {
      MYFLT *ftable;
      int   tmp;
      ftable = csoundGetTable(csound, tableNum, &tmp);
      tableLength = tmp;
      return ftable;
    }
    int CreateGlobalVariable(const char *name, size_t nbytes)
    {
      return csoundCreateGlobalVariable(csound, name, nbytes);
    }
    void *QueryGlobalVariable(const char *name)
    {
      return csoundQueryGlobalVariable(csound, name);
    }
    void *QueryGlobalVariableNoCheck(const char *name)
    {
      return csoundQueryGlobalVariableNoCheck(csound, name);
    }
    int DestroyGlobalVariable(const char *name)
    {
      return csoundDestroyGlobalVariable(csound, name);
    }
    void **GetRtRecordUserData()
    {
      return csoundGetRtRecordUserData(csound);
    }
    void **GetRtPlayUserData()
    {
      return csoundGetRtPlayUserData(csound);
    }
    int RegisterSenseEventCallback(void (*func)(CSOUND *, void *),
                                   void *userData)
    {
      return csoundRegisterSenseEventCallback(csound, func, userData);
    }
    int RunUtility(const char *name, int argc, char **argv)
    {
      return csoundRunUtility(csound, name, argc, argv);
    }
    char **ListUtilities()
    {
      return csoundListUtilities(csound);
    }
    const char *GetUtilityDescription(const char *utilName)
    {
      return csoundGetUtilityDescription(csound, utilName);
    }
    int GetChannelPtr(MYFLT* &p, const char *name, int type)
    {
      MYFLT *tmp;
      int   retval;
      retval = csoundGetChannelPtr(csound, &tmp, name, type);
      p = tmp;
      return retval;
    }
    int ListChannels(char** &names, int* &types)
    {
      char  **tmp1;
      int   *tmp2;
      int   retval;
      retval = csoundListChannels(csound, &tmp1, &tmp2);
      names = tmp1;
      types = tmp2;
      return retval;
    }
    int SetControlChannelParams(const char *name,
                                int type, MYFLT dflt, MYFLT min, MYFLT max)
    {
      return csoundSetControlChannelParams(csound, name, type, dflt, min, max);
    }
    int GetControlChannelParams(const char *name,
                                MYFLT &dflt, MYFLT &min, MYFLT &max)
    {
      MYFLT tmp1 = (MYFLT) 0, tmp2 = (MYFLT) 0, tmp3 = (MYFLT) 0;
      int   retval;
      retval = csoundGetControlChannelParams(csound, name, &tmp1, &tmp2, &tmp3);
      dflt = tmp1;
      min = tmp2;
      max = tmp3;
      return retval;
    }
    // cfgvar.h interface
    int CreateConfigurationVariable(const char *name, void *p,
                                    int type, int flags, void *min, void *max,
                                    const char *shortDesc, const char *longDesc)
    {
      return csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                               min, max, shortDesc, longDesc);
    }
    int CopyGlobalConfigurationVariable(const char *name, void *p)
    {
      return csoundCopyGlobalConfigurationVariable(csound, name, p);
    }
    int CopyGlobalConfigurationVariables()
    {
      return csoundCopyGlobalConfigurationVariables(csound);
    }
    int SetConfigurationVariable(const char *name, void *value)
    {
      return csoundSetConfigurationVariable(csound, name, value);
    }
    int ParseConfigurationVariable(const char *name, const char *value)
    {
      return csoundParseConfigurationVariable(csound, name, value);
    }
    csCfgVariable_t *QueryConfigurationVariable(const char *name)
    {
      return csoundQueryConfigurationVariable(csound, name);
    }
    csCfgVariable_t **ListConfigurationVariables()
    {
      return csoundListConfigurationVariables(csound);
    }
    int DeleteConfigurationVariable(const char *name)
    {
      return csoundDeleteConfigurationVariable(csound, name);
    }
    // constructors
    // FIXME: should throw exception on failure ?
    Csound()
    {
      csound = csoundCreate((void*) 0);
    }
    Csound(void *hostData)
    {
      csound = csoundCreate(hostData);
    }
    // destructor
    ~Csound()
    {
      csoundReset(csound);
      csoundDestroy(csound);
    }
};

// thread lock

class CsoundThreadLock {
  protected:
    void  *threadLock;
  public:
    int Lock(size_t milliseconds)
    {
      return csoundWaitThreadLock(threadLock, milliseconds);
    }
    void Lock()
    {
      csoundWaitThreadLockNoTimeout(threadLock);
    }
    int TryLock()
    {
      return csoundWaitThreadLock(threadLock, (size_t) 0);
    }
    void Unlock()
    {
      csoundNotifyThreadLock(threadLock);
    }
    // constructors
    // FIXME: should throw exception on failure ?
    CsoundThreadLock()
    {
      threadLock = csoundCreateThreadLock();
    }
    CsoundThreadLock(int locked)
    {
      threadLock = csoundCreateThreadLock();
      if (locked)
        csoundWaitThreadLock(threadLock, (size_t) 0);
    }
    // destructor
    ~CsoundThreadLock()
    {
      csoundDestroyThreadLock(threadLock);
    }
};

// Mersenne Twister (MT19937) pseudo-random number generator

class CsoundRandMT {
  protected:
    CsoundRandMTState   mt;
  public:
    uint32_t Random()
    {
      return csoundRandMT(&mt);
    }
    void Seed(uint32_t seedVal)
    {
      csoundSeedRandMT(&mt, (uint32_t*) 0, seedVal);
    }
    void Seed(const uint32_t *initKey, int keyLength)
    {
      csoundSeedRandMT(&mt, initKey, (uint32_t) keyLength);
    }
    // constructors
    CsoundRandMT()
    {
      csoundSeedRandMT(&mt, (uint32_t*) 0, csoundGetRandomSeedFromTime());
    }
    CsoundRandMT(uint32_t seedVal)
    {
      csoundSeedRandMT(&mt, (uint32_t*) 0, seedVal);
    }
    CsoundRandMT(const uint32_t *initKey, int keyLength)
    {
      csoundSeedRandMT(&mt, initKey, (uint32_t) keyLength);
    }
};

// timer (csoundInitialize() should be called before using this)

class CsoundTimer {
  protected:
    RTCLOCK rt;
  public:
    double GetRealTime()
    {
      return csoundGetRealTime(&rt);
    }
    double GetCPUTime()
    {
      return csoundGetCPUTime(&rt);
    }
    void Reset()
    {
      csoundInitTimerStruct(&rt);
    }
    // constructor
    CsoundTimer()
    {
      csoundInitTimerStruct(&rt);
    }
};

#endif  // __CSOUND_HPP__

