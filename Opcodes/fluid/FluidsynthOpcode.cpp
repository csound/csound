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
*
* C S O U N D   M A N   P A G E
*
* fluid -- Csound plugin opcode for SoundFonts.
*
* DESCRIPTION
*
* fluid is a simple Csound opcode wrapper around Peter Hanappe's 
* fluidsynth SoundFont2 synthesizer. This implementation accepts any MIDI note on, 
* note off, controller, pitch bend, or program change message at k-rate.
* Maximum polyphony is 4096 simultaneously sounding voices.
*
* SYNTAX
* 
* aleft, aright fluid sfilename, iprogram, kstatus, kchannel, kkey, kvelocity 
*               [, olistprograms]
*
* PERFORMANCE
*
* aleft - left channel audio output.
*
* aright - right channel audio output.
*
* sfilename - String specifying a SoundFont filename. Note that any number of
* SoundFonts may be loaded (obviously, by different invocations of fluid).
*
* iprogram - Number of the fluidsynth program to be assigned to a MIDI channel.
*
* kstatus - MIDI channel message status byte: 128 for note off, 144 for note on, 
* 176 for control change, 192 for program change, or 224 for pitch bend. Note off
* messages need not be specified, as one is automatically generated when each Csound
* note expires or is released.
*
* kchannel - MIDI channel number to which the fluidsynth program is assigned: 
* from 0 to 255. MIDI channels numbered 16 or higher are virtual channels.
*
* kkey - MIDI key number: from 0 (lowest) to 127 (highest), where 60 is middle C.
*
* kvelocity - MIDI key velocity: from 0 (no sound) to 127 (loudest).
*
* olistprograms - If specified, lists all fluidsynth programs for the SoundFont.
* A fluidsynth program is a combination of SoundFont ID, bank number, 
* and preset number that can be assigned to a MIDI channel.
* 
* In this implementation, SoundFont effects such as chorus or reverb 
* are used if and only if they are defaults for the preset. 
* There is no means of turning such effects on or off, 
* or of changing their parameters, from Csound.
*/
#include "Soundfonts.hpp"
#include <cs.h>

#if defined(WIN32)
#define PUBLIC __declspec(dllexport)
#else
#define PUBLIC
#endif

extern "C"
{
    // A fluidsynth instance needs to be constructed first (i.e. here),
    // or there are problems with static initialization.
    fluid_synth_t *dummy = new_fluid_synth(new_fluid_settings());
    fluid_synth_t *fluidSynth = 0;
    std::map<int, int> programsForChannels;
    std::map<int, int> soundfontIdsForPrograms;
    std::map<std::string, fluid_sfont_t *> soundfontsForNames;
    std::vector<fluid_preset_t> programs;
    
    struct FluidsynthOpcode
    {
        OPDS h;
        // Outputs.
        MYFLT *aLeftOut;
        MYFLT *aRightOut;
        // Inputs.
        MYFLT *iSoundfontName;
        MYFLT *iFluidProgram;
        MYFLT *kMidiStatus;
        MYFLT *kMidiChannel;
        MYFLT *kMidiData1;
        MYFLT *kMidiData2;
        MYFLT *oListPresets;
        // Internal state.
        ENVIRON *csound;
        bool released;
        int blockSize;
        int soundfontId;
        int iMidiStatus;
        int iMidiChannel;
        int iMidiData1;
        int iMidiData2;
        int midiStatus;
        int midiChannel;
        int midiData1;
        int midiData2;
        int priorMidiStatus;
        int priorMidiChannel;
        int priorMidiData1;
        int priorMidiData2;
    };

    /**
    * Called by Csound to begin processing an "i" statement
    * or MIDI "note on" event.
    */
    int fluidIopadr(void *data)
    {
        FluidsynthOpcode *fluid = (FluidsynthOpcode *)data;
        fluid->csound           = fluid->h.insdshead->csound;
        fluid->iMidiStatus      = 0xf0 & (int) (*fluid->kMidiStatus);                      
        fluid->iMidiChannel     = (int) (*fluid->kMidiChannel);                      
        fluid->iMidiData1       = (int) (*fluid->kMidiData1);                      
        fluid->iMidiData2       = (int) (*fluid->kMidiData2);                      
        fluid->priorMidiStatus  = -1;
        fluid->priorMidiChannel = -1;
        fluid->priorMidiData1   = -1;
        fluid->priorMidiData2   = -1;
        fluid->midiStatus       = fluid->iMidiStatus;                      
        fluid->midiChannel      = fluid->iMidiChannel;                      
        fluid->midiData1        = fluid->iMidiData1;                      
        fluid->midiData2        = fluid->iMidiData2;                      
        fluid->released         = false;
        fluid->soundfontId = -1;
        if(dummy)
        {
            delete_fluid_synth(dummy);
            dummy = 0;
        }
        if(!fluidSynth)
        {
            fluid_settings_t *fluidSettings = new_fluid_settings();
            fluidSynth = new_fluid_synth(fluidSettings);
            float samplingRate = (float) fluid->csound->GetSr(fluid->csound);
            fluid_settings_setnum(fluidSettings, "synth.sample-rate", samplingRate);
            fluid_settings_setint(fluidSettings, "synth.polyphony", 4096);
            fluid_settings_setint(fluidSettings, "synth.midi-channels", 256);
            fluid->csound->Message(fluid->csound, 
                "Allocated fluidsynth with sampling rate = %f.\n",
                samplingRate);
            // delete_fluid_settings(fluidSettings);
        }
        fluid->blockSize = fluid->csound->GetKsmps(fluid->csound);
        fluid->csound->Message(fluid->csound, 
            "SoundFont ksmps = %d.\n",
            fluid->blockSize);
        std::string filename = fluid->STRARG;
        fluid_sfont_t *fluidSoundfont = 0;
        if(soundfontsForNames.find(filename) == soundfontsForNames.end())
        {
            fluid->soundfontId = fluid_synth_sfload(fluidSynth, filename.c_str(), false);
            if(fluid->soundfontId == -1)
            {
                fluid->csound->Message(fluid->csound, "Failed to load SoundFont %s.\n", filename.c_str());
            }
            fluidSoundfont = fluid_synth_get_sfont_by_id(fluidSynth, fluid->soundfontId );
            fluid->csound->Message(fluid->csound, 
                "Loaded SoundFont '%s' id %d.\n", 
                filename.c_str(),
                fluid->soundfontId);
            soundfontsForNames[filename] = fluidSoundfont;
            fluid_preset_t fluidPreset;
            fluidSoundfont->iteration_start(fluidSoundfont);
            char buffer[0xff];
            int programIndex = soundfontIdsForPrograms.size();
            while(fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
            {
                soundfontIdsForPrograms[programIndex] = fluid->soundfontId;
                programs.push_back(fluidPreset);
                if(*fluid->oListPresets)
                {
                    sprintf(buffer, 
                        "Program:%4d  SoundFont:%8d  Bank:%3d  Preset:%3d  %s\n",
                        programIndex++,
                        fluid->soundfontId,
                        fluidPreset.get_banknum(&fluidPreset),
                        fluidPreset.get_num(&fluidPreset),
                        fluidPreset.get_name(&fluidPreset));
                        fluid->csound->Message(fluid->csound, buffer);
                }
#if defined(WIN32) && defined(__DEBUG__)
                OutputDebugString(buffer);
#endif
            }
        }  
        if(*fluid->iFluidProgram >= 0)
        {
            if(programsForChannels.find(fluid->midiChannel) == programsForChannels.end())
            {
                fluid_preset_t &fluidPreset = programs[(int) (*fluid->iFluidProgram)];
                fluid_synth_program_select(fluidSynth, 
                    fluid->midiChannel,
                    soundfontIdsForPrograms[(int) (*fluid->iFluidProgram)],
                    fluidPreset.get_banknum(&fluidPreset),
                    fluidPreset.get_num(&fluidPreset));
                programsForChannels[fluid->midiChannel] = (int) (*fluid->iFluidProgram);
                unsigned int sfontId = 0;
                unsigned int bank_num = 0;
                unsigned int preset_num = 0;
                int status = fluid_synth_get_program(fluidSynth, 
                   fluid->midiChannel,
                   &sfontId,
                   &bank_num,
                   &preset_num);
                if(!status)
                {
                    fluid->csound->Message(fluid->csound, 
                        "Assigned program %d (SoundFont %d bank %d preset %d) to channel %d.\n",             
                        (int) *fluid->iFluidProgram,
                        sfontId,
                        bank_num,
                        preset_num,
                        fluid->midiChannel);
                }
                else
                {
                    fluid->csound->Message(fluid->csound, 
                        "Failed to assign program %d (SoundFont %d bank %d preset %d) to channel %d.\nStatus: %d error: '%s'\n",             
                        (int) *fluid->iFluidProgram,
                        sfontId,
                        bank_num,
                        preset_num,
                        fluid->midiChannel,
                        status,
                        fluid_synth_error(fluidSynth));
                }
            }
        }
        return 0;
    }
    
    /**
    * Called by Csound to process a block of audio samples
    * that is ksmps frames long.
    */
    int fluidAopadr(void *data)
    {
        FluidsynthOpcode *fluid =  (FluidsynthOpcode *)data;
        fluid->midiStatus       = 0xf0 & (int) (*fluid->kMidiStatus);                      
        fluid->midiChannel      = (int) (*fluid->kMidiChannel);                      
        fluid->midiData1        = (int) (*fluid->kMidiData1);                      
        fluid->midiData2        = (int) (*fluid->kMidiData2);                      
        if( fluid->midiStatus   != fluid->priorMidiStatus  ||
            fluid->midiChannel  != fluid->priorMidiChannel ||
            fluid->midiData1    != fluid->priorMidiData1   ||
            fluid->midiData2    != fluid->priorMidiData2)
        {
            switch(fluid->midiStatus)
            {
                // Note off.
                case (int) 0x80:
                    fluid_synth_noteoff(fluidSynth, 
                        fluid->midiChannel, 
                        fluid->midiData1); 
                    fluid->csound->Message(fluid->csound, 
                        "Note off: s:%d c:%d k:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1);                         
                    break;
                // Note on.
                case (int) 0x90:
                    fluid_synth_noteon(fluidSynth, 
                        fluid->midiChannel, 
                        fluid->midiData1, 
                        fluid->midiData2); 
                    fluid->csound->Message(fluid->csound, 
                        "Note on: s:%d c:%d k:%d v:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1,
                        fluid->midiData2);                         
                    break;
                // Key pressure.
                case (int) 0xa0:
                    fluid->csound->Message(fluid->csound, 
                        "Key pressure (not handled): s:%d c:%d k:%d v:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1,
                        fluid->midiData2);                         
                    break;
                // Control change.
                case (int) 0xb0:
                    fluid_synth_cc(fluidSynth, 
                        fluid->midiChannel, 
                        fluid->midiData1, 
                        fluid->midiData2);
                    fluid->csound->Message(fluid->csound, 
                        "Control change: s:%d c:%d c:%d v:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1,
                        fluid->midiData2);                         
                    break;
                // Program change.
                case (int) 0xc0:
                    fluid_synth_program_change(fluidSynth, 
                        fluid->midiChannel, 
                        fluid->midiData1); 
                    fluid->csound->Message(fluid->csound, 
                        "Program change: s:%d c:%d p:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1);                         
                    break;
                // After touch.
                case (int) 0xd0:
                    fluid->csound->Message(fluid->csound, 
                        "After touch (not handled): s:%d c:%d k:%d v:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1,
                        fluid->midiData2);                         
                    break;
                // Pitch bend.
                case (int) 0xe0:
                    fluid_synth_pitch_bend(fluidSynth, 
                        fluid->midiChannel, 
                        fluid->midiData1); 
                    fluid->csound->Message(fluid->csound, 
                        "Pitch bend: s:%d c:%d b:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1);                         
                    break;
                // System exclusive.
                case (int) 0xf0:
                    fluid->csound->Message(fluid->csound, 
                        "System exclusive (not handled) c:%d k:%d v:%d\n",
                        fluid->midiStatus,
                        fluid->midiChannel,
                        fluid->midiData1,
                        fluid->midiData2);                         
                    break;
            }
        }
        // There is really only 1 fluidsynth no matter how many instances are active,
        // so we wait for the last active instance to write audio.
        float leftSample[1];
        float rightSample[1];
        MYFLT *leftOut    = fluid->aLeftOut;
        MYFLT *rightOut   = fluid->aRightOut;
        if(!fluid->h.insdshead->nxtact)
        {
            for(int i = 0; i < fluid->blockSize; ++i)
            {
                leftSample[0] = 0;
                rightSample[0] = 0;
                fluid_synth_write_float(fluidSynth, 
                    1, 
                    leftSample, 
                    0, 
                    1, 
                    rightSample, 
                    0, 
                    1);    
                *leftOut++  = leftSample[0];
                *rightOut++ = rightSample[0];
//                fluid->csound->Message(fluid->csound, 
//                    "%4d L %f R %f AL %f AR %f.\n", 
//                    i,
//                    leftSample,
//                    rightSample,
//                    fluid->aLeftOut[i], 
//                    fluid->aRightOut[i]);
            }
//            fluid->csound->Message(fluid->csound, 
//                "insdshead 0x%x insdshead->prvact 0x%x score time %f active %d\n",
//                fluid->h.insdshead,
//                fluid->h.insdshead->prvact,
//                fluid->csound->GetScoreTime(fluid->csound),
//                fluid->h.insdshead->actflg);
        }
        else
        {
            for(int i = 0; i < fluid->blockSize; ++i)
            {
                *leftOut++  = (MYFLT) 0.0;
                *rightOut++ = (MYFLT) 0.0;
            }
        }
        if((!fluid->released) &&
           (fluid->h.insdshead->offtim <= fluid->csound->GetScoreTime(fluid->csound) ||
            fluid->h.insdshead->relesing))
        {
            fluid->released = true;
            fluid_synth_noteoff(fluidSynth, 
                fluid->iMidiChannel, 
                fluid->iMidiData1); 
            fluid->csound->Message(fluid->csound, 
                "Released s:%d c:%d k:%d v:%d\n",
                fluid->iMidiStatus,
                fluid->iMidiChannel,
                fluid->iMidiData1,
                fluid->iMidiData2);                         
        }
        fluid->priorMidiStatus  = fluid->midiStatus;
        fluid->priorMidiChannel = fluid->midiChannel;
        fluid->priorMidiData1   = fluid->midiData1;
        fluid->priorMidiData2   = fluid->midiData2;
        return 0;
    }

    /**
    * Called by Csound to de-initialize the opcode 
    * just before destroying it.
    */
    int fluidDopadr(void *data)
    {
        FluidsynthOpcode *fluid = (FluidsynthOpcode *)data;
        if(fluidSynth)
        {
            delete_fluid_synth(fluidSynth);
            fluidSynth = 0;
        }
        if(!soundfontsForNames.empty())
        {
            soundfontsForNames.clear();
        }
        if(!programsForChannels.empty())
        {
            programsForChannels.clear();
        }
        if(!programs.empty())
        {
            programs.clear();
        }
        if(!soundfontIdsForPrograms.empty())
        {
            soundfontIdsForPrograms.clear();
        }
        fluid->csound->Message(fluid->csound, "fluid cleanup.\n");
        return 0;
    }

    OENTRY fluidOentry = 
    {
        "fluid",
        sizeof(FluidsynthOpcode),
        5,
        "aa",
        "Sikkkko",
        &fluidIopadr,
        0,
        &fluidAopadr,
        &fluidDopadr
    };
    
    /**
    * Called by Csound to obtain the size of
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC int opcode_size()
    {
        return sizeof(OENTRY);
    }

    /**
    * Called by Csound to obtain a pointer to
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC OENTRY *opcode_init(ENVIRON *csound)
    {
        return &fluidOentry;
    }
};
