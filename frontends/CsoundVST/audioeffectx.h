//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.3 Extension
// © 2003, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __audioeffectx__
#define __audioeffectx__

#ifndef __AudioEffect__
#include "AudioEffect.hpp"	 // Version 1.0 base class AudioEffect
#endif

#ifndef __aeffectx__
#include "aeffectx.h"		 // Version 2.0 'C' extensions and structures
#endif

#define VST_2_1_EXTENSIONS 1 // Version 2.1 extensions
#define VST_2_2_EXTENSIONS 1 // Version 2.2 extensions
#define VST_2_3_EXTENSIONS 1 // Version 2.3 extensions

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
// AudioEffectX extends AudioEffect with new features. So you should derive
// your Plugin from AudioEffectX
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

class AudioEffectX : public AudioEffect
{
public:
	// Constructor
	AudioEffectX (audioMasterCallback audioMaster, long numPrograms, long numParams);
	
	// Destructor
	virtual ~AudioEffectX ();

	// Dispatcher
	virtual long dispatcher (long opCode, long index, long value, void *ptr, float opt);
	
	virtual AEffEditor* getEditor () { return editor; } // Returns the attached editor

	// 'Plug -> Host' are methods which go from Plugin to Host, and are usually not overridden
	// 'Host -> Plug' are methods which you may override to implement the according functionality (to Host)

//-------------------------------------------------------------------------------------------------------
// Events + Time
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual void wantEvents (long filter = 1);		// Filter is currently ignored, midi channel data only (default)
	virtual VstTimeInfo* getTimeInfo (long filter);	// Returns const VstTimeInfo* (or 0 if not supported)
													// filter should contain a mask indicating which fields are requested
													// (see valid masks in aeffectx.h), as some items may require extensive conversions 
	virtual long tempoAt (long pos);				// Returns tempo (in bpm * 10000) at sample frame location <pos>
	bool sendVstEventsToHost (VstEvents* events);	// Returns true when success

	// Host -> Plug
	virtual long processEvents (VstEvents* events) { return 0; }// 0 means 'wants no more'...else returns 1!
																// VstEvents and VstMidiEvents are declared in aeffectx.h

//-------------------------------------------------------------------------------------------------------
// Parameters and Programs
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual long getNumAutomatableParameters ();	// Returns the number of automatable Parameters (should be <= than numParams)
	virtual long getParameterQuantization ();		// Returns the integer value for +1.0 representation,
													// or 1 if full single float precision is maintained
													// in automation. parameter index in <value> (-1: all, any)
	// Host -> Plug
	virtual bool canParameterBeAutomated (long index) { return true; }
	virtual bool string2parameter (long index, char* text) { return false; } // Note: implies setParameter. text==0 is to be
																			 // expected to check the capability (returns true).	
	virtual float getChannelParameter (long channel, long index) { return 0.f; }
	virtual long getNumCategories () { return 1L; }
	virtual bool getProgramNameIndexed (long category, long index, char* text) { return false; }
	virtual bool copyProgram (long destination) { return false; }

//-------------------------------------------------------------------------------------------------------
// Connections, Configuration
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual bool ioChanged ();				// Tell Host numInputs and/or numOutputs and/or initialDelay and/or numParameters has changed
	virtual bool needIdle ();				// Plugin needs idle calls (outside its editor window), will call fxIdle ()
	virtual bool sizeWindow (long width, long height);

	virtual double updateSampleRate ();		// Returns sample rate from Host (may issue setSampleRate() )
	virtual long updateBlockSize ();		// Same for block size
	virtual long getInputLatency ();		// Returns input Latency
	virtual long getOutputLatency ();		// Returns output Latency
	virtual AEffect* getPreviousPlug (long input);	// Input can be -1 in which case the first found is returned
	virtual AEffect* getNextPlug (long output);		// Output can be -1 in which case the first found is returned

	// Host -> Plug
	virtual void inputConnected (long index, bool state) {}		// Input at <index> has been (dis-)connected,
	virtual void outputConnected (long index, bool state) {}	// Same as input; state == true: connected
	virtual bool getInputProperties (long index, VstPinProperties* properties) { return false; }
	virtual bool getOutputProperties (long index, VstPinProperties* properties) { return false; }

	virtual VstPlugCategory getPlugCategory ()
	{ if (cEffect.flags & effFlagsIsSynth) return kPlugCategSynth; return kPlugCategUnknown; } // See aeffects.h for Category

//-------------------------------------------------------------------------------------------------------
// Realtime
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual long willProcessReplacing ();	// Returns 0: not implemented, 1: replacing, 2: accumulating
	virtual long getCurrentProcessLevel ();	// Returns 0: not supported,
											// 1: currently in user thread (gui)
											// 2: currently in audio thread or irq (where process is called)
											// 3: currently in 'sequencer' thread or irq (midi, timer etc)
											// 4: currently offline processing and thus in user thread
											// other: not defined, but probably pre-empting user thread.
	virtual long getAutomationState ();		// Returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
	virtual void wantAsyncOperation (bool state = true);	// Notify Host that we want to operate asynchronously.
											// process () will return immediately; Host will poll getCurrentPosition
											// to see if data are available in time.
	virtual void hasExternalBuffer (bool state = true);		// For external DSP, may have their own output buffer (32 bit float)
											// Host then requests this via effGetDestinationBuffer

	// Host -> Plug
	virtual long reportCurrentPosition () { return 0; }		// For external DSP, see wantAsyncOperation ()
	virtual float* reportDestinationBuffer () { return 0; }	// For external DSP (dma option)

//-------------------------------------------------------------------------------------------------------
// Offline
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual bool offlineRead (VstOfflineTask* offline, VstOfflineOption option, bool readSource = true);
	virtual bool offlineWrite (VstOfflineTask* offline, VstOfflineOption option);
	virtual bool offlineStart (VstAudioFile* ptr, long numAudioFiles, long numNewAudioFiles);
	virtual long offlineGetCurrentPass ();
	virtual long offlineGetCurrentMetaPass ();

	// Host -> Plug
	virtual bool offlineNotify (VstAudioFile* ptr, long numAudioFiles, bool start) { return false; }
	virtual bool offlinePrepare (VstOfflineTask* offline, long count) { return false; }
	virtual bool offlineRun (VstOfflineTask* offline, long count) { return false; }

	virtual long offlineGetNumPasses () { return 0; }
	virtual long offlineGetNumMetaPasses () { return 0; }

//-------------------------------------------------------------------------------------------------------
// Other
//-------------------------------------------------------------------------------------------------------

	// Plug -> Host
	virtual void setOutputSamplerate (float samplerate);
	virtual VstSpeakerArrangement* getInputSpeakerArrangement ();
	virtual VstSpeakerArrangement* getOutputSpeakerArrangement ();

	virtual bool getHostVendorString (char* text);	// Fills <text> with a string identifying the vendor (max 64 char)
	virtual bool getHostProductString (char* text);	// Fills <text> with a string with product name (max 64 char)
	virtual long getHostVendorVersion ();			// Returns vendor-specific version
	virtual long hostVendorSpecific (long lArg1, long lArg2, void* ptrArg, float floatArg);	// No specific definition
	virtual long canHostDo (char* text);			// See 'hostCanDos' in audioeffectx.cpp
													// returns 0: don't know (default), 1: yes, -1: no

	virtual void isSynth (bool state = true);		// Will call wantEvents if true
	virtual void noTail (bool state = true);		// true: tells Host we produce no output when silence comes in
													// enables Host to omit process() when no data are present on any one input.

	virtual long getHostLanguage ();				// Returns VstHostLanguage (see aeffectx.h)
	virtual void* openWindow (VstWindow*);			// Create new window
	virtual bool closeWindow (VstWindow*);			// Close a newly created window
	virtual void* getDirectory ();					// Get the Plugin's directory, FSSpec on MAC, else char*
	virtual bool updateDisplay ();					// Something has changed, update display like program list, parameter list
													// returns true if supported

	// Host -> Plug
	virtual bool processVariableIo (VstVariableIo* varIo) { return false; } // If called with varIo == NULL, returning true indicates that this call is supported by the Plugin
	virtual bool setSpeakerArrangement (VstSpeakerArrangement* pluginInput, VstSpeakerArrangement* pluginOutput) { return false; }
	virtual bool getSpeakerArrangement (VstSpeakerArrangement** pluginInput, VstSpeakerArrangement** pluginOutput) { *pluginInput = 0; *pluginOutput = 0; return false; }
	virtual void setBlockSizeAndSampleRate (long blockSize, float sampleRate)
		{ this->blockSize = blockSize; this->sampleRate = sampleRate; }
	virtual bool setBypass (bool onOff) { return false; }		// For 'soft-bypass; process () still called

	virtual bool getEffectName (char* name) { return false; }	// Name max 32 char
	virtual bool getErrorText (char* text) { return false; }	// Text max 256 char
	virtual bool getVendorString (char* text) { return false; }	// Fill text with a string identifying the vendor (max 64 char)
	virtual bool getProductString (char* text) { return false; }// Fill text with a string identifying the product name (max 64 char)					// fills <ptr> with a string with product name (max 64 char)
	virtual long getVendorVersion () { return 0; }				// Return vendor-specific version
	virtual long vendorSpecific (long lArg, long lArg2, void* ptrArg, float floatArg) { return 0; }
														// No definition, vendor specific handling
	virtual long canDo (char* text) { return 0; }		// See 'plugCanDos' in audioeffectx.cpp
														// returns 0: don't know (default), 1: yes, -1: no
	virtual void* getIcon () { return 0; }				// Not yet defined
	virtual bool setViewPosition (long x, long y) { return false; }
	virtual long getGetTailSize () { return 0; }
	virtual long fxIdle () { return 0; }				// Called when NeedIdle () was called
	virtual bool getParameterProperties (long index, VstParameterProperties* p) { return false; }
	virtual bool keysRequired () { return false; }		// Version 1 Plugins will return true

	#if VST_2_3_EXTENSIONS
	virtual long getVstVersion () { return 2300; }		// Returns the current VST Version
	#elif VST_2_2_EXTENSIONS
	virtual long getVstVersion () { return 2200; }
	#elif VST_2_1_EXTENSIONS
	virtual long getVstVersion () { return 2100; }
	#else
	virtual long getVstVersion () { return 2; }
	#endif

//---------------------------------------------------------
#if VST_2_1_EXTENSIONS
//-------------------------------------------------------------------------------------------------------
// Midi Program Names, are always defined per channel, valid channels are 0 - 15
//-------------------------------------------------------------------------------------------------------
	
	// Host -> Plug
	virtual long getMidiProgramName (long channel, MidiProgramName* midiProgramName) { return 0; }
								// Struct will be filled with information for 'thisProgramIndex'.
								// returns number of used programIndexes.
								// If 0 is returned, no MidiProgramNames supported.
	virtual long getCurrentMidiProgram (long channel, MidiProgramName* currentProgram) { return -1; }
								// Struct will be filled with information for the current program.
								// Returns the programIndex of the current program. -1 means not supported.
	virtual long getMidiProgramCategory (long channel, MidiProgramCategory* category) { return 0; }
								// Struct will be filled with information for 'thisCategoryIndex'.
								// returns number of used categoryIndexes. 
								// if 0 is returned, no MidiProgramCategories supported/used.
	virtual bool hasMidiProgramsChanged (long channel) { return false; }
								// Returns true if the MidiProgramNames, MidiKeyNames or 
								// MidiControllerNames had changed on this channel.
	virtual bool getMidiKeyName (long channel, MidiKeyName* keyName) { return false; }
								// Struct will be filled with information for 'thisProgramIndex' and 'thisKeyNumber'
								// if keyName is "" the standard name of the key will be displayed.
								// if false is returned, no MidiKeyNames defined for 'thisProgramIndex'.

	virtual bool beginSetProgram () { return false; } // Called before a program is loaded
	virtual bool endSetProgram () { return false; }   // Called after...

	// Plug -> Host
	virtual bool beginEdit (long index);  // To be called before a setParameterAutomated with mouse move (one per Mouse Down)
	virtual bool endEdit (long index);    // To be called after a setParameterAutomated (on Mouse Up)

	virtual bool openFileSelector (VstFileSelect *ptr); // Open a Host File selector (see aeffectx.h for VstFileSelect definition)

#endif // VST_2_1_EXTENSIONS

//---------------------------------------------------------
#if VST_2_2_EXTENSIONS
	
	bool closeFileSelector (VstFileSelect *ptr);	// Close the Host File selector which was opened by openFileSelector
	bool getChunkFile (void* nativePath);			// Returns in platform format the path of the current chunk (could be called in setChunk ()) (FSSpec on MAC else char*)

#endif // VST_2_2_EXTENSIONS

//---------------------------------------------------------
#if VST_2_3_EXTENSIONS
	
	// Host -> Plug
	virtual long setTotalSampleToProcess (long value) {	return value; } // Called in offline (non RealTime) Process before process is called, indicates how many sample will be processed

	virtual long getNextShellPlugin (char* name) { return 0; }
								// Tthis opcode is only called, if Plugin is of type kPlugCategShell.
								// should return the next plugin's uniqueID.
								// name points to a char buffer of size 64, which is to be filled
       							// with the name of the plugin including the terminating zero.
	virtual long startProcess () { return 0; }	// Called one time before the start of process call
	virtual long stopProcess () { return 0;	}	// Called after the stop of process call

	virtual bool setPanLaw (long type, float val) { return false; }	// Set the Panning Law used by the Host
	
	virtual long beginLoadBank (VstPatchChunkInfo* ptr) { return 0; }
								// Called before a Bank is loaded.
								// returns -1 if the Bank cannot be loaded, returns 1 if it can be loaded else 0 (for compatibility)
	virtual long beginLoadProgram (VstPatchChunkInfo* ptr) { return 0; }
								// Called before a Program is loaded. (called before beginSetProgram)
								// returns -1 if the Program cannot be loaded, returns 1 if it can be loaded else 0 (for compatibility)

	// Tools
	virtual bool allocateArrangement (VstSpeakerArrangement** arrangement, long nbChannels);
				// Allocate memory for a VstSpeakerArrangement containing the given
				// number of channels
	virtual bool deallocateArrangement (VstSpeakerArrangement** arrangement);
				// Delete/free memory for a speaker arrangement
	virtual bool copySpeaker (VstSpeakerProperties* to, VstSpeakerProperties* from);
				// Feed the "to" speaker properties with the same values than "from"'s ones.
				// It is assumed here that "to" exists yet, ie this function won't
				// allocate memory for the speaker (this will prevent from having
				// a difference between an Arrangement's number of channels and
				// its actual speakers...)
	virtual bool matchArrangement (VstSpeakerArrangement** to, VstSpeakerArrangement* from);
				// "to" is deleted, then created and initialized with the same values as "from" ones ("from" must exist).
#endif // VST_2_3_EXTENSIONS
};

#endif //__audioeffectx__
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
