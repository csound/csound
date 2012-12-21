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

#include "csdl.h"
#include "fluidOpcodes.h"
#include "csGblMtx.h"

typedef struct {
  fluid_synth_t   **fluid_engines;
  size_t          cnt;
} fluidSynthGlobals;

static CS_NOINLINE fluidSynthGlobals * fluid_allocGlobals(CSOUND *csound)
{
    fluidSynthGlobals   *p;

    if (csound->CreateGlobalVariable(csound, "fluid.engines",
                                     sizeof(fluidSynthGlobals)) != 0)
      csound->Die(csound, "fluid: error allocating globals\n");
    p = (fluidSynthGlobals *) csound->QueryGlobalVariable(csound,
                                                          "fluid.engines");
    p->fluid_engines = (fluid_synth_t **) NULL;
    p->cnt = (size_t) 0;

    return p;
}

static CS_NOINLINE fluidSynthGlobals * fluid_getGlobals(CSOUND *csound)
{
    fluidSynthGlobals   *p;

    p = (fluidSynthGlobals *) csound->QueryGlobalVariable(csound,
                                                          "fluid.engines");
    if (p == NULL)
      return fluid_allocGlobals(csound);

    return p;
}

/* FLUID_ENGINE */

static int fluidEngine_Alloc(CSOUND *csound, fluid_synth_t *p)
{
    fluidSynthGlobals   *pp = fluid_getGlobals(csound);
    size_t              ndx;

    ndx = pp->cnt;
    pp->cnt++;
    pp->fluid_engines =
      (fluid_synth_t **) csound->ReAlloc(csound, pp->fluid_engines,
                                         sizeof(fluid_synth_t *) * pp->cnt);
    pp->fluid_engines[ndx] = p;

    return (int) ndx;
}

/**
 * Creates a fluidEngine and returns a MYFLT to user as identifier for
 * engine
 */

static int fluidEngineIopadr(CSOUND *csound, FLUIDENGINE *p)
{
    fluid_synth_t     *fluidSynth = NULL;
    fluid_settings_t  *fluidSettings;
    int               ndx;
    int chorusEnabled = (int) *p->iChorusEnabled;
    int reverbEnabled = (int) *p->iReverbEnabled;
    int numChannels = (int) *p->iNumChannels;
    int polyphony = (int) *p->iPolyphony;

    if (numChannels <= 0) {
      numChannels = 256;
    } else if (numChannels < 16) {
      numChannels = 16;
    } else if (numChannels > 256) {
      numChannels = 256;
    }

    if (polyphony <= 0) {
      polyphony = 4096;
    } else if (polyphony < 16) {
      polyphony = 16;
    } else if (polyphony > 4096) {
      polyphony = 4096;
    }

    csound_global_mutex_lock();
    fluidSettings = new_fluid_settings();
    if (fluidSettings != NULL) {
      fluid_settings_setnum(fluidSettings,
                            "synth.sample-rate", (double) csound->esr);
      fluid_settings_setint(fluidSettings,
                            "synth.polyphony", polyphony);
      fluid_settings_setint(fluidSettings,
                            "synth.midi-channels", numChannels);
      fluidSynth = new_fluid_synth(fluidSettings);
    }
    csound_global_mutex_unlock();

    if (fluidSynth == NULL) {
      if (fluidSettings != NULL)
        delete_fluid_settings(fluidSettings);
      return csound->InitError(csound, Str("error allocating fluid engine\n"));
    }
    csound_global_mutex_lock();
    fluid_synth_set_chorus_on(fluidSynth, chorusEnabled);
    fluid_synth_set_reverb_on(fluidSynth, reverbEnabled);
    csound_global_mutex_unlock();

    ndx = fluidEngine_Alloc(csound, fluidSynth);
    csound->Message(csound, Str("Created fluidEngine %d with sampling rate = %f, "
                                "chorus %s, reverb %s, channels %d, "
                                "polyphony %d.\n"),
                    ndx, (double) csound->esr,
                    chorusEnabled ? Str("on") : Str("off"),
                    reverbEnabled ? Str("on") : Str("off"),
                    numChannels,
                    polyphony);
    *(p->iEngineNum) = (MYFLT) ndx;

    return OK;
}

/* FLUID_LOAD */

/**
 * Loads a Soundfont into a Fluid Engine
 */

static int fluidLoadIopadr(CSOUND *csound, FLUIDLOAD *p)
{
    fluidSynthGlobals   *pp = fluid_getGlobals(csound);
    char    *filename, *filename_fullpath;
    int     engineNum = (int) *(p->iEngineNum);
    int     sfontId = -1;

    if (engineNum >= (int) pp->cnt || engineNum < 0) {
      csound->InitError(csound, Str("Illegal Engine Number: %i.\n"), engineNum);
      return NOTOK;
  }
    filename = csound->strarg2name(csound,
                                   (char*) NULL, p->filename, "fluid.sf2.",
                                   (int) csound->GetInputArgSMask(p));
    filename_fullpath = csound->FindInputFile(csound, filename, "SFDIR;SSDIR");
    if (filename_fullpath != NULL && fluid_is_soundfont(filename_fullpath)) {
      csound->Message(csound, Str("Loading SoundFont : %s.\n"), filename_fullpath);
      sfontId = fluid_synth_sfload(pp->fluid_engines[engineNum],
                                   filename_fullpath, 0);
    }
    *(p->iInstrumentNumber) = (MYFLT) sfontId;
    if (sfontId < 0)
      csound->InitError(csound, Str("fluid: unable to load %s"), filename);
    csound->NotifyFileOpened(csound, filename_fullpath, CSFTYPE_SOUNDFONT, 0, 0);
    csound->Free(csound, filename_fullpath);
    csound->Free(csound, filename);
    if (sfontId < 0)
      return NOTOK;

    if (*(p->iListPresets) != FL(0.0)) {
      fluid_sfont_t   *fluidSoundfont;
      fluid_preset_t  fluidPreset;

      fluidSoundfont =
        fluid_synth_get_sfont_by_id(pp->fluid_engines[engineNum], sfontId);

      fluidSoundfont->iteration_start(fluidSoundfont);
      if (csound->oparms->msglevel & 0x7)
        while (fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset)) {
          csound->Message(csound,
                          Str("SoundFont: %3d  Bank: %3d  Preset: %3d  %s\n"),
                          sfontId,
                          fluidPreset.get_banknum(&fluidPreset),
                          fluidPreset.get_num(&fluidPreset),
                          fluidPreset.get_name(&fluidPreset));
        }
    }

    return OK;
}

/* FLUID_PROGRAM_SELECT */

static int fluidProgramSelectIopadr(CSOUND *csound, FLUID_PROGRAM_SELECT *p)
{
    fluidSynthGlobals   *pp             = fluid_getGlobals(csound);
    int                 engineNum       = (int) *(p->iEngineNumber);
    int                 channelNum      = (int) *(p->iChannelNumber);
    int                 instrumentNum   = (int) *(p->iInstrumentNumber);
    int                 bankNum         = (int) *(p->iBankNumber);
    int                 presetNum       = (int) *(p->iPresetNumber);

    fluid_synth_program_select(pp->fluid_engines[engineNum],
                               channelNum,
                               (unsigned int) instrumentNum,
                               (unsigned int) bankNum,
                               (unsigned int) presetNum);

    return OK;
}

/* FLUID_CC */

static int fluidCC_I_Iopadr(CSOUND *csound, FLUID_CC *p)
{
    fluidSynthGlobals   *pp             = fluid_getGlobals(csound);
    int                 engineNum       = (int) *(p->iEngineNumber);
    int                 channelNum      = (int) *(p->iChannelNumber);
    int                 controllerNum   = (int) *(p->iControllerNumber);
    int                 value           = (int) *(p->kVal);

    fluid_synth_cc(pp->fluid_engines[engineNum],
                   channelNum, controllerNum, value);

    return OK;
}

static int fluidCC_K_Iopadr(CSOUND *csound, FLUID_CC *p)
{
    fluidSynthGlobals *pp = fluid_getGlobals(csound);

    p->fluidEngine = pp->fluid_engines[(int) *(p->iEngineNumber)];
    p->priorMidiValue = -1;

    return OK;
}

static int fluidCC_K_Kopadr(CSOUND *csound, FLUID_CC *p)
{
    int   value = (int) *(p->kVal);

    (void) csound;
    if (value != p->priorMidiValue) {
      p->priorMidiValue = value;
      fluid_synth_cc(p->fluidEngine,
                     (int) *(p->iChannelNumber),
                     (int) *(p->iControllerNumber),
                     value);
    }

    return OK;
}

/* FLUID_NOTE */

static int fluidNoteTurnoff(CSOUND *csound, FLUID_NOTE *p)
{
    (void) csound;
    if (p->initDone > 0)
      fluid_synth_noteoff(p->fluidEngine, p->iChn, p->iKey);
    p->initDone = 0;

    return OK;
}

static int fluidNoteIopadr(CSOUND *csound, FLUID_NOTE *p)
{
    fluidSynthGlobals   *pp         = fluid_getGlobals(csound);
    int                 engineNum   = (int) *(p->iEngineNumber);
    int                 channelNum  = (int) *(p->iChannelNumber);
    int                 key         = (int) *(p->iMidiKeyNumber);
    int                 velocity    = (int) *(p->iVelocity);

  /* csound->Message(csound, "%i : %i : %i : %i\n",
     engineNum, instrNum, key, velocity); */
    p->iChn = channelNum;
    p->iKey = key;
    p->fluidEngine = pp->fluid_engines[engineNum];

    if (p->initDone)
      fluidNoteTurnoff(csound, p);
    else
      csound->RegisterDeinitCallback(csound, (void*) p,
                                     (int (*)(CSOUND *, void *))
                                     fluidNoteTurnoff);
    fluid_synth_noteon(p->fluidEngine, channelNum, key, velocity);
    p->initDone = 1;

    return OK;
}

static int fluidNoteKopadr(CSOUND *csound, FLUID_NOTE *p)
{
    if (p->h.insdshead->relesing) {
      if (p->initDone > 0) {
        fluidNoteTurnoff(csound, p);
        p->initDone = -1;
      }
    }
    return OK;
}

/* FLUID_OUT */

static int fluidOutIopadr(CSOUND *csound, FLUIDOUT *p)
{
    fluidSynthGlobals *pp = fluid_getGlobals(csound);
    int   engineNum = (int) *(p->iEngineNum);

    if (engineNum >= (int) pp->cnt || engineNum < 0) {
      csound->InitError(csound, Str("Illegal Engine Number: %i.\n"), engineNum);
      return NOTOK;
    }
    p->fluidEngine = pp->fluid_engines[engineNum];

    return OK;
}

static int fluidOutAopadr(CSOUND *csound, FLUIDOUT *p)
{
    float   leftSample, rightSample;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *al = p->aLeftOut, *ar = p->aRightOut;
    memset(al, '\0', offset*sizeof(MYFLT));
    memset(ar, '\0', offset*sizeof(MYFLT));

    for (n=offset; n<nsmps; n++) {
      leftSample = 0.0f;
      rightSample = 0.0f;
      fluid_synth_write_float(p->fluidEngine, 1,
                              &leftSample, 0, 1, &rightSample, 0, 1);
      p->aLeftOut[n] = (MYFLT)leftSample /* * csound->e0dbfs */;
      p->aRightOut[n] = (MYFLT)rightSample /* * csound->e0dbfs */;
    }

  return OK;
}

static int fluidAllOutIopadr(CSOUND *csound, FLUIDALLOUT *p)
{
    p->fluidGlobals = (void *) fluid_getGlobals(csound);
    return OK;
}

static int fluidAllOutAopadr(CSOUND *csound, FLUIDALLOUT *p)
{
    fluid_synth_t **fluid_engines;
    float   leftSample, rightSample;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    int     j, cnt;
    MYFLT *al = p->aLeftOut, *ar = p->aRightOut;
    memset(al, '\0', offset*sizeof(MYFLT));
    memset(ar, '\0', offset*sizeof(MYFLT));

    cnt = (int) ((fluidSynthGlobals *) p->fluidGlobals)->cnt;
    if (cnt <= 0)
      return OK;
    fluid_engines = ((fluidSynthGlobals *) p->fluidGlobals)->fluid_engines;
    for (n=offset; n<nsmps; n++) {
      MYFLT aal = FL(0.0), aar = FL(0.0);
      for (j=0; j<cnt; j++) {
        leftSample = 0.0f;
        rightSample = 0.0f;
        fluid_synth_write_float(fluid_engines[j], 1,
                                &leftSample, 0, 1, &rightSample, 0, 1);
        aal += (MYFLT) leftSample /* * csound->e0dbfs */;
        aar += (MYFLT) rightSample /* * csound->e0dbfs */;
      }
      al[n] = aal;
      ar[n] = aar;
    }

    return OK;
}

static int fluidControl_init(CSOUND *csound, FLUIDCONTROL *p)
{
    fluidSynthGlobals *pp = fluid_getGlobals(csound);

    p->fluidEngine = pp->fluid_engines[(int) *(p->iFluidEngine)];

    p->priorMidiStatus = -1;
    p->priorMidiChannel = -1;
    p->priorMidiData1 = -1;
    p->priorMidiData2 = -1;

    return OK;
}

static int fluidControl_kontrol(CSOUND *csound, FLUIDCONTROL *p)
{
    int   midiStatus    = 0xF0 & (int) *(p->kMidiStatus);
    int   midiChannel   = *(p->kMidiChannel);
    int   midiData1     = *(p->kMidiData1);
    int   midiData2     = *(p->kMidiData2);
    int res =  -1;

    if (midiData2 != p->priorMidiData2 ||
        midiData1 != p->priorMidiData1 ||
        midiChannel != p->priorMidiChannel ||
        midiStatus != p->priorMidiStatus) {
      fluid_synth_t *fluidEngine = p->fluidEngine;
      int           printMsgs = ((csound->oparms->msglevel & 7) == 7 ? 1 : 0);

      switch (midiStatus) {
      case (int) 0x80:
      noteOff:
        res = fluid_synth_noteoff(fluidEngine, midiChannel, midiData1);
        if (printMsgs)
          csound->Message(csound, Str("result: %d \n Note off: c:%3d k:%3d\n"),res,
                          midiChannel, midiData1);
        break;
      case (int) 0x90:
        if (!midiData2)
          goto noteOff;
        res = fluid_synth_noteon(fluidEngine, midiChannel, midiData1, midiData2);
        if (printMsgs)
          csound->Message(csound, Str("result: %d \nNote on: c:%3d k:%3d v:%3d\n"),
                          res, midiChannel, midiData1, midiData2);
        break;
      case (int) 0xA0:
        if (printMsgs)
          csound->Message(csound, Str("Key pressure (not handled): "
                                      "c:%3d k:%3d v:%3d\n"),
                          midiChannel, midiData1, midiData2);
        break;
      case (int) 0xB0:
        res = fluid_synth_cc(fluidEngine, midiChannel, midiData1, midiData2);
        if (printMsgs)
          csound->Message(csound,
                          Str("Result: %d Control change: c:%3d c:%3d v:%3d\n"),
                          res, midiChannel, midiData1, midiData2);
        break;
      case (int) 0xC0:
        res = fluid_synth_program_change(fluidEngine, midiChannel, midiData1);
        if (printMsgs)
          csound->Message(csound,
                          Str("Result: %d Program change: c:%3d p:%3d\n"),res,
                          midiChannel, midiData1);
        break;
      case (int) 0xD0:
        if (printMsgs)
          csound->Message(csound, "After touch (not handled): c:%3d v:%3d\n",
                          midiChannel, midiData1);
        break;
      case (int) 0xE0:
        {
          int pbVal = midiData1 + (midiData2 << 7);
          fluid_synth_pitch_bend(fluidEngine, midiChannel, pbVal);
          if (printMsgs)
            csound->Message(csound, Str("Result: %d, Pitch bend:     c:%d b:%d\n"),
                            res, midiChannel, pbVal);
        }
        break;
      case (int) 0xF0:
        if (printMsgs)
          csound->Message(csound, Str("System exclusive (not handled): "
                                      "c:%3d v1:%3d v2:%3d\n"),
                          midiChannel, midiData1, midiData2);
        break;
      }
      p->priorMidiStatus = midiStatus;
      p->priorMidiChannel = midiChannel;
      p->priorMidiData1 = midiData1;
      p->priorMidiData2 = midiData2;
    }

    return OK;
}

static int fluidSetInterpMethod(CSOUND *csound, FLUID_SET_INTERP_METHOD *p)
{
    fluidSynthGlobals *pp = fluid_getGlobals(csound);
    int engineNum = (int) *(p->iEngineNumber);
    int channelNum  = (int) *(p->iChannelNumber);
    int interpMethod = (int) *(p->iInterpMethod);

    if (engineNum >= (int) pp->cnt || engineNum < 0)
      {
        csound->InitError(csound, Str("Illegal Engine Number: %i.\n"),
                          engineNum);
        return NOTOK;
      }

    if(interpMethod != 0 && interpMethod != 1 && interpMethod != 4 &&
       interpMethod != 7)
      {
        csound->InitError(csound,
                          Str("Illegal Interpolation Method: Must "
                              "be either 0, 1, 4, or 7.\n"));
        return NOTOK;
      }

    fluid_synth_set_interp_method(
                                  pp->fluid_engines[engineNum], channelNum, interpMethod);

    return OK;
}

/* OPCODE LIBRARY STUFF */

static OENTRY localops[] = {
  { "fluidEngine",          sizeof(FLUIDENGINE),            1,  "i",  "ppoo",
    (SUBR) fluidEngineIopadr,       (SUBR) 0,   (SUBR) 0                  },
  { "fluidLoad",            sizeof(FLUIDLOAD),              1,  "i",  "Tio",
    (SUBR) fluidLoadIopadr,         (SUBR) 0,   (SUBR) 0                  },
  { "fluidProgramSelect",   sizeof(FLUID_PROGRAM_SELECT),   1,  "",   "iiiii",
    (SUBR) fluidProgramSelectIopadr,  (SUBR) 0, (SUBR) 0                  },
  { "fluidCCi",             sizeof(FLUID_CC),               1,  "",   "iiii",
    (SUBR) fluidCC_I_Iopadr,        (SUBR) 0,   (SUBR) 0                  },
  { "fluidCCk",             sizeof(FLUID_CC),               3,  "",   "ikkk",
    (SUBR) fluidCC_K_Iopadr,        (SUBR) fluidCC_K_Kopadr, (SUBR) 0     },
  { "fluidNote",            sizeof(FLUID_NOTE),             3,  "",   "iiii",
    (SUBR) fluidNoteIopadr,         (SUBR) fluidNoteKopadr, (SUBR) 0      },
  { "fluidOut",             sizeof(FLUIDOUT),               5,  "aa", "i",
    (SUBR) fluidOutIopadr,          (SUBR) 0,   (SUBR) fluidOutAopadr     },
  { "fluidAllOut",          sizeof(FLUIDALLOUT),            5,  "aa", "i",
    (SUBR) fluidAllOutIopadr,       (SUBR) 0,   (SUBR) fluidAllOutAopadr  },
  { "fluidControl",         sizeof(FLUIDCONTROL),           3,  "",   "ikkkk",
    (SUBR) fluidControl_init,       (SUBR) fluidControl_kontrol, (SUBR) 0 },
  { "fluidSetInterpMethod", sizeof(FLUID_SET_INTERP_METHOD), 1,  "",   "iii",
    (SUBR) fluidSetInterpMethod, (SUBR) 0, (SUBR) 0 },
  { NULL,                   0,                              0,  NULL, NULL,
    (SUBR) 0,                       (SUBR) 0,   (SUBR) 0                  }
};

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    OENTRY  *ep;
    int     err = 0;

    for (ep = (OENTRY *) &(localops[0]); ep->opname != NULL; ep++) {
      err |= csound->AppendOpcode(csound, ep->opname,
                                  ep->dsblksiz, ep->thread,
                                  ep->outypes, ep->intypes,
                                  (int (*)(CSOUND *, void *)) ep->iopadr,
                                  (int (*)(CSOUND *, void *)) ep->kopadr,
                                  (int (*)(CSOUND *, void *)) ep->aopadr);
    }

    return err;
}

/**
 * Called by Csound to de-initialize the opcode
 * just before destroying it.
 */

PUBLIC int csoundModuleDestroy(CSOUND *csound)
{
    fluidSynthGlobals *pp;

    pp = (fluidSynthGlobals *) csound->QueryGlobalVariable(csound,
                                                           "fluid.engines");
    if (pp != NULL && pp->cnt) {
      size_t  i;
      csound->Message(csound, "Cleaning up Fluid Engines - Found: %d\n",
                      (int) pp->cnt);
      for (i = (size_t) 0; i < pp->cnt; i++) {
        fluid_settings_t  *tmp;
        tmp = fluid_synth_get_settings(pp->fluid_engines[i]);
        delete_fluid_synth(pp->fluid_engines[i]);
        pp->fluid_engines[i] = (fluid_synth_t *) NULL;
        delete_fluid_settings(tmp);
      }
    }

    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}

