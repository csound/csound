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
  virtual int CompileOrc(const char *str, int async = 0) {
    return csoundCompileOrc(csound, str, async);
  }
  virtual MYFLT EvalCode(const char *str)
  {
    return csoundEvalCode(csound, str);
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
  virtual int Compile(const char *orcName, const  char *scoName)
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
  virtual int CompileCSD(const char *csd, int mode)
  {
    return csoundCompileCSD(csound, csd, mode);
  }
  virtual int Start()
  {
    return csoundStart(csound);
  }
  virtual int PerformKsmps()
  {
    return csoundPerformKsmps(csound);
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
  virtual int GetChannels(int isInput = 0)
  {
    return csoundGetChannels(csound,isInput);
  }
  virtual MYFLT Get0dBFS()
  {
    return csoundGet0dBFS(csound);
  }
  virtual MYFLT *GetSpin()
  {
    return csoundGetSpin(csound);
  }
  virtual const MYFLT *GetSpout()
  {
    return csoundGetSpout(csound);
  }
  virtual long GetCurrentTimeSamples(){
    return csoundGetCurrentTimeSamples(csound);
  }
  virtual void SetHostAudioIO()
  {
    csoundSetHostAudioIO(csound);
  }
  virtual void SetHostMIDIIO()
  {
    csoundSetHostMIDIIO(csound);
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
  virtual void KeyPressed(char c)
  {
    csoundKeyPress(csound, c);
  }
  virtual void Event(int type, MYFLT *pFields, long numFields, int async = 0)
  {
    csoundEvent(csound, type, pFields, numFields, async);
  }
  virtual void EventString(const char *s, int async = 0)
  {
    csoundEventString(csound, s, async);
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
  virtual int AppendOpcode(const char *opname, int dsblksiz, int flags,
                           const char *outypes, const char *intypes, int(*init)(CSOUND *, void *),
                           int(*perf)(CSOUND *, void *), int(*deinit)(CSOUND *, void *))
  {
    return csoundAppendOpcode(csound, opname, dsblksiz, flags,
                              outypes, intypes, init, perf, deinit);
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
  virtual int GetTable(MYFLT **tablePtr, int tableNum){
    return csoundGetTable(csound, tablePtr, tableNum);
  }

  virtual int GetTableArgs(MYFLT **argsPtr, int tableNum){
    return csoundGetTableArgs(csound, argsPtr, tableNum);
  }

  virtual int GetChannelPtr(void* &p, const char *name, int type)
  {
    MYFLT *tmp;
    int   retval;
    if(strlen(name) == 0) return CSOUND_ERROR;
    retval = csoundGetChannelPtr(csound, (void **) &tmp, name, type);
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
  virtual void SetChannel(const char *name, const char *string)
  {
    csoundSetStringChannel(csound,name,string);
  }
  virtual void SetStringChannel(const char *name, const char *string)
  {
    csoundSetStringChannel(csound,name,string);
  }
  virtual void SetChannel(const char *name, const MYFLT *samples)
  {
    csoundSetAudioChannel(csound,name,samples);
  }
  virtual MYFLT GetChannel(const char *name, int *err = NULL)
  {
    return csoundGetControlChannel(csound,name,err);
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
  virtual ARRAYDAT *InitArrayChannel(CSOUND *csound, const char *name,
                                          const char *type, int dimensions,
                                           const int *sizes) {
    return csoundInitArrayChannel(csound, name, type, dimensions, sizes);
  }
  virtual PVSDAT *InitPvsChannel(const char* name, int size, int overlap,
                                 int winsize, int wintype, int format) {
    return csoundInitPvsChannel(csound, name, size, overlap, winsize,
                                wintype, format);
  }
  Csound()
    {
      csound = csoundCreate((CSOUND*) 0, NULL);
    }
#if (__cplusplus >= 201103L)
  Csound(const Csound &other) = delete;
  explicit
#endif
    Csound(CSOUND *csound_) : csound(csound_)
  {

  }
#if (__cplusplus >= 201103L)
  explicit
#endif
    Csound(void *hostData, const char *opcodedir = NULL)
  {
    csound = csoundCreate(hostData, opcodedir);
  }
  virtual ~Csound()
  {
    csoundDestroy(csound);
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
  virtual void SetAudioChannel(const char *name, MYFLT *samples) {
    csoundSetAudioChannel(csound, name, samples);
  }
  virtual MYFLT SystemSr(MYFLT value) {
    return csoundSystemSr(csound, value);
  }
};

#endif  // __cplusplus

#endif  // __CSOUND_HPP__
