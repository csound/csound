/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
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
 */
#include "CppSound.hpp"

// Functions that alias the Csound "C" API declared in csound.h.

PUBLIC int CppSound::Initialize(int *argc, char ***argv, int flags)
{
  return csoundInitialize(argc, argv, flags);
}

PUBLIC CSOUND *CppSound::Create(void *hostData)
{
  return csoundCreate(hostData);
}

PUBLIC int CppSound::PreCompile()
{
  return csoundPreCompile(csound);
}

PUBLIC int CppSound::InitializeCscore(FILE* insco, FILE* outsco)
{
  return csoundInitializeCscore(csound, insco, outsco);
}

PUBLIC int CppSound::QueryInterface(const char *name, void **iface, int *version)
{
  return csoundQueryInterface(name, iface, version);
}

PUBLIC void CppSound::Destroy()
{
  csoundDestroy(csound);
}

PUBLIC int CppSound::GetVersion(void)
{
  return csoundGetVersion();
}

PUBLIC int CppSound::GetAPIVersion(void)
{
  return csoundGetAPIVersion();
}

PUBLIC void *CppSound::GetHostData()
{
  return csoundGetHostData(csound);
}

PUBLIC void CppSound::SetHostData(void *hostData)
{
  csoundSetHostData(csound, hostData);
}

PUBLIC const char *CppSound::GetEnv(const char *name)
{
  return csoundGetEnv(csound, name);
}

PUBLIC int CppSound::SetDefaultEnv(const char *name, const char *value)
{
  return csoundSetDefaultEnv(name, value);
}

PUBLIC int CppSound::Perform(int argc, char **argv)
{
  return csoundPerform(csound, argc, argv);
}

PUBLIC int CppSound::Compile(int argc, char **argv)
{
  return csoundCompile(csound, argc, argv);
}

PUBLIC int CppSound::PerformKsmps()
{
  return csoundPerformKsmps(csound);
}

PUBLIC int CppSound::PerformKsmpsAbsolute()
{
  return csoundPerformKsmpsAbsolute(csound);
}

PUBLIC int CppSound::Cleanup()
{
  return csoundCleanup(csound);
}

PUBLIC void CppSound::Reset()
{
  return csoundReset(csound);
}

PUBLIC MYFLT CppSound::GetSr()
{
  return csoundGetSr(csound);
}

PUBLIC MYFLT CppSound::GetKr()
{
  return csoundGetKr(csound);
}

PUBLIC int CppSound::GetKsmps()
{
  return csoundGetKsmps(csound);
}

PUBLIC int CppSound::GetNchnls()
{
  return csoundGetNchnls(csound);
}

PUBLIC MYFLT CppSound::Get0dBFS()
{
  return csoundGet0dBFS(csound);
}

PUBLIC int CppSound::GetStrVarMaxLen()
{
  return csoundGetStrVarMaxLen(csound);
}

PUBLIC int CppSound::GetSampleFormat()
{
  return csoundGetSampleFormat(csound);
}

PUBLIC int CppSound::GetSampleSize()
{
  return csoundGetSampleSize(csound);
}

PUBLIC long CppSound::GetInputBufferSize()
{
  return csoundGetInputBufferSize(csound);
}

PUBLIC long CppSound::GetOutputBufferSize()
{
  return csoundGetOutputBufferSize(csound);
}

PUBLIC MYFLT *CppSound::GetInputBuffer()
{
  return csoundGetInputBuffer(csound);
}

PUBLIC MYFLT *CppSound::GetOutputBuffer()
{
  return csoundGetOutputBuffer(csound);
}

PUBLIC MYFLT *CppSound::GetSpin()
{
  return csoundGetSpin(csound);
}

PUBLIC MYFLT *CppSound::GetSpout()
{
  return csoundGetSpout(csound);
}

PUBLIC const char *CppSound::GetOutputFileName()
{
  return csoundGetOutputFileName(csound);
}

PUBLIC void CppSound::SetHostImplementedAudioIO(int state, int bufSize)
{
  return csoundSetHostImplementedAudioIO(csound, state, bufSize);
}

PUBLIC MYFLT CppSound::GetScoreTime()
{
  return csoundGetScoreTime(csound);
}

PUBLIC int CppSound::IsScorePending()
{
  return csoundIsScorePending(csound);
}

PUBLIC void CppSound::SetScorePending(int pending)
{
  csoundSetScorePending(csound, pending);
}

PUBLIC MYFLT CppSound::GetScoreOffsetSeconds()
{
  return csoundGetScoreOffsetSeconds(csound);
}

PUBLIC void CppSound::SetScoreOffsetSeconds(MYFLT time)
{
  csoundSetScoreOffsetSeconds(csound, time);
}

PUBLIC void CppSound::RewindScore()
{
  csoundRewindScore(csound);
}

PUBLIC void CppSound::SetCscoreCallback(void (*cscoreCallback)(CSOUND *))
{
  csoundSetCscoreCallback(csound, cscoreCallback);
}

PUBLIC void CppSound::Message(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  csoundMessageV(csound, 0, format, args);
  va_end(args);
}

PUBLIC void CppSound::MessageS(int attr, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  csoundMessageV(csound, attr, format, args);
  va_end(args);
}

PUBLIC void CppSound::MessageV(int attr, const char *format, va_list args)
{
  csoundMessageV(csound, attr, format, args);
}

PUBLIC void CppSound::ThrowMessage(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  csoundThrowMessageV(csound, format, args);
  va_end(args);
}

PUBLIC void CppSound::ThrowMessageV(const char *format, va_list args)
{
  csoundThrowMessageV(csound, format, args);
}

PUBLIC void CppSound::SetMessageCallback(void (*csoundMessageCallback)(CSOUND *,int attr,
								       const char *format,
								       va_list valist))
{
  csoundSetMessageCallback(csound, csoundMessageCallback);
}

PUBLIC void CppSound::SetThrowMessageCallback(void (*throwMessageCallback)(CSOUND *,
									   const char *format,
									   va_list valist))
{
  csoundSetThrowMessageCallback(csound, throwMessageCallback);
}

PUBLIC int CppSound::GetMessageLevel()
{
  return csoundGetMessageLevel(csound);
}

PUBLIC void CppSound::SetMessageLevel(int messageLevel)
{
  csoundSetMessageLevel(csound, messageLevel);
}

PUBLIC void CppSound::InputMessage(const char *message)
{
  csoundInputMessage(csound, message);
}

PUBLIC void CppSound::KeyPress(char c)
{
  csoundKeyPress(csound, c);
}

PUBLIC void CppSound::SetInputValueCallback(void (*inputValueCalback)(CSOUND *,
								      const char *channelName,
								      MYFLT *value))
{
  csoundSetInputValueCallback(csound, inputValueCalback);
}

PUBLIC void CppSound::SetOutputValueCallback(void (*outputValueCalback)(CSOUND *,
									const char *channelName,
									MYFLT value))
{
  csoundSetOutputValueCallback(csound, outputValueCalback);
}

PUBLIC int CppSound::ScoreEvent(char type, const MYFLT *pFields, long numFields)
{
  csoundScoreEvent(csound, type, pFields, numFields);
}

PUBLIC void CppSound::SetExternalMidiInOpenCallback(int (*func)(CSOUND *, void **, const char *))
{
  csoundSetExternalMidiInOpenCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiReadCallback(int (*func)(CSOUND *, void *, unsigned char *, int))
{
  csoundSetExternalMidiReadCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiInCloseCallback(int (*func)(CSOUND *, void *))
{
  csoundSetExternalMidiInCloseCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiOutOpenCallback(int (*func)(CSOUND *, void **, const char *))
{
  csoundSetExternalMidiOutOpenCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiWriteCallback(int (*func)(CSOUND *, void *, const unsigned char *, int))
{
  csoundSetExternalMidiWriteCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiOutCloseCallback(int (*func)(CSOUND *, void *))
{
  csoundSetExternalMidiOutCloseCallback(csound, func);
}

PUBLIC void CppSound::SetExternalMidiErrorStringCallback(const char *(*func)(int))
{
  csoundSetExternalMidiErrorStringCallback(csound, func);
}

PUBLIC int CppSound::SetIsGraphable(int isGraphable)
{
  csoundSetIsGraphable(csound, isGraphable);
}

PUBLIC void CppSound::SetMakeGraphCallback(void (*makeGraphCallback)(CSOUND *,
								     WINDAT *windat,
								     const char *name))
{
  csoundSetMakeGraphCallback(csound, makeGraphCallback);
}

PUBLIC void CppSound::SetDrawGraphCallback(void (*drawGraphCallback)(CSOUND *,
								     WINDAT *windat))
{
  csoundSetDrawGraphCallback(csound, drawGraphCallback);
}

PUBLIC void CppSound::SetKillGraphCallback(void (*killGraphCallback)(CSOUND *,
								     WINDAT *windat))
{
  csoundSetKillGraphCallback(csound, killGraphCallback);
}

PUBLIC void CppSound::SetMakeXYinCallback(void (*makeXYinCallback)(CSOUND *, XYINDAT *,
								   MYFLT x, MYFLT y))
{
  csoundSetMakeXYinCallback(csound, makeXYinCallback);
}

PUBLIC void CppSound::SetReadXYinCallback(void (*readXYinCallback)(CSOUND *, XYINDAT *))
{
  csoundSetReadXYinCallback(csound, readXYinCallback);
}

PUBLIC void CppSound::SetKillXYinCallback(void (*killXYinCallback)(CSOUND *, XYINDAT *))
{
  csoundSetKillXYinCallback(csound, killXYinCallback);
}

PUBLIC void CppSound::SetExitGraphCallback(int (*exitGraphCallback)(CSOUND *))
{
  csoundSetExitGraphCallback(csound, exitGraphCallback);
}

PUBLIC int CppSound::NewOpcodeList(opcodeListEntry **opcodelist)
{
  return csoundNewOpcodeList(csound, opcodelist);
}

PUBLIC void CppSound::DisposeOpcodeList(opcodeListEntry *opcodelist)
{
  csoundDisposeOpcodeList(csound, opcodelist);
}

PUBLIC int CppSound::AppendOpcode(const char *opname,
				  int dsblksiz, int thread,
				  const char *outypes, const char *intypes,
				  int (*iopadr)(CSOUND *, void *),
				  int (*kopadr)(CSOUND *, void *),
				  int (*aopadr)(CSOUND *, void *))
{
  return csoundAppendOpcode(csound, opname, dsblksiz, thread, outypes, intypes, iopadr, kopadr, aopadr);
}

PUBLIC int CppSound::OpenLibrary(void **library, const char *libraryPath)
{
  return csoundOpenLibrary(library, libraryPath);
}

PUBLIC int CppSound::CloseLibrary(void *library)
{
  return csoundCloseLibrary(library);
}

PUBLIC void *CppSound::GetLibrarySymbol(void *library, const char *symbolName)
{
  return csoundGetLibrarySymbol(library, symbolName);
}

PUBLIC void CppSound::SetYieldCallback(int (*yieldCallback)(CSOUND *))
{
  return csoundSetYieldCallback(csound, yieldCallback);
}

PUBLIC void CppSound::SetPlayopenCallback(int (*playopen__)(CSOUND *,
							    const csRtAudioParams *parm))
{
  return csoundSetPlayopenCallback(csound, playopen__);
}

PUBLIC void CppSound::SetRtplayCallback(void (*rtplay__)(CSOUND *,
							 const MYFLT *outBuf, int nbytes))
{
  return csoundSetRtplayCallback(csound, rtplay__);
}

PUBLIC void CppSound::SetRecopenCallback(int (*recopen_)(CSOUND *,
							 const csRtAudioParams *parm))
{
  return csoundSetRecopenCallback(csound, recopen_);
}

PUBLIC void CppSound::SetRtrecordCallback(int (*rtrecord__)(CSOUND *,
							    MYFLT *inBuf, int nbytes))
{
  csoundSetRtrecordCallback(csound, rtrecord__);
}

PUBLIC void CppSound::SetRtcloseCallback(void (*rtclose__)(CSOUND *))
{
  csoundSetRtcloseCallback(csound, rtclose__);
}

PUBLIC int CppSound::GetDebug()
{
  return csoundGetDebug(csound);
}

PUBLIC void CppSound::SetDebug(int debug)
{
  return csoundSetDebug(csound, debug);
}

PUBLIC int CppSound::TableLength(int table)
{
  return csoundTableLength(csound, table);
}

PUBLIC MYFLT CppSound::TableGet(int table, int index)
{
  return csoundTableGet(csound, table, index);
}

PUBLIC void CppSound::TableSet(int table, int index, MYFLT value)
{
  csoundTableSet(csound, table, index, value);
}

PUBLIC MYFLT *CppSound::GetTable(int tableNum, int *tableLength)
{
  return csoundGetTable(csound, tableNum, tableLength);
}

PUBLIC void *CppSound::CreateThread(uintptr_t (*threadRoutine)(void *), void *userdata)
{
  return csoundCreateThread(threadRoutine, userdata);
}

PUBLIC uintptr_t CppSound::JoinThread(void *thread)
{
  return csoundJoinThread(thread);
}

PUBLIC void *CppSound::CreateThreadLock(void)
{
  return csoundCreateThreadLock();
}

PUBLIC int CppSound::WaitThreadLock(void *lock, size_t milliseconds)
{
  return csoundWaitThreadLock(lock, milliseconds);
}

PUBLIC void CppSound::WaitThreadLockNoTimeout(void *lock)
{
  csoundWaitThreadLockNoTimeout(lock);
}

PUBLIC void CppSound::NotifyThreadLock(void *lock)
{
  csoundNotifyThreadLock(lock);
}

PUBLIC void CppSound::DestroyThreadLock(void *lock)
{
  csoundDestroyThreadLock(lock);
}

PUBLIC void CppSound::Sleep(size_t milliseconds)
{
  csoundSleep(milliseconds);
}


PUBLIC void CppSound::InitTimerStruct(RTCLOCK *rtclock)
{
  csoundInitTimerStruct(rtclock);
}

PUBLIC double CppSound::GetRealTime(RTCLOCK *rtclock)
{
  return csoundGetRealTime(rtclock);
}

PUBLIC double CppSound::GetCPUTime(RTCLOCK *rtclock)
{
  return csoundGetCPUTime(rtclock);
}

PUBLIC uint32_t CppSound::GetRandomSeedFromTime(void)
{
  return csoundGetRandomSeedFromTime();
}

PUBLIC void CppSound::SetLanguage(cslanguage_t lang_code)
{
  return csoundSetLanguage(lang_code);
}

PUBLIC char *CppSound::LocalizeString(const char *s)
{
  return csoundLocalizeString(s);
}

PUBLIC int CppSound::CreateGlobalVariable(const char *name, size_t nbytes)
{
  return csoundCreateGlobalVariable(csound, name, nbytes);
}

PUBLIC void *CppSound::QueryGlobalVariable(const char *name)
{
  return csoundQueryGlobalVariable(csound, name);
}

PUBLIC void *CppSound::QueryGlobalVariableNoCheck(const char *name)
{
  return csoundQueryGlobalVariableNoCheck(csound, name);
}

PUBLIC int CppSound::DestroyGlobalVariable(const char *name)
{
  return csoundDestroyGlobalVariable(csound, name);
}

PUBLIC int CppSound::GetSizeOfMYFLT(void)
{
  return csoundGetSizeOfMYFLT();
}

PUBLIC void **CppSound::GetRtRecordUserData()
{
  return csoundGetRtRecordUserData(csound);
}

PUBLIC void **CppSound::GetRtPlayUserData()
{
  return csoundGetRtPlayUserData(csound);
}

PUBLIC int CppSound::RegisterSenseEventCallback(void (*func)(CSOUND *, void *), void *userData)
{
  return csoundRegisterSenseEventCallback(csound, func, userData);
}

PUBLIC int CppSound::RunUtility(const char *name, int argc, char **argv)
{
  return csoundRunUtility(csound, name, argc, argv);
}

PUBLIC char **CppSound::ListUtilities()
{
  return csoundListUtilities(csound);
}

PUBLIC const char *CppSound::GetUtilityDescription(const char *utilName)
{
  return csoundGetUtilityDescription(csound, utilName);
}

PUBLIC int CppSound::GetChannelPtr(MYFLT **p, const char *name, int type)
{
  return csoundGetChannelPtr(csound, p, name, type);
}

PUBLIC int CppSound::ListChannels(char ***names, int **types)
{
  return csoundListChannels(csound, names, types);
}

PUBLIC int CppSound::SetControlChannelParams(const char *name, int type, MYFLT dflt, MYFLT min, MYFLT max)
{
  return csoundSetControlChannelParams(csound, name, type, dflt, min, max);
}

PUBLIC int CppSound::GetControlChannelParams(const char *name, MYFLT *dflt, MYFLT *min, MYFLT *max)
{
  return csoundGetControlChannelParams(csound, name, dflt, min, max);
}

PUBLIC int CppSound::Rand31(int *seedVal)
{
  return csoundRand31(seedVal);
}

PUBLIC void CppSound::SeedRandMT(CsoundRandMTState *p, const uint32_t *initKey, uint32_t keyLength)
{
  csoundSeedRandMT(p, initKey, keyLength);
}

PUBLIC uint32_t CppSound::RandMT(CsoundRandMTState *p)
{
  return csoundRandMT(p);
}

// Functions that alias the Csound "C" API functions declared in H/cfgvar.h.

PUBLIC int CppSound::CreateGlobalConfigurationVariable(const char *name,
						       void *p, int type, int flags,
						       void *min, void *max,
						       const char *shortDesc,
						       const char *longDesc)
{
  return csoundCreateGlobalConfigurationVariable(name, p, type, flags, min, max, shortDesc, longDesc);
}

PUBLIC int CppSound::CreateConfigurationVariable(const char *name, void *p, int type, int flags,
						 void *min, void *max,
						 const char *shortDesc,
						 const char *longDesc)
{
  return csoundCreateConfigurationVariable(csound, name, p, type, flags, min, max, shortDesc, longDesc);
}

PUBLIC int CppSound::CopyGlobalConfigurationVariable(const char *name, void *p)
{
  return csoundCopyGlobalConfigurationVariable(csound, name, p);
}

PUBLIC int CppSound::CopyGlobalConfigurationVariables()
{
  return csoundCopyGlobalConfigurationVariables(csound);
}

PUBLIC int CppSound::SetGlobalConfigurationVariable(const char *name,
						    void *value)
{
  return csoundSetGlobalConfigurationVariable(name, value);
}

PUBLIC int CppSound::SetConfigurationVariable(const char *name,
					      void *value)
{
  return csoundSetConfigurationVariable(csound, name, value);
}

PUBLIC int CppSound::ParseGlobalConfigurationVariable(const char *name,
						      const char *value)
{
  return csoundParseGlobalConfigurationVariable(name, value);
}

PUBLIC int CppSound::ParseConfigurationVariable(const char *name,
						const char *value)
{
  return csoundParseConfigurationVariable(csound, name, value);
}

PUBLIC csCfgVariable_t *CppSound::QueryGlobalConfigurationVariable(const char *name)
{
  return csoundQueryGlobalConfigurationVariable(name);
}

PUBLIC csCfgVariable_t *CppSound::QueryConfigurationVariable(const char *name)
{
  return csoundQueryConfigurationVariable(csound, name);
}

PUBLIC csCfgVariable_t **CppSound::ListGlobalConfigurationVariables(void)
{
  return csoundListGlobalConfigurationVariables();
}

PUBLIC csCfgVariable_t **CppSound::ListConfigurationVariables()
{
  return csoundListConfigurationVariables(csound);
}

PUBLIC int CppSound::DeleteGlobalConfigurationVariable(const char *name)
{
  return csoundDeleteGlobalConfigurationVariable(name);
}

PUBLIC int CppSound::DeleteConfigurationVariable(const char *name)
{
  return csoundDeleteConfigurationVariable(csound, name);
}

PUBLIC int CppSound::DeleteAllGlobalConfigurationVariables(void)
{
  return csoundDeleteAllGlobalConfigurationVariables();
}

PUBLIC const char *CppSound::CfgErrorCodeToString(int errcode)
{
  return csoundCfgErrorCodeToString(errcode);
}

// Additional functions defined in this class.

PUBLIC CppSound::CppSound() 
{
  csound = csoundCreate(0);
}

PUBLIC CppSound::CppSound(void *hostData) 
{
  csound = csoundCreate(hostData);
}

PUBLIC CppSound::~CppSound()
{
  if (csound) {
    csoundDestroy(csound);
    csound = 0;
  }
}
