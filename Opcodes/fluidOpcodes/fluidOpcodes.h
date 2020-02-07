/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FLUIDOPCODE_H
#define FLUIDOPCODE_H

#include "csdl.h"
#include <fluidsynth.h>

typedef struct {
  OPDS    h;
  /* OUTPUTS */
  MYFLT   *iEngineNum;
  MYFLT   *iReverbEnabled;
  MYFLT   *iChorusEnabled;
  MYFLT   *iNumChannels;
  MYFLT   *iPolyphony;
} FLUIDENGINE;

typedef struct {
  OPDS    h;
  /* INPUTS */
  MYFLT   *iEngineNumber, *iChannelNumber, *iInterpMethod;
} FLUID_SET_INTERP_METHOD;

typedef struct {
  OPDS    h;
  /* OUTPUTS */
  MYFLT   *iInstrumentNumber;
  /* INPUTS */
  MYFLT   *filename, *iEngineNum, *iListPresets;
} FLUIDLOAD;

typedef struct {
  OPDS    h;
  /* INPUTS */
  MYFLT   *iEngineNumber, *iChannelNumber, *iInstrumentNumber, *iBankNumber;
  MYFLT   *iPresetNumber;
} FLUID_PROGRAM_SELECT;

typedef struct {
  OPDS    h;
  /* INPUTS */
  MYFLT   *iEngineNumber, *iChannelNumber, *iControllerNumber, *kVal;
  int32_t     priorMidiValue;
  fluid_synth_t *fluidEngine;
} FLUID_CC;

typedef struct {
  OPDS    h;
  /* INPUTS */
  MYFLT   *iEngineNumber, *iChannelNumber, *iMidiKeyNumber, *iVelocity;
  int32_t     initDone, iChn, iKey;
  fluid_synth_t *fluidEngine;
} FLUID_NOTE;

typedef struct {
  OPDS    h;
  MYFLT   *aLeftOut, *aRightOut;
  MYFLT   *iEngineNum;
  fluid_synth_t *fluidEngine;
} FLUIDOUT;

typedef struct {
  OPDS    h;
  MYFLT   *aLeftOut, *aRightOut;
  void    *fluidGlobals;
} FLUIDALLOUT;

typedef struct {
  OPDS    h;
  /* Inputs. */
  MYFLT   *iFluidEngine;
  MYFLT   *kMidiStatus;
  MYFLT   *kMidiChannel;
  MYFLT   *kMidiData1;
  MYFLT   *kMidiData2;
  /* No outputs. */
  /* Internal state. */
  int32_t     priorMidiStatus;
  int32_t     priorMidiChannel;
  int32_t     priorMidiData1;
  int32_t     priorMidiData2;
  fluid_synth_t *fluidEngine;
} FLUIDCONTROL;

#endif

