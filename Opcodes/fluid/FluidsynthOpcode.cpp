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
* fluidload, fluidcontrol, fluidout -- Csound plugin opcode for SoundFonts.
*
* DESCRIPTION
*
* These opcodes provide a simple Csound opcode wrapper around Peter Hanappe's 
* Fluidsynth SoundFont2 synthesizer. This implementation accepts any MIDI note on, 
* note off, controller, pitch bend, or program change message at k-rate.
* Maximum polyphony is 4096 simultaneously sounding voices.
*
* SYNTAX
*
* fluidload sfilename iprogram, ichannel [, olistprograms]
* fluidcontrol kstatus, kchannel, kdata1, data2
* aleft, aright fluidout
*
* PERFORMANCE
*
* aleft - left channel audio output.
*
* aright - right channel audio output.
*
* sfilename - String specifying a SoundFont filename. Note that any number of
* SoundFonts may be loaded (obviously, by different invocations of fluidload).
*
* iprogram - Number of the Fluidsynth program to be assigned to a MIDI channel.
*
* kstatus - MIDI channel message status byte: 128 for note off, 144 for note on, 
* 176 for control change, 192 for program change, or 224 for pitch bend. Note off
* messages need not be specified, as one is automatically generated when each Csound
* note expires or is released.
*
* ichannel, kchannel - MIDI channel number to which the Fluidsynth program is
* assigned: from 0 to 255. MIDI channels numbered 16 or higher are virtual channels.
*
* kdata1 - For note on, MIDI key number: from 0 (lowest) to 127 (highest), 
* where 60 is middle C. For continuous controller messages, controller number.
* 
* kdata2 - For note on, MIDI key velocity: from 0 (no sound) to 127 (loudest).
* For continous controller messages, controller value.
*
* olistprograms - If specified, lists all Fluidsynth programs for the SoundFont.
* A Fluidsynth program is a combination of SoundFont ID, bank number, 
* and preset number that is assigned to a MIDI channel.
* 
* In this implementation, SoundFont effects such as chorus or reverb 
* are used if and only if they are defaults for the preset. 
* There is no means of turning such effects on or off, 
* or of changing their parameters, from Csound.
*
* Invoke fluidload in the orchestra header any number of times. 
* The same SoundFont may be invoked to assign programs to MIDI channels 
* any number of times; the SoundFont is only loaded the first time.
*
* Invoke fluidcontrol in instrument definitions that actually play notes
* and send control messages. Such a definition must consistently use one 
* MIDI channel that was assigned to a Fluidsynth program using fluidload.
*
* Invoke fluidout in an instrument definition numbered higher than any 
* fluidcontrol instrument definitions. All SoundFonts send their output to 
* this single opcode.
*/
#include "Soundfonts.hpp"
#include <OpcodeBase.hpp>
#include <cs.h>

#if defined(WIN32)
#define PUBLIC __declspec(dllexport)
#else
#define PUBLIC
#endif

extern "C"
{
    // A Fluidsynth instance needs to be constructed first (i.e. here),
    // or there are problems with static initialization.
    // Each soundfont has its own effects, so there is no reason for there to 
    // be more than one Fluidsynth per Csound instance.
    fluid_synth_t *dummy = new_fluid_synth(new_fluid_settings());
    fluid_synth_t *Fluidsynth = 0;
    std::map<int, int> programsForChannels;
    std::map<int, int> soundfontIdsForPrograms;
    std::map<std::string, fluid_sfont_t *> soundfontsForNames;
    std::vector<fluid_preset_t> programs;
    
    class FLUIDLOAD : public OpcodeBase<FLUIDLOAD>
    {
    public:
        // Inputs.
        MYFLT *iSoundfontName;
        MYFLT *iFluidProgram;
        MYFLT *iMidiChannel;
        MYFLT *oListPresets;
        // No outputs.
        // State.
        int fluidProgram;
        int midiChannel;
        bool listPresets;
        int soundfontId;
        int init()
        {
            fluidProgram = (int) (*iFluidProgram);                  
            midiChannel = (int) (*iMidiChannel);   
            listPresets = (bool) (*oListPresets); 
            soundfontId = -1;
            if(dummy) {
                delete_fluid_synth(dummy);
                dummy = 0;
            }
            if(!Fluidsynth) {
                fluid_settings_t *fluidSettings = new_fluid_settings();
                Fluidsynth = new_fluid_synth(fluidSettings);
                float samplingRate = (float) cs()->GetSr(cs());
                fluid_settings_setnum(fluidSettings, "synth.sample-rate", samplingRate);
                fluid_settings_setint(fluidSettings, "synth.polyphony", 4096);
                fluid_settings_setint(fluidSettings, "synth.midi-channels", 256);
                log("Allocated Fluidsynth with sampling rate = %f.\n",
                    samplingRate);
            }
            std::string filename = STRARG;
            fluid_sfont_t *fluidSoundfont = 0;
            if(soundfontsForNames.find(filename) == soundfontsForNames.end()) {
                soundfontId = fluid_synth_sfload(Fluidsynth, filename.c_str(), false);
                if(soundfontId == -1)
                {
                    log("Failed to load SoundFont %s.\n", filename.c_str());
                }
                fluidSoundfont = fluid_synth_get_sfont_by_id(Fluidsynth, soundfontId );
                log("Loaded SoundFont '%s' id %d.\n", 
                    filename.c_str(),
                    soundfontId);
                soundfontsForNames[filename] = fluidSoundfont;
                fluid_preset_t fluidPreset;
                fluidSoundfont->iteration_start(fluidSoundfont);
                char buffer[0xff];
                int programIndex = soundfontIdsForPrograms.size();
                while(fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset)) {
                    soundfontIdsForPrograms[programIndex] = soundfontId;
                    programs.push_back(fluidPreset);
                    if(listPresets) {
                        sprintf(buffer, 
                            "Program:%4d  SoundFont:%8d  Bank:%3d  Preset:%3d  %s\n",
                            programIndex++,
                            soundfontId,
                            fluidPreset.get_banknum(&fluidPreset),
                            fluidPreset.get_num(&fluidPreset),
                            fluidPreset.get_name(&fluidPreset));
                            log(buffer);
                    }
    #if defined(WIN32) && defined(__DEBUG__)
                    OutputDebugString(buffer);
    #endif
                }
            }  
            if(fluidProgram >= 0) {
                if(programsForChannels.find(midiChannel) == programsForChannels.end())
                {
                    fluid_preset_t &fluidPreset = programs[fluidProgram];
                    fluid_synth_program_select(Fluidsynth, 
                        midiChannel,
                        soundfontIdsForPrograms[fluidProgram],
                        fluidPreset.get_banknum(&fluidPreset),
                        fluidPreset.get_num(&fluidPreset));
                    programsForChannels[midiChannel] = fluidProgram;
                    unsigned int sfontId = 0;
                    unsigned int bank_num = 0;
                    unsigned int preset_num = 0;
                    int status = fluid_synth_get_program(Fluidsynth, 
                       midiChannel,
                       &sfontId,
                       &bank_num,
                       &preset_num);
                    if(!status) {
                        log("Assigned program %d (SoundFont %d bank %d preset %d) to channel %d.\n",             
                            fluidProgram,
                            sfontId,
                            bank_num,
                            preset_num,
                            midiChannel);
                    } else {
                        log("Failed to assign program %d (SoundFont %d bank %d preset %d) to channel %d.\nStatus: %d error: '%s'\n",             
                            fluidProgram,
                            sfontId,
                            bank_num,
                            preset_num,
                            midiChannel,
                            status,
                            fluid_synth_error(Fluidsynth));
                    }
                }
            }
            return OK;
        }
        int deinit()
        {
            if(Fluidsynth) {
                delete_fluid_synth(Fluidsynth);
                Fluidsynth = 0;
            }
            if(!soundfontsForNames.empty()) {
                soundfontsForNames.clear();
            }
            if(!programsForChannels.empty()) {
                programsForChannels.clear();
            }
            if(!programs.empty()) {
                programs.clear();
            }
            if(!soundfontIdsForPrograms.empty()) {
                soundfontIdsForPrograms.clear();
            }
            warn("Fluidsynth cleanup.\n");
            return OK;
        }
    };

    class FLUIDCONTROL : public OpcodeBase<FLUIDCONTROL>
    {
    public:
        // Inputs.
        MYFLT *kMidiStatus;
        MYFLT *kMidiChannel;
        MYFLT *kMidiData1;
        MYFLT *kMidiData2;
        // No outputs.
        // Internal state.
        bool released;
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
        int init()
        {
            released          = false;
            iMidiStatus       = 0xf0 & (int) (*kMidiStatus);                      
            iMidiChannel      = (int) (*kMidiChannel);                      
            iMidiData1        = (int) (*kMidiData1);                      
            iMidiData2        = (int) (*kMidiData2);                      
            priorMidiStatus   = -1;
            priorMidiChannel  = -1;
            priorMidiData1    = -1;
            priorMidiData2    = -1;
            return OK;
        }
        int kontrol()
        {
            midiStatus       = 0xf0 & (int) (*kMidiStatus);                      
            midiChannel      = (int) (*kMidiChannel);                      
            midiData1        = (int) (*kMidiData1);                      
            midiData2        = (int) (*kMidiData2);                      
            if( midiStatus   != priorMidiStatus  ||
                midiChannel  != priorMidiChannel ||
                midiData1    != priorMidiData1   ||
                midiData2    != priorMidiData2)
            {
                switch(midiStatus)
                {
                    case (int) 0x80:
                        fluid_synth_noteoff(Fluidsynth, 
                            midiChannel, 
                            midiData1); 
                        warn("Note off: s:%d c:%d k:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1);                         
                        break;
                    case (int) 0x90:
                        fluid_synth_noteon(Fluidsynth, 
                            midiChannel, 
                            midiData1, 
                            midiData2); 
                        warn("Note on: s:%d c:%d k:%d v:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1,
                            midiData2);                         
                        break;
                    case (int) 0xa0:
                        warn("Key pressure (not handled): s:%d c:%d k:%d v:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1,
                            midiData2);                         
                        break;
                    case (int) 0xb0:
                        fluid_synth_cc(Fluidsynth, 
                            midiChannel, 
                            midiData1, 
                            midiData2);
                        warn("Control change: s:%d c:%d c:%d v:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1,
                            midiData2);                         
                        break;
                    case (int) 0xc0:
                        fluid_synth_program_change(Fluidsynth, 
                            midiChannel, 
                            midiData1); 
                        warn("Program change: s:%d c:%d p:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1);                         
                        break;
                    case (int) 0xd0:
                        warn("After touch (not handled): s:%d c:%d k:%d v:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1,
                            midiData2);                         
                        break;
                    case (int) 0xe0:
                        fluid_synth_pitch_bend(Fluidsynth, 
                            midiChannel, 
                            midiData1); 
                        warn("Pitch bend: s:%d c:%d b:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1);                         
                        break;
                    case (int) 0xf0:
                        warn("System exclusive (not handled): c:%d k:%d v:%d\n",
                            midiStatus,
                            midiChannel,
                            midiData1,
                            midiData2);                         
                        break;
                }
            }
            if((!released) &&
              (h.insdshead->offtim <= cs()->GetScoreTime(cs()) ||
                h.insdshead->relesing)) {
                released = true;
                fluid_synth_noteoff(Fluidsynth, 
                    iMidiChannel, 
                    iMidiData1); 
                warn("Note off: s:%d c:%d k:%d v:%d\n",
                    iMidiStatus,
                    iMidiChannel,
                    iMidiData1,
                    iMidiData2);                         
            }
            priorMidiStatus  = midiStatus;
            priorMidiChannel = midiChannel;
            priorMidiData1   = midiData1;
            priorMidiData2   = midiData2;
            return OK;
        }
    };
    
    class FLUIDOUT : public OpcodeBase<FLUIDOUT>
    {
    public:
        // No inputs.
        // Outputs.
        MYFLT *aLeftOut;
        MYFLT *aRightOut;
        int audio()
        {
            float leftSample[1];
            float rightSample[1];
            MYFLT *leftOut    = aLeftOut;
            MYFLT *rightOut   = aRightOut;
            for(int i = 0, n = cs()->GetKsmps(cs()); i < n; ++i) {
                leftSample[0] = 0;
                rightSample[0] = 0;
                fluid_synth_write_float(Fluidsynth, 
                    1, 
                    leftSample, 
                    0, 
                    1, 
                    rightSample, 
                    0, 
                    1);    
                leftOut[i]  = leftSample[0];
                rightOut[i] = rightSample[0];
            }
            return OK;
        }
    };

    OENTRY oentries[] = 
    {
        {"fluidload",    sizeof(FLUIDLOAD),    1, "",   "Siio", &FLUIDLOAD::init_,    0,                       0,                 &FLUIDLOAD::deinit_},
        {"fluidcontrol", sizeof(FLUIDCONTROL), 3, "",   "kkkk", &FLUIDCONTROL::init_, &FLUIDCONTROL::kontrol_, 0,                 0                  },
        {"fluidout",     sizeof(FLUIDOUT),     4, "aa", "",     0,                    0,                       &FLUIDOUT::audio_, 0                  },
    };
    
    /**
    * Called by Csound to obtain the size of
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC int opcode_size()
    {
        return sizeof(oentries);
    }

    /**
    * Called by Csound to obtain a pointer to
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC OENTRY *opcode_init(ENVIRON *csound)
    {
        return oentries;
    }
};
