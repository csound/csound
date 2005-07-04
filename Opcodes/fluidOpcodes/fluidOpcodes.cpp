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
 * fluid_cc              - send midi controller data to fluid
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
 * iEngineNumber     fluidEngine
 *
 * iInstrumentNumber fluidLoad      sfilename, iEngineNumber
 *
 *                   fluidProgramSelect    iEngineNumber, iChannelNumber,
 *                                           iInstrumentNumber, iBankNumber,
 *                                           iPresetNumber
 *
 *                   fluidCC        iEngineNumber, iChannelNumber,
 *                                  iControllerNumber, kValue
 *
 *                   fluidNote      iEngineNumber, iInstrumentNumber,
 *                                  iMidiKeyNumber, iVelocity
 *
 * aLeft, aRight     fluidOut       iEngineNum
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
#include <OpcodeBase.hpp>
#include <csdl.h>
#include <string>
#include <vector>
#include <stdlib.h>

#ifdef MAKEDLL
#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
#define PUBLIC
#define DIR_SEP '/'
#endif

extern "C"
{

  std::vector<fluid_synth_t *> fluid_engines;

  /* FLUID_ENGINE */

  /**
   * Creates a fluidEngine and returns a MYFLT to user as identifier for
   * engine
   */
  int fluidEngineIopadr(ENVIRON *csound, void *data)
  {
    FLUIDENGINE *fluid = (FLUIDENGINE *)data;
    fluid_synth_t *fluidSynth = 0;
    fluid_settings_t *fluidSettings = new_fluid_settings();
    fluidSynth = new_fluid_synth(fluidSettings);
    float samplingRate_ = (float) csound->GetSr(csound);
    fluid_settings_setnum(fluidSettings, "synth.sample-rate", samplingRate_);
    fluid_settings_setint(fluidSettings, "synth.polyphony", 4096);
    fluid_settings_setint(fluidSettings, "synth.midi-channels", 256);
    csound->Message(csound,
                    "Allocated fluidsynth with sampling rate = %f.\n",
                    samplingRate_);
    fluid_engines.push_back(fluidSynth);
    csound->Message(csound,
                    "Created Fluid Engine - Number : %d.\n",
                    fluid_engines.size() - 1);
    *fluid->iEngineNum = (MYFLT)(fluid_engines.size() - 1);
    return OK;
  }

  /* FLUID_LOAD */

  /**
   * Loads a Soundfont into a Fluid Engine
   */
  int fluidLoadIopadr(ENVIRON *csound, void *data)
  {
    FLUIDLOAD *fluid = (FLUIDLOAD *)data;
    std::string filename = (char*) fluid->filename;
    int engineNum = (int)(*fluid->iEngineNum);
    if(engineNum > int(fluid_engines.size()) || engineNum < 0) {
      csound->Message(csound, "Illegal Engine Number: %i.\n", engineNum);
      return NOTOK;
    }
    char ssdirPath[256];
    sprintf(ssdirPath, "%s%c%s",
            csound->GetEnv(csound, "SSDIR"), DIR_SEP, (char*) fluid->filename);
    char sfdirPath[256];
    sprintf(sfdirPath, "%s%c%s",
            csound->GetEnv(csound, "SFDIR"), DIR_SEP, (char*) fluid->filename);
    /*      printf("SADIRPATH: %s\n", ssdirPath);
            printf("SFDIRPATH: %s\n", sfdirPath);
            printf("SEPARATOR: %c\n", DIR_SEP); */
    int sfontId = 0;
    if (fluid_is_soundfont((char*) fluid->filename)) {
      sfontId = fluid_synth_sfload(fluid_engines[engineNum],
                                   filename.c_str(), false);
      csound->Message(csound,
                      "Loading SoundFont : %s.\n", filename.c_str());
      *fluid->iInstrumentNumber = (MYFLT)(sfontId);
    } else if(fluid_is_soundfont(ssdirPath)) {
      sfontId = fluid_synth_sfload(fluid_engines[engineNum], ssdirPath, false);
      csound->Message(csound,
                      "Loading SoundFont : %s.\n", ssdirPath);
      *fluid->iInstrumentNumber = (MYFLT)(sfontId);
    } else if(fluid_is_soundfont(sfdirPath)) {
      sfontId = fluid_synth_sfload(fluid_engines[engineNum], sfdirPath, false);
      csound->Message(csound,
                      "Loading SoundFont : %s.\n", sfdirPath);
      *fluid->iInstrumentNumber = (MYFLT)(sfontId);
    } else {
      csound->Message(csound,
                      "[ERROR] - Unable to load soundfont\n");
      return OK;
    }
    if(*fluid->iListPresets) {
      fluid_sfont_t *fluidSoundfont = fluid_synth_get_sfont_by_id(fluid_engines[engineNum], sfontId);
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      char buffer[0xff];
      while(fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset)) {
        sprintf(buffer,
                "SoundFont:%3d  Bank:%3d  Preset:%3d  %s\n",
                sfontId,
                fluidPreset.get_banknum(&fluidPreset),
                fluidPreset.get_num(&fluidPreset),
                fluidPreset.get_name(&fluidPreset));
        csound->Message(csound,
                        buffer);
      }
    }
    return OK;
  }

  /* FLUID_PROGRAM_SELECT */

  int fluidProgramSelectIopadr(ENVIRON *csound, void *data)
  {
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

  /* FLUID_CC */

  int fluidCC_I_Iopadr(ENVIRON *csound, void *data)
  {
    FLUID_CC *fluid  = (FLUID_CC *)data;
    int engineNum               = (int)(*fluid->iEngineNumber);
    int channelNum              = (int)(*fluid->iChannelNumber);
    unsigned int controllerNum  = (unsigned int)(*fluid->iControllerNumber);
    int value                   = (int)(*fluid->kVal);
    fluid_synth_cc(fluid_engines[engineNum],
                   channelNum,
                   controllerNum,
                   value);
    return OK;
  }

  int fluidCC_K_Iopadr(ENVIRON *csound, void *data)
  {
    FLUID_CC *fluid  = (FLUID_CC *)data;
    fluid->priorMidiValue = -1;
    return OK;
  }

  int fluidCC_K_Kopadr(ENVIRON *csound, void *data)
  {
    FLUID_CC *fluid  = (FLUID_CC *)data;
    int engineNum               = (int)(*fluid->iEngineNumber);
    int channelNum              = (int)(*fluid->iChannelNumber);
    unsigned int controllerNum  = (unsigned int)(*fluid->iControllerNumber);
    int value                   = (int)(*fluid->kVal);
    int previousValue           = fluid->priorMidiValue;
    if(value != previousValue) {
      fluid_synth_cc(fluid_engines[engineNum],
                     channelNum,
                     controllerNum,
                     value);
    }
    fluid->priorMidiValue = value;
    return OK;
  }

  /* FLUID_NOTE */

  int fluidNoteTurnoff(void *csound_, void *data) {

    //              ENVIRON *csound = (ENVIRON *)csound_;

    FLUID_NOTE *fluid = (FLUID_NOTE *)data;

    int engineNum   = (int)(*fluid->iEngineNumber);
    int channelNum  = (int)(*fluid->iChannelNumber);
    int key         = (int)(*fluid->iMidiKeyNumber);

    //csound->Message(csound, "Fluid Note Off: key %i\n", key);

    fluid_synth_noteoff(fluid_engines[engineNum],
                        channelNum,
                        key);
    fluid->initDone = 0;
    return OK;
  }

  int fluidNoteIopadr(ENVIRON *csound, void *data)
  {
    FLUID_NOTE *fluid = (FLUID_NOTE *)data;
    int engineNum   = (int)(*fluid->iEngineNumber);
    int channelNum  = (int)(*fluid->iChannelNumber);
    int key         = (int)(*fluid->iMidiKeyNumber);
    int velocity    = (int)(*fluid->iVelocity);
    fluid->released = false;
    // fluid->h.insdshead->csound->Message(fluid->h.insdshead->csound, "%i : %i : %i : %i\n", engineNum, instrNum, key, velocity);
    if (fluid->initDone)
      fluidNoteTurnoff((void*) csound, data);
    else
      csound->RegisterDeinitCallback((void *)&csound, (void *)&fluid->h,
                                     &fluidNoteTurnoff);
    fluid_synth_noteon(fluid_engines[engineNum], channelNum, key, velocity);
    //MYFLT offTime = fluid->h.insdshead->p3;
    //unsigned int dur = (int)(offTime *
    //fluid->evt                = new_fluid_event();
    //fluid_event_note(fluid->evt, channelNum, key, vel,

    fluid->initDone = 1;
    return OK;
  }

  //  int fluidNoteKopadr(ENVIRON *csound, void *data)
  //  {
  //    FLUID_NOTE *fluid = (FLUID_NOTE *)data;
  //    int engineNum   = (int)(*fluid->iEngineNumber);
  //    int channelNum  = (int)(*fluid->iChannelNumber);
  //    int key         = (int)(*fluid->iMidiKeyNumber);
  //    MYFLT scoreTime = csound->GetScoreTime(csound);
  //    MYFLT offTime = fluid->h.insdshead->offtim;
  //    //int kSmps = csound->GetKsmps(csound);
  //    //int sRate = (int)csound->GetSr(csound);
  //    //csound->Message(csound, "Times score:%f off:%f\n", scoreTime, offTime);
  //    if(!fluid->released &&
  //       ( offTime <= scoreTime + .025 || fluid->h.insdshead->relesing)) {
  //      fluid->released = true;
  //      fluid_synth_noteoff(fluid_engines[engineNum],
  //                          channelNum,
  //                          key);
  //      //csound->Message(csound, "Release c:%i k:%i\n", channelNum, key);
  //    }
  //    return OK;
  //  }

  /* FLUID_OUT */

  int fluidOutIopadr(ENVIRON *csound, void *data)
  {
    FLUIDOUT *fluid = (FLUIDOUT *)data;
    fluid->blockSize = csound->GetKsmps(csound);
    return OK;
  }

  int fluidOutAopadr(ENVIRON *csound, void *data)
  {
    FLUIDOUT *fluid = (FLUIDOUT *)data;
    float leftSample[1];
    float rightSample[1];
    MYFLT *leftOut    = fluid->aLeftOut;
    MYFLT *rightOut   = fluid->aRightOut;
    int engineNum = (int)(*fluid->iEngineNum);
    if(engineNum > int(fluid_engines.size()) || engineNum < 0) {
      csound->Message(csound,
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

  int fluidAllOutIopadr(ENVIRON *csound, void *data)
  {
    FLUIDALLOUT *fluid = (FLUIDALLOUT *)data;
    fluid->blockSize = csound->GetKsmps(csound);
    return OK;
  }

  int fluidAllOutAopadr(ENVIRON *csound, void *data)
  {
    FLUIDALLOUT *fluid = (FLUIDALLOUT *)data;
    float leftSample[1];
    float rightSample[1];
    MYFLT *leftOut    = fluid->aLeftOut;
    MYFLT *rightOut   = fluid->aRightOut;
    for(int i = 0; i < fluid->blockSize; ++i)
      {
        for(int j = 0; j < (int) fluid_engines.size(); j++)
          {
            leftSample[0] = 0;
            rightSample[0] = 0;
            fluid_synth_write_float(fluid_engines[j],
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
      }
    return OK;
  }

  class FLUIDCONTROL : public OpcodeBase<FLUIDCONTROL>
  {
  public:
    // Inputs.
    MYFLT *iFluidEngine;
    MYFLT *kMidiStatus;
    MYFLT *kMidiChannel;
    MYFLT *kMidiData1;
    MYFLT *kMidiData2;
    // No outputs.
    // Internal state.
    int fluidId;
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
    int init(ENVIRON *csound)
    {
      fluidId           = (int) (*iFluidEngine);
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
    int kontrol(ENVIRON *csound)
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
              fluid_synth_noteoff(fluid_engines[fluidId],
                                  midiChannel,
                                  midiData1);
              warn(csound, "Note off:   s:%3d c:%3d k:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1);
              break;
            case (int) 0x90:
              fluid_synth_noteon(fluid_engines[fluidId],
                                 midiChannel,
                                 midiData1,
                                 midiData2);
              warn(csound, "Note on:    s:%3d c:%3d k:%3d v:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1,
                   midiData2);
              break;
            case (int) 0xa0:
              warn(csound, "Key pressure (not handled): s:%3d c:%3d k:%3d v:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1,
                   midiData2);
              break;
            case (int) 0xb0:
              fluid_synth_cc(fluid_engines[fluidId],
                             midiChannel,
                             midiData1,
                             midiData2);
              warn(csound, "Control change: s:%3d c:%3d c:%3d v:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1,
                   midiData2);
              break;
            case (int) 0xc0:
              fluid_synth_program_change(fluid_engines[fluidId],
                                         midiChannel,
                                         midiData1);
              warn(csound, "Program change: s:%3d c:%3d p:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1);
              break;
            case (int) 0xd0:
              warn(csound, "After touch (not handled): s:%3d c:%3d k:%3d v:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1,
                   midiData2);
              break;
            case (int) 0xe0:
              fluid_synth_pitch_bend(fluid_engines[fluidId],
                                     midiChannel,
                                     midiData1);
              warn(csound, "Pitch bend: s:%d c:%d b:%d\n",
                   midiStatus,
                   midiChannel,
                   midiData1);
              break;
            case (int) 0xf0:
              warn(csound, "System exclusive (not handled): c:%3d k:%3d v:%3d\n",
                   midiStatus,
                   midiChannel,
                   midiData1,
                   midiData2);
              break;
            }
        }
      if((!released) &&
         (h.insdshead->offtim <= csound->GetScoreTime(csound) + 0.25 ||
          h.insdshead->relesing)) {
        released = true;
        fluid_synth_noteoff(fluid_engines[fluidId],
                            iMidiChannel,
                            iMidiData1);
        warn(csound, "Releasing:  s:%3d c:%3d k:%3d v:%3d\n",
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

  /* OPCODE LIBRARY STUFF */

  static OENTRY localops[] = {
    {
      "fluidEngine",
      sizeof(FLUIDENGINE),
      1,
      "i",
      "",
      (SUBR)&fluidEngineIopadr,
      0,
      0
    },
    {
      "fluidLoad",
      sizeof(FLUIDLOAD),
      1,
      "i",
      "Sio",
      (SUBR)&fluidLoadIopadr,
      0,
      0
    },
    {
      "fluidProgramSelect",
      sizeof(FLUID_PROGRAM_SELECT),
      1,
      "",
      "iiiii",
      (SUBR)&fluidProgramSelectIopadr,
      0,
      0
    },
    {
      "fluidCCi",
      sizeof(FLUID_CC),
      1,
      "",
      "iiii",
      (SUBR)&fluidCC_I_Iopadr,
      0,
      0
    },
    {
      "fluidCCk",
      sizeof(FLUID_CC),
      3,
      "",
      "iiik",
      (SUBR)&fluidCC_K_Iopadr,
      (SUBR)&fluidCC_K_Kopadr,
      0
    },
    {
      "fluidNote",
      sizeof(FLUID_NOTE),
      1,
      "",
      "iiii",
      (SUBR)&fluidNoteIopadr,
      //(SUBR)&fluidNoteKopadr,
      0,
      0
    },
    {
      "fluidOut",
      sizeof(FLUIDOUT),
      5,
      "aa",
      "i",
      (SUBR)&fluidOutIopadr,
      0,
      (SUBR)&fluidOutAopadr
    },
    {
      "fluidAllOut",
      sizeof(FLUIDALLOUT),
      5,
      "aa",
      "i",
      (SUBR)&fluidAllOutIopadr,
      0,
      (SUBR)&fluidAllOutAopadr
    },
    {
      "fluidControl",
      sizeof(FLUIDCONTROL),
      3,
      "",
      "ikkkk",
      &FLUIDCONTROL::init_,
      &FLUIDCONTROL::kontrol_,
      0
    },
    { NULL, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
  };

  PUBLIC int csoundModuleCreate(void *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(void *csound_)
  {
    ENVIRON *csound = (ENVIRON*) csound_;
    OENTRY  *ep = (OENTRY*) &(localops[0]);
    int     err = 0;

    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound, ep->opname, ep->dsblksiz, ep->thread,
                                  ep->outypes, ep->intypes,
                                  (int (*)(void*, void*)) ep->iopadr,
                                  (int (*)(void*, void*)) ep->kopadr,
                                  (int (*)(void*, void*)) ep->aopadr);
      ep++;
    }
    return err;
  }

  /**
   * Called by Csound to de-initialize the opcode
   * just before destroying it.
   */
  PUBLIC int csoundModuleDestroy(void *csound_)
  {
    ENVIRON *csound = (ENVIRON*) csound_;
    if(!fluid_engines.empty()) {
      csound->Message(csound,
                      "Cleaning up Fluid Engines - Found: %d\n",
                      fluid_engines.size());
      for(size_t i = 0; i < fluid_engines.size(); i++) {
        delete_fluid_synth(fluid_engines[i]);
        fluid_engines[i] = 0;
      }
      fluid_engines.clear();
    }
    return 0;
  }

}; // END EXTERN C

