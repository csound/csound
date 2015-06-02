/*
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound.
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
#ifndef __CSOUNDVST_H
#define __CSOUNDVST_H

#include <Platform.hpp>
// Hack to compile all this GNU stuff on Windows.
#ifdef _MSC_VER
#include <windows.h>
#include <mmsystem.h>
#endif
#include "public.sdk/source/vst2.x/audioeffectx.h"
#include <float-version.h>
#include <CppSound.hpp>
#include <list>

class CsoundVstFltk;

const char *nameForOpcode(int opcode);

class Preset
{
public:
  std::string name;
  std::string text;
};

class CsoundVST :
  public CppSound,
  public AudioEffectX
{
public:
    enum {
      kNumInputs = 2
    };
    enum {
      kNumOutputs = 2
    };
    enum {
      kNumPrograms = 10
    };
  static double inputScale;
  static double outputScale;
  /**
   * The thread that calls Fl::wait().
   */
  static void *fltkWaitThreadId;
  bool isSynth;
  bool isMultiThreaded;
  size_t csoundFrameI;
  size_t csoundLastFrame;
  size_t channelI;
  size_t channelN;
  size_t hostFrameI;
  float vstSr;
  float vstCurrentSampleBlockStart;
  float vstCurrentSampleBlockEnd;
  float vstCurrentSamplePosition;
  float vstPriorSamplePosition;
    char *CSOUNDVST_PRINT_OPCODES;
  CsoundVstFltk *csoundVstFltk;
  std::list<VstMidiEvent> midiEventQueue;
  std::vector<Preset> bank;
  // AudioEffectX overrides.
  CsoundVST(audioMasterCallback audioMaster);
  virtual ~CsoundVST();
  virtual AEffEditor *getEditor();
  virtual bool getEffectName(char* name);
  virtual bool getVendorString(char* name);
  virtual bool getProductString(char* name);
  virtual VstInt32 canDo(char* text);
  virtual bool getInputProperties(VstInt32 index, VstPinProperties* properties);
  virtual bool getOutputProperties(VstInt32 index, VstPinProperties* properties);
  virtual bool keysRequired();
  virtual VstInt32 getProgram();
  virtual void setProgram(VstInt32 program);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual bool copyProgram(VstInt32 destination);
  virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);
  virtual VstInt32 getChunk(void** data, bool isPreset);
  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset);
  virtual void suspend();
  virtual void resume();
  virtual VstInt32 processEvents (VstEvents* events);
  virtual void process(float **inputs, float **outputs, VstInt32 sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
  // Shell overrides.
  virtual void open();
  // Peculiar to CsoundVST.
  CsoundVST();
  virtual bool getIsSynth() const;
  virtual void setIsSynth(bool isSynth);
  virtual bool getIsVst() const;
  virtual uintptr_t performanceThreadRoutine();
  virtual int performance();
  virtual std::string getText();
  virtual void setText(const std::string text);
  virtual void synchronizeScore();
  virtual void reset();
  virtual void openFile(std::string filename);
  virtual void openView(bool doRun = true);
  virtual void closeView();
  virtual bool getIsMultiThreaded() const;
  virtual void setIsMultiThreaded(bool isMultiThreaded);
  virtual void fltklock();
  virtual void fltkunlock();
  virtual void fltkflush();
  virtual void fltkwait();
  virtual int fltkrun();
    /**
     * Override to permit logging opcode dispatches for diagnostic purposes.
     */
    virtual VstIntPtr dispatcher (VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
  static int midiDeviceOpen(CSOUND *csound, void **userData,
                            const char *devName);
  static int midiDeviceClose(CSOUND *csound, void *userData);
  static int midiRead(CSOUND *csound, void *userData,
                      unsigned char *buf, int nbytes);
};

#if !defined(SWIGJAVA)

extern "C"
{
    SILENCE_PUBLIC AEffect* VSTPluginMain(audioMasterCallback audioMaster);
    SILENCE_PUBLIC CsoundVST* CreateCsoundVST();
    SILENCE_PUBLIC void RunCsoundVST(const char *filename);
}

#endif

#endif

