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
#include "Soundfonts.hpp"
#include <FL/Fl_File_Chooser.H>
#include <memory.h>

Soundfonts::Soundfonts(audioMasterCallback audioMaster)	: 
  AudioEffectX(audioMaster, kNumPrograms, kNumParameters),
  fluidSettings(0),
  fluidSynth(0),
  fluidSoundfont(0),
  soundfontParameter(0),
  soundfontId(-1)
{
  setNumInputs(kNumInputs);		// stereo in
  setNumOutputs(kNumOutputs);		// stereo out
  setUniqueID('vsf2');	// identify
  canMono();				// makes sense to feed both inputs with the same signal
  canProcessReplacing();	// supports both accumulating and replacing output
  fluidSettings = new_fluid_settings();
  fluidSynth = new_fluid_synth(fluidSettings);
  isSynth(true);
  wantEvents(true);
  programsAreChunks(true);
  curProgram = 0;
  vstPrograms.resize(kNumPrograms);
  for(size_t i = 0; i < vstPrograms.size(); i++)
    {
      char buffer[0x24];
      sprintf(buffer, "Program%d", i + 1);
      strcpy(vstPrograms[i].name, buffer);
    }
}

Soundfonts::~Soundfonts()
{
  if(fluidSettings)
    {
      delete_fluid_settings(fluidSettings);
      fluidSettings = 0;
    }
  if(fluidSynth)
    {
      delete_fluid_synth(fluidSynth);
      fluidSynth = 0;
    }
}

void Soundfonts::setParameter(long index, float value)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "BEGAN Soundfonts::setParameter(%d, %f)\n", index, value);
  OutputDebugString(buffer);
#endif
  char *filename = 0;
  switch(index)
    {
    case kSoundfont:
      if(value != 0.0f)
	{
	  soundfontParameter = 0.0f;
	  filename = fl_file_chooser("Load a SoundFont file...", "*.sf2", 0, false);
	  if(filename)
	    {
	      loadSoundfont(filename);
	    }
	}
      break;
    case kChannel01Program:
    case kChannel02Program:
    case kChannel03Program:
    case kChannel04Program:
    case kChannel05Program:
    case kChannel06Program:
    case kChannel07Program:
    case kChannel08Program:
    case kChannel09Program:
    case kChannel10Program:
    case kChannel11Program:
    case kChannel12Program:
    case kChannel13Program:
    case kChannel15Program:
    case kChannel16Program:
      if(soundfontPrograms.size() > 0)
	{
	  int soundfontProgram = (int) (value * soundfontPrograms.size());
	  assignSoundfontProgram(index, soundfontProgram);
	}
      break;
    }
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::setParameter\n");
#endif
}

float Soundfonts::getParameter(long index)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "Soundfonts::getParameter(%d)\n", index);
  OutputDebugString(buffer);
#endif
  switch(index)
    {
    case kSoundfont:
      return 0.0f;
      break;
    case kChannel01Program:
    case kChannel02Program:
    case kChannel03Program:
    case kChannel04Program:
    case kChannel05Program:
    case kChannel06Program:
    case kChannel07Program:
    case kChannel08Program:
    case kChannel09Program:
    case kChannel10Program:
    case kChannel11Program:
    case kChannel12Program:
    case kChannel13Program:
    case kChannel15Program:
    case kChannel16Program:
      if(soundfontPrograms.size() > 0)
	{
	  return float(vstPrograms[curProgram].soundfontProgramsForChannels[index]) / float(soundfontPrograms.size());
	}
      else
	{
	  return 0.0f;
	}
      break;
    }
  return 0.0f;
}

void Soundfonts::getParameterDisplay(long index, char *text)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "Soundfonts::getParameterDisplay(%d)\n", index);
  OutputDebugString(buffer);
#endif
  fluid_preset_t &fluidPreset = soundfontPrograms[vstPrograms[curProgram].soundfontProgramsForChannels[index]];
  switch(index)
    {
    case kSoundfont:
      if(fluidSoundfont)
	{
	  sprintf(text, 
		  " %.12s", 
		  fl_filename_name(fluidSoundfont->get_name(fluidSoundfont)));
	}
      else
	{
	  sprintf(text, "None");
	}
      break;
    case kChannel01Program:
    case kChannel02Program:
    case kChannel03Program:
    case kChannel04Program:
    case kChannel05Program:
    case kChannel06Program:
    case kChannel07Program:
    case kChannel08Program:
    case kChannel09Program:
    case kChannel10Program:
    case kChannel11Program:
    case kChannel12Program:
    case kChannel13Program:
    case kChannel14Program:
    case kChannel15Program:
    case kChannel16Program:
      if(soundfontPrograms.size() > 0)
	{
	  int soundfontProgram = vstPrograms[curProgram].soundfontProgramsForChannels[index];
	  fluidPreset = soundfontPrograms[soundfontProgram];
	  sprintf(text, 
		  "%.12s", 
		  fluidPreset.get_name(&fluidPreset));
     	}
      else
     	{
	  sprintf(text, 
		  "None");
	}
      break;
    }
}

void Soundfonts::getParameterName(long index, char *label)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "Soundfonts::getParameterName(%d)\n", index);
  OutputDebugString(buffer);
#endif
  switch(index)
    {
    case kSoundfont:
      strcpy(label, "SoundFont");
      break;
    case kChannel01Program:
    case kChannel02Program:
    case kChannel03Program:
    case kChannel04Program:
    case kChannel05Program:
    case kChannel06Program:
    case kChannel07Program:
    case kChannel08Program:
    case kChannel09Program:
    case kChannel10Program:
    case kChannel11Program:
    case kChannel12Program:
    case kChannel13Program:
    case kChannel14Program:
    case kChannel15Program:
    case kChannel16Program:
      sprintf(label, "Ch %d", int(index + 1));
      break;
    }
}

void Soundfonts::getParameterLabel(long index, char *label)
{
}

long Soundfonts::processEvents(VstEvents *vstEvents)
{
  for(int i = 0; i < vstEvents->numEvents; i++)
    {
      if(vstEvents->events[i]->type == kVstMidiType)
	{
#if defined(WIN32) && defined(__DEBUG__)
	  char buffer[0xff];
	  sprintf(buffer, "Soundfonts::processEvents MIDI event %d\n", i);
	  OutputDebugString(buffer);
#endif
	  const VstMidiEvent *vstMidiEvent = (VstMidiEvent *)vstEvents->events[i];
	  midiEventQueue.push_back(*vstMidiEvent);
	}
    }
  return 1;
}

void Soundfonts::process(float **inputs, float **outputs, long sampleFrames)
{
  double leftSample;
  double rightSample;
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
#endif
  for(long sampleFrame = 0; sampleFrame < sampleFrames; ++sampleFrame)
    {
      while(!midiEventQueue.empty())
        {
	  if(midiEventQueue.front().deltaFrames != sampleFrame)
            {
	      break;
	    }
	  else
	    {
	      const VstMidiEvent &vstMidiEvent = midiEventQueue.front();
	      int status  = vstMidiEvent.midiData[0] & 0xf0;
	      int channel = vstMidiEvent.midiData[0] & 0x0f;
	      int data1   = vstMidiEvent.midiData[1];
	      int data2   = vstMidiEvent.midiData[2];
#if defined(WIN32) && defined(__DEBUG__)
	      sprintf(buffer, "s %d c %d k %d v %d delta %d\n", status, channel, data1, data2, vstMidiEvent.deltaFrames);
	      OutputDebugString(buffer);
#endif
	      switch(status)
                {
		  // Note off.
		case 0x80:
		  fluid_synth_noteoff(fluidSynth, channel, data1); 
		  break;
		  // Note on.
		case 0x90:
		  fluid_synth_noteon(fluidSynth, channel, data1, data2); 
		  break;
		  // Key pressure.
		case 0xa0:
		  break;
		  // Control change.
		case 0xb0:
		  fluid_synth_cc(fluidSynth, channel, data1, data2);
		  break;
		  // Program change.
		case 0xc0:
		  fluid_synth_program_change(fluidSynth, channel, data1);
		  break;
		  // After touch.
		case 0xd0:
		  break;
		  // Pitch bend.
		case 0xe0:
		  fluid_synth_pitch_bend(fluidSynth, channel, data1);
		  break;
		  // System exclusive.
		case 0xf0:
		  break;
                }
	      midiEventQueue.pop_front();
            }
        }
      fluid_synth_write_float(fluidSynth, 1, &leftSample, 0, 1, &rightSample, 0, 1);    
      outputs[0][sampleFrame] += leftSample;
      outputs[1][sampleFrame] += rightSample;
    }
}

void Soundfonts::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
#endif
  for(long sampleFrame = 0; sampleFrame < sampleFrames; ++sampleFrame)
    {
      while(!midiEventQueue.empty())
        {
	  if(midiEventQueue.front().deltaFrames != sampleFrame)
            {
	      break;
	    }
	  else
            {
	      const VstMidiEvent &vstMidiEvent = midiEventQueue.front();
	      int status  = vstMidiEvent.midiData[0] & 0xf0;
	      int channel = vstMidiEvent.midiData[0] & 0x0f;
	      int data1   = vstMidiEvent.midiData[1];
	      int data2   = vstMidiEvent.midiData[2];
#if defined(WIN32) && defined(__DEBUG__)
	      sprintf(buffer, "s %d c %d k %d v %d delta %d\n", status, channel, data1, data2, vstMidiEvent.deltaFrames);
	      OutputDebugString(buffer);
#endif
	      switch(status)
                {
		  // Note off.
		case 0x80:
		  fluid_synth_noteoff(fluidSynth, channel, data1); 
		  break;
		  // Note on.
		case 0x90:
		  fluid_synth_noteon(fluidSynth, channel, data1, data2); 
		  break;
		  // Key pressure.
		case 0xa0:
		  break;
		  // Control change.
		case 0xb0:
		  fluid_synth_cc(fluidSynth, channel, data1, data2);
		  break;
		  // Program change.
		case 0xc0:
		  fluid_synth_program_change(fluidSynth, channel, data1);
		  break;
		  // After touch.
		case 0xd0:
		  break;
		  // Pitch bend.
		case 0xe0:
		  fluid_synth_pitch_bend(fluidSynth, channel, data1);
		  break;
		  // System exclusive.
		case 0xf0:
		  break;
                }
	      midiEventQueue.pop_front();
            }
        }
      fluid_synth_write_float(fluidSynth, 1, outputs[0], sampleFrame, 1, outputs[1], sampleFrame, 1);    
    }
}

bool Soundfonts::getInputProperties(long index, VstPinProperties* properties)
{
  return false;
}

bool Soundfonts::getOutputProperties(long index, VstPinProperties* properties)
{
  if(index < kNumOutputs)
    {
      sprintf(properties->label, "My %1d Out", int(index + 1));
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool Soundfonts::getEffectName(char* name)
{
  strcpy(name, "Soundfonts");
  return true;
}

bool Soundfonts::getVendorString(char* text)
{
  strcpy(text, "Irreducible Productions");
  return true;
}

bool Soundfonts::getProductString(char* text)
{
  strcpy(text, "Soundfonts");
  return true;
}

long Soundfonts::canDo(char* text)
{
  if(strcmp(text, "receiveVstEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "receiveVstMidiEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "asyncProcessing") == 0)
    {
      return 1;
    }
  if(strcmp(text, "0in2out") == 0)
    {
      return 1;
    }
  return 0;
}

void Soundfonts::suspend()
{
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("BEGAN Soundfonts::suspend\n");
#endif
  midiEventQueue.clear();
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::suspend\n");
#endif
}

void Soundfonts::resume()
{
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("BEGAN Soundfonts::resume\n");
#endif
  wantEvents(true);
  fluid_settings_t *fluidSettings_ = fluid_synth_get_settings(fluidSynth);
  float sampleFramesPerSecond = getSampleRate();
  fluid_settings_setnum(fluidSettings_, "synth.sample-rate", sampleFramesPerSecond);
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::resume\n");
#endif
}

long Soundfonts::getProgram()
{
  return curProgram;
}

void Soundfonts::setProgram(long program)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "BEGAN Soundfonts::setProgram(%d)\n", program);
  OutputDebugString(buffer);
#endif
  if(program >= 0 && program < kNumPrograms)
    {
      curProgram = program;
      setVstProgram(vstPrograms[curProgram]);
    }
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::setProgram\n");
#endif
}

void Soundfonts::setProgramName(char *name)
{
  strcpy(vstPrograms[curProgram].name, name);
}

void Soundfonts::getProgramName(char *name)
{
  strcpy(name, vstPrograms[curProgram].name);    
}

bool Soundfonts::copyProgram(long destination)
{
  if(destination >= 0 && destination < kNumPrograms)
    {
      vstPrograms[destination] = vstPrograms[curProgram];
      return true;
    }
  return false;
}

bool Soundfonts::getProgramNameIndexed(long category, long index, char* text)
{
  if(index >= 0 && index < kNumPrograms)
    {
      strcpy(text, vstPrograms[curProgram].name);
      return true;
    }
  return false;
}

long Soundfonts::getChunk(void** data, bool isPreset)
{
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("Soundfonts::getChunk\n");
#endif
  long returnValue = 0;
  if(isPreset)
    {
      *data = &vstPrograms[curProgram];
      returnValue = sizeof(VstProgram);
    }
  else
    {
      *data = &vstPrograms[0];
      returnValue = sizeof(VstProgram) * kNumPrograms;
    }
  return returnValue;
}

long Soundfonts::setChunk(void* data, long byteSize, bool isPreset)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "BEGAN Soundfonts::setChunk(%x, %d, %d)\n", data, byteSize, isPreset);
  OutputDebugString(buffer);
#endif
  long returnValue = 0;
  VstProgram *data_ = (VstProgram *)data;
  if(isPreset)
    {
      memcpy(&vstPrograms[curProgram], data_, sizeof(VstProgram));
    }
  else
    {
      memcpy(&vstPrograms[0], data_, sizeof(VstProgram) * kNumPrograms);
    }
  setProgram(curProgram);
  returnValue = byteSize;
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::setChunk\n");
#endif
  return returnValue;
}

void Soundfonts::setVstProgram(const VstProgram &vstProgram)
{
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("BEGAN Soundfonts::setVstProgram\n");
#endif
  // Turn everything off.
  suspend();
  // Clear program-related internal state.
  fluidSoundfont = 0;
  soundfontId = 0;
  soundfontPrograms.clear();
  soundfontIdsForSoundfontPrograms.clear();
  // Unload all SoundFonts.
  while(fluid_synth_sfcount(fluidSynth))
    {
      fluid_synth_sfunload(fluidSynth, 0, true);
    }
  // Load all programmed SoundFonts.
  for(int i = 0; i < 16; i++)
    {
      if(strlen(vstProgram.soundfontFilenames[i]) > 0)
        {
	  loadSoundfont(vstProgram.soundfontFilenames[i]);
        }
    }
  // Set all channel assignments.
  for(int i = 0; i < 16; i++)
    {
      assignSoundfontProgram(i, vstProgram.soundfontProgramsForChannels[i]);
    }
#if defined(WIN32) && defined(__DEBUG__)
  OutputDebugString("ENDED Soundfonts::setVstProgram\n");
#endif
}

void Soundfonts::loadSoundfont(const char *filename)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "Soundfonts::loadSoundfont(%s)\n", filename);
  OutputDebugString(buffer);
#endif
  fluidSoundfont = 0;
  soundfontId = 0;
  soundfontId = fluid_synth_sfload(fluidSynth, filename, true);
  fluidSoundfont = fluid_synth_get_sfont_by_id(fluidSynth, soundfontId);
  if(fluidSoundfont)
    {
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      while(fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
        {
	  soundfontIdsForSoundfontPrograms[soundfontPrograms.size()] = soundfontId;
	  soundfontPrograms.push_back(fluidPreset);
        }
      for(int i = 0; i < 16; ++i)
        {
	  if(strcmp(vstPrograms[curProgram].soundfontFilenames[i], filename) == 0)
	    {
	      return;
	    }
	  else if(strlen(vstPrograms[curProgram].soundfontFilenames[i]) == 0)
            {
	      strcpy(vstPrograms[curProgram].soundfontFilenames[i], filename);
	      return;
            }
        }
    }
}

void Soundfonts::assignSoundfontProgram(int channel_, int program_)
{
#if defined(WIN32) && defined(__DEBUG__)
  char buffer[0xff];
  sprintf(buffer, "Soundfonts::assignSoundfontProgram(%d, %d)\n", channel_, program_);
  OutputDebugString(buffer);
#endif
  if(channel_ >= 0 && channel_ < 16 && program_ >= 0 && program_ < int(soundfontPrograms.size()))
    {
      fluid_preset_t &fluidPreset = soundfontPrograms[program_];
      fluid_synth_program_select(fluidSynth, 
				 channel_,
				 soundfontIdsForSoundfontPrograms[program_],
				 fluidPreset.get_banknum(&fluidPreset),
				 fluidPreset.get_num(&fluidPreset));
      vstPrograms[curProgram].soundfontProgramsForChannels[channel_] = program_;
    }
}

VstProgram::VstProgram() : version(0)
{
  memset(this, 0, sizeof(VstProgram));
}

