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

//-------------------------------------------------------------------------------------------------------
long dispatchEffectClass (AEffect *e, long opCode, long index, long value, void *ptr, float opt)
{
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
	AudioEffect *ae = (AudioEffect*)(e->object);
	return ae->getParameter (index);
}

//-------------------------------------------------------------------------------------------------------
void setParameterClass (AEffect *e, long index, float value)
{
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
	this->audioMaster = audioMaster;
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
