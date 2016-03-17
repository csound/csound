//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his Vst Host program
//  and from the vst~ object by Thomas Grill,
//  which in turn borrows from the Psycle tracker.
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA

#ifndef VSTPLUGIN_HOST_H
#define VSTPLUGIN_HOST_H

#include "csdl.h"

#if !defined(SWIG) && defined(CS_VSTHOST)
#include <audioeffectx.h>
#endif

#include <vector>
#include <map>
#include <string>

/**
 * THE REAL VST INTERFACE IS JUST THIS, USE THE EXACT SAME TYPES:
 *
 * typedef      VstIntPtr (VSTCALLBACK *audioMasterCallback) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
 * typedef VstIntPtr (VSTCALLBACK *AEffectDispatcherProc) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
 * typedef void (VSTCALLBACK *AEffectProcessProc) (AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);
 * typedef void (VSTCALLBACK *AEffectProcessDoubleProc) (AEffect* effect, double** inputs, double** outputs, VstInt32 sampleFrames);
 * typedef void (VSTCALLBACK *AEffectSetParameterProc) (AEffect* effect, VstInt32 index, float parameter);
 * typedef float (VSTCALLBACK *AEffectGetParameterProc) (AEffect* effect, VstInt32 index);
 */

typedef enum {
  MAX_EVENTS = 64,
  MAX_INOUTS = 8,
  VSTINSTANCE_ERR_NO_VALID_FILE = -1,
  VSTINSTANCE_ERR_NO_VST_PLUGIN = -2,
  VSTINSTANCE_ERR_REJECTED = -3,
  VSTINSTANCE_NO_ERROR = 0
} VST4CS_ENUM;

typedef AEffect * (*PVSTMAIN)(audioMasterCallback audioMaster);

class Fl_Window;

class VSTPlugin {
 public:
  CSOUND *csound;
  void *libraryHandle;
  AEffect *aeffect;
  bool hasEditor;
  AEffEditor *editor;
  ERect rect;
  Fl_Window *window;
  void *windowHandle;
  char productName[64];
  char vendorName[64];
  char libraryName[0x100];
  unsigned long pluginVersion;
  bool pluginIsSynth;
  std::vector<float *> inputs;
  std::vector<float *> outputs;
  std::vector< std::vector<float> > inputs_;
  std::vector< std::vector<float> > outputs_;
  std::vector<VstMidiEvent> vstMidiEvents;
  std::vector<char> vstEventsBuffer;
  bool overwrite;
  bool edited;
  bool showParameters;
  VstTimeInfo vstTimeInfo;
  size_t framesPerSecond;
  size_t framesPerBlock;
  int targetFLpanel; //GAB

  VSTPlugin(CSOUND *csound);
  virtual ~VSTPlugin();
  virtual void StopEditing();
  virtual int GetNumCategories();
  virtual bool GetProgramName(int cat, int p, char *buf);
  virtual bool ShowParams();
  virtual void SetShowParameters(bool s);
  virtual void OnEditorClose();
  virtual void SetEditWindow(void *h);
  virtual ERect GetEditorRect();
  virtual void EditorIdle();
  virtual bool replace();
  virtual void Free();
  virtual int Instantiate(const char *libraryPathname);
  virtual void Info();
  virtual void Init();
  virtual int GetNumParams(void);
  virtual void GetParamName(int param, char *name);
  virtual float GetParamValue(int param);
  virtual int getNumInputs(void);
  virtual int getNumOutputs(void);
  virtual char* GetName(void);
  virtual unsigned long GetVersion();
  virtual char* GetVendorName(void);
  virtual char* GetDllName(void);
  virtual long NumParameters(void);
  virtual float GetParameter(long parameter);
  virtual bool DescribeValue(int parameter,char* psTxt);
  virtual bool SetParameter(int parameter, float value);
  virtual bool SetParameter(int parameter, int value);
  virtual void SetCurrentProgram(int prg);
  virtual int GetCurrentProgram();
  virtual int NumPrograms();
  virtual bool IsSynth();
  virtual bool AddMIDI(int data, int deltaFrames, int detune);
  virtual void SendMidi();
  virtual void processReplacing(float **inputs,
                                float **outputs,
                                long sampleframes);
  virtual void process(float **inputs,
                       float **outputs,
                       long sampleframes);
  // typedef VstIntPtr (VSTCALLBACK *AEffectDispatcherProc) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

  virtual VstIntPtr Dispatch(VstInt32 opCode,
                        VstInt32 index=0,
                        VstIntPtr value=0,
                        void *ptr=0,
                        float opt=0);
  virtual void Log(const char *format, ...);
  virtual void Debug(const char *format, ...);
  virtual void OpenEditor();
  virtual void CloseEditor();
  virtual VstTimeInfo *GetTime();
  static bool OnInputConnected(AEffect *effect, long input);
  static bool OnOutputConnected(AEffect *effect, long output);
  static long OnGetVersion(AEffect *effect);
  static bool OnCanDo(const char *ptr);

  //  typedef   VstIntPtr (VSTCALLBACK *audioMasterCallback) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

  static VstIntPtr Master(AEffect *effect,
                     VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr,
                     float opt);
  static void initializeOpcodes();
  long EffGetChunk(void **ptr, bool isPreset = false) // GAB
  {
    return Dispatch(effGetChunk, isPreset, 0, ptr);
  }
  long EffGetProgram()
  {
    return Dispatch(effGetProgram);
  }
  void EffSetProgram(long lValue)
  {
    Dispatch(effSetProgram, 0, lValue);
  }
  void EffGetProgramName(char *ptr)
  {
    Dispatch(effGetProgramName, 0, 0, ptr);
  }
  static std::map<long, std::string> &masterOpcodes()
  {
    static std::map<long, std::string> masterOpcodes_;
    return masterOpcodes_;
  }
  static std::map<long, std::string> &dispatchOpcodes()
  {
    static std::map<long, std::string> dispatchOpcodes_;
    return dispatchOpcodes_;
  }
  static size_t &opcodeRefCount()
  {
    static size_t opcodeRefCount_ = 0;
    return opcodeRefCount_;
  }
};

inline VstIntPtr VSTPlugin::Dispatch(VstInt32 opCode,
                        VstInt32 index,
                        VstIntPtr value,
                        void *ptr,
                        float opt)
{
  if (aeffect)
    return aeffect->dispatcher(aeffect, opCode, index, value, ptr, opt);
  else
    return 0;
}

inline void VSTPlugin::processReplacing(float **ins, float **outs, long frames)
{
  if (aeffect) {
    SendMidi();
    aeffect->processReplacing(aeffect, ins, outs, frames);
  }
}

inline void VSTPlugin::process(float **ins, float **outs, long frames)
{
  if (aeffect) {
    SendMidi();
    aeffect->processReplacing(aeffect, ins, outs, frames);
  }
}

#endif
