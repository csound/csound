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
#include <Python.h>
#include "CsoundVST.hpp"
#include "CsoundVstFltk.hpp"
#include "System.hpp"


const MYFLT CsoundVST::inputScale = (float) 32767.0;
const MYFLT CsoundVST::outputScale = (float) (1.0 / 32767.0);
std::list<VstMidiEvent> CsoundVST::midiEventQueue;

CsoundVST::CsoundVST(audioMasterCallback audioMaster) : 
AudioEffectX(audioMaster, kNumPrograms, 0),
csoundVstFltk(0),
isSynth(true),
cppSound(&cppSound_),
isVst(true),
isPython(false)
{
	setNumInputs(kNumInputs);		// stereo in
	setNumOutputs(kNumOutputs);		// stereo out
	setUniqueID('cVsT');	// identify
	canMono();				// makes sense to feed both inputs with the same signal
	canProcessReplacing();	// supports both accumulating and replacing output
	open();
	csoundVstFltk = new CsoundVstFltk(this);
	setEditor(csoundVstFltk);
	int number = 0;
	csoundVstFltk->preferences.get("IsSynth", number, 0);
	if(number)
	{
		AudioEffectX::isSynth(true);
	}
	else
	{
		AudioEffectX::isSynth(false);
	}
	wantEvents(true);
	programsAreChunks(true);
	curProgram = 0;
	bank.resize(kNumPrograms);
	for(size_t i = 0; i < bank.size(); i++)
	{
		char buffer[0x24];
		sprintf(buffer, "Program%d", i + 1);
		bank[i].name = buffer;
	}
}

CsoundVST::CsoundVST() : 
AudioEffectX(0, kNumPrograms, 0),
isVst(false),
isSynth(true),
isPython(false),
csoundVstFltk(0),
cppSound(&cppSound_)
{
	setNumInputs(2);		// stereo in
	setNumOutputs(2);		// stereo out
	setUniqueID('cVsT');	// identify
	canMono();				// makes sense to feed both inputs with the same signal
	canProcessReplacing();	// supports both accumulating and replacing output
	open();
	csoundVstFltk = new CsoundVstFltk(this);
	setEditor(csoundVstFltk);
	bank.resize(kNumPrograms);
	curProgram = 0;
	for(size_t i = 0; i < bank.size(); i++)
	{
		char buffer[0x24];
		sprintf(buffer, "Program%d", i + 1);
		bank[i].name = buffer;
	}
}

CsoundVST::~CsoundVST()
{
	Shell::close();
}

bool CsoundVST::getIsSynth() const
{
	return isSynth;
}

void CsoundVST::setIsSynth(bool isSynth)
{
	this->isSynth = isSynth;
}

bool CsoundVST::getIsVst() const
{
	return isVst;
}

void CsoundVST::setIsVst(bool isVst)
{
	this->isVst = isVst;
}

bool CsoundVST::getIsPython() const
{
	return isPython;
}

void CsoundVST::setIsPython(bool isPython)
{
	this->isPython = isPython;
}

//-----------------------------------------------------------------------------------------
void CsoundVST::setProgramName(char *name)
{
	bank[curProgram].name = name;
}

//-----------------------------------------------------------------------------------------
void CsoundVST::getProgramName(char *name)
{
	strcpy(name, bank[curProgram].name.c_str());
}


AEffEditor* CsoundVST::getEditor()
{
	return editor;
}

void CsoundVST::performanceThreadRoutine()
{
	getCppSound()->stop();
	getCppSound()->setExternalMidiEnabled(false);
	getCppSound()->setExternalMidiReadCallback(0);
	if(getIsPython())
	{
		Shell::save(Shell::getFilename());
		csound::System::inform("Saved as: '%s'.\n", Shell::getFilename().c_str());
		reset();
		if(getIsVst())
		{
			getCppSound()->setExternalMidiEnabled(true);
			getCppSound()->setExternalMidiReadCallback(&CsoundVST::midiRead);
		}
		Shell::run();
	}
	else
	{
		cppSound->exportForPerformance();
		csound::System::inform("Saved as: '%s' and '%s'.\n", cppSound->getOrcFilename().c_str(), cppSound->getScoFilename().c_str());
		reset();
		if(getIsVst())
		{
			getCppSound()->setExternalMidiEnabled(true);
			getCppSound()->setExternalMidiReadCallback(&CsoundVST::midiRead);
			if(getCppSound()->compile())
			{
				reset();
			}
		}
		else
		{
			cppSound->perform();
		}     
	}
}

void performanceThreadRoutine_(void *data)
{
	((CsoundVST *)data)->performanceThreadRoutine();
}

static int threadYieldCallback(void *)
{
    Fl::lock();
    Fl::wait(0.0);
    Fl::unlock();
    return 1;
}

static int nonThreadYieldCallback(void *)
{
    Fl::unlock();
    return 1;
}

int CsoundVST::perform()
{
	int result = 0;
	if(getIsVst())
	{
	    csoundSetYieldCallback(getCppSound()->getCsound(), nonThreadYieldCallback); 
		performanceThreadRoutine();
	}
	else
	{
	    csoundSetYieldCallback(getCppSound()->getCsound(), threadYieldCallback); 
		result = (int) csound::System::createThread(performanceThreadRoutine_, this, 0);
	}
	return result;
}

CppSound *CsoundVST::getCppSound()
{
	return cppSound;
}

void CsoundVST::open()
{
	int result = 0;
	Shell::open();
	char *argv[] = {"",""};
	PySys_SetArgv(1, argv);
	PyObject *mainModule = PyImport_ImportModule("__main__");
	result = Shell::run("import sys\n");
	if(result)
	{
		PyErr_Print();
	}
	result = Shell::run("import CsoundVST\n");
	if(result)
	{
		PyErr_Print();
	}
	result = Shell::run("csound = CsoundVST.CppSound()\n");
	if(result)
	{
		PyErr_Print();
	}
	result = Shell::run("sys.stdout = sys.stderr = csound\n");
	if(result)
	{
		PyErr_Print();
	}
	PyObject *pyCsound = PyObject_GetAttrString(mainModule, "csound");
	// No doubt SWIG or the Python API could do this directly,
	// but damned if I could figure out how, and this works.
	PyObject* pyCppSound = PyObject_CallMethod(pyCsound, "getThis", "");
	cppSound = (CppSound *) PyLong_AsLong(pyCppSound);
	std::string filename_ = getFilename();
	if(filename_.length() > 0)
	{
		cppSound->setFilename(filename_);
	}
}

void CsoundVST::reset()
{
	csoundFrameI = 0;
	csoundLastFrame = 0;
	channelI = 0;
	channelN = 0;
	hostFrameI = 0;
	vstSr = 0;
	vstCurrentSampleBlockStart = 0;
	vstCurrentSampleBlockEnd = 0;
	vstCurrentSamplePosition = 0;
	vstPriorSamplePosition = 0;
	midiEventQueue.clear();
}

void CsoundVST::setProgram(long program)
{
	csound::System::message("RECEIVED CsoundVST::setProgram(%d)...\n", program);
	if(program < kNumPrograms)
	{
		curProgram = program;
		setText(bank[curProgram].text);
	}
}

void CsoundVST::suspend()
{
	csound::System::inform("RECEIVED CsoundVST::suspend()...\n");
	stop();
}		

void CsoundVST::resume()
{
	csound::System::inform("RECEIVED CsoundVST::resume()...\n");
	perform();
	wantEvents(true);
}		

long CsoundVST::processEvents(VstEvents *vstEvents)
{
	if(getCppSound()->getIsGo())
	{
		for(int i = 0; i < vstEvents->numEvents; i++)
		{
			if(vstEvents->events[i]->type == kVstMidiType)
			{
				VstMidiEvent *vstMidiEvent = (VstMidiEvent *)vstEvents->events[i];
				midiEventQueue.push_back(*vstMidiEvent);
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

int CsoundVST::midiRead(void * csound, unsigned char *mbuf, int size)
{
	if(CsoundVST::midiEventQueue.empty())
	{
		return 0;
	}
	else
	{
		MIDIMESSAGE *midiMessage = (MIDIMESSAGE *)mbuf;
		const VstMidiEvent &event = CsoundVST::midiEventQueue.front();
		midiMessage->bData[0] = event.midiData[0];
		midiMessage->bData[1] = event.midiData[1];
		midiMessage->bData[2] = event.midiData[2];
		csound::System::message("CsoundVST::midiRead(%x, %x, %x)\n", 
			(int) midiMessage->bData[0],
			(int) midiMessage->bData[1],
			(int) midiMessage->bData[2]);
		CsoundVST::midiEventQueue.pop_front();
		return 3;
	}
}

void CsoundVST::process(float **hostInput, float **hostOutput, long hostFrameN)
{
	if(getCppSound()->getIsGo())
	{
		synchronizeScore();
		MYFLT *csoundInput = cppSound->getSpin();
		MYFLT *csoundOutput = cppSound->getSpout();
		size_t csoundLastFrame = cppSound->getKsmps() - 1;
		size_t channelN = cppSound->getNchnls();
		size_t channelI;
		for(long hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++)
		{
			for(channelI = 0; channelI < channelN; channelI++)
			{
				csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
			}
			if(csoundFrameI == 0)
			{
				cppSound->performKsmps();
			}
			for(channelI = 0; channelI < channelN; channelI++)
			{
				hostOutput[channelI][hostFrameI] += csoundOutput[(csoundFrameI * channelN) + channelI] * outputScale;
				csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
			}
			csoundFrameI++;
			if(csoundFrameI > csoundLastFrame)
			{
				csoundFrameI = 0;
			}		  
		}
	}
}

void CsoundVST::processReplacing(float **hostInput, float **hostOutput, long hostFrameN)
{
	if(getCppSound()->getIsGo())
	{
		synchronizeScore();
		MYFLT *csoundInput = cppSound->getSpin();
		MYFLT *csoundOutput = cppSound->getSpout();
		size_t csoundLastFrame = cppSound->getKsmps() - 1;
		size_t channelN = cppSound->getNchnls();
		size_t channelI;
		for(long hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++)
		{
			for(channelI = 0; channelI < channelN; channelI++)
			{
				csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
			}
			if(csoundFrameI == 0)
			{
				cppSound->performKsmps();
			}
			for(channelI = 0; channelI < channelN; channelI++)
			{
				hostOutput[channelI][hostFrameI] = csoundOutput[(csoundFrameI * channelN) + channelI] * outputScale;
				csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
			}
			csoundFrameI++;
			if(csoundFrameI > csoundLastFrame)
			{
				csoundFrameI = 0;
			}		  
		}
	}
}

void CsoundVST::synchronizeScore()
{
	vstPriorSamplePosition = vstCurrentSamplePosition;
	VstTimeInfo *vstTimeInfo = getTimeInfo(0);
	vstSr = double(vstTimeInfo->sampleRate);
	vstCurrentSamplePosition = (int) vstTimeInfo->samplePos;
	vstCurrentSampleBlockStart = vstTimeInfo->samplePos / vstSr;
	if((vstCurrentSamplePosition && !vstPriorSamplePosition) || 
		(vstCurrentSamplePosition < vstPriorSamplePosition))
	{
		if(getCppSound()->getIsGo())
		{
			getCppSound()->setScorePending(1);
			getCppSound()->rewindScore();
			getCppSound()->setScoreOffsetSeconds(vstCurrentSampleBlockStart);
			csound::System::inform("Score synchronized at %f...\n", vstCurrentSampleBlockStart);
		}
	}
}

bool CsoundVST::getInputProperties(long index, VstPinProperties* properties)
{
	if(index < kNumInputs)
	{
		sprintf(properties->label, "My %1d In", index + 1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

bool CsoundVST::getOutputProperties(long index, VstPinProperties* properties)
{
	if(index < kNumOutputs)
	{
		sprintf(properties->label, "My %1d Out", index + 1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

bool CsoundVST::getProgramNameIndexed(long category, long index, char* text)
{
	if(index < kNumPrograms)
	{
		strcpy(text, bank[curProgram].name.c_str());
		return true;
	}
	return false;
}

bool CsoundVST::getEffectName(char* name)
{
	strcpy(name, "CsoundVST");
	return true;
}

bool CsoundVST::getVendorString(char* text)
{
	strcpy(text, "Irreducible Productions");
	return true;
}

bool CsoundVST::getProductString(char* text)
{
	strcpy(text, "CsoundVST");
	return true;
}

long CsoundVST::canDo(char* text)
{
	csound::System::inform("RECEIVED CsoundVST::canDo('%s')...\n", text);
	if(strcmp(text, "receiveVstTimeInfo") == 0)
	{
		return 1;
	}
	if(strcmp(text, "receiveVstEvents") == 0)
	{
		return 1;
	}
	if(strcmp(text, "receiveVstMidiEvents") == 0)
	{
		return 1;
	}
	if(strcmp(text, "sendVstMidiEvents") == 0)
	{
		return 1;
	}
	if(strcmp(text, "plugAsChannelInsert") == 0)
	{
		return 1;
	}
	if(strcmp(text, "plugAsSend") == 0)
	{
		return 1;
	}
	if(strcmp(text, "sizeWindow") == 0)
	{
		return 1;
	}
	if(strcmp(text, "asyncProcessing") == 0)
	{
		return 1;
	}
	if(strcmp(text, "2in2out") == 0)
	{
		return 1;
	}
	return 0;
}

bool CsoundVST::keysRequired()
{
	csound::System::message("RECEIVED CsoundVST::keysRequired...\n");
	return 1;
}		

long CsoundVST::getProgram() 
{
	//csound::System::message("RECEIVED CsoundVST::getProgram...\n");
	//bank[curProgram].text = getText();
	return curProgram;
}

bool CsoundVST::copyProgram(long destination)
{
	csound::System::message("RECEIVED CsoundVST::copyProgram(%d)...\n", destination);
	if(destination < kNumPrograms)
	{
		bank[destination] = bank[curProgram];
		return true;
	}
	return false;
}

long CsoundVST::getChunk(void** data, bool isPreset)
{
	csound::System::message("BEGAN CsoundVST::getChunk(%d)...\n", (int) isPreset);
	((CsoundVstFltk *)getEditor())->updateModel();
	long returnValue = 0;
	static std::string bankBuffer;
	bank[curProgram].text = getText();
	if(isPreset)
	{
		*data = (void *)bank[curProgram].text.c_str();
		returnValue = (long) strlen((char *)*data) + 1;
	}
	else
	{
		std::ostringstream stream;
		int n = bank.size();
		stream << n << "\n";
		for(std::vector<Preset>::iterator it = bank.begin(); it != bank.end(); ++it)
		{
			Preset &preset = (*it);
			stream << preset.name.c_str() << "\n";
			stream << preset.text.size() << "\n";
			for(std::string::iterator jt = preset.text.begin(); jt != preset.text.end(); ++jt)
			{
				stream.put(*jt);	
			}
			stream << "\n";
		}
		bankBuffer = stream.str();
		*data = (void *)bankBuffer.c_str();
		returnValue = bankBuffer.size();
	}
	csound::System::message("ENDED CsoundVST::getChunk, returned %d...\n", returnValue);
	return returnValue;
}

long CsoundVST::setChunk(void* data, long byteSize, bool isPreset)
{
	csound::System::message("RECEIVED CsoundVST::setChunk(%d, %d)...\n", byteSize, (int) isPreset);
	long returnValue = 0;
	if(isPreset)
	{
		bank[curProgram].text = strdup((const char *)data);
		setText(bank[curProgram].text);
		returnValue = byteSize;
	}
	else
	{
#if defined(__GNUC__)
		std::string inputBuffer = (const char *)data;
		std::istringstream stream(inputBuffer);
#else
		std::istringstream stream(strdup((const char *)data), byteSize);
#endif
		std::string buffer;
		stream >> buffer;
		stream >> std::ws;
		int n = atoi(buffer.c_str());
		bank.resize(n);
		for(int i = 0; i < n; i++)
		{
			Preset preset;
			stream >> preset.name;
			stream >> std::ws;
			stream >> buffer;
			stream >> std::ws;
			int length = atoi(buffer.c_str());
			preset.text.resize(length);
			char c;
			for(int j = 0; j < length; j++)
			{
				stream.get(c);
				preset.text[j] = c;
			}
			bank[i] = preset;
		}
		returnValue = byteSize;
	}
	setProgram(curProgram);
	editor->update();
	return returnValue;
}

std::string CsoundVST::getText()
{
	csound::System::message("BEGAN CsoundVST::getText...");
	std::string buffer;
	if(getIsPython())
	{
		buffer = getScript();
	}
	else
	{
		buffer = getCppSound()->getCSD();
	}
	csound::System::message("ENDED CsoundVST::getText.");
	return buffer;
}

void CsoundVST::setText(const std::string text)
{
	if(getIsPython())
	{
		setScript(text);
	}
	else
	{
		getCppSound()->setCSD(text);
	}
}

extern "C"
{
#if __GNUC__ && (WIN32||BEOS)
extern "C" __declspec(dllexport) CsoundVST *CreateCsoundVST();
#else
	CsoundVST *CreateCsoundVST();
#endif
	PUBLIC CsoundVST *CreateCsoundVST()
	{
		return new CsoundVST;
	}
};


