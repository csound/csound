/**
 * S C O R E   G E N E R A T O R   V S T 
 *
 * A VST plugin for writing score generators in Python.
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
#ifndef __SCOREGENERATORVST_H
#define __SCOREGENERATORVST_H

#ifdef SWIG

%module ScoreGeneratorVst
%{
#include "MusicModel.hpp"
#include "Shell.hpp"
#include <list>
%}

#else
// Hack to compile all this GNU stuff on Windows.
#ifdef _MSC_VER
#include <windows.h>
#include <mmsystem.h>
#endif

#include "audioeffectx.h"
#include "MusicModel.hpp"
#include "Shell.hpp"
#include <list>

#endif

class ScoreGeneratorVstFltk;

class Preset
{
public:
  std::string name;
  std::string text;
};

class ScoreGeneratorVst :
  public AudioEffectX,
  public csound::Shell
{
protected:
  enum
    {
      kNumInputs = 2,
    };
  enum
    {
      kNumOutputs = 2,
    };
  enum
    {
      kNumPrograms = 10,
    };
  VstEvents vstEvents;
  std::vector<VstMidiEvent> vstMidiEvents;
  std::vector<VstMidiEvent>::iterator vstMidiEventIterator;
  float vstSr;
  float vstCurrentSampleBlockStart;
  float vstCurrentSampleBlockEnd;
  float vstCurrentSamplePosition;
  float vstPriorSamplePosition;
  char alive;
  ScoreGeneratorVstFltk *ScoreGeneratorVstFltk;
public:
  std::vector<Preset> bank;
  // AudioEffectX overrides.
  ScoreGeneratorVst(audioMasterCallback audioMaster);
  virtual ~ScoreGeneratorVst();
  virtual AEffEditor *getEditor();
  virtual bool getEffectName(char* name);
  virtual bool getVendorString(char* name);
  virtual bool getProductString(char* name);
  virtual long canDo(char* text);
  virtual bool getInputProperties(long index, VstPinProperties* properties);
  virtual bool getOutputProperties(long index, VstPinProperties* properties);
  virtual bool keysRequired();
  virtual long getProgram();
  virtual void setProgram(long program);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual bool copyProgram(long destination);
  virtual bool getProgramNameIndexed(long category, long index, char* text);
  virtual long getChunk(void** data, bool isPreset);
  virtual long setChunk(void* data, long byteSize, bool isPreset);
  virtual void suspend();
  virtual void resume();
  virtual long processEvents(VstEvents *vstEvents);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

  // Shell overrides.
  virtual void open();
  // Peculiar to ScoreGeneratorVst.
  ScoreGeneratorVst();
  virtual std::string getText();
  virtual void setText(const std::string text);
  virtual void synchronizeScore();
  virtual void reset();
  virtual void openFile(std::string filename);
  virtual void openView(bool doRun = true);
  virtual void closeView();
  virtual int generate();
  virtual void clearEvents();
  virtual void addEvent(double start, double duration, double status, double channel, double data1, double data2);
};

#if !defined(SWIGJAVA)

extern "C"
{
  PUBLIC ScoreGeneratorVst *CreateScoreGeneratorVst();
};

#endif

#endif

