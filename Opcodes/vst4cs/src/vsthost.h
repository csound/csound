//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from the vst~ object, 
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

#ifndef _VSTPLUGIN_HOST
#define _VSTPLUGIN_HOST

#include "cs.h"
#include "AEffectx.h"
#include <vector>

typedef enum {

MAX_EVENTS = 64,
MAX_INOUTS = 8,

VSTINSTANCE_ERR_NO_VALID_FILE = -1,
VSTINSTANCE_ERR_NO_VST_PLUGIN = -2,
VSTINSTANCE_ERR_REJECTED = -3,
VSTINSTANCE_NO_ERROR = 0,

MIDI_NOTEON = 144,
MIDI_NOTEOFF = 128,
MIDI_POLYAFTERTOUCH = 160,
MIDI_CONTROLCHANGE = 176,
MIDI_PROGRAMCHANGE = 192,
MIDI_AFTERTOUCH = 208,
MIDI_PITCHBEND = 224,

} VST4CS_ENUM;

typedef AEffect* (*PVSTMAIN)(audioMasterCallback audioMaster);

class VSTPlugin
{
public:
    ENVIRON *csound;
	void *libraryHandle;
	AEffect *aeffect;
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
	size_t channels;
	char midiChannel;
	bool editor;
	
    VSTPlugin(ENVIRON *csound);
	virtual ~VSTPlugin();
	void StopEditing();
	int GetNumCategories();
	bool GetProgramName(int cat, int p , char* buf);
	void AddControlChange(int control , int value);
	void AddProgramChange(int value);
	void AddPitchBend(int value);
	void AddAftertouch(int value);
	bool ShowParams();
	void SetShowParameters(bool s);
	void OnEditorClose();
	void SetEditWindow(void *h);
	//CEditorThread* b;
	//RECT GetEditorRect();
	void EditorIdle();
	void edit(void);
	bool replace();
	void Free();
	int Instantiate(const char *libraryPathname);
	void Info();
	void Init();
	virtual int GetNumParams(void);
	virtual void GetParamName(int param, char* name);
	virtual float GetParamValue(int param);
	int getNumInputs(void);
	int getNumOutputs(void);
	virtual char* GetName(void);
	unsigned long GetVersion();
	char* GetVendorName(void);
	char* GetDllName(void);
	long NumParameters(void);
	float GetParameter(long parameter);
	bool DescribeValue(int parameter,char* psTxt);
	bool SetParameter(int parameter, float value); 
	bool SetParameter(int parameter, int value); 
	void SetCurrentProgram(int prg);
	int GetCurrentProgram();
	int NumPrograms();
	bool IsSynth();
	bool AddMIDI(int data0, int data1 = 0, int data2 = 0);
	void SendMidi();
	void processReplacing(float **inputs, float **outputs, long sampleframes);
	void process(float **inputs, float **outputs, long sampleframes);
	long Dispatch(long opCode, long index, long value, void *ptr, float opt);
	bool AddNoteOn(int channel, MYFLT note, MYFLT speed);
	bool AddNoteOff(int channel,  MYFLT note);
    static bool OnInputConnected(AEffect *effect, long input);
    static bool OnOutputConnected(AEffect *effect, long output);
	static long OnGetVersion(AEffect *effect);
	static bool OnCanDo(const char *ptr);
	static long Master(AEffect *effect, 
        long opcode, long index, long value, void *ptr, float opt);
};

inline long VSTPlugin::Dispatch(long opCode, 
    long index, long value, void *ptr, float opt) 
{
    if(aeffect)
        return aeffect->dispatcher(aeffect, opCode, index, value, ptr, opt);
}

inline void VSTPlugin::processReplacing(float **ins, float **outs, long frames)
{
    if(aeffect) {
        SendMidi();
	    aeffect->processReplacing(aeffect, ins, outs, frames);
    }
}

inline void VSTPlugin::process(float **ins, float **outs, long frames)
{
    if(aeffect) {
        SendMidi();
	    aeffect->process(aeffect, ins, outs, frames);
    }
}

#endif // _VSTPLUGIN_HOST
