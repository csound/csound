//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.3 Extension
// © 2003 Steinberg Media Technologies, All Rights Reserved
//
// you should not have to edit this file
// use override methods instead, as suggested in the class declaration (audioeffectx.h)
//-------------------------------------------------------------------------------------------------------

#ifndef __audioeffectx__
#include "audioeffectx.h"
#endif

#ifndef __AEffEditor__
#include "AEffEditor.hpp"
#endif

//---------------------------------------------------------------------------------------------
// 'canDo' strings. note other 'canDos' can be evaluated by calling the according
// function, for instance if getSampleRate returns 0, you
// will certainly want to assume that this selector is not supported.
//---------------------------------------------------------------------------------------------

const char* hostCanDos [] =
{
	"sendVstEvents",
	"sendVstMidiEvent",
	"sendVstTimeInfo",
	"receiveVstEvents",
	"receiveVstMidiEvent",
	"receiveVstTimeInfo",
	
	"reportConnectionChanges",
	"acceptIOChanges",
	"sizeWindow",

	"asyncProcessing",
	"offline",
	"supplyIdle",
	"supportShell",		// 'shell' handling via uniqueID as suggested by Waves
	"openFileSelector"
#if VST_2_2_EXTENSIONS
	,
	"editFile",
	"closeFileSelector"
#endif // VST_2_2_EXTENSIONS
#if VST_2_3_EXTENSIONS
	,
	"startStopProcess"
#endif // VST_2_3_EXTENSIONS
};

const char* plugCanDos [] =
{
	"sendVstEvents",
	"sendVstMidiEvent",
	"sendVstTimeInfo",
	"receiveVstEvents",
	"receiveVstMidiEvent",
	"receiveVstTimeInfo",
	"offline",
	"plugAsChannelInsert",
	"plugAsSend",
	"mixDryWet",
	"noRealTime",
	"multipass",
	"metapass",
	"1in1out",
	"1in2out",
	"2in1out",
	"2in2out",
	"2in4out",
	"4in2out",
	"4in4out",
	"4in8out",	// 4:2 matrix to surround bus
	"8in4out",	// surround bus to 4:2 matrix
	"8in8out"
#if VST_2_1_EXTENSIONS
	,
	"midiProgramNames",
	"conformsToWindowRules"		// mac: doesn't mess with grafport. general: may want
								// to call sizeWindow (). if you want to use sizeWindow (),
								// you must return true (1) in canDo ("conformsToWindowRules")
#endif // VST_2_1_EXTENSIONS

#if VST_2_3_EXTENSIONS
	,
	"bypass"
#endif // VST_2_3_EXTENSIONS
};

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
// AudioEffectX extends AudioEffect with the new features. so you should derive
// your plug from AudioEffectX
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// VstEvents + VstTimeInfo
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
AudioEffectX::AudioEffectX (audioMasterCallback audioMaster, long numPrograms, long numParams)
	: AudioEffect (audioMaster, numPrograms, numParams)
{
}

//-------------------------------------------------------------------------------------------------------
AudioEffectX::~AudioEffectX ()
{
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::dispatcher (long opCode, long index, long value, void *ptr, float opt)
{
	long v = 0;
	switch(opCode)
	{
		// VstEvents
		case effProcessEvents:
			v = processEvents ((VstEvents*)ptr);
			break;

		// parameters and programs
		case effCanBeAutomated:
			v = canParameterBeAutomated (index) ? 1 : 0;
			break;
		case effString2Parameter:
			v = string2parameter (index, (char*)ptr) ? 1 : 0;
			break;

		case effGetNumProgramCategories:
			v = getNumCategories ();
			break;
		case effGetProgramNameIndexed:
			v = getProgramNameIndexed (value, index, (char*)ptr) ? 1 : 0;
			break;
		case effCopyProgram:
			v = copyProgram (index) ? 1 : 0;
			break;

		// connections, configuration
		case effConnectInput:
			inputConnected (index, value ? true : false);
			v = 1;
			break;	
		case effConnectOutput:
			outputConnected (index, value ? true : false);
			v = 1;
			break;	
		case effGetInputProperties:
			v = getInputProperties (index, (VstPinProperties*)ptr) ? 1 : 0;
			break;
		case effGetOutputProperties:
			v = getOutputProperties (index, (VstPinProperties*)ptr) ? 1 : 0;
			break;
		case effGetPlugCategory:
			v = (long)getPlugCategory ();
			break;

		// realtime
		case effGetCurrentPosition:
			v = reportCurrentPosition ();
			break;
			
		case effGetDestinationBuffer:
			v = (long)reportDestinationBuffer ();
			break;

		// offline
		case effOfflineNotify:
			v = offlineNotify ((VstAudioFile*)ptr, value, index != 0);
			break;
		case effOfflinePrepare:
			v = offlinePrepare ((VstOfflineTask*)ptr, value);
			break;
		case effOfflineRun:
			v = offlineRun ((VstOfflineTask*)ptr, value);
			break;

		// other
		case effSetSpeakerArrangement:
			v = setSpeakerArrangement ((VstSpeakerArrangement*)value, (VstSpeakerArrangement*)ptr) ? 1 : 0;
			break;
		case effProcessVarIo:
			v = processVariableIo ((VstVariableIo*)ptr) ? 1 : 0;
			break;
		case effSetBlockSizeAndSampleRate:
			setBlockSizeAndSampleRate (value, opt);
			v = 1;
			break;
		case effSetBypass:
			v = setBypass (value ? true : false) ? 1 : 0;
			break;
		case effGetEffectName:
			v = getEffectName ((char *)ptr) ? 1 : 0;
			break;
		case effGetErrorText:
			v = getErrorText ((char *)ptr) ? 1 : 0;
			break;
		case effGetVendorString:
			v = getVendorString ((char *)ptr) ? 1 : 0;
			break;
		case effGetProductString:
			v = getProductString ((char *)ptr) ? 1 : 0;
			break;
		case effGetVendorVersion:
			v = getVendorVersion ();
			break;
		case effVendorSpecific:
			v = vendorSpecific (index, value, ptr, opt);
			break;
		case effCanDo:
			v = canDo ((char*)ptr);
			break;
		case effGetIcon:
			v = (long)getIcon ();
			break;
		case effSetViewPosition:
			v = setViewPosition (index, value) ? 1 : 0;
			break;		
		case effGetTailSize:
			v = getGetTailSize ();
			break;
		case effIdle:
			v = fxIdle ();
			break;

		case effGetParameterProperties:
			v = getParameterProperties (index, (VstParameterProperties*)ptr) ? 1 : 0;
			break;

		case effKeysRequired:
			v = (keysRequired () ? 0 : 1);	// reversed to keep v1 compatibility
			break;
		case effGetVstVersion:
			v = getVstVersion ();
			break;

	#if VST_2_1_EXTENSIONS
		case effEditKeyDown:
			if (editor)
			{
				VstKeyCode keyCode = {index, (unsigned char)value, (unsigned char)opt};
				v = editor->onKeyDown (keyCode);
			}
			break;

		case effEditKeyUp:
			if (editor)
			{
				VstKeyCode keyCode = {index, (unsigned char)value, (unsigned char)opt};
				v = editor->onKeyUp (keyCode);
			}
			break;

		case effSetEditKnobMode:
			if (editor)
				v = editor->setKnobMode (value);
			break;

		case effGetMidiProgramName:
			v = getMidiProgramName (index, (MidiProgramName*)ptr);
			break;
		case effGetCurrentMidiProgram:
			v = getCurrentMidiProgram (index, (MidiProgramName*)ptr);
			break;
		case effGetMidiProgramCategory:
			v = getMidiProgramCategory (index, (MidiProgramCategory*)ptr);
			break;
		case effHasMidiProgramsChanged:
			v = hasMidiProgramsChanged (index) ? 1 : 0;
			break;
		case effGetMidiKeyName:
			v = getMidiKeyName (index, (MidiKeyName*)ptr) ? 1 : 0;
			break;

		case effBeginSetProgram:
			v = beginSetProgram () ? 1 : 0;
			break;
		case effEndSetProgram:
			v = endSetProgram () ? 1 : 0;
			break;

		#endif // VST_2_1_EXTENSIONS

		#if VST_2_3_EXTENSIONS
		case effGetSpeakerArrangement:
			v = getSpeakerArrangement ((VstSpeakerArrangement**)value, (VstSpeakerArrangement**)ptr) ? 1 : 0;
			break;

		case effSetTotalSampleToProcess:
			v = setTotalSampleToProcess (value);
			break;

		case effShellGetNextPlugin:
			v = getNextShellPlugin ((char*)ptr);
			break;

		case effStartProcess:
			v = startProcess ();
			break;
		case effStopProcess:
			v = stopProcess ();
			break;

		case effSetPanLaw:
			v = setPanLaw (value, opt) ? 1 : 0;
			break;

		case effBeginLoadBank:
			v = beginLoadBank ((VstPatchChunkInfo*)ptr);
			break;
		case effBeginLoadProgram:
			v = beginLoadProgram ((VstPatchChunkInfo*)ptr);
			break;
		#endif // VST_2_3_EXTENSIONS

		// version 1.0 or unknown
		default:
			v = AudioEffect::dispatcher (opCode, index, value, ptr, opt);
	}
	return v;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::wantEvents (long filter)
{
	if (audioMaster)
		audioMaster (&cEffect, audioMasterWantMidi, 0, filter, 0, 0);
	
}

//-------------------------------------------------------------------------------------------------------
VstTimeInfo* AudioEffectX::getTimeInfo (long filter)
{
	if (audioMaster)
		return (VstTimeInfo*) audioMaster (&cEffect, audioMasterGetTime, 0, filter, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::tempoAt (long pos)
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterTempoAt, 0, pos, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::sendVstEventsToHost (VstEvents* events)
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterProcessEvents, 0, 0, events, 0) == 1;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
// Parameters
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getNumAutomatableParameters ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetNumAutomatableParameters, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getParameterQuantization ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetParameterQuantization, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
// Configuration
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::ioChanged ()
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterIOChanged, 0, 0, 0, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::needIdle ()
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterNeedIdle, 0, 0, 0, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::sizeWindow (long width, long height)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterSizeWindow, width, height, 0, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
double AudioEffectX::updateSampleRate ()
{
	if (audioMaster)
	{
		long res = audioMaster (&cEffect, audioMasterGetSampleRate, 0, 0, 0, 0);
		if (res > 0)
			sampleRate = (float)res;
	}
	return sampleRate;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::updateBlockSize ()
{
	if (audioMaster)
	{
		long res = audioMaster (&cEffect, audioMasterGetBlockSize, 0, 0, 0, 0);
		if (res > 0)
			blockSize = res;
	}
	return blockSize;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getInputLatency ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetInputLatency, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getOutputLatency ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetOutputLatency, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
AEffect* AudioEffectX::getPreviousPlug (long input)
{
	if (audioMaster)
		return (AEffect*) audioMaster (&cEffect, audioMasterGetPreviousPlug, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
AEffect* AudioEffectX::getNextPlug (long output)
{
	if (audioMaster)
		return (AEffect*) audioMaster (&cEffect, audioMasterGetNextPlug, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
// Configuration
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::willProcessReplacing ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterWillReplaceOrAccumulate, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getCurrentProcessLevel ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetCurrentProcessLevel, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getAutomationState ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetAutomationState, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::wantAsyncOperation (bool state)
{
	if (state)
		cEffect.flags |= effFlagsExtIsAsync;
	else
		cEffect.flags &= ~effFlagsExtIsAsync;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::hasExternalBuffer (bool state)
{
	if (state)
		cEffect.flags |= effFlagsExtHasBuffer;
	else
		cEffect.flags &= ~effFlagsExtHasBuffer;
}

//-------------------------------------------------------------------------------------------------------
// Offline
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::offlineRead (VstOfflineTask* offline, VstOfflineOption option, bool readSource)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterOfflineRead, readSource, option, offline, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::offlineWrite (VstOfflineTask* offline, VstOfflineOption option)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterOfflineWrite, 0, option, offline, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::offlineStart (VstAudioFile* audioFiles, long numAudioFiles, long numNewAudioFiles)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterOfflineStart, numNewAudioFiles, numAudioFiles, audioFiles, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::offlineGetCurrentPass ()
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterOfflineGetCurrentPass, 0, 0, 0, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::offlineGetCurrentMetaPass ()
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterOfflineGetCurrentMetaPass, 0, 0, 0, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
// Other
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::setOutputSamplerate (float sampleRate)
{
	if (audioMaster)
		audioMaster (&cEffect, audioMasterSetOutputSampleRate, 0, 0, 0, sampleRate);
}

//-------------------------------------------------------------------------------------------------------
VstSpeakerArrangement* AudioEffectX::getInputSpeakerArrangement ()
{
	if (audioMaster)
		return (VstSpeakerArrangement*)audioMaster (&cEffect, audioMasterGetInputSpeakerArrangement, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
VstSpeakerArrangement* AudioEffectX::getOutputSpeakerArrangement ()
{
	if (audioMaster)
		return (VstSpeakerArrangement*)audioMaster (&cEffect, audioMasterGetOutputSpeakerArrangement, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::getHostVendorString (char* text)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterGetVendorString, 0, 0, text, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::getHostProductString (char* text)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterGetProductString, 0, 0, text, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getHostVendorVersion ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetVendorVersion, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::hostVendorSpecific (long lArg1, long lArg2, void* ptrArg, float floatArg)
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterVendorSpecific, lArg1, lArg2, ptrArg, floatArg);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::canHostDo (char* text)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterCanDo, 0, 0, text, 0) != 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::isSynth (bool state)
{
	if (state)
		cEffect.flags |= effFlagsIsSynth;
	else
		cEffect.flags &= ~effFlagsIsSynth;
}

//-------------------------------------------------------------------------------------------------------
void AudioEffectX::noTail (bool state)
{
	if (state)
		cEffect.flags |= effFlagsNoSoundInStop;
	else
		cEffect.flags &= ~effFlagsNoSoundInStop;
}

//-------------------------------------------------------------------------------------------------------
long AudioEffectX::getHostLanguage ()
{
	if (audioMaster)
		return audioMaster (&cEffect, audioMasterGetLanguage, 0, 0, 0, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
void* AudioEffectX::openWindow (VstWindow* window)
{
	if (audioMaster)
		return (void*)audioMaster (&cEffect, audioMasterOpenWindow, 0, 0, window, 0);
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::closeWindow (VstWindow* window)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterCloseWindow, 0, 0, window, 0) != 0);
	return false;
}

//-------------------------------------------------------------------------------------------------------
void* AudioEffectX::getDirectory ()
{
	if (audioMaster)
		return (void*)(audioMaster (&cEffect, audioMasterGetDirectory, 0, 0, 0, 0));
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::updateDisplay ()
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterUpdateDisplay, 0, 0, 0, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::beginEdit (long index)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterBeginEdit, index, 0, 0, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::endEdit (long index)
{
	if (audioMaster)
		return (audioMaster (&cEffect, audioMasterEndEdit, index, 0, 0, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::openFileSelector (VstFileSelect* ptr)
{
	if (audioMaster && ptr)
		return (audioMaster (&cEffect, audioMasterOpenFileSelector, 0, 0, ptr, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::closeFileSelector (VstFileSelect* ptr)
{
	if (audioMaster && ptr)
		return (audioMaster (&cEffect, audioMasterCloseFileSelector, 0, 0, ptr, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::getChunkFile (void* nativePath)
{
	if (audioMaster && nativePath)
		return (audioMaster (&cEffect, audioMasterGetChunkFile, 0, 0, nativePath, 0)) ? true : false;
	return 0;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::allocateArrangement (VstSpeakerArrangement** arrangement, long nbChannels)
{
	if (*arrangement) 
	{ 
		char *ptr = (char*)(*arrangement); 
		delete []ptr;
	}
	long size = sizeof (long) + sizeof (long) + (nbChannels) * sizeof (VstSpeakerProperties);
	char *ptr = new char [size];
	if (!ptr)
		return false;
	memset (ptr, 0, size);
	*arrangement = (VstSpeakerArrangement*)ptr;
	(*arrangement)->numChannels = nbChannels;
	return true;
}

//-------------------------------------------------------------------------------------------------------	
bool AudioEffectX::deallocateArrangement (VstSpeakerArrangement** arrangement)
{
	if (*arrangement) 
	{ 
		char *ptr = (char*)(*arrangement); 
		delete []ptr; 
		*arrangement = 0;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------	
bool AudioEffectX::copySpeaker (VstSpeakerProperties* to, VstSpeakerProperties* from)
{
	// We assume here that "to" exists yet, ie this function won't
	// allocate memory for the speaker (this will prevent from having
	// a difference between an Arrangement's number of channels and
	// its actual speakers...)
	if ((from == NULL) || (to == NULL))
		return false;
	
	strcpy (to->name, from->name);
	to->type = from->type;
	to->azimuth = from->azimuth;
	to->elevation = from->elevation;
	to->radius = from->radius;
	to->reserved = from->reserved;
	memcpy (to->future, from->future, 28);
	
	return true;
}

//-------------------------------------------------------------------------------------------------------
bool AudioEffectX::matchArrangement (VstSpeakerArrangement** to, VstSpeakerArrangement* from)
{
	if (from == NULL)
		return false;

	if ((!deallocateArrangement (to)) || (!allocateArrangement (to, from->numChannels)))
		return false;
	
	(*to)->type = from->type;
	for (int i = 0; i < (*to)->numChannels; i++)
	{
		if (!copySpeaker (&((*to)->speakers[i]), &(from->speakers[i])))
			return false;
	}

	return true;

}

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------



