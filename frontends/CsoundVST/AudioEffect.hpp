//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 1.0
// © 2003, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __AudioEffect__
#define __AudioEffect__

#ifndef __AEffect__
#include "AEffect.h"	// "c" interface
#endif

class AEffEditor;
class AudioEffect;

//-------------------------------------------------------------------------------------------------------
// Needs to be defined by the audio effect and is
// called to create the audio effect object instance.
AudioEffect* createEffectInstance (audioMasterCallback audioMaster);

long dispatchEffectClass (AEffect *e, long opCode, long index, long value, void *ptr, float opt);
float getParameterClass (long index);
void setParameterClass (long index, float value);
void processClass (AEffect *e, float **inputs, float **outputs, long sampleFrames);
void processClassReplacing (AEffect *e, float **inputs, float **outputs, long sampleFrames);

//-------------------------------------------------------------------------------------------------------
class AudioEffect
{
friend class AEffEditor;
friend long dispatchEffectClass (AEffect *e, long opCode, long index, long value, void *ptr, float opt);
friend float getParameterClass (AEffect *e, long index);
friend void setParameterClass (AEffect *e, long index, float value);
friend void processClass (AEffect *e, float **inputs, float **outputs, long sampleFrames);
friend void processClassReplacing (AEffect *e, float **inputs, float **outputs, long sampleFrames);

public:
	AudioEffect (audioMasterCallback audioMaster, long numPrograms, long numParams);
	virtual ~AudioEffect ();

	virtual void  setParameter (long index, float value) { index = index; value = value; }
	virtual float getParameter (long index) { index = index; return 0; }
	virtual void  setParameterAutomated (long index, float value);

	AEffect *getAeffect () { return &cEffect; }		// Returns the AEffect Structure

	void setEditor (AEffEditor *editor)
	{	this->editor = editor;
		if (editor) cEffect.flags |= effFlagsHasEditor;
		else cEffect.flags &= ~effFlagsHasEditor; }	// Should be called if you want to define your own editor

	//---Called from audio master (Host -> Plug)---------------
	virtual void process (float **inputs, float **outputs, long sampleFrames) = 0;
	virtual void processReplacing (float **inputs, float **outputs, long sampleFrames)
		{ inputs = inputs; outputs = outputs; sampleFrames = sampleFrames; }
	
	virtual long dispatcher (long opCode, long index, long value, void *ptr, float opt);	// Opcodes dispatcher

	virtual void open () {}		// Called when Plugin is initialized
	virtual void close () {}	// Called when Plugin will be released
	
	//---Program----------------------------
	virtual long getProgram () { return curProgram; }
	virtual void setProgram (long program) { curProgram = program; }// Don't forget to set curProgram
	virtual void setProgramName (char *name) { *name = 0; }			// All following refer to curProgram
	virtual void getProgramName (char *name) { *name = 0; }
	
	virtual void getParameterLabel (long index, char *label) { index = index; *label = 0; } // example: "dB"
	virtual void getParameterDisplay (long index, char *text) { index = index; *text = 0; } // example: "6.01"
	virtual void getParameterName (long index, char *text) { index = index; *text = 0; }    // example: "Volume"
	
	virtual float getVu () { return 0; }
	
	virtual long getChunk (void** data, bool isPreset = false) { return 0; } // Returns the Size in bytes of the chunk (Plugin allocates the data array)
	virtual long setChunk (void* data, long byteSize, bool isPreset = false) { return 0; }
	
	virtual void setSampleRate (float sampleRate) { this->sampleRate = sampleRate; }
	virtual void setBlockSize (long blockSize)    { this->blockSize = blockSize; }
	
	virtual void suspend () {}	// Called when Plugin is switched to Off
	virtual void resume () {}	// Called when Plugin is switched to On

	//---Setup---------------------------
	virtual void setUniqueID (long iD) { cEffect.uniqueID = iD; }		// must call this!
	virtual void setNumInputs (long inputs) { cEffect.numInputs = inputs; }
	virtual void setNumOutputs (long outputs) { cEffect.numOutputs = outputs; }

	virtual void hasVu (bool state = true);
	virtual void hasClip (bool state = true);
	virtual void canMono (bool state = true);
	virtual void canProcessReplacing (bool state = true);	// Tells that the processReplacing () could be used
	virtual void programsAreChunks (bool state = true);
	virtual void setRealtimeQualities (long qualities);
	virtual void setOfflineQualities (long qualities);
	virtual void setInitialDelay (long delay);	// Uses to report the Plugin's latency (Group Delay)

	//---Inquiry-------------------------
	virtual float getSampleRate () { return sampleRate; }
	virtual long getBlockSize ()   { return blockSize; }

	//---Host communication--------------
	virtual long getMasterVersion ();
	virtual long getCurrentUniqueId ();
	virtual void masterIdle ();
	virtual bool isInputConnected (long input);
	virtual bool isOutputConnected (long output);

	//---Tools---------------------------
	virtual void dB2string (float value, char *text);
	virtual void Hz2string (float samples, char *text);
	virtual void ms2string (float samples, char *text);
	virtual void float2string (float value, char *string);
	virtual void long2string (long value, char *text);

protected:	
	//---Members-------------------------
	float sampleRate;
	AEffEditor *editor;
	audioMasterCallback audioMaster;
	long numPrograms;
	long numParams;
	long curProgram;
	long blockSize;
	AEffect cEffect;
};

#endif // __AudioEffect__
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

