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
#ifndef CSND_CPPSOUND_H
#define CSND_CPPSOUND_H
#ifdef SWIG
%module csnd
%include "std_string.i"
%include "std_vector.i"
%{
#include "CsoundFile.hpp"
#include <string>
#include <vector>
  %}
%template(MyfltVector) std::vector<MYFLT>;
#else
#include "CsoundFile.hpp"
#include <string>
#include <vector>
#include <csound.h>
#include <cfgvar.h>
#endif

/**
 * C++ interface to the Csound "C" API.
 * Most of these functions are straight wrappers.
 * The main purpose of this class is to simplify automatic interface generation
 * for other object-oriented languages such as Python, Java, Lua, and Scheme.
 */
class CppSound : public CsoundFile
{
protected:
  CSOUND *csound;
public:

  // Functions that alias the Csound "C" API declared in csound.h.

  PUBLIC static int Initialize(int *argc, char ***argv, int flags);
  PUBLIC CSOUND *Create(void *hostData);
  PUBLIC int PreCompile();
  PUBLIC int InitializeCscore(FILE* insco, FILE* outsco);
  PUBLIC int QueryInterface(const char *name, void **iface, int *version);
  PUBLIC void Destroy();
  PUBLIC static int GetVersion(void);
  PUBLIC static int GetAPIVersion(void);
  PUBLIC void *GetHostData();
  PUBLIC void SetHostData(void *hostData);
  PUBLIC const char *GetEnv(const char *name);
  PUBLIC static int SetDefaultEnv(const char *name, const char *value);
  PUBLIC int Perform(int argc, char **argv);
  PUBLIC int Compile(int argc, char **argv);
  PUBLIC int PerformKsmps();
  PUBLIC int PerformKsmpsAbsolute();
  PUBLIC int Cleanup();
  PUBLIC void Reset();
  PUBLIC MYFLT GetSr();
  PUBLIC MYFLT GetKr();
  PUBLIC int GetKsmps();
  PUBLIC int GetNchnls();
  PUBLIC MYFLT Get0dBFS();
  PUBLIC int GetStrVarMaxLen();
  PUBLIC int GetSampleFormat();
  PUBLIC int GetSampleSize();
  PUBLIC long GetInputBufferSize();
  PUBLIC long GetOutputBufferSize();
  PUBLIC MYFLT *GetInputBuffer();
  PUBLIC MYFLT *GetOutputBuffer();
  PUBLIC MYFLT *GetSpin();
  PUBLIC MYFLT *GetSpout();
  PUBLIC const char *GetOutputFileName();
  PUBLIC void SetHostImplementedAudioIO(int state, int bufSize);
  PUBLIC MYFLT GetScoreTime();
  PUBLIC int IsScorePending();
  PUBLIC void SetScorePending(int pending);
  PUBLIC MYFLT GetScoreOffsetSeconds();
  PUBLIC void SetScoreOffsetSeconds(MYFLT time);
  PUBLIC void RewindScore();
  PUBLIC void SetCscoreCallback(void (*cscoreCallback)());
  PUBLIC CS_PRINTF2 void Message(const char *format, ...);
  PUBLIC CS_PRINTF3 void MessageS(int attr, const char *format, ...);
  PUBLIC void MessageV(int attr, const char *format, va_list args);
  PUBLIC void ThrowMessage(const char *format, ...);
  PUBLIC void ThrowMessageV(const char *format, va_list args);
  PUBLIC void SetMessageCallback(void (*csoundMessageCallback)(CSOUND *,int attr,
							       const char *format,
							       va_list valist));
  PUBLIC void SetThrowMessageCallback(void (*throwMessageCallback)(CSOUND *,
								   const char *format,
								   va_list valist));
  PUBLIC int GetMessageLevel();
  PUBLIC void SetMessageLevel(int messageLevel);
  PUBLIC void InputMessage(const char *message);
  PUBLIC void KeyPress(char c);
  PUBLIC void SetInputValueCallback(void (*inputValueCalback)(CSOUND *,
							      const char *channelName,
							      MYFLT *value));
  PUBLIC void SetOutputValueCallback(void (*outputValueCalback)(CSOUND *,
								const char *channelName,
								MYFLT value));
  PUBLIC int ScoreEvent(char type, const MYFLT *pFields, long numFields);
  PUBLIC void SetExternalMidiInOpenCallback(int (*func)(CSOUND *, void **, const char *));
  PUBLIC void SetExternalMidiReadCallback(int (*func)(CSOUND *, void *, unsigned char *, int));
  PUBLIC void SetExternalMidiInCloseCallback(int (*func)(CSOUND *, void *));
  PUBLIC void SetExternalMidiOutOpenCallback(int (*func)(CSOUND *, void **, const char *));
  PUBLIC void SetExternalMidiWriteCallback(int (*func)(CSOUND *, void *, const unsigned char *, int));
  PUBLIC void SetExternalMidiOutCloseCallback(int (*func)(CSOUND *, void *));
  PUBLIC void SetExternalMidiErrorStringCallback(const char *(*func)(int));
  PUBLIC int SetIsGraphable(int isGraphable);
  PUBLIC void SetMakeGraphCallback(void (*makeGraphCallback)(CSOUND *,
							     WINDAT *windat,
							     const char *name));
  PUBLIC void SetDrawGraphCallback(void (*drawGraphCallback)(CSOUND *,
							     WINDAT *windat));
  PUBLIC void SetKillGraphCallback(void (*killGraphCallback)(CSOUND *,
							     WINDAT *windat));
  PUBLIC void SetMakeXYinCallback(void (*makeXYinCallback)(CSOUND *, XYINDAT *,
							   MYFLT x, MYFLT y));
  PUBLIC void SetReadXYinCallback(void (*readXYinCallback)(CSOUND *, XYINDAT *));
  PUBLIC void SetKillXYinCallback(void (*killXYinCallback)(CSOUND *, XYINDAT *));
  PUBLIC void SetExitGraphCallback(int (*exitGraphCallback)(CSOUND *));
  PUBLIC int NewOpcodeList(opcodeListEntry **opcodelist);
  PUBLIC void DisposeOpcodeList(opcodeListEntry *opcodelist);
  PUBLIC int AppendOpcode(const char *opname,
			  int dsblksiz, int thread,
			  const char *outypes, const char *intypes,
			  int (*iopadr)(CSOUND *, void *),
			  int (*kopadr)(CSOUND *, void *),
			  int (*aopadr)(CSOUND *, void *));
  PUBLIC static int OpenLibrary(void **library, const char *libraryPath);
  PUBLIC static int CloseLibrary(void *library);
  PUBLIC static void *GetLibrarySymbol(void *library, const char *symbolName);
  PUBLIC void SetYieldCallback(int (*yieldCallback)(CSOUND *));
  PUBLIC void SetPlayopenCallback(int (*playopen__)(CSOUND *,
						    const csRtAudioParams *parm));
  PUBLIC void SetRtplayCallback(void (*rtplay__)(CSOUND *,
						 const MYFLT *outBuf, int nbytes));
  PUBLIC void SetRecopenCallback(int (*recopen_)(CSOUND *,
						 const csRtAudioParams *parm));
  PUBLIC void SetRtrecordCallback(int (*rtrecord__)(CSOUND *,
						    MYFLT *inBuf, int nbytes));
  PUBLIC void SetRtcloseCallback(void (*rtclose__)(CSOUND *));
  PUBLIC int GetDebug();
  PUBLIC void SetDebug(int debug);
  PUBLIC int TableLength(int table);
  PUBLIC MYFLT TableGet(int table, int index);
  PUBLIC void TableSet(int table, int index, MYFLT value);
  PUBLIC MYFLT *GetTable(int tableNum, int *tableLength);
  PUBLIC static void *CreateThread(uintptr_t (*threadRoutine)(void *), void *userdata);
  PUBLIC static uintptr_t JoinThread(void *thread);
  PUBLIC static void *CreateThreadLock(void);
  PUBLIC static int WaitThreadLock(void *lock, size_t milliseconds);
  PUBLIC static void WaitThreadLockNoTimeout(void *lock);
  PUBLIC static void NotifyThreadLock(void *lock);
  PUBLIC static void DestroyThreadLock(void *lock);
  PUBLIC static void Sleep(size_t milliseconds);
  PUBLIC static void InitTimerStruct(RTCLOCK *);
  PUBLIC static double GetRealTime(RTCLOCK *);
  PUBLIC static double GetCPUTime(RTCLOCK *);
  PUBLIC static uint32_t GetRandomSeedFromTime(void);
  PUBLIC void SetLanguage(cslanguage_t lang_code);
  PUBLIC char *LocalizeString(const char *s);
  PUBLIC static int CreateGlobalVariable(const char *name, size_t nbytes);
  PUBLIC static void *QueryGlobalVariable(const char *name);
  PUBLIC static void *QueryGlobalVariableNoCheck(const char *name);
  PUBLIC static int DestroyGlobalVariable(const char *name);
  PUBLIC static int GetSizeOfMYFLT(void);
  PUBLIC void **GetRtRecordUserData();
  PUBLIC void **GetRtPlayUserData();
  PUBLIC int RegisterSenseEventCallback(void (*func)(CSOUND *, void *), void *userData);
  PUBLIC int RunUtility(const char *name, int argc, char **argv);
  PUBLIC char **ListUtilities();
  PUBLIC const char *GetUtilityDescription(const char *utilName);
  PUBLIC int GetChannelPtr(MYFLT **p, const char *name, int type);
  PUBLIC int ListChannels(char ***names, int **types);
  PUBLIC int SetControlChannelParams(const char *name, int type, MYFLT dflt, MYFLT min, MYFLT max);
  PUBLIC int GetControlChannelParams(const char *name, MYFLT *dflt, MYFLT *min, MYFLT *max);
  PUBLIC static int Rand31(int *seedVal);
  PUBLIC static void SeedRandMT(CsoundRandMTState *p, const uint32_t *initKey, uint32_t keyLength);
  PUBLIC static uint32_t RandMT(CsoundRandMTState *p);

  // Functions that alias the Csound "C" API functions declared in H/cfgvar.h.

  PUBLIC static int CreateGlobalConfigurationVariable(const char *name,
						      void *p, int type, int flags,
						      void *min, void *max,
						      const char *shortDesc,
						      const char *longDesc);
  PUBLIC int CreateConfigurationVariable(int flags,
					 void *min, void *max,
					 const char *shortDesc,
					 const char *longDesc);
  PUBLIC static int CopyGlobalConfigurationVariable(const char *name, void *p);
  PUBLIC static int CopyGlobalConfigurationVariables();
  PUBLIC static int SetGlobalConfigurationVariable(const char *name,
						   void *value);
  PUBLIC int SetConfigurationVariable(const char *name,
				      void *value);
  PUBLIC static int ParseGlobalConfigurationVariable(const char *name,
						     const char *value);
  PUBLIC int ParseConfigurationVariable(const char *name,
					const char *value);
  PUBLIC static csCfgVariable_t *QueryGlobalConfigurationVariable(const char *name);
  PUBLIC csCfgVariable_t *QueryConfigurationVariable(const char *name);
  PUBLIC static csCfgVariable_t **ListGlobalConfigurationVariables(void);
  PUBLIC csCfgVariable_t **ListConfigurationVariables();
  PUBLIC static int DeleteGlobalConfigurationVariable(const char *name);
  PUBLIC int DeleteConfigurationVariable(const char *name);
  PUBLIC static int DeleteAllGlobalConfigurationVariables(void);
  PUBLIC static const char *CfgErrorCodeToString(int errcode);

  // Additional functions defined in this class.

  PUBLIC CppSound();
  PUBLIC CppSound(void *hostData);
  PUBLIC virtual ~CppSound();

};

#endif

