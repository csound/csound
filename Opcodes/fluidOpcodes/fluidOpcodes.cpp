/**
 * FLUID SYNTH OPCODES
 *
 * Adapts Fluidsynth to use global engines, soundFonts, and outputs
 *
 * Based on work by Michael Gogins and Steven Yi.  License is identical to
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "OpcodeBase.hpp"
#include "csdl.h"
#include <fluidsynth.h>
#include <fluidsynth/version.h>

static std::vector<fluid_synth_t *> &fluidsynths_for_ids() {
    static std::vector<fluid_synth_t *> fluidsynths_for_ids_;
    return fluidsynths_for_ids_;
}

static void tof(fluid_synth_t *f, MYFLT *a) {
    auto &instances = fluidsynths_for_ids();
    instances.push_back(f);
    *a = instances.size() - 1;
};

static void toa(MYFLT *f, fluid_synth_t *&a) {
    auto &instances = fluidsynths_for_ids();
    int index = *f;
    a = instances[index];
};

class FluidEngine : public csound::OpcodeBase<FluidEngine> {
  // Outputs.
  MYFLT *iFluidSynth;
  // Inputs.
  MYFLT *iChorusEnabled;
  MYFLT *iReverbEnabled;
  MYFLT *iChannelCount;
  MYFLT *iVoiceCount;
  // State.
  // fluid_synth_t *fluidSynth;
  // fluid_settings_t *fluidSettings;
  int32_t chorusEnabled;
  int32_t reverbEnabled;
  int32_t channelCount;
  int32_t voiceCount;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    int32_t result = OK;
    fluid_synth_t *fluidSynth = 0;
    fluid_settings_t *fluidSettings = 0;
    chorusEnabled = (int32_t)*iChorusEnabled;
    reverbEnabled = (int32_t)*iReverbEnabled;
    channelCount = (int32_t)*iChannelCount;
    voiceCount = (int32_t)*iVoiceCount;
    if (channelCount <= 0) {
      channelCount = 256;
    } else if (channelCount < 16) {
      channelCount = 16;
    } else if (channelCount > 256) {
      channelCount = 256;
    }
    if (voiceCount <= 0) {
      voiceCount = 4096;
    } else if (voiceCount < 16) {
      voiceCount = 16;
    } else if (voiceCount > 4096) {
      voiceCount = 4096;
    }
    fluidSettings = new_fluid_settings();
    if (fluidSettings != NULL) {
      fluid_settings_setnum(fluidSettings, (char *)"synth.sample-rate",
                            (double)csound->GetSr(csound));
      fluid_settings_setint(fluidSettings, (char *)"synth.midi-channels",
                            channelCount);
      fluid_settings_setint(fluidSettings, (char *)"synth.polyphony",
                            voiceCount);
      fluidSynth = new_fluid_synth(fluidSettings);
    }
    if (!fluidSynth) {
      if (fluidSettings) {
        delete_fluid_settings(fluidSettings);
      }
      result =
          csound->InitError(csound, "%s", Str("error allocating fluid engine\n"));
    } else {
#if (FLUIDSYNTH_VERSION_MAJOR >= 2)
      // TODO: Change -1 to a configurable FX group?
      fluid_synth_chorus_on(fluidSynth, -1, chorusEnabled);
      fluid_synth_reverb_on(fluidSynth, -1, reverbEnabled);
#else
      fluid_synth_set_chorus_on(fluidSynth, chorusEnabled);
      fluid_synth_set_reverb_on(fluidSynth, reverbEnabled);
#endif
      log(csound, "Created fluidEngine 0x%p with sampling rate = %f, "
                  "chorus %s, reverb %s, channels %d, voices %d.\n",
          fluidSynth, (double)csound->GetSr(csound),
          chorusEnabled ? "on" : "off", reverbEnabled ? "on" : "off",
          channelCount, voiceCount);
      tof(fluidSynth, iFluidSynth);
      {
        void *fluid_synths_mutex = 0;
        csound::QueryGlobalPointer(csound, "fluid_synths_mutex",
                                   fluid_synths_mutex);
        csound::LockGuard synthsGuard(csound, fluid_synths_mutex);
        std::vector<fluid_synth_t *> *fluid_synths = 0;
        csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
        fluid_synths->push_back(fluidSynth);
      }
    }
    return result;
  }
};

class FluidLoad : public csound::OpcodeBase<FluidLoad> {
  // Outputs.
  MYFLT *iInstrumentNumber;
  // Inputs.
  MYFLT *iFilename;
  MYFLT *iFluidSynth;
  MYFLT *iListPresets;
  // State.
  char *filename;
  char *filepath;
  fluid_synth_t *fluidSynth;
  int32_t soundFontId;
  int32_t listPresets;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    int32_t result = OK;
    soundFontId = -1;
    toa(iFluidSynth, fluidSynth);
    listPresets = (int32_t)*iListPresets;
    CS_TYPE *argType = csound->GetTypeForArg(iFilename);
    if (strcmp("S", argType->varTypeName) == 0) {
      filename = csound->Strdup(csound, ((STRINGDAT *)iFilename)->data);
    } else
      filename = csound->strarg2name(
          csound, (char *)NULL,
          (std::isnan(*iFilename) ? csound->GetString(csound, *iFilename)
                                  : (char *)iFilename),
          (char *)"fluid.sf2.", std::isnan(*iFilename));

    filepath = csound->FindInputFile(csound, filename, "SFDIR;SSDIR");
    if (filepath && fluid_is_soundfont(filepath)) {
      log(csound, "Loading SoundFont: %s.\n", filepath);
      soundFontId = fluid_synth_sfload(fluidSynth, filepath, 0);
      log(csound, "fluidSynth: 0x%p  soundFontId: %d.\n", fluidSynth,
          soundFontId);
    }
    *iInstrumentNumber = (MYFLT)soundFontId;
    if (UNLIKELY(soundFontId < 0)) {
      return csound->InitError(csound, Str("fluid: unable to load %s"),
                               filename);
    }
    csound->NotifyFileOpened(csound, filepath, CSFTYPE_SOUNDFONT, 0, 0);
    if (soundFontId < 0) {
      result = NOTOK;
    } else if (listPresets) {
      fluid_sfont_t *fluidSoundfont =
          fluid_synth_get_sfont_by_id(fluidSynth, soundFontId);
#if FLUIDSYNTH_VERSION_MAJOR < 2
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 0x7)
        while (fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
        {
          log(csound, "SoundFont: %3d  Bank: %3d  Preset: %3d  %s\n",
              soundFontId, fluidPreset.get_banknum(&fluidPreset),
              fluidPreset.get_num(&fluidPreset),
              fluidPreset.get_name(&fluidPreset));
#else
      fluid_preset_t *fluidPreset;
      fluid_sfont_iteration_start(fluidSoundfont);
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 0x7)
        while ((fluidPreset = fluid_sfont_iteration_next(fluidSoundfont)))
        {
          log(csound, "SoundFont: %3d  Bank: %3d  Preset: %3d  %s\n",
              soundFontId, fluid_preset_get_banknum(fluidPreset),
              fluid_preset_get_num(fluidPreset),
              fluid_preset_get_name(fluidPreset));
#endif
        }
    }
    return result;
  }
};

#include "arrays.h"

//Rory Walsh 2018
class FluidInfo : public csound::OpcodeBase<FluidInfo> {
  // Outputs.
  ARRAYDAT *outArr;
  // Inputs.
  MYFLT *iFluidSynth;
  // State.
  fluid_synth_t *fluidSynth;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
      std::vector<std::string> programs;
      char *program;
      mutex = csound->Create_Mutex(0);
      csound::LockGuard guard(csound, mutex);
      int32_t result = OK;
      toa(iFluidSynth, fluidSynth);
      fluid_sfont_t *fluidSoundfont = fluid_synth_get_sfont(fluidSynth, 0);
#if FLUIDSYNTH_VERSION_MAJOR < 2
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 0x7)
        while (fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
        {
            std::stringstream ss;
            ss << "Bank: " << fluidPreset.get_banknum(&fluidPreset) <<
              " Preset: " << fluidPreset.get_num(&fluidPreset) <<
                " Name: " << fluidPreset.get_name(&fluidPreset);
#else
      fluid_preset_t *fluidPreset;
      fluid_sfont_iteration_start(fluidSoundfont);
      OPARMS oparms;
      csound->GetOParms(csound, &oparms);
      if (oparms.msglevel & 0x7)
        while ((fluidPreset = fluid_sfont_iteration_next(fluidSoundfont)))
        {
            std::stringstream ss;
            ss << "Bank: " << fluid_preset_get_banknum(fluidPreset) <<
              " Preset: " << fluid_preset_get_num(fluidPreset) <<
                " Name: " << fluid_preset_get_name(fluidPreset);
#endif
          programs.push_back(ss.str());
        }

      tabinit(csound, outArr, programs.size());
      STRINGDAT *strings = (STRINGDAT *)outArr->data;

    for (unsigned int i = 0; i < programs.size(); i++) {
        program = &programs[i][0u];
        strings[i].size = strlen(program) + 1;
        strings[i].data = csound->Strdup(csound, program);
    }

    programs.clear();

    return result;
  }
};

class FluidProgramSelect : public csound::OpcodeBase<FluidProgramSelect>
{
        // Inputs.
        MYFLT *iFluidSynth;
        MYFLT *iChannelNumber;
        MYFLT *iInstrumentNumber;
        MYFLT *iBankNumber;
        MYFLT *iPresetNumber;
        // State.
        fluid_synth_t *fluidSynth;
        int32_t channel;
        uint32_t instrument;
        uint32_t bank;
        uint32_t preset;
        void *mutex;
public:
        int32_t init(CSOUND *csound)
        {
                mutex = csound->Create_Mutex(0);
                csound::LockGuard guard(csound, mutex);
                toa(iFluidSynth, fluidSynth);
                channel = (int32_t) *iChannelNumber;
                instrument = (uint32_t) *iInstrumentNumber;
                bank = (uint32_t) *iBankNumber;
                preset = (uint32_t) *iPresetNumber;
                fluid_synth_program_select(fluidSynth,
                                           channel,
                                           instrument,
                                           bank,
                                           preset);
                return OK;
        }
};

class FluidCCI : public csound::OpcodeBase<FluidCCI> {
  // Inputs.
  MYFLT *iFluidSynth;
  MYFLT *iChannelNumber;
  MYFLT *iControllerNumber;
  MYFLT *kVal;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t controller;
  int32_t value;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    controller = (int32_t)*iControllerNumber;
    value = (int32_t)*kVal;
    fluid_synth_cc(fluidSynth, channel, controller, value);
    return OK;
  }
};

class FluidCCK : public csound::OpcodeBase<FluidCCK> {
  // Inputs.
  MYFLT *iFluidSynth;
  MYFLT *iChannelNumber;
  MYFLT *iControllerNumber;
  MYFLT *kVal;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t controller;
  int32_t value;
  int32_t priorValue;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    priorValue = -1;
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    value = (int32_t)*kVal;
    if (value != priorValue) {
      priorValue = value;
      channel = (int32_t)*iChannelNumber;
      controller = (int32_t)*iControllerNumber;
      fluid_synth_cc(fluidSynth, channel, controller, value);
    }
    return OK;
  }
};

class FluidNote : public csound::OpcodeNoteoffBase<FluidNote> {
  // Inputs.
  MYFLT *iFluidSynth;
  MYFLT *iChannelNumber;
  MYFLT *iMidiKeyNumber;
  MYFLT *iVelocity;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t key;
  int32_t velocity;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    key = (int32_t)*iMidiKeyNumber;
    velocity = (int32_t)*iVelocity;
    fluid_synth_noteon(fluidSynth, channel, key, velocity);
    return OK;
  }
  int32_t noteoff(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    fluid_synth_noteoff(fluidSynth, channel, key);
    return OK;
  }
};

class FluidOut : public csound::OpcodeBase<FluidOut> {
  // Outputs.
  MYFLT *aLeftOut;
  MYFLT *aRightOut;
  // Inputs.
  MYFLT *iFluidSynth;
  // State.
  fluid_synth_t *fluidSynth;
  float leftSample;
  float rightSample;
  int32_t frame;
  int32_t ksmps;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    ksmps = opds.insdshead->ksmps;
    return OK;
  }
  int32_t audio(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early = opds.insdshead->ksmps_no_end;
    if (UNLIKELY(offset)) {
      memset(aLeftOut, '\0', offset * sizeof(MYFLT));
      memset(aRightOut, '\0', offset * sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      ksmps -= early;
      memset(&aLeftOut[ksmps], '\0', early * sizeof(MYFLT));
      memset(&aRightOut[ksmps], '\0', early * sizeof(MYFLT));
    }
    for (frame = offset; frame < ksmps; frame++) {
      leftSample = 0.0f;
      rightSample = 0.0f;
      fluid_synth_write_float(fluidSynth, 1, &leftSample, 0, 1, &rightSample, 0,
                              1);
      aLeftOut[frame] = leftSample /* * csound->e0dbfs */;
      aRightOut[frame] = rightSample /* * csound->e0dbfs */;
    }
    return OK;
  }
};

class FluidAllOut : public csound::OpcodeBase<FluidAllOut> {
  // Outputs.
  MYFLT *aLeftOut;
  MYFLT *aRightOut;
  // State.
  float leftSample;
  float rightSample;
  int32_t frame;
  int32_t ksmps;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    ksmps = opds.insdshead->ksmps;
    return OK;
  }
  int32_t audio(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early = opds.insdshead->ksmps_no_end;
    if (UNLIKELY(offset)) {
      memset(aLeftOut, '\0', offset * sizeof(MYFLT));
      memset(aRightOut, '\0', offset * sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      ksmps -= early;
      memset(&aLeftOut[ksmps], '\0', early * sizeof(MYFLT));
      memset(&aRightOut[ksmps], '\0', early * sizeof(MYFLT));
    }
    std::vector<fluid_synth_t *> *fluid_synths = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
    void *fluid_synths_mutex = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths_mutex",
                               fluid_synths_mutex);
    csound::LockGuard synthsGuard(csound, fluid_synths_mutex);
    for (frame = offset; frame < ksmps; frame++) {
      aLeftOut[frame] = FL(0.0);
      aRightOut[frame] = FL(0.0);
      for (size_t i = 0, n = fluid_synths->size(); i < n; i++) {
        fluid_synth_t *fluidSynth = (*fluid_synths)[i];
        leftSample = FL(0.0);
        rightSample = FL(0.0);
        fluid_synth_write_float(fluidSynth, 1, &leftSample, 0, 1, &rightSample,
                                0, 1);
        aLeftOut[frame] += (MYFLT)leftSample /* * csound->e0dbfs */;
        aRightOut[frame] += (MYFLT)rightSample /* * csound->e0dbfs */;
      }
    }
    return OK;
  }
};

class FluidControl : public csound::OpcodeBase<FluidControl> {
  // Inputs.
  MYFLT *iFluidSynth;
  MYFLT *kMidiStatus;
  MYFLT *kMidiChannel;
  MYFLT *kMidiData1;
  MYFLT *kMidiData2;
  MYFLT *imsgs;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t midiStatus;
  int32_t midiChannel;
  int32_t midiData1;
  int32_t midiData2;
  int32_t priorMidiStatus;
  int32_t priorMidiChannel;
  int32_t priorMidiData1;
  int32_t priorMidiData2;
  int32_t printMsgs;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    priorMidiStatus = -1;
    priorMidiChannel = -1;
    priorMidiData1 = -1;
    priorMidiData2 = -1;
    OPARMS oparms;
    csound->GetOParms(csound, &oparms);
    printMsgs = (*imsgs == 0 ? 0 : 1);
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
   csound::LockGuard guard(csound, mutex);
    midiStatus = 0xF0 & (int32_t)*kMidiStatus;
    midiChannel = (int32_t)*kMidiChannel;
    midiData1 = (int32_t)*kMidiData1;
    midiData2 = (int32_t)*kMidiData2;
    int32_t result = -1;

    if (midiData2 != priorMidiData2 || midiData1 != priorMidiData1 ||
        midiChannel != priorMidiChannel || midiStatus != priorMidiStatus) {
      switch (midiStatus) {
      case (int32_t)0x80:
      noteOff:
        result = fluid_synth_noteoff(fluidSynth, midiChannel, midiData1);
        if (printMsgs)
          csound->Message(csound, Str("result: %d\n Note off: c:%3d k:%3d\n"),
                          result, midiChannel, midiData1);
        break;
      case (int32_t)0x90:
        if (!midiData2) {
          goto noteOff;
        }
        result =
            fluid_synth_noteon(fluidSynth, midiChannel, midiData1, midiData2);
        if (printMsgs)
          log(csound, "result: %d\nNote on: c:%3d k:%3d v:%3d\n", result,
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xA0:
        if (printMsgs)
          log(csound, "Key pressure (not handled): "
                      "c:%3d k:%3d v:%3d\n",
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xB0:
        result = fluid_synth_cc(fluidSynth, midiChannel, midiData1, midiData2);
        if (printMsgs)
          log(csound, "Result: %d Control change: c:%3d c:%3d v:%3d\n", result,
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xC0:
        result = fluid_synth_program_change(fluidSynth, midiChannel, midiData1);
        if (printMsgs)
          log(csound, "Result: %d Program change: c:%3d p:%3d\n", result,
              midiChannel, midiData1);
        break;
      case (int32_t)0xD0:
        if (printMsgs)
          log(csound, "After touch (not handled): c:%3d v:%3d\n", midiChannel,
              midiData1);
        break;
      case (int32_t)0xE0: {
        int32_t pbVal = midiData1 + (midiData2 << 7);
        fluid_synth_pitch_bend(fluidSynth, midiChannel, pbVal);
        if (printMsgs)
          log(csound, "Result: %d, Pitch bend:     c:%d b:%d\n", result,
              midiChannel, pbVal);
      } break;
      case (int32_t)0xF0:
        if (printMsgs)
          log(csound, "System exclusive (not handled): "
                      "c:%3d v1:%3d v2:%3d\n",
              midiChannel, midiData1, midiData2);
        break;
      }
      priorMidiStatus = midiStatus;
      priorMidiChannel = midiChannel;
      priorMidiData1 = midiData1;
      priorMidiData2 = midiData2;
    }
    return OK;
  }
};

class FluidSetInterpMethod : public csound::OpcodeBase<FluidSetInterpMethod> {
  // Inputs.
  MYFLT *iFluidSynth;
  MYFLT *iChannelNumber;
  MYFLT *iInterpMethod;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t interpolationMethod;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    interpolationMethod = (int32_t)*iInterpMethod;
    if (UNLIKELY(interpolationMethod != 0 && interpolationMethod != 1 &&
                 interpolationMethod != 4 && interpolationMethod != 7)) {
      return csound->InitError(csound, "%s",
                               Str("Illegal Interpolation Method: Must be "
                                   "either 0, 1, 4, or 7.\n"));
    } else {
      fluid_synth_set_interp_method(fluidSynth, channel, interpolationMethod);
    }
    return OK;
  }
};

static OENTRY localops[] = {
    {(char *)"fluidEngine", sizeof(FluidEngine), 0, 1, (char *)"i",
     (char *)"ppoo", (SUBR)&FluidEngine::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidLoad", sizeof(FluidLoad), 0, 1, (char *)"i", (char *)"Tio",
     (SUBR)&FluidLoad::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidInfo", sizeof(FluidInfo), 0, 1, (char *)"S[]", (char *)"i",
     (SUBR)&FluidInfo::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidProgramSelect", sizeof(FluidProgramSelect), 0, 1, (char *)"",
     (char *)"iiiii", (SUBR)&FluidProgramSelect::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidCCi", sizeof(FluidCCI), 0, 1, (char *)"", (char *)"iiii",
     (SUBR)&FluidCCI::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidCCk", sizeof(FluidCCK), 0, 3, (char *)"", (char *)"ikkk",
     (SUBR)&FluidCCK::init_, (SUBR)&FluidCCK::kontrol_, (SUBR)0},
    {(char *)"fluidNote", sizeof(FluidNote), 0, 3, (char *)"", (char *)"iiii",
     (SUBR)&FluidNote::init_, (SUBR)&FluidNote::kontrol_, (SUBR)0},
    {(char *)"fluidOut", sizeof(FluidOut), 0, 3, (char *)"aa", (char *)"i",
     (SUBR)&FluidOut::init_, (SUBR)&FluidOut::audio_},
    {(char *)"fluidAllOut", sizeof(FluidAllOut), 0, 3, (char *)"aa", (char *)"",
     (SUBR)&FluidAllOut::init_, (SUBR)&FluidAllOut::audio_},
    {(char *)"fluidControl", sizeof(FluidControl), 0, 3, (char *)"",
     (char *)"ikkkkp", (SUBR)FluidControl::init_, (SUBR)FluidControl::kontrol_,
     (SUBR)0},
    {(char *)"fluidSetInterpMethod", sizeof(FluidSetInterpMethod), 0, 1,
     (char *)"", (char *)"iii", (SUBR)&FluidSetInterpMethod::init_, (SUBR)0,
     (SUBR)0},
    {0, 0, 0, 0, 0, 0, (SUBR)0, (SUBR)0, (SUBR)0}};

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  std::vector<fluid_synth_t *> *fluid_synths =
      new std::vector<fluid_synth_t *>();
  int32_t result = 0;
  result = csound::CreateGlobalPointer(csound, "fluid_synths", fluid_synths);
  void *fluid_synths_mutex = csound->Create_Mutex(0);
  result = csound::CreateGlobalPointer(csound, "fluid_synths_mutex",
                                       fluid_synths_mutex);
  return result;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  // printf("csoundModuleInit: %p\n", csound);
  OENTRY *ep;
  int32_t err = 0;

  for (ep = (OENTRY *)&(localops[0]); ep->opname != NULL; ep++) {
    err |= csound->AppendOpcode(csound, ep->opname, ep->dsblksiz, ep->flags,
                                ep->thread, ep->outypes, ep->intypes,
                                (int32_t (*)(CSOUND *, void *))ep->iopadr,
                                (int32_t (*)(CSOUND *, void *))ep->kopadr,
                                (int32_t (*)(CSOUND *, void *))ep->aopadr);
  }
  return err;
}

/**
 * Called by Csound to de-initialize the opcode
 * just before destroying it.
 */
PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  // printf("csoundModuleDestroy: %p\n", csound);
  void *fluid_synths_mutex = 0;
  csound::QueryGlobalPointer(csound, "fluid_synths_mutex", fluid_synths_mutex);
  if (fluid_synths_mutex) {
    std::vector<fluid_synth_t *> *fluid_synths = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
    csound->LockMutex(fluid_synths_mutex);
    if (fluid_synths) {
      for (size_t i = 0, n = fluid_synths->size(); i < n; i++) {
        fluid_synth_t *fluidSynth = (*fluid_synths)[i];
        // printf("deleting engine: %p\n", fluidSynth);
        fluid_settings_t *fluidSettings = fluid_synth_get_settings(fluidSynth);
        delete_fluid_synth(fluidSynth);
        delete_fluid_settings(fluidSettings);
      }
      fluid_synths->clear();
      delete fluid_synths;
      fluid_synths = 0;
    }
    csound->UnlockMutex(fluid_synths_mutex);
    csound->DestroyMutex(fluid_synths_mutex);
    fluid_synths_mutex = 0;
  }
  return 0;
}

PUBLIC int32_t csoundModuleInfo(void) {
  return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int32_t)sizeof(MYFLT));
}
