/**
* S O U N D F O N T S   V S T 
*
* Adapts Fluidsynth to be both a VST plugin instrument
* and a Csound plugin opcode.
* Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
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
#ifndef __Soundfonts_H
#define __Soundfonts_H

#include "audioeffectx.h"
#include <fluidsynth.h>
#include <string>
#include <vector>
#include <deque>
#include <map>

struct VstProgram
{
	VstProgram();
    int version;
    char name[0xff];
    char soundfontFilenames[0xf][0xff];
    int soundfontProgramsForChannels[0xf];
};

class Soundfonts : public AudioEffectX
{
protected:
	// Represents the FluidSynth.
    fluid_settings_t *fluidSettings;
    fluid_synth_t *fluidSynth;
    // Temporary representation of a SoundFont.
    fluid_sfont_t *fluidSoundfont;
    // Table of SoundFont programs, that is,
    // all presets in all loaded SoundFonts.
    std::vector<fluid_preset_t> soundfontPrograms;
    // Presets don't contain SoundFont IDs, 
    // so this associates SoundFont IDs with SoundFont program numbers.
    std::map<int, int> soundfontIdsForSoundfontPrograms;
    // Buffer variables for getting and setting parameters.
    float soundfontParameter;
    int soundfontId;
    // Bank of VST programs (chunks).
    std::vector<VstProgram> vstPrograms;
    // Stores incoming MIDI events until they are consumed.
    std::deque<VstMidiEvent> midiEventQueue;
    // Helper functions for loading a SoundFont.
    virtual void loadSoundfont(const char *filename);
    // Helper function for assigning a SoundFont program to a MIDI channel.
    virtual void assignSoundfontProgram(int channel, int soundfontProgram);
    // Helper function for setting the VST program.
    virtual void setVstProgram(const VstProgram &vstProgram);
public:
    enum
    {
        kNumPrograms = 10
    };
    enum
    {
        kNumInputs = 0,
        kNumOutputs = 2
    };
    enum
    {
    	kChannel01Program,
    	kChannel02Program,
    	kChannel03Program,
    	kChannel04Program,
    	kChannel05Program,
    	kChannel06Program,
    	kChannel07Program,
    	kChannel08Program,
    	kChannel09Program,
    	kChannel10Program,
    	kChannel11Program,
    	kChannel12Program,
    	kChannel13Program,
    	kChannel14Program,
    	kChannel15Program,
    	kChannel16Program,
    	kSoundfont,
        kNumParameters
    };
	Soundfonts(audioMasterCallback audioMaster);
	virtual ~Soundfonts();
	virtual bool getEffectName(char* name);
	virtual bool getVendorString(char* name);
	virtual bool getProductString(char* name);
	virtual long canDo(char* text);
	virtual bool getInputProperties(long index, VstPinProperties* properties);
	virtual bool getOutputProperties(long index, VstPinProperties* properties);
	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterLabel(long index, char *label);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterName(long index, char *text);
	virtual long getChunk(void** data, bool isPreset);
	virtual long setChunk(void* data, long byteSize, bool isPreset);
	virtual long getProgram();
	virtual void setProgram(long program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool copyProgram(long destination);
	virtual bool getProgramNameIndexed(long category, long index, char* text);
	virtual void suspend();
	virtual void resume();
	virtual long processEvents(VstEvents *vstEvents);
	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);
};

#endif
