/*
    csound.hpp:

    Copyright (C) 2005 Istvan Varga, Michael Gogins

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

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, this file does not
    by itself cause the resulting executable or library to be covered
    by the GNU Lesser General Public License. This exception does not
    however invalidate any other reasons why the library or executable
    file might be covered by the GNU Lesser General Public License.
*/

#ifndef __CSOUND_HPP__
#define __CSOUND_HPP__

#ifdef SWIG
%module csnd6
%{
#include "csound.h"
%}
#else
#include "csound.h"
#if defined(HAVE_PTHREAD_SPIN_LOCK) && !defined(SWIG)
#include <pthread.h>
#endif
#ifdef __BUILDING_CSOUND_INTERFACES
#endif
#endif

#ifdef SWIGPYTHON
#define MESSAGE_BUFFER_LENGTH 8192
struct PUBLIC pycbdata {
  PyObject *mfunc,*invalfunc,*outvalfunc;
  PyObject *midiinopenfunc, *midireadfunc, *midiinclosefunc;
  PyObject *hostdata;
  char messageBuffer[MESSAGE_BUFFER_LENGTH];
  int messageBufferIndex;
};
#endif

#if defined(__cplusplus)

#if defined(HAVE_PTHREAD_SPIN_LOCK) && !defined(SWIG)
struct Spinlock
{
    pthread_spinlock_t lock_;
    Spinlock()
    {
        pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
    }
    ~Spinlock()
    {
        pthread_spin_destroy(&lock_);
    }
    void lock()
    {
        pthread_spin_lock(&lock_);
    }
    void unlock()
    {
        pthread_spin_unlock(&lock_);
    }
};

struct Spinlocker
{
    Spinlock &spinlock;
    #if (__cplusplus >= 201103L)
    explicit
    #endif
    Spinlocker(Spinlock &spinlock_) : spinlock(spinlock_)
    {
        spinlock.lock();
    }
    ~Spinlocker()
    {
        spinlock.unlock();
    }
};
#endif


/**
 * C++ interface to the "C" Csound API.
 */
class PUBLIC Csound
{
protected:
  CSOUND *csound;
public:
  void *pydata;
public:
  virtual CSOUND *GetCsound()
  {
    return csound;
  }
  virtual void SetCsound(CSOUND *csound_)
  {
    csound = csound_;
  }
  virtual int InitializeCscore(FILE *insco, FILE *outsco)
  {
    return csoundInitializeCscore(csound, insco, outsco);
  }
  virtual int LoadPlugins(const char *dir){
    return csoundLoadPlugins(csound, dir);
  }
  virtual int GetVersion()
  {
    return csoundGetVersion();
  }
  virtual int GetAPIVersion()
  {
    return csoundGetAPIVersion();
  }
  virtual void *GetHostData()
  {
    return csoundGetHostData(csound);
  }
  virtual void SetHostData(void *hostData)
  {
    csoundSetHostData(csound, hostData);
  }
  virtual const char *GetEnv(const char *name)
  {
    return csoundGetEnv(csound, name);
  }
  virtual int SetGlobalEnv(const char *name,const char *value)
  {
    return csoundSetGlobalEnv(name, value);
  }
  virtual int SetOption(const char *option)
  {
   return csoundSetOption(csound, option);
  }
  virtual void SetParams(CSOUND_PARAMS *p){
   csoundSetParams(csound, p);
  }
  virtual void GetParams(CSOUND_PARAMS *p){
   csoundGetParams(csound, p);
  }
  virtual void SetOutput(const char *name,const char *type,const char *format){
    csoundSetOutput(csound, name, type, format);
  }
  virtual void SetInput(const char *name){
    csoundSetInput(csound, name);
  }
  virtual void SetMIDIInput(const char *name){
    csoundSetMIDIInput(csound,name);
  }
  virtual void SetMIDIFileInput(const char *name){
    csoundSetMIDIFileInput(csound,name);
  }
  virtual void SetMIDIOutput(const char *name){
     csoundSetMIDIOutput(csound,name);
  }
   virtual void SetMIDIFileOutput(const char *name){
    csoundSetMIDIFileOutput(csound,name);
  }
  virtual TREE *ParseOrc(const char *str)
  {
    return csoundParseOrc(csound, str);
  }
  virtual int CompileTree(TREE *root)
  {
    return csoundCompileTree(csound, root);
  }
  virtual void DeleteTree(TREE *root)
  {
    csoundDeleteTree(csound, root);
  }
  virtual int CompileOrc(const char *str)
  {
    return csoundCompileOrc(csound, str);
  }
  virtual MYFLT EvalCode(const char *str)
  {
    return csoundEvalCode(csound, str);
  }
  virtual int ReadScore(const char *str)
  {
    return csoundReadScore(csound, str);
  }
  virtual int CompileArgs(int argc,const char **argv)
  {
    return csoundCompileArgs(csound, argc, argv);
  }
  virtual int Compile(int argc,const char **argv)
  {
    return csoundCompile(csound, argc, argv);
  }
  virtual int Compile(const char *csdName)
  {
    const char  *argv[3];
    argv[0] = (char*)"csound";
    argv[1] = csdName;
    argv[2] = (char*) 0;
    return csoundCompile(csound, 2, &(argv[0]));
  }
  virtual int Compile(const char *orcName,const  char *scoName)
  {
    const char  *argv[4];
    argv[0] = (char*)"csound";
    argv[1] = orcName;
    argv[2] = scoName;
    argv[3] = (char*) 0;
    return csoundCompile(csound, 3, &(argv[0]));
  }
  virtual int Compile(const char *arg1,const  char *arg2,const  char *arg3)
  {
    const char  *argv[5];
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = (char*) 0;
    return csoundCompile(csound, 4, &(argv[0]));
  }
  virtual int Compile(const char *arg1,const  char *arg2,const  char *arg3,const  char *arg4)
  {
    const char  *argv[6];
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;
    argv[5] = (char*) 0;
    return csoundCompile(csound, 5, &(argv[0]));
  }
  virtual int Compile(const char *arg1,const  char *arg2,const char *arg3,
                      const char *arg4,const  char *arg5)
  {
    const char  *argv[7];
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;
    argv[5] = arg5;
    argv[6] = (char*) 0;
    return csoundCompile(csound, 6, &(argv[0]));
  }
  virtual int CompileCsd(const char *csd)
  {
    return csoundCompileCsd(csound, csd);
  }
  virtual int CompileCsdText(const char *csd_text)
  {
    return csoundCompileCsdText(csound, csd_text);
  }
  virtual int Start()
  {
    return csoundStart(csound);
  }
  virtual int Perform()
  {
    return csoundPerform(csound);
  }
  virtual int Perform(int argc, const char **argv)
  {
    int result = csoundCompile(csound, argc, argv);
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int Perform(const char *csdName)
  {
    const char  *argv[3];
    int result;
    argv[0] = (char*)"csound";
    argv[1] = csdName;
    argv[2] = (char*) 0;
    result = csoundCompile(csound, 2, &(argv[0]));
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int Perform(const char *orcName, const char *scoName)
  {
    const char  *argv[4];
    int result;
    argv[0] = (char*)"csound";
    argv[1] = orcName;
    argv[2] = scoName;
    argv[3] = (char*) 0;
    result = csoundCompile(csound, 3, &(argv[0]));
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int Perform(const char *arg1, const char *arg2, const char *arg3)
  {
    const char  *argv[5];
    int result;
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = (char*) 0;
    result = csoundCompile(csound, 4, &(argv[0]));
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int Perform(const char *arg1, const  char *arg2, const  char *arg3, const  char *arg4)
  {
    const char  *argv[6];
    int result;
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;
    argv[5] = (char*) 0;
    result = csoundCompile(csound, 5, &(argv[0]));
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int Perform(const char *arg1, const  char *arg2, const  char *arg3,
                      const char *arg4, const  char *arg5)
  {
    const char  *argv[7];
    int result;
    argv[0] = (char*)"csound";
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;
    argv[5] = arg5;
    argv[6] = (char*) 0;
    result = csoundCompile(csound, 6, &(argv[0]));
    if (result == 0) {
      result = csoundPerform(csound);
    }
    csoundCleanup(csound);
    return (result >= 0 ? 0 : result);
  }
  virtual int PerformKsmps()
  {
    return csoundPerformKsmps(csound);
  }
  virtual int PerformBuffer()
  {
    return csoundPerformBuffer(csound);
  }
  virtual void Stop()
  {
    csoundStop(csound);
  }
  virtual int Cleanup()
  {
    return csoundCleanup(csound);
  }
  virtual void Reset()
  {
    csoundReset(csound);
  }
  virtual MYFLT GetSr()
  {
    return csoundGetSr(csound);
  }
  virtual MYFLT GetKr()
  {
    return csoundGetKr(csound);
  }
  virtual int GetKsmps()
  {
    return csoundGetKsmps(csound);
  }
  virtual int GetNchnls()
  {
    return csoundGetNchnls(csound);
  }
  virtual int GetNchnlsInput()
  {
    return csoundGetNchnlsInput(csound);
  }
  virtual MYFLT Get0dBFS()
  {
    return csoundGet0dBFS(csound);
  }
  virtual long GetInputBufferSize()
  {
    return csoundGetInputBufferSize(csound);
  }
  virtual long GetOutputBufferSize()
  {
    return csoundGetOutputBufferSize(csound);
  }
  virtual MYFLT *GetInputBuffer()
  {
    return csoundGetInputBuffer(csound);
  }
  virtual MYFLT *GetOutputBuffer()
  {
    return csoundGetOutputBuffer(csound);
  }
  virtual MYFLT *GetSpin()
  {
    return csoundGetSpin(csound);
  }
  virtual MYFLT *GetSpout()
  {
    return csoundGetSpout(csound);
  }
  virtual const char *GetOutputName()
  {
    return csoundGetOutputName(csound);
  }
  virtual long GetCurrentTimeSamples(){
    return csoundGetCurrentTimeSamples(csound);
  }
  virtual void SetHostImplementedAudioIO(int state, int bufSize)
  {
    csoundSetHostImplementedAudioIO(csound, state, bufSize);
  }
  virtual void SetHostImplementedMIDIIO(int state)
  {
    csoundSetHostImplementedMIDIIO(csound, state);
  }
  virtual double GetScoreTime()
  {
    return csoundGetScoreTime(csound);
  }
  virtual int IsScorePending()
  {
    return csoundIsScorePending(csound);
  }
  virtual void SetScorePending(int pending)
  {
    csoundSetScorePending(csound, pending);
  }
  virtual MYFLT GetScoreOffsetSeconds()
  {
    return csoundGetScoreOffsetSeconds(csound);
  }
  virtual void SetScoreOffsetSeconds(double time)
  {
    csoundSetScoreOffsetSeconds(csound, (MYFLT) time);
  }
  virtual void RewindScore()
  {
    csoundRewindScore(csound);
  }
  virtual void SetCscoreCallback(void (*cscoreCallback_)(CSOUND *))
  {
    csoundSetCscoreCallback(csound, cscoreCallback_);
  }
  virtual int ScoreSort(FILE *inFile, FILE *outFile)
  {
    return csoundScoreSort(csound, inFile, outFile);
  }
  virtual int ScoreExtract(FILE *inFile, FILE *outFile, FILE *extractFile)
  {
    return csoundScoreExtract(csound, inFile, outFile, extractFile);
  }
  virtual void Message(const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundMessageV(csound, 0, format, args);
    va_end(args);
  }
  virtual void MessageS(int attr, const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    csoundMessageV(csound, attr, format, args);
    va_end(args);
  }
  virtual void MessageV(int attr, const char *format, va_list args)
  {
    csoundMessageV(csound, attr, format, args);
  }
  virtual void SetMessageCallback(
      void (*csoundMessageCallback_)(CSOUND *, int attr,
                                     const char *format, va_list valist))
  {
    csoundSetMessageCallback(csound, csoundMessageCallback_);
  }
  virtual int GetMessageLevel()
  {
    return csoundGetMessageLevel(csound);
  }
  virtual void SetMessageLevel(int messageLevel)
  {
    csoundSetMessageLevel(csound, messageLevel);
  }
  virtual void InputMessage(const char *message)
  {
    csoundInputMessage(csound, message);
  }
  virtual void KeyPressed(char c)
  {
    csoundKeyPress(csound, c);
  }
  virtual int ScoreEvent(char type, const MYFLT *pFields, long numFields)
  {
    return csoundScoreEvent(csound, type, pFields, numFields);
  }
  virtual int ScoreEventAbsolute(char type, const MYFLT *pFields,
                                 long numFields, double time_ofs)
  {
    return csoundScoreEventAbsolute(csound, type, pFields, numFields, time_ofs);
  }
  virtual void SetExternalMidiInOpenCallback(
      int (*func)(CSOUND *, void **, const char *))
  {
    csoundSetExternalMidiInOpenCallback(csound, func);
  }
  virtual void SetExternalMidiReadCallback(
      int (*func)(CSOUND *, void *, unsigned char *, int))
  {
    csoundSetExternalMidiReadCallback(csound, func);
  }
  virtual void SetExternalMidiInCloseCallback(
      int (*func)(CSOUND *, void *))
  {
    csoundSetExternalMidiInCloseCallback(csound, func);
  }
  virtual void SetExternalMidiOutOpenCallback(
      int (*func)(CSOUND *, void **, const char *))
  {
    csoundSetExternalMidiOutOpenCallback(csound, func);
  }
  virtual void SetExternalMidiWriteCallback(
      int (*func)(CSOUND *, void *, const unsigned char *, int))
  {
    csoundSetExternalMidiWriteCallback(csound, func);
  }
  virtual void SetExternalMidiOutCloseCallback(
      int (*func)(CSOUND *, void *))
  {
    csoundSetExternalMidiOutCloseCallback(csound, func);
  }
  virtual void SetExternalMidiErrorStringCallback(
      const char *(*func)(int))
  {
    csoundSetExternalMidiErrorStringCallback(csound, func);
  }
  virtual int SetIsGraphable(int isGraphable)
  {
    return csoundSetIsGraphable(csound, isGraphable);
  }
  virtual void SetMakeGraphCallback(
      void (*makeGraphCallback_)(CSOUND *, WINDAT *windat, const char *name))
  {
    csoundSetMakeGraphCallback(csound, makeGraphCallback_);
  }
  virtual void SetDrawGraphCallback(
      void (*drawGraphCallback_)(CSOUND *, WINDAT *windat))
  {
    csoundSetDrawGraphCallback(csound, drawGraphCallback_);
  }
  virtual void SetKillGraphCallback(
      void (*killGraphCallback_)(CSOUND *, WINDAT *windat))
  {
    csoundSetKillGraphCallback(csound, killGraphCallback_);
  }
  virtual void SetExitGraphCallback(
      int (*exitGraphCallback_)(CSOUND *))
  {
    csoundSetExitGraphCallback(csound, exitGraphCallback_);
  }
  virtual int NewOpcodeList(opcodeListEntry* &opcodelist)
  {
    opcodeListEntry *tmp = (opcodeListEntry*) 0;
    int retval;
    retval = csoundNewOpcodeList(csound, &tmp);
    opcodelist = tmp;
    return retval;
  }
  virtual void DisposeOpcodeList(opcodeListEntry *opcodelist)
  {
    csoundDisposeOpcodeList(csound, opcodelist);
  }
  virtual int AppendOpcode(const char *opname, int dsblksiz, int flags,
                           int thread,
                           const char *outypes, const char *intypes,
                           int (*iopadr)(CSOUND *, void *),
                           int (*kopadr)(CSOUND *, void *),
                           int (*aopadr)(CSOUND *, void *))
  {
      return csoundAppendOpcode(csound, opname, dsblksiz, flags, thread,
                              outypes, intypes, iopadr, kopadr, aopadr);
  }
  virtual void SetYieldCallback(int (*yieldCallback_)(CSOUND *))
  {
    csoundSetYieldCallback(csound, yieldCallback_);
  }
  virtual void SetPlayopenCallback(
      int (*playopen__)(CSOUND *, const csRtAudioParams *parm))
  {
    csoundSetPlayopenCallback(csound, playopen__);
  }
  virtual void SetRtplayCallback(
      void (*rtplay__)(CSOUND *, const MYFLT *outBuf, int nbytes))
  {
    csoundSetRtplayCallback(csound, rtplay__);
  }
  virtual void SetRecopenCallback(
      int (*recopen_)(CSOUND *, const csRtAudioParams *parm))
  {
    csoundSetRecopenCallback(csound, recopen_);
  }
  virtual void SetRtrecordCallback(
      int (*rtrecord__)(CSOUND *, MYFLT *inBuf, int nbytes))
  {
    csoundSetRtrecordCallback(csound, rtrecord__);
  }
  virtual void SetRtcloseCallback(
      void (*rtclose__)(CSOUND *))
  {
    csoundSetRtcloseCallback(csound, rtclose__);
  }
  virtual int GetDebug()
  {
    return csoundGetDebug(csound);
  }
  virtual void SetDebug(int debug)
  {
    csoundSetDebug(csound, debug);
  }
  virtual int TableLength(int table)
  {
    return csoundTableLength(csound, table);
  }
  virtual MYFLT TableGet(int table, int index)
  {
    return csoundTableGet(csound, table, index);
  }
  virtual void TableSet(int table, int index, double value)
  {
    csoundTableSet(csound, table, index, (MYFLT) value);
  }
  virtual int GetTable(MYFLT* &tablePtr, int tableNum)
  {
    MYFLT *ftable;
    int   tmp;
    tmp = csoundGetTable(csound, &ftable, tableNum);
    tablePtr = ftable;
    return tmp;
  }
  virtual void TableCopyOut(int table, MYFLT *dest){
    csoundTableCopyOut(csound,table,dest);
  }
  virtual void TableCopyIn(int table, MYFLT *src){
    csoundTableCopyIn(csound,table,src);
  }
  virtual int CreateGlobalVariable(const char *name, size_t nbytes)
  {
    return csoundCreateGlobalVariable(csound, name, nbytes);
  }
  virtual void *QueryGlobalVariable(const char *name)
  {
    return csoundQueryGlobalVariable(csound, name);
  }
  virtual void *QueryGlobalVariableNoCheck(const char *name)
  {
    return csoundQueryGlobalVariableNoCheck(csound, name);
  }
  virtual int DestroyGlobalVariable(const char *name)
  {
    return csoundDestroyGlobalVariable(csound, name);
  }
  virtual void **GetRtRecordUserData()
  {
    return csoundGetRtRecordUserData(csound);
  }
  virtual void **GetRtPlayUserData()
  {
    return csoundGetRtPlayUserData(csound);
  }
  virtual int RegisterSenseEventCallback(void (*func)(CSOUND *, void *),
                                         void *userData)
  {
    return csoundRegisterSenseEventCallback(csound, func, userData);
  }
  virtual int RunUtility(const char *name, int argc, char **argv)
  {
    return csoundRunUtility(csound, name, argc, argv);
  }
  virtual char **ListUtilities()
  {
    return csoundListUtilities(csound);
  }
  virtual void DeleteUtilityList(char **lst)
  {
    csoundDeleteUtilityList(csound, lst);
  }
  virtual const char *GetUtilityDescription(const char *utilName)
  {
    return csoundGetUtilityDescription(csound, utilName);
  }
  virtual int GetChannelPtr(MYFLT* &p, const char *name, int type)
  {
    MYFLT *tmp;
    int   retval;
    if(strlen(name) == 0) return CSOUND_ERROR;
    retval = csoundGetChannelPtr(csound, &tmp, name, type);
    p = tmp;
    return retval;
  }
  virtual int ListChannels(controlChannelInfo_t* &lst)
  {
    controlChannelInfo_t  *tmp;
    int                     retval;
    retval = csoundListChannels(csound, &tmp);
    lst = tmp;
    return retval;
  }
  virtual void DeleteChannelList(controlChannelInfo_t *lst)
  {
    csoundDeleteChannelList(csound, lst);
  }
  virtual int SetControlChannelHints(const char *name,
                                      controlChannelHints_t hints)
  {
    return csoundSetControlChannelHints(csound, name, hints);
  }
  virtual int GetControlChannelHints(const char *name, controlChannelHints_t *hints)
  {
    return csoundGetControlChannelHints(csound, name, hints);
  }
  virtual void SetChannel(const char *name, double value)
  {
    csoundSetControlChannel(csound,name,value);
  }
  virtual void SetControlChannel(const char *name, double value)
  {
    csoundSetControlChannel(csound,name,value);
  }
  virtual void SetChannel(const char *name, char *string)
  {
   csoundSetStringChannel(csound,name,string);
  }
  virtual void SetStringChannel(const char *name, char *string)
  {
   csoundSetStringChannel(csound,name,string);
  }
  virtual void SetChannel(const char *name, MYFLT *samples)
  {
   csoundSetAudioChannel(csound,name,samples);
  }
  virtual MYFLT GetChannel(const char *name, int *err = NULL)
  {
   return csoundGetControlChannel(csound,name, err);
  }
  virtual MYFLT GetControlChannel(const char *name, int *err = NULL)
  {
   return csoundGetControlChannel(csound,name, err);
  }
  virtual void GetStringChannel(const char *name, char *string)
  {
    csoundGetStringChannel(csound,name,string);
  }
  virtual void GetAudioChannel(const char *name, MYFLT *samples)
  {
    csoundGetAudioChannel(csound,name,samples);
  }
  virtual int PvsinSet(const PVSDATEXT* value, const char *name)
  {
    return csoundSetPvsChannel(csound, value, name);
  }

  virtual int PvsoutGet(PVSDATEXT* value, const char *name)
  {
    return csoundGetPvsChannel(csound, value, name);
  }

  virtual void SetInputChannelCallback(channelCallback_t inputChannelCalback){
    csoundSetInputChannelCallback(csound, inputChannelCalback);
  }

  virtual void SetOutputChannelCallback(channelCallback_t outputChannelCalback){
    csoundSetOutputChannelCallback(csound, outputChannelCalback);
  }

  // cfgvar.h interface
  virtual int CreateConfigurationVariable(const char *name, void *p,
                                          int type, int flags,
                                          void *min, void *max,
                                          const char *shortDesc,
                                          const char *longDesc)
  {
    return csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                             min, max, shortDesc, longDesc);
  }
  virtual int SetConfigurationVariable(const char *name, void *value)
  {
    return csoundSetConfigurationVariable(csound, name, value);
  }
  virtual int ParseConfigurationVariable(const char *name, const char *value)
  {
    return csoundParseConfigurationVariable(csound, name, value);
  }
  virtual csCfgVariable_t *QueryConfigurationVariable(const char *name)
  {
    return csoundQueryConfigurationVariable(csound, name);
  }
  virtual csCfgVariable_t **ListConfigurationVariables()
  {
    return csoundListConfigurationVariables(csound);
  }
  virtual int DeleteConfigurationVariable(const char *name)
  {
    return csoundDeleteConfigurationVariable(csound, name);
  }
  Csound()
  {
    csound = csoundCreate((CSOUND*) 0);
     #ifdef SWIGPYTHON
      pydata =(pycbdata *) new pycbdata;
      memset(pydata, 0, sizeof(pycbdata));
    ((pycbdata *)pydata)->mfunc = NULL;
    ((pycbdata *)pydata)->messageBufferIndex = 0;
    csoundSetHostData(csound, this);
    #else
    pydata = NULL;
    #endif
  }
  #if (__cplusplus >= 201103L)
  Csound(const Csound &other) = delete;
  explicit
  #endif
  Csound(CSOUND *csound_) : csound(csound_)
  {
     #ifdef SWIGPYTHON
      pydata =(pycbdata *) new pycbdata;
    ((pycbdata *)pydata)->mfunc = NULL;
    ((pycbdata *)pydata)->messageBufferIndex = 0;
    csoundSetHostData(csound, this);
    #else
    pydata = NULL;
    #endif

  }
    #if (__cplusplus >= 201103L)
    explicit
    #endif
    Csound(void *hostData)
    {
    csound = csoundCreate(hostData);
    #ifdef SWIGPYTHON
    pydata =(pycbdata *) new pycbdata;
    ((pycbdata *)pydata)->mfunc = NULL;
    ((pycbdata *)pydata)->messageBufferIndex = 0;
    csoundSetHostData(csound, this);
    #else
    pydata = NULL;
    #endif
  }
  virtual ~Csound()
  {
    csoundDestroy(csound);
    #ifdef SWIGPYTHON
    ((pycbdata *)pydata)->mfunc = NULL;
    delete (pycbdata *)pydata;
    #endif

  }
  virtual void CreateMessageBuffer(int toStdOut)
  {
    csoundCreateMessageBuffer(csound, toStdOut);
  }
  virtual const char *GetFirstMessage()
  {
    return csoundGetFirstMessage(csound);
  }
  virtual int GetFirstMessageAttr()
  {
    return csoundGetFirstMessageAttr(csound);
  }
  virtual void PopFirstMessage()
  {
    csoundPopFirstMessage(csound);
  }
  virtual int GetMessageCnt()
  {
    return csoundGetMessageCnt(csound);
  }
  virtual void DestroyMessageBuffer()
  {
    csoundDestroyMessageBuffer(csound);
  }
  virtual void AddSpinSample(int frame, int channel, MYFLT sample)
  {
    csoundAddSpinSample(csound, frame, channel, sample);
  }
    virtual void SetSpinSample(int frame, int channel, MYFLT sample)
  {
    csoundSetSpinSample(csound, frame, channel, sample);
  }
  virtual MYFLT GetSpoutSample(int frame, int channel) const
  {
    return csoundGetSpoutSample(csound, frame, channel);
  }
  virtual const char *GetInputName()
  {
    return csoundGetInputName(csound);
  }
  virtual void SetAudioChannel(const char *name, MYFLT *samples) {
    csoundSetAudioChannel(csound, name, samples);
  }
  virtual MYFLT SystemSr(MYFLT value) {
    return csoundSystemSr(csound, value);
  }
};

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

class CsoundMutex {
protected:
  void  *mutex_;
public:
  void Lock()
  {
    csoundLockMutex(mutex_);
  }
  // FIXME: this may be unimplemented on Windows
  int TryLock()
  {
    return csoundLockMutexNoWait(mutex_);
  }
  void Unlock()
  {
    csoundUnlockMutex(mutex_);
  }
  CsoundMutex()
  {
    mutex_ = csoundCreateMutex(1);
  }
  #if (__cplusplus >= 201103L)
  explicit
  #endif
  CsoundMutex(int isRecursive)
  {
    mutex_ = csoundCreateMutex(isRecursive);
  }
  ~CsoundMutex()
  {
    csoundDestroyMutex(mutex_);
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
  CsoundRandMT()
  {
    csoundSeedRandMT(&mt, (uint32_t*) 0, csoundGetRandomSeedFromTime());
  }
  #if (__cplusplus >= 201103L)
  explicit
  #endif
  CsoundRandMT(uint32_t seedVal)
  {
    csoundSeedRandMT(&mt, (uint32_t*) 0, seedVal);
  }
  CsoundRandMT(const uint32_t *initKey, int keyLength)
  {
    csoundSeedRandMT(&mt, initKey, (uint32_t) keyLength);
  }
  ~CsoundRandMT()
  {
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
  ~CsoundTimer()
  {
  }
};

#endif  // __cplusplus

#endif  // __CSOUND_HPP__
