//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 1.0
// © 2003 Steinberg Media Technologies, All Rights Reserved
//
// you should not have to edit this file
// use override methods instead
//-------------------------------------------------------------------------------------------------------

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef __AudioEffect__
#include "AudioEffect.hpp"
#endif

#ifndef __AEffEditor__
#include "AEffEditor.hpp"
#endif

#ifdef WIN32
extern "C"
{
    extern __stdcall void OutputDebugStringA(const char *text);
};
#endif

static const char *opcodeDescriptions[] = 
{
    //  VST 1.0
	"effOpen", 
	"effClose",
	"effSetProgram",
	"effGetProgram",
	"effSetProgramName",
	"effGetProgramName",
	"effGetParamLabel",
 	"effGetParamDisplay",
	"effGetParamName",
	"effGetVu",
	"effSetSampleRate",
	"effSetBlockSize",
	"effMainsChanged",
	"effEditGetRect",
	"effEditOpen",
	"effEditClose",
	"effEditDraw",
	"effEditMouse",
	"effEditKey",
	"effEditIdle",
	"effEditTop",
	"effEditSleep",
	"effIdentify",
	"effGetChunk",
	"effSetChunk", 
	// VST 2.0
	"effProcessEvents",
	"effCanBeAutomated",
	"effString2Parameter",
	"effGetNumProgramCategories",
	"effGetProgramNameIndexed",
	"effCopyProgram",							
 	"effConnectInput",						
  	"effConnectOutput",						
   	"effGetInputProperties",					
    "effGetOutputProperties",					
    "effGetPlugCategory",
 	"effGetCurrentPosition",					
 	"effGetDestinationBuffer",				
 	"effOfflineNotify",						
 	"effOfflinePrepare",						
 	"effOfflineRun",							
 	"effProcessVarIo",						
 	"effSetSpeakerArrangement",				
 	"effSetBlockSizeAndSampleRate",			
 	"effSetBypass",							
	"effGetEffectName",						
	"effGetErrorText",						
	"effGetVendorString",					
	"effGetProductString",					
	"effGetVendorVersion",					
	"effVendorSpecific",					
	"effCanDo",								
	"effGetTailSize",						
	"effIdle",								
	"effGetIcon",								
 	"effSetViewPosition",						
 	"effGetParameterProperties",				
	"effKeysRequired",						
	"effGetVstVersion",			
    // VST 2.1			
	"effEditKeyDown", 
 	"effEditKeyUp",                           
  	"effSetEditKnobMode",                     
	"effGetMidiProgramName",				
 	"effGetCurrentMidiProgram",				
  	"effGetMidiProgramCategory",			
	"effHasMidiProgramsChanged",				
 	"effGetMidiKeyName",						
  	"effBeginSetProgram",					
   	"effEndSetProgram",			
    // VST 2.3		
    "effGetSpeakerArrangement",
 	"effShellGetNextPlugin",					
  	"effStartProcess",						
   	"effStopProcess",							
    "effSetTotalSampleToProcess",			   
    "effSetPanLaw",							
    "effBeginLoadBank",						
    "effBeginLoadProgram",					
};

static const char *audioMasterDescriptions[] = 
{
    // VST 1.0
	"audioMasterAutomate",
	"audioMasterVersion",
	"audioMasterCurrentId",
	"audioMasterIdle",				
	"audioMasterPinConnected",
	"audioMasterPinConnected1",
	// VST 2.0
	"audioMasterWantMidi",
	"audioMasterGetTime",
 	"audioMasterProcessEvents",
	"audioMasterSetTime",			
    "audioMasterTempoAt",				
	"audioMasterGetNumAutomatableParameters",
	"audioMasterGetParameterQuantization",	
 	"audioMasterIOChanged",				
  	"audioMasterNeedIdle",				
   	"audioMasterSizeWindow",				
    "audioMasterGetSampleRate",
	"audioMasterGetBlockSize",
	"audioMasterGetInputLatency",
	"audioMasterGetOutputLatency",
	"audioMasterGetPreviousPlug",			
 	"audioMasterGetNextPlug",				
  	"audioMasterWillReplaceOrAccumulate",	
   	"audioMasterGetCurrentProcessLevel",
	"audioMasterGetAutomationState",		
 	"audioMasterOfflineStart",
	"audioMasterOfflineRead",				
	"audioMasterOfflineWrite",			
	"audioMasterOfflineGetCurrentPass",
	"audioMasterOfflineGetCurrentMetaPass",
	"audioMasterSetOutputSampleRate",		
 	"audioMasterGetSpeakerArrangement",	
	"audioMasterGetVendorString",		
	"audioMasterGetProductString",		
	"audioMasterGetVendorVersion",		
	"audioMasterVendorSpecific",		
	"audioMasterSetIcon",				
	"audioMasterCanDo",					
	"audioMasterGetLanguage",			
	"audioMasterOpenWindow",			
	"audioMasterCloseWindow",			
	"audioMasterGetDirectory",			
	"audioMasterUpdateDisplay",			
	// VST 2.1
	"audioMasterBeginEdit",             
	"audioMasterEndEdit",               
	"audioMasterOpenFileSelector",	
 	// VST 2.2
	"audioMasterCloseFileSelector",		
	"audioMasterEditFile",				
	"audioMasterGetChunkFile",			
 	// VST 2.3
	"audioMasterGetInputSpeakerArrangement",	
 };

#define VST_TRACE 1

long VSTCALLBACK AudioEffect::audioMasterTrace(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
#if defined(__WIN32__) && defined(VST_TRACE)
    static char buffer[0xff];
    sprintf(buffer, "<< PLUG audioMaster: AEffect 0x%x opcode %d %s index %d value %d ptr 0x%x opt %f\n", effect, opcode, audioMasterDescriptions[opcode], index, value, ptr, opt);
    OutputDebugStringA(buffer);
#endif
	return ((AudioEffect *)effect->object)->originalAudioMaster(effect, opcode, index, value, ptr, opt);
}

//-------------------------------------------------------------------------------------------------------
long dispatchEffectClass (AEffect *e, long opCode, long index, long value, void *ptr, float opt)
{
#if defined(__WIN32__) && defined(VST_TRACE)
    static char buffer[0xff];
    sprintf(buffer, "HOST >> dispatchEffectClass: AEffect 0x%x opCode %d %s index %d value %d ptr 0x%x opt %f\n", e, opCode, opcodeDescriptions[opCode], index, value, ptr, opt);
    OutputDebugStringA(buffer);
#endif
	AudioEffect *ae = (AudioEffect*)(e->object);

	if (opCode == effClose)
	{
		ae->dispatcher (opCode, index, value, ptr, opt);
		delete ae;
		return 1;
	}

	return ae->dispatcher (opCode, index, value, ptr, opt);
}

//-------------------------------------------------------------------------------------------------------
float getParameterClass (AEffect *e, long index)
{
#if defined(__WIN32__) && defined(VST_TRACE)
    static char buffer[0xff];
    sprintf(buffer, "HOST >> getParameterClass: AEffect 0x%x index %d\n", e, index);
    OutputDebugStringA(buffer);
#endif
	AudioEffect *ae = (AudioEffect*)(e->object);
	return ae->getParameter (index);
}

//-------------------------------------------------------------------------------------------------------
void setParameterClass (AEffect *e, long index, float value)
{
#if defined(__WIN32__) && defined(VST_TRACE)
    static char buffer[0xff];
    sprintf(buffer, "HOST >> setParameterClass: AEffect 0x%x index %d value %f\n", e, index, value);
    OutputDebugStringA(buffer);
#endif
	AudioEffect *ae = (AudioEffect*)(e->object);
	ae->setParameter (index, value);
}

//-------------------------------------------------------------------------------------------------------
void processClass (AEffect *e, float **inputs, float **outputs, long sampleFrames)
{
	AudioEffect *ae = (AudioEffect*)(e->object);
	ae->process (inputs, outputs, sampleFrames);
}

//-------------------------------------------------------------------------------------------------------
void processClassReplacing (AEffect *e, float **inputs, float **outputs, long sampleFrames)
{
	AudioEffect *ae = (AudioEffect*)(e->object);
	ae->processReplacing (inputs, outputs, sampleFrames);
}


//-------------------------------------------------------------------------------------------------------
// AudioEffect Implementation
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
AudioEffect::AudioEffect (audioMasterCallback audioMaster, long numPrograms, long numParams)
{
#if defined(VST_TRACE)
	this->originalAudioMaster = audioMaster;
	this->audioMaster = &audioMasterTrace;
#else
	this->audioMaster = audioMaster;
#endif
	editor = 0;
	this->numPrograms = numPrograms;
	this->numParams   = numParams;
	curProgram = 0;

	memset(&cEffect, 0, sizeof (cEffect));
	cEffect.magic = kEffectMagic;
	cEffect.dispatcher = dispatchEffectClass;
	cEffect.process = processClass;
	cEffect.setParameter = setParameterClass;
	cEffect.getParameter = getParameterClass;
	cEffect.numPrograms  = numPrograms;
	cEffect.numParams    = numParams;
	cEffect.numInputs  = 1;
	cEffect.numOutputs = 2;
	cEffect.flags = 0;
	cEffect.resvd1 = 0;
	cEffect.resvd2 = 0;
	cEffect.initialDelay  = 0;
	cEffect.realQualities = 0;
	cEffect.offQualities  = 0;
	cEffect.ioRatio = 1.f;
	cEffect.object = this;
	cEffect.user = 0;
	cEffect.uniqueID = CCONST ('N', 'o', 'E', 'f');		// you must set this!
	cEffect.version = 1;
	cEffect.processReplacing = processClassReplacing;

	sampleRate = 44100.f;
	blockSize = 1024L;
}

//-------------------------------------------------------------------------------------------------------
AudioEffect::~AudioEffect ()
{
	if (editor)
		delete editor;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffect::dispatcher (long opCode, long index, long value, void *ptr, float opt)
{
	long v = 0;
	
	switch (opCode)
	{
		case effOpen:				open ();											break;
		case effClose:				close ();											break;
		case effSetProgram:			if (value < numPrograms) setProgram (value);		break;
		case effGetProgram:			v = getProgram ();									break;
		case effSetProgramName: 	setProgramName ((char *)ptr);						break;
		case effGetProgramName: 	getProgramName ((char *)ptr);						break;
		case effGetParamLabel:		getParameterLabel (index, (char *)ptr);				break;
		case effGetParamDisplay:	getParameterDisplay (index, (char *)ptr);			break;
		case effGetParamName:		getParameterName (index, (char *)ptr);				break;

		case effSetSampleRate:		setSampleRate (opt);								break;
		case effSetBlockSize:		setBlockSize (value);								break;
		case effMainsChanged:		if (!value) suspend (); else resume ();				break;
		case effGetVu:				v = (long)(getVu () * 32767.);						break;


		// Editor	
		case effEditGetRect:		if (editor) v = editor->getRect ((ERect **)ptr);	break;
		case effEditOpen:			if (editor) v = editor->open (ptr);					break;
		case effEditClose:			if (editor) editor->close ();						break;		
		case effEditIdle:			if (editor) editor->idle ();						break;
		
		#if MAC
		case effEditDraw:			if (editor) editor->draw ((ERect *)ptr);			break;
		case effEditMouse:			if (editor) v = editor->mouse (index, value);		break;
		case effEditKey:			if (editor) v = editor->key (value);				break;
		case effEditTop:			if (editor) editor->top ();							break;
		case effEditSleep:			if (editor) editor->sleep ();						break;
		#endif
		
		case effIdentify:			v = CCONST ('N', 'v', 'E', 'f');											break;
		case effGetChunk:			v = getChunk ((void**)ptr, index ? true : false);	break;
		case effSetChunk:			v = setChunk (ptr, value, index ? true : false);	break;
	}
	return v;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffect::getMasterVersion ()
{
	long version = 1;
	if (audioMaster)
	{
		version = audioMaster (&cEffect, audioMasterVersion, 0, 0, 0, 0);
		if (!version)	// old
			version = 1;
	}
	return version;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffect::getCurrentUniqueId ()
{
	long id = 0;
	if (audioMaster)
		id = audioMaster (&cEffect, audioMasterCurrentId, 0, 0, 0, 0);
	return id;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::masterIdle ()
{
	if (audioMaster)
		audioMaster (&cEffect, audioMasterIdle, 0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffect::isInputConnected (long input)
{
	long ret = 0;
	if (audioMaster)
		ret = audioMaster (&cEffect, audioMasterPinConnected, input, 0, 0, 0);
	return ret ? false : true;		// return value is 0 for true
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffect::isOutputConnected (long output)
{
	long ret = 0;
	if (audioMaster)
		ret = audioMaster (&cEffect, audioMasterPinConnected, output, 1, 0, 0);
	return ret ? false : true;		// return value is 0 for true
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::setParameterAutomated (long index, float value)
{
	setParameter (index, value);
	if (audioMaster)
		audioMaster (&cEffect, audioMasterAutomate, index, 0, 0, value);	// value is in opt
}

//-------------------------------------------------------------------------------------------------------
// Flags
//-------------------------------------------------------------------------------------------------------
void AudioEffect::hasVu (bool state)
{
	if (state)
		cEffect.flags |= effFlagsHasVu;
	else
		cEffect.flags &= ~effFlagsHasVu;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::hasClip (bool state)
{
	if (state)
		cEffect.flags |= effFlagsHasClip;
	else
		cEffect.flags &= ~effFlagsHasClip;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::canMono (bool state)
{
	if (state)
		cEffect.flags |= effFlagsCanMono;
	else
		cEffect.flags &= ~effFlagsCanMono;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::canProcessReplacing (bool state)
{
	if (state)
		cEffect.flags |= effFlagsCanReplacing;
	else
		cEffect.flags &= ~effFlagsCanReplacing;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::programsAreChunks (bool state)
{
	if (state)
		cEffect.flags |= effFlagsProgramChunks;
	else
		cEffect.flags &= ~effFlagsProgramChunks;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::setRealtimeQualities (long qualities)
{
	cEffect.realQualities = qualities;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::setOfflineQualities (long qualities)
{
	cEffect.offQualities = qualities;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::setInitialDelay (long delay)
{
	cEffect.initialDelay = delay;
}

//-------------------------------------------------------------------------------------------------------
// Strings Conversion
//-------------------------------------------------------------------------------------------------------
void AudioEffect::dB2string (float value, char *text)
{
	if (value <= 0)
	#if MAC
		strcpy (text, "-°");
	#else
		strcpy (text, "-oo");
	#endif
	else
		float2string ((float)(20. * log10 (value)), text);
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::Hz2string (float samples, char *text)
{
	float sampleRate = getSampleRate ();
	if (!samples)
		float2string (0, text);
	else
		float2string (sampleRate / samples, text);
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::ms2string (float samples, char *text)
{
	float2string ((float)(samples * 1000. / getSampleRate ()), text);
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::float2string (float value, char *text)
{
	long c = 0, neg = 0;
	char string[32];
	char *s;
	double v, integ, i10, mantissa, m10, ten = 10.;
	
	v = (double)value;
	if (v < 0)
	{
		neg = 1;
		value = -value;
		v = -v;
		c++;
		if (v > 9999999.)
		{
			strcpy (string, "Huge!");
			return;
		}
	}
	else if( v > 99999999.)
	{
		strcpy (string, "Huge!");
		return;
	}

	s = string + 31;
	*s-- = 0;
	*s-- = '.';
	c++;
	
	integ = floor (v);
	i10 = fmod (integ, ten);
	*s-- = (char)((long)i10 + '0');
	integ /= ten;
	c++;
	while (integ >= 1. && c < 8)
	{
		i10 = fmod (integ, ten);
		*s-- = (char)((long)i10 + '0');
		integ /= ten;
		c++;
	}
	if (neg)
		*s-- = '-';
	strcpy (text, s + 1);
	if (c >= 8)
		return;

	s = string + 31;
	*s-- = 0;
	mantissa = fmod (v, 1.);
	mantissa *= pow (ten, (double)(8 - c));
	while (c < 8)
	{
		if (mantissa <= 0)
			*s-- = '0';
		else
		{
			m10 = fmod (mantissa, ten);
			*s-- = (char)((long)m10 + '0');
			mantissa /= 10.;
		}
		c++;
	}
	strcat (text, s + 1);
}

//-------------------------------------------------------------------------------------------------------
void AudioEffect::long2string (long value, char *text)
{
	char string[32];
	
	if (value >= 100000000)
	{
		strcpy (text, "Huge!");
		return;
	}
	sprintf (string, "%7d", (int)value);
	string[8] = 0;
	strcpy (text, (char *)string);
}
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
