/**
* FLUID SYNTH OPCODES
*
* Adapts Fluidsynth to use global engines, soundFonts, and outputs
*
* Based on work by Michael Gogins.  License is identical to 
* SOUNDFONTS VST License (listed below)
*
* Copyright (c) 2003 by Steven Yi. All rights reserved.
*
* [ORIGINAL INFORMATION BELOW]
*
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
* fluid_engine          - creates a fluid engine
* fluid_load            - loads a soundfont into a fluid engine
* fluid_program_select  - assign a bank and preset of a soundFont to a midi 
*                         channel as well as select
* fluid_play            - plays a note on a channel
* fluid_out             - outputs sound from a fluid engine
*
* DESCRIPTION
*
* This family of opcodes are meant to be used together to load and play 
* SoundFonts using Peter Hannape's Fluidsynth.  
*
* SYNTAX
* 
* iEngineNumber     fluid_engine    
* 
* iInstrumentNumber fluid_load      sfilename, iEngineNumber
* 
*                   fluid_program_select    iEngineNumber, iChannelNumber,
*                                           iInstrumentNumber, iBankNumber,
*                                           iPresetNumber
* 
*                   fluid_play      iEngineNumber, iInstrumentNumber,
*                                   iMidiKeyNumber, iVelocity
* 
* aLeft, aRight     fluid_out       iEngineNum
*
* PERFORMANCE
*
* iEngineNumber - engine number assigned from fluid_engine
* 
* iInstrumentNumber - instrument number assigned from fluid_load
* 
* sfilename - String specifying a SoundFont filename
* 
* aLeft - left channel audio output.
* aRight - right channel audio output.
*
* iMidiKeyNumber - midi key number to play (0-127)
* 
* iVelocity - midi velocity to play at (0-127)
* 
* iBankNum - bank number on soundfont to play
* 
* iPresetNum - preset number on soundfont to play
*
* iprogram - Number of the fluidsynth program to be assigned to a MIDI channel.
* 
* In this implementation, SoundFont effects such as chorus or reverb 
* are used if and only if they are defaults for the preset. 
* There is no means of turning such effects on or off, 
* or of changing their parameters, from Csound.
*/
#include "fluidOpcodes.hpp"
#include <string>
#include <vector>
#include <stdlib.h>

#undef sfdirpath
#undef ssdirpath



#ifdef MAKEDLL
#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
#define PUBLIC
#define DIR_SEP '/'
#endif

#define CENVIRON(p) (p->h.insdshead->csound)

extern "C"
{  
   
    std::vector<fluid_synth_t *> fluid_engines;
  
/* FLUID_ENGINE */
    
    /**
     * Creates a fluidEngine and returns a MYFLT to user as identifier for 
     * engine
     */
    int fluidEngineIopadr(void *data) {
        FLUIDENGINE *fluid = (FLUIDENGINE *)data;
        fluid_synth_t *fluidSynth = 0;
        
        fluid_settings_t *fluidSettings = new_fluid_settings();
        fluidSynth = new_fluid_synth(fluidSettings);
        float samplingRate = (float) fluid->h.insdshead->csound->GetSr(fluid->h.insdshead->csound);
        fluid_settings_setnum(fluidSettings, "synth.sample-rate", samplingRate);
        fluid_settings_setint(fluidSettings, "synth.polyphony", 4096);
        fluid_settings_setint(fluidSettings, "synth.midi-channels", 256);
            
        CENVIRON(fluid)->Message(CENVIRON(fluid), 
            "Allocated fluidsynth with sampling rate = %f.\n",
            samplingRate);            
        
        fluid_engines.push_back(fluidSynth);
                 
        CENVIRON(fluid)->Message(CENVIRON(fluid), 
            "Created Fluid Engine - Number : %d.\n",
            fluid_engines.size() - 1);
            
        *fluid->iEngineNum = (MYFLT)(fluid_engines.size() - 1);
        
        return OK;
    }

    /**
    * Called by Csound to de-initialize the opcode 
    * just before destroying it.
    */
    int fluidEngineDopadr(void *data) {
        FLUIDENGINE *fluid = (FLUIDENGINE *)data;
       
        if(!fluid_engines.empty()) {
            CENVIRON(fluid)->Message(CENVIRON(fluid), 
                "Cleaning up Fluid Engines - Found: %d\n", 
                fluid_engines.size());
            
            for(int i = 0; i < fluid_engines.size(); i++) {               
                delete_fluid_synth(fluid_engines[i]);  
                fluid_engines[i] = 0;
            }
            fluid_engines.clear();   
        }
        return OK;
    } 
    
    
/* FLUID_LOAD */
    
    /**
     * Loads a Soundfont into a Fluid Engine
     */
    int fluidLoadIopadr(void *data) {
        FLUIDLOAD *fluid = (FLUIDLOAD *)data;
        
        std::string filename = fluid->STRARG;        
                      
        int engineNum = (int)(*fluid->iEngineNum);
        
        
        if(engineNum > fluid_engines.size() || engineNum < 0) {
               CENVIRON(fluid)->Message(CENVIRON(fluid), 
                    "Illegal Engine Number: %i.\n", engineNum);
               
               return NOTOK;
        }
        
        
        
        char ssdirPath[256];
        sprintf(ssdirPath,"%s%c%s",getenv("SSDIR"),DIR_SEP,fluid->STRARG);       
        
        char sfdirPath[256];
        sprintf(sfdirPath,"%s%c%s",getenv("SFDIR"),DIR_SEP,fluid->STRARG);          

/*      printf("SADIRPATH: %s\n", ssdirPath);       
        printf("SFDIRPATH: %s\n", sfdirPath);    
        printf("SEPARATOR: %c\n", DIR_SEP); */
        
        if(fluid_is_soundfont(fluid->STRARG)) {
            int sfontId = fluid_synth_sfload(fluid_engines[engineNum], 
                                             filename.c_str(), false);  
                                             
            CENVIRON(fluid)->Message(CENVIRON(fluid), 
                    "Loading SoundFont : %s.\n", filename.c_str());         
                    
            *fluid->iInstrumentNumber = (MYFLT)(sfontId);
        } else if(fluid_is_soundfont(ssdirPath)) {
            int sfontId = fluid_synth_sfload(fluid_engines[engineNum], ssdirPath, false);                           
            
            CENVIRON(fluid)->Message(CENVIRON(fluid), 
                "Loading SoundFont : %s.\n", ssdirPath);    
                     
            *fluid->iInstrumentNumber = (MYFLT)(sfontId);           
        } else if(fluid_is_soundfont(sfdirPath)) {
            int sfontId = fluid_synth_sfload(fluid_engines[engineNum], sfdirPath, false);                                       

            CENVIRON(fluid)->Message(CENVIRON(fluid), 
                "Loading SoundFont : %s.\n", sfdirPath);
                
            *fluid->iInstrumentNumber = (MYFLT)(sfontId);                       
        } else {

            CENVIRON(fluid)->Message(CENVIRON(fluid), 
                "[ERROR] - Unable to load soundfont");
        }
        
        return OK;
    }
    
/* FLUID_PROGRAM_SELECT */

    int fluidProgramSelectIopadr(void *data) {
        FLUID_PROGRAM_SELECT *fluid = (FLUID_PROGRAM_SELECT *)data;
        
        int engineNum               = (int)(*fluid->iEngineNumber);
        int channelNum              = (int)(*fluid->iChannelNumber);
        unsigned int instrumentNum  = (unsigned int)(*fluid->iInstrumentNumber);
        unsigned int bankNum        = (unsigned int)(*fluid->iBankNumber);
        unsigned int presetNum      = (unsigned int)(*fluid->iPresetNumber);
               
        fluid_synth_program_select(fluid_engines[engineNum], 
                                   channelNum, instrumentNum, 
                                   bankNum, presetNum);     
        
        return OK;  
    }    
    
/* FLUID_PLAY */
    
    int fluidPlayIopadr(void *data) {
        FLUIDPLAY *fluid = (FLUIDPLAY *)data;
        
        int engineNum   = (int)(*fluid->iEngineNumber);
        int channelNum  = (int)(*fluid->iChannelNumber);
        int key         = (int)(*fluid->iMidiKeyNumber);
        int velocity    = (int)(*fluid->iVelocity);
       
        fluid->released = false;
        
        // fluid->h.insdshead->csound->Message(fluid->h.insdshead->csound, "%i : %i : %i : %i\n", engineNum, instrNum, key, velocity);
        fluid_synth_noteon(fluid_engines[engineNum], channelNum, key, velocity);
        return OK;
    }
    
    int fluidPlayKopadr(void *data) {
        FLUIDPLAY *fluid = (FLUIDPLAY *)data;
        
        int engineNum   = (int)(*fluid->iEngineNumber);
        int channelNum  = (int)(*fluid->iChannelNumber);
        int key         = (int)(*fluid->iMidiKeyNumber);
               
        MYFLT scoreTime = CENVIRON(fluid)->GetScoreTime(CENVIRON(fluid));
        MYFLT offTime = fluid->h.insdshead->offtim;
        
        int kSmps = CENVIRON(fluid)->GetKsmps(CENVIRON(fluid));      
        int sRate = (int)CENVIRON(fluid)->GetSr(CENVIRON(fluid));
        
        
        if(!fluid->released &&        
             ((int)(scoreTime * sRate) + (20 * kSmps) >= (int)(offTime * sRate) ||
                fluid->h.insdshead->relesing)) {
                
                fluid->released = true;
            
                fluid_synth_noteoff(fluid_engines[engineNum], 
                    channelNum, 
                    key); 
        }
        return OK;
    }
    
/* FLUID_OUT */
    
    int fluidOutIopadr(void *data) {
        FLUIDOUT *fluid = (FLUIDOUT *)data;
        
        fluid->blockSize = CENVIRON(fluid)->GetKsmps(CENVIRON(fluid));
        return OK;
    }

    int fluidOutAopadr(void *data) {
        FLUIDOUT *fluid = (FLUIDOUT *)data;
        
        float leftSample[1];
        float rightSample[1];
        MYFLT *leftOut    = fluid->aLeftOut;
        MYFLT *rightOut   = fluid->aRightOut;
        
        int engineNum = (int)(*fluid->iEngineNum);
        
        if(engineNum > fluid_engines.size() || engineNum < 0) {
               CENVIRON(fluid)->Message(CENVIRON(fluid), 
                    "Illegal Engine Number: %i.\n", engineNum);
               return NOTOK;
        }
        
        
        for(int i = 0; i < fluid->blockSize; ++i)
        {
                leftSample[0] = 0;
                rightSample[0] = 0;
                fluid_synth_write_float(fluid_engines[engineNum], 
                    1, 
                    leftSample, 
                    0, 
                    1, 
                    rightSample, 
                    0, 
                    1);    
                *leftOut++  = leftSample[0];
                *rightOut++ = rightSample[0];
        }

        return OK;
    }    
    
    
/* OPCODE LIBRARY STUFF */
    
    OENTRY fluidOentry[] = { 
        {   "fluid_engine", sizeof(FLUIDENGINE), 1, "i",   "",         &fluidEngineIopadr, 0, 0, &fluidEngineDopadr },
        {   "fluid_load",   sizeof(FLUIDLOAD), 1, "i",   "Si",       &fluidLoadIopadr,   0, 0, 0 },
        {   "fluid_program_select",   sizeof(FLUID_PROGRAM_SELECT), 1, "",   "iiiii",       &fluidProgramSelectIopadr,   0, 0, 0 },
        {   "fluid_play",   sizeof(FLUIDPLAY), 3, "",    "iiii",  &fluidPlayIopadr,   &fluidPlayKopadr, 0, 0 },
        {   "fluid_out",    sizeof(FLUIDOUT), 5, "aa",  "i",        &fluidOutIopadr,    0, &fluidOutAopadr, 0}
    };
    
    /**
    * Called by Csound to obtain the size of
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC int opcode_size()
    {
        return sizeof(OENTRY) * 5;
    }

    /**
    * Called by Csound to obtain a pointer to
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC OENTRY *opcode_init(ENVIRON *csound)
    {
        return fluidOentry;
    }
    
}; // END EXTERN C
