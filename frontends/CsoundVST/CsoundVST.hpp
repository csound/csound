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
#ifndef __CSOUNDVST_H
#define __CSOUNDVST_H

// Hack to compile all this GNU stuff on Windows.
#ifdef _MSC_VER                    
#undef u_char
#undef u_short
#undef u_int
#undef u_long
#include <windows.h>
#include <mmsystem.h>
#endif

#include "audioeffectx.h"
#include "CppSound.hpp"
#include "Shell.hpp"
#include <list>

class CsoundVstFltk;

class Preset
{
public:
	std::string name;
	std::string text;
};

class CsoundVST : 
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
    const static MYFLT inputScale;
    const static MYFLT outputScale;
    CppSound cppSound_;
    CppSound *cppSound;
    bool isSynth;
    bool isVst;
    bool isPython;
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
    CsoundVstFltk *csoundVstFltk;
    static std::list<VstMidiEvent> midiEventQueue;
public:
    std::vector<Preset> bank;
    // AudioEffectX overrides.
	CsoundVST(audioMasterCallback audioMaster);
	virtual ~CsoundVST();
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
	// Peculiar to CsoundVST.
	CsoundVST();
	virtual CppSound *getCppSound();
	virtual bool getIsSynth() const;
	virtual void setIsSynth(bool isSynth);
	virtual bool getIsVst() const;
	virtual void setIsVst(bool isSynth);
	virtual bool getIsPython() const;
	virtual void setIsPython(bool isPython);
	virtual void performanceThreadRoutine();
	virtual int perform();
	virtual std::string getText();
	virtual void setText(const std::string text);
	virtual void synchronizeScore();
	virtual void reset();
	static int midiRead(void * csound, unsigned char *mbuf, int size);
};

extern "C"
{
    PUBLIC CsoundVST *CreateCsoundVST();
};

#endif
